#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_WANS

#include "physicallayer.h"

NICATTR		DEFAULTWAN;
WAN		*WANS	= NULL;

// ---------------------------------------------------------------------

NICATTR *add_wan(int src, int dest)
{
    NODE	*np;
    NICATTR	*min_nicattr, *max_nicattr;
    WAN		*wan;
    int		minnode, maxnode, w;
    char	buf[16];

    if(src < dest)
	minnode = src,  maxnode = dest;
    else
	minnode = dest, maxnode = src;

//  CHECK IF WE'VE SEEN A WAN LINK BETWEEN THESE TWO NODES BEFORE
    for(w=0, wan=WANS; w<gattr.nwans ; w++, wan++)
	if(wan->minnode == minnode && wan->maxnode == maxnode) {
	    if(src == minnode)
		return(&NODES[src].nics[wan->minnode_nic]);
	    else
		return(&NODES[src].nics[wan->maxnode_nic]);
	}

//  BEGIN CREATING A NEW WAN
    WANS		= realloc(WANS, (unsigned)(gattr.nwans+1)*sizeof(WAN));
    CHECKALLOC(WANS);
    wan			= &WANS[gattr.nwans];
    memset(wan, 0, sizeof(WAN));

    wan->up		= true;
    wan->minnode	= minnode;
    wan->maxnode	= maxnode;

//  FILL IN WAN ATTRIBUTES ON MIN-NODE
    np			= &NODES[minnode];
    wan->minnode_nic	= np->nnics+1;
    np->nics		= realloc(np->nics,(np->nnics+2)*sizeof(NICATTR));
    CHECKALLOC(np->nics);
    min_nicattr		= &np->nics[np->nnics+1];

    memcpy(min_nicattr, &np->defaultwan, sizeof(NICATTR));
    memcpy(min_nicattr->nicaddr, NICADDR_ZERO,  LEN_NICADDR);

    sprintf(buf, "wan%i", np->nwans);
    min_nicattr->name	= strdup(buf);
    CHECKALLOC(min_nicattr->name);
    np->wans		= realloc(np->wans,(np->nnics+2)*sizeof(int));
    CHECKALLOC(np->wans);
    np->wans[np->nnics+1] = gattr.nwans;

    ++np->nnics;
    ++np->nwans;

//  FILL IN WAN ATTRIBUTES ON MAX-NODE
    np			= &NODES[maxnode];
    wan->maxnode_nic	= np->nnics+1;
    np->nics		= realloc(np->nics,(np->nnics+2)*sizeof(NICATTR));
    CHECKALLOC(np->nics);
    max_nicattr		= &np->nics[np->nnics+1];

    memcpy(max_nicattr, &np->defaultwan, sizeof(NICATTR));
    memcpy(max_nicattr->nicaddr, NICADDR_ZERO,  LEN_NICADDR);

    sprintf(buf, "wan%i", np->nwans);
    max_nicattr->name	= strdup(buf);
    CHECKALLOC(max_nicattr->name);
    np->wans		= realloc(np->wans,(np->nnics+2)*sizeof(int));
    CHECKALLOC(np->wans);
    np->wans[np->nnics+1] = gattr.nwans;

    ++np->nnics;
    ++np->nwans;
    ++gattr.nwans;

    return (src == minnode ? min_nicattr : max_nicattr);
}

void check_wans(bool **adj)
{
    WAN		*wan;
    int		n;

    for(n=0, wan=WANS ; n<gattr.nwans ; n++, wan++) {
	int	from	= wan->minnode;
	int	to	= wan->maxnode;

//  FILL IN THE ADJACENCY MATRIX FOR THESE NODES
	adj[from][to] = adj[to][from] = true;

//  ENSURE THAT EACH LAN NIC HAS AN ADDRESS
	assign_nicaddr(wan->minnode, wan->minnode_nic);
    }
}


// ---------------------------------------------------------------------

