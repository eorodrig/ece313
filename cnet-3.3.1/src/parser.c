#include "cnetprivate.h"
#include "lexical.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define expect(t, msg)	if(token == t) gettoken(); \
			else SYNTAX("%s expected\n", msg)

#define	skip_commas()	while(token == T_COMMA) gettoken()

#define	SORRY_NO_WANS	\
	SYNTAX("this implementation of %s does not support WANS\n", argv0)
#define	SORRY_NO_LANS	\
	SYNTAX("this implementation of %s does not support LANS\n", argv0)
#define	SORRY_NO_WLANS	\
	SYNTAX("this implementation of %s does not support WLANS\n", argv0)

static void SYNTAX(const char *fmt, ...)
{
    va_list	ap;

    va_start(ap, fmt);

//  POINT TO THE ERROR
    fputs(input.line, stderr);
    for(int i=0 ; i<input.cc ; ++i)
	if(input.line[i] == '\t')
	     fputc('\t', stderr);
	else fputc(' ', stderr);
    F(stderr, "\n%s, line %i: ", input.name,input.lc);

    vfprintf(stderr, fmt, ap);
    va_end(ap);
    CLEANUP(1);
}

// ------------------- CHECK ALL REQUIRED DATA TYPES --------------------

static void check_integer(int *i)
{
    if(token != T_INTCONST)
	SYNTAX("integer constant expected\n"); 
    *i = input.ivalue;
    gettoken();
}

static void check_positive(int *i)
{
    if(token != T_INTCONST || input.ivalue <= 0)
	SYNTAX("positive integer constant expected\n"); 
    *i = input.ivalue;
    gettoken();
}

static void check_double(double *x)
{
    switch ((int)token) {
	case T_INTCONST :
	    *x = (double)input.ivalue;
	    break;
	case T_REALCONST :
	    *x = input.dvalue;
	    break;
	default:
	    SYNTAX("integer or floating point constant expected\n"); 
	    break;
    }
    gettoken();
}

static void check_Boolean(bool *truth)
{
    if(token == T_TRUE)
	*truth = true;
    else if(token == T_FALSE)
	*truth = false;
    else if(token == T_TOGGLE)
	*truth = !(*truth);
    else
	SYNTAX("Boolean attribute expected\n"); 
    gettoken();
}

static void check_string(char **strp)
{
    if(token != T_STRCONST)
	SYNTAX("string constant expected\n"); 
    else {
	*strp = strdup(chararray);
	CHECKALLOC(*strp);
    }
    gettoken();
}

static void check_distance(double *x)
{
    switch ((int)token) {
	case T_INTCONST :
	    *x = (double)input.ivalue;
	    break;
	case T_REALCONST :
	    *x = input.dvalue;
	    break;
	default:
	    SYNTAX("integer or floating point constant expected\n"); 
	    break;
    }
    gettoken();
//  ONLY ASSUMED UNITS OF DISTANCE ARE m
    if(token == T_METRES || token == T_PIXELS)
	gettoken();
}

static void check_time_scale(CnetTime *value)
{
//  UNITS OF ALL TIMES ARE MICROSECONDS
    CnetTime	value64;

    if(token != T_INTCONST || input.ivalue < 0)
	SYNTAX("time constant expected\n"); 
    value64	= (CnetTime)input.ivalue;
    gettoken();

    switch (token) {
	case T_USEC :						break;
	case T_MSEC :	value64	*= (CnetTime)1000;		break;
	case T_SEC :	value64 *= (CnetTime)1000000;		break;
	default :	SYNTAX("'s', 'ms' or 'us' expected\n"); break;
    }
    *value	= value64;
    gettoken();
}

#if	CNET_PROVIDES_WLANS
static void check_current(double *value)
{
//  ONLY POSSIBLE UNITS OF CURRENT ARE mA
    check_double(value);
    if(token != T_mA)
	SYNTAX("mA expected\n"); 
    gettoken();
}

static void check_scale_wlanfreq(double *value)
{
//  UNITS OF WLAN's frequency ARE GHz
    check_double(value);
    switch (token) {
	case T_GHz :						break;
	case T_MHz :		*value = *value / 1000.0;	break;
	default :		SYNTAX("GHz or MHz expected\n"); break;
    }
    gettoken();
}

static void check_scale_wlanpower(double *value)
{
//  UNITS OF WLAN's power ARE dBm
    check_double(value);
    switch (token) {
	case T_dBm :						break;
	case T_mW :		*value = mW2dBm(*value);	break;
	default :		SYNTAX("dBm or mW expected\n"); break;
    }
    gettoken();
}
#endif

// ----------------- SCALE ATTRIBUTES BY BASIC MULTIPLIERS --------------

