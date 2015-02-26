#include "cnetprivate.h"
#include "scheduler.h"
#include "physicallayer.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

int		nqueued		= 0;

#if	defined(USE_CALENDARQ)

/*  THIS IMPLEMENTATION OF A CALENDAR QUEUE CLOSELY FOLLOWS THE IDEAS
    FIRST PUBLISHED IN:

	R. Brown. "Calendar queues: A fast O(1) priority
	queue implementation for the simulation event set problem".
	Comm. ACM, 31(10):1220--1227, Oct.  1988.
 */

static	CALENDARQ	*enqueue(CALENDARQ *Q, EVENT *new);
static	CALENDARQ	*dequeue(CALENDARQ *Q, EVENT **oldev);

static	CALENDARQ	*CQ	= NULL;

static	CALENDARQ *newqueue(int nbuckets, int64_t width, int64_t lastusec)
{
    CALENDARQ	*Q;
    int64_t	b64;

// INITIALIZE MEMORY FOR A NEW CALENDARQ
    Q			= calloc(1, sizeof(CALENDARQ));
    CHECKALLOC(Q);
    Q->buckets		= calloc(nbuckets, sizeof(BUCKET));
    CHECKALLOC(Q->buckets);
    if(Q->buckets == NULL) {
	REPORT("cannot allocate space for %i buckets\n", nbuckets);
	exit(EXIT_FAILURE);
    }
    Q->nbuckets		= nbuckets;

    Q->bucketwidth	= width;
    Q->lastusec		= lastusec;

    b64			= Q->lastusec / Q->bucketwidth;
    Q->lastbucket	= (int)(b64 % (int64_t)nbuckets);
    Q->buckettop	= (b64+(int64_t)1)*Q->bucketwidth +
				Q->bucketwidth/(int64_t)2;

//  SET LIMITS ON WHEN TO RESIZE
    Q->minthreshold	= nbuckets/2 - 2;
    Q->maxthreshold	= nbuckets*2;

    Q->resizeable	= true;
    return Q;
}

static int64_t newwidth(CALENDARQ *Q)
{
#define	MAXSAMPLES	25
    EVENT	*samples[MAXSAMPLES];
    int		n, nsamples, count;
    double	sepsum, sepavg;

    int		save_lastbucket;
    int64_t	save_lastusec;
    int64_t	save_buckettop;

    if(Q->nevents < 2)
	return 1;

//  DETERMINE THE NUMBER OF EVENTS TO SAMPLE
    if(Q->nevents <= 5)
	nsamples	= Q->nevents;
    else {
	nsamples	= 5 + Q->nevents/10;
	if(nsamples > MAXSAMPLES)
	    nsamples	= MAXSAMPLES;
    }

//  SAVE THE STATE OF THE CURRENT QUEUE
    save_lastbucket	= Q->lastbucket;
    save_lastusec	= Q->lastusec;
    save_buckettop	= Q->buckettop;
    Q->resizeable	= false;

//  DETERMINE THE SEPARATION AVERAGE OF THE CURRENT QUEUE
    sepsum	= 0.0;
    Q	= dequeue(Q, &samples[0]);
    for(n=1, count=0 ; n<nsamples ; ++n) {
	Q	= dequeue(Q, &samples[n]);
	sepsum += (double)(samples[n]->usec - samples[n-1]->usec);
	++count;
    }
    sepavg	= sepsum / (double)count;

//  DETERMINE THE SEPARATION AVERAGE OF "CLOSE ENOUGH" SAMPLES
    sepsum	= 0.0;
    Q		= enqueue(Q, samples[0]);
    for(n=1, count=0 ; n<nsamples ; ++n) {
        double	diff;

	diff	= (double)(samples[n]->usec - samples[n-1]->usec);
        if(diff < (sepavg * 2.0)) {
            sepsum += diff;
            ++count;
        }
	Q	= enqueue(Q, samples[n]);
    }
    sepavg	= sepsum / (double)count;

//  RESTORE THE SAVED STATE
    Q->resizeable	= true;
    Q->lastbucket	= save_lastbucket;
    Q->lastusec		= save_lastusec;
    Q->buckettop	= save_buckettop;

    sepavg	= 3.0 * sepavg;
    if(sepavg < 1.0)
	return 1;
    return (int64_t)sepavg;
#undef	MAXSAMPLES
}

