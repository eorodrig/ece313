#if	!defined(_CNETPRIVATE_H_)
#define	_CNETPRIVATE_H_		1

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "preferences.h"	// installation dependent preferences
#include "hidenames.h"		// renames important global symbols
#include "cnet.h"		// this is the students' header file
#include "mt19937.h"		// Mersenne Twister random number generator

#if	defined(USE_LINUX)
#define	NDATASEGS		2
#define	PREPEND_DOT_TO_LDPATH

#elif	defined(USE_IPOD)
#undef	_NNODES
#undef	THISNODE
#define	_NNODES			1
#define	THISNODE		0
#define	NDATASEGS		0
#define	WLAN_LINK		1

#elif   defined(USE_MACOSX)
#define NDATASEGS               1
#ifndef OS_INLINE
#define OS_INLINE		static __inline__
#endif
#undef	PRId64
#define	PRId64			"lld"

#elif	defined(USE_SOLARIS)
#define	NDATASEGS		1
#define	PREPEND_DOT_TO_LDPATH

#elif	defined(USE_FREEBSD)
#define	NDATASEGS		1
#define	PREPEND_DOT_TO_LDPATH
 
#elif	defined(USE_NETBSD)
#define	NDATASEGS		2
#define	PREPEND_DOT_TO_LDPATH
 
#elif	defined(USE_IRIX5)
#define	NDATASEGS		2
#define	PREPEND_DOT_TO_LDPATH
#define	UNLINK_SO_LOCATIONS

#elif	defined(USE_OSF1)
#define	NDATASEGS		4
#define	UNLINK_SO_LOCATIONS

#elif	defined(USE_SUNOS)
#define	NDATASEGS		1

#endif

#if   !defined(USE_MACOSX)
extern	char	*strdup(const char *);
extern	int	strcasecmp(const char *, const char *);
#endif

#define	NODE_HAS_AL(n)		(NODES[n].nodetype == NT_HOST || \
			 	 NODES[n].nodetype == NT_MOBILE)
#define	NODE_HAS_KB(n)		(NODES[n].nodetype == NT_HOST || \
			 	 NODES[n].nodetype == NT_MOBILE)
#define	NODE_HAS_WANS(n)	(NODES[n].nodetype != NT_MOBILE)
#define	NODE_HAS_LANS(n)	(NODES[n].nodetype != NT_MOBILE)
#define	NODE_HAS_WLANS(n)	(NODES[n].nodetype == NT_HOST || \
				 NODES[n].nodetype == NT_MOBILE || \
				 NODES[n].nodetype == NT_ACCESSPOINT)
#define	NODE_HAS_MOBILITY(n)	(NODES[n].nodetype == NT_MOBILE)

#define	N_STATS			18

// ---------------------------------------------------------------------

#if	defined(USE_TCLTK)
#include "tcltkfunctions.h"

extern	void	draw_node_icon(int n, char *canvas, int x, int y);

#elif	defined(USE_ASCII)

#else
#error	You have not specified a valid windowing system in preferences.h
#endif

// ---------------------------------------------------------------------

#define	PLURAL(n)		((n)==1 ? "" : "s")

#define	F			fprintf
#define	UNKNOWN			(-1)
#define	DEFAULT			(-1)

#define	WHICH(current,def)	((current == DEFAULT) ? def : current)

typedef	enum {
    STATE_RUNNING = 0,	STATE_REBOOTING,	STATE_SHUTDOWN,
    STATE_CRASHED,	STATE_AUTOREBOOT,	STATE_PAUSED,
    STATE_UNDERREPAIR,	STATE_SINGLESTEP,	STATE_GAMEOVER,
    STATE_UNKNOWN
} RUNSTATE;

extern void newevent(CnetEvent ne, int node,
			CnetTime usecs, CnetTimerID timer, CnetData data);


// ------------------------ NIC INFORMATION -----------------------

typedef struct {
    CnetLinkType	linktype;
    CnetNICaddr		nicaddr;
    char		*name;

    CnetTime		tx_until;
    CnetTime		rx_until;
    bool		buffered;
    bool	 	up;

    int			bandwidth;
    CnetTime		jitter;
    CnetTime		propagation;
    int			mtu;

    int			probframecorrupt;	// actual run-time value
    int			probframeloss;

    int			costperbyte;
    int			costperframe;

    CnetTime		linkmtbf;
    CnetTime		linkmttr;

#if	CNET_PROVIDES_LANS
    CnetTime		slottime;		// usecs, only for LT_LAN
#endif
#if	CNET_PROVIDES_WLANS
    WLANINFO		*wlaninfo;		// only for LT_WLAN
#endif

    CnetLinkStats	linkstats;
#if	defined(USE_TCLTK)
    int			displayed;
    bool		stats_changed;
#endif
} NICATTR;

extern void init_nicattrs(NICATTR *defw, NICATTR *defl, NICATTR *defwl);


// ------------------------ WAN INFORMATION -----------------------

