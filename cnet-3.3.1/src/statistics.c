#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

static const char *stats_titles[N_STATS] = {
	"Simulation time",
	"Events raised",
	"API errors",
	"",
	"Messages generated",
	"Messages delivered",
	"Messages incorrect",
	"Message bandwidth",
	"Average delivery time",
	"",
	"Frames transmitted",
	"Frames received",
	"Frames corrupted",
	"Frames lost",
	"Frame collisions",
	"",
	"Efficiency (bytes AL) / (bytes PL)",
	"Transmission cost"
};

#define	Xh(x)	diff_hiddenstats.x = gattr.hiddenstats.x - prev_hiddenstats.x
#define	Xn(x)	diff_nodestats.x   = gattr.nodestats.x   - prev_nodestats.x
#define	Xl(x)	diff_linkstats.x   = gattr.linkstats.x   - prev_linkstats.x

#define	calc_diffstats()	Xh(tx_frames_lost);	\
				Xh(msgs_deliverytime);	\
				Xh(msgs_write_errors);	\
				Xn(nevents);		\
				Xn(nerrors);		\
				Xn(msgs_generated);	\
				Xn(bytes_generated);	\
				Xn(msgs_received);	\
				Xn(bytes_received);	\
				Xl(tx_frames);		\
				Xl(tx_bytes);		\
				Xl(tx_cost);		\
				Xl(rx_frames);		\
				Xl(rx_bytes);		\
				Xl(rx_frames_corrupted); \
				Xl(rx_frames_collisions)

// --------------- GLOBAL STATS INFORMATION FIRST ---------------------


static	bool	iflag	= false;
static	bool	zflag	= false;


//  REPORT STATISTICS TO STDOUT, PERIODICALLY OR ON TERMINATION
static void stats_listing(CnetTime usecs, CnetHiddenStats *hs,
			    CnetNodeStats *ns, CnetLinkStats *ls)
{
    int64_t	ivalue;
    double	xvalue;
    int		t=0;

#define	OUTI(i)	if((i) != 0 || zflag) \
	    F(stdout, "%-36s: %" PRId64 "\n", stats_titles[t], i); ++t
#define	OUTD(x)	if((x) != 0.0 || zflag) \
	    F(stdout, "%-36s: %.2f\n", stats_titles[t], x); ++t

//  GLOBAL STATISTICS
    OUTI(usecs);
    OUTI(ns->nevents);
    OUTI(ns->nerrors);
    ++t;

//  APPLICATION LAYER STATISTICS
    OUTI(ns->msgs_generated);
    OUTI(ns->msgs_received);
    OUTI(hs->msgs_write_errors);
    if(hs->msgs_deliverytime != 0) {
	ivalue	= (ns->bytes_received*(int64_t)8000000) / hs->msgs_deliverytime;
	OUTI(ivalue);
    }
    else {
	OUTI((int64_t)0);
    }
    if(ns->msgs_received != 0) {
	ivalue	= (int)(hs->msgs_deliverytime / ns->msgs_received);
	OUTI(ivalue);
    }
    else {
	OUTI((int64_t)0);
    }
    ++t;

//  PHYSICAL LAYER STATISTICS
    OUTI(ls->tx_frames);
    OUTI(ls->rx_frames);
    OUTI(ls->rx_frames_corrupted);
    OUTI(hs->tx_frames_lost);
    OUTI(ls->rx_frames_collisions);
    ++t;

//  EFFICIENCY AND COST
    if(ls->tx_bytes != 0) {
	xvalue	= 100.0 * ((double)ns->bytes_received / (double)ls->tx_bytes);
	OUTD(xvalue);
    }
    else {
	OUTD(0.0);
    }
    OUTI(ls->tx_cost);

#undef	OUTD
#undef	OUTI

    for(t=0 ; t<N_CNET_EVENTS ; ++t)
	if(hs->event_count[t] != 0 || zflag)
	    F(stdout, "%-36s: %" PRId64 "\n",
				cnet_evname[t], hs->event_count[t]);
    fflush(stdout);
}