static CALENDARQ *resize(CALENDARQ *oldq, int newnbuckets)
{
    CALENDARQ	*newq;
    EVENT	*evp;
    int		n;

    if(oldq->resizeable == false)
	return oldq;

//  ALLOCATE A NEW QUEUE OF THE DESIRED SIZE
    newq	= newqueue(newnbuckets, newwidth(oldq), oldq->lastusec);

//  COPY ALL OF THE EXISTING EVENTS OVER TO THE NEW QUEUE
    oldq->resizeable	= false;
    n			= oldq->nevents;
    while(n > 0) {
	oldq	= dequeue(oldq, &evp);
	newq	= enqueue(newq, evp);
	--n;
    }

//  DEALLOCATE SPACE FROM THE OLD QUEUE
    FREE(oldq->buckets);
    FREE(oldq);

//  RETURN THE NEW QUEUE
    return newq;
}

static CALENDARQ *enqueue(CALENDARQ *Q, EVENT *new)
{
    BUCKET	*bp;
    int		b;
    int64_t	b64;
    int64_t	usec	= new->usec;

    b64	= (usec / Q->bucketwidth);
    b	= (int)(b64 % (int64_t)Q->nbuckets);
    bp	= &Q->buckets[b];

    new->next	= NULL;
//  FIRST EVENT IN THIS BUCKET
    if(bp->nevents == 0) {
	bp->head	= new;
	bp->tail	= new;
	bp->earliest	= usec;
    }
//  EARLIEST EVENT IN THIS BUCKET?
    else if(usec < bp->earliest) {
	new->next	= bp->head;
	bp->head	= new;
	bp->earliest	= usec;
    }
//  LATEST EVENT IN THIS BUCKET?
    else if(usec >= bp->tail->usec) {
	bp->tail->next	= new;
	bp->tail	= new;
    }
//  "MIDDLE" EVENT IN THIS BUCKET
    else {
	EVENT	*this	= bp->head;
	EVENT	*prev	= this;

	while(usec >= this->usec) {
	    prev	= this;
	    this	= this->next;
	}
	prev->next	= new;
	new->next	= this;
    }

//  GROW THE CALENDAR IF NECESSARY
    ++bp->nevents;
    ++Q->nevents;
    if(Q->nevents > Q->maxthreshold)
	Q	= resize(Q, Q->nbuckets*2);

    return Q;
}

#if 0
// DIRECT SEARCH FOR EARLIEST BUCKET
static EVENT *peek(CALENDARQ *Q)
{
    BUCKET	*bp;
    int		b;

    if(Q->nevents == 0)
	return NULL;

    for(b=Q->lastbucket ; ; ) {
	bp	= &Q->buckets[b];
	if(bp->head && bp->head->usec < Q->buckettop)
	    return bp->head;

	if(++b == Q->nbuckets)
	    b	= 0;
	Q->buckettop	+= Q->bucketwidth;
	if(b == Q->lastbucket)
	    break;
    }

    Q->lastbucket	= 0;
    Q->lastusec		= INT64_MAX;
    for(b=0, bp=Q->buckets ; b<Q->nbuckets ; ++b, ++bp) {
	if(bp->head && bp->earliest < Q->lastusec) {
	    Q->lastusec		= bp->earliest;
	    Q->lastbucket	= b;
	}
    }
    Q->buckettop	= ((Q->lastusec / Q->bucketwidth)+1) * Q->bucketwidth +
				Q->bucketwidth / 2;

    return( peek(Q) );
}
#endif