static void scale_bps(int *value)
{
//  UNITS OF nodeattr.bandwidth ARE BITS PER SECOND
    switch (token) {
	case T_BITSPS :							break;
	case T_KBITSPS :	*value *= 1000;				break;
	case T_MBITSPS :	*value *= (1000*1000);			break;
	case T_BYTESPS :	*value *= 8;				break;
	case T_KBYTESPS :	*value *= (8*1000);			break;
	case T_MBYTESPS :	*value *= (8*1000*1000);		break;
	default :		SYNTAX("bandwidth units expected\n");	break;
    }
    if(*value < 0)		// attempt to detect overflow
	SYNTAX("value overflows an integer\n");
    gettoken();
}

static void scale_bytes(int *value)
{
//  UNITS OF nodeattr.*messagesize ARE BYTES
    switch (token) {
	case T_BYTES :						 break;
	case T_KBYTES :		*value *= 1024;			 break;
	case T_MBYTES :		*value *= (1024*1024);		 break;
	default :		SYNTAX("byte units expected\n"); break;
    }
    if(*value < 0)		// attempt to detect overflow
	SYNTAX("value overflows an integer\n");
    gettoken();
}


// ----------------------------------------------------------------------

static void node_attr(TOKEN attr, NODEATTR *na)
{
    gettoken();
    if(attr == T_NODEATTR_VAR) {
	char	*name;

	if(token != T_NAME)
	    SYNTAX("variable name expected\n");
	name = strdup(chararray);
	CHECKALLOC(name);
	gettoken();
	expect(T_EQ,"'='");
	if(token != T_STRCONST)
	    SYNTAX("variable's string value expected\n");
	set_node_var(na, name, chararray);
	free(name);
	gettoken();
	return;
    }

    expect(T_EQ,"'='");

    switch ((int)attr) {
    case T_NODEATTR_ADDRESS :
	check_integer((int *)&na->address);
	break;
    case T_NODEATTR_COMPILE :
	check_string(&na->compile);
	break;
    case T_NODEATTR_ICONTITLE :
	check_string(&na->icontitle);
	break;

    case T_NODEATTR_MAXMESSAGESIZE :
	check_positive(&na->maxmessagesize);
	scale_bytes(&na->maxmessagesize);
	break;
    case T_NODEATTR_MESSAGERATE :
	check_time_scale(&na->messagerate);
	break;
    case T_NODEATTR_MINMESSAGESIZE :
	check_positive(&na->minmessagesize);
	scale_bytes(&na->minmessagesize);
	break;

    case T_NODEATTR_MTBF :
	check_time_scale(&na->nodemtbf);
	break;
    case T_NODEATTR_MTTR :
	check_time_scale(&na->nodemttr);
	break;
    case T_NODEATTR_OUTPUTFILE :
	check_string(&na->outputfile);
	break;
    case T_NODEATTR_REBOOTARGV : {
	extern	void init_reboot_argv(NODEATTR *, int, char **);

	int	argc;
	char	**argv;
	char	*str	= "";
	char	*s, *t, ch;

	check_string(&str);
	argc	= 0;
	argv	= malloc((strlen(str)/2+2) * sizeof(char *));
	CHECKALLOC(argv);

	t = str;
	while(*t) {
	    while(*t == ' ' || *t == '\n')
		++t;
	    if(*t == '\0')
		break;
	    s	= t;
	    while(*t && (*t != ' ' && *t != '\t'))
		++t;
	    ch	= *t;
	    *t	= '\0';

	    argv[argc]	= strdup(s);
	    CHECKALLOC(argv[argc]);
	    ++argc;

	    *t	= ch;
	}
	argv[argc]	= NULL;

	init_reboot_argv(na, argc, argv);
	FREE(argv);
	FREE(str);
	break;
    }
    case T_NODEATTR_REBOOTFUNC :
	check_string(&na->rebootfunc);
	break;
    case T_NODEATTR_TRACE :
	check_Boolean(&na->trace_all);
	break;
    case T_NODEATTR_WINOPEN :
	check_Boolean(&na->winopen);
	break;
    case T_NODEATTR_WINX :
	check_integer(&na->winx);
	break;
    case T_NODEATTR_WINY :
	check_integer(&na->winy);
	break;
/*
    case T_NODEATTR_X :
	check_integer(&na->position.x);
	break;
    case T_NODEATTR_Y :
	check_integer(&na->position.y);
	break;
    case T_NODEATTR_Z :
	check_integer(&na->position.z);
	break;
 */
    case T_NODEATTR_X :
	check_double(&na->position.x);
	break;
    case T_NODEATTR_Y :
	check_double(&na->position.y);
	break;
    case T_NODEATTR_Z :
	check_double(&na->position.z);
	break;
    }
}

