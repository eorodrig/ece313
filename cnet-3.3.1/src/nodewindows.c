#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

CnetEvent CNET_unused_debug_event(void)
{
    CnetEvent	ev = EV_DEBUG0;

    for(int e=0 ; e < N_CNET_DEBUGS ; ++e, ++ev) {
	if(NP->handler[(int)ev] == NULL)
	    return ev;
    }
    return EV_NULL;
}

int CNET_set_debug_string(CnetEvent ev, const char *str)
{
    int result	= -1;

    if(str == NULL)
	str = "";

    if((int)ev<(int)EV_DEBUG0 || (int)ev >= ((int)EV_DEBUG0 + N_CNET_DEBUGS))
	ERROR(ER_BADEVENT);
    else {
	result = 0;
#if	defined(USE_TCLTK)
	if(Wflag) {
	    int	d = (int)ev-(int)EV_DEBUG0;

	    if(NP->debug_str[d])
		FREE(NP->debug_str[d]);

	    if(*str == '\0')
		NP->debug_str[d]		= strdup("");
	    else {
		strncpy(chararray,str,WIDTH_DEBUG_BUTTON_STRING);
		chararray[WIDTH_DEBUG_BUTTON_STRING]	= '\0';
		NP->debug_str[d]		= strdup(chararray);
	    }
	    CHECKALLOC(NP->debug_str[d]);

	    str = NP->debug_str[d];
	    TCLTK("SetDebugString .node%i.d.debug%i \"%s\"",
		    THISNODE, d, (str == NULL || *str == '\0') ? " " : str);
	}
#endif	// defined(USE_TCLTK)
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(%s,\"%s\") = 0\n",
			__func__, cnet_evname[(int)ev], str);
	else
	    TRACE(1, "\t%s(ev=%i,\"%s\") = -1 %s\n",
			__func__, (int)ev, str, cnet_errname[(int)cnet_errno]);
    }
    return result;
}


// ------------------ ONLY CODE FOR USE_TCLTK ---------------------


#if	defined(USE_TCLTK)
static TCLTK_COMMAND(node_debug_button)
{
    extern void	single_event(int, CnetEvent, CnetTimerID);

    NODE	*np;
    CnetEvent	ev;

    int	n	= atoi(argv[1]);
    int	d	= atoi(argv[2]);
    if(n < 0 || n >= _NNODES || d < 0 || d >= N_CNET_DEBUGS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid node or debug #", -1));
	return TCL_ERROR;
    }

    np	= &NODES[n];
    if(np->runstate == STATE_CRASHED)
	return TCL_OK;

    ev	= (CnetEvent)((int)EV_DEBUG0+d);
    if(vflag)
	REPORT("%s.%s\n", np->nodename, cnet_evname[(int)ev]);

    if(cnet_state == STATE_PAUSED) {
	int	savequiet1	= np->nattr.stdio_quiet;
	int	savequiet2	= DEF_NODEATTR.stdio_quiet;

	np->nattr.stdio_quiet	= false;
	DEF_NODEATTR.stdio_quiet	= false;
	    single_event(n, ev, NULLTIMER);
	DEF_NODEATTR.stdio_quiet	= savequiet2;
	np->nattr.stdio_quiet	= savequiet1;
    }
    else
	newevent(ev, n, (CnetTime)1, NULLTIMER, np->data[(int)ev]);

    return TCL_OK;
}


#define	MSGRATE			0
#define	MINSIZE			1
#define	MAXSIZE			2

#define	N_MSGRATE		7
#define	N_MINSIZE		7
#define	N_MAXSIZE		7

static struct {
    const char	*label;
    const char	*keyword;
} sliders[] = {
    {	"message rate",		"messagerate"		},
    {	"min message size",	"minmessagesize"	},
    {	"max message size",	"maxmessagesize"	}
};

static int msgsizes[] = {
	0, MIN_MESSAGE_SIZE, 256, 512, 1024, 2048, 4096, MAX_MESSAGE_SIZE
};

#define	N_NODE_SLIDERS	(sizeof(sliders) / sizeof(sliders[0]))
#define	N_MSGSIZES	(sizeof(msgsizes) / sizeof(msgsizes[0]))


static int my_pow10(int n)
{
    int	pow10 = 1;

    while(n-- > 0)
	pow10 *= 10;
    return(pow10);
}

static void set_node_scale_label(int n, int scale, int value, NODEATTR *na)
{
    char	desc[32];

    switch (value) {
    case -1 :	strcpy(desc, "(default)");
		break;

    default :	switch (scale) {
		case MSGRATE :
			sprintf(desc,"(%s usec)",CNET_format64(na->messagerate));
			break;
		case MINSIZE :
			sprintf(desc, "(%i bytes)", na->minmessagesize);
			break;
		case MAXSIZE :
			sprintf(desc, "(%i bytes)", na->maxmessagesize);
			break;
		}
		break;
    }

    if(n == DEFAULT)
	TCLTK(".nodedefault.top.left.scales.scale%i config -label \"%s %s\"",
		    scale, sliders[scale].label, desc);
    else
	TCLTK(".node%i.top.left.scales.scale%i config -label \"%s %s\"",
		    n, scale, sliders[scale].label, desc);
}

