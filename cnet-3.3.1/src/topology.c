#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	TOPOLOGY_SEED	6432867
static	MT		*mt;


// -------------- generate a random (aesthetic?) topology --------------

#define	MAXDEGREE		4

static bool connected(int nnodes, bool **adj)
{
    bool	**w;
    int		i, j, k;
    bool	rtn	= true;

    w	= malloc(nnodes * sizeof(bool *));
    CHECKALLOC(w);
    for(i=0 ; i<nnodes ; ++i) {
	w[i]	= malloc(nnodes * sizeof(bool));
	CHECKALLOC(w[i]);
	for(j=0 ; j<nnodes ; ++j)
	    w[i][j] = adj[i][j];
    }

    for(k=0 ; k<nnodes ; ++k)
	for(i=0 ; i<nnodes ; ++i)
	    for(j=0 ; j<nnodes ; ++j)
		if(!w[i][j])
		    w[i][j] = w[i][k] && w[k][j];

    for(i=0 ; i<nnodes ; ++i)
	for(j=0 ; j<nnodes ; ++j)
	    if(!w[i][j]) {
		rtn	= false;
		goto done;
	    }
done:
    for(i=0 ; i<nnodes ; ++i)
	FREE(w[i]);
    FREE(w);
    return(rtn);
}

#define	iabs(n)	((n)<0 ? -(n) : (n))

static void add_1_wan(int n, int **grid, int nnodes, bool **adj)
{
    extern NICATTR *add_wan(int src, int dest);

    int		from, to;
    int		x, y;
    int		dx, dy;

    for(;;) {
	x	= mt19937_int31(mt)%n;
	y	= mt19937_int31(mt)%n;
	if(grid[x][y] == UNKNOWN)
	    continue;
	from	= grid[x][y];
	if(NODES[from].nnics == MAXDEGREE)
	    continue;

	do {
	    dx	= (mt19937_int31(mt)%3)-1;		// dx:  -1, 0, +1
	    dy	= (mt19937_int31(mt)%3)-1;		// dy:  -1, 0, +1
#if	RANDOM_DIAGONALS
	} while(dx == 0 && dy == 0);
#else
	} while(iabs(dx) == iabs(dy));
#endif

	x	+= dx;
	y	+= dy;
	while(x>=0 && x<n && y>=0 && y<n) {
	    if(grid[x][y] != UNKNOWN) {
		to	= grid[x][y];
		if(NODES[to].nnics == MAXDEGREE || adj[from][to] == true)
		    break;
		add_wan(from, to);
		adj[from][to] = adj[to][from] = true;
		return;
	    }
	    x	+= dx;
	    y	+= dy;
	}
    }
}

void random_topology(const char *rflag, int Sflag)
{
    extern int add_node(CnetNodeType, char *, bool, NODEATTR *);

    int		i, j, n, nnodes, minlinks;
    int		x, y;
    int		**grid;
    bool	**adj;

    nnodes	= atoi(rflag);
    if(nnodes < 2)
	FATAL("invalid # of random nodes\n");
    mt		= mt19937_init((unsigned long)(TOPOLOGY_SEED+Sflag));

         if(nnodes <= 10 )	DEF_NODEATTR.icontitle = "host%i";
    else if(nnodes <= 100)	DEF_NODEATTR.icontitle = "host%02d";
    else			DEF_NODEATTR.icontitle = "host%03d";

    n		= (int)sqrt((double)(nnodes-1)) + 1;
    grid	= malloc(n * sizeof(int *));
    CHECKALLOC(grid);
    for(i=0 ; i<n ; ++i) {
	grid[i]	= malloc(n * sizeof(int));
	CHECKALLOC(grid[i]);
	for(j=0 ; j<n ; ++j)
	    grid[i][j] = UNKNOWN;
    }

    adj		= malloc(nnodes * sizeof(bool *));
    CHECKALLOC(adj);
    for(i=0 ; i<nnodes ; ++i) {
	adj[i]	= calloc((unsigned)nnodes, sizeof(bool));
	CHECKALLOC(adj[i]);
    }
    for(i=0 ; i<nnodes ; ) {
	x = mt19937_int31(mt)%n;
	y = mt19937_int31(mt)%n;
	if(grid[x][y] == UNKNOWN)
	    grid[x][y] = i++;
    }

    for(i=0, j=0 ; i<nnodes; j++) {
	x	= j%n;
	y	= j/n;
	if(grid[x][y] == UNKNOWN)
	    continue;
	sprintf(chararray, DEF_NODEATTR.icontitle, i);
	add_node(NT_HOST, chararray, true, NULL);
	NODES[i].nattr.position.x	= (1.5*x+1) * DEFAULT_NODE_SPACING;
	NODES[i].nattr.position.y	= (1.5*y+1) * DEFAULT_NODE_SPACING;
	NODES[i].nattr.position.z	= 0;
	grid[x][y]	= i++;
    }

    minlinks	= (nnodes<6) ? n/2 : (3*nnodes)/2;
    for(i=0 ; i<minlinks ; i++)
	add_1_wan(n, grid, nnodes, adj);
    i=0;
    while(connected(nnodes, adj) == false) {
	add_1_wan(n, grid, nnodes, adj);
	++i;
    }
    if(vflag)
	REPORT("%i extra link%s required for connectivity\n", i, PLURAL(i));

    for(i=0 ; i<n ; ++i)
	FREE(grid[i]);
    FREE(grid);

    for(i=0 ; i<nnodes ; ++i)
	FREE(adj[i]);
    FREE(adj);
}

