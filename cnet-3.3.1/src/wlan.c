#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_WLANS

#include "physicallayer.h"

WLAN		*WLANS		= NULL;

NICATTR		DEFAULTWLAN = {
    .linktype		= LT_WLAN,
    .up			= true,
    .bandwidth		= WLAN_BANDWIDTH,
    .propagation	= WLAN_PROPAGATION,
    .buffered		= true,
    .mtu		= WLAN_MTU,

    .costperbyte	= 0,
    .costperframe	= 0,
    .probframecorrupt	= 0,
    .probframeloss	= 0,

    .linkmtbf		= (CnetTime)0,
    .linkmttr		= (CnetTime)0,
    .tx_until		= (CnetTime)0,
    .rx_until		= (CnetTime)0
};

#define	FOREACH_WLAN	for(w=0, wlan=WLANS ; w<gattr.nwlans ; ++w, ++wlan)
#define	FOREACH_RX_HISTORY	for(int h=0 ; h<WLAN_RX_HISTORY ; ++h)

// ----------------------------------------------------------------------

#if	defined(USE_TCLTK)
TCLTK_COMMAND(toggle_drawwlans)
{
    gattr.drawwlans	= !gattr.drawwlans;
    return TCL_OK;
}

void flush_wlan_history(NODE *np)
{
    if(np->displayed) {
	double	sum	= 0.0;
	double	avg	= 0.0;
	int	nbars	= 0;

//  CALCULATE AVERAGE SIGNAL STRENGTH
	FOREACH_RX_HISTORY
	    sum += np->wlan_rx_history[h];
	avg	= sum / WLAN_RX_HISTORY;

//  NEED A BETTER SCHEME THAN THIS!
	     if(avg > WLAN_5_BARS)	nbars = 5;
	else if(avg > WLAN_4_BARS)	nbars = 4;
	else if(avg > WLAN_3_BARS)	nbars = 3;
	else if(avg > WLAN_2_BARS)	nbars = 2;
	else if(avg > WLAN_1_BAR )	nbars = 1;

	if(np->wlan_prev_nbars != nbars) {
	    TCLTK("set_wifi_bars %i %i", np-NODES, nbars);
	    np->wlan_prev_nbars	= nbars;
	}
    }

//  AGE OLDEST SIGNAL STRENGTH BY SETTING NEXT ONE TO 'SILENCE'
    np->wlan_rx_history[np->wlan_index_history]	= WLAN_1_BAR - 0.01;
    np->wlan_index_history = (np->wlan_index_history+1) % WLAN_RX_HISTORY;
}

bool draw_wlansignal(int srcwlan)
{
    WLAN	*wlan	= &WLANS[srcwlan];
    NODEATTR	*na	= &NODES[wlan->node].nattr;

    bool	again	= true;
    int		savev	= vflag;

    vflag	= 0;

    if(wlan->radius == 0) {
	wlan->radius	= DRAW_WLAN_RADIUS;
	TCLTK(
#if	FILL_WLAN_ARCS
	    "$map create oval %i %i %i %i -fill %s -tags {n%i w%iss}",
#else
	    "$map create oval %i %i %i %i -outline %s -tags {n%i w%iss}",
#endif
		    M2PX(na->position.x - wlan->radius),
		    M2PX(na->position.y - wlan->radius),
		    M2PX(na->position.x + wlan->radius),
		    M2PX(na->position.y + wlan->radius),
		    wlan->signalcolour,
		    wlan->node, srcwlan );
    }
    else if(wlan->radius >= wlan->maxradius) {
	TCLTK("$map delete w%iss", srcwlan);
	wlan->radius	= 0;
	again		= false;
    }
    else {
	wlan->radius	+= DRAW_WLAN_RADIUS;
	TCLTK("$map coords w%iss %i %i %i %i",
		    srcwlan,
		    M2PX(na->position.x - wlan->radius),
		    M2PX(na->position.y - wlan->radius),
		    M2PX(na->position.x + wlan->radius),
		    M2PX(na->position.y + wlan->radius) );
    }

    vflag	= savev;
    return again;
}
#endif

// ----------------------------------------------------------------------

