#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


// ---------- CALLED VIA cnet -p OR THE "Save Topology" BUTTON ----------

#define	OFFSET	25

static	FILE	*topfp;

void print_nic1(FILE *fp, const char *type,
		    int always, NICATTR *nic, NICATTR *wrt, const char *indent)
{
    char	fmt[64], name[64];

//  FIRSTLY, PRINT THE NIC'S ADDRESS
    if(memcmp(nic->nicaddr, NICADDR_ZERO, LEN_NICADDR) != 0) {
	char	buf[32];

	sprintf(fmt, "%s%%-%is", indent, OFFSET-(int)strlen(indent));
	CNET_format_nicaddr(buf, nic->nicaddr);
	sprintf(name, fmt, "nicaddr");
	F(fp,"%s= %s\n", name, buf);
    }

//  NEXT, THE ATTRIBUTES THAT CAN'T BE CHANGED BY THE GUI
    sprintf(fmt, "%s%s-%%-%is",
		indent, type, OFFSET-(int)strlen(indent)-(int)strlen(type)-1);

    if(always ||
	(nic->bandwidth != DEFAULT && nic->bandwidth != wrt->bandwidth)) {
	sprintf(name, fmt, "bandwidth");
	F(fp,"%s= %ibps\n", name, nic->bandwidth);
    }
    if(always || (nic->mtu != DEFAULT && nic->mtu != wrt->mtu)) {
	sprintf(name, fmt, "mtu");
	F(fp,"%s= %ibytes\n", name, nic->mtu);
    }
    if(always ||
	(nic->jitter != DEFAULT && nic->jitter != wrt->jitter)) {
	sprintf(name, fmt, "jitter");
	F(fp,"%s= %susec\n", name, CNET_format64(nic->jitter));
    }
    if(always ||
	(nic->propagation != DEFAULT && nic->propagation != wrt->propagation)) {
	sprintf(name, fmt, "propagationdelay");
	F(fp,"%s= %susec\n", name, CNET_format64(nic->propagation));
    }
    if(always ||
	(nic->linkmtbf != DEFAULT && nic->linkmtbf != wrt->linkmtbf)) {
	sprintf(name, fmt, "linkmtbf");
	F(fp,"%s= %susec", name, CNET_format64(nic->linkmtbf));
	if(nic->linkmtbf == 0)
	    F(fp,"\t\t/* will not fail */\n");
	else
	    F(fp,"\n");
    }
    if(always ||
	(nic->linkmttr != DEFAULT && nic->linkmttr != wrt->linkmttr)) {
	sprintf(name, fmt, "linkmttr");
	F(fp,"%s= %susec", name, CNET_format64(nic->linkmttr));
	if(nic->linkmttr == 0)
	    F(fp,"\t\t/* instant repair */\n");
	else
	    F(fp,"\n");
    }

/*
    if(nic->linktype == LT_WLAN) {
      if(always ||
  (nic->rxsensitivity != DEFAULT && nic->rxsensitivity != wrt->rxsensitivity)) {
	sprintf(name, fmt, "rxsensitivity");
	F(fp,"%s= %.3fdBm\n", name, nic->rxsensitivity);
      }

      if(always ||
	(nic->txpower != DEFAULT && nic->txpower != wrt->txpower)) {
	sprintf(name, fmt, "txpower");
	F(fp,"%s= %.3fdBm\n", name, nic->txpower);
      }
    }
 */

//  NOW, THE ATTRIBUTES THAT CAN BE CHANGED BY THE GUI
    if(always ||
	(nic->costperbyte != DEFAULT && nic->costperbyte != wrt->costperbyte)) {
	sprintf(name, fmt, "costperbyte");
	F(fp,"%s= %i\n", name, nic->costperbyte);
    }
    if(always ||
	(nic->costperframe != DEFAULT && nic->costperframe != wrt->costperframe)) {
	sprintf(name, fmt, "costperframe");
	F(fp,"%s= %i\n", name, nic->costperframe);
    }
    if(always ||
	(nic->probframecorrupt != DEFAULT && nic->probframecorrupt != wrt->probframecorrupt)) {
	sprintf(name, fmt, "probframecorrupt");
	F(fp,"%s= %i\n", name, nic->probframecorrupt);
    }
    if(always ||
	(nic->probframeloss != DEFAULT && nic->probframeloss != wrt->probframeloss)) {
	sprintf(name, fmt, "probframeloss");
	F(fp,"%s= %i\n", name, nic->probframeloss);
    }
}

