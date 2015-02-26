#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_LANS

#include "physicallayer.h"

NICATTR		DEFAULTLAN;
LAN		*LANS		= NULL;

// ---------------------------------------------------------------------

NICATTR *extend_lan(int whichlan, int whichnode) // called from parser.c
{
    NODE	*np		= &NODES[whichnode];
    LAN		*lan		= &LANS[whichlan];
    NICATTR	*nic;
    EACHNIC	*each;
    char	buf[16];

//  ADD A NEW NIC TO whichnode
    np->nics	= realloc(np->nics,(np->nnics+2)*sizeof(NICATTR));
    CHECKALLOC(np->nics);
    nic		= &np->nics[np->nnics+1];

    memcpy(nic, &lan->segmentnic, sizeof(NICATTR)); // default NIC attributes
    memcpy(nic->nicaddr, NICADDR_ZERO,  LEN_NICADDR);
    sprintf(buf, "lan%i", np->nlans);
    nic->name	= strdup(buf);
    CHECKALLOC(nic->name);

    np->lans		= realloc(np->lans,(np->nnics+2)*sizeof(int));
    CHECKALLOC(np->lans);
    np->lans[np->nnics+1] = whichlan;

//  ADD THIS NEW NIC TO whichlan
    lan->nics		= realloc(lan->nics, (lan->nnics+1)*sizeof(EACHNIC));
    CHECKALLOC(lan->nics);
    each		= &lan->nics[lan->nnics];
    memset(each, 0, sizeof(EACHNIC));
    each->node		= whichnode;
    each->nodeslink	= np->nnics+1;

    ++np->nlans;
    ++np->nnics;
    ++lan->nnics;

    return nic;
}

int add_lan(char *name)			// called from parser.c
{
    LAN		*lan;
    int		l;

    for(l=0, lan=LANS ; l<gattr.nlans ; ++l, ++lan)
	if(strcasecmp(lan->name, name) == 0)
	    return l;
    LANS		= realloc(LANS, (gattr.nlans+1)*sizeof(LAN));
    CHECKALLOC(LANS);

    lan			= &LANS[gattr.nlans];
    memset(lan, 0, sizeof(LAN));
    lan->name		= strdup(name);
    CHECKALLOC(lan->name);
    lan->x		= UNKNOWN;
    lan->y		= UNKNOWN;

    lan->segmentnic	= DEFAULTLAN;
    lan->nics		= calloc(4, sizeof(EACHNIC));
    CHECKALLOC(lan->nics);
    return gattr.nlans++;
}

// ---------------------------------------------------------------------

static int generate_lan_collision(NODE *np, int thislan, NICATTR *segment)
{
    extern	void	unschedule_lan_collision(int);

    LAN		*lan	= &LANS[thislan];
    EACHNIC	*each;

    int		nhear	= 0;
    int		n;

    if(vflag)
	REPORT("node %s introduces collision on LAN %s\n",
		np->nodename, LANS[thislan].name);

//  REMOVE ALL FRAMES AND COLLISIONS SCHEDULED ON THIS LAN
    unschedule_lan_collision(thislan);

//  RECORD THAT THIS SEGMENT IS NOW BUSY DUE TO JAMMING
    segment->tx_until	= MICROSECONDS + segment->slottime +
	  ((segment->jitter == 0) ? 0 : poisson_usecs(segment->jitter,np->mt));

//  DELIVER A COLLISION FRAME TO ALL NODES ON THIS LAN (INC. OURSELVES)
    for(n=0, each=lan->nics ; n<lan->nnics ; ++n, ++each) {
	NODE		*destnp = &NODES[each->node];
	NICATTR		*destnic;

	if(destnp->runstate == STATE_RUNNING) {
	    destnic	= &destnp->nics[each->nodeslink];

//  EACH DESTINATION NIC CANNOT TRANSMIT WHILE COLLISION IS BEING RECEIVED
	    destnic->tx_until	= segment->tx_until;

//  PROPAGATION DELAY COMES FROM DISTANCE OF (SRC -> DEST) AT 200m/usec     
	    destnic->rx_until	= segment->tx_until +
		      abs(np->nattr.position.x-destnp->nattr.position.x)/200;

//  PREPARE A FRAMEARRIVAL STRUCTURE FOR THE DESTINATION
	    FRAMEARRIVAL	*newf = NEW(FRAMEARRIVAL);

	    CHECKALLOC(newf);
	    newf->destnode	= each->node;
	    newf->destlink	= each->nodeslink;
	    newf->linktype	= LT_LAN;
	    newf->linkindex	= thislan;
	    newf->arrives	= destnic->rx_until + LAN_DECODE_TIME;

	    newevent(EV_FRAMECOLLISION, newf->destnode,
			newf->arrives-MICROSECONDS, NULLTIMER, (CnetData)newf);
	    ++nhear;
	}
    }
    return nhear;
}