// DIRECT SEARCH FOR EARLIEST BUCKET
static CALENDARQ *dequeue(CALENDARQ *Q, EVENT **oldev)
{
    BUCKET	*bp;
    EVENT	*evp = NULL;
    int		b;

    if(Q->nevents == 0) {
	*oldev	= NULL;
	return Q;
    }

    for(b=Q->lastbucket ; ; ) {
	bp	= &Q->buckets[b];
	if(bp->head && bp->head->usec < Q->buckettop) {
	    evp	= bp->head;
	    if(bp->nevents == 1) {
		bp->head	= NULL;
		bp->tail	= NULL;
		bp->earliest	= PREDAWN;
	    }
	    else {
		bp->head	= bp->head->next;
		bp->earliest	= bp->head->usec;
	    }
	    Q->lastbucket	= b;
	    Q->lastusec		= evp->usec;

//  SHRINK THE CALENDAR IF NECESSARY
	    --bp->nevents;
	    --Q->nevents;
	    if(Q->nevents < Q->minthreshold)
		Q	= resize(Q, Q->nbuckets/2);

	    *oldev	= evp;
	    return Q;
	}

	if(++b == Q->nbuckets)
	    b	= 0;
	Q->buckettop	+= Q->bucketwidth;
	if(b == Q->lastbucket)
	    break;
    }

    Q->lastbucket	= 0;
    Q->lastusec		= INT64_MAX;
    for(b=0, bp=Q->buckets ; b<Q->nbuckets ; ++b, ++bp) {
	if(bp->head && bp->earliest < Q->lastusec) {
	    Q->lastusec		= bp->earliest;
	    Q->lastbucket	= b;
	}
    }
    Q->buckettop	= ((Q->lastusec / Q->bucketwidth)+1) * Q->bucketwidth +
				Q->bucketwidth / 2;

    return( Q = dequeue(Q, oldev) );
}

// ----------------------------------------------------------------------

void unschedule_node(int deadnode)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if((e->se==SCHED_FRAMEARRIVAL || e->se==SCHED_FRAMECOLLISION) &&
	        e->node == deadnode) {

		FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

		if(arrival->frame)
		    FREE(arrival->frame);
		FREE(arrival);
		e->se	= SCHED_NULL;
	    }
	}
    }
    NODES[deadnode].nframes = 0;
}

void unschedule_link(int destnode, int destlink)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if((e->se==SCHED_FRAMEARRIVAL || e->se==SCHED_FRAMECOLLISION) &&
	        e->node == destnode) {

		FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

		if(arrival->destlink == destlink) {
		    if(e->se == SCHED_FRAMEARRIVAL)
			--NODES[destnode].nframes;

		    if(arrival->frame)
			FREE(arrival->frame);
		    FREE(arrival);
		    e->se	= SCHED_NULL;
		}
	    }
	}
    }
}

#if	CNET_PROVIDES_APPLICATION_LAYER
void unschedule_application(int node)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if(e->se == SCHED_POLLAPPL && e->node == node) {
		e->se	= SCHED_NULL;
		return;
	    }
	}
    }
}
#endif

void unschedule_lan_collision(int thislan)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if(e->se == SCHED_FRAMEARRIVAL || e->se == SCHED_FRAMECOLLISION) {

		FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

		if(arrival->linkindex == thislan) {
		    if(e->se == SCHED_FRAMEARRIVAL)
			--NODES[e->node].nframes;

		    if(arrival->frame)
			FREE(arrival->frame);
		    FREE(arrival);
		    e->se	= SCHED_NULL;
		}
	    }
	}
    }
}

bool unschedule_timer(CnetTimerID timer, CnetData *datawas, bool delete)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if(e->se==SCHED_TIMER && timer == e->timer && THISNODE == e->node) {
		if(delete)
		    e->se	= SCHED_NULL;
		if(datawas)
		    *datawas	= e->data;
		return true;
	    }
	}
    }
    return false;
}

#if 0
bool find_timer_data(CnetTimerID timer, CnetData *datawas)
{
    BUCKET	*bp;
    EVENT	*e;
    int		b;

    FOREACH_BUCKET {
	FOREACH_EVENT {
	    if(e->se==SCHED_TIMER && timer == e->timer && THISNODE == e->node) {
		if(datawas)
		    *datawas	= e->data;
		return true;
	    }
	}
    }
    return false;
}
#endif