//  REPORT STATISTICS TO STDOUT, PERIODICALLY OR ON TERMINATION
void flush_allstats(bool header)
{
//  DISPLAYING INCREMENTAL STATISTICS?
    if(iflag) {
	static CnetHiddenStats	prev_hiddenstats	= {0};
	static CnetNodeStats	prev_nodestats		= {0};
	static CnetLinkStats	prev_linkstats		= {0};
	static CnetTime		prev_usecs		= 0;

	CnetHiddenStats	diff_hiddenstats;
	CnetNodeStats	diff_nodestats;
	CnetLinkStats	diff_linkstats;

	calc_diffstats();
	for(int n=0 ; n<N_CNET_EVENTS ; ++n)
	    Xh(event_count[n]);

	stats_listing(MICROSECONDS - prev_usecs,
			&diff_hiddenstats, &diff_nodestats, &diff_linkstats);

	prev_hiddenstats	= gattr.hiddenstats;
	prev_nodestats		= gattr.nodestats;
	prev_linkstats		= gattr.linkstats;
	prev_usecs		= MICROSECONDS;
    }
//  OTHERWISE, DISPLAYING CUMMULATIVE STATISTICS
    else {
	if(header)
	    F(stdout,
		    "%i host%s, %i router%s, %i mobile%s, %i accesspoint%s\n",
			gattr.nhosts,		PLURAL(gattr.nhosts),
			gattr.nrouters,		PLURAL(gattr.nrouters),
			gattr.nmobiles,		PLURAL(gattr.nmobiles),
			gattr.naccesspoints,	PLURAL(gattr.naccesspoints) );
	stats_listing(MICROSECONDS,
			&gattr.hiddenstats, &gattr.nodestats, &gattr.linkstats);
    }
}


// ----------------- TCLTK windowing-specific stats follows ----------------

#if	defined(USE_TCLTK)

static	int		stats_displayed		= false;
static	int		events_displayed	= false;


//  FORMAT AN INTEGER VALUE
static char *FMTI(char *p, int64_t i)
{
    *p++	= ' ';
    if(i == 0)
	*p++ = '-';
    else {
	strcpy(p, CNET_format64(i));
	while(*p)
	    ++p;
    }
    return(p);
}

//  FORMAT A DOUBLE FLOATING VALUE
static char *FMTD(char *p, double x)
{
    *p++	= ' ';
    if(x == 0.0)
	*p++ = '-';
    else {
	sprintf(p, "%.2f", x);
	while(*p)
	    ++p;
    }
    return(p);
}

static char *fmt_mainstats(char *p, CnetTime usecs,
		    CnetHiddenStats *hs, CnetNodeStats *ns, CnetLinkStats *ls)
{
    *p++	= ' ';
    *p++	= '{';
//  GLOBAL STATISTICS
    p	= FMTI(p, (int64_t)usecs);
    p	= FMTI(p, ns->nevents);
    p	= FMTI(p, ns->nerrors);

//  BLANK LINE
    *p++ = ' '; *p++ = '"'; *p++ = '"';

//  APPLICATION LAYER STATISTICS
    p	= FMTI(p, ns->msgs_generated);
    p	= FMTI(p, ns->msgs_received);
    p	= FMTI(p, hs->msgs_write_errors);
    if(hs->msgs_deliverytime != 0)
	p = FMTI(p,
		(ns->bytes_received*(int64_t)8000000) / hs->msgs_deliverytime);
    else
	p = FMTI(p, 0);
    if(ns->msgs_received != 0)
	p = FMTI(p, (int)(hs->msgs_deliverytime / ns->msgs_received));
    else
	p = FMTI(p, 0);

//  BLANK LINE
    *p++ = ' '; *p++ = '"'; *p++ = '"';

//  PHYSICAL LAYER STATISTICS
    p	= FMTI(p, ls->tx_frames);
    p	= FMTI(p, ls->rx_frames);
    p	= FMTI(p, ls->rx_frames_corrupted);
    p	= FMTI(p, hs->tx_frames_lost);
    p	= FMTI(p, ls->rx_frames_collisions);

//  BLANK LINE
    *p++ = ' '; *p++ = '"'; *p++ = '"';

//  EFFICIENCY AND COST
    if(ls->tx_bytes != 0)
	p = FMTD(p, 100.0*((double)ns->bytes_received / (double)ls->tx_bytes));
    else
	p = FMTD(p, 0.0);
    p	= FMTI(p, ls->tx_cost);

    *p++	= ' ';
    *p++	= '}';
    *p		= '\0';
    return(p);
}