static void print_nic0(int always, NICATTR *nic, NICATTR *wrt, int curly)
{
    const char *type="???";
    const char *indent;

    if(always)		indent = "";
    else if(curly)	indent = "         ";
    else		indent = "    ";

    if(curly)
	F(topfp, "{\t/* %s */\n", nic->name);

    switch((int)nic->linktype) {
	case LT_WAN :	type	= "wan";	break;
	case LT_LAN :	type	= "lan";	break;
	case LT_WLAN :	type	= "wlan";	break;
    }

    print_nic1(topfp, type,  always, nic, wrt, indent);

    if(curly)
	fputs("    }",topfp);
}

static void print_nodes_nics(int thisnode)
{
    NODE	*np	= &NODES[thisnode];
    NICATTR	*nic;
    int		n;

    for(n=1, nic=&np->nics[1] ; n<=np->nnics ; n++, nic++) {
	fputs("    ",topfp);

	switch((int)nic->linktype) {
	case LT_WAN :
#if	CNET_PROVIDES_WANS
	{
	    WAN		*wan	= &WANS[ np->wans[n] ];

	    if(wan->minnode == thisnode)
		F(topfp,"wan to %s ",NODES[wan->maxnode].nodename);
	    else
		F(topfp,"wan to %s ",NODES[wan->minnode].nodename);
	    print_nic0(false, nic, &np->defaultwan, true);
	    fputs("\n",topfp);
	}
#endif
	    break;
	case LT_LAN :
#if	CNET_PROVIDES_LANS
	{
	    LAN		*lan	= &LANS[ np->lans[n] ];

	    F(topfp,"lan to %s {\n", lan->name);
	    print_nic0(false, nic, &np->defaultlan, true);
	    fputs("\n",topfp);
	}
#endif
	    break;
	case LT_WLAN :
#if	CNET_PROVIDES_WLANS
	{
	    F(topfp,"wlan ");
	    print_nic0(false, nic, &np->defaultwlan, true);
	    fputs("\n",topfp);
	}
#endif
	    break;
	}
    }
}

void print_node_attr(int always, NODEATTR *me, const char *indent)
{
    char	fmt[64], name[64];

    sprintf(fmt, "%s%%-%is", indent, OFFSET-(int)strlen(indent));
//  FIRSTLY, THE ATTRIBUTES THAT CAN'T BE CHANGED BY THE GUI
    if(always || strcmp(me->compile, DEF_NODEATTR.compile) != 0) {
	sprintf(name, fmt, "compile");
	F(topfp,"%s= \"%s\"\n", name, me->compile);
    }
    if(always || strcmp(me->rebootfunc, DEF_NODEATTR.rebootfunc) != 0) {
	sprintf(name, fmt, "rebootfunc");
	F(topfp,"%s= \"%s\"\n", name, me->rebootfunc);
    }

    if(always ||
	(me->nodemtbf != DEFAULT && me->nodemtbf != DEF_NODEATTR.nodemtbf)) {
	sprintf(name, fmt, "nodemtbf");
	F(topfp,"%s= %susec", name, CNET_format64(me->nodemtbf));
	if(me->nodemtbf == 0)
	    F(topfp,"\t\t/* will not fail */\n");
	else
	    F(topfp,"\n");
    }
    if(always ||
	(me->nodemttr != DEFAULT && me->nodemttr != DEF_NODEATTR.nodemttr)) {
	sprintf(name, fmt, "nodemttr");
	F(topfp,"%s= %susec", name, CNET_format64(me->nodemttr));
	if(me->nodemttr == 0)
	    F(topfp,"\t\t/* instant repair */\n");
	else
	    F(topfp,"\n");
    }

//  NOW, THE ATTRIBUTES THAT CAN BE CHANGED BY THE GUI
    if(always ||
	(me->messagerate != DEFAULT && me->messagerate != DEF_NODEATTR.messagerate)) {
	sprintf(name, fmt, "messagerate");
	F(topfp,"%s= %susec\n", name, CNET_format64(me->messagerate));
    }
    if(always ||
	(me->minmessagesize != DEFAULT && me->minmessagesize != DEF_NODEATTR.minmessagesize)) {
	sprintf(name, fmt, "minmessagesize");
	F(topfp,"%s= %ibytes\n", name, me->minmessagesize);
    }
    if(always ||
	(me->maxmessagesize != DEFAULT && me->maxmessagesize != DEF_NODEATTR.maxmessagesize)) {
	sprintf(name, fmt, "maxmessagesize");
	F(topfp,"%s= %ibytes\n", name, me->maxmessagesize);
    }
}

