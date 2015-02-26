#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


//  ------------ Nothing in here for USE_ASCII compilation ------------

#if	defined(USE_TCLTK)

static TCLTK_COMMAND(exit_button)
{
    cnet_state  = STATE_UNKNOWN;
    CLEANUP(0);
    return TCL_OK;
}

static TCLTK_COMMAND(run_pause_step)
{
    if(strcmp(argv[1], "run") == 0) {
        if(cnet_state == STATE_PAUSED) {
	    TCLTK(".cnet.mb.cnet entryconfigure 0 -label pause");
            cnet_state  = STATE_RUNNING;
            tcltk_notify_stop();
        }
        else if(cnet_state == STATE_RUNNING) {
	    TCLTK(".cnet.mb.cnet entryconfigure 0 -label run");
            cnet_state  = STATE_PAUSED;
        }
    }
    else /* (argv[1] == "step") */ {
	TCLTK(".cnet.mb.cnet entryconfigure 0 -label run");
        cnet_state      = STATE_SINGLESTEP;
        tcltk_notify_stop();
    }
    return TCL_OK;
}

static TCLTK_COMMAND(save_button)
{
    extern void	save_topology(const char *);

    save_topology(argc > 1 ? argv[1] : NULL);
    return TCL_OK;
}

// ------------------ EVENT_HANDLE/REFRESH/REDRAW canvas-------------------

TCLTK_COMMAND(mouse_position)
{
    int x	= atoi(argv[1]);
    int y	= atoi(argv[2]);
    if(x > 0 && y > 0) {
	sprintf(chararray," %i,%i", PX2M(x), PX2M(y) );
	Tcl_SetVar(tcl_interp, "CN_POSITION", chararray, TCL_GLOBAL_ONLY);
    }
    return TCL_OK;
}

TCLTK_COMMAND(node_click)
{
    extern void	display_nodemenu(int);

    int	n	= atoi(argv[1]);
    int	button	= atoi(argv[2]);
    int	evx	= atoi(argv[3]);
    int	evy	= atoi(argv[4]);

    if(button == LEFT_BUTTON)
	TCLTK("toggleNode %i %i %i", n, evx, evy);
    else if(cnet_state == STATE_RUNNING)
	display_nodemenu(n);

    return TCL_OK;
}

static void draw_map(void)
{
//  DRAW THE BACKGROUP IMAGE IF WE HAVE DEFINED ONE
    if(gattr.mapimage) {
	TCLTK("$map configure -background white");
	TCLTK("$map create image %i %i -image im_map -anchor c",
		    gattr.mapwidth/2, gattr.mapheight/2);	// in pixels
    }
//  OR THE BACKGROUND TILE
    else if(gattr.maptile) {
	TCLTK("$map configure -background white");
	for(int x=0 ; x<gattr.maxposition.x ; x += gattr.mapwidth)
	    for(int y=0 ; y<gattr.maxposition.y ; y += gattr.mapheight)
		TCLTK("$map create image %i %i -image im_map -anchor nw", x, y);
    }
//  OR THE SOLID BACKGROUND COLOUR IS ALWAYS DRAWN
    else if(gattr.mapcolour[0] != '\0')
	TCLTK("$map configure -background \"%s\"", gattr.mapcolour);

//  DRAW THE BACKGROUD HEX PATTERN
    if(gattr.maphex) {
	int	x  = 0;
	int	y  = 0;
	int	dx = (int)(1.5*gattr.maphex);
	int	dy = (int)(sin(60.0/M_PI_2)*gattr.maphex);

	while(y < gattr.maxposition.y) {
	    x	= 0;
	    while(x < gattr.maxposition.x) {
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x, y, x+gattr.maphex-1, y, COLOUR_GRID);
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x+gattr.maphex, y,
				    x+dx, y-dy, COLOUR_GRID);
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x+gattr.maphex+1, y,
				    x+dx, y+dy-1, COLOUR_GRID);
		x	+= 2*dx;
	    }
	    y	+= dy;

	    x	= dx;
	    while(x < gattr.maxposition.x) {
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x, y, x+gattr.maphex-1, y, COLOUR_GRID);
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x+gattr.maphex, y,
				    x+dx, y-dy, COLOUR_GRID);
		TCLTK("$map create line %i %i %i %i -fill \"%s\"",
				    x+gattr.maphex+1, y,
				    x+dx, y+dy-1, COLOUR_GRID);
		x	+= 2*dx;
	    }
	    y	+= dy;
	}
    }