static void link_attr(TOKEN attr,
		      NICATTR *wanattr, NICATTR *lanattr, NICATTR *wlanattr)
{
#if	!CNET_PROVIDES_WANS
    SORRY_NO_WANS;
#endif
#if	!CNET_PROVIDES_LANS
    SORRY_NO_LANS;
#endif
#if	!CNET_PROVIDES_WLANS
    SORRY_NO_WLANS;
#endif

    if(wanattr == NULL	&& is_wan_attr(attr))
	SYNTAX("WAN attributes not permitted here\n");
    if(lanattr == NULL	&& is_lan_attr(attr))
	SYNTAX("LAN attributes not permitted here\n");
    if(wlanattr == NULL	&& is_wlan_attr(attr))
	SYNTAX("WLAN attributes not permitted here\n");

    gettoken();
    expect(T_EQ,"'='");

    switch ((int)attr) {
//  WAN SPECIFIC ATTRIBUTES
#if	CNET_PROVIDES_WANS
    case T_WANATTR_BANDWIDTH :
	check_positive((int *)&wanattr->bandwidth);
	scale_bps((int *)&wanattr->bandwidth);
	break;
    case T_WANATTR_BUFFERED :
	check_Boolean(&wanattr->buffered);
	break;
    case T_WANATTR_JITTER :
	check_time_scale(&wanattr->jitter);
	break;
    case T_WANATTR_MTU :
	check_positive(&wanattr->mtu);
	scale_bytes(&wanattr->mtu);
	break;
    case T_WANATTR_PROPAGATION :
	check_time_scale(&wanattr->propagation);
	break;
    case T_WANATTR_COSTPERBYTE :
	check_integer(&wanattr->costperbyte);
	if(token == T_C) gettoken();
	break;
    case T_WANATTR_COSTPERFRAME :
	check_integer(&wanattr->costperframe);
	if(token == T_C) gettoken();
	break;
    case T_WANATTR_MTBF :
	check_time_scale(&wanattr->linkmtbf);
	break;
    case T_WANATTR_MTTR :
	check_time_scale(&wanattr->linkmttr);
	break;
    case T_WANATTR_PROBFRAMECORRUPT :
	check_integer(&wanattr->probframecorrupt);
	break;
    case T_WANATTR_PROBFRAMELOSS :
	check_integer(&wanattr->probframeloss);
	break;
#endif

//  LAN SPECIFIC ATTRIBUTES
#if	CNET_PROVIDES_LANS
    case T_LANATTR_BANDWIDTH :
	check_positive((int *)&lanattr->bandwidth);
	scale_bps((int *)&lanattr->bandwidth);
	break;
    case T_LANATTR_BUFFERED :
	check_Boolean(&lanattr->buffered);
	break;
    case T_LANATTR_JITTER :
	check_time_scale(&lanattr->jitter);
	break;
    case T_LANATTR_MTU :
	check_positive(&lanattr->mtu);
	scale_bytes(&lanattr->mtu);
	break;
    case T_LANATTR_PROPAGATION :
	check_time_scale(&lanattr->propagation);
	break;
    case T_LANATTR_COSTPERBYTE :
	check_integer(&lanattr->costperbyte);
	if(token == T_C) gettoken();
	break;
    case T_LANATTR_COSTPERFRAME :
	check_integer(&lanattr->costperframe);
	if(token == T_C) gettoken();
	break;
    case T_LANATTR_MTBF :
	check_time_scale(&lanattr->linkmtbf);
	break;
    case T_LANATTR_MTTR :
	check_time_scale(&lanattr->linkmttr);
	break;
    case T_LANATTR_PROBFRAMECORRUPT :
	check_integer(&lanattr->probframecorrupt);
	break;
    case T_LANATTR_PROBFRAMELOSS :
	check_integer(&lanattr->probframeloss);
	break;
    case T_LANATTR_SLOTTIME :
	check_time_scale(&lanattr->slottime);
	break;
#endif

//  WLAN SPECIFIC ATTRIBUTES
#if	CNET_PROVIDES_WLANS
    case T_WLANATTR_BANDWIDTH :
	check_positive((int *)&wlanattr->bandwidth);
	scale_bps((int *)&wlanattr->bandwidth);
	break;
    case T_WLANATTR_BUFFERED :
	check_Boolean(&wlanattr->buffered);
	break;
    case T_WLANATTR_JITTER :
	check_time_scale(&lanattr->jitter);
	break;
    case T_WLANATTR_MTU :
	check_positive(&wlanattr->mtu);
	scale_bytes(&wlanattr->mtu);
	break;

    case T_WLANATTR_FREQUENCY :
	check_scale_wlanfreq(&wlanattr->wlaninfo->frequency_GHz);
	break;

    case T_WLANATTR_TXPOWER :
	check_scale_wlanpower(&wlanattr->wlaninfo->tx_power_dBm);
	break;
    case T_WLANATTR_TXCABLELOSS :
	check_scale_wlanpower(&wlanattr->wlaninfo->tx_cable_loss_dBm);
	break;
    case T_WLANATTR_TXANTENNAGAIN :
	check_scale_wlanpower(&wlanattr->wlaninfo->tx_antenna_gain_dBi);
	break;

    case T_WLANATTR_RXANTENNAGAIN :
	check_scale_wlanpower(&wlanattr->wlaninfo->rx_antenna_gain_dBi);
	break;
    case T_WLANATTR_RXCABLELOSS :
	check_scale_wlanpower(&wlanattr->wlaninfo->rx_cable_loss_dBm);
	break;
    case T_WLANATTR_RXSENSITIVITY :
	check_scale_wlanpower(&wlanattr->wlaninfo->rx_sensitivity_dBm);
	break;
    case T_WLANATTR_RXSIGNALTONOISE :
	check_scale_wlanpower(&wlanattr->wlaninfo->rx_signal_to_noise_dBm);
	break;

    case T_WLANATTR_IDLECURRENT :
	check_current(&wlanattr->wlaninfo->idle_current_mA);
	break;
    case T_WLANATTR_SLEEPCURRENT :
	check_current(&wlanattr->wlaninfo->sleep_current_mA);
	break;
    case T_WLANATTR_RXCURRENT :
	check_current(&wlanattr->wlaninfo->rx_current_mA);
	break;
    case T_WLANATTR_TXCURRENT :
	check_current(&wlanattr->wlaninfo->tx_current_mA);
	break;

    case T_WLANATTR_COSTPERBYTE :
	check_integer(&wlanattr->costperbyte);
	if(token == T_C) gettoken();
	break;
    case T_WLANATTR_COSTPERFRAME :
	check_integer(&wlanattr->costperframe);
	if(token == T_C) gettoken();
	break;
    case T_WLANATTR_MTBF :
	check_time_scale(&wlanattr->linkmtbf);
	break;
    case T_WLANATTR_MTTR :
	check_time_scale(&wlanattr->linkmttr);
	break;
    case T_WLANATTR_PROBFRAMECORRUPT :
	check_integer(&wlanattr->probframecorrupt);
	break;
    case T_WLANATTR_PROBFRAMELOSS :
	check_integer(&wlanattr->probframeloss);
	break;
#endif
    default:
	break;
    }
}

