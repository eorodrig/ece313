#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

/*  This file presents the "standard" application layer which is used
    (by default) if an alternative application layer is not requested
    with the  cnet -A  option.

    Functions in this file are called indirectly through their interfaces
    in applicationlayer.c (functions here have std_ prepended to their name).
    Wherever possible, the interface functions perform all error
    checking of the arguments, for example ensuring that a given node
    is an NT_HOST or an NT_MOBILE (not a NT_ROUTER or NT_ACCESSPOINT).

    This file should be used as a guide to writing any new application
    layers, such as those paying more attention to message size distributions
    and source/destination pairings. In particular, the following functions
    are required:

	void	std_init_application_layer(bool Qflag, int Sflag)
	int	std_application_bounds(int *minmsg, int *maxmsg)
	int	std_reboot_application_layer(CnetTime *poll_next);
	int	std_poll_application(CnetTime *poll_next)

	int	std_CNET_read_application(CnetAddr *dest, void *m,size_t *len);
	int	std_CNET_write_application(char *msg, int *len);

	int	std_CNET_enable_application (CnetAddr destaddr);
	int	std_CNET_disable_application(CnetAddr destaddr);

    In general, functions return 0 on success and -1 (and set cnet_errno)
    on failure. std_poll_application() returns true or false.

    This application layer uses mt19937_int31() to provide a repeatable
    stream of random numbers and, hence, a repeatable stream of messages.
 */

#define	APPLICATION_SEED	2495691
static	MT			*mt;

static	bool	Qflag		= false;

typedef	struct {
    int		to;	// nodenumber (not address)
    int		from;	// nodenumber (not address)
    int		checksum;
    size_t	len;
    int		session;
    int		seqno;
    CnetTime	timecreated;
    char	msg[1];
} MESSAGE;

#if	!defined(offsetof)
#define offsetof(type, f)	\
		((size_t)((char *)&((type *)0)->f - (char *)(type *)0))
#endif

#define	MSG_HEADER_SIZE		(offsetof(MESSAGE, msg))
#define	MSG_SIZE(m)		(MSG_HEADER_SIZE + m->len)

typedef struct {
    CnetAddr	msgdest;
    size_t	msglength;
    MESSAGE	*MSG;
    int		session;
    int		*seqsent;
    int		*seqrecv;
    bool	*enabled;
    int		Nenabled;
    CnetTime	poll_next;
} APPLICATION;


static	APPLICATION	**APPL;
static	APPLICATION	*ap;
static	char		chargen[128], *endchargen;


// ------------------------ Application Layer -----------------------------


int std_application_bounds(int *minmsg, int *maxmsg)
{
    *minmsg	= sizeof(MESSAGE);
    *maxmsg	= MAX_MESSAGE_SIZE;
    return(0);
}

int std_init_application_layer(bool _Qflag, int Sflag)
{
    Qflag	= _Qflag;
    APPL = calloc((unsigned)_NNODES, sizeof(APPLICATION *));
    CHECKALLOC(APPL);

    for(int n=0 ; n<_NNODES ; ++n)
	if(NODE_HAS_AL(n)) {
	    ap		=
	    APPL[n]	= malloc(sizeof(APPLICATION));
	    CHECKALLOC(ap);
	    ap->MSG	= NULL;
	    ap->session	= (n*n);
	    ap->seqsent	= malloc((unsigned)_NNODES * sizeof(int));
	    CHECKALLOC(ap->seqsent);
	    ap->seqrecv	= malloc((unsigned)_NNODES * sizeof(int));
	    CHECKALLOC(ap->seqrecv);
	    ap->enabled	= malloc((unsigned)_NNODES * sizeof(bool));
	    CHECKALLOC(ap->enabled);
	}

    endchargen = chargen;
    for(int n=040 ; n<=0176 ; ++n)
	*endchargen++ = (char)n;

    mt	= mt19937_init((unsigned long)(APPLICATION_SEED+Sflag));

    return(0);
}

// ----------------------------------------------------------------------


/*  ALL OF THE FOLLOWING FUNCTIONS ARE CALLED BY, OR ON BEHALF OF, AN
    INDIVIDUAL NODE, IMPLICITLY INDICATED BY 'THISNODE'.
 */

static CnetTime calc_poll_next(void)
{
    CnetTime mean = WHICH(NP->nattr.messagerate,
			  DEF_NODEATTR.messagerate);
    return poisson_usecs(mean, mt);
}


int std_reboot_application_layer(CnetTime *poll_next)
{
    ap			= APPL[THISNODE];
    if(ap->MSG)
	FREE(ap->MSG);
    ap->msgdest		= ALLNODES;
    ap->msglength	= UNKNOWN;
    ap->session++ ;
    ap->Nenabled	= 0;

    for(int n=0 ; n<_NNODES ; ++n) {
	ap->seqsent[n]	=
	ap->seqrecv[n]	= 0;
	ap->enabled[n]	= false;

	if(NODE_HAS_AL(n)) {
	    APPL[n]->seqsent[THISNODE]	= 0;
	    APPL[n]->seqrecv[THISNODE]	= 0;
	}
    }
    *poll_next		=
    ap->poll_next	= (CnetTime)0;

    return(0);
}

