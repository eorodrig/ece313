#include "cnetprivate.h"
#include <netinet/in.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if   defined(USE_TCLTK)
static	void	init_node_images(void);

static	int	image_maxw	= 0;
static	int	image_maxh	= 0;
#endif

void find_mapsize(void)
{
    NODEATTR	*na;
    int		maxx	= gattr.maxposition.x;	// may not be set yet => 0
    int		maxy	= gattr.maxposition.y;

#if   defined(USE_TCLTK)
    if(Wflag) {
	if(gattr.drawnodes)
	    init_node_images();

    //  IF WE HAVE A BACKGROUND IMAGE, DETERMINE THE DIMENSIONS FROM IT
	if(gattr.mapimage || gattr.maptile) {
	    int	pxw, pxh, metres;

	    TCLTK("image create photo im_map -file %s",
		find_cnetfile(gattr.mapimage ? gattr.mapimage : gattr.maptile,
							false, true));
	    TCLTK("image width im_map");
	    pxw		= atoi(TCLTK_result());
	    metres	= PX2M(pxw);
	    if(maxx < metres)
		maxx	= metres;

	    TCLTK("image height im_map");
	    pxh		= atoi(TCLTK_result());
	    metres	= PX2M(pxh);
	    if(maxy < metres)
		maxy	= metres;

	    if(gattr.mapimage) {
		gattr.mapwidth		= pxw;
		gattr.mapheight		= pxh;
	    }
	    else {
		gattr.tilewidth		= pxw;
		gattr.tileheight	= pxh;
	    }
	}
    }
#endif

//  NOW, DETERMINE THE MAXIMUM DIMENSIONS OF THE WHOLE MAP
    for(int n=0 ; n<_NNODES ; ++n) {
	na      = &NODES[n].nattr;
	if(maxx < na->position.x)
	    maxx = na->position.x;
	if(maxy < na->position.y)
	    maxy = na->position.y;
    }
#if   defined(USE_TCLTK)
    if(Wflag && gattr.drawnodes && gattr.nmobiles == 0)
	maxy	+= DEFAULT_NODE_SPACING;
#endif

//  SET THE MAP'S DIMENSIONS BASED ON THE MAXIMUMS FOUND
    gattr.maxposition.x	= (maxx == 0) ? DEFAULT_MAPWIDTH  : maxx;
    gattr.maxposition.y	= (maxy == 0) ? DEFAULT_MAPHEIGHT : maxy;
    gattr.maxposition.z	= 0;
}


// ----------------- only USE_TCLTK code follows -----------------------

#if   defined(USE_TCLTK)

static void load_image(const char *filenm)
{
    int		h, w;
    char	*path;
    char	gif_name[32];

    sprintf(gif_name, "%s.gif", filenm);
    path	= find_cnetfile(gif_name, false, true);
    TCLTK("image create photo im_%s -file %s", filenm, path);
    FREE(path);

    TCLTK("image height im_%s", filenm);
    h	= atoi(TCLTK_result());
    if(image_maxh < h)
	image_maxh	= h;

    TCLTK("image width im_%s", filenm);
    w	= atoi(TCLTK_result());
    if(image_maxw < w)
	image_maxw	= w;
}

static void init_node_images(void)
{
    load_image("host");

#if	CNET_PROVIDES_WANS
    if(gattr.nrouters > 0)
	load_image("router");
    if(gattr.nwans > 0)
	load_image("zap");		// needed for DRAWFRAMES
#endif

#if	CNET_PROVIDES_WLANS
    if(gattr.nmobiles > 0)
	load_image("mobile");
    if(gattr.naccesspoints > 0)
	load_image("accesspoint");
#endif

    load_image("paused");
    load_image("repair");
}

// ---------------------------------------------------------------------

static void bigX(char *canvas, int x, int y, int size, int t)
{
    typedef struct {
	int		x, y;
    } POINTLIST;

    static POINTLIST BIG[12] = {
	{ -3, -3 },
	{ -1, -3 },
	{ 0, -1 },
	{ 1, -3 },
	{ 3, -3 },
	{ 1, 0 },
	{ 3, 3 },
	{ 1, 3 },
	{ 0, 1 },
	{ -1, 3 },
	{ -3, 3 },
	{ -1, 0 }
    };

    char	buf[256], *b = buf;
    int		p;

    for(p=0 ; p<12 ; ++p) {
	sprintf(b, "%i %i ", x + BIG[p].x * size, y + BIG[p].y * size);
	while(*b) b++;
    }
    TCLTK("%s create polygon %s -fill red -outline black -tags n%i",
		canvas, buf, t);
}