#if	CNET_PROVIDES_WANS

#if	defined(USE_TCLTK)
typedef struct {
    int			wan;
    int			src;
    bool		corrupt;
    bool		lost;
    CnetTime		leaves;
    CnetTime		inflight;
    CnetDrawFrame	cdf;
} DRAWFRAME;
#endif

typedef struct {
    bool	 	up;
    int64_t	 	ntxed;

    int		 	minnode;
    int		 	minnode_nic;

    int		 	maxnode;
    int		 	maxnode_nic;
#if	defined(USE_TCLTK)
    bool		drawframes_init;
    int			drawframes_displayed;
    DRAWFRAME		*dfs[MAX_DRAWFRAMES];
    int			ndrawframes;
    int			lhsnode;
    int			rhsnode;
#endif
} WAN;

extern	WAN		*WANS;
extern	NICATTR		DEFAULTWAN;
#endif

// ------------------------ LAN INFORMATION -----------------------

#if	CNET_PROVIDES_LANS
typedef struct {
    int			node;		// index into NODES[]
    int			nodeslink;	// link of this nic on node
    int			ncollisions;
    int			nsuccesses;
} EACHNIC;

typedef struct {
    char		*name;
    int			x, y, x1;	// all LANs drawn horizontally

    NICATTR		segmentnic;	// default attrs of whole segment
    int			nnics;		// number of NICs on segment
    EACHNIC		*nics;		// vector of NICs on segment
} LAN;

extern	LAN		*LANS;
extern	NICATTR		DEFAULTLAN;
#endif

// ------------------------ WLAN INFORMATION -----------------------

#if	CNET_PROVIDES_WLANS
typedef struct {
    int			node;		// node owning this WLAN's nic
    int			node_link;	// index into node's linkinfo[]
    WLANSTATE		state;
    WLANINFO		info;
    double		rx_signal;
    double		rx_angle;

#if	defined(USE_TCLTK)
    CnetColour		signalcolour;
    int			radius;
    int			maxradius;
#endif
} WLAN;

extern	WLAN		*WLANS;
extern	NICATTR		DEFAULTWLAN;
#endif

// ------------------------ NODE INFORMATION -----------------------

typedef struct {
    int64_t		tx_frames_lost;
    int64_t		msgs_deliverytime;
    int64_t		msgs_write_errors;
    int64_t		event_count[N_CNET_EVENTS];
} CnetHiddenStats;

typedef struct {
    CnetAddr		address;

#if	CNET_PROVIDES_APPLICATION_LAYER
    int64_t		messagerate;		// actual run-time value
    int			minmessagesize;
    int			maxmessagesize;
#endif

    char		**vnames;
    char		**vvalues;
    int			nvars;

    CnetTime		nodemtbf;
    CnetTime		nodemttr;

#if	CNET_PROVIDES_MOBILITY
    CnetPosition	position;		// coords in metres
    double		battery_volts;		// only for NT_MOBILE
    double		battery_mAH;		// only for NT_MOBILE
#endif

    char		*icontitle;
    int			iconheight;

    char		*compile;
    char		**reboot_argv;
    char		*rebootfunc;

    bool		stdio_quiet;
    char		*outputfile;

    bool		trace_all;
    int			trace_mask;

    int			winx, winy;
    bool		winopen;
} NODEATTR;


typedef struct {
    CnetNodeType	nodetype;
    RUNSTATE		runstate;
    char		*nodename;

    NODEATTR		nattr;		// actual attributes of this node
    NICATTR		*nics;		// indexed by link we're using
    int			nnics;
    int			nframes;

    int			os_errno;
    CnetError		cnet_errno;

#if	NDATASEGS
    unsigned long	length_data[NDATASEGS];
    char		*incore_data[NDATASEGS];
    char		*private_data[NDATASEGS];
    char		*original_data[NDATASEGS];
#endif

    int			nexttimer;
    CnetTime		reboot_time;
    CnetTime		resume_time;
    CnetTime		clock_skew;
    int			nnewrands;

    int			nwans;		// # of WAN NICs on this node
    int			nlans;		// # of LAN NICs on this node
    int			nwlans;		// # of WLAN NICs on this node

#if	CNET_PROVIDES_WANS
    int			*wans;		// index into WANS[]
    NICATTR		defaultwan;	// default WAN attributes
#endif

#if	CNET_PROVIDES_LANS
    int			*lans;		// index into LANS[]
    NICATTR		defaultlan;	// default LAN attributes
#endif

#if	CNET_PROVIDES_WLANS
    NICATTR		defaultwlan;	// default WLAN attributes
#endif

    void	(*handler[N_CNET_EVENTS])(CnetEvent, CnetTimerID, CnetData);
    CnetData		data[N_CNET_EVENTS];
    int			outputfd;	// to file given by -o

    MT			*mt;		// random sequence used by protocol

    CnetHiddenStats	hiddenstats;
    CnetNodeStats	nodestats;

#if	CNET_PROVIDES_KEYBOARD
    char		*inputline;	// from the keyboard
    size_t		inputlen;
#endif

#if	defined(USE_TCLTK)
    int			displayed;
    bool		stats_changed;
    char		*debug_str[N_CNET_DEBUGS];

#if	CNET_PROVIDES_WLANS
    double       	wlan_rx_history[WLAN_RX_HISTORY];
    int			wlan_index_history;
    int			wlan_prev_nbars;
#endif
#endif
} NODE;