static void flush_statswindow(void)
{
    static CnetHiddenStats	prev_hiddenstats	= {0};
    static CnetNodeStats	prev_nodestats		= {0};
    static CnetLinkStats	prev_linkstats		= {0};
    static CnetTime		prev_usecs		= 0;
    static int64_t		prev_tmsgs		= 0;

    int64_t			tmsgs;

    if(stats_displayed) {
	CnetHiddenStats	diff_hiddenstats;
	CnetNodeStats	diff_nodestats;
	CnetLinkStats	diff_linkstats;

	char		cmdbuf[BUFSIZ], *p=cmdbuf;

	calc_diffstats();
	sprintf(p, "updateStats %i", N_STATS);
	while(*p)
	    ++p;
	p = fmt_mainstats(p, MICROSECONDS - prev_usecs,
			&diff_hiddenstats, &diff_nodestats, &diff_linkstats);
	p = fmt_mainstats(p, MICROSECONDS,
			&gattr.hiddenstats,&gattr.nodestats,&gattr.linkstats);

	TCLTK(cmdbuf);
    }

    sprintf(chararray,"%susec", CNET_format64(MICROSECONDS));
    Tcl_SetVar(tcl_interp, "CN_TIMENOW", chararray, TCL_GLOBAL_ONLY);

    tmsgs = gattr.nodestats.msgs_received + gattr.hiddenstats.msgs_write_errors;
    if(tmsgs != prev_tmsgs) {
	double		ratio = 100.0;

	if(gattr.hiddenstats.msgs_write_errors != 0)
	    ratio = (double)gattr.nodestats.msgs_received / (double)tmsgs*100.0;
	sprintf(chararray,"%10s (%5.1f%%)  ",
		CNET_format64(gattr.nodestats.msgs_received), ratio);
	Tcl_SetVar(tcl_interp, "CN_DELIVERY", chararray, TCL_GLOBAL_ONLY);
	prev_tmsgs	= tmsgs;
    }

    prev_hiddenstats	= gattr.hiddenstats;
    prev_nodestats	= gattr.nodestats;
    prev_linkstats	= gattr.linkstats;
    prev_usecs		= MICROSECONDS;
}

// ----------------------------------------------------------------------

static void flush_eventswindow(void)
{
    static int64_t	PREV_EV_COUNT[N_CNET_EVENTS]	= {0};

    if(events_displayed) {
	int64_t		total	= 1;
	char		cmdbuf[BUFSIZ], *p;

	for(int n=0 ; n<N_CNET_EVENTS ; ++n)
	    total += (gattr.hiddenstats.event_count[n] - PREV_EV_COUNT[n]);

	sprintf(p = cmdbuf, "updateEvents %i %" PRId64, N_CNET_EVENTS, total);
	while(*p)
	    ++p;
	*p++	= ' ';
	*p++	= '{';
	for(int n=0 ; n<N_CNET_EVENTS ; ++n) {
	    sprintf(p, " %" PRId64,
			gattr.hiddenstats.event_count[n] - PREV_EV_COUNT[n]);
	    while(*p)
		++p;
	}
	*p++	= '}';
	*p	= '\0';
	TCLTK(cmdbuf);
    }
    memcpy(PREV_EV_COUNT, gattr.hiddenstats.event_count, sizeof(PREV_EV_COUNT));
}

void init_eventswindow(void)
{
    for(int n=0 ; n<N_CNET_EVENTS ; ++n) {
	sprintf(chararray, "cnet_evname(%i)", n);
	Tcl_SetVar(tcl_interp,chararray,cnet_evname[n],TCL_GLOBAL_ONLY);
    }
    Tcl_LinkVar(tcl_interp, "events_displayed",
				(char *)&events_displayed, TCL_LINK_BOOLEAN);
}

// --------------- Per-node TCL/TK stats code follows ----------------

static void flush_nodestats(void)
{
#define	NSi(x)	if(ns->x == 0) *p++ = '-';	\
		else { strcpy(p, CNET_format64(ns->x)); while(*p) ++p; } \
		*p++ = ' '
#define	NSt(x)	if(ns->x == 0) *p++ = '-';	\
		else { sprintf(p, "%.2f", \
				(ns->x/1024.0) / (MICROSECONDS/1000000.0)); \
			while(*p) ++p; } \
		*p++ = ' '

    NODE	*np;
    int		n;

    FOREACH_NODE {
	if(np->nodetype != NT_HOST && np->nodetype != NT_MOBILE)
	    continue;

	if(np->displayed && np->stats_changed) {
	    CnetNodeStats	*ns	= &np->nodestats;
	    char		cmdbuf[BUFSIZ], *p = cmdbuf;

	    sprintf(p, "updateNodeStats %i {", n);
	    while(*p)
		++p;

	    NSi(msgs_generated);
	    NSi(bytes_generated);
	    NSt(bytes_generated);

	    NSi(msgs_received);
	    NSi(bytes_received);
	    NSt(bytes_received);

	    NSi(nerrors);

	    *p++	= '}';
	    *p		= '\0';
	    TCLTK(cmdbuf);
	    np->stats_changed	= false;
	}

#if	CNET_PROVIDES_WLANS
	if(np->nwlans > 0) {
	    extern void flush_wlan_history(NODE *np);

	    flush_wlan_history(np);
	}
#endif
    }
#undef	NSt
#undef	NSi
}

// ------------------ PER-NIC TCL/TK STATS CODE FOLLOWS -------------

