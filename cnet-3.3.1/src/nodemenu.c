#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	defined(USE_TCLTK)

static const char	*menu_strings[] = {
	"Reboot",
	"Crash",
	"Shutdown, reboot",
	"Pause 10s, resume",
	"Pause 60s, resume"
};

#define	N_NODEMENU_STRINGS	(sizeof(menu_strings) / sizeof(menu_strings[0]))

#define	RE		0
#define	CR		1
#define	SR		2
#define	P1		3
#define	P6		4

#define	REMASK		(1<<RE)
#define	CRMASK		(1<<CR)
#define	SRMASK		(1<<SR)
#define	P1MASK		(1<<P1)
#define	P6MASK		(1<<P6)


static int nodemenu_mask(int n)
{
    int	mask;

    switch ((int)NODES[n].runstate) {
    case STATE_PAUSED :
    case STATE_RUNNING :
	mask	= REMASK | CRMASK | SRMASK | P1MASK | P6MASK;	break;
    case STATE_CRASHED :
    case STATE_UNDERREPAIR :
	mask	= REMASK;					break;
    default :
	mask	= 0;						break;
    }
    return(mask);
}

static TCLTK_COMMAND(nodemenu_select)
{
    extern void	unschedule_node(int);
    NODE	*np;

    int	n	= atoi(argv[1]);
    int	s	= atoi(argv[2]);
    if(n < 0 || n >= _NNODES || s < 0 || s >= N_NODEMENU_STRINGS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid node or menu arg",-1));
	return TCL_ERROR;
    }

    np	= &NODES[n];
    switch (s) {
    case RE :	np->runstate  = STATE_REBOOTING;
		newevent(EV_REBOOT, n,
			    (CnetTime)1, NULLTIMER, np->data[(int)EV_REBOOT]);
		break;
    case CR :	np->runstate  = STATE_CRASHED;
		unschedule_node(n);
		break;
    case SR :	np->runstate	= STATE_AUTOREBOOT;
		newevent(EV_REBOOT, n,
			    (CnetTime)1, NULLTIMER, np->data[(int)EV_REBOOT]);
		break;

    case P1 :	np->runstate	= STATE_PAUSED;
		np->resume_time	= MICROSECONDS + 1000000;
		newevent(EV_REBOOT, n,
			(CnetTime)1000000, NULLTIMER, np->data[(int)EV_REBOOT]);
		break;

    case P6 :	np->runstate	= STATE_PAUSED;
		np->resume_time	= MICROSECONDS + 6000000;
		newevent(EV_REBOOT, n,
			(CnetTime)6000000, NULLTIMER, np->data[(int)EV_REBOOT]);
		break;
    }
    draw_node_icon(n, NULL, 0, 0);
    if(dflag)
	REPORT("%s.menu -> %s\n", np->nodename, menu_strings[s]);

    return TCL_OK;
}

void display_nodemenu(int n)
{
    static int	first_time	= true;
    NODE	*np		= &NODES[n];

    if(first_time) {
	int	s;

	TCLTK_createcommand("nodemenu_select", nodemenu_select);
	for(s=0 ; s<N_NODEMENU_STRINGS ; s++) {
	    sprintf(chararray,"nodemenu_label(%i)", s);
	    Tcl_SetVar(tcl_interp, chararray, menu_strings[s], TCL_GLOBAL_ONLY);
	}
	first_time = false;
    }
    TCLTK("popupNodemenu %i %s . %i %i %i",
			n, np->nodename,
			nodemenu_mask(n),
			np->nattr.position.x, np->nattr.position.y);
}

#endif	// defined(USE_TCLTK)
