#include "cnetprivate.h"
#include "scheduler.h"
#include "physicallayer.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	SCHEDULER_SEED		6432085

#if	defined(USE_TCLTK)
static	TCLTK_COMMAND(scheduler_speed);

static	int64_t		gui_every_nevents	= 0;
static	CnetTime	gui_every_nusecs	= 0;
#endif

static	MT		*mt;
static	CnetTime	BIGBANG;
static	int		SWAPPED_IN		= UNKNOWN;

static	int64_t		finish_after_nevents	= 0;
static	CnetTime	finish_after_nusecs	= 0;
static	int64_t		stats_every_nevents	= 0;
static	CnetTime	stats_every_nusecs	= 0;

static	int		cntx			= 0;
static	int		events_handled		= 0;
static	int		reveal_nnodes 		= 0;
static	int		min_nqueued		= 0;

static	bool		sflag			= false;
static	bool		Tflag			= false;

static EVENT_HANDLER( HANDLER );

void single_event(int n, CnetEvent ev, CnetTimerID timer, CnetData data)
{
    if(NODES[n].runstate == STATE_RUNNING && NODES[n].handler[(int)ev])  {
#if	!defined(USE_IPOD)
	THISNODE	= n;
#endif
	NP	= &NODES[THISNODE];
	HANDLER(ev, timer, data);
    }
}

// ---------- CALCULATE TIMES OF NODE AND LINK FAILURE AND REPAIR ---------

#if	!defined(USE_IPOD)
static CnetTime node_mtbf(void)
{
    CnetTime extent = WHICH(NP->nattr.nodemtbf, DEF_NODEATTR.nodemtbf);

    if(extent == 0)
	return((CnetTime)0);
    else
	return(poisson_usecs(extent, mt));
}

static CnetTime node_mttr(void)
{
    CnetTime extent = WHICH(NP->nattr.nodemttr, DEF_NODEATTR.nodemttr);

    if(extent == 0)
	return((CnetTime)0);
    else
	return(poisson_usecs(extent, mt));
}
#endif

#if	CNET_PROVIDES_WANS
static CnetTime link_mtbf(WAN *wanp)
{
    CnetTime	t1, t2, extent;

    t1	= WHICH(NODES[wanp->minnode].nics[wanp->minnode_nic].linkmtbf,
		DEFAULTWAN.linkmtbf);

    t2	= WHICH(NODES[wanp->maxnode].nics[wanp->maxnode_nic].linkmtbf,
		DEFAULTWAN.linkmtbf);

         if(t1 == 0)
	extent = t2;
    else if(t2 == 0 || t1 < t2)
	extent = t1;
    else
	extent = t2;

    if(extent == 0)
	return((CnetTime)0);
    else
	return(poisson_usecs(extent, mt));
}

static CnetTime link_mttr(WAN *wanp)
{
    CnetTime	t1, t2;

    t1	= WHICH(NODES[wanp->minnode].nics[wanp->minnode_nic].linkmttr,
		DEFAULTWAN.linkmttr);

    t2	= WHICH(NODES[wanp->maxnode].nics[wanp->maxnode_nic].linkmttr,
		DEFAULTWAN.linkmttr);

    if(t1 > t2)
	return(poisson_usecs(t1, mt));
    else
	return(poisson_usecs(t2, mt));
}

void report_linkstate(int w)
{
    WAN	*wanp	= &WANS[w];

#if	defined(USE_TCLTK)
    if(Wflag) {
	extern	void draw_wan(int, bool, int);

	draw_wan(w, true, CANVAS_FATLINK/2);
	draw_node_icon(wanp->minnode, NULL, 0, 0);
	draw_node_icon(wanp->maxnode, NULL, 0, 0);
    }
#endif

    NODES[wanp->minnode].nics[wanp->minnode_nic].up	= wanp->up;
    if(wanp->minnode == SWAPPED_IN)
	linkinfo[wanp->minnode_nic].linkup      = wanp->up;
    single_event(wanp->minnode,
		 EV_LINKSTATE, NULLTIMER, (CnetData)wanp->minnode_nic);

    NODES[wanp->maxnode].nics[wanp->maxnode_nic].up	= wanp->up;
    if(wanp->maxnode == SWAPPED_IN)
	linkinfo[wanp->maxnode_nic].linkup      = wanp->up;
    single_event(wanp->maxnode,
		 EV_LINKSTATE, NULLTIMER, (CnetData)wanp->maxnode_nic);
}
#endif