// -----------------------------------------------------------------

#elif	defined(USE_LINEARQ)

//  Not using a Calendar Queue, so use the default (poor) Linear Queue.

static	LINEARQ		*LQ	= NULL;

static LINEARQ *newqueue(void)
{
    LINEARQ	*lq = calloc(1, sizeof(LINEARQ));
    CHECKALLOC(lq);
    return lq;
}

static void enqueue(LINEARQ *Q, EVENT *new)
{
    CnetTime	usec	= new->usec;

//  FIRST EVENT IN THIS QUEUE
    if(Q->nevents == 0)
	Q->head	= new;

//  "MIDDLE" EVENT IN THIS QUEUE
    else {
	EVENT	*this	= Q->head;
	EVENT	*prev	= NULL;

	while(this && usec >= this->usec) {
	    prev	= this;
	    this	= this->next;
	}
	if(prev == NULL) {
	    new->next	= Q->head;
	    Q->head	= new;
	}
	else {
	    prev->next	= new;
	    new->next	= this;
	}
    }

    ++Q->nevents;
}

static EVENT *peek(LINEARQ *Q)
{
    return Q->head;
}

static void dequeue(LINEARQ *Q, EVENT **oldev)
{
    if(Q->nevents == 0)
	*oldev	= NULL;
    else {
	*oldev	= Q->head;
	Q->head	= Q->head->next;
	--Q->nevents;
    }
}

void unschedule_node(int deadnode)
{
    EVENT	*e;

    FOREACH_EVENT {
	if((e->se==SCHED_FRAMEARRIVAL || e->se==SCHED_FRAMECOLLISION) &&
	    e->node == deadnode) {

	    FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

	    if(arrival->frame)
		FREE(arrival->frame);
	    FREE(arrival);
	    e->se	= SCHED_NULL;
	}
    }
    NODES[deadnode].nframes = 0;
}

void unschedule_link(int destnode, int destlink)
{
    EVENT	*e;

    FOREACH_EVENT {
	if((e->se==SCHED_FRAMEARRIVAL || e->se==SCHED_FRAMECOLLISION) &&
	    e->node == destnode) {

	    FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

	    if(arrival->destlink == destlink) {
		if(e->se == SCHED_FRAMEARRIVAL)
		    --NODES[destnode].nframes;

		if(arrival->frame)
		    FREE(arrival->frame);
		FREE(arrival);
		e->se	= SCHED_NULL;
	    }
	}
    }
}

#if	CNET_PROVIDES_APPLICATION_LAYER
void unschedule_application(int node)
{
    EVENT	*e;

    FOREACH_EVENT {
	if(e->se == SCHED_POLLAPPL && e->node == node) {
	    e->se	= SCHED_NULL;
	    return;
	}
    }
}
#endif

void unschedule_lan_collision(int thislan)
{
    EVENT	*e;

    FOREACH_EVENT {
	if(e->se == SCHED_FRAMEARRIVAL || e->se == SCHED_FRAMECOLLISION) {

	    FRAMEARRIVAL	*arrival = (FRAMEARRIVAL *)e->data;

	    if(arrival->linkindex == thislan) {
		if(e->se == SCHED_FRAMEARRIVAL)
		    --NODES[e->node].nframes;

		if(arrival->frame)
		    FREE(arrival->frame);
		FREE(arrival);
		e->se	= SCHED_NULL;
	    }
	}
    }
}

bool unschedule_timer(CnetTimerID timer, CnetData *datawas, bool delete)
{
    EVENT	*e;

    FOREACH_EVENT {
	if(e->se==SCHED_TIMER && timer == e->timer && THISNODE == e->node) {
	    if(delete)
		e->se		= SCHED_NULL;
	    if(datawas)
		*datawas	= e->data;
	    return true;
	}
    }
    return false;
}
#endif

// -----------------------------------------------------------------

void init_queuing(void)
{
#if	defined(USE_CALENDARQ)
    CQ	= newqueue(INIT_NBUCKETS, INIT_WIDTH, 0);
#elif	defined(USE_LINEARQ)
    LQ	= newqueue();
#endif
}