int write_wan(int thislink, char *frame, size_t len, bool unreliable)
{
    NICATTR		*nic	= &NP->nics[thislink];
    int			thiswan	= NP->wans[thislink];
    WAN			*wan	= &WANS[thiswan];

    FRAMEARRIVAL	*newf;

    CnetTime		Twrite;
    int			destnode, destlink, tx_tcost;

    bool		draw_lost	= false;
    bool		draw_corrupt	= false;

//  FIND TO WHICH NODE, and LINK (AND ITS ATTRIBUTES) WE ARE SENDING
    if(wan->minnode == THISNODE) {
	destnode	= wan->maxnode;
	destlink	= wan->maxnode_nic;
    }
    else {
	destnode	= wan->minnode;
	destlink	= wan->minnode_nic;
    }

//  ENSURE THAT THE DESTINATION IS ACTUALLY READING OFF FRAMES
    if(NODES[destnode].nframes > MAX_PENDING_FRAMES) {
	ERROR(ER_TOOBUSY);
	return(-1);
    }

#if	defined(USE_TCLTK)
    wan->ntxed	+= len;
#endif

//  PREPARE THE FRAME FOR ARRIVAL VIA A WAN LINK AT DESTINATION
    if(NODES[destnode].runstate != STATE_RUNNING)
	goto done;

//  POSSIBLY LOSE THE FRAME (not transmitted)
    if(unreliable && lose_frame(nic, len) == true) {
	draw_lost	= true;
	goto done;
    }

//  WRITING_TIME = NOW + (LENGTH_IN_BITS / BANDWIDTH_BPS)  + JITTER
    Twrite	= (len * (CnetTime)8000000) / nic->bandwidth +
		  ((nic->jitter == 0) ? 0 : poisson_usecs(nic->jitter,NP->mt));

//  WE CANNOT COMMENCE WRITING IF THIS NIC IS CURRENTLY TRANSMITTING
    if(MICROSECONDS <= nic->tx_until)
	nic->tx_until	= nic->tx_until + Twrite;
    else
	nic->tx_until	= MICROSECONDS + Twrite;

    newf		= NEW(FRAMEARRIVAL);
    CHECKALLOC(newf);
    newf->destnode	= destnode;
    newf->destlink	= destlink;
    newf->linktype	= LT_WAN;
    newf->linkindex	= NODES[destnode].wans[destlink];
    newf->arrives	= nic->tx_until + nic->propagation;
    newf->len		= len;
    newf->frame		= malloc(len);
    CHECKALLOC(newf->frame);
    memcpy(newf->frame, frame, len);
    NODES[destnode].nframes++;

//  POSSIBLY CORRUPT THE FRAME (still transmitted)
    draw_corrupt	=
    newf->corrupted	=
	    (unreliable && corrupt_frame(nic, newf->frame, newf->len));

    newevent(EV_PHYSICALREADY, destnode,
		    newf->arrives - MICROSECONDS, NULLTIMER, (CnetData)newf);


    tx_tcost	= WHICH(nic->costperframe, NP->defaultwan.costperframe) +
		  ((len) * WHICH(nic->costperbyte,NP->defaultwan.costperbyte));

//  UPDATE THIS TRANSMITTING NIC's STATISTICS
    nic->linkstats.tx_frames++;
    nic->linkstats.tx_bytes	+= len;
    nic->linkstats.tx_cost	+= tx_tcost;
#if	defined(USE_TCLTK)
    nic->stats_changed		= true;
#endif

//  UPDATE GLOBAL LINK TRANSMISSION STATISTICS
    gattr.linkstats.tx_frames++;
    gattr.linkstats.tx_bytes	+= len;
    gattr.linkstats.tx_cost	+= tx_tcost;

done:
#if	defined(USE_TCLTK)
    if(wan->drawframes_displayed && wan->ndrawframes < MAX_DRAWFRAMES) {
	extern	void new_drawframe(int, size_t, char *, bool, bool,
					CnetTime, CnetTime);

	new_drawframe(thiswan, len, frame, draw_lost, draw_corrupt,
			nic->tx_until - MICROSECONDS, nic->propagation);
    }
#endif
    return 0;
}


// ---------------------------------------------------------------------

#if	defined(USE_TCLTK)
void draw_wan(int n, bool clear_first, int width)
{
    WAN		*wan	= &WANS[n];
    NODE	*minp	= &NODES[wan->minnode];
    NODE	*maxp	= &NODES[wan->maxnode];

    TCLTK("global map");
    if(clear_first)
       TCLTK("$map delete wan%i", wan->minnode, wan->maxnode);

    TCLTK("set x [$map create line %i %i %i %i -fill %s -width %i -tags wan%i]",
		M2PX(minp->nattr.position.x),M2PX(minp->nattr.position.y),
		M2PX(maxp->nattr.position.x),M2PX(maxp->nattr.position.y),
		wan->up ? COLOUR_WAN : COLOUR_SEVERED,
		width, n);
    TCLTK("$map bind $x <Button> \"wan_click %i %%b %%x %%y\"", n);
#if	defined(USE_MACOSX)
    TCLTK("$map bind $x <Control-ButtonPress-1> \"wan_click %i 2 %%x %%y\"", n);
#endif

    if(gattr.showcostperframe || gattr.showcostperbyte) {
#define	H2	6
#define	W2	3

	char	buf[64];
	int	midx	= minp->nattr.position.x +
			(maxp->nattr.position.x - minp->nattr.position.x)/2;
	int	midy	= minp->nattr.position.y +
			(maxp->nattr.position.y - minp->nattr.position.y)/2;
	int	cmin, cmax;

	if(gattr.showcostperframe)
	    cmin = WHICH(minp->nics[wan->minnode_nic].costperframe,
			 minp->defaultwan.costperframe),
	    cmax = WHICH(maxp->nics[wan->maxnode_nic].costperframe,
			 maxp->defaultwan.costperframe);
	else
	    cmin = WHICH(minp->nics[wan->minnode_nic].costperbyte,
			 minp->defaultwan.costperbyte),
	    cmax = WHICH(maxp->nics[wan->maxnode_nic].costperbyte,
			 maxp->defaultwan.costperbyte);

	if(cmin == cmax)
	    sprintf(buf, "%i", cmin);
	else
	    sprintf(buf, "%i/%i", cmin, cmax);

	width	= strlen(buf)*W2 + 2;
	TCLTK("$map create rectangle %i %i %i %i -fill %s -outline %s",
		    M2PX(midx) - width, M2PX(midy) - H2,
		    M2PX(midx) + width, M2PX(midy) + H2,
		    "yellow", "black" );
    TCLTK("$map create text %i %i -anchor c -font myfont -text \"%s\"",
		    M2PX(midx), M2PX(midy), buf);
#undef	W2
#undef	H2
    }
}