// ------------------------------------------------------------------------

//  THE COMPILER SHOULD UNROLL THESE LOOPS IF NDATASEGS > 1

#if	NDATASEGS > 0
static void swapout_data(int n)
{
    NODE *p	= &NODES[n];

    for(int i=0 ; i<NDATASEGS ; ++i)
	if(p->length_data[i])
	    memcpy(p->private_data[i], p->incore_data[i], p->length_data[i]);
}

static void swapin_data(int n)
{
    NODE *p	= &NODES[n];

    for(int i=0 ; i<NDATASEGS ; ++i)
	if(p->length_data[i])
	    memcpy(p->incore_data[i], p->private_data[i], p->length_data[i]);
}

static void reboot_data(void)
{
    for(int i=0 ; i<NDATASEGS ; ++i)
	if(NP->length_data[i])
	    memcpy(NP->incore_data[i],NP->original_data[i],NP->length_data[i]);
}
#endif

// ------------------------------------------------------------------------

static void context_switch(void)
{
    CnetLinkInfo	*li;
    NICATTR		*nic;

// SWAP OUT THE SWAPPED_IN PROCESS, SWAP IN THE THISNODE PROCESS
    if(SWAPPED_IN >= 0) {
	NODES[SWAPPED_IN].os_errno		= errno;
	NODES[SWAPPED_IN].cnet_errno	= cnet_errno;
    }

#if	NDATASEGS > 0
    swapout_data(SWAPPED_IN);
    swapin_data(THISNODE);
#endif

    NP			= &NODES[THISNODE];
    nodeinfo.nodenumber	= THISNODE;
    strcpy(nodeinfo.nodename, NP->nodename);
    nodeinfo.nodetype	= NP->nodetype;
    nodeinfo.address	= NP->nattr.address;

#if	CNET_PROVIDES_APPLICATION_LAYER
    nodeinfo.minmessagesize	=
	    WHICH(NP->nattr.minmessagesize, DEF_NODEATTR.minmessagesize);
    nodeinfo.maxmessagesize	=
	    WHICH(NP->nattr.maxmessagesize, DEF_NODEATTR.maxmessagesize);
    nodeinfo.messagerate	=
	    WHICH(NP->nattr.messagerate, DEF_NODEATTR.messagerate);
#endif

    nodeinfo.nlinks		= NP->nnics;

//  INITIALIZE THE NODE'S LOOPBACK LINK
    memset(&linkinfo[LOOPBACK_LINK], 0, sizeof(CnetLinkInfo));
    strcpy(linkinfo[LOOPBACK_LINK].linkname, "lo");
    linkinfo[LOOPBACK_LINK].linktype	= LT_LOOPBACK;
    linkinfo[LOOPBACK_LINK].linkup	= true;
#if	CNET_PROVIDES_APPLICATION_LAYER
    linkinfo[LOOPBACK_LINK].mtu		= MAX_MESSAGE_SIZE + 1024;
#else
    linkinfo[LOOPBACK_LINK].mtu		= 8192;
#endif

//  INITIALIZE THE NODE'S PHYSICAL LINKS
    int	l;
    for(l=1, li=&linkinfo[1] ; l<=NP->nnics ; ++l, ++li) {
	nic			= &NP->nics[l];
	strcpy(li->linkname, nic->name);

	li->linktype		= nic->linktype;
	li->linkup		= nic->up;
	memcpy(li->nicaddr, nic->nicaddr, LEN_NICADDR);

//  THESE ATTRIBUTES CANNOT BE CHANGED VIA THE GUI
	li->bandwidth		= nic->bandwidth;
	li->mtu			= nic->mtu;
	li->propagationdelay	= nic->propagation;

//  THESE ATTRIBUTES MAY HAVE BEEN CHANGED
    switch(nic->linktype) {
      case LT_LOOPBACK :
	break;
      case LT_WAN :
#if	CNET_PROVIDES_WANS
	li->costperbyte  = WHICH(nic->costperbyte, DEFAULTWAN.costperbyte);
	li->costperframe = WHICH(nic->costperframe,DEFAULTWAN.costperframe);
#endif
	break;
      case LT_LAN :
#if	CNET_PROVIDES_LANS
	li->costperbyte  = WHICH(nic->costperbyte, DEFAULTLAN.costperbyte);
	li->costperframe = WHICH(nic->costperframe,DEFAULTLAN.costperframe);
#endif
	break;
      case LT_WLAN :
#if	CNET_PROVIDES_WLANS
	li->costperbyte  = WHICH(nic->costperbyte, DEFAULTWLAN.costperbyte);
	li->costperframe = WHICH(nic->costperframe,DEFAULTWLAN.costperframe);
#endif
	break;
      }
    }

//  AND FINALLY, THE NODE'S FINAL GLOBAL ATTRIBUTES
    errno		= NP->os_errno;
    cnet_errno		= NP->cnet_errno;

    NNODES		= reveal_nnodes;	// iff invoked with -N

    SWAPPED_IN		= THISNODE;
    cntx++;
}

