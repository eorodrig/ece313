#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

// ------------------ NO CODE IN HERE FOR USE_ASCII ---------------------

#if	defined(USE_TCLTK)

#define	PFC		0
#define	PFL		1
#define	CPB		2
#define	CPF		3

#define	MAX_PFC		11
#define	MAX_PFL		11
#define	MAX_CPB		11
#define	MAX_CPF		11

static struct {
    const char	*label;
    const char	*keyword;
    int		maxvalue;
} sliders[] = {
    {	"probability of frame corruption",	"probframecorrupt", MAX_PFC },
    {	"probability of frame loss",		"probframeloss",    MAX_PFL },
    {	"cost per byte",			"costperbyte",      MAX_CPB },
    {	"cost per frame",			"costperframe",     MAX_CPF }
};

#define	N_NIC_SLIDERS	(sizeof(sliders) / sizeof(sliders[0]))

typedef	struct {
    const char		*nictype;
    NICATTR		*thisnic;
    NICATTR		*defnic;
    const char		*fromstr;
    const char		*tostr;
    int			from;
    int			to;
    int			which;
    int			value;
    char		*title;
} NICARG;

/*  Possibilities:

	WAN	-1	-1	default WAN
	WAN	from	to	WAN link
	LAN	lan-n	-1	LAN lan-n
	LAN	lan-n	nicn	nicn on LAN lan-n
 */

static NICARG *new_nicarg(char *nictype, char *from, char *to)
{
    char	title[128];

    NICARG	*nicarg	= NEW(NICARG);
    CHECKALLOC(nicarg);

    nicarg->nictype	= nictype;
    nicarg->fromstr	= from;
    nicarg->tostr	= to;

    nicarg->from	= atoi(from);
    nicarg->to		= atoi(to);

#if	CNET_PROVIDES_WANS
    if(strcmp(nictype, "WAN") == 0) {
	if(nicarg->to == DEFAULT) {
	    nicarg->thisnic	=
	    nicarg->defnic	= &DEFAULTWAN;
	    strcpy(title, "default WAN");
	}
	else {			// a specific WAN
	    NODE	*fromnp;
	    WAN		*wan;
	    int		w, min, max;

	    fromnp		= &NODES[nicarg->from];
	    nicarg->thisnic	= &DEFAULTWAN;

	    if(nicarg->from < nicarg->to)
		min = nicarg->from,	max = nicarg->to;
	    else
		min = nicarg->to,	max = nicarg->from;

	    for(w=0, wan=WANS ; w<gattr.nwans ; w++, wan++)
		if(wan->minnode == min && wan->maxnode == max) {

		    if(nicarg->from == wan->minnode) {
			nicarg->thisnic = &fromnp->nics[wan->minnode_nic];
			nicarg->defnic  = &fromnp->defaultwan;
		    }
		    else {
			nicarg->thisnic = &fromnp->nics[wan->maxnode_nic];
			nicarg->defnic  = &fromnp->defaultwan;
		    }
		    break;
		}
	    sprintf(title, "WAN %s->%s",
		    NODES[nicarg->from].nodename, NODES[nicarg->to].nodename);
	}
    }
#endif
#if	CNET_PROVIDES_LANS
    if(strcmp(nictype, "LAN") == 0) {
	if(nicarg->to == DEFAULT) {
	    if(nicarg->from == DEFAULT) {	// LAN -1 -1
		nicarg->thisnic	=
		nicarg->defnic	= &DEFAULTLAN;
		strcpy(title, "default LAN");
	    }
	    else {				// LAN lan-n -1
		nicarg->thisnic	= &LANS[nicarg->from].segmentnic;
		nicarg->defnic	= &DEFAULTLAN;
		sprintf(title, "LAN %s", LANS[nicarg->from].name);
	    }
	}
	else {					// LAN lan-n nic-n
	    EACHNIC *each	= &LANS[nicarg->from].nics[nicarg->to];

	    nicarg->thisnic	= &NODES[each->node].nics[each->nodeslink];
	    nicarg->defnic	= &DEFAULTLAN;
	    sprintf(title, "%s->LAN %s",
			NODES[each->node].nodename, LANS[nicarg->from].name);
	}
    }
#endif
#if	CNET_PROVIDES_WLANS
    if(strcmp(nictype, "WLAN") == 0) {
	if(nicarg->to == DEFAULT) {
	    nicarg->thisnic	=
	    nicarg->defnic	= &DEFAULTWLAN;
	    strcpy(title, "default WLAN");
	}
	else {
	    WLAN *wlan	= &WLANS[nicarg->to];
	    nicarg->thisnic	= &NODES[wlan->node].nics[wlan->node_link];
	    nicarg->defnic	= &DEFAULTWLAN;
	    sprintf(title, "WLAN node %i", nicarg->to);
	}
    }
#endif

    nicarg->title	= strdup(title);
    CHECKALLOC(nicarg->title);
    return nicarg;
}

static void free_nicarg(NICARG *na)
{
    FREE(na->title);
    FREE(na);
}

// ---------------------------------------------------------------------