int std_poll_application(CnetTime *poll_next)
{
    int		minsize, maxsize;
    size_t 	len;

    ap		= APPL[THISNODE];

    if(MICROSECONDS < ap->poll_next) {		// time for new msg?
	*poll_next = ap->poll_next - MICROSECONDS;
	return(false);				// not yet anyway
    }

    *poll_next		= calc_poll_next();
    ap->poll_next	= MICROSECONDS + *poll_next;

    if(ap->Nenabled == 0)			// not really want message?
	return(false);

    if(ap->MSG != NULL)				// already one ready?
	return(true);

//  CHOOSE A RANDOM DESTINATION HOST (not a ROUTER)
    ap->msgdest = mt19937_int31(mt)%_NNODES;
    for(;;) {
	if(ap->enabled[ap->msgdest])
	    break;
	if(++ap->msgdest == (unsigned int)_NNODES)
	    ap->msgdest = 0;
    }

//  FILL MESSAGE WITH SOME FICTICIOUS DATA
    minsize	= WHICH(NP->nattr.minmessagesize, DEF_NODEATTR.minmessagesize);
    maxsize	= WHICH(NP->nattr.maxmessagesize, DEF_NODEATTR.maxmessagesize);
    len		= (minsize - MSG_HEADER_SIZE) +
		  	  (mt19937_int31(mt) % (maxsize - minsize + 1));

    ap->MSG		= malloc((unsigned)(MSG_HEADER_SIZE + len));
    CHECKALLOC(ap->MSG);
    ap->MSG->len	= len;

/*  THIS IS A SIMPLE CHARACTER GENERATOR WHICH BORROWS HEAVILY FROM
	~src/bsd/etc/inetd.c:chargen_stream()
 */
    {
	static char	*cs	= chargen;
	char		*mp	= ap->MSG->msg,
			*cp;
	int		i;

	if(cs++ >= endchargen)
	    cs	= chargen;
	cp	= cs;
	while(len > 0) {
	    i	= (len < endchargen-cp) ? len : endchargen-cp;
	    memcpy(mp, cp, (unsigned)i);
	    mp	+= i;
	    len -= i;
	    if((cp += i) >= endchargen)
		cp	= chargen;
	}
    }

    ap->MSG->to			= ap->msgdest;
    ap->MSG->from		= THISNODE;
    ap->MSG->session		= ap->session;
    ap->MSG->seqno		= ap->seqsent[ap->msgdest]++;
    ap->MSG->timecreated	= MICROSECONDS;
    ap->msglength		= MSG_SIZE(ap->MSG);
    ap->MSG->checksum		= 0;
    ap->MSG->checksum		= CNET_IP_checksum(
				    (unsigned short *)ap->MSG, ap->msglength);
    return(true);
}


int std_CNET_read_application(CnetAddr *destaddr, void *msg, size_t *len)
{
    ap	= APPL[THISNODE];
    if(ap->MSG == NULL) {				// nothing pending!
	ERROR(ER_NOTREADY);
	goto bad;
    }

    if(*len < ap->msglength) {
	*len		= ap->msglength;
	ERROR(ER_BADSIZE);
	goto bad;
    }

    *destaddr	= NODES[ap->msgdest].nattr.address;	// not nodenumber!
    memcpy(msg, ap->MSG, ap->msglength);
    *len	= ap->msglength;

    NP->nodestats.msgs_generated++;
    NP->nodestats.bytes_generated	+= *len;
#if	defined(USE_TCLTK)
    NP->stats_changed			= true;
#endif
    gattr.nodestats.msgs_generated++;
    gattr.nodestats.bytes_generated	+= *len;

    ap->msgdest		= ALLNODES;		// only give it once!
    ap->msglength	= UNKNOWN;
    if(ap->MSG)
	FREE(ap->MSG);

    return 0;

bad:
    *destaddr		= ALLNODES;
    ((char *)msg)[0]	= '\0';
    *len		= '0';
    return (-1);
}