int write_lan(int thislink, char *packet, size_t len, bool unreliable)
{
    int		thislan	= NP->lans[thislink];

    LAN		*lan	= &LANS[thislan];
    NICATTR	*segment= &lan->segmentnic;
    NICATTR	*nic	= &NP->nics[thislink];

    EACHNIC	*each;

    CnetTime	Twrite;
    int		n, tcost;

//  ENSURE THAT PACKET IS NEITHER TOO SHORT, NOR TOO BIG 
    if(len < LAN_MINDATA || len > LAN_MAXDATA) {
	ERROR(ER_BADSIZE);
	return (-1);
    }

//  IF WE TRANSMIT WHILE THE SEGMENT OR NIC IS IN USE, WE HAVE A COLLISION
    if(MICROSECONDS <= segment->tx_until || MICROSECONDS <= nic->rx_until) {
	if(generate_lan_collision(NP, thislan, segment) > 1) {
	    segment->linkstats.rx_frames_collisions++;
	    gattr.linkstats.rx_frames_collisions++;
	}
	goto write_done;
    }

//  POSSIBLY LOSE THE FRAME (not really transmitted)
    if(unreliable && lose_frame(nic, len))
	goto write_done;

//  NIC BUSY UNTIL = NOW + (LENGTH_IN_BITS / BANDWIDTH_BPS) 
    Twrite	= LAN_ENCODE_TIME +
		  ((int64_t)len*(int64_t)8000000) / segment->bandwidth;

//  WE CANNOT COMMENCE WRITING IF OUR NIC IS CURRENTLY TRANSMITTING
    if(MICROSECONDS <= nic->tx_until)
	nic->tx_until	= nic->tx_until + Twrite;
    else
	nic->tx_until	= MICROSECONDS + Twrite;
    segment->tx_until	= nic->tx_until +
	  ((segment->jitter == 0) ? 0 : poisson_usecs(segment->jitter,NP->mt));
    nic->rx_until	= nic->tx_until;

//  SCHEDULE A NEW FRAME TO ARRIVE AT EACH OTHER NIC ON THIS LAN SEGMENT
    for(n=0, each=lan->nics ; n<lan->nnics ; ++n, ++each) {
	if(each->node == THISNODE)
	    continue;

	NODE	*destnp = &NODES[each->node];

	if(destnp->runstate == STATE_RUNNING) {
	    NICATTR	*destnic = &destnp->nics[each->nodeslink];

//  PROPAGATION DELAY COMES FROM DISTANCE OF (SRC -> DEST) AT 200m/usec     
	    destnic->rx_until	= nic->tx_until +
		    abs(NP->nattr.position.x - destnp->nattr.position.x)/200;

//  PREPARE A FRAMEARRIVAL STRUCTURE FOR THE DESTINATION
	    FRAMEARRIVAL *newf	= NEW(FRAMEARRIVAL);

	    CHECKALLOC(newf);
	    newf->destnode	= each->node;
	    newf->destlink	= each->nodeslink;
	    newf->linktype	= LT_LAN;
	    newf->linkindex	= thislan;
	    newf->len		= len;
	    newf->frame		= calloc(1, (unsigned)len);
	    CHECKALLOC(newf->frame);
	    memcpy(newf->frame, packet, (unsigned)len);

//  POSSIBLY CORRUPT THE FRAME (still transmitted)
	    newf->corrupted	=
		    (unreliable && corrupt_frame(nic, newf->frame, newf->len));
	    destnp->nframes++;

	    newf->arrives	= destnic->rx_until + LAN_DECODE_TIME;
	    newevent(EV_PHYSICALREADY, each->node,
			newf->arrives-MICROSECONDS, NULLTIMER, (CnetData)newf);
	}
    }

write_done:
    tcost = WHICH(nic->costperframe, DEFAULTLAN.costperframe) +
		  (len * WHICH(nic->costperbyte, DEFAULTLAN.costperbyte));

//  UPDATE GLOBAL LINK TRANSMISSION STATISTICS
    gattr.linkstats.tx_frames++;
    gattr.linkstats.tx_bytes	+= len;
    gattr.linkstats.tx_cost	+= tcost;

//  UPDATE THIS TRANSMITTING SEGMENT's STATISTICS
    segment->linkstats.tx_frames++;
    segment->linkstats.tx_bytes	+= len;
    segment->linkstats.tx_cost	+= tcost;

//  UPDATE THIS TRANSMITTING NIC's STATISTICS
    nic->linkstats.tx_frames++;
    nic->linkstats.tx_bytes	+= len;
    nic->linkstats.tx_cost	+= tcost;
#if	defined(USE_TCLTK)
    segment->stats_changed	= true;
    nic->stats_changed		= true;
#endif

    return 0;
}