static void node_attrs(NODEATTR *nodeattr,
			NICATTR *wanattr, NICATTR *lanattr, NICATTR *wlanattr)
{
    for(;;) {
	if(is_node_attr(token))
	    node_attr(token, nodeattr);
	else if(is_nic_attr(token))
	    link_attr(token, wanattr, lanattr, wlanattr);
	else if(token == T_COMMA)
	    gettoken();
	else
	    break;
    }
}


// ----------------------------------------------------------------------

static FILE *preprocess_topology(char **defines, char *filenm)
{
    extern FILE	*fdopen(int, const char *);

    char	*av[64];		// hoping that this is enough
    char	*cpp, *tmp;
    int		ac=0, p[2];

    if(pipe(p) == -1)
	FATAL("cannot create pipe\n");

    switch (fork()) {
    case -1 :	FATAL("cannot fork\n");
		break;

//  $CNETCPP file (in child) | parse() (in parent)
    case 0  :	close(p[0]);				// child
		dup2(p[1],1);
		close(p[1]);

		cpp		= strdup(findenv("CNETCPP", CNETCPP));
		CHECKALLOC(cpp);
		av[ac++]	= (tmp = strrchr(cpp,'/')) ? tmp+1 : cpp;

		if(strstr(cpp, "gcc")) {
		    av[ac++]	= "-E";
		    av[ac++]	= "-x";
		    av[ac++]	= "c-header";
		}

		av[ac++]	= OS_DEFINE;
		if(defines)
		    while(*defines)		// add any -D.. switches
		       av[ac++] = *defines++;
		av[ac++]	= filenm;
		av[ac]		= NULL;

		if(dflag) {
		    REPORT("%s",cpp);
		    for(ac=1 ; av[ac] ; ++ac)
			REPORT(" %s", av[ac]);
		    REPORT("\n");
		}

		execv(cpp, av);
		FATAL("cannot exec %s\n", cpp);
		break;
    default  :	close(p[1]);			// parent
		return fdopen(p[0],"r");
		break;
    }
    return NULL;
}

