#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

const char *cnet_evname[N_CNET_EVENTS] = {
    "EV_NULL",			"EV_REBOOT",		"EV_SHUTDOWN",
    "EV_PHYSICALREADY",

#if	CNET_PROVIDES_APPLICATION_LAYER
    "EV_APPLICATIONREADY",
#endif
#if	CNET_PROVIDES_KEYBOARD
    "EV_KEYBOARDREADY",
#endif

#if	CNET_PROVIDES_WANS
    "EV_DRAWFRAME",
    "EV_LINKSTATE",
#endif
#if	CNET_PROVIDES_LANS || CNET_PROVIDES_WLANS
    "EV_FRAMECOLLISION",

#endif

    "EV_PERIODIC",
    "EV_UPDATEGUI",

    "EV_DEBUG0", "EV_DEBUG1", "EV_DEBUG2", "EV_DEBUG3", "EV_DEBUG4",
    "EV_TIMER0", "EV_TIMER1", "EV_TIMER2", "EV_TIMER3", "EV_TIMER4",
    "EV_TIMER5", "EV_TIMER6", "EV_TIMER7", "EV_TIMER8", "EV_TIMER9"
};


static int check_handler(CnetEvent ev, long handler, CnetData data,
			 const char *name)
{
    int		result = -1;

    if((int)ev < (int)EV_NULL || (int)ev >= N_CNET_EVENTS)
	ERROR(ER_BADEVENT);
#if	CNET_PROVIDES_APPLICATION_LAYER
    else if(ev == EV_APPLICATIONREADY && !NODE_HAS_AL(THISNODE))
	ERROR(ER_NOTSUPPORTED);
#endif
#if	CNET_PROVIDES_KEYBOARD
    else if(ev == EV_KEYBOARDREADY && !NODE_HAS_KB(THISNODE))
	ERROR(ER_NOTSUPPORTED);
#endif
    else
	result = 0;

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(%s,%s,data=%ld) = 0\n",
			name, cnet_evname[(int)ev],
			find_trace_name((void *)handler), data);
	else
	    TRACE(1, "\t%s(ev=%i,%s,data=%ld) = -1 %s\n",
			name, (int)ev,
			find_trace_name((void *)handler), data,
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_set_handler(CnetEvent ev,
		     void (*handler)(CnetEvent, CnetTimerID, CnetData),
		     CnetData data)
{
    if(check_handler(ev, (long)handler, (long)data, __func__) == 0) {
	NP->handler[(int)ev - (int)EV_NULL] = handler;
	NP->data   [(int)ev - (int)EV_NULL] = data;
	return 0;
    }
    return (-1);
}

int CNET_get_handler(CnetEvent ev,
		     void (*handler)(CnetEvent, CnetTimerID, CnetData),
		     CnetData *data)
{
    if(check_handler(ev, (long)handler, (long)data, __func__) == 0) {
	if(handler)
	    handler	= NP->handler[(int)ev - (int)EV_NULL];
	if(data)
	    *data	= NP->data   [(int)ev - (int)EV_NULL];
	return 0;
    }
    return (-1);
}
