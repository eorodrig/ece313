#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_APPLICATION_LAYER

#if	!defined(USE_IPOD)
#include <dlfcn.h>
#endif


static	int	(*INIT_APPLICATION_LAYER)(bool, int);
static	int	(*APPLICATION_BOUNDS)(int *, int *);
static	int	(*REBOOT_APPLICATION_LAYER)(CnetTime *);
static	int	(*POLL_APPLICATION)(CnetTime *);
static	int	(*READ_APPLICATION)(CnetAddr *,  void *, size_t *);
static	int	(*WRITE_APPLICATION)(const void *, size_t *);
static	int	(*ENABLE_APPLICATION)(CnetAddr);
static	int	(*DISABLE_APPLICATION)(CnetAddr);


void init_application_layer(const char *Aflag, bool Qflag, int Sflag)
{
    extern void check_application_bounds(int minmsg, int maxmsg);

#if	defined(USE_IPOD)
    extern int	ipod_init_application_layer(int);
    extern int	ipod_application_bounds(int *minmsg, int *maxmsg);
    extern int	ipod_reboot_application_layer(CnetTime *);
    extern int	ipod_poll_application(CnetTime *);
    extern int	ipod_CNET_read_application(CnetAddr *, void *, size_t *);
    extern int	ipod_CNET_write_application(const void *, size_t *);
    extern int	ipod_CNET_enable_application(CnetAddr);
    extern int	ipod_CNET_disable_application(CnetAddr);

    INIT_APPLICATION_LAYER	= ipod_init_application_layer;
    APPLICATION_BOUNDS		= ipod_application_bounds;
    REBOOT_APPLICATION_LAYER	= ipod_reboot_application_layer;
    POLL_APPLICATION		= ipod_poll_application;
    READ_APPLICATION		= ipod_CNET_read_application;
    WRITE_APPLICATION		= ipod_CNET_write_application;
    ENABLE_APPLICATION		= ipod_CNET_enable_application;
    DISABLE_APPLICATION		= ipod_CNET_disable_application;

#else

    if(Aflag) {
	static	char *PROTOTYPES[] = {
	    "new_init_application_layer(bool, int)",
	    "new_application_bounds(int *minmsg, int *maxmsg)",
	    "new_reboot_application_layer(CnetTime *)",
	    "new_poll_application(CnetTime *)",
	    "new_CNET_read_application(CnetAddr *, void *, size_t *)",
	    "new_CNET_write_application(const void *, size_t *)",
	    "new_CNET_enable_application(CnetAddr)",
	    "new_CNET_disable_application(CnetAddr)"
	};

	extern char	*compile_string(char **, const char *, bool);
	char		*so_file;
	void		*handle;
	char		*name, *b;
	int		f	= 0;

	if((so_file = compile_string(NULL, Aflag, true)) == NULL)
	    CLEANUP(1);

	if((handle = dlopen(so_file, RTLD_LAZY)) == NULL)
	    FATAL("cannot load application layer from %s\n", so_file);

#define	LOAD(fn)				\
	name	= strdup(PROTOTYPES[f]);	\
	CHECKALLOC(name);			\
	b	= strchr(name, '('); *b = '\0';	\
	if(dlsym(handle, name) == NULL)		\
	 FATAL("cannot find the function '%s' in %s\n",PROTOTYPES[f],so_file); \
	FREE(name);				\
	++f

	LOAD(INIT_APPLICATION_LAYER);
	LOAD(APPLICATION_BOUNDS);
	LOAD(REBOOT_APPLICATION_LAYER);
	LOAD(POLL_APPLICATION);
	LOAD(READ_APPLICATION);
	LOAD(WRITE_APPLICATION);
	LOAD(ENABLE_APPLICATION);
	LOAD(DISABLE_APPLICATION);
#undef	LOAD

	// we do *not* dlclose(handle)
    }
    else {
	extern int	std_init_application_layer(bool, int);
	extern int	std_application_bounds(int *minmsg, int *maxmsg);
	extern int	std_reboot_application_layer(CnetTime *);
	extern int	std_poll_application(CnetTime *);
	extern int	std_CNET_read_application(CnetAddr *, void *, size_t *);
	extern int	std_CNET_write_application(const void *, size_t *);
	extern int	std_CNET_enable_application(CnetAddr);
	extern int	std_CNET_disable_application(CnetAddr);

	INIT_APPLICATION_LAYER		= std_init_application_layer;
	APPLICATION_BOUNDS		= std_application_bounds;
	REBOOT_APPLICATION_LAYER	= std_reboot_application_layer;
	POLL_APPLICATION		= std_poll_application;
	READ_APPLICATION		= std_CNET_read_application;
	WRITE_APPLICATION		= std_CNET_write_application;
	ENABLE_APPLICATION		= std_CNET_enable_application;
	DISABLE_APPLICATION		= std_CNET_disable_application;
    }

#endif
    (*INIT_APPLICATION_LAYER)(Qflag, Sflag);

    int minmsg, maxmsg;
    (*APPLICATION_BOUNDS)(&minmsg, &maxmsg);
    check_application_bounds(minmsg, maxmsg);
}