#if	CNET_PROVIDES_WANS
static void wan_defn(int thisnode, NICATTR *nic)
{
    extern int	add_node(CnetNodeType, char *, bool, NODEATTR *);

    if(!NODE_HAS_WANS(thisnode))
	SYNTAX("this nodetype may not have fixed WAN links\n");

    gettoken();
    expect(T_TO,"'to'");
    if(token == T_NAME) {
	extern NICATTR	*add_wan(int thisn, int othern);
	int		othernode;

	othernode	= add_node(NT_HOST, chararray, false, NULL);
	if(thisnode == othernode)
	    SYNTAX("link to oneself is invalid\n");

	nic	= add_wan(thisnode, othernode);
	gettoken();
	if(token == T_LCURLY) {
	    gettoken();
	    while(is_nic_attr(token))
		link_attr(token, nic, NULL, NULL);
	    expect(T_RCURLY,"'}'");
	}
    }
    else
	SYNTAX("name of node at other end of WAN expected\n");
}
#endif

#if	CNET_PROVIDES_LANS
static void lan_defn(int thisnode, NICATTR *nic)
{
    extern int	    add_lan(char *name);
    extern NICATTR *extend_lan(int thislan, int thisn);

    char	*lanname;
    int		newlan;

    if(!NODE_HAS_LANS(thisnode))
	SYNTAX("this nodetype may not have fixed LAN links\n");

    gettoken();
    if(token == T_JOIN || token == T_TO)
	gettoken();

    if(token != T_NAME)
	SYNTAX("LAN segment name expected\n");
    lanname	= strdup(chararray);
    CHECKALLOC(lanname);
    gettoken();

    newlan	= add_lan(lanname);
    FREE(lanname);

    expect(T_LCURLY,"'{'");
    nic	= extend_lan(newlan, thisnode);
    for(;;) {
	if(token == T_NICADDR) {
	    gettoken();
	    expect(T_EQ,"'='");
	    if(token == T_NICADDRVALUE) {
		memcpy(nic->nicaddr, &input.nicaddr, LEN_NICADDR);
		gettoken();
	    }
	    else
		SYNTAX("NIC address expected\n");
	}
	else if(is_nic_attr(token))
	    link_attr(token, NULL, nic, NULL);
	else
	    break;
    }
    expect(T_RCURLY,"'}'");
}
#endif

#if	CNET_PROVIDES_WLANS
static void wlan_defn(int thisnode, NICATTR *nic)
{
    extern NICATTR	*add_wlan(int thisn);

    if(!NODE_HAS_WLANS(thisnode))
	SYNTAX("this nodetype may not have WLAN links\n");

    nic	= add_wlan(thisnode);
    gettoken();
    expect(T_LCURLY,"'{'");
    for(;;) {
	if(token == T_NICADDR) {
	    gettoken();
	    expect(T_EQ,"'='");
	    if(token == T_NICADDRVALUE) {
		memcpy(nic->nicaddr, &input.nicaddr, LEN_NICADDR);
		gettoken();
	    }
	    else
		SYNTAX("NIC address expected\n");
	}
	else if(is_nic_attr(token))
	    link_attr(token, NULL, NULL, nic);
	else
	    break;
    }
    expect(T_RCURLY,"'}'");
}
#endif