// ------------------------------------------------------------------------

static void HANDLER(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    if(NP->handler[(int)ev]) {
	CnetTime	usec;

	if(THISNODE != SWAPPED_IN)
	    context_switch();

//  SET THE REAL-WORLD TIME ON THE NODE, ALLOWING FOR ITS CLOCK SKEW
	usec			= BIGBANG + MICROSECONDS + NP->clock_skew;
	nodeinfo.time_of_day.sec  = (int)(usec / (CnetTime)1000000);
	nodeinfo.time_of_day.usec = (int)(usec % (CnetTime)1000000);
	nodeinfo.time_in_usec	= MICROSECONDS - NP->reboot_time;

	NP->nodestats.nevents++;
	gattr.hiddenstats.event_count[(int)ev]++;
	gattr.nodestats.nevents++;

	gattr.stdio_quiet	= DEF_NODEATTR.stdio_quiet	|
				    NP->nattr.stdio_quiet;

	gattr.trace_events	= DEF_NODEATTR.trace_all		|
				    NP->nattr.trace_all		|
				    (NP->nattr.trace_mask & (1<<(int)ev));
	HANDLING = ev;
	if(gattr.trace_events) {
	    extern void	trace_handler(NODE *, CnetEvent, CnetTimerID, CnetData);

	    trace_handler(NP, ev, timer, data);
	}
	else
	    NP->handler[(int)ev](ev, timer, data);

	++events_handled;
    }
}

static void invoke_reboot(int which)
{
#if	!defined(USE_IPOD)
    THISNODE	= which;
#endif
    NP	= &NODES[THISNODE];
    if(NP->handler[(int)EV_REBOOT]) {

	extern int	reboot_stdio_layer(void);
	extern void	reboot_timer_layer(void);

	context_switch();		// ALWAYS PERFORMED
#if	NDATASEGS > 0
	reboot_data();
#endif
	unschedule_node(THISNODE);

#if	CNET_PROVIDES_APPLICATION_LAYER
	if(NODE_HAS_AL(THISNODE)) {
	    extern int	reboot_application_layer(CnetTime *evtime);

	    CnetTime evtime;
	    reboot_application_layer(&evtime);
	}
#endif

	reboot_stdio_layer();
	reboot_timer_layer();

	memset(&NP->nodestats, 0, sizeof(CnetNodeStats));
	NP->reboot_time	= MICROSECONDS;
	NP->nframes	= 0;
	NP->os_errno	= 0;
	NP->cnet_errno	= ER_OK;
	cnet_errno	= ER_OK;

	NP->runstate	= STATE_REBOOTING;
	HANDLER(EV_REBOOT, NULLTIMER, (CnetData)NP->nattr.reboot_argv);
	NP->runstate	= STATE_RUNNING;

#if	!defined(USE_IPOD)
//  NOW INSERT AN EVENT IF THIS NODE IS DOOMED TO FAIL
	CnetTime evtime	= node_mtbf();
	if(evtime != 0)
	    internal_event(SCHED_NODESTATE, EV_REBOOT, THISNODE, evtime, 0);
#endif
    }
    else
	FATAL("%s rebooted without an EV_REBOOT handler!\n", NP->nodename);
}

void invoke_shutdown(int which)
{
#if	!defined(USE_IPOD)
    THISNODE	= which;
#endif
    NP	= &NODES[THISNODE];

    HANDLER(EV_SHUTDOWN, NULLTIMER, NP->data[(int)EV_SHUTDOWN]);
    NP->runstate	= STATE_SHUTDOWN;
}


// ------------------------------------------------------------------------