// ---------------------------------------------------------------------

void init_reboot_argv(NODEATTR *na, int argc, char **argv)
{
    na->reboot_argv	= malloc((argc+1) * sizeof(char *));
    CHECKALLOC(na->reboot_argv);
    for(int n=0 ; n<argc ; ++n) {
	na->reboot_argv[n]	= strdup(argv[n]);
	CHECKALLOC(na->reboot_argv[n]);
    }
    na->reboot_argv[argc]	= NULL;
}

void check_application_bounds(int minmsg, int maxmsg)
{
    int		n;
    NODE	*np;

    FOREACH_NODE {
	NODEATTR	*na;
	int		my_minmsg, my_maxmsg;

	na	= &np->nattr;

//  CHECK THAT USER-REQUESTED MESSAGE SIZES ARE NEITHER TOO BIG NOR TOO SMALL
	my_minmsg	= WHICH(na->minmessagesize, DEF_NODEATTR.minmessagesize);
	my_maxmsg	= WHICH(na->maxmessagesize, DEF_NODEATTR.maxmessagesize);

	if(my_minmsg < minmsg) {
	   WARNING("%s.minmessagesize(=%i) < Application Layer requires(=%i)\n",
			    np->nodename, my_minmsg, minmsg);
	    ++nerrors;
	}
	if(my_maxmsg > maxmsg) {
	   WARNING("%s.maxmessagesize(=%i) > Application Layer requires(=%i)\n",
			    np->nodename, my_maxmsg, maxmsg);
	    ++nerrors;
	}
	if(my_minmsg > my_maxmsg) {
	    WARNING("%s.minmessagesize(=%i) > maxmessagesize(=%i)\n",
			    np->nodename, my_minmsg, my_maxmsg);
	    ++nerrors;
	}
    }
}