static int node_defn(CnetNodeType nodetype, NODEATTR *defaults)
{
    extern int	add_node(CnetNodeType, char *name, bool defn, NODEATTR *);

    NODE	*np		= NULL;
    int		thisnode	= UNKNOWN;
    NICATTR	*nic;

    if(token == T_NAME) {
	thisnode	= add_node(nodetype, chararray, true, defaults);
	np		= &NODES[thisnode];
	gettoken();
    }
    else
	SYNTAX("node name expected\n");

    expect(T_LCURLY,"'{'");

//  GLOBAL ATTRIBUTES FOR THIS NODE
    for(;;) {

//  PERFORM SOME SANITY CHECKS ABOUT THE CURRENT ATTRIBUTE NAME
	if(is_node_attr(token) || is_nic_attr(token)) {
	    if(is_AL_attr(token) && !NODE_HAS_AL(thisnode))
		SYNTAX("this nodetype does not have an Application Layer\n");

	    if(is_wan_attr(token) && !NODE_HAS_WANS(thisnode))
		SYNTAX("this nodetype may not specify WAN attributes\n");

	    if(is_lan_attr(token) && !NODE_HAS_LANS(thisnode))
		SYNTAX("this nodetype may not specify LAN attributes\n");

	    if(is_wlan_attr(token) && !NODE_HAS_WLANS(thisnode))
		SYNTAX("this nodetype may not specify WLAN attributes\n");

	    node_attrs(&np->nattr,
#if	CNET_PROVIDES_WANS
			&np->defaultwan,
#else
			NULL,
#endif
#if	CNET_PROVIDES_LANS
			&np->defaultlan,
#else
			NULL,
#endif
#if	CNET_PROVIDES_WLANS
			&np->defaultwlan
#else
			NULL
#endif
			);
	}
	else if(is_compass_direction(token)) {
	    int	dx=0, dy=0;

	    while(is_compass_direction(token)) {
		switch ((int)token) {
		case T_NORTH	: --dy;		break;
		case T_NORTHEAST: --dy;	++dx;	break;
		case T_EAST	: 	++dx;	break;
		case T_SOUTHEAST: ++dy;	++dx;	break;
		case T_SOUTH	: ++dy;		break;
		case T_SOUTHWEST: ++dy;	--dx;	break;
		case T_WEST	: 	--dx;	break;
		case T_NORTHWEST: --dy;	--dx;	break;
		}
		gettoken();
	    }
	    expect(T_OF,"'of' or compass direction");
	    if(token == T_NAME) {
		NODE	*onp;
		int	othernode;

		othernode	= add_node(NT_HOST, chararray, false, NULL);
		if(othernode == thisnode)
		    SYNTAX("this direction makes no sense\n");
		onp		= &NODES[othernode];
		np->nattr.position.x	=
			    onp->nattr.position.x + 2*dx*DEFAULT_NODE_SPACING;
		np->nattr.position.y	=
			    onp->nattr.position.y + 2*dy*DEFAULT_NODE_SPACING;
		gettoken();
	    }
	    else
		SYNTAX("node name expected\n");
	    skip_commas();
	}
	else if(token == T_EXTENSION) {
	    if(gattr.nextensions < MAXEXTENSIONS)
		check_string(&gattr.extensions[gattr.nextensions++]);
	}
	else
	    break;
    }

//  FINISHED WITH ALL GLOBAL ATTRIBUTES, RESET PRE-EXISTING WAN ATTRIBUTES
#if	CNET_PROVIDES_WANS
    int		n;
    for(n=1, nic=&np->nics[1] ; n<=np->nnics ; n++, nic++) {
	if(nic->linktype == LT_WAN) {
	    char	*save	= nic->name;

	    memcpy(nic, &np->defaultwan, sizeof(NICATTR));
	    nic->name	= save;
	}
    }
#endif

//  SPECIFIC LINK CONNECTION DETAILS OF THIS NODE
    while(token==T_NAME || token==T_WAN || token==T_LAN || token==T_WLAN) {

	if(token == T_NAME) {
	    extern NICATTR	*find_linktype(char *, CnetLinkType *);

	    CnetLinkType	nictype;

	    nic		= find_linktype(chararray, &nictype);
	    if(nic == NULL)
		SYNTAX("unknown linktype\n");

	    switch(nictype) {
	    case LT_LOOPBACK :
		break;
	    case LT_WAN :
#if	CNET_PROVIDES_WANS
		wan_defn(thisnode, nic);
#endif
		break;
	    case LT_LAN :
#if	CNET_PROVIDES_LANS
		lan_defn(thisnode, nic);
#endif
		break;
	    case LT_WLAN :
#if	CNET_PROVIDES_WLANS
		wlan_defn(thisnode, nic);
#endif
		break;
	    }
	}
//  ADD A NEW WAN NIC TO THIS NODE
	else if(token == T_WAN)
#if	CNET_PROVIDES_WANS
	    wan_defn(thisnode, &np->defaultwan);
#else
	    SORRY_NO_WANS;
#endif
//  ADD A NEW LAN NIC TO THIS NODE
	else if(token == T_LAN)
#if	CNET_PROVIDES_LANS
	    lan_defn(thisnode, &np->defaultlan);
#else
	    SORRY_NO_LANS;
#endif
//  ADD A NEW WLAN NIC TO THIS NODE
	else if(token == T_WLAN)
#if	CNET_PROVIDES_WLANS
	    wlan_defn(thisnode, &np->defaultwlan);
#else
	    SORRY_NO_WLANS;
#endif

	skip_commas();
    }

    expect(T_RCURLY,"'}'");
    return thisnode;
}

#if	CNET_PROVIDES_LANS
static void lansegment_defn(void)
{
    extern int	add_lan(char *name);

    LAN			*lan;
    NICATTR		*lanattr;
    char		*lanname;
    int			thislan	= UNKNOWN;

    if(token != T_NAME)
	SYNTAX("LAN segment name expected\n");
    lanname	= strdup(chararray);
    CHECKALLOC(lanname);
    thislan	= add_lan(lanname);
    FREE(lanname);
    lan		= &LANS[thislan];
    lanattr	= &lan->segmentnic;

    gettoken();
    expect(T_LCURLY,"'{'");
    for(;;) {
	if(token == T_NODEATTR_X) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_positive(&lan->x);
	}
	else if(token == T_NODEATTR_Y) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_positive(&lan->y);
	}
	else if(is_nic_attr(token))
	    link_attr(token, NULL, lanattr, NULL);
	else if(token == T_COMMA)
	    gettoken();
	else
	    break;
    }
    expect(T_RCURLY,"'}'");
}
#endif