int std_CNET_write_application(char *msg, size_t *len)
{
    MESSAGE	header, *r;
    CnetTime	deliverytime;
    int		aligned, got_chk, true_chk;

    ap	= APPL[THISNODE];

//  DETERMINE IF THE PROVIDED MESSAGE IS ON A LONG-INT BOUNDARY
    aligned = (((long)msg % sizeof(long)) == 0);

    if(aligned)
	r	= (MESSAGE *)msg;
    else {						// align it
	r	= &header;
	memcpy(r, msg, (unsigned)MSG_HEADER_SIZE);
    }

    if(*len < MSG_HEADER_SIZE || *len > MAX_MESSAGE_SIZE) {
	ERROR(ER_BADARG);
	goto bad;
    }
    if(r->len <= 0 || r->len > MAX_MESSAGE_SIZE) {
	ERROR(ER_CORRUPTFRAME);
	goto bad;
    }

//  NEXT WE CALCULATE THE CHECKSUM - CAREFUL IF MSG IS NOT ON AN INT BOUNDARY
    if(aligned) {
	got_chk		= r->checksum;
	r->checksum	= 0;
    }
    else {
	memcpy(&got_chk, &(((MESSAGE *)msg)->checksum), sizeof(int));
	memset(&(((MESSAGE *)msg)->checksum), 0, sizeof(int));
    }
    true_chk	= CNET_IP_checksum((unsigned short *)msg, MSG_SIZE(r));

//  NOW, PUT THE CHECKSUM BACK IN ITS CORRECT POSITION
    if(aligned)
	r->checksum	= got_chk;
    else
	memcpy(&(((MESSAGE *)msg)->checksum), &got_chk, sizeof(int));

    if(got_chk != true_chk) {
	ERROR(ER_CORRUPTFRAME);
	goto bad;
    }
    if(*len != (int)MSG_SIZE(r)) {
	ERROR(ER_BADSIZE);
	goto bad;
    }
    if(r->to != THISNODE) {
	ERROR(ER_NOTFORME);
	goto bad;
    }
    if(r->from < 0 || r->from >= _NNODES || r->from == THISNODE) {
	ERROR(ER_BADSENDER);
	goto bad;
    }

/*  Notice that I'm not treating the 'wrong session' error as an error
    which affects the statistics. Now, even with nodes crashing and rebooting,
    it should still be possible to keep that 100% delivery performance.
    (Students should implement their own Session Layer to handle their
     own session numbers, which are negotiated when a host reboots).
 */
    if(r->session != APPL[r->from]->session) {
	r->checksum	= got_chk;
	ERROR(ER_BADSESSION);
	return(-1);
    }

    if(Qflag == false) {
	if(r->seqno < ap->seqrecv[r->from]) {
	    ERROR(ER_DUPLICATEMSG);
	    goto bad;
	}
	if(r->seqno > ap->seqrecv[r->from]) {
	    ERROR(ER_MISSINGMSG);
	    goto bad;
	}
    }
    ap->seqrecv[r->from]	= r->seqno + 1;

    deliverytime			= MICROSECONDS-r->timecreated;
    NP->nodestats.msgs_received++;
    NP->nodestats.bytes_received	+= *len;
#if	defined(USE_TCLTK)
    NP->stats_changed			= true;
#endif
    NP->hiddenstats.msgs_deliverytime	+= deliverytime;

    gattr.nodestats.msgs_received++;
    gattr.nodestats.bytes_received	+= *len;
    gattr.hiddenstats.msgs_deliverytime	+= deliverytime;

    return 0;

bad:
    NP->hiddenstats.msgs_write_errors++;
    gattr.hiddenstats.msgs_write_errors++;
    return (-1);
}


// --------------    Enable/Disable Application Layer -------------------


int std_CNET_enable_application(CnetAddr destaddr)
{
    int		dest;
    int		oldNenabled;

    if(destaddr == NP->nattr.address) {
	ERROR(ER_BADARG);
	return (-1);
    }

    ap		= APPL[THISNODE];
    oldNenabled	= ap->Nenabled;

    if(destaddr == ALLNODES) {
	ap->Nenabled		= 0;
	for(dest=0 ; dest<_NNODES ; ++dest)
	    if(dest != THISNODE && NODE_HAS_AL(dest)) {
		ap->enabled[dest]	= true;
		ap->Nenabled++;
	    }
	    else
		ap->enabled[dest]	= false;
    }
    else {
	oldNenabled		= ap->Nenabled;
	for(dest=0 ; dest<_NNODES ; ++dest)
	    if(NODES[dest].nattr.address == destaddr) {
		if(dest != THISNODE && NODE_HAS_AL(dest)) {
		    if(ap->enabled[dest] == false)
			++ap->Nenabled;
		    ap->enabled[dest] = true;
		}
		break;
	    }
    }

    if(oldNenabled == 0 && ap->Nenabled > 0) {
	CnetTime	next;

	next		= calc_poll_next();
	ap->poll_next = MICROSECONDS + next;
	newevent(EV_APPLICATIONREADY, THISNODE, next, NULLTIMER, (CnetData)0);
    }
    return 0;
}

int std_CNET_disable_application(CnetAddr destaddr)
{
    extern void	unschedule_application(int n);

    int	dest;

    if(destaddr == NP->nattr.address) {
	ERROR(ER_BADARG);
	return (-1);
    }

    ap	= APPL[THISNODE];

    if(destaddr == ALLNODES) {
	for(dest=0 ; dest<_NNODES ; ++dest)
	    ap->enabled[dest]	= false;
	ap->Nenabled		= 0;
    }
    else
	for(dest=0 ; dest<_NNODES ; ++dest)
	    if(NODES[dest].nattr.address == destaddr) {
		if(ap->enabled[dest] == true) {
		    ap->enabled[dest] = false;
		    --ap->Nenabled;
		}
		break;
	    }

    if(ap->Nenabled == 0)
	unschedule_application(THISNODE);
    return(0);
}
