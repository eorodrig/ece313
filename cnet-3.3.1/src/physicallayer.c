#include "cnetprivate.h"
#include "physicallayer.h"
#include <dlfcn.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	PHYSICAL_SEED	64882533
static	MT		*mt;

static	bool		Eflag	= false;
static	bool		Nflag	= false;

static FRAMEARRIVAL	*justarrived	= NULL;

// ------------------------- PHYSICAL LAYER -----------------------------

void init_physical_layer(bool _Eflag, bool _Nflag, int Sflag)
{
    int maxlinks = UNKNOWN;

    Eflag	= _Eflag;
    Nflag	= _Nflag;
    mt		= mt19937_init((unsigned)(PHYSICAL_SEED+Sflag));

    for(int n=0 ; n<_NNODES ; ++n)
	if(maxlinks < NODES[n].nnics)
	    maxlinks = NODES[n].nnics;

    linkinfo	= calloc((unsigned)(maxlinks+1), sizeof(CnetLinkInfo));
    CHECKALLOC(linkinfo);

#if	CNET_PROVIDES_WANS
    extern void	init_wans(void);
    init_wans();
#endif

#if	CNET_PROVIDES_LANS
    extern void	init_lans(void);
    init_lans();
#endif

#if	CNET_PROVIDES_WLANS
    extern void	init_wlans(void);
    init_wlans();
#endif
}


// --------------------- UNRELIABLE PHYSICAL LAYER -------------------------

bool prepare4framearrival(FRAMEARRIVAL *newarrival)
{
    NICATTR *destnic = &NODES[newarrival->destnode].nics[newarrival->destlink];

    if(destnic->up == false)
	return false;

//  FREE ANY PREVIOUS, BUT UNREAD, FRAME
    if(justarrived) {
	FREE(justarrived->frame);
	FREE(justarrived);
    }
    justarrived	= newarrival;

    switch (justarrived->linktype) {
    case LT_LOOPBACK :
	break;

    case LT_WAN :
	break;
    case LT_LAN :
	break;

    case LT_WLAN :
#if	CNET_PROVIDES_WLANS
    {
//  EXTRA PROCESSING IF THIS IS A WLAN LINK
	WLAN	*destwlan	= &WLANS[justarrived->linkindex];

	if(destwlan->state == WLAN_SLEEP)
	    return false;
	else {
//  TODO: CONSUME RECEIVING NODE's BATTERY POWER HERE....

	    destwlan->rx_signal	= justarrived->rx_signal;
	    destwlan->rx_angle	= justarrived->rx_angle;

#if	defined(USE_TCLTK)
//  SAVE WLAN HISTORY FOR SIGNAL-STRENTH BARS
	    if(Wflag) {
		NODE	*np = &NODES[newarrival->destnode];

		np->wlan_rx_history[np->wlan_index_history]	=
				justarrived->rx_signal;
		np->wlan_index_history				=
				(np->wlan_index_history+1) % WLAN_RX_HISTORY;
	    }
#endif
	}
      }
#endif
	break;
    }
    return true;
}

bool prepare4framecollision(FRAMEARRIVAL *newarrival)
{
    NICATTR *destnic = &NODES[newarrival->destnode].nics[newarrival->destlink];
    bool rtn = false;

    if(destnic->up) {
	switch (newarrival->linktype) {
	    case LT_LOOPBACK :
		break;

	    case LT_WAN :
		break;

	    case LT_LAN :
		rtn	= true;
		break;

	    case LT_WLAN :
#if	CNET_PROVIDES_WLANS
	    {
//  EXTRA PROCESSING IF THIS IS A WLAN LINK
		int	 linkindex	= newarrival->linkindex;
		WLAN	*destwlan	= &WLANS[linkindex];

		if(destwlan->state != WLAN_SLEEP) {
		    gattr.linkstats.rx_frames_collisions++;
		    destnic->linkstats.rx_frames_collisions++;
#if	defined(USE_TCLTK)
		    destnic->stats_changed	= true;
#endif
		    rtn	= true;
		}
	    }
#endif
		break;
	}
    }
    FREE(newarrival->frame);
    FREE(newarrival);

    return rtn;
}

