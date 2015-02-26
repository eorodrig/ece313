#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_KEYBOARD
int CNET_read_keyboard(char *line, size_t *len)
{
    int		result	= -1;

    if(!NODE_HAS_KB(THISNODE))
	ERROR(ER_NOTSUPPORTED);

    else if(line == NULL || len == NULL || *len <= 0)
	ERROR(ER_BADARG);

    else if(NP->inputline == NULL)
	ERROR(ER_NOTREADY);

    else if(*len <= NP->inputlen)
	ERROR(ER_BADSIZE);

    else {					// only provide inputline once
	strcpy(line, NP->inputline);
	*len		= NP->inputlen+1;	// strlen() + NULL
	FREE(NP->inputline);
	NP->inputlen	= 0;
	result		= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(%s,%s) = 0\n",
			__func__,
			find_trace_name(line), find_trace_name(len));
	else
	    TRACE(1, "\t%s(%s,%s) = -1 %s\n",
			__func__,
			find_trace_name(line), find_trace_name(len),
			cnet_errname[(int)cnet_errno]);
    }
    return result;
}

void set_keyboard_input(int n, const char *line)
{
    NODE *np	= &NODES[n];

    if(np->inputline)
	FREE(np->inputline);
    np->inputlen	= 0;

    if(np->handler[(int)EV_KEYBOARDREADY]) {
	np->inputline	= strdup(line);
	CHECKALLOC(np->inputline);
	np->inputlen	= strlen(line);
	newevent(EV_KEYBOARDREADY, n,
	    (CnetTime)1, NULLTIMER, np->data[(int)EV_KEYBOARDREADY]);
    }
}

#endif