void internal_event(SCHEDEVENT se, CnetEvent ne, int node,
			     CnetTime usec, CnetData data)
{
    EVENT *new	= calloc(1, sizeof(EVENT));

    CHECKALLOC(new);
    new->se	= se;
    new->ne	= ne;
    new->node	= node;
    new->usec	= MICROSECONDS + usec;
    new->timer	= NULLTIMER;
    new->data	= data;

#if	defined(USE_CALENDARQ)
    CQ	= enqueue(CQ, new);
#elif	defined(USE_LINEARQ)
    enqueue(LQ, new);
#endif
    ++nqueued;
}

void newevent(CnetEvent ne, int node,
		    CnetTime usecs, CnetTimerID timer, CnetData data)
{
    EVENT *new	= calloc(1, sizeof(EVENT));

    CHECKALLOC(new);
    new->ne	= ne;
    new->node	= node;
    new->usec	= MICROSECONDS + usecs;
    new->timer	= timer;
    new->data	= data;

    if(IS_TIMER(ne))
	new->se	= SCHED_TIMER;
#if	defined(USE_TCLTK) || defined(USE_IPOD)
    else if(IS_DEBUG(ne)) {
	new->se		= SCHED_DEBUGBUTTON;
	new->usec	= MICROSECONDS+1;	// IMMEDIATELY
	debug_pressed	= true;
    }
#endif
    else switch (ne) {
    case EV_REBOOT :		new->se	= SCHED_NODESTATE;
				break;
#if	CNET_PROVIDES_APPLICATION_LAYER
    case EV_APPLICATIONREADY :	new->se	= SCHED_POLLAPPL;
				break;
#endif

    case EV_PHYSICALREADY :	new->se	= SCHED_FRAMEARRIVAL;
				break;
    case EV_FRAMECOLLISION :	new->se	= SCHED_FRAMECOLLISION;
				break;

#if	CNET_PROVIDES_WANS
#if	defined(USE_TCLTK)
    case EV_DRAWFRAME :		new->se	= SCHED_DRAWFRAME;
				break;
#endif
    case EV_LINKSTATE :		new->se	= SCHED_LINKSTATE;
				break;
#endif

#if	CNET_PROVIDES_KEYBOARD
    case EV_KEYBOARDREADY :	new->se	= SCHED_KEYBOARDREADY;
				break;
#endif
    default :		FATAL("unknown CnetEvent in %s()\n", __func__);
			break;
    }

#if	defined(USE_CALENDARQ)
    CQ	= enqueue(CQ, new);
#elif	defined(USE_LINEARQ)
    enqueue(LQ, new);
#endif
    ++nqueued;
}

// ---------------------------------------------------------------------

#if 0
//  FIND (BUT DON'T REMOVE) THE NEXT EVENT FROM THE QUEUE
EVENT *peek_next_event(void)
{
    EVENT	*next	= NULL;

    for(;;) {
#if	defined(USE_CALENDARQ)
	next	= peek(CQ);
#elif	defined(USE_LINEARQ)
	next	= peek(LQ);
#endif

	if(next == NULL)
	    break;
	if(next->se != SCHED_NULL)
	    break;

#if	defined(USE_CALENDARQ)
	CQ	= dequeue(CQ, &next);
#elif	defined(USE_LINEARQ)
	dequeue(LQ, &next);
#endif
	--nqueued;
    }
    return next;
}
#endif

//  GET (AND REMOVE) THE NEXT EVENT FROM THE QUEUE
EVENT *get_next_event(void)
{
    EVENT	*next	= NULL;

    for(;;) {
#if	defined(USE_CALENDARQ)
	CQ	= dequeue(CQ, &next);
#elif	defined(USE_LINEARQ)
	dequeue(LQ, &next);
#endif
//  WAS THE EVENT QUEUE EMPTY?
	if(next == NULL)
	    return NULL;

	--nqueued;
//  WAS THE "NEXT" EVENT A DELETED ONE?
	if(next->se != SCHED_NULL)
	    break;
    }
    return next;
}