static WLAN *find_wlan(int thislink)
{
    NICATTR	*nic;
    WLAN	*wlan;
    int		w;


//  ENSURE THAT WE HAVE A VALID LINK NUMBER
    if(thislink < 0 || thislink > NP->nnics) {
	ERROR(ER_BADLINK);
	return NULL;
    }

    nic	= &NP->nics[thislink];
//  ENSURE THAT WE ARE TRYING TO SET A WLAN LINK
    if(nic->linktype != LT_WLAN) {
	ERROR(ER_NOTSUPPORTED);
	return NULL;
    }

//  ITERATE THROUGH ALL WLANs AND FIND THIS ONE
    FOREACH_WLAN {
	if(wlan->node == THISNODE && wlan->node_link == thislink)
	    return wlan;
    }
    ERROR(ER_BADLINK);
    return NULL;
}

// ----------------------------------------------------------------------

/* http://www.terabeam.com/support/calculations/som.php
	FSL	= 32.5   + 20.log10(km) + 20.log10(MHz)
	FSL	= 92.467 + 20.log10(km) + 20.log10(GHz)

   http://www.swisswireless.org/wlan_calc_en.html
	dBm			= 10*log10(Watts / 0.001)
	Tx power of Orinoco:	30mW == 14.771 dBm
	Cable-loss in coaxial:	assumed 0 for us
	Antenna gain:		2.14dBi (isotropic decibels)
	Radiated power:		Tx-power(dBm) - cable-loss + antenna-gain(dBi)
	Receiver sensitivity:	Orinoco cards: 11Mbps => -82dBm
	Signal/Noise ratio:	10*log10(signal-watts / noise-watts)
				Orinoco cards: 11Mbps => 16dBm
 */

/*  WE SHOULD (RE-)INTRODUCE EFFICIENCY, AND INLINE THIS DEFAULT FUNCTION
    AND CALCULATE (ONCE) ITS COMMON EXPRESSIONS  */
static WLANRESULT default_WLAN_model(WLANSIGNAL *sig)
{
    int		dx, dy, dz;
    double	metres;
    double	TXtotal, FSL, budget;

//  CALCULATE THE TOTAL OUTPUT POWER LEAVING TRANSMITTER
    TXtotal	= sig->tx_info->tx_power_dBm - sig->tx_info->tx_cable_loss_dBm +
		    sig->tx_info->tx_antenna_gain_dBi;

//  CALCULATE THE DISTANCE TO THE DESTINATION NODE
    dx		= (sig->tx_pos.x - sig->rx_pos.x);
    dy		= (sig->tx_pos.y - sig->rx_pos.y);
    dz		= (sig->tx_pos.z - sig->rx_pos.z);
    metres	= sqrt((double)(dx*dx + dy*dy + dz*dz)) + 0.1;

//  CALCULATE THE FREE-SPACE-LOSS OVER THIS DISTANCE
    FSL		= (92.467 + 20.0*log10(sig->tx_info->frequency_GHz)) +
		    20.0*log10(metres/1000.0);

//  CALCULATE THE SIGNAL STRENGTH ARRIVING AT RECEIVER
    sig->rx_strength_dBm = TXtotal - FSL +
	    sig->rx_info->rx_antenna_gain_dBi - sig->rx_info->rx_cable_loss_dBm;

//  CAN THE RECEIVER DETECT THIS SIGNAL AT ALL?
    budget	= sig->rx_strength_dBm - sig->rx_info->rx_sensitivity_dBm;
    if(budget < 0.0)
	return WLAN_TOOWEAK;

    if(dflag) {
	static int	last_tx_seqno = UNKNOWN;

	if(last_tx_seqno != sig->tx_seqno) {
	    REPORT("\n\n%s:\t\t@%.3fGHz TXpower=%.3fdBm TXtotal=%.3fdBm\n",
		    NODES[sig->tx_n].nodename, sig->tx_info->frequency_GHz,
		    sig->tx_info->tx_power_dBm, TXtotal);
	    last_tx_seqno	= sig->tx_seqno;
	}
      REPORT("\tn=%2d d=%3d FSL=%.3f Rx=%.3f sens=%.3f budget=%.3f s/n=%.3f\n",
		    sig->rx_n,
		    (int)metres, FSL,
		    sig->rx_strength_dBm, sig->rx_info->rx_sensitivity_dBm,
		    budget, sig->rx_info->rx_signal_to_noise_dBm);
    }

//  CAN THE RECEIVER DECODE THIS SIGNAL?
    return (budget < sig->rx_info->rx_signal_to_noise_dBm) ?
		WLAN_TOONOISY : WLAN_RECEIVED;
}