// -------------------------------------------------------------------------

static int hidden_read(int *thislink, char *frame, size_t *len)
{
    NICATTR	*nic;
    int		result	= 0;

/*  ENSURE THAT THIS NODE WAS ACTUALLY CALLED VIA AN EVENT,
    AND THAT STUDENTS ARE NOT JUST POLLING THE PHYSICAL LAYER */
    if(justarrived == NULL) {
	if(thislink)
	    *thislink	= UNKNOWN;
	*frame		= '\0';
	*len		= 0;
	ERROR(ER_NOTREADY);
	result		= -1;
    }
//  ENSURE THAT STUDENTS HAVE PROVIDED ENOUGH SPACE FOR NEXT FRAME
    else if(*len < justarrived->len) {
	if(thislink)
	    *thislink	= UNKNOWN;
	*frame		= '\0';
	*len		= justarrived->len;
	ERROR(ER_BADSIZE);
	result		= -1;
    }
    else {
	if(thislink)
	    *thislink	= justarrived->destlink;
	memcpy(frame, justarrived->frame, justarrived->len);
	*len		= justarrived->len;

	NP->nframes--;
	nic		= &NP->nics[justarrived->destlink];

	if(justarrived->corrupted) {
	    nic->linkstats.rx_frames_corrupted++;
	    gattr.linkstats.rx_frames_corrupted++;
	    if(Eflag) {
		ERROR(ER_CORRUPTFRAME);
		result		= -1;
	    }
	}

//  UPDATE THIS RECEIVING NIC's STATISTICS
	nic->linkstats.rx_frames++;
	nic->linkstats.rx_bytes		+= *len;
#if	defined(USE_TCLTK)
	nic->stats_changed	= true;
#endif

//  UPDATE GLOBAL LINK RECEIVING STATISTICS
	gattr.linkstats.rx_frames++;
	gattr.linkstats.rx_bytes	+= *len;
    }

    if(justarrived) {
	FREE(justarrived->frame);
	FREE(justarrived);
    }
    return result;
}


// ----------------------------------------------------------------------


//  DETERMINE IF A FRAME WILL BE LOST
int lose_frame(NICATTR *nic, size_t len)
{
    int	prob	= 0;

#if	CNET_PROVIDES_WANS
    prob = WHICH(nic->probframeloss, DEFAULTWAN.probframeloss);
#endif
    if(prob > 0 && (mt19937_int31(mt) % (1<<prob)) == 0) {
	NP->hiddenstats.tx_frames_lost++;
	gattr.hiddenstats.tx_frames_lost++;
	return true;
    }
    return false;
}

// DETERMINE IF A FRAME WILL BE CORRUPTED - AND CORRUPT IT
int corrupt_frame(NICATTR *nic, char *frame, size_t len)
{
    int	prob	= 0;

#if	CNET_PROVIDES_WANS
    prob = WHICH(nic->probframecorrupt, DEFAULTWAN.probframecorrupt);
#endif
    if(prob > 0 && (mt19937_int31(mt) % (1<<prob)) == 0) {

#if	MAY_TRUNCATE_FRAMES
//  CORRUPT FRAME BY REDUCING ITS LENGTH
	if((mt19937_int31(mt) % 8) == 0)
	    len	= (int)(len * 0.9);
	else
#endif
//  CORRUPT FRAME BY COMPLEMENTING TWO OF ITS BYTES
	{
	    int	 offset	= mt19937_int31(mt) % (len - 2);
	    char *s	= &frame[offset];

	    *s		= ~(*s);	// detectable by all checksums
	    ++s;
	    *s		= ~(*s);
	}
	return true;
    }
    return false;
}

// ----------------------------------------------------------------------