// ---------------------------------------------------------------------

void draw_node_icon(int n, char *canvas, int x, int y)
{
    NODE	*np	= &NODES[n];
    const char	*name	= "";

//  DETERMINE THE ICON/IMAGE FOR THIS NODE
    switch (np->runstate) {
    case STATE_RUNNING :
    case STATE_REBOOTING :
    case STATE_AUTOREBOOT :
    case STATE_CRASHED :

	switch (np->nodetype) {
	case NT_HOST :
	    name	= "host";
	    break;
	case NT_ROUTER :
	    name	= "router";
	    break;
	case NT_MOBILE :
	    name	= "mobile";
	    break;
	case NT_ACCESSPOINT :
	    name	= "accesspoint";
	    break;
	}
	break;

    case STATE_UNDERREPAIR :
	name	= "repair";
	break;

    case STATE_PAUSED :
	name	= "paused";
	break;

    default :
	FATAL("no node icon for %s\n", np->nodename);
	break;
    }

    if(canvas == NULL) {
	canvas	= "$map";
	TCLTK("%s delete n%i nl%i", canvas, n, n);
	x	= M2PX(np->nattr.position.x);
	y	= M2PX(np->nattr.position.y);
    }

//  DRAW THE ICON/IMAGE FOR THIS NODE
    TCLTK("set x [%s create image %i %i -image im_%s -anchor c -tags n%i]",
		canvas, x, y, name, n);
    TCLTK("%s bind $x <Button> \"node_click %i %%b %%X %%Y\"", canvas, n);
#if	defined(USE_MACOSX)
    TCLTK("%s bind $x <Control-ButtonPress-1> \"node_click %i 2 %%X %%Y\"",
		canvas, n);
#endif
    if(np->runstate == STATE_CRASHED)
	bigX(canvas, x, y, 6, n);

//  IF WANTED, DRAW THE NODE'S TITLE BELOW THE NODE'S ICON
    char *title	= format_nodeinfo(np, np->nattr.icontitle);

    if(*title) {
	TCLTK("image height im_%s", name);
	np->nattr.iconheight = atoi(TCLTK_result());
	TCLTK("%s create text %i %i -text \"%s\" -font myfont -anchor n -tags nl%i",
		canvas, x, y+np->nattr.iconheight/2, title, n);
    }
    FREE(title);

#if	CNET_PROVIDES_WANS
//  DRAW THE WAN LINK NUMBERS FOR THIS NODE
    if(strcmp(canvas, "$map") == 0)
      for(int l=1 ; l<=np->nnics ; l++) {
	WAN	*wan;
	int	otherend, dx, dy;
	double	theta;

	if(np->nics[l].linktype != LT_WAN)
	    continue;

	wan	= &WANS[ np->wans[l] ];
	otherend = (wan->minnode == n) ? wan->maxnode : wan->minnode;

	dx	= NODES[otherend].nattr.position.x - np->nattr.position.x;
	dy	= NODES[otherend].nattr.position.y - np->nattr.position.y;
	theta	= atan2((double)dy, (double)dx) + 0.3;
	x	= np->nattr.position.x + (int)(image_maxw/1.5*cos(theta));
	y	= np->nattr.position.y + (int)(image_maxh/1.5*sin(theta));

	TCLTK(
	"%s create text %i %i -text %i -font myfont -anchor c -tags nl%i",
		canvas, M2PX(x), M2PX(y), l, n);
    }
#endif

    if(np->nodetype == NT_MOBILE) {
	TCLTK("%s addtag mobiles withtag  n%i", canvas, n);
	TCLTK("%s addtag mobiles withtag nl%i", canvas, n);
    }
    else
	TCLTK("%s raise mobiles", canvas);

#if 0
{
#define	LEDSIZE	6
    int	led;

    x	= np->nattr.position.x - (int)(CNET_NLEDS * LEDSIZE / 2.5);
    y	= np->nattr.position.y - l/2 - LEDSIZE - 1;
    for(led=0 ; led<CNET_NLEDS ; ++led) {
	TCLTK("$map create rectangle %i %i %i %i -tags {nl%i map%iled%i}",
			x, y, x+LEDSIZE, y+LEDSIZE, n, n, led);
	x += LEDSIZE;
    }
}
#endif
}

#endif		// defined(USE_TCLTK)