static void flush_1_nicstat(CnetLinkType t, CnetLinkStats *ls, int from, int to)
{
#define	LSi(x)	if(ls->x == 0) *p++ = '-';	\
		else { strcpy(p, CNET_format64(ls->x)); while(*p) ++p; } \
		*p++ = ' '
#define	LSt(x)	if(ls->x == 0) *p++ = '-';	\
		else { sprintf(p, "%.2f", \
				(ls->x/1024.0) / (MICROSECONDS/1000000.0)); \
			while(*p) ++p; } \
		*p++ = ' '

    char	cmdbuf[BUFSIZ], *p;

    p	= cmdbuf;
    switch (t) {
    case LT_WAN :
	sprintf(p, "updateNICstats WAN %i %i {", from, to);
	break;

    case LT_LAN :
	sprintf(p, "updateNICstats LAN %i %i {", from, to);
	break;

    default:
	break;
    }
    while(*p)
	++p;

    LSi(tx_frames);
    LSi(tx_bytes);
    LSt(tx_bytes);

    LSi(rx_frames);
    LSi(rx_bytes);
    LSt(rx_bytes);

    switch (t) {
    case LT_WAN :
	LSi(rx_frames_corrupted);
	break;

    case LT_LAN :
	LSi(rx_frames_collisions);
	break;

    default:
	break;
    }

    *p++	= '}';
    *p		= '\0';
    TCLTK(cmdbuf);
#undef	LSt
#undef	LSi
}

static void flush_nicstats(void)
{
    NICATTR		*nic;
    int			n;
    CnetLinkStats	*ls;

//  FLUSH STATS PANELS OF ALL WANS
#if	CNET_PROVIDES_WANS
    WAN			*wan;
    for(n=0, wan=WANS ; n<gattr.nwans ; ++n, ++wan) {
	NODE	*np	= &NODES[wan->minnode];

	nic	= &np->nics[wan->minnode_nic];
	if(nic->displayed && nic->stats_changed) {
	    ls	= &nic->linkstats;
	    flush_1_nicstat(LT_WAN, ls, wan->minnode, wan->maxnode);
	    nic->stats_changed	= false;
	}

	np	= &NODES[wan->maxnode];
	nic	= &np->nics[wan->maxnode_nic];
	if(nic->displayed && nic->stats_changed) {
	    ls	= &nic->linkstats;
	    flush_1_nicstat(LT_WAN, ls, wan->maxnode, wan->minnode);
	    nic->stats_changed	= false;
	}
    }
#endif

//  FLUSH STATS PANELS OF ALL LAN SEGMENTS
#if	CNET_PROVIDES_LANS
    LAN			*lan;
    for(n=0, lan=LANS ; n<gattr.nlans ; ++n, ++lan) {
	nic	= &lan->segmentnic;
	if(nic->displayed && nic->stats_changed) {
	    ls	= &nic->linkstats;
	    flush_1_nicstat(LT_LAN, ls, n, DEFAULT);
	    nic->stats_changed	= false;
	}

//  FLUSH STATS PANEL OF EACH NIC ON THIS LAN SEGMENT
	int	n1;
	EACHNIC	*each;

	for(n1=0, each=lan->nics ; n1<lan->nnics ; ++n1, ++each) {
	    nic	= &NODES[each->node].nics[each->nodeslink];     
	    if(nic->displayed && nic->stats_changed) {
		ls	= &nic->linkstats;
		flush_1_nicstat(LT_LAN, ls, n, n1);
		nic->stats_changed	= false;
	    }
	}
    }
#endif
}

//  REPORT STATISTICS TO TCL/TK GUI PERIODICALLY
void flush_GUIstats(void)
{
    flush_statswindow();
    flush_eventswindow();
    flush_nodestats();
    flush_nicstats();
}

void init_statswindows(void)
{
//  THE MAIN STATISTICS
    for(int n=0 ; n<N_STATS ; ++n) {
	sprintf(chararray, "STATS_TITLES(%i)", n);
	Tcl_SetVar(tcl_interp,chararray,stats_titles[n],TCL_GLOBAL_ONLY);
    }
    Tcl_LinkVar(tcl_interp, "stats_displayed",
				(char *)&stats_displayed, TCL_LINK_BOOLEAN);

//  THE EVENT STATISTICS
    for(int n=0 ; n<N_CNET_EVENTS ; ++n) {
	sprintf(chararray, "cnet_evname(%i)", n);
	Tcl_SetVar(tcl_interp,chararray,cnet_evname[n],TCL_GLOBAL_ONLY);
    }
    Tcl_LinkVar(tcl_interp, "events_displayed",
				(char *)&events_displayed, TCL_LINK_BOOLEAN);
}
#endif	// defined(USE_TCLTK)


// -----------------------------------------------------------------------

void init_stats_layer(bool i, bool z)
{
    iflag = i;
    zflag = z;

    memset(&gattr.nodestats,	0, sizeof(CnetNodeStats));
    memset(&gattr.linkstats,	0, sizeof(CnetLinkStats));
    memset(&gattr.hiddenstats,	0, sizeof(CnetHiddenStats));
}