#if	CNET_PROVIDES_WANS
static void set_nic_scale_label(NICARG *na)
{
    NICATTR	*nicattr;
    char	desc[32];
    int		value=0;

    switch (na->value) {
//  ONLY true NICs CAN SET VALUE TO -1, REQUESTING THEIR DEFAULT VALUES
    case -1 :	value	= -1;
		break;

//  A SCALE VALUE OF ZERO REQUESTS THE DEFAULT VALUE
    case 0 :	nicattr = (na->to == DEFAULT) ? na->defnic : na->thisnic;
		switch (na->which) {
		case PFC :	value	= DEFAULTWAN.probframecorrupt ?
		    DEFAULTWAN.probframecorrupt : nicattr->probframecorrupt;
				break;
		case PFL :	value	= DEFAULTWAN.probframeloss ?
		    DEFAULTWAN.probframeloss : nicattr->probframeloss;
				break;
		case CPB :	value	= DEFAULTWAN.costperbyte ?
		    DEFAULTWAN.costperbyte : nicattr->costperbyte;
				break;
		case CPF :	value	= DEFAULTWAN.costperframe ?
		    DEFAULTWAN.costperframe : nicattr->costperframe;
				break;
		}
		break;

//  ANY OTHER, POSITIVE, VALUE REQUESTS ONE OF THE DEFAULTS
    default :	value	= na->value-1;
		break;
    }

    if(value == -1)
	strcpy(desc, "(default value)");
    else if(value == 0)
	strcpy(desc, "(zero)");
    else if(na->which == PFL || na->which == PFC)
	sprintf(desc, "(1 chance in %i)", 1<<value);
    else
	sprintf(desc, "(%i)", value);

    TCLTK(".nic(%s,%i,%i).scales.scale%i config -label \"%s %s :\"",
		na->nictype, na->from, na->to,
		na->which, sliders[na->which].label, desc);
}

static TCLTK_COMMAND(set_nic_scale)
{
    NICATTR	*nicattr;
    NICARG	*nicarg;
    int		now=0;

    nicarg		= new_nicarg(argv[1], argv[2], argv[3]);
    nicarg->which	= atoi(argv[4]);
    nicarg->value	= atoi(argv[5]);

    if(nicarg->which < 0 || nicarg->which > N_NIC_SLIDERS) {
	free_nicarg(nicarg);
	Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid set_nic scale #", -1));
	return TCL_ERROR;
    }

    nicattr	= (nicarg->to == DEFAULT) ? nicarg->defnic : nicarg->thisnic;

    switch (nicarg->which) {
    case PFC:   if(nicarg->value == 0)
		    nicattr->probframecorrupt = DEFAULTWAN.probframecorrupt;
		else
		    nicattr->probframecorrupt = nicarg->value;
		now	= nicattr->probframecorrupt;
		break;

    case PFL:   if(nicarg->value == 0)
		    nicattr->probframeloss = DEFAULTWAN.probframeloss;
		else
		    nicattr->probframeloss = nicarg->value;
		now	= nicattr->probframeloss;
		break;

    case CPB:   if(nicarg->value == 0)
		    nicattr->costperbyte = DEFAULTWAN.costperbyte;
		else
		    nicattr->costperbyte = nicarg->value;
		now	= nicattr->costperbyte;
		break;

    case CPF:   if(nicarg->value == 0)
		    nicattr->costperframe = DEFAULTWAN.costperframe;
		else
		    nicattr->costperframe = nicarg->value;
		now	= nicattr->costperframe;
		break;
    }

    set_nic_scale_label(nicarg);
    if(dflag)
	REPORT("%s.%s = %i\n",nicarg->title,sliders[nicarg->which].keyword,now);

    free_nicarg(nicarg);
    return TCL_OK;
}
#endif

TCLTK_COMMAND(nic_created)
{
    static int	first_time	= true;

    NICARG	*nicarg;
    char	tmpbuf[32];

//  CALLED AS:	nic_created $nictype $from $to
    if(first_time) {
	first_time = false;

#if	CNET_PROVIDES_WANS
	TCLTK_createcommand("set_nic_scale", set_nic_scale);
#endif

	for(int s=0 ; s<N_NIC_SLIDERS; ++s) {
	    sprintf(chararray,"nic_scale_max(%i)", s);
	    sprintf(tmpbuf   ,"%i", sliders[s].maxvalue);
	    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);
	}
    }

    nicarg		= new_nicarg(argv[1], argv[2], argv[3]);

#if	CNET_PROVIDES_WANS
    sprintf(chararray,"nic_scale_value(%i)", PFC);
    sprintf(tmpbuf, "%i", (nicarg->to == DEFAULT) ? 0 :
			  (DEFAULTWAN.probframecorrupt ? 0:-1));
    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);

    sprintf(chararray,"nic_scale_value(%i)", PFL);
    sprintf(tmpbuf, "%i", (nicarg->to == DEFAULT) ? 0 :
			  (DEFAULTWAN.probframeloss ? 0 : -1));
    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);

    sprintf(chararray,"nic_scale_value(%i)", CPB);
    sprintf(tmpbuf, "%i", (nicarg->to == DEFAULT) ? 0 :
			  (DEFAULTWAN.costperbyte ? 0 : -1));
    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);

    sprintf(chararray,"nic_scale_value(%i)", CPF);
    sprintf(tmpbuf, "%i", (nicarg->to == DEFAULT) ? 0 :
			  (DEFAULTWAN.costperframe ? 0 : -1));
    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);

    sprintf(chararray, "nic_displayed(%s,%s,%s)", argv[1], argv[2], argv[3]);
    Tcl_LinkVar(tcl_interp, chararray,
    		(char *)&nicarg->thisnic->displayed, TCL_LINK_BOOLEAN);
#endif

    free_nicarg(nicarg);
    return TCL_OK;
}
#endif