static WLANRESULT (*WLAN_MODEL)(WLANSIGNAL *sig)	= default_WLAN_model;

int CNET_set_wlan_model( WLANRESULT (*newmodel)(WLANSIGNAL *sig))
{
    if(gattr.trace_events)
	TRACE(0, "\t%s(newmodel=%s) = 0\n",
		    __func__, find_trace_name(&newmodel));

    WLAN_MODEL = (newmodel == NULL) ? default_WLAN_model : newmodel;
    return 0;
}

// ----------------------------------------------------------------------

int CNET_get_wlaninfo(int thislink, WLANINFO *info)
{
    int		result	= -1;
    WLAN	*wlan;

    if(info == NULL)
	ERROR(ER_BADARG);
    else {
	wlan	= find_wlan(thislink);
	if(wlan) {
	    memcpy(info, &wlan->info, sizeof(WLANINFO));
	    result	= 0;
	}
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,info=%s) = 0\n",
			__func__,
			thislink, find_trace_name(info));
	else
	    TRACE(1, "\t%s(link=%i,info=%s) = -1 %s\n",
			__func__,
			thislink, find_trace_name(info),
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_set_wlaninfo(int thislink, WLANINFO *newinfo)
{
    int		result	= -1;
    WLAN	*wlan;

    if(newinfo == NULL)
	ERROR(ER_BADARG);
    else {
	wlan	= find_wlan(thislink);
	if(wlan) {
	    memcpy(&wlan->info, newinfo, sizeof(WLANINFO));
	    result	= 0;
	}
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,newinfo=%s) = 0\n",
			__func__,
			thislink, find_trace_name(newinfo));
	else
	    TRACE(1, "\t%s(link=%i,newinfo=%s) = -1 %s\n",
			__func__,
			thislink, find_trace_name(newinfo),
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}

// ----------------------------------------------------------------------

static char *wlan_statenames[]	= {
	"WLAN_SLEEP", "WLAN_IDLE", "WLAN_TX", "WLAN_RX"
};

int CNET_get_wlanstate(int thislink, WLANSTATE *state)
{
    int		result	= -1;
    WLAN	*wlan;

    wlan	= find_wlan(thislink);
    if(wlan) {
	NICATTR	*nic = &NODES[wlan->node].nics[wlan->node_link];

	if(wlan->state == WLAN_SLEEP)
	    ;
	else if(MICROSECONDS <= nic->tx_until)
	    wlan->state	= WLAN_TX;
	else if(MICROSECONDS <= nic->rx_until)
	    wlan->state	= WLAN_RX;
	else
	    wlan->state	= WLAN_IDLE;

	*state	= wlan->state;
	result	= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,%s) = 0 (*state=%s)\n",
			__func__,
			thislink, find_trace_name(state),
			wlan_statenames[(int)wlan->state]);
	else
	    TRACE(1, "\t%s(link=%i,%s) = -1 %s\n",
			__func__,
			thislink, find_trace_name(state),
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_set_wlanstate(int thislink, WLANSTATE state)
{
    int		result	= -1;
    WLAN	*wlan	= NULL;

    if(state != WLAN_SLEEP && state != WLAN_IDLE)
	ERROR(ER_BADARG);
    else {
	wlan	= find_wlan(thislink);
	if(wlan) {
	    wlan->state	= state;
	    result	= 0;
	}
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,%s) = 0\n",
			__func__, thislink, wlan_statenames[(int)wlan->state]);
	else
	    TRACE(1, "\t%s(link=%i,newstate) = -1 %s\n",
			__func__, thislink, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

// ----------------------------------------------------------------------

int CNET_wlan_arrival(int thislink, double *rx_signal_dBm, double *rx_angle)
{
    int		result	= -1;
    WLAN	*wlan;

    wlan	= find_wlan(thislink);
    if(wlan) {
	if(rx_signal_dBm)
	    *rx_signal_dBm	= wlan->rx_signal;
	if(rx_angle)
	    *rx_angle		= wlan->rx_angle;
	result	= 0;
    }

    if(gattr.trace_events) {
	if(result == 0) {
	    char	fmta[64], fmtb[64];

	    if(rx_signal_dBm)
		sprintf(fmta, "%.3f", *rx_signal_dBm);
	    else
		strcpy(fmta, "NULL");
	    if(rx_angle)
		sprintf(fmtb, "%.3f", *rx_angle);
	    else
		strcpy(fmtb, "NULL");
	    TRACE(0, "\t%s(link=%i,&rx_signal,&rx_angle) = 0 (%s,%s)\n",
			__func__, thislink, fmta, fmtb);
	}
	else
	    TRACE(1, "\t%s(link=%i,&rx_signal,&rx_angle) = -1 %s\n",
			__func__, thislink, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_set_wlancolour(int thislink, CnetColour colour)
{
    int		result	= -1;
    WLAN	*wlan;

    wlan	= find_wlan(thislink);
    if(wlan) {
#if	defined(USE_TCLTK)
	if(Wflag)
	    wlan->signalcolour	= colour;
#endif
	result = 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(link=%i,colour=\"%s\") = 0\n",
		    __func__, thislink, colour);
	else
	    TRACE(1, "\t%s(link=%i,colour=\"%s\") = ",
		    __func__, thislink, colour, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

// ----------------------------------------------------------------------

int write_wlan(int thislink, char *frame, size_t len, bool unreliable)
{
    NICATTR		*srcnic;
    WLAN		*srcwlan;

    NODE		*destnp;
    NICATTR		*destnic;
    WLAN		*wlan;

    FRAMEARRIVAL	*newf;
    CnetTime		Twrite, Tarrives;
    WLANSIGNAL		sig;
    WLANRESULT		wlan_result;

    int			w, dx, dy, dz;
    double		metres;

#if	defined(USE_TCLTK)
    double		maxmetres=0.0;
#endif

    srcwlan	= find_wlan(thislink);
    if(srcwlan == NULL)
	return(-1);
    srcnic	= &NP->nics[srcwlan->node_link];

//  IS THE TRANSMITTING WIRELESS CARD SLEEPING?
    if(srcwlan->state == WLAN_SLEEP) {
	ERROR(ER_NOTREADY);
	return(-1);
    }

//  HAVE WE EXHAUSTED OUR MOBILE'S BATTERY?
#if	CNET_PROVIDES_MOBILITY
    if(NP->nodetype == NT_MOBILE && NP->nattr.battery_mAH == 0.0) {
	ERROR(ER_NOBATTERY);
	return(-1);
    }
#endif

//  TODO: CONSUME TRANSMITTING NODE's BATTERY POWER HERE....

//  PREPARE DETAILS OF THIS SIGNAL
    static int64_t	tx_seqno	= 0;

    sig.tx_seqno	= ++tx_seqno;
    sig.tx_n		= THISNODE;
    sig.tx_pos.x	= NP->nattr.position.x;
    sig.tx_pos.y	= NP->nattr.position.y;
    sig.tx_pos.z	= NP->nattr.position.z;
    sig.tx_info		= &srcwlan->info;

//  UPDATE GLOBAL LINK TRANSMISSION STATISTICS
    gattr.linkstats.tx_frames++;
    gattr.linkstats.tx_bytes	+= len;

//  UPDATE THIS TRANSMITTING NIC's STATISTICS
    srcnic->linkstats.tx_frames++;
    srcnic->linkstats.tx_bytes	+= len;
#if	defined(USE_TCLTK)
    srcnic->stats_changed	= true;
#endif

//  WRITING UNTIL  = NOW + (LENGTH_IN_BITS / BANDWIDTH_BPS) + JITTER
    Twrite	= WLAN_ENCODE_TIME +
	  ((CnetTime)len*(CnetTime)8000000)/srcnic->bandwidth +
	  ((srcnic->jitter == 0) ? 0 : poisson_usecs(srcnic->jitter,NP->mt));

//  WE CANNOT COMMENCE WRITING IF THIS NIC IS CURRENTLY TRANSMITTING
    srcnic->tx_until	= Twrite + MAX(MICROSECONDS, srcnic->tx_until);

//  FOREACH POTENTIAL DESTINATION WLAN NIC ....
    FOREACH_WLAN {
	if(wlan == srcwlan)		// RXnode == TXnode ?
	    continue;

	sig.rx_info	= &wlan->info;

//  TRANSMITTING AND RECEIVING WLANs MUST BE OPERATING AT THE SAME FREQUENCY
	if(sig.tx_info->frequency_GHz != sig.rx_info->frequency_GHz)
	    continue;

	destnp		= &NODES[wlan->node];
	destnic		= &destnp->nics[wlan->node_link];

//  CALCULATE THE DISTANCE TO THE DESTINATION NODE
	dx		= (NP->nattr.position.x - destnp->nattr.position.x);
	dy		= (NP->nattr.position.y - destnp->nattr.position.y);
	dz		= (NP->nattr.position.z - destnp->nattr.position.z);
	metres		= sqrt((double)(dx*dx + dy*dy + dz*dz)) + 0.1;
	Tarrives	= srcnic->tx_until + metres/(CnetTime)299;

	sig.rx_n	= wlan->node;
	sig.rx_pos.x	= destnp->nattr.position.x;
	sig.rx_pos.y	= destnp->nattr.position.y;
	sig.rx_pos.z	= destnp->nattr.position.z;

//  CALL THE WLAN PROPAGATION MODEL
	wlan_result	= (*WLAN_MODEL)(&sig);

//  CAN THE DESTINATION DETECT THE SIGNAL AT ALL?
	if(wlan_result == WLAN_TOOWEAK)
	    continue;

//  IS THE DESTINATION NODE OR WIRELESS CARD SLEEPING?
	if(wlan->state == WLAN_SLEEP || destnp->runstate != STATE_RUNNING) {
	    destnic->rx_until	= MAX(Tarrives, destnic->rx_until);
	    continue;
	}
#if 0
	if(unreliable && lose_frame(thislink, srcnic, len) == true)
	    continue;
#endif

//  IF DESTINATION IS EITHER TRANSMITTING OR RECEIVING ==> COLLISION
	if(unreliable &&
	  (Tarrives <= destnic->tx_until || Tarrives <= destnic->rx_until)) {
	    extern void unschedule_link(int destnode, int destlink);

	    unschedule_link(wlan->node, wlan->node_link);
	    destnic->rx_until	= MAX(Tarrives, destnic->rx_until);

	    newf		= NEW(FRAMEARRIVAL);
	    CHECKALLOC(newf);
	    newf->destnode	= wlan->node;
	    newf->destlink	= wlan->node_link;
	    newf->linktype	= LT_WLAN;
	    newf->linkindex	= w;

	    newf->rx_signal	= sig.rx_strength_dBm;
	    newf->rx_angle	= atan2(-dy, dx);
	    newf->arrives	= Tarrives + WLAN_DECODE_TIME;
	    newf->corrupted	= true;
	    newevent(EV_FRAMECOLLISION, newf->destnode,
		     newf->arrives - MICROSECONDS, NULLTIMER, (CnetData)newf);
	}
//  CAN WE DECODE THE SIGNAL'S DATA AMIDST THE BACKGROUND NOISE?
	else if(wlan_result != WLAN_RECEIVED) {
	    destnic->rx_until	= MAX(Tarrives, destnic->rx_until);
	    continue;
	}

//  FRAME IS DECODED AND WILL ARRIVE
	else {
	    destnic->rx_until	= MAX(Tarrives, destnic->rx_until);

	    newf		= NEW(FRAMEARRIVAL);
	    CHECKALLOC(newf);
	    newf->destnode	= wlan->node;
	    newf->destlink	= wlan->node_link;
	    newf->linktype	= LT_WLAN;
	    newf->linkindex	= w;

	    newf->rx_signal	= sig.rx_strength_dBm;
	    newf->rx_angle	= atan2(-dy, dx);
	    newf->arrives	= Tarrives + WLAN_DECODE_TIME;
	    newf->len		= len;
	    newf->frame		= malloc(len);
	    CHECKALLOC(newf->frame);
	    memcpy(newf->frame, frame, len);
	    newf->corrupted	=
		(unreliable && corrupt_frame(srcnic, newf->frame, newf->len));

//  SCHEDULE A NEW EVENT FOR THIS FRAME ARRIVAL
	    newevent(EV_PHYSICALREADY, newf->destnode,
		     newf->arrives - MICROSECONDS, NULLTIMER, (CnetData)newf);

#if	defined(USE_TCLTK)
//  REMEMBER THE FURTHEST DISTANCE THAT THIS SIGNAL IS DECODED
	    if(maxmetres < metres)
		maxmetres	= metres;
#endif
	}
    }		//  FOREACH POTENTIAL DESTINATION WLAN NIC ....

#if	defined(USE_TCLTK)
    if(Wflag) {
	extern void schedule_drawwlansignal(int thewlan);

	if(gattr.drawwlans && srcwlan->radius == 0) {
	    srcwlan->maxradius	= (int)maxmetres;
	    schedule_drawwlansignal(srcwlan - WLANS);
	}
    }
#endif

    return 0;
}

// ----------------------------------------------------------------------

NICATTR *add_wlan(int whichnode)
{
    NODE	*np;
    NICATTR	*nic;
    WLAN	*wlan;
    char	buf[16];

//  ADD A NEW NIC TO whichnode
    np		= &NODES[whichnode];
    np->nics	= realloc(np->nics,(np->nnics+2)*sizeof(NICATTR));
    CHECKALLOC(np->nics);
    nic		= &np->nics[np->nnics+1];

    memcpy(nic, &np->defaultwlan, sizeof(NICATTR));	// defaults
    memcpy(nic->nicaddr, NICADDR_ZERO,  LEN_NICADDR);
    sprintf(buf, "wlan%i", np->nwlans);
    nic->name	= strdup(buf);
    CHECKALLOC(nic->name);

    WLANS		= realloc(WLANS, (gattr.nwlans+1)*sizeof(WLAN));
    CHECKALLOC(WLANS);
    wlan		= &WLANS[gattr.nwlans];
    wlan->node		= whichnode;
    wlan->node_link	= np->nnics+1;

#if	defined(USE_TCLTK)
    if(Wflag) {
	wlan->radius		= 0;
	wlan->signalcolour	= COLOUR_WLAN;

	FOREACH_RX_HISTORY {
	    np->wlan_rx_history[h] = WLAN_1_BAR - 0.01;
	}
	np->wlan_index_history	= 0;
	np->wlan_prev_nbars	= UNKNOWN;
    }
#endif
    nic->wlaninfo	= &wlan->info;
/*
    memcpy(&wlan->info, &np->defaultwlan.wlaninfo, sizeof(WLANINFO));
 */
    memcpy(&wlan->info, DEFAULTWLAN.wlaninfo, sizeof(WLANINFO));
    wlan->state		= WLAN_IDLE;

    ++np->nwlans;
    ++np->nnics;
    ++gattr.nwlans;

    return nic;
}

// ----------------------------------------------------------------------

void check_wlans(bool **adj)
{
    WLAN	*wlan;
    int		w;

//  ENSURE THAT EACH WLAN NIC HAS AN ADDRESS
    FOREACH_WLAN {
	assign_nicaddr(wlan->node, wlan->node_link);
    }
}


// ----------------------------------------------------------------------

void init_defaultwlan(bool Bflag)
{
    WLANINFO	*wlaninfo;

//  INITIALIZE DEFAULT WLAN ATTRIBUTES
    DEFAULTWLAN.buffered		= Bflag;

    wlaninfo				=
    DEFAULTWLAN.wlaninfo		= NEW(WLANINFO);
    CHECKALLOC(DEFAULTWLAN.wlaninfo);

    wlaninfo->frequency_GHz		= WLAN_FREQUENCY_GHz;
    wlaninfo->tx_power_dBm		= WLAN_TX_POWER_dBm;
    wlaninfo->tx_cable_loss_dBm		= WLAN_TX_CABLE_LOSS_dBm;
    wlaninfo->tx_antenna_gain_dBi	= WLAN_TX_ANTENNA_GAIN_dBi;

    wlaninfo->rx_antenna_gain_dBi	= WLAN_RX_ANTENNA_GAIN_dBi;
    wlaninfo->rx_cable_loss_dBm		= WLAN_RX_CABLE_LOSS_dBm;
    wlaninfo->rx_sensitivity_dBm	= WLAN_RX_SENSITIVITY_dBm;
    wlaninfo->rx_signal_to_noise_dBm	= WLAN_RX_SIGNAL_TO_NOISE_dBm;

    wlaninfo->sleep_current_mA		= WLAN_SLEEP_mA;
    wlaninfo->idle_current_mA		= WLAN_IDLE_mA;
    wlaninfo->tx_current_mA		= WLAN_TX_mA;
    wlaninfo->rx_current_mA		= WLAN_RX_mA;
}

void init_wlans(void)
{
}

#endif