//  OR THE BACKGROUD GRID PATTERN
    else if(gattr.mapgrid) {
	int	x = gattr.mapgrid;
	int	y = gattr.mapgrid;

	while(x < gattr.maxposition.x) {
	    TCLTK("$map create line %i %i %i %i -fill \"%s\"",
			M2PX(x), 0,
			M2PX(x), M2PX(gattr.maxposition.y),
			COLOUR_GRID);
	    x += gattr.mapgrid;
	}
	while(y < gattr.maxposition.y) {
	    TCLTK("$map create line %i %i %i %i -fill \"%s\"",
			0, M2PX(y),
			M2PX(gattr.maxposition.x), M2PX(y),
			COLOUR_GRID);
	    y += gattr.mapgrid;
	}
    }

//  DRAW ALL OF THE LANS
#if	CNET_PROVIDES_LANS
    extern void	draw_lans(void);

    if(gattr.drawlinks && gattr.nlans > 0)
	draw_lans();
#endif

//  DRAW ALL OF THE WANS
#if	CNET_PROVIDES_WANS
    extern void	draw_wans(void);

    if(gattr.drawlinks && gattr.nwans > 0)
	draw_wans();
#endif

//  DRAW THE NIC NUMBERS AROUND EACH NODE AND THE NODE ICONS
    if(gattr.drawnodes)
	for(int n=0 ; n<_NNODES ; ++n)
	    draw_node_icon(n, NULL, 0, 0);

    TCLTK("$map create text 4 4 -anchor nw -font myfont -text \"%s\"",
		CNET_VERSION);
}


// ------------------------------------------------------------------------

char *NICdesc(NICATTR *nic)
{
    char	desc[128], *p=desc;

    sprintf(p, "bandwidth %sbps, ",	CNET_format64(nic->bandwidth));
    while(*p)
	++p;
    sprintf(p, "propagation %susecs",	CNET_format64(nic->propagation));

    p	= strdup(desc);
    CHECKALLOC(p);
    return p;
}

void init_mainwindow(const char *topology,
		     const char *Fflag, int gflag, bool Tflag, bool tflag)
{
    extern int	nic_created(ClientData, Tcl_Interp *, int, char **);
#if	CNET_PROVIDES_KEYBOARD
    extern int	tk_stdio_input(ClientData, Tcl_Interp *, int, char **);
#endif
#if	CNET_PROVIDES_WLANS
    extern int	toggle_drawwlans(ClientData, Tcl_Interp *, int, char **);
#endif

    char	*tcltk_source;

    if(Fflag == NULL)
	Fflag	= findenv("CNETTCLTK", CNETTCLTK);
    tcltk_source = find_cnetfile(Fflag, false, true);
    if(dflag)
	REPORT("reading \"%s\"\n\n", tcltk_source);
    if(Tcl_EvalFile(tcl_interp, tcltk_source) != TCL_OK) {
	FATAL("%s\n", TCLTK_result());
	CLEANUP(1);
    }
    FREE(tcltk_source);

    TCLTK_createcommand("exit_button",		exit_button);
    TCLTK_createcommand("mouse_position",	mouse_position);
    TCLTK_createcommand("nic_created",		nic_created);
    TCLTK_createcommand("node_click",		node_click);
    TCLTK_createcommand("run_pause_step",	run_pause_step);
    TCLTK_createcommand("save_topology",	save_button);
#if	CNET_PROVIDES_KEYBOARD
    TCLTK_createcommand("stdio_input",		tk_stdio_input);
#endif
#if	CNET_PROVIDES_WLANS
    TCLTK_createcommand("toggle_drawwlans",	toggle_drawwlans);
#endif

    TCLTK(
"initworld %s \"%s\" %i %i %s %i {%s %s %s %s} {%i %i %i} {\"%s\" \"%s\" \"%s\"} %i %i",
	topology,
	gattr.mapcolour,
	M2PX(gattr.maxposition.x)+1, M2PX(gattr.maxposition.y)+1,
	gflag ? "pause" : "run",
	(int)Tflag,
	"exit_button", "run_pause_step", "save_topology", "toggle_drawwlans",
	gattr.nwans, gattr.nlans, gattr.nwlans,

#if	CNET_PROVIDES_WANS
	gattr.nwans  > 0 ? NICdesc(&DEFAULTWAN)  : "",
#else
	"",
#endif

#if	CNET_PROVIDES_LANS
	gattr.nlans  > 0 ? NICdesc(&DEFAULTLAN)  : "",
#else
	"",
#endif

#if	CNET_PROVIDES_WLANS
	gattr.nwlans > 0 ? NICdesc(&DEFAULTWLAN) : "",
#else
	"",
#endif
	N_CNET_EVENTS, N_STATS);

    draw_map();
    if(tflag)
	TCLTK("toggleTrace \"%s\"", topology);
}
#endif		// defined(USE_TCLTK)