// ---------------------------------------------------------------------

void check_lans(bool **adj)
{
    LAN		*lan;
    EACHNIC	*each1, *each2;
    NICATTR	*nic1, *nic2;
    int		l, n1, n2;
    int		maxlen=0, x;

//  ENSURE THAT EACH LAN NIC HAS AN ADDRESS
    for(l=0, lan=LANS ; l<gattr.nlans ; ++l, ++lan)
	for(n1=0, each1=lan->nics ; n1<lan->nnics ; ++n1, ++each1)
	    assign_nicaddr(each1->node, each1->nodeslink);

//  WARN IF ANY LAN ADDRESSES ON A SINGLE SEGMENT ARE NOT UNIQUE
    for(l=0, lan=LANS ; l<gattr.nlans ; ++l, ++lan)
	for(n1=0, each1=lan->nics ; n1<(lan->nnics-1) ; ++n1, ++each1) {
	    nic1	= &NODES[each1->node].nics[each1->nodeslink];
	    for(n2=n1+1, each2=&lan->nics[n2] ; n2<lan->nnics ; ++n2, ++each2) {
		nic2	= &NODES[each2->node].nics[each2->nodeslink];
		if(memcmp(nic1->nicaddr, nic2->nicaddr, LEN_NICADDR) == 0) {
		    char	buf[32];

		    CNET_format_nicaddr(buf, nic1->nicaddr);
		    WARNING(
	  "LAN segment %s has duplicate NIC addresses\n\t%s(%s) and %s(%s)\n",
				    lan->name,
				    NODES[each1->node].nodename, buf,
				    NODES[each2->node].nodename, buf);
		}
//  ESTABLISH ENTRIES IN THE WHOLE NETWORK'S ADJACENCY MATRIX
		adj[each1->node][each2->node] =
		adj[each2->node][each1->node] = true;
	    }
	}

//  GIVE EACH SEGMENT MEANINGFUL COORDINATES IF NOT ALREADY ASSIGNED
    for(l=0, lan=LANS ; l<gattr.nlans ; ++l, ++lan) {

	if(lan->x == UNKNOWN || lan->y == UNKNOWN) {

//  IF ANY NIC ALREADY HAS COORDINATES, POSITION THIS SEGMENT WRT THAT NIC
	    for(n1=0, each1=lan->nics ; n1<lan->nnics ; ++n1, ++each1)
		if(NODES[each1->node].nattr.position.x != UNKNOWN	&&
		   NODES[each1->node].nattr.position.y != UNKNOWN) {

		    lan->x	= NODES[each1->node].nattr.position.x -
			(n1+1)*DEFAULT_NODE_SPACING + DEFAULT_NODE_SPACING/2;
		    lan->y	= NODES[each1->node].nattr.position.y +
			(((n1%2) == 0) ? 1 : -1) * DEFAULT_NODE_SPACING;
		    break;
		}

//  IF NO COORDINATES YET, POSITION SEGMENT RELATIVE TO OTHER SEGMENTS
	    if(lan->x == UNKNOWN || lan->y == UNKNOWN) {
		lan->x	= (l/2+1)*DEFAULT_NODE_SPACING + (l/2)*maxlen;
		lan->y	= 2*DEFAULT_NODE_SPACING + (l%2)*4*DEFAULT_NODE_SPACING;
	    }
	}
	lan->x1 = lan->x + lan->nnics*DEFAULT_NODE_SPACING;
	if(maxlen < (lan->x1-lan->x))
	    maxlen	= (lan->x1-lan->x);
    }

//  FINALLY, POSITION ALL OF THIS SEGMENT'S NICs THAT DO NOT HAVE COORDINATES
    for(l=0, lan=LANS ; l<gattr.nlans ; ++l, ++lan) {
	x	= lan->x + DEFAULT_NODE_SPACING/2;

	for(n1=0, each1=lan->nics ; n1<lan->nnics ; ++n1, ++each1) {
	    if(NODES[each1->node].nattr.position.x == UNKNOWN)
		NODES[each1->node].nattr.position.x	= x;
	    x	+= DEFAULT_NODE_SPACING;
	    if(NODES[each1->node].nattr.position.y == UNKNOWN)
		NODES[each1->node].nattr.position.y =
		(lan->y-DEFAULT_NODE_SPACING) + (n1%2)*2*DEFAULT_NODE_SPACING;
	}
    }
}

