#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_WANS

//  ------------ Nothing in here for USE_ASCII compilation ------------

#if	defined(USE_TCLTK)

typedef enum {
	OP_MOVEFRAME, OP_ZAPON, OP_ZAPOFF, OP_TURNWHITE, OP_DELFRAME
} OPTYPE;

typedef struct {
    OPTYPE	optype;
    int		wan;
    int		which;
    int		dx;
} OP_DRAWFRAME;

//  CALLED AFTER EV_DRAWFRAME HANDLER ADDS COLOURS, PIXEL LENGTHS, AND STRING
void add_drawframe(DRAWFRAME *df)
{
    extern void	schedule_moveframe(CnetTime when, CnetData data);

    WAN			*wanp	= &WANS[df->wan];
    OP_DRAWFRAME	*op;
    CnetTime		when, delta;

    char		cmd[BUFSIZ], *s, *t;
    int			nfields, which, dx;
    int			startx, starty, dir;

    FREE(df->cdf.frame);
    if(df->cdf.nfields >= DRAWFRAME_MAXFIELDS)
	df->cdf.nfields = DRAWFRAME_MAXFIELDS;

//  ENSURE THAT AT LEAST ONE VALID COLOUR AND LENGTH WAS PROVIDED
    for(nfields=0 ; nfields < df->cdf.nfields ; ++nfields)
	if(df->cdf.colours[nfields] == NULL || df->cdf.pixels[nfields] <= 0)
	    break;
    if(nfields == 0) {
	FREE(df);
	return;
    }
    df->cdf.nfields = nfields;

//  ENSURE THAT WE HAVE A FREE SLOT TO REMEMBER THIS DRAWFRAME
    for(which=0 ; which<MAX_DRAWFRAMES ; ++which)
	if(wanp->dfs[which] == NULL)
	    break;
    if(which == MAX_DRAWFRAMES) {
	FREE(df);
	return;		// doh! too many already, cannot draw
    }

    df->cdf.text[DRAWFRAME_TEXTLEN-1]	= '\0';
    wanp->dfs[which]			= df;
    wanp->ndrawframes++ ;

//  BUILD THE TCL/TK COMMAND IN cmd
    if(df->src == wanp->lhsnode) {
	startx	= 2 * DEFAULT_NODE_SPACING;
	starty	= DEFAULT_NODE_SPACING / 4;
	dir	= -1;
    }
    else {
	startx	= DRAWFRAME_WIDTH - 2 * DEFAULT_NODE_SPACING;
	starty	= DRAWFRAME_HEIGHT - DEFAULT_NODE_SPACING / 2;
	dir	= 1;
    }
    s	= cmd;
    sprintf(cmd, "DrawFrame %i %i %i %i df%i-%i %i {",
		    df->wan, nfields, startx, starty, df->wan, which, dir);
    while(*s) ++s;
    for(int n=0 ; n<nfields ; ++n) {
	sprintf(s, " \"%s\"", df->cdf.colours[n]);
	while(*s) ++s;
    }
    *s++	= ' ';
    *s++	= '}';
    *s++	= ' ';
    *s++	= '{';
    for(int n=0 ; n<nfields ; ++n) {
	sprintf(s, " %i", df->cdf.pixels[n] % 101);
	while(*s) ++s;
    }
    *s++	= ' ';
    *s++	= '}';
    *s++	= ' ';
    *s++	= '"';
    t		= df->cdf.text;
    while(*t) {
	if(*t == '"')
	    *s++ = '\\';
	*s++	 = *t++;
    }
    *s++	= '"';
    *s++	= '\0';
    TCLTK(cmd);

    dx		= -dir * (DRAWFRAME_WIDTH - 4 * DEFAULT_NODE_SPACING) /
			    DRAWFRAME_WAYPOINTS;
    delta	= df->inflight / DRAWFRAME_WAYPOINTS;
    when	= 0;

//  SCHEDULE AN EVENT FOR EACH WAYPOINT OF THIS FRAME
    for(int wp=0 ; wp<=DRAWFRAME_WAYPOINTS ; ++wp) {
	op		= NEW(OP_DRAWFRAME);
	CHECKALLOC(op);
	op->optype	= OP_MOVEFRAME;
	op->wan		= df->wan;
	op->which	= which;
	op->dx		= dx;
	when		+= delta;
	schedule_moveframe(when, (CnetData)op);
    }
//  SCHEDULE AN EVENT TO DELETE THIS FRAME
    op			= NEW(OP_DRAWFRAME);
    CHECKALLOC(op);
    op->optype		= OP_DELFRAME;
    op->wan		= df->wan;
    op->which		= which;
    when		+= delta;
    schedule_moveframe(when, (CnetData)op);

//  SCHEDULE AN EVENT IF THIS FRAME IS CORRUPTED MID-FLIGHT
    if(df->corrupt) {
	op		= NEW(OP_DRAWFRAME);
	CHECKALLOC(op);
	op->optype	= OP_ZAPON;
	op->wan		= df->wan;
	op->which	= which;
	when		= df->inflight / 2;
	schedule_moveframe(when, (CnetData)op);

	op		= NEW(OP_DRAWFRAME);
	CHECKALLOC(op);
	op->optype	= OP_ZAPOFF;
	op->wan		= df->wan;
	op->which	= which;
	schedule_moveframe(when + DRAWFRAME_ZAPDELAY, (CnetData)op);
    }
//  OR SCHEDULE AN EVENT IF THIS FRAME IS LOST MID-FLIGHT
    else if(df->lost) {
	op		= NEW(OP_DRAWFRAME);
	CHECKALLOC(op);
	op->optype	= OP_TURNWHITE;
	op->wan		= df->wan;
	op->which	= which;
	when		= df->inflight / 2;
	schedule_moveframe(when, (CnetData)op);
    }
}