void check_topology(int cflag, int Sflag, int argc, char **argv)
{
    extern void find_mapsize(void);

    NODE	*np;
    NODEATTR	*na;
    CnetAddr	a;
    int		n, p;
    int		x, y, rootn;
    bool	**adj;

    FOREACH_NODE {
	switch (np->nodetype) {
	case NT_HOST :		++gattr.nhosts;		break;
	case NT_ROUTER :	++gattr.nrouters;	break;
	case NT_MOBILE :	++gattr.nmobiles;	break;
	case NT_ACCESSPOINT :	++gattr.naccesspoints;	break;
	default :		FATAL("unable to determine nodetype of '%s'\n",
					    np->nodename);
				break;
	}
    }
    if(dflag) {
	REPORT("%i host%s, %i accesspoint%s, %i mobile%s, %i router%s, %i wan%s, %i lan%s\n",
			gattr.nhosts,		PLURAL(gattr.nhosts),
			gattr.naccesspoints,	PLURAL(gattr.naccesspoints),
			gattr.nmobiles,		PLURAL(gattr.nmobiles),
			gattr.nrouters,		PLURAL(gattr.nrouters),
			gattr.nwans,		PLURAL(gattr.nwans),
			gattr.nlans,		PLURAL(gattr.nlans) );
    }

//  ENSURE THAT WE HAVE AT LEAST 2 HOSTS (APPLICATION LAYERS)
    if((gattr.nhosts + gattr.nmobiles) < 2) {
	WARNING("A network must have >= 2 nodes with Application Layers\n");
	++nerrors;
    }

    mt		= mt19937_init((unsigned long)(TOPOLOGY_SEED+Sflag));

    FOREACH_NODE {
	if(np->nnics == 0) {
	    WARNING("%s has no communications links\n", np->nodename);
	    ++nerrors;
	}

//  NEXT, ASSIGN A NODE ADDRESS IF NECESSARY, AND CHECK UNIQUENESS
	na	= &np->nattr;
	if(na->address == (CnetAddr)UNKNOWN)
	    na->address	= (CnetAddr)n;
	a	= na->address;
	for(p=n+1 ; p<_NNODES ; p++)
	    if(a == NODES[p].nattr.address) {
		WARNING("%s and %s have the same node address (%u)\n",
				np->nodename, NODES[p].nodename, a);
		++nerrors;
	    }

//  CHECK THAT ALL NT_ACCESSPOINTs HAVE >= 1 WIRELESS LINK
	if(np->nodetype == NT_ACCESSPOINT && np->nwlans == 0) {
	    WARNING("accesspoint %s does not have any wireless links\n",
			    np->nodename);
	    ++nerrors;
	}

//  ASSIGN A RANDOM CLOCK TIME
#if	(NODE_CLOCK_SKEW == 0)
	np->clock_skew	   = (CnetTime)0;
#else
	if(cflag)
	    np->clock_skew = (CnetTime)0;
	else
	    np->clock_skew = (mt19937_int31(mt)%(2*NODE_CLOCK_SKEW) -
				NODE_CLOCK_SKEW);
#endif
    }

//  ALLOCATE AND INITIALIZE THE ADJACENCY MATRIX 
    adj		= malloc(_NNODES * sizeof(bool *));
    CHECKALLOC(adj);
    for(int n=0 ; n<_NNODES ; ++n) {
	adj[n]	= calloc(_NNODES, sizeof(bool));
	CHECKALLOC(adj[n]);
    }

#if	CNET_PROVIDES_WANS
    extern void	check_wans(bool **adj);
    check_wans(adj);
#endif
#if	CNET_PROVIDES_LANS
    extern void	check_lans(bool **adj);
    check_lans(adj);
#endif
#if	CNET_PROVIDES_WLANS
    extern void	check_wlans(bool **adj);
    check_wlans(adj);
#endif

//  PROVIDE A WARNING (ONLY) IF THE TOPOLOGY IS NOT CONNECTED
    if(gattr.nwans > 0 && connected(_NNODES, adj) == false)
	WARNING("this topology is not connected\n");
    for(int n=0 ; n<_NNODES ; ++n)
	FREE(adj[n]);
    FREE(adj);

    find_mapsize();

//  IF ANY SIGNIFICANT ERRORS OCCURED, TERMINATE THE PROGRAM
    if(nerrors)
	CLEANUP(1);

//  GIVE SOME DEFAULT X,Y,Z COORDINATES IF THEY HAVE NOT BEEN SPECIFIED
    rootn	= (int)sqrt((double)(_NNODES-1)) + 1;
    FOREACH_NODE {
	na	= &np->nattr;
	x	= n%rootn;
	y	= n/rootn;

	if(na->position.x == UNKNOWN) {
/*
	    if(np->nodetype == NT_MOBILE)
		na->position.x	= mt19937_int31(mt) %
			(3*gattr.maxposition.x/4) + gattr.maxposition.x/8;
	    else
 */
		na->position.x	= (2*x+1) * DEFAULT_NODE_SPACING;
	}
	if(na->position.y == UNKNOWN) {
/*
	    if(np->nodetype == NT_MOBILE)
		na->position.y	= mt19937_int31(mt) %
			(3*gattr.maxposition.y/4) + gattr.maxposition.y/8;
	    else
 */
		na->position.y	= (2*y+1) * DEFAULT_NODE_SPACING;
	}
	if(na->position.z == UNKNOWN)
	    na->position.z = 0;

//  IF UNKNOWN, SET THIS NODE'S POPUP WINDOW POSITION
	if(na->winx == UNKNOWN)
	    na->winx	= na->position.x;
	if(na->winy == UNKNOWN)
	    na->winy	= na->position.y;

//  COMMAND-LINE ARGUMENTS USED IF NO rebootargv PROVIDED BY TOPOLOGY FILE
	if(na->reboot_argv == NULL)
	    init_reboot_argv(na, argc, argv);
    }

//  SEED EACH NODE'S DEFAULT RANDOM NUMBER GENERATOR
    FOREACH_NODE {
	np->mt	= mt19937_init((unsigned long)Sflag);
    }
}
