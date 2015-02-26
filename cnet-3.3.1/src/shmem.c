#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

typedef	struct {
    char	*name;
    void	*addr;
    size_t	length;
} SEGMENT;

static	SEGMENT	*segments	= NULL;
static	int	nsegments	= 0;

static void *hidden_shmem(const char *name, size_t wanted)
{
    void	*result	= NULL;

    if(name == NULL || wanted == 0) {
	ERROR(ER_BADARG);
	return NULL;
    }

    SEGMENT	*sp;
    int		s;

    for(s=0, sp=segments ; s<nsegments ; ++s, ++sp) {
	if(strcmp(sp->name, name) == 0) {
	    if(wanted != sp->length)
		ERROR(ER_BADSIZE);
	    else
		result	= sp->addr;
	    return result;
	}
    }

    segments	= realloc(segments, (nsegments+1)*sizeof(SEGMENT));
    if(segments == NULL)
	ERROR(ER_BADALLOC);
    else {
	sp		= &segments[nsegments];
	sp->addr	= calloc((unsigned)1, (size_t)wanted);
	if(sp->addr == NULL)
	    ERROR(ER_BADALLOC);
	else {
	    sp->name	= strdup(name);
	    sp->length	= wanted;
	    ++nsegments;
	    result	= sp->addr;
	}
    }
    return result;
}

void *CNET_shmem2(const char *name, size_t wanted)
{
    void	*result = hidden_shmem(name, wanted);

    if(gattr.trace_events) {
	if(result != NULL)
	    TRACE(0, "\t%s(len=%u) = 0%0xs\n", __func__, wanted, result);
	else
	    TRACE(1, "\t%s(len=%u) = -1 %s\n",
			__func__, wanted, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

void *CNET_shmem(size_t wanted)
{
    void	*result = hidden_shmem("_", wanted);

    if(gattr.trace_events) {
	if(result != NULL)
	    TRACE(0, "\t%s(len=%u) = 0%0xs\n", __func__, wanted, result);
	else
	    TRACE(1, "\t%s(len=%u) = -1 %s\n",
			__func__, wanted, cnet_errname[(int)cnet_errno]);
    }
    return result;
}