// ----------------------------------------------------------------------

int reboot_application_layer(CnetTime *ask_next)
{
    return (*REBOOT_APPLICATION_LAYER)(ask_next);
}

int poll_application(CnetTime *ask_next)
{
    return (*POLL_APPLICATION)(ask_next);
}

int CNET_read_application(CnetAddr *destaddr, void *msg, size_t *len)
{
    int	result		= -1;
    int	lengiven	= 0;

    if(destaddr == NULL || msg == NULL || len == NULL || *len == 0)
	ERROR(ER_BADARG);
    else if(!NODE_HAS_AL(THISNODE))
	ERROR(ER_NOTSUPPORTED);
    else {
	lengiven	= *len;
	result		= (*READ_APPLICATION)(destaddr, msg, len);
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(%s,%s,*len=%ld) = 0 (*dest=%lu,*len=%i)\n",
		    __func__,
		    find_trace_name(destaddr), find_trace_name(msg), lengiven,
		    *destaddr, *len);
	else {
	    char	buf[64];

	    if(len == NULL)
		strcpy(buf, "NULL");
	    else
		sprintf(buf, "*len=%zd", *len);

	    TRACE(1, "\t%s(%s,%s,%s) = -1 %s\n",
		    __func__,
		    find_trace_name(destaddr), find_trace_name(msg),
		    buf,
		    cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

int CNET_write_application(const void *msg, size_t *len)
{
    int result 		= -1;
    int lengiven	= 0;

    if(msg == NULL || len == NULL || *len == 0)
	ERROR(ER_BADARG);
    else if(!NODE_HAS_AL(THISNODE))
	ERROR(ER_NOTSUPPORTED);
    else {
	lengiven	= *len;
	result		= (*WRITE_APPLICATION)(msg, len);
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(%s,*len=%ld) = 0 (*len=%i)\n",
			__func__,
			find_trace_name(msg), lengiven, *len);
	else {
	    char	buf[64];

	    if(len == NULL)
		strcpy(buf, "NULL");
	    else
		sprintf(buf, "*len=%zd", *len);

	    TRACE(1, "\t%s(%s,%s) = -1 %s\n",
			__func__,
			find_trace_name(msg), buf,
			cnet_errname[(int)cnet_errno]);
	}
    }
    return result;
}

// --------------    Enable/Disable Application Layer -------------------


int CNET_enable_application(CnetAddr destaddr)
{
    int	result	= -1;

    if(!NODE_HAS_AL(THISNODE))
	ERROR(ER_NOTSUPPORTED);
    else
	result	= (*ENABLE_APPLICATION)(destaddr);

    if(gattr.trace_events) {
	char	buf[32];

	if(destaddr == ALLNODES)
	    strcpy(buf, "ALLNODES");
	else
	    sprintf(buf, "dest=%lu", (unsigned long)destaddr);

	if(result == 0)
	    TRACE(0, "\t%s(%s) = 0\n", __func__, buf);
	else
	    TRACE(1, "\t%s(%s) = -1 %s\n",
			__func__, buf, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_disable_application(CnetAddr destaddr)
{
    int	result	= -1;

    if(!NODE_HAS_AL(THISNODE))
	ERROR(ER_NOTSUPPORTED);
    else
	result	= (*DISABLE_APPLICATION)(destaddr);

    if(gattr.trace_events) {
	char	buf[32];

	if(destaddr == ALLNODES)
	    strcpy(buf, "ALLNODES");
	else
	    sprintf(buf, "dest=%lu", (unsigned long)destaddr);

	if(result == 0)
	    TRACE(0, "\t%s(%s) = 0\n", __func__, buf);
	else
	    TRACE(1, "\t%s(%s) = -1 %s\n",
			__func__, buf, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

#endif