static int hidden_write(int thislink, char *frame, size_t *len, bool unreliable)
{
    NICATTR	*nic;

//  ENSURE THAT THE INDICATE LINK NUMBER IS VALID
    if(thislink < 0 || thislink > NP->nnics) {
	ERROR(ER_BADLINK);
	return(-1);
    }

//  ENSURE THAT THE FRAME AND LENGTH ARE VALID
    if(frame == NULL || len == NULL || *len == 0) {
	ERROR(ER_BADARG);
	return(-1);
    }

//  ENSURE THAT WRITING NODE IS NOT REBOOTING
    if(NP->runstate == STATE_REBOOTING) {
	ERROR(ER_NOTREADY);
	return(-1);
    }

    nic	= &NP->nics[thislink];

//  ENSURE THAT THIS LINK/NIC IS UP
    if(nic->up == false) {
	ERROR(ER_LINKDOWN);
	return(-1);
    }

//  IF THIS LINK DOES NOT BUFFER FRAMES, ENSURE THAT IT'S NOT BUSY
    if(nic->buffered == false && MICROSECONDS <= nic->tx_until) {
	ERROR(ER_TOOBUSY);
	return(-1);
    }

//  ENSURE THAT WE'RE NOT WRITING MORE THAN THE LINK'S CAPACITY
    if(*len > nic->mtu) {
	ERROR(ER_BADSIZE);
	return(-1);
    }

    switch (nic->linktype) {
    case LT_LOOPBACK : {
//  WRITING TO THE LOOPBACK LINK - ALWAYS DELIVER RELIABLY
	FRAMEARRIVAL		*newf;

	newf			= NEW(FRAMEARRIVAL);
	CHECKALLOC(newf);
	newf->destnode		= THISNODE;
	newf->destlink		= LOOPBACK_LINK;
	newf->linktype		= LT_LOOPBACK;
	newf->arrives		= DELAY_WRITE_LOOPBACK;
	newf->len		= *len;
	newf->frame		= malloc(*len);
	CHECKALLOC(newf->frame);
	memcpy(newf->frame, frame, *len);
	newf->corrupted		= false;

	newevent(EV_PHYSICALREADY, THISNODE,
			DELAY_WRITE_LOOPBACK, NULLTIMER, (CnetData)newf);
	break;
    }

    case LT_WAN :
#if	CNET_PROVIDES_WANS
	{
	    extern int	write_wan (int, char *, size_t, bool);
	    return write_wan(thislink, frame, *len, unreliable);
	}
#endif
	break;

    case LT_LAN:
#if	CNET_PROVIDES_LANS
	{
	    extern int	write_lan (int, char *, size_t, bool);
	    return write_lan(thislink, frame, *len, unreliable);
	}
#endif
	break;

    case LT_WLAN :
#if	CNET_PROVIDES_WLANS
	{
	    extern int	write_wlan(int, char *, size_t, bool);
	    return write_wlan(thislink, frame, *len, unreliable);
	}
#endif
	break;
    }
    return 0;
}


// ----------------------------------------------------------------------

static int hidden_write_direct(CnetAddr destaddr,char *frame,size_t *len)
{
    NICATTR		*srcnic	= &NP->nics[1];
    FRAMEARRIVAL	*newf;
    int			destnode;

//  UPDATE THIS TRANSMITTING NIC's STATISTICS
    srcnic->linkstats.tx_frames++;
    srcnic->linkstats.tx_bytes	+= *len;
#if	defined(USE_TCLTK)
    srcnic->stats_changed	= true;
#endif

//  UPDATE GLOBAL LINK TRANSMISSION STATISTICS
    gattr.linkstats.tx_frames++;
    gattr.linkstats.tx_bytes	+= *len;

    for(destnode=0 ; destnode<_NNODES ; ++destnode)
	if(NODES[destnode].nattr.address == destaddr)
	    break;

    if(destnode == _NNODES) {
	if(Nflag) {
	    ERROR(ER_BADARG);
	    return(-1);
	}
	return 0;
    }
    if(NODES[destnode].runstate != STATE_RUNNING)
	return 0;

    newf		= NEW(FRAMEARRIVAL);
    CHECKALLOC(newf);
    newf->destnode	= destnode;
    newf->destlink	= 1;
    newf->linktype	= LT_WAN;
    newf->arrives	= DELAY_WRITE_DIRECT;
    newf->len		= *len;
    newf->frame		= malloc(newf->len);
    CHECKALLOC(newf->frame);
    memcpy(newf->frame, frame, newf->len);
    newf->corrupted	= false;

    newevent(EV_PHYSICALREADY, destnode,
			DELAY_WRITE_DIRECT, NULLTIMER, (CnetData)newf);
    return 0;
}

