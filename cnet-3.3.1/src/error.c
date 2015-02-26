#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


// ------------------------- RUN-TIME ERRORS --------------------------

const char *cnet_errname[] = {
    "ER_OK",
    "ER_NOTSUPPORTED",
    "ER_NOTREADY",
    "ER_TOOBUSY",

    "ER_BADALLOC",
    "ER_BADARG",
    "ER_BADEVENT",
    "ER_BADLINK",
    "ER_BADNODE",
    "ER_BADSIZE",
    "ER_BADTIMERID",
    "ER_CORRUPTFRAME",
    "ER_LINKDOWN",

#if	CNET_PROVIDES_APPLICATION_LAYER
    "ER_BADSENDER",
    "ER_BADSESSION",
    "ER_DUPLICATEMSG",
    "ER_MISSINGMSG",
    "ER_NOTFORME",
#endif

#if	CNET_PROVIDES_MOBILITY
    "ER_BADPOSITION",
    "ER_NOBATTERY",
#endif
};

#define	N_CNET_ERRORS	(sizeof(cnet_errname) / sizeof(cnet_errname[0]))

const char *cnet_errstr[N_CNET_ERRORS] = {
    /* ER_OK */		  "No error",
    /* ER_NOTSUPPORTED */ "Invalid operation for this node or link type",
    /* ER_NOTREADY */	  "Function called when service not available",
    /* ER_TOOBUSY */	  "Function is too busy/congested to handle request",

    /* ER_BADALLOC */	  "Allocation of dynamic memory failed",
    /* ER_BADARG */	  "Invalid argument passed to a function",
    /* ER_BADEVENT */	  "Invalid event passed to a function",
    /* ER_BADLINK */	  "Invalid link number passed to a function",
    /* ER_BADNODE */	  "Invalid node passed to a function",
    /* ER_BADSIZE */	  "Indicated length is of incorrect size",
    /* ER_BADTIMERID */	  "Invalid CnetTimerID passed to a function",
    /* ER_CORRUPTFRAME */ "Attempted to transfer a corrupt data frame",
    /* ER_LINKDOWN */	  "Attempted to transmit on a link that is down",

#if	CNET_PROVIDES_APPLICATION_LAYER
    /* ER_BADSENDER */	  "Application Layer given msg from an unknown node",
    /* ER_BADSESSION */	  "Application Layer given msg from incorrect session",
    /* ER_DUPLICATEMSG */ "Application Layer given a duplicate msg",
    /* ER_MISSINGMSG */	 "Application Layer given msg before all previous ones",
    /* ER_NOTFORME */	  "Application Layer given msg for another node",
#endif

#if	CNET_PROVIDES_MOBILITY
    /* ER_BADPOSITION */  "Attempt to move mobile node off the map",
    /* ER_NOBATTERY */	  "Battery exhaused on a mobile node",
#endif
};

void CNET_perror(char *user_msg)
{
    if((int)cnet_errno >= 0 && (int)cnet_errno < N_CNET_ERRORS) {
	if(user_msg == NULL || *user_msg == '\0')
	    user_msg	= NP->nodename;
	F(stderr,"%s: %s (%s)\n", user_msg,
		cnet_errname[(int)cnet_errno], cnet_errstr[(int)cnet_errno]);
    }
}


// --------------- COMPILE AND RUNTIME ERRORS -------------------------

#if	!defined(USE_IPOD)
void WARNING(const char *fmt, ...)
{
    va_list	ap;
    char	warn[BUFSIZ];

    va_start(ap,fmt);
	sprintf(warn, "*** warning: %s", fmt);
	vfprintf(stderr,warn,ap);
    va_end(ap);
}
#endif

void REPORT(const char *fmt, ...)	// IFF dflag or vflag
{
    va_list	ap;

    va_start(ap,fmt);
	vfprintf(stderr,fmt,ap);
    va_end(ap);
}

void FATAL(const char *fmt, ...)
{
    va_list	ap;
    char	fatal[BUFSIZ];

    va_start(ap,fmt);
	sprintf(fatal, "%s: %s", argv0, fmt);
	vfprintf(stderr,fatal,ap);
    va_end(ap);
    CLEANUP(1);				// does not return
}

void ERROR(CnetError err)
{
    cnet_errno	= err;
    ++NP->nodestats.nerrors;
    ++gattr.nodestats.nerrors;
}
