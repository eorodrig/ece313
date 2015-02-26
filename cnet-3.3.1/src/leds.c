#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

int CNET_set_LED(int led, CnetColour colour)
{
    int result	= -1;

    if(colour == NULL)
	colour = "";

    if(led < 0 || led >= CNET_NLEDS)
	ERROR(ER_BADARG);
    else {
#if	defined(USE_TCLTK)
	if(Wflag)
	    TCLTK("SetLED %i %i \"%s\"", THISNODE, led, colour);
#endif
	result	= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(l=%i,c=%s) = 0\n", __func__, led, colour);
	else
	    TRACE(1, "\t%s(l=%i,c=%s) = -1 %s\n",
		    __func__, led, colour, cnet_errname[(int)cnet_errno]);
    }
    return result;
}