void draw_wans(void)
{
    WAN		*wan;
    CnetTime	av_frames;
    int		n;

//  CALCULATE THE AVERAGE TRAFFIC PER WAN
    av_frames  = 0;
    for(n=0, wan=WANS ; n<gattr.nwans ; n++, wan++)
	if(wan->up)
	    av_frames	+= wan->ntxed;
    av_frames	/= gattr.nwans;

    for(n=0, wan=WANS ; n<gattr.nwans ; n++, wan++) {
	int	width = 1;

	if(wan->ntxed >= av_frames) {
	    if(wan->ntxed > (av_frames / 2))
		width = CANVAS_FATLINK;
	    else
		width = CANVAS_FATLINK/2;
	}
	draw_wan(n, false, width);
	wan->ntxed	= 0;
    }
}

static void toggle_wan(NICATTR *nic, int from, int to, int x, int y)
{
    extern char	*NICdesc(NICATTR *);

    TCLTK("toggleNIC WAN %i %i \"%s->%s\" %i %i \"%s\"",
		    from, to, NODES[from].nodename, NODES[to].nodename,
		    x, y, NICdesc(nic));
}

static TCLTK_COMMAND(wan_click)
{
    extern void	display_wanmenu(WAN *, int, int);

    WAN		*wan;
    int		dx, dy;

    int	n	= atoi(argv[1]);
    int	button	= atoi(argv[2]);
    int	evx	= atoi(argv[3]);
    int	evy	= atoi(argv[4]);

    wan		= &WANS[n];

    if(button == LEFT_BUTTON) {
	NODE	*minnp, *maxnp;
	int	distmin, distmax;

	minnp	= &NODES[wan->minnode];
	dx	= minnp->nattr.position.x - evx;
	dy	= minnp->nattr.position.y - evy;
	distmin	= dx*dx + dy*dy;

	maxnp	= &NODES[wan->maxnode];
	dx	= maxnp->nattr.position.x - evx;
	dy	= maxnp->nattr.position.y - evy;
	distmax	= dx*dx + dy*dy;

	if(distmin < distmax)
	    toggle_wan(&minnp->nics[wan->minnode_nic],
			wan->minnode, wan->maxnode, evx, evy);
	else
	    toggle_wan(&maxnp->nics[wan->maxnode_nic],
			wan->maxnode, wan->minnode, evx, evy);
    }
    else if(cnet_state == STATE_RUNNING || cnet_state == STATE_PAUSED)
	display_wanmenu(wan, evx, evy);

    return TCL_OK;
}
#endif

// ---------------------------------------------------------------------

void init_defaultwan(bool Bflag)
{
//  INITIALIZE DEFAULT WAN ATTRIBUTES
    memset(&DEFAULTWAN,  0, sizeof(DEFAULTWAN));
    DEFAULTWAN.linktype			= LT_WAN;
    DEFAULTWAN.up			= true;
    DEFAULTWAN.bandwidth		= WAN_BANDWIDTH;
    DEFAULTWAN.buffered			= Bflag;
    DEFAULTWAN.mtu			= MAX_MESSAGE_SIZE + 1024;
    DEFAULTWAN.propagation		= (CnetTime)WAN_PROPAGATION;

    DEFAULTWAN.costperbyte		= 0;
    DEFAULTWAN.costperframe		= 0;
    DEFAULTWAN.probframecorrupt		= 0;
    DEFAULTWAN.probframeloss		= 0;

    DEFAULTWAN.linkmtbf			= (CnetTime)0;
    DEFAULTWAN.linkmttr			= (CnetTime)0;
    DEFAULTWAN.tx_until			= (CnetTime)0;
    DEFAULTWAN.rx_until			= (CnetTime)0;
}

void init_wans(void)
{
#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK_createcommand("wan_click", wan_click);
#endif
}

#else

NICATTR *add_wan(int src, int dest)
{
    return NULL;
}

#endif
