#include "cnetprivate.h"
#include <fcntl.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


#if	defined(USE_TCLTK) && CNET_PROVIDES_KEYBOARD
TCLTK_COMMAND(tk_stdio_input)
{
    int	n	= atoi(argv[1]);
    if(n < -1 || n >= _NNODES) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid node", -1));
	return TCL_ERROR;
    }
    if(strlen(argv[2]) > 0)
	set_keyboard_input(n, argv[2]);
    return TCL_OK;
}
#endif


// -------------------------------------------------------------------------

static void stdio_flush(char *str)
{
    char	*t	= str;
    int		len	= 0;
    int		w;

#if	defined(USE_TCLTK)
    char	tcltk_buf[BUFSIZ];
#endif

    while(*t++)
	len++;

    if(NP->outputfd >= 0) {		// duplicate in local file
//  THIS FOOLISHNESS HUSHES UBUNTU, WHICH SETS -Werror=unused-but-set-variable
	w = write(NP->outputfd, str, (unsigned)len);
	w += 0;
    }

    if(gattr.stdio_quiet)
	return;

#if	defined(USE_TCLTK)
    while(*str) {
	t	= tcltk_buf;
	while(*str && *str != '\n') {
	    if(*str == '"' || *str == '[' || *str == '\\')
		*t++ = '\\';		// elide significant Tcl chars
	    *t++ = *str++;
	}
	*t	= '\0';
	TCLTK("stdiooutput %i \"%s\" %i", THISNODE, tcltk_buf, (*str=='\n'));
	if(*str == '\n')
	    ++str;
    }
#endif
}

int CNET_printf(const char *fmt, ...)
{
    va_list	ap;
    char	stdio_buf[BUFSIZ];

    if(gattr.stdio_quiet && NP->outputfd < 0)	// faster!
	return 0;

    va_start(ap,fmt);
    vsprintf(stdio_buf,fmt,ap);
    va_end(ap);
    stdio_flush(stdio_buf);
    return 0;
}

int CNET_puts(const char *str)		// the substitute for puts()
{
    char	stdio_buf[BUFSIZ];
    char	*p = stdio_buf;

    while((*p++ = *str++))
	;
    *(p-1)	= '\n';
    *p		= '\0';
    stdio_flush(stdio_buf);
    return 0;
}

#if	defined(putchar)
#undef	putchar
#endif

int CNET_putchar(int ch)		// the substitute for putchar()
{
    char	stdio_buf[4];

    stdio_buf[0] = ch; stdio_buf[1] = '\0';
    stdio_flush(stdio_buf);
    return 0;
}

void CNET_clear(void)
{
#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK("stdioclr %i", THISNODE);
#endif
}


// -------------------- NO USER-SERVICEABLE CODE BELOW --------------------

static int set_stdio_outputfile(int n, const char *newfile)
{
    NODE	*np = &NODES[n];

//  CLOSE ANY PREVIOUSLY OPEN OUTPUT FILE
    if(np->nattr.outputfile) {
	unlink(np->nattr.outputfile);
	FREE(np->nattr.outputfile);
	close(np->outputfd);
	np->outputfd = UNKNOWN;
    }

//  IF GIVEN A NEW FILENAME, ATTEMPT TO OPEN IT
    if(newfile) {
	char	*try = format_nodeinfo(np, newfile);		// strdup'ed

	np->outputfd = open(try, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if(np->outputfd >= 0) {
	    np->nattr.outputfile	= try;
	    return 0;
	}
	else
	    F(stderr, "%s: cannot create %s\n", argv0, try);
	FREE(try);
	return 1;
    }
    return 0;
}

int CNET_set_outputfile(const char *filenm)
{
    int		result = -1;

    if(filenm == NULL)
	ERROR(ER_BADARG);
    else {
	result = set_stdio_outputfile(THISNODE, filenm);
	if(result != 0)
	    ERROR(ER_BADARG);
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(\"%s\") = 0\n", __func__, filenm);
	else {
	    if(filenm == NULL)
		filenm = "NULL";
	    TRACE(1, "\t%s(filenm=%s) = -1 %s\n",
			__func__, filenm, cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

void init_stdio_layer(char *oflag)
{
//  COMMAND-LINE ARGUMENT OVERRIDES TOPOLOGY FILE ATTRIBUTE
    if(oflag)
	DEF_NODEATTR.outputfile	= oflag;

//  FOREACH NODE, POSSIBLY OPEN A FILE TO MIRROR ITS stdout
    for(int n=0 ; n<_NNODES ; n++) {
	NODE	*np	= &NODES[n];
	char	*new	= NULL;

	if(np->nattr.outputfile) {
	    new	= strdup(np->nattr.outputfile);
	    CHECKALLOC(new);
	}
	else if(DEF_NODEATTR.outputfile) {
	    new	= strdup(DEF_NODEATTR.outputfile);
	    CHECKALLOC(new);
	}
	set_stdio_outputfile(n, new);
	FREE(new);
    }
}


void reboot_stdio_layer(void)
{
#if	defined(USE_TCLTK) && CNET_PROVIDES_KEYBOARD
    if(Wflag) {
	if(NP->inputline)
	    FREE(NP->inputline);
	NP->inputlen	= 0;
    }
#endif
}