//  ONLY CALLED FROM write_wan() IF WAN's DRAWFRAME WINDOW IS DISPLAYED
void new_drawframe(int wan, size_t len, char *frame,
		   bool lost, bool corrupt,
		   CnetTime Twhen, CnetTime Tprop)
{
    DRAWFRAME	*df;

    df			= NEW(DRAWFRAME);
    CHECKALLOC(df);
    df->wan		= wan;
    df->src		= THISNODE;
    df->inflight	= Tprop;
    df->corrupt		= corrupt;
    df->lost		= lost;

    df->cdf.text[0]	= '\0';
    df->cdf.len		= len;
    df->cdf.frame	= malloc(len);
    CHECKALLOC(df->cdf.frame);
    memcpy(df->cdf.frame, frame, len);

//  ENABLE USER-CODE TO ADD REQUIRED COLOURS, PIXEL LENGTHS, AND STRING
    newevent(EV_DRAWFRAME, THISNODE, Twhen, NULLTIMER, (CnetData)df);
}

void forget_drawframes(int wan)
{
    WAN		*wanp	= &WANS[wan];
    int		n;

    for(n=0 ; n<MAX_DRAWFRAMES ; ++n)
	if(wanp->dfs[n])
	    TCLTK(".drawframe%i.cf.can itemconfigure df%i-%i -fill white",
			wan, wan, n);
    memset(&wanp->dfs, 0, MAX_DRAWFRAMES * sizeof(DRAWFRAME *));
    wanp->ndrawframes	 = 0;
}

//  RE-COLOUR, DELETE, AND MOVE ALL OF THE FRAMES CURRENTLY IN TRANSIT
void move_drawframe(CnetData data)	// called from scheduler.c
{
    OP_DRAWFRAME *op	= (OP_DRAWFRAME *)data;

    char	canvas[32];
    int		savev	= vflag;

    vflag	= 0;

    sprintf(canvas, ".drawframe%i.cf.can", op->wan);
    switch (op->optype) {
    case OP_MOVEFRAME :
	TCLTK("%s move df%i-%i %i 0 ; %s move tdf%i-%i %i 0",
		    canvas, op->wan, op->which, op->dx,
		    canvas, op->wan, op->which, op->dx);
	break;

    case OP_TURNWHITE :
	TCLTK("%s itemconfigure df%i-%i -fill white",
		    canvas, op->wan, op->which);
	break;

    case OP_ZAPON :
	TCLTK("%s create image %i %i -image im_zap -anchor w -tags dfzap",
		    canvas, DRAWFRAME_WIDTH/2, DRAWFRAME_HEIGHT/2);
	TCLTK("%s itemconfigure df%i-%i -fill grey50",
		    canvas, op->wan, op->which);
	break;

    case OP_ZAPOFF :
	TCLTK("%s delete dfzap", canvas);
	break;

    case OP_DELFRAME: {
	WAN		*wanp	= &WANS[op->wan];

	TCLTK("%s delete df%i-%i ; %s delete tdf%i-%i",
		    canvas, op->wan, op->which,
		    canvas, op->wan, op->which);

	FREE(wanp->dfs[op->which]);
	wanp->ndrawframes-- ;
	break;
      }
    }
    FREE(op);
    vflag	= savev;
}

void toggle_drawframes(int wan)
{
    WAN	*wanp	= &WANS[wan];

    if(wanp->drawframes_init == false) {
	char	name[64];

	memset(wanp->dfs, 0, MAX_DRAWFRAMES * sizeof(DRAWFRAME *));
	wanp->ndrawframes	= 0;

	sprintf(name, "drawframe_displayed(%i)", wan);
	wanp->drawframes_displayed	= false;
	Tcl_LinkVar(tcl_interp, name,
                     (char *)&wanp->drawframes_displayed, TCL_LINK_BOOLEAN);

	if(NODES[wanp->minnode].nattr.position.x <=
	   NODES[wanp->maxnode].nattr.position.x) {
	    wanp->lhsnode	= wanp->minnode;
	    wanp->rhsnode	= wanp->maxnode;
	}
	else {
	    wanp->lhsnode	= wanp->maxnode;
	    wanp->rhsnode	= wanp->minnode;
	}
	TCLTK("toggleDrawframe %i \"WAN frames between %s and %s\" %i %i",
		wan,
		NODES[wanp->lhsnode].nodename, NODES[wanp->rhsnode].nodename,
		DRAWFRAME_WIDTH, DRAWFRAME_HEIGHT);

	sprintf(name, ".drawframe%i.cf.can", wan);
	draw_node_icon(wanp->lhsnode, name,
		    DEFAULT_NODE_SPACING, DRAWFRAME_HEIGHT/2);
	draw_node_icon(wanp->rhsnode, name,
		    DRAWFRAME_WIDTH-DEFAULT_NODE_SPACING, DRAWFRAME_HEIGHT/2);

	wanp->drawframes_init		= true;
    }
    else
	TCLTK("toggleDrawframe %i 0 0 0", wan);
}
#endif		// defined(USE_TCLTK)

#endif		// CNET_PROVIDES_WANS
