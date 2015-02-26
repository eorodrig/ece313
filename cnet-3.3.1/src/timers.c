#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


//  WE DO *NOT* START TIMERS AT 0, IN AN ATTEMPT TO TRAP ANY INCORRECT TIMERS
void reboot_timer_layer(void)
{
    NP->nexttimer	= 1000;
}

CnetEvent CNET_unused_timer_event(void)
{
    CnetEvent	ev = EV_TIMER0;

    for(int e=0 ; e< N_CNET_TIMERS ; ++e, ++ev) {
	if(NP->handler[(int)ev] == NULL)
	    return ev;
    }
    return EV_NULL;
}

CnetTimerID CNET_start_timer(CnetEvent ev, CnetTime usec, CnetData data)
{
    CnetTimerID	timer	= NULLTIMER;

    if(usec <= 0)
	ERROR(ER_BADARG);
    else if(!IS_TIMER(ev))
	ERROR(ER_BADEVENT);
    else {
	timer	= (CnetTimerID)NP->nexttimer++ ;
	newevent(ev, THISNODE, usec, timer, data);
    }
    
    if(gattr.trace_events) {
	if(timer != NULLTIMER)
	    TRACE(0, "\t%s(%s,usec=%s,data=%ld) = %ld\n",
			__func__,
			cnet_evname[(int)ev], CNET_format64(usec),
			data, timer);
	else
	    TRACE(1, "\t%s(ev=%i,usec=%s,data=%ld) = %ld\n",
			__func__,
			(int)ev, CNET_format64(usec),
			data, cnet_errname[(int)cnet_errno]);
    }
    return timer;
}


int CNET_stop_timer(CnetTimerID timer)
{
    extern bool	unschedule_timer(CnetTimerID, CnetData *, bool);

    int	result	= -1;

    if(unschedule_timer(timer, NULL, true) == true)
	result	= 0;
    else
	ERROR(ER_BADTIMERID);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(t=%u) = 0\n", __func__, (unsigned)timer);
	else
	    TRACE(1, "\t%s(t=%u) = -1 %s\n",
			__func__,
			(unsigned)timer, cnet_errname[(int)cnet_errno]);
    }
    return result;
}


int CNET_timer_data(CnetTimerID timer, CnetData *data)
{
    extern bool	unschedule_timer(CnetTimerID, CnetData *, bool);

    int	result	= -1;

    if(data == NULL)
	ERROR(ER_BADARG);
    else if(unschedule_timer(timer, data, false) == true)
	result = 0;
    else
	ERROR(ER_BADTIMERID);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(t=%u,%s) = 0\n",
			__func__, (unsigned)timer, find_trace_name(data));
	else
	    TRACE(1, "\t%s(t=%u,%s) = -1 %s\n",
			__func__, (unsigned)timer,
			find_trace_name(data), cnet_errname[(int)cnet_errno]);
    }
    return result;
}