void save_topology(const char *filenm)
{
    extern void print_nodetypes(FILE *);
    extern void print_linktypes(FILE *);

    bool	commas;
    NODE	*np;
    int		n;

    if(filenm == NULL || *filenm == '\0')
	topfp	= stdout;
    else
	if((topfp = fopen(filenm, "w")) == NULL) {
	    WARNING("%s: cannot create %s\n", filenm);
	    return;
	}

    commas	= CNET_set_commas(false);

//  PRINT ANY GLOBAL ATTRIBUTES THAT HAVE BEEN DEFINED
    F(topfp,"/* global attributes */\n");
    if(gattr.mapimage)
	F(topfp,"mapimage = \"%s\"\n", gattr.mapimage);
    else if(gattr.maptile)
	F(topfp,"maptile = \"%s\"\n", gattr.maptile);
    else if(gattr.mapgrid != 0)
	F(topfp,"mapgrid = %.1f\n", gattr.mapgrid);

    if(gattr.mapscale != 1.0)
	F(topfp,"mapscale = %.1f\n", gattr.mapscale);
    if(gattr.mapmargin != 0.0)
	F(topfp,"mapmargin = %ipx\n", (int)gattr.mapmargin);

    if(gattr.trace_filenm)
	F(topfp,"tracefile = \"%s\"\n", gattr.trace_filenm);
    if(gattr.showcostperframe)
	F(topfp,"showcostperframe = true\n");
    else if(gattr.showcostperbyte)
	F(topfp,"showcostperbyte = true\n");
    F(topfp,"\n");

//  FIRSTLY, PRINT THE DEFAULT NODE AND NIC ATTRIBUTES
    F(topfp,"/* default node attributes */\n");
	print_node_attr(true, &DEF_NODEATTR, "");
    F(topfp,"\n");
#if	CNET_PROVIDES_WANS
    if(gattr.nwans > 0) {
	F(topfp,"/* default WAN attributes */\n");
	    print_nic0(true, &DEFAULTWAN,  NULL, false);
	F(topfp,"\n");
    }
#endif
#if	CNET_PROVIDES_LANS
    if(gattr.nlans > 0) {
	F(topfp,"/* default LAN attributes */\n");
	    print_nic0(true, &DEFAULTLAN,  NULL, false);
	F(topfp,"\n");
    }
#endif
#if	CNET_PROVIDES_WLANS
    if(gattr.nwlans > 0) {
	F(topfp,"/* default WLAN attributes */\n");
	    print_nic0(true, &DEFAULTWLAN, NULL, false);
	F(topfp,"\n");
    }
#endif

    print_nodetypes(topfp);
    print_linktypes(topfp);

//  NEXT, PRINT ANY LAN SEGMENTS THAT WE HAVE
#if	CNET_PROVIDES_LANS
    extern void	print_lans(FILE *fp);
    print_lans(topfp);
#endif

//  FINALLY, FOR EACH NODE, PRINT ITS LOCAL NODE AND NIC INFORMATION
    FOREACH_NODE {
	switch (np->nodetype) {
	    case NT_HOST	: F(topfp, "host ");	break;
	    case NT_ROUTER	: F(topfp, "router ");	break;
	    case NT_MOBILE	: F(topfp, "mobile ");	break;
	    case NT_ACCESSPOINT	: F(topfp, "accesspoint "); break;
	}
	F(topfp,"%s {\n", np->nodename);
	F(topfp,"    x=%.1f, y=%.1f\n",
			np->nattr.position.x, np->nattr.position.y);

	print_node_attr(false, &np->nattr, "    ");
#if	CNET_PROVIDES_WANS
	print_nic0(false, &np->defaultwan,  &DEFAULTWAN, false);
#endif
#if	CNET_PROVIDES_LANS
	print_nic0(false, &np->defaultlan,  &DEFAULTLAN, false);
#endif
#if	CNET_PROVIDES_WLANS
	print_nic0(false, &np->defaultwlan, &DEFAULTWLAN, false);
#endif
	F(topfp,"\n");
	print_nodes_nics(n);
	F(topfp,"}\n");
	if(n<(_NNODES-1))
	    F(topfp,"\n");
    }
    if(topfp != stdout)
	fclose(topfp);

    CNET_set_commas(commas);
}