static void parse_period(const char *str, int64_t *nevents, CnetTime *nusecs,
			 char *buf)
{
    const char	*s	= str;
    int64_t	intval	= 0;

    char	*u1	= "Updated every";
    char	*u2	= "of simulated time";

    *nevents	= 0;
    *nusecs	= 0;

    while(isdigit(*s)) {
	intval = intval*(int64_t)10 + (int64_t)(*s - '0');
	++s;
    }
    while(*s == ' ' || *s == '\t')
	++s;
    switch (*s) {
    case 's' :	if(buf)
		    sprintf(buf, "%s %" PRId64 " second%s %s",
					    u1, intval, PLURAL(intval), u2);
		*nusecs = intval * (int64_t)1000000;
		break;
    case 'm' :	if(*(s+1) == 's') {
		    if(buf)
			sprintf(buf, "%s %" PRId64 "msec %s", u1, intval, u2);
		    *nusecs = intval * (int64_t)1000;
		}
		else {
		    if(buf)
			sprintf(buf, "%s %" PRId64 "min%s %s",
					    u1, intval, PLURAL(intval), u2);
		    *nusecs = intval * (int64_t)60 * (int64_t)1000000;
		}
		break;
    case 'h' :	if(buf)
		    sprintf(buf, "%s %" PRId64 " hour%s %s",
					    u1, intval, PLURAL(intval), u2);
		*nusecs = intval * (int64_t)3600 * (int64_t)1000000;
		break;
    case 'u' :	if(buf)
		    sprintf(buf, "%s %" PRId64 "usec%s %s",
					    u1, intval, PLURAL(intval), u2);
		*nusecs = intval;
		break;
    case 'e' :	if(buf)
		    sprintf(buf, "%s %" PRId64 " event%s %s",
					    u1, intval, PLURAL(intval), u2);
		*nevents = intval;
		break;
    default :	FATAL("unrecognized period '%s'\n", str);
		break;
    }
}

// ------------------------------------------------------------------------

#if	defined(USE_TCLTK)
static	char	*update_title;

//  CHANGE THE FREQUENCY WITH WHICH WE FLUSH THE GUI
static TCLTK_COMMAND(update_speed)
{
    parse_period(argv[1],&gui_every_nevents,&gui_every_nusecs,update_title);
    Tcl_UpdateLinkedVar(tcl_interp, "UPDATE_TITLE");
    return TCL_OK;
}
#endif

// ------------------------------------------------------------------------

void init_scheduler(const char *eflag, const char *fflag, bool Nflag,
			int Sflag, bool _sflag, bool _Tflag, const char *uflag)
{
    struct timeval	NOW;

    gettimeofday(&NOW, NULL);
    BIGBANG		= (CnetTime)NOW.tv_sec*(CnetTime)1000000+NOW.tv_usec;
    MICROSECONDS	= 0;

    reveal_nnodes	= Nflag ? _NNODES : 0;
    mt			= mt19937_init((unsigned long)(SCHEDULER_SEED+Sflag));
    sflag		= _sflag;
    Tflag		= _Tflag;

#if	defined(USE_IPOD)
    SWAPPED_IN		= UNKNOWN;
#else
    THISNODE		= -1;
    SWAPPED_IN		= _NNODES-1;
#endif

    init_queuing();

//  IT IS ESSENTIAL THAT NODE 0 REBOOTS FIRST (cf. context_switch() )
    for(int n=0 ; n<_NNODES ; ++n) {
	NODES[n].runstate = STATE_REBOOTING;
	internal_event(SCHED_NODESTATE, EV_REBOOT, n, (CnetTime)n, 0);
    }

#if	CNET_PROVIDES_WANS
//  NEXT, INSERT ALL POSSIBLE WAN FAILURES
    for(int n=0 ; n<gattr.nwans ; ++n) {
	CnetTime	evtime	= link_mtbf(&WANS[n]);

	if(evtime != 0)
	    newevent(EV_LINKSTATE,WANS[n].minnode,evtime,NULLTIMER,(CnetData)n);
    }
#endif

//  DETERMINE THE FREQUENCY WITH WHICH WE FLUSH THE GUI
#if	defined(USE_TCLTK)
    if(Wflag) {
	update_title	= Tcl_Alloc(128);
	Tcl_LinkVar(tcl_interp, "UPDATE_TITLE", (char *)&update_title,
			TCL_LINK_STRING|TCL_LINK_READ_ONLY); 
	parse_period(uflag,&gui_every_nevents,&gui_every_nusecs,update_title);
	Tcl_UpdateLinkedVar(tcl_interp, "UPDATE_TITLE");
	if(gui_every_nusecs != 0) {
	    internal_event(SCHED_UPDATEGUI,EV_NULL,UNKNOWN,gui_every_nusecs,0);
	    ++min_nqueued;
	}
	TCLTK_createcommand("update_speed", update_speed);
	TCLTK_createcommand("scheduler_speed", scheduler_speed);
    }
#endif

//  INSERT AN EVENT TO PERIODICALLY DO THINGS
    if(fflag) {
	parse_period(fflag,&stats_every_nevents,&stats_every_nusecs,NULL);
	if(stats_every_nusecs != 0) {
	    internal_event(SCHED_PERIODIC,EV_NULL,UNKNOWN,stats_every_nusecs,0);
	    ++min_nqueued;
	}
    }

//  AND FINALLY, AN EVENT TO FINISH THE SIMULATION
    parse_period(eflag, &finish_after_nevents, &finish_after_nusecs, NULL);
    if(finish_after_nusecs != 0) {
	internal_event(SCHED_GAMEOVER, EV_NULL, UNKNOWN,
			finish_after_nusecs+(CnetTime)1, 0);
	++min_nqueued;
    }
}


