#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	defined(USE_TCLTK)

#if 0
#define	DoOneEvent	Tcl_DoOneEvent
#define	DoWhenIdle	Tcl_DoWhenIdle
#define	ALL_EVENTS	TCL_ALL_EVENTS
#define	DONT_WAIT	TCL_DONT_WAIT
#endif

void	*tcl_interp;

void *TCLTK_interp(void)
{
    return (void *)tcl_interp;
}

void TCLTK_init(void)
{
    Tk_Window	tcl_mainwindow;

    tcl_interp	= Tcl_CreateInterp();
    Tcl_Preserve(tcl_interp);

    if(Tcl_Init(tcl_interp) != TCL_OK || Tk_Init(tcl_interp) != TCL_OK) {
	FATAL("%s\n", TCLTK_result());
	CLEANUP(1);
    }

    tcl_mainwindow = Tk_MainWindow(tcl_interp);
    if(tcl_mainwindow == NULL) {
	F(stderr, "%s\n", TCLTK_result());
        exit(EXIT_FAILURE);
    }
    Tk_GeometryRequest(tcl_mainwindow, 200, 200);

    Tcl_ResetResult(tcl_interp);
}

void TCLTK(const char *fmt, ...)
{
    if(Wflag) {
	va_list	ap;

	va_start(ap,fmt);
	vsprintf(chararray,fmt,ap);
	va_end(ap);

	if(vflag > 1)
	    REPORT("%s\n", chararray);
	if(Tcl_EvalEx(tcl_interp, chararray, strlen(chararray), 0) != TCL_OK) {
	    FATAL("TCLTK(\"%s\") error - %s\n",
			chararray, TCLTK_result());
	    CLEANUP(1);
	}
    }
}

// -------------------------------------------------------------------

static	int	notify_running	= false;

int tcltk_notify_start(void)
{
    if(notify_running)
	return(false);
    notify_running	= true;
    do {
	Tcl_DoOneEvent(TCL_ALL_EVENTS);
    } while(notify_running);
    notify_running	= false;

    return(true);
}

int tcltk_notify_stop(void)
{
    if(!notify_running)
	return(false);
    notify_running	= false;
    return(true);
}

int tcltk_notify_dispatch(void)
{
    if(notify_running)
	return(false);
    while(Tcl_DoOneEvent(TCL_DONT_WAIT) != 0)
	;
    return(true);
}

#else

void *TCLTK_interp(void)
{
    return NULL;
}

void TCLTK(const char *fmt, ...)
{
}

#endif