// ---------------------------------------------------------------------

void print_lans(FILE *fp)
{
    if(gattr.nlans > 0) {
	extern void print_nic1(FILE *, const char *, int,
			   NICATTR *, NICATTR *, const char *);

	LAN	*lan;
	int	n;

	F(fp, "/* LAN definitions */\n");
	for(n=0, lan=LANS ; n<gattr.nlans ; ++n, ++lan) {
	    F(fp, "lan %s {\t\t/* has %i NICS */\n",lan->name,lan->nnics);
	    F(fp, "    x=%i, y=%i\n", lan->x, lan->y);
	    print_nic1(fp, "lan", false, &lan->segmentnic, &DEFAULTLAN, "    ");
	    F(fp, "}\n");
	}
	F(fp, "\n");
    }
}

// ---------------------------------------------------------------------

#if	defined(USE_TCLTK)

static TCLTK_COMMAND(lan_click)
{
    LAN		*lan;
    char	desc[BUFSIZ];
    bool	was;

    int	lann	= atoi(argv[1]);	// segment number
    int	nicn	= atoi(argv[2]);	// nth NIC on this segment
    int	button	= atoi(argv[3]);
    int	evx	= atoi(argv[4]);
    int	evy	= atoi(argv[5]);

    lan		= &LANS[lann];
    was		= CNET_set_commas(true);
    sprintf(desc, "bandwidth %sbps, slottime %susecs",
		    CNET_format64(lan->segmentnic.bandwidth),
		    CNET_format64(lan->segmentnic.slottime));
    CNET_set_commas(was);

    if(nicn == UNKNOWN) {
	if(button == LEFT_BUTTON)
	    TCLTK("toggleNIC LAN %i %i \"LAN %s\" %i %i \"%s\"",
			lann, UNKNOWN, lan->name, evx, evy, desc);
    }
    else
	TCLTK("toggleNIC LAN %i %i \"%s->LAN %s\" %i %i \"%s\"",
		lann, nicn, NODES[lan->nics[nicn].node].nodename, lan->name,
		evx, evy, desc);

    return TCL_OK;
}

