#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

int CNET_vflag(void)
{
    return vflag;
}

bool CNET_Wflag(void)
{
    return Wflag;
}

char *CNET_getvar(const char *name)
{
    NODEATTR	*na = &NP->nattr;

    for(int n=0 ; n<na->nvars ; ++n)
	if(strcmp(name, na->vnames[n]) == 0)
	    return na->vvalues[n];
    return "";
}

void clone_node_vars(NODEATTR *na)
{
    NODEATTR	*da	= &DEF_NODEATTR;
    int		nv	= da->nvars;

    if(nv != 0) {
	na->vnames	= malloc(nv * sizeof(char *));
	CHECKALLOC(na->vnames);
	na->vvalues	= malloc(nv * sizeof(char *));
	CHECKALLOC(na->vvalues);
	na->nvars	= nv;
	for(int n=0 ; n<nv ; ++n) {
	    na->vnames[n]	= strdup(da->vnames[n]);
	    na->vvalues[n]	= strdup(da->vvalues[n]);
	}
    }
    else {
	na->vnames	= NULL;
	na->vvalues	= NULL;
	na->nvars	= 0;
    }
}

void set_node_var(NODEATTR *na, const char *name, const char *value)
{
    int	nv = na->nvars;

    for(int n=0 ; n<nv ; ++n)
	if(strcmp(name, na->vnames[n]) == 0) {
	    free(na->vvalues[n]);
	    na->vvalues[n]	= strdup(value);
	    return;
	}
    na->vnames		= realloc(na->vnames,  (nv+1)*sizeof(char *));
    CHECKALLOC(na->vnames);
    na->vvalues		= realloc(na->vvalues, (nv+1)*sizeof(char *));
    CHECKALLOC(na->vvalues);

    na->vnames[nv]	= strdup(name);
    na->vvalues[nv]	= strdup(value);
    ++na->nvars;
}

//  vim: ts=8 sw=4