// ------------------------------------------------------------------------

#if	defined(USE_TCLTK) || defined(USE_IPOD)
bool	debug_pressed		= false;
#endif

static void do_periodically(void)
{
    extern void	flush_allstats(bool);

//  PERIODICALLY REPORT SCHEDULER CONTEXT TO STDERR
    if(dflag) {
	REPORT("\tusec=%s contexts=%5d events=%6d nqueued=%3d\n",
		  CNET_format64(MICROSECONDS), cntx, events_handled, nqueued);
	events_handled	= 0;
	cntx		= 0;
    }
//  PERIODICALLY REPORT STATISTICS TO STDOUT
    if(sflag)
	flush_allstats(false);

//  INVOKE THE EV_PERIODIC HANDLER FOR ANY INTERESTED NODE
    for(int n=0 ; n<_NNODES ; ++n) {
#if	!defined(USE_IPOD)
	THISNODE	= n;
#endif
	NP	= &NODES[THISNODE];
	HANDLER(EV_PERIODIC, NULLTIMER, NP->data[(int)EV_PERIODIC]);
    }
}

#if	defined(USE_TCLTK)
static void update_GUI(void)		// only called if -W set
{
    extern void	flush_GUIstats(void);

    flush_GUIstats();			// report statistics to TCL/TK GUI

//  INVOKE THE EV_UPDATEGUI HANDLER FOR ANY INTERESTED NODE
    for(int n=0 ; n<_NNODES ; ++n) {
#if	!defined(USE_IPOD)
	THISNODE	= n;
#endif
	NP	= &NODES[THISNODE];
	HANDLER(EV_UPDATEGUI, NULLTIMER, NP->data[(int)EV_UPDATEGUI]);
    }
}

// ------------------------------------------------------------------------

static	double	delaymult	= 1.0;

static TCLTK_COMMAND(scheduler_speed)
{
    int	n	= atoi(argv[1]);
    if(n < 1 || n > N_SPEEDS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid speed #", -1));
	return TCL_ERROR;
    }

    Tflag	= (n == N_SPEEDS);
    if(n == NORMAL_SPEED)
	delaymult = 1.0;
    else if(n < NORMAL_SPEED)
	delaymult = (1<<(NORMAL_SPEED - n));
    else	// n > NORMAL_SPEED
	delaymult = 1.0/(1<<(n - NORMAL_SPEED));
    return TCL_OK;
}

#if	CNET_PROVIDES_WLANS
void schedule_drawwlansignal(int thiswan)
{
    internal_event(SCHED_DRAWWLANSIGNAL, EV_NULL, THISNODE,
		  (CnetTime)DELAY_DRAW_WLANSIGNAL, (CnetData)thiswan);
}
#endif

#if	CNET_PROVIDES_WANS
void schedule_moveframe(CnetTime usec, CnetData data)
{
    internal_event(SCHED_MOVEFRAME, EV_NULL, THISNODE, usec, data);
}
#endif