static void draw_1_lan(int n)
{
    LAN		*lan	= &LANS[n];
    EACHNIC	*nic;
    int		i, x, y;

//  DRAW THE MAIN LAN SEGMENT
    TCLTK("set x [$map create line %i %i %i %i -fill %s -width %i]",
		    M2PX(lan->x),  M2PX(lan->y),
		    M2PX(lan->x1), M2PX(lan->y),
		    COLOUR_LAN, CANVAS_FATLINK/2 );
    TCLTK("$map bind $x <Button> \"lan_click %i -1 %%b %%x %%y\"", n);
#if	defined(USE_MACOSX)
    TCLTK("$map bind $x <Control-ButtonPress-1> \"lan_click %i -1 2 %%x %%y\"", n);
#endif
//  DRAW THE TWO TERMINATORS ON THE SEGMENT
    TCLTK("$map create line %i %i %i %i -fill %s -width %i",
		    M2PX(lan->x), M2PX(lan->y)-CANVAS_FATLINK/2,
		    M2PX(lan->x), M2PX(lan->y)+CANVAS_FATLINK/2,
		    COLOUR_LAN, CANVAS_FATLINK/2 );
    TCLTK("$map create line %i %i %i %i -fill %s -width %i",
		    M2PX(lan->x1), M2PX(lan->y)-CANVAS_FATLINK/2,
		    M2PX(lan->x1), M2PX(lan->y)+CANVAS_FATLINK/2,
		    COLOUR_LAN, CANVAS_FATLINK/2 );
//  AND THE NAME OF THE SEGMENT
    TCLTK("$map create text %i %i -anchor w -font myfont -text %s",
		    M2PX(lan->x)+4, M2PX(lan->y)+8, lan->name);

//  DRAW EACH NODE'S CONNECTION TO THE SEGMENT
    x	= lan->x + DEFAULT_NODE_SPACING/2;
    for(i=0, nic=lan->nics ; i<lan->nnics ; ++i, ++nic) {
	y	= NODES[nic->node].nattr.position.y;

	if(x == NODES[nic->node].nattr.position.x) {
	    TCLTK("set x [$map create line %i %i %i %i -fill %s -width %i]",
		    M2PX(NODES[nic->node].nattr.position.x), M2PX(y),
		    M2PX(x), M2PX(lan->y),
		    COLOUR_LAN, CANVAS_FATLINK/2 );
	    TCLTK("$map bind $x <Button> \"lan_click %i %i %%b %%x %%y\"",n,i);
#if	defined(USE_MACOSX)
	    TCLTK("$map bind $x <Control-ButtonPress-1> \"lan_click %i %i 2 %%x %%y\"",n,i);
#endif
	}
	else {
	    int	midy = lan->y + (lan->y < y ? 1 : -1)*DEFAULT_NODE_SPACING/4;

		TCLTK("set x [$map create line %i %i %i %i -fill %s -width %i]",
			M2PX(x), M2PX(lan->y),
			M2PX(x), M2PX(midy),
			COLOUR_LAN, CANVAS_FATLINK/2 );
	    TCLTK("$map bind $x <Button> \"lan_click %i %i %%b %%x %%y\"",n,i);
#if	defined(USE_MACOSX)
	    TCLTK("$map bind $x <Control-ButtonPress-1> \"lan_click %i %i 2 %%x %%y\"",n,i);
#endif

	    TCLTK("set x [$map create line %i %i %i %i -fill %s -width %i]",
			M2PX(x), M2PX(midy),
			M2PX(NODES[nic->node].nattr.position.x), M2PX(y),
			COLOUR_LAN, CANVAS_FATLINK/2 );
	    TCLTK("$map bind $x <Button> \"lan_click %i %i %%b %%x %%y\"",n,i);
#if	defined(USE_MACOSX)
	    TCLTK("$map bind $x <Control-ButtonPress-1> \"lan_click %i %i 2 %%x %%y\"",n,i);
#endif
	}

//  AND THE LINK-NUMBER OF THIS CONNECTION TO THE SEGMENT
	TCLTK("$map create text %i %i -anchor c -font myfont -text %i",
		    M2PX(x)+8, M2PX(lan->y)-8 + (y<lan->y ? 0 : 16),
		    nic->nodeslink);
	x	+= DEFAULT_NODE_SPACING;
    }

    if(gattr.showcostperframe || gattr.showcostperbyte) {
#define	H2	6
#define	W2	3

	int	midx	= lan->x + (lan->nnics * DEFAULT_NODE_SPACING)/2;
	int	width	= W2 + 2;
	TCLTK("$map create rectangle %i %i %i %i -fill %s -outline %s",
			M2PX(midx) - width, M2PX(lan->y) - H2,
			M2PX(midx) + width, M2PX(lan->y) + H2,
			"yellow", "black" );
	TCLTK("$map create text %i %i -anchor c -font 5x7 -text \"%s\"",
			M2PX(midx), M2PX(lan->y), "0");
#undef	W2
#undef	H2
    }
}

void draw_lans(void)
{
    for(int n=0 ; n<gattr.nlans ; ++n)
	draw_1_lan(n);
}
#endif

// ---------------------------------------------------------------------

void init_defaultlan(bool Bflag)
{
//  INITIALIZE DEFAULT LAN ATTRIBUTES
    memset(&DEFAULTLAN,  0, sizeof(DEFAULTLAN));
    DEFAULTLAN.linktype			= LT_LAN;
    DEFAULTLAN.up			= true;
    DEFAULTLAN.bandwidth		= LAN_BANDWIDTH;
    DEFAULTLAN.buffered			= Bflag;
    DEFAULTLAN.mtu			= LAN_MTU;
    DEFAULTLAN.propagation		= (CnetTime)LAN_PROPAGATION;
    DEFAULTLAN.slottime			= LAN_SLOTTIME;
}

void init_lans(void)
{
#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK_createcommand("lan_click", lan_click);
#endif
}

#endif