int CNET_write_direct(CnetAddr destaddr, void *frame, size_t *len)
{
    int  result		= -1;
    int	 lengiven	= 0;

    if(frame == NULL || len == NULL || *len == 0)
	ERROR(ER_BADARG);
    else if(destaddr == NP->nattr.address)
	ERROR(ER_BADNODE);
    else if(NP->runstate == STATE_REBOOTING)
	ERROR(ER_NOTREADY);
    else {
	lengiven	= *len;
	if(destaddr == ALLNODES) {
	    for(int n=0 ; n<_NNODES ; n++)
		if(n != THISNODE && NODES[n].runstate == STATE_RUNNING) {
		    result	= hidden_write_direct(destaddr, frame, len);
		    if(result != 0)
			break;
		}
	}
	else
	    result	= hidden_write_direct(destaddr, frame, len);
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(destaddr=%lu,%s,*len=%ld) = 0 (*len=%ld)\n",
			__func__,
			destaddr, find_trace_name(frame),
			lengiven, *len);
	else {
	    char	buf[64];

	    if(len == NULL)
		strcpy(buf, "NULL");
	    else
		sprintf(buf, "*len=%zd", *len);

	    TRACE(1, "\t%s(destaddr=%lu,%s,%s) = -1 %s\n",
			__func__,
			destaddr, find_trace_name(frame), buf,
			cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

// ------------------- PHYSICAL LAYER INTERFACE -------------------------


int CNET_read_physical(int *thislink, void *frame, size_t *len)
{
    int result		= -1;
    int	lengiven	= 0;

    if(frame == NULL || len == NULL || *len == 0)
	ERROR(ER_BADARG);
    else {
	lengiven	= *len;
	result	= hidden_read(thislink, frame, len);
    }

    if(gattr.trace_events) {
	char	buf[64];

	if(result == 0) {
	    if(thislink == NULL)
		buf[0]	= '\0';
	    else
		sprintf(buf, "link=%i,", *thislink);

	    TRACE(0, "\t%s(%s,%s,*len=%ld) = 0 (%s*len=%ld)\n",
			__func__,
			find_trace_name(thislink), find_trace_name(frame),
			lengiven, buf, *len);
	}
	else {
	    if(len == NULL)
		strcpy(buf, "NULL");
	    else
		sprintf(buf, "*len=%zd", *len);

	    TRACE(1, "\t%s(%s,%s,%s) = -1 %s\n",
			__func__,
			find_trace_name(thislink), find_trace_name(frame),
			len, cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

static int cwp(const char *fn, int thislink, char *frame, size_t *len, bool cor)
{
    int	result		= -1;
    int	lengiven	= 0;

    if(len)
	lengiven	= *len;
    result	= hidden_write(thislink, frame, len, cor);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,%s,*len=%ld) = 0 (*len=%ld)\n",
			fn,
			thislink, find_trace_name(frame),
			lengiven, *len);
	else {
	    char	buf[64];

	    if(len == NULL)
		strcpy(buf, "NULL");
	    else
		sprintf(buf, "*len=%zd", *len);

	    TRACE(1, "\t%s(link=%i,%s,%s) = -1 %s\n",
			fn,
			thislink, find_trace_name(frame), buf,
			cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

int CNET_write_physical(int thislink, void *frame, size_t *len)
{
    return cwp(__func__, thislink, frame, len, true);
}

int CNET_write_physical_reliable(int thislink, void *frame, size_t *len)
{
    return cwp(__func__, thislink, frame, len, false);
}

// ------------------------------------------------------------------------

int CNET_carrier_sense(int thislink)
{
    NICATTR	*nic;
    int		result	= -1;

    if(thislink < 0 || thislink > NP->nnics) {
	ERROR(ER_BADLINK);
	goto done;
    }
    if(NP->runstate == STATE_REBOOTING) {
	ERROR(ER_NOTREADY);
	goto done;
    }

    nic		= &NP->nics[thislink];
    if(nic->up == false) {
	ERROR(ER_LINKDOWN);
	goto done;
    }

//  THE DEFINITION OF carrier_sense DEPENDS ON OUR LINK TYPE
    switch (nic->linktype) {
    case LT_LAN :
    case LT_WLAN :
	result =
	  (MICROSECONDS <= nic->tx_until)||(MICROSECONDS <= nic->rx_until)?1:0;
	break;

    default :
	ERROR(ER_NOTSUPPORTED);
	break;
    }

done:
    if(gattr.trace_events) {
	if(result >= 0)
	    TRACE(0, "\t%s(link=%i) = %i\n", __func__, thislink, result);
	else
	    TRACE(1, "\t%s(link=%i) = -1 %s\n",
			__func__, thislink, cnet_errname[(int)cnet_errno]);
    }
    return(result);
}


// ------------------------------------------------------------------------

int CNET_get_linkstats(int thislink, CnetLinkStats *stats)
{
    int		result	= -1;

    if(thislink < 0 || thislink > NP->nnics)
	ERROR(ER_BADLINK);
    else if(stats == NULL)
	ERROR(ER_BADARG);
    else {
	NICATTR	*nic	= &NP->nics[thislink];

	memcpy(stats, &nic->linkstats, sizeof(CnetLinkStats));
	result	= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,%s) = 0\n",
			__func__,
			thislink, find_trace_name(stats));
	else
	    TRACE(1, "\t%s(link=%i,%s) = -1 %s\n",
			__func__,
			thislink, find_trace_name(stats),
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}


// ------------------------------------------------------------------------

static bool allbytes(CnetNICaddr nicaddr, int byte)
{
    for(int n=0 ; n<sizeof(CnetNICaddr) ; ++n)
	if(nicaddr[n] != (unsigned char)byte)
	    return false;
    return true;
}

int CNET_set_nicaddr(int thislink, CnetNICaddr nicaddr)
{
    int		result	= -1;
    NICATTR	*nic;

//  ENSURE THAT A VALID LINK HAS BEEN PROVIDED
    if(thislink < 1 || thislink > NP->nnics)
	ERROR(ER_BADLINK);

//  ENSURE THAT NEITHER THE ZERO- NOR BROADCAST ADDRESS HAS BEEN REQUESTED
    else if(allbytes(nicaddr,0) == true || allbytes(nicaddr,255) == true)
	ERROR(ER_BADARG);

    else {
	nic	= &NP->nics[thislink];

//  ENSURE THAT WE ARE TRYING TO MODIFY A LAN OR WLAN LINK
	if(nic->linktype != LT_LAN && nic->linktype != LT_WLAN)
	    ERROR(ER_NOTSUPPORTED);
	else {

//  CHANGE THE NICADDR ON NICATTR OF THISNODE
	    memcpy(nic->nicaddr, nicaddr, sizeof(CnetNICaddr));

//  CHANGE THE NICADDR OF THE (SWAPPED IN) LINKINFO
	    memcpy(linkinfo[thislink].nicaddr, nicaddr, sizeof(CnetNICaddr));
	    result	= 0;
	}
    }

    if(gattr.trace_events) {
	if(result == 0) {
	    char	buf[32];

	    CNET_format_nicaddr(buf, nicaddr);
	    TRACE(0, "\t%s(link=%i,%s) = 0 (nicaddr=%s)\n",
			__func__, thislink,find_trace_name(nicaddr), buf);
	}
	else
	    TRACE(1, "\t%s(link=%i,%s) = -1 %s\n",
			__func__, thislink, cnet_errname[(int)cnet_errno]);
    }
    return result;
}