static void PING_TCLTK(void)
{
    static CnetTime	time_next_ping	= 0;

    struct timeval	NOW;
    CnetTime		now_usecs;

    gettimeofday(&NOW, NULL);
    now_usecs	= (CnetTime)NOW.tv_sec*(CnetTime)1000000 + NOW.tv_usec;

    if(now_usecs > time_next_ping) {
	tcltk_notify_dispatch();

	time_next_ping	= now_usecs + PING_TCLTK_FREQ;
    }
}
#endif

static void advance_time(CnetTime newMICROSECONDS)
{
/*  WE ADVANCE TIME TO THE TIME OF THE NEXT PENDING EVENT.
    WE EITHER DO THIS IMMEDIATELY OR MORE SLOWLY VIA A SERIES OF SHORT SLEEPs.
    WE USE THIS OPPORTUNITY TO SEE IF ANY TCL/TK EVENTS NEED PROCESSING. */

    if(Tflag) {
#if	defined(USE_TCLTK)
	if(Wflag)
	    PING_TCLTK();
#endif
	;
    }
    else {
	struct timeval	timeout;
	CnetTime	usecs = newMICROSECONDS - MICROSECONDS;

#if	defined(USE_TCLTK)
	usecs		*= delaymult;
	debug_pressed	 = false;
#endif

	while(usecs > MIN_SELECT) {
#if	defined(USE_TCLTK)
	    if(Wflag) {
		PING_TCLTK();
		if(debug_pressed) {	// break from sleeping
		    newMICROSECONDS = MICROSECONDS + 1;
		    break;
		}
	    }
#endif
	    if(usecs > EACH_SELECT) {
		timeout.tv_sec	= 0;
		timeout.tv_usec	= EACH_SELECT;
		usecs		-= EACH_SELECT;
	    }
	    else {
		timeout.tv_sec	= usecs / (int64_t)1000000;
		timeout.tv_usec	= usecs % (int64_t)1000000;
		usecs		= 0;
	    }
	    select(0, NULL, NULL, NULL, &timeout);
	}
    }
    MICROSECONDS	= newMICROSECONDS;
}

// ------------------------------------------------------------------------