extern	NODE		*NODES;
extern	NODE		*NP;		// always  &NODES[THISNODE]
extern	NODEATTR	DEF_NODEATTR;

#define	FOREACH_NODE	for(n=0, np=NODES ; n < _NNODES ; ++n, ++np)

// ------------------------------------------------------------------

typedef struct {
    int			nhosts;
    int			nwans;
    int			nlans;
    int			nwlans;
    int			nrouters;
    int			naccesspoints;
    int			nmobiles;

    int			Sflag;			// random seed from cmdline

#if	CNET_PROVIDES_WLANS
    bool		drawwlans;
#endif

    CnetPosition	maxposition;		// coords in metres
    int			mapwidth, mapheight;	// coords in pixels
    int			tilewidth, tileheight;	// coords in pixels
    double		mapgrid;
    double		maphex;
    double		mapscale;
    double		mapmargin;		// in pixels

    char		*mapcolour;
    char		*mapimage;
    char		*maptile;

    bool		drawnodes;
    bool		drawlinks;
    bool		showcostperbyte;
    bool		showcostperframe;
    bool		stdio_quiet;
    bool		trace_events;
    char		*trace_filenm;
    FILE		*tfp;

    int			nextensions;
    char		*extensions[MAXEXTENSIONS];

    CnetHiddenStats	hiddenstats;
    CnetNodeStats	nodestats;
    CnetLinkStats	linkstats;

#if	CNET_PROVIDES_MOBILITY
    int			positionerror;		// coords in metres
#endif
} GLOBALATTR;

extern	GLOBALATTR	gattr;

#if	!defined(USE_IPOD)
extern	int		THISNODE;
extern	int		_NNODES;	// used internally by cnet
#endif

extern	CnetEvent	HANDLING;

// ------------- PARSING INFORMATION AND GLOBAL THINGS --------------

typedef struct {
    FILE		*fp;
    char		*name;
    char		line[BUFSIZ];
    int			cc, ll, lc;

    int			ivalue;
    double		dvalue;
    CnetNICaddr		nicaddr;
} INPUT ;

extern	INPUT		input;

extern	char		chararray[];
extern	int		nerrors;

extern	RUNSTATE	cnet_state;

extern	CnetTime	MICROSECONDS;

extern	char		*argv0;

extern	bool		dflag;		// turn on debugging/diagnostics
extern	bool		qflag;		// quiet (except during EV_DEBUGs)
extern	bool		Wflag;		// run under the windowing env.
extern	int		vflag;		// be verbose about cnet's actions

extern	const char	*findenv(const char *, const char *);
extern	char		*find_cnetfile(const char *fm, int wantdir, bool fatal);
extern	char		*find_trace_name(const void *addr);
extern	char		*format_nodeinfo(NODE *np, const char *str);

#if	CNET_PROVIDES_KEYBOARD
extern	void		set_keyboard_input(int n, const char *line);
#endif

extern	CnetTime	poisson_usecs(CnetTime, MT *);

extern	void		CLEANUP(int);			// calls exit(int)
extern	void		ERROR(CnetError err);		// sets cnet_errno
extern	void		FATAL(const char *fmt, ...);	// calls CLEANUP(1)
extern	void		REPORT(const char *fmt, ...);	// IFF dflag or vflag
extern	void		TRACE(int result, const char *fmt, ...);
extern	void		WARNING(const char *fmt, ...);

#if	defined(USE_IPOD)
extern	void		popupAlert(char *msg);
#endif

extern	void	clone_node_vars(NODEATTR *na);
extern	void	set_node_var(NODEATTR *na, const char *name, const char *value);

// ----------------------------------------------------------------------

#define NEW(type)	calloc((unsigned)1, sizeof(type))
#define FREE(ptr)	do { if(ptr) { free(ptr); ptr = NULL; } } while(false)
#define CHECKALLOC(p)	do { if((p) == NULL) { F(stderr, "allocation failed - %s:%s(), line %d\n", __FILE__, __func__, __LINE__); exit(2); }} while(false)

#ifndef	M_PI
#define	M_PI		3.14159265358979323846  // pi                        
#endif
#ifndef	M_PI_2
#define M_PI_2		1.57079632679489661923	// pi/2
#endif

#define M2PX(metres)	((int)((metres)/gattr.mapscale) + (int)gattr.mapmargin)
#define PX2M(pixel)	((int)((pixel - (int)gattr.mapmargin)*gattr.mapscale))

#if	!defined(MAX)
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#endif
