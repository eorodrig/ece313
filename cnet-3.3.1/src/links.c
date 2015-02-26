#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

typedef struct _n {
    CnetLinkType	linktype;
    char		*name;
    NICATTR		nic;
    struct _n		*next;
} NICLIST;

static NICLIST	*nic_head	= NULL;

NICATTR *add_linktype(CnetLinkType linktype, char *name)
{
    NICLIST		*new	= nic_head;

    while(new) {
	if(strcasecmp(name, new->name) == 0) {
	    WARNING("link type '%s' redefined\n", name);
	    break;
	}
	new	= new->next;
    }
    new			= NEW(NICLIST);
    CHECKALLOC(new);
    new->linktype	= linktype;
    new->name		= strdup(name);
    CHECKALLOC(new->name);

    switch (linktype) {
	case LT_LOOPBACK:
		break;
	case LT_WAN:
#if	CNET_PROVIDES_WAN
		init_nicattrs(&new->nic, NULL, NULL);
#endif
		break;
	case LT_LAN:
#if	CNET_PROVIDES_LAN
		init_nicattrs(NULL, &new->nic, NULL);
#endif
		break;
	case LT_WLAN:
#if	CNET_PROVIDES_WLAN
		init_nicattrs(NULL, NULL, &new->nic);
#endif
		break;
    }

    new->next		= nic_head;
    nic_head		= new;

    return(&nic_head->nic);
}

NICATTR *find_linktype(char *name, CnetLinkType *type)
{
    NICLIST	*nlp = nic_head;

    while(nlp) {
	if(strcasecmp(name, nlp->name) == 0) {
	    *type	= nlp->linktype;
	    return(&nlp->nic);
	}
	nlp	= nlp->next;
    }
    return(NULL);
}

void print_linktypes(FILE *topfp)
{
    extern void print_nic1(FILE *, const char *, int,
			   NICATTR *, NICATTR *, const char *);

    NICLIST	*ltp	= nic_head;

    while(ltp) {
	switch (ltp->linktype) {
	case LT_LOOPBACK:
	    break;

	case LT_WAN:
#if	CNET_PROVIDES_WAN
	    F(topfp,"wantype %s {\n", ltp->name);
	    print_nic1(topfp, "wan", false, &ltp->nic, &DEFAULTWAN, "    ");
#endif
	    break;

	case LT_LAN:
#if	CNET_PROVIDES_LAN
	    F(topfp,"lantype %s {\n", ltp->name);
	    print_nic1(topfp, "lan", false, &ltp->nic, &DEFAULTLAN, "    ");
#endif
	    break;

	case LT_WLAN:
#if	CNET_PROVIDES_WAN
	    F(topfp,"wlantype %s {\n", ltp->name);
	    print_nic1(topfp, "wlan", false, &ltp->nic, &DEFAULTWLAN, "    ");
#endif
	    break;
	}
	F(topfp,"}\n\n");
	ltp	= ltp->next;
    }
}
