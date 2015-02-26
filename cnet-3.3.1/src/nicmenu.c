#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	defined(USE_TCLTK)

#if	CNET_PROVIDES_WANS

static const char	*menu_strings[] = {
	"Draw frames",
	"Repair link",
	"Sever link",
	NULL,
	NULL
};

#define	N_NICMENU_STRINGS	(sizeof(menu_strings) / sizeof(menu_strings[0]))

#define	DF			0
#define	RE			1
#define	SE			2
#define	SSHORT			3
#define	SLONG			4

#define	DFMASK			(1<<DF)
#define	REMASK			(1<<RE)
#define	SEMASK			(1<<SE)
#define	SSHORTMASK		(1<<SSHORT)
#define	SLONGMASK		(1<<SLONG)


static TCLTK_COMMAND(nicmenu_select)
{
    extern void	unschedule_link(int destnode, int destlink);
    extern void report_linkstate(int wan);
    extern void toggle_drawframes(int wan);

    WAN		*wanp;
    int		l, s;

    l	= atoi(argv[1]);
    s	= atoi(argv[2]);
    if(l < 0 || l >= gattr.nwans || s < 0 || s >= N_NICMENU_STRINGS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid nic or menu arg", -1));
	return TCL_ERROR;
    }

    wanp	= &WANS[l];
    switch (s) {
    case DF :		toggle_drawframes(l);
			return TCL_OK;
			break;

    case RE :		wanp->up	= true;
			break;
    case SE :		wanp->up	= false;
			unschedule_link(wanp->minnode, wanp->minnode_nic);
			unschedule_link(wanp->maxnode, wanp->maxnode_nic);
			break;
    case SSHORT :
    case SLONG :	wanp->up	= false;
			unschedule_link(wanp->minnode, wanp->minnode_nic);
			unschedule_link(wanp->maxnode, wanp->maxnode_nic); 
			if(s == SSHORT)
			    newevent(EV_REBOOT, -l-1, // neg(nicno-1)
				(CnetTime)LINK_SEVER_SHORT*(CnetTime)1000000,
				NULLTIMER, 0);
			else
			    newevent(EV_REBOOT, -l-1, // neg(nicno-1)
				(CnetTime)LINK_SEVER_LONG*(CnetTime)1000000,
				NULLTIMER, 0);
			break;
    }
    if(dflag)
	REPORT("%s->%s.menu.%s\n",
		NODES[wanp->minnode].nodename, NODES[wanp->minnode].nodename,
		menu_strings[s]);
    report_linkstate(l);

    return TCL_OK;
}

void display_wanmenu(WAN *wanp, int x, int y)
{
    static bool	first_time	= true;

    if(first_time) {
	int	s;

	sprintf(chararray, "Sever for %is, repair", LINK_SEVER_SHORT);
	menu_strings[3]	= strdup(chararray);
	CHECKALLOC(menu_strings[3]);
	sprintf(chararray, "Sever for %is, repair", LINK_SEVER_LONG);
	menu_strings[4]	= strdup(chararray);
	CHECKALLOC(menu_strings[4]);

	TCLTK_createcommand("nicmenu_select", nicmenu_select);
	for(s=0 ; s<N_NICMENU_STRINGS ; s++) {
	    sprintf(chararray,"nicmenu_label(%i)", s);
	    Tcl_SetVar(tcl_interp, chararray,
				(char *)menu_strings[s], TCL_GLOBAL_ONLY);
	}
	first_time = false;
    }
    TCLTK("popupNICmenu %i \"between %s and %s\" %i %i %i",
		(int)(wanp-WANS),
		NODES[wanp->minnode].nodename,
		NODES[wanp->maxnode].nodename,
		wanp->up ?  (DFMASK | SEMASK | SSHORTMASK | SLONGMASK) :
			    (DFMASK | REMASK),
		x, y);
}
#else

static TCLTK_COMMAND(nicmenu_select)
{
    return TCL_OK;
}
#endif

#endif	// defined(USE_TCLTK)
