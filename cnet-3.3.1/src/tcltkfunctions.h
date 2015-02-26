
// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


#ifndef	_TCLTK_FUNCTIONS_H

#define	_TCLTK_FUNCTIONS_H

#if	!defined(CNET4IPOD)

#include <tk.h>

#define	LEFT_BUTTON	1

#define	TCLTK_COMMAND(func)	\
	int func(ClientData data, Tcl_Interp *interp, int argc, char *argv[])

#define TCLTK_createcommand(str, func) \
	Tcl_CreateCommand(tcl_interp, str, (Tcl_CmdProc *)func, \
			(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)

#define	TCLTK_result()	Tcl_GetStringResult(tcl_interp)

extern	void		*tcl_interp;

extern	void		TCLTK(const char *fmt, ...);
extern	int		tcltk_notify_start(void);
extern	int		tcltk_notify_stop(void);
extern	int		tcltk_notify_dispatch(void);

#endif
#endif
