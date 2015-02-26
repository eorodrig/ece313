#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#ifndef	_SCHEDULER_H_
#define	_SCHEDULER_H_	1

/*  We can employ one of two types of queueing here - either a basic
    Linear Queuing, which is reasonable for queuing up to a hundred pending
    events, or the more complex and fast Calendar Queuing, which remains
    fast for several thousand pending events.
    Define either:
			USE_CALENDARQ
	or		USE_LINEARQ
 */

#define	USE_CALENDARQ


#define	EACH_SELECT		20000		// in usecs
#define	MIN_SELECT		1000		// in usecs

#if	defined(USE_TCLTK)
#define	PING_TCLTK_FREQ		40000		// in usecs
#define	N_SPEEDS		10
#define	NORMAL_SPEED		((N_SPEEDS+1) / 2)
#endif

#if	defined(USE_TCLTK) || defined(USE_IPOD)
extern	bool	debug_pressed;
#endif

typedef enum {
    SCHED_NULL = 0,
    SCHED_NODESTATE,				// typically EV_REBOOT
    SCHED_FRAMEARRIVAL,
    SCHED_PERIODIC,
    SCHED_TIMER,
    SCHED_GAMEOVER,

#if     CNET_PROVIDES_APPLICATION_LAYER
    SCHED_POLLAPPL,
#endif

#if	CNET_PROVIDES_WANS
    SCHED_LINKSTATE,
#endif

#if	defined(USE_TCLTK) && CNET_PROVIDES_WANS
    SCHED_DRAWFRAME,
    SCHED_MOVEFRAME,
#endif

#if	defined(USE_TCLTK) && CNET_PROVIDES_WLANS
    SCHED_DRAWWLANSIGNAL,
#endif

#if	CNET_PROVIDES_WANS || CNET_PROVIDES_WLANS
    SCHED_FRAMECOLLISION,
#endif

#if	CNET_PROVIDES_KEYBOARD
    SCHED_KEYBOARDREADY,
#endif

#if	defined(USE_TCLTK) || defined(USE_IPOD)
    SCHED_DEBUGBUTTON,
    SCHED_UPDATEGUI,
#endif

} SCHEDEVENT;

typedef struct _e {
    int64_t	usec;
    SCHEDEVENT	se;
    int		node;
    CnetEvent	ne;
    CnetTimerID	timer;
    CnetData	data;
    struct _e	*next;
} EVENT;


#if	defined(USE_CALENDARQ)

/*  THIS IMPLEMENTATION OF A CALENDAR QUEUE CLOSELY FOLLOWS THE IDEAS
    FIRST PUBLISHED IN:

	R. Brown. "Calendar queues: A fast O(1) priority
	queue implementation for the simulation event set problem".
	Comm. ACM, 31(10):1220--1227, Oct.  1988.
 */

#define	INIT_NBUCKETS		2
#define	INIT_WIDTH		3
#define	PREDAWN			(0)

#define FOREACH_BUCKET  for(b=0, bp=CQ->buckets ; b<CQ->nbuckets ; ++b, ++bp)
#define	FOREACH_EVENT	for(e = bp->head ; e ; e = e->next)

typedef struct {
    int		nevents;
    int64_t	earliest;
    EVENT	*head;
    EVENT	*tail;
} BUCKET;

typedef struct {
    int		nevents;	// # of events in the calendar
    int		nbuckets;	// number of buckets in queue
    BUCKET	*buckets;	// point to Q->nbuckets buckets

    int64_t	lastusec;
    int		lastbucket;

    int64_t	bucketwidth;	// width of each day in the calendar
    int64_t	buckettop;

    int		minthreshold;
    int		maxthreshold;

    bool	resizeable;
} CALENDARQ;


#elif	defined(USE_LINEARQ)

#define	FOREACH_EVENT	for(e = LQ->head ; e ; e = e->next)

typedef struct {
    EVENT	*head;
    int		nevents;
} LINEARQ;
#endif

extern	void	init_scheduler(const char *eflag, const char *fflag, bool Nflag,
			int Sflag, bool _sflag, bool _Tflag, const char *uflag);

extern	void	init_queuing(void);
extern	EVENT	*get_next_event(void);

extern	void	internal_event(SCHEDEVENT, CnetEvent, int, CnetTime, CnetData);
extern	void	unschedule_node(int node);
extern	void	unschedule_link(int node, int link);

#if	defined(USE_TCLTK) && CNET_PROVIDES_WLANS
extern	void	schedule_drawwlansignal(int thiswan);
#endif

extern	int	nqueued;

#endif