static TCLTK_COMMAND(set_node_scale)
{
    NODEATTR	*na;
    int		n, scale, value;

    if(strcmp(argv[1], "default") == 0)
	n	= DEFAULT;
    else
	n	= atoi(argv[1]);
    scale	= atoi(argv[2]);
    value	= atoi(argv[3]);

    if(n < -1 || n >= _NNODES || scale < 0 || scale > N_NODE_SLIDERS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid set_node_scale #",-1));
	return TCL_ERROR;
    }

    na	= (n==DEFAULT) ? &DEF_NODEATTR : (&NODES[n].nattr);

    switch (scale) {
    case MSGRATE:
	if(value == DEFAULT)
	    na->messagerate	= DEFAULT;
	else if(value == 0)
	    na->messagerate	= na->messagerate;
	else
	    na->messagerate	= my_pow10(value);
	break;

    case MINSIZE:
	if(value == DEFAULT)
	    na->minmessagesize	= DEFAULT;
	else if(value == 0)
	    na->minmessagesize	= MIN_MESSAGE_SIZE;
	else
	    na->minmessagesize	= (int)msgsizes[value];
	break;

    case MAXSIZE:
	if(value == DEFAULT)
	    na->maxmessagesize	= DEFAULT;
	else if(value == 0)
	    na->maxmessagesize	= MAX_MESSAGE_SIZE;
	else
	    na->maxmessagesize	= (int)msgsizes[value];
	break;
    }

    set_node_scale_label(n, scale, value, na);
    if(dflag)
	REPORT("%s.%s = %i\n", n == DEFAULT ? "defaultnode" : NODES[n].nodename,
				sliders[scale].keyword, value);
    return TCL_OK;
}

// ----------------------------------------------------------------------

static void append_scales(int n, char *tkcmd)
{
    char	*p;

    p	= tkcmd;
    while(*p)
	++p;

    sprintf(p, " {%i %i %i}", N_MSGRATE, N_MINSIZE, N_MAXSIZE);
    while(*p)
	++p;

    if(n == DEFAULT)
	sprintf(p, " {0 1 %i}", (int)N_MSGSIZES);
    else
	sprintf(p, " {-1 -1 -1}");
}

static char *nodetypename[] = {
	"host", "router", "mobile", "accesspoint"
};

void init_nodewindow(int n)
{
    static int	first_time	= true;
    char	tkcmd[BUFSIZ];

    if(first_time)  {
	TCLTK_createcommand("set_node_scale",	 set_node_scale);
	TCLTK_createcommand("node_debug_button", node_debug_button);
	first_time = false;
    }

    if(n == DEFAULT) {
	static int	default_displayed = false;	// never used

	strcpy(chararray, "stdio_quiet(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&DEF_NODEATTR.stdio_quiet, TCL_LINK_BOOLEAN);
	strcpy(chararray, "trace_events(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&DEF_NODEATTR.trace_all, TCL_LINK_BOOLEAN);
	strcpy(chararray, "node_displayed(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&default_displayed, TCL_LINK_BOOLEAN);

	sprintf(tkcmd, "newNode default default default 40 40");
	append_scales(n, tkcmd);
	TCLTK(tkcmd);
    }
    else {
	NODE		*np	= &NODES[n];

	sprintf(chararray, "stdio_quiet(%i)", n);
	Tcl_LinkVar(tcl_interp, chararray,
			(char *)&np->nattr.stdio_quiet, TCL_LINK_BOOLEAN);
	sprintf(chararray, "trace_events(%i)", n);
	Tcl_LinkVar(tcl_interp, chararray,
			(char *)&np->nattr.trace_all, TCL_LINK_BOOLEAN);
	sprintf(chararray, "node_displayed(%i)", n);
	Tcl_LinkVar(tcl_interp, chararray,
			(char *)&np->displayed, TCL_LINK_BOOLEAN);

	sprintf(tkcmd, "newNode %i %s %s %i %i",
		    n, np->nodename, nodetypename[(int)np->nodetype],
		    CNET_NLEDS, np->nwlans);
	append_scales(n, tkcmd);
	TCLTK(tkcmd);

	if(np->nattr.winopen)
	    TCLTK("toggleNode %i %i %i",
			n, M2PX(np->nattr.winx), M2PX(np->nattr.winy));
    }
}
#endif	// defined(USE_TCLTK)
