#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

typedef struct _n {
    struct _n	*next;
    void	*addr;
    char	*name;
} T_NAME;

static	T_NAME	*t_name		= NULL;

#if	defined(USE_TCLTK)
static	int	trace_displayed	= false;
#endif

// ---------------------------------------------------------------------

static void trace_forget(void)
{
    T_NAME	*t;

    while(t_name) {
	t	= t_name;
	t_name	= t_name->next;
	FREE(t->name);
	FREE(t);
    }
    t_name	= NULL;
}

static char *annotate(const char *name)
{
    char *s;

    if(*name == '&') {
	s	= strdup(name);
	CHECKALLOC(s);
    }
    else {
	s	= malloc(strlen(name)+2);
	CHECKALLOC(s);
	s[0]	= '&';
	strcpy(&s[1], name);
    }
    return(s);
}

int CNET_trace_name(void *addr, const char *name)
{
    int		result	= -1;

    if(addr == NULL || name == NULL || *name == '\0')
	ERROR(ER_BADARG);
    else {
	T_NAME	*t = t_name;

	while(t) {
	    if(t->addr == addr) {
		FREE(t->name);
		t->name	= annotate(name);
		break;
	    }
	    t	= t->next;
	}
	if(t) {
	    t		= NEW(T_NAME);
	    CHECKALLOC(t);
	    t->addr	= addr;
	    t->name	= annotate(name);
	    t->next	= t_name;
	    t_name	= t;
	}
	result	= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(addr=0x%lx,name=\"%s\") = 0\n",
			__func__, (long)addr, name);
	else
	    TRACE(1, "\t%s(addr=%ld,name=%ld) = -1 %s\n",
			__func__,
			(long)addr, (long)name, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

char *find_trace_name(const void *addr)
{
    T_NAME	*t;
    char	buf[32];

    if(NULL == addr)
	return "NULL";

    t	= t_name;
    while(t) {
	if(t->addr == addr)
	    return(t->name);
	t	= t->next;
    }
    t		= NEW(T_NAME);
    CHECKALLOC(t);
    t->addr	= (void *)addr;
    sprintf(buf, "0x%lx", (long)addr);
    t->name	= strdup(buf);
    CHECKALLOC(t->name);
    t->next	= t_name;
    t_name	= t;
    return t->name;
}

// ---------------------------------------------------------------------

#if	defined(USE_TCLTK)
static char	*maptag	= NULL;
#endif

void TRACE(int err, const char *fmt, ...)
{
    va_list	ap;
    char	buf[4096];

    va_start(ap,fmt);
    vsprintf(buf,fmt,ap);
    va_end(ap);

    if(gattr.tfp) {			// possibly mirror in trace-file
	fputs(buf, gattr.tfp);
	fflush(gattr.tfp);
    }

    if(!Wflag) {			// if not under Tcl/Tk, just stderr
	fputs(buf, stderr);
	return;
    }

#if	defined(USE_TCLTK)
    if(trace_displayed) {
	char	tcltk_buf[4096];
	char	*b=buf, *t;

	while(*b) {
	    t	= tcltk_buf;
	    while(*b && *b != '\n') {
		if(*b == '"' || *b == '[' || *b == '\\')
		    *t++ = '\\';	// elide significant Tcl chars
		*t++ = *b++;
	    }
	    *t	= '\0';
	    TCLTK("traceoutput \"%s\" %s %i",
				tcltk_buf, err ? "bg2" : maptag, (*b=='\n'));
	    if(*b == '\n')
		++b;
	}
    }
#endif
}

void trace_handler(NODE *np, CnetEvent ev, CnetTimerID timer, CnetData data)
{
    char	tbuf[32];

#if	defined(USE_TCLTK)
    static int	h=0;
    maptag	= ((++h % 2) == 0) ? "bg0" : "bg1";
#endif

    if(timer == NULLTIMER)
	strcpy(tbuf, "NULLTIMER");
    else
	sprintf(tbuf, "t=%i", timer);

    switch ((int)ev) {

    case EV_REBOOT : {
	char	**av = (char **)data;
	int	ac, len;
	char	*abuf, *ap;

	for(ac=0, len=1 ; av[ac] ; ++ac)
	    len	+= strlen(av[ac]) + 3;
	abuf	= malloc((unsigned)len);
	CHECKALLOC(abuf);

	for(ac=0, ap=abuf ; av[ac] ; ++ac) {
	    char *a = av[ac];

	    *ap++ = '"';
	    while(*a)
		*ap++ = *a++;
	    *ap++ = '"';
	    if(av[ac+1])
		*ap++ = ',';
	}
	*ap	= '\0';

	TRACE(0,"%s(%s,%s,argv=[%s]) @%susec\n",
		    np->nodename, cnet_evname[(int)ev], tbuf,
		    abuf, CNET_format64(MICROSECONDS));
	FREE(abuf);
	break;
    }
#if	CNET_PROVIDES_WANS
    case EV_LINKSTATE : {
	TRACE(0,"%s(%s,%s,link=%i) @%susec\n",
		    np->nodename, cnet_evname[(int)ev], tbuf,
		    (int)data, CNET_format64(MICROSECONDS));
	break;
    }
#if	!defined(USE_IPOD)
    case EV_DRAWFRAME : {
	TRACE(0,"%s(%s,%s,&draw=0x%lx) @%susec\n",
		    np->nodename, cnet_evname[(int)ev], tbuf,
		    (long)data, CNET_format64(MICROSECONDS));
	break;
    }
#endif
#endif
    default :
	TRACE(0,"%s(%s,%s,data=%ld) @%susec\n",
		    np->nodename, cnet_evname[(int)ev], tbuf,
		    data, CNET_format64(MICROSECONDS));
	break;
    }

//  CALL THE NODE'S HANDLER FOR THIS EVENT
    np->handler[(int)ev](ev, timer, data);

    TRACE(0, "%s(%s)\n", np->nodename, cnet_evname[(int)ev]);
    trace_forget();
}


// ---------------------------------------------------------------------


int CNET_trace(const char *fmt, ...)
{
    va_list	ap;

    if(gattr.trace_events) {
	char	buf[4096];

	va_start(ap,fmt);
	vsprintf(buf,fmt,ap);
	va_end(ap);
	TRACE(0, "\t%s", buf);
    }
    return 0;
}

static void sprint_tmask(char *buf, int mask)
{
    int	e;

    if(mask == TE_NOEVENTS)
	strcpy(buf, "TE_NOEVENTS");
    else if(mask == TE_ALLEVENTS)
	strcpy(buf, "TE_ALLEVENTS");
    else {
	char	*s = buf;

	for(e=1 ; e<N_CNET_EVENTS ; ++e)
	    if(mask & (1<<e)) {
		mask = mask & ~(1<<e);
		sprintf(s, "TE_%s%s", &cnet_evname[e][3], mask ? "|" : "");
		while(*s)
		    ++s;
	    }
    }
}

// ---------------------------------------------------------------------

int CNET_get_trace(void)
{
    int mask	= NP->nattr.trace_mask ;

    if(gattr.trace_events) {
	char	buf[4096];

	sprint_tmask(buf, mask);
	TRACE(0, "\t%s() = %s\n", __func__, buf);
    }
    return(mask);
}

int CNET_set_trace(int newmask)
{
    int oldmask	= NP->nattr.trace_mask ;

    if(!gattr.trace_events && newmask)
	TRACE(0, "nowin %s.%s @%susec\n", NP->nodename,
		    cnet_evname[(int)HANDLING], CNET_format64(MICROSECONDS));

    if(gattr.trace_events || newmask) {
	char	buf1[4096], buf2[4096];

	sprint_tmask(buf1, newmask);
	sprint_tmask(buf2, oldmask);
	TRACE(0, "\t%s(%s) = %s\n", __func__, buf1, buf2);
    }
    NP->nattr.trace_mask = newmask;
    gattr.trace_events =	DEF_NODEATTR.trace_all		|
				NP->nattr.trace_all	|
				(newmask & (1<<(int)HANDLING));
    return(oldmask);
}

// ---------------------------------------------------------------------

static void set_trace_outputfile(const char *newfile)
{
//  CLOSE ANY PREVIOUSLY OPENED TRACE FILE
    if(gattr.trace_filenm) {
	unlink(gattr.trace_filenm);
	FREE(gattr.trace_filenm);
    }
    if(gattr.tfp) {
	fclose(gattr.tfp);
	gattr.tfp	= NULL;
    }

//  IF GIVEN A TRACE FILENAME, TRY TO OPEN IT
    if(newfile) {
	gattr.tfp = fopen(newfile, "w");
	if(gattr.tfp) {
	    gattr.trace_filenm	= strdup(newfile);
	    CHECKALLOC(gattr.trace_filenm);
	}
	else
	    FATAL("cannot create trace-file '%s'\n", newfile);
    }
}

void init_trace(void)
{
    if(gattr.trace_filenm) {
	char	*cp	= strdup(gattr.trace_filenm);

	set_trace_outputfile(cp);
	FREE(cp);
    }
#if	defined(USE_TCLTK)
    if(Wflag)
	Tcl_LinkVar(tcl_interp, "trace_displayed",
				(char *)&trace_displayed, TCL_LINK_BOOLEAN);
#endif
}