void parse_topology(char *topology, char **defines)
{
    extern NODEATTR	*add_nodetype(CnetNodeType ntype, char *name);
    extern NICATTR	*add_linktype(CnetLinkType, char *);

    NODEATTR	*nodeattr	= NULL;
    NICATTR	*nic		= NULL;

    int		run_cpp		= (defines[0] != NULL);
    char	*s;

//  PARSING TOPOLOGY FROM STDIN?
    if(strcmp(topology, "-") == 0)
	run_cpp	= true;

//  ELSE, PARSING TOPOLOGY FROM A NAMED FILE
    else {
	if((input.fp = fopen(topology, "r")) == NULL) {
	    F(stderr, "%s : cannot open %s\n", argv0, topology);
	    CLEANUP(1);
	}

//  SAVE AN UNNECESSARY FORK/EXEC?
	while(run_cpp == false && fgets(chararray, BUFSIZ, input.fp)) {
	    s	= chararray;
	    while(*s == ' ' || *s == '\t')
		++s;
	    if(*s == '#') {		// a preprocessor control line
		run_cpp	= true;
		fclose(input.fp);
	    }
	}
    }

//  DO WE NEED TO RUN THE C-PREPROCSSOR?
    if(run_cpp)
	input.fp = preprocess_topology(defines, topology);
    else
	rewind(input.fp);
    input.name	= topology;

    init_lexical();
    gettoken();
    while(token != T_EOF) {
	switch (token) {

//  GLOBAL SPECIFICATION A NEW ACCESSPOINTTYPE
	case T_ACCESSPOINTTYPE :
#if	CNET_PROVIDES_WLANS
	    gettoken();
	    if(token == T_NAME) {
		nodeattr	= add_nodetype(NT_ACCESSPOINT, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new accesspoint type expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_node_attr(token))
		    node_attr(token, nodeattr);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
#else
	    SORRY_NO_WLANS;
#endif
	    break;

//  GLOBAL SPECIFICATION A NEW HOSTTYPE
	case T_HOSTTYPE :
	    gettoken();
	    if(token == T_NAME) {
		nodeattr	= add_nodetype(NT_HOST, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new hosttype expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_node_attr(token))
		    node_attr(token, nodeattr);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
	    break;

//  GLOBAL SPECIFICATION A NEW MOBILETYPE
	case T_MOBILETYPE :
#if	CNET_PROVIDES_WLANS
	    gettoken();
	    if(token == T_NAME) {
		nodeattr	= add_nodetype(NT_MOBILE, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new mobiletype expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_node_attr(token))
		    node_attr(token, nodeattr);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
#else
	    SORRY_NO_WLANS;
#endif
	    break;

//  GLOBAL SPECIFICATION A NEW ROUTERTYPE
	case T_ROUTERTYPE :
	    gettoken();
	    if(token == T_NAME) {
		nodeattr	= add_nodetype(NT_ROUTER, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new routertype expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_node_attr(token))
		    node_attr(token, nodeattr);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
	    break;

//  GLOBAL SPECIFICATION OF A NEW WANTYPE
	case T_WANTYPE :
#if	CNET_PROVIDES_WANS
	    gettoken();
	    if(token == T_NAME) {
		nic = add_linktype(LT_WAN, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new WAN type expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_nic_attr(token))
		    link_attr(token, nic, NULL, NULL);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
#else
	    SORRY_NO_WANS;
#endif
	    break;

//  GLOBAL SPECIFICATION OF A NEW LANTYPE
	case T_LANTYPE :
#if	CNET_PROVIDES_LANS
	    gettoken();
	    if(token == T_NAME) {
		nic = add_linktype(LT_LAN, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new LAN type expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_nic_attr(token))
		    link_attr(token, NULL, nic, NULL);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
#else
	    SORRY_NO_LANS;
#endif
	    break;

//  GLOBAL SPECIFICATION OF A NEW WLANTYPE
	case T_WLANTYPE :
#if	CNET_PROVIDES_WLANS
	    gettoken();
	    if(token == T_NAME) {
		nic = add_linktype(LT_WLAN, chararray);
		gettoken();
	    }
	    else
		SYNTAX("name of new WLAN type expected\n");

	    expect(T_LCURLY,"'{'");
	    for(;;) {
		if(is_nic_attr(token))
		    link_attr(token, NULL, NULL, nic);
		else if(token == T_COMMA)
		    gettoken();
		else
		    break;
	    }
	    expect(T_RCURLY,"'}'");
#else
	    SORRY_NO_WLANS;
#endif
	    break;

//  GLOBAL SPECIFICATION ON A LAN SEGMENT
	case T_LANSEGMENT :
#if	CNET_PROVIDES_LANS
	    gettoken();
	    lansegment_defn();
#else
	    SORRY_NO_LANS;
#endif
	    break;

// INTRODUCING 1 OF THE 4 NODETYPES
	case T_ACCESSPOINT :
#if	CNET_PROVIDES_WLANS
	    gettoken();
	    node_defn(NT_ACCESSPOINT, NULL);
#else
	    SORRY_NO_WLANS;
#endif
	    break;
	case T_HOST :
	    gettoken();
	    node_defn(NT_HOST, NULL);
	    break;
	case T_MOBILE : {
#if	CNET_PROVIDES_WLANS
	    int	orig;

	    gettoken();
	    orig = node_defn(NT_MOBILE, NULL);
	    if(token == T_NAME && chararray[0] == 'x' && isdigit(chararray[1])) {
		int	ntimes	= atoi(&chararray[1]);

		if(ntimes > 0) {
		    extern void clone_mobile_node(int orig, int ntimes);

		    clone_mobile_node(orig, ntimes);
		    gettoken();
		}
		else
		    SYNTAX("invalid mobile node multiplier\n"); 
	    }
#else
	    SORRY_NO_WLANS;
#endif
	    break;
	}
	case T_ROUTER :
	    gettoken();
	    node_defn(NT_ROUTER, NULL);
	    break;

//  SUNDRY GLOBAL ATTRIBUTES
	case T_MAPWIDTH :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_distance(&gattr.maxposition.x);
	    break;
	case T_MAPHEIGHT :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_distance(&gattr.maxposition.y);
	    break;
	case T_DRAWNODES :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_Boolean(&gattr.drawnodes);
	    break;
	case T_DRAWLINKS :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_Boolean(&gattr.drawlinks);
	    break;
	case T_MAPGRID :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_distance(&gattr.mapgrid);
	    break;
	case T_MAPHEX :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_distance(&gattr.maphex);
	    break;
	case T_MAPCOLOUR :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.mapcolour);
	    break;
	case T_MAPIMAGE :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.mapimage);
	    break;
	case T_MAPMARGIN :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_distance(&gattr.mapmargin);
	    break;
	case T_MAPSCALE :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_double(&gattr.mapscale);
	    if(gattr.mapscale <= 0.0)
		SYNTAX("mapscale must be greater than zero\n"); 
	    break;
	case T_MAPTILE :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.maptile);
	    break;
#if	CNET_PROVIDES_MOBILITY
	case T_POSITIONERROR :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_integer(&gattr.positionerror);
	    if(token == T_METRES)	//  ASSUMED UNITS OF DISTANCE ARE m
		gettoken();
	    break;
#endif
	case T_SHOWCOSTPERFRAME :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_Boolean(&gattr.showcostperframe);
	    break;
	case T_SHOWCOSTPERBYTE :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_Boolean(&gattr.showcostperbyte);
	    break;
	case T_TRACEFILE :
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.trace_filenm);
	    if(gattr.trace_filenm) {
		gattr.trace_filenm = strdup(gattr.trace_filenm);
		CHECKALLOC(gattr.trace_filenm);
	    }
	    break;

	default :
//  GLOBAL, DEFAULT, NODE OR LINK ATTRIBUTES
	    if(is_node_attr(token) || is_nic_attr(token))    // topology-wide
		node_attrs(&DEF_NODEATTR,
#if	CNET_PROVIDES_WANS
			&DEFAULTWAN,
#else
			NULL,
#endif
#if	CNET_PROVIDES_LANS
			&DEFAULTLAN,
#else
			NULL,
#endif
#if	CNET_PROVIDES_WLANS
			&DEFAULTWLAN
#else
			NULL
#endif
		);

//  IS THIS THE NAME OF AN EXISTING NODETYPE?
	    else if(token == T_NAME) {
		extern NODEATTR	*find_nodetype(char *, CnetNodeType *);

		CnetNodeType	typetype;
		NODEATTR	*nattr;

		nattr	= find_nodetype(chararray, &typetype);
		if(nattr == NULL)
		    goto err;
		gettoken();
		node_defn(typetype, nattr);
	    }
	    else
		goto err;
	    break;
	}			// end of switch(token)
	skip_commas();
    }			// while(!feof(input.fp))
    fclose(input.fp);
    return;

err:
    SYNTAX("unknown symbol or premature end of file\n");
    fclose(input.fp);
}