void schedule(bool singlestep)
{
    EVENT		*ev;
    SCHEDEVENT		se;
    CnetEvent		ne;
    CnetTimerID		timer;
    CnetData		data;
#if	!defined(USE_IPOD)
    CnetTime		evtime;
#endif

//  KEEP PULLING NEW EVENTS UNTIL NO MORE OR UNTIL SIMULATION FINISHES
    while(cnet_state == STATE_RUNNING && (ev = get_next_event())) {

//  ADVANCE THE VALUE OF MICROSECONDS
	advance_time(ev->usec);

//  DELIVER NEXT EVENT FROM THE EVENT QUEUE
#if	!defined(USE_IPOD)
	THISNODE	= ev->node;
#endif
	NP		= &NODES[THISNODE];
	se		= ev->se;
	ne		= ev->ne;
	timer		= ev->timer;
	data		= ev->data;
	FREE(ev);

	switch (se) {

	case SCHED_TIMER : {
	    if(NP->runstate == STATE_PAUSED)
		internal_event(se,ne,THISNODE,NP->resume_time-MICROSECONDS+1,0);
	    else
		HANDLER(ne, timer, data);
	    break;
	}				// end of SCHED_TIMER

	case SCHED_FRAMEARRIVAL : {
	    extern bool prepare4framearrival(FRAMEARRIVAL *arrival);

	    if(prepare4framearrival((FRAMEARRIVAL *)data))
		HANDLER(EV_PHYSICALREADY, NULLTIMER,
				NP->data[(int)EV_PHYSICALREADY]);
	    break;
	}				// end of SCHED_FRAMEARRIVAL

#if	CNET_PROVIDES_APPLICATION_LAYER
	case SCHED_POLLAPPL : {
#if	defined(USE_IPOD)
	    HANDLER(EV_APPLICATIONREADY, NULLTIMER, data);
#else
	    if(NP->runstate == STATE_PAUSED)
		internal_event(SCHED_POLLAPPL, EV_APPLICATIONREADY, THISNODE,
				NP->resume_time-MICROSECONDS+(CnetTime)1, 0);
	    else {
		extern int	poll_application(CnetTime *);

		if(poll_application(&evtime))
		    HANDLER(EV_APPLICATIONREADY, NULLTIMER, data);
		internal_event(SCHED_POLLAPPL, EV_APPLICATIONREADY, THISNODE,
				evtime, 0);
	    }
#endif
	    break;
	}				// end of SCHED_POLLAPPL
#endif

#if	CNET_PROVIDES_LANS || CNET_PROVIDES_WLANS
#if	!defined(USE_IPOD)
	case SCHED_FRAMECOLLISION : {
	    extern bool prepare4framecollision(FRAMEARRIVAL *arrival);

	    FRAMEARRIVAL	*arr	= (FRAMEARRIVAL *)data;

	    if(prepare4framecollision(arr))
		HANDLER(EV_FRAMECOLLISION, NULLTIMER, (CnetData)arr->destlink);
	    break;
	}				// end of SCHED_FRAMECOLLISION
#endif
#endif

#if	CNET_PROVIDES_KEYBOARD
	case SCHED_KEYBOARDREADY : {
	    HANDLER(ne, timer, data);
	    break;
	}				// end of SCHED_KEYBOARDREADY
#endif

#if	defined(USE_TCLTK) || defined(USE_IPOD)
	case SCHED_DEBUGBUTTON : {
	    int	savequiet1		= NP->nattr.stdio_quiet;
	    int	savequiet2		= DEF_NODEATTR.stdio_quiet;

	    NP->nattr.stdio_quiet	= false;
	    DEF_NODEATTR.stdio_quiet	= false;
		HANDLER(ne, timer, data);
	    DEF_NODEATTR.stdio_quiet	= savequiet2;
	    NP->nattr.stdio_quiet	= savequiet1;
	    break;
	}				// end of SCHED_DEBUGBUTTON
#endif

#if	CNET_PROVIDES_WANS
	case SCHED_LINKSTATE : {
	    extern void report_linkstate(int);

	    int		w	= data;	// WAN # that just changed
	    WAN		*wanp	= &WANS[w];

	    if(wanp->up) {
		wanp->up = false;
		unschedule_link(wanp->minnode, wanp->minnode_nic);
		unschedule_link(wanp->maxnode, wanp->maxnode_nic);
		evtime	= link_mttr(wanp);
		if(evtime != 0)
		    newevent(EV_LINKSTATE, wanp->minnode,
				    evtime, NULLTIMER, (CnetData)w);
#if	defined(USE_TCLTK)
		if(wanp->ndrawframes != 0) {
		    extern void	forget_drawframes(int w);

		    forget_drawframes(w);
		}
#endif
	    }
	    else {
		wanp->up = true;
		evtime	= link_mtbf(wanp);
		if(evtime != 0)
		    newevent(EV_LINKSTATE, wanp->minnode,
				    evtime, NULLTIMER, (CnetData)w);
	    }
	    if(dflag)
		REPORT("%s->%s.%s\n",
			NODES[wanp->minnode].nodename,
			NODES[wanp->maxnode].nodename,
			wanp->up ? "up" : "down" );
	    report_linkstate(w);
	}
	    break;			// end of SCHED_LINKSTATE

#if	defined(USE_TCLTK)
	case SCHED_DRAWFRAME : {
	    extern void		add_drawframe(DRAWFRAME *df);
	    DRAWFRAME		*df	= (DRAWFRAME *)data;

//  DOES THIS NODE HAVE AN EV_DRAWFRAME HANDLER?
	    if(NP->handler[(int)EV_DRAWFRAME])
		HANDLER(EV_DRAWFRAME, NULLTIMER, (CnetData)&df->cdf);
	    else {
//  PROVIDE A SIMPLE, DEFAULT EV_DRAWFRAME HANDLER
		sprintf(df->cdf.text, "%zd", df->cdf.len);
		df->cdf.colours[0]	= DRAWFRAME_COLOUR;
		df->cdf.pixels[0]	= 30;
		df->cdf.pixels[1]	= 0;
	    }
	    add_drawframe(df);
	    break;
	}				// end of SCHED_DRAWFRAME

	case SCHED_MOVEFRAME : {
	    extern void	 move_drawframe(CnetData);

	    move_drawframe(data);
	    break;
	}				// end of SCHED_MOVEFRAME
#endif					// USE_TCLTK
#endif					// CNET_PROVIDES_WANS

#if	defined(USE_TCLTK) && CNET_PROVIDES_WLANS
	case SCHED_DRAWWLANSIGNAL : {
	    extern bool	 draw_wlansignal(int wlan);

	    if(draw_wlansignal((int)data))
		schedule_drawwlansignal((int)data);
	    break;
	}				// end of SCHED_DRAWWLANSIGNAL
#endif

//  INFREQUENT EVENTS FOR ALL REMAINING POSSIBILITIES
	case SCHED_NODESTATE : {
	    switch (NP->runstate) {

	    case STATE_AUTOREBOOT :
		invoke_shutdown(THISNODE);
		invoke_reboot(THISNODE);
		break;

	    case STATE_REBOOTING :
		invoke_reboot(THISNODE);
		break;

#if	!defined(USE_IPOD)
	    case STATE_UNDERREPAIR :
		if(dflag)
		    REPORT("%s.repaired\n",NP->nodename);
		// drop through
	    case STATE_CRASHED :
		invoke_reboot(THISNODE);
		break;

	    case STATE_PAUSED :
		NP->runstate	= STATE_RUNNING;
		if(dflag)
		    REPORT("%s.resumed\n",NP->nodename);
		break;

	    case STATE_RUNNING :
		NP->runstate	= STATE_UNDERREPAIR;
		if(dflag)
		    REPORT("%s.underrepair\n",NP->nodename);

		unschedule_node(THISNODE);
		evtime = node_mttr();
		if(evtime != 0)
		    internal_event(SCHED_NODESTATE,EV_REBOOT,THISNODE,evtime,0);
		break;
#endif

	     default:
		break;
	    }				// end of switch (NP->runstate)
#if	defined(USE_TCLTK)
	    if(Wflag)
		draw_node_icon(THISNODE, NULL, 0, 0);
#endif
	    break;
	}				// end of case SCHED_NODESTATE

#if	defined(USE_TCLTK)
	case SCHED_UPDATEGUI : {
	    update_GUI();
	    internal_event(SCHED_UPDATEGUI,EV_NULL,UNKNOWN,gui_every_nusecs,0);
	    break;
	}				// end of SCHED_UPDATEGUI
#endif

	case SCHED_PERIODIC : {
	    do_periodically();
	    internal_event(SCHED_PERIODIC,EV_NULL,UNKNOWN,stats_every_nusecs,0);
	    break;
	}				// end of SCHED_PERIODIC

	case SCHED_GAMEOVER :
	    --MICROSECONDS;
	    cnet_state	= STATE_GAMEOVER;
	    goto done;
	    break;

	default:
	    FATAL("unknown value of SE in global event queue");
	    break;
	}				// end of switch( SCHED_? )

//  CHECK FOR ACTIONS PERFORMED AFTER A CERTAIN NUMBER OF EVENTS 
	if(stats_every_nevents != 0 &&
	  (gattr.nodestats.nevents % stats_every_nevents) == 0)
	    do_periodically();

	if(singlestep)
	    break;

	if(gattr.nodestats.nevents == finish_after_nevents) {
	    cnet_state	= STATE_GAMEOVER;
	    break;
	}

#if	defined(USE_TCLTK)
	if(gui_every_nevents != 0 &&
	  (gattr.nodestats.nevents % gui_every_nevents) == 0)
	    update_GUI();
#endif

#if	!defined(USE_IPOD)
	if(nqueued == min_nqueued) {
	    WARNING("no more events are scheduled\n");
	    min_nqueued	= (1<<30);
	}
#endif
    }

done:
#if	defined(USE_TCLTK)
    if(Wflag)
	update_GUI();
#endif
    fflush(stdout);
}


// ------------------------------------------------------------------------


// DOESN'T REALLY BELONG IN THIS FILE, BUT NEEDS BIGBANG!
int CNET_set_time_of_day(long newsec, long newusec)
{
    int		result	= -1;
    CnetTime	delta;

    if(newsec < 0 || newusec < 0 || newusec >= 1000000)
	ERROR(ER_BADARG);
    else {
	delta = (CnetTime)newsec*(CnetTime)1000000 + newusec;
	NODES[THISNODE].clock_skew	= delta - (BIGBANG + MICROSECONDS);

	nodeinfo.time_of_day.sec	= newsec;
	nodeinfo.time_of_day.usec	= newusec;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(newsec=%ld, newusec=%ld) = 0\n",
		    __func__, newsec, newusec);
	else
	    TRACE(1, "\t%s(newsec=%ld, newusec=%ld) = -1 %s\n",
		    __func__, newsec, newusec, cnet_errname[(int)cnet_errno]);
    }
    return result;
}
