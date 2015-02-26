#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_MOBILITY

#include "physicallayer.h"


//  COORDINATES ARE IN METRES
int CNET_get_position(CnetPosition *now, CnetPosition *max)
{
    NODEATTR	*na = &NP->nattr;

    if(now) {
	if(gattr.positionerror == 0) {
//  REPORT CURRENT POSITION ACCURATELY
	    now->x	= na->position.x;
	    now->y	= na->position.y;
	    now->z	= na->position.z;
	}
	else {
//  INTRODUCE SOME ERROR INTO THE REPORTED POSITION
	    double angle  = (CNET_nextrand(NP->mt) % 360) * M_PI / 180.0;
	    double radius = (CNET_nextrand(NP->mt) % (gattr.positionerror+1));
	    double dx	  = radius * cos(angle);
	    double dy	  = radius * sin(angle);

	    now->x	= na->position.x + dx;
	    now->y	= na->position.y + dy;
	    now->z	= na->position.z;
	}
    }

//  THE SIMULATION BOUNDS ARE ALWAYS REPORTED ACCURATELY
    if(max) {
	max->x	= gattr.maxposition.x;
	max->y	= gattr.maxposition.y;
	max->z	= gattr.maxposition.z;
    }

    if(gattr.trace_events) {
	char	fmta[64], fmtb[64];

	if(now)
	    sprintf(fmta,"{x=%.1f,y=%.1f,z=%.1f}", now->x, now->y, now->z);
	else
	    strcpy(fmta,"NULL");

	if(max)
	    sprintf(fmtb,"{x=%.1f,y=%.1f,z=%.1f}", max->x, max->y, max->z);
	else
	    strcpy(fmtb,"NULL");

	TRACE(0, "\t%s(&now,&max) = 0 (%s,%s)\n", __func__, fmta, fmtb);
    }
    return 0;
}

//  COORDINATES ARE IN METRES
int CNET_set_position(CnetPosition new)
{
    int		result	= -1;
    NODEATTR	*na	= &NP->nattr;

    if(!NODE_HAS_MOBILITY(THISNODE))
	ERROR(ER_NOTSUPPORTED);
    else if(new.x < 0 || new.x >= gattr.maxposition.x ||
	    new.y < 0 || new.y >= gattr.maxposition.y ||
	    new.z < 0 )
	ERROR(ER_BADPOSITION);
    else {
#if	defined(USE_TCLTK)
	if(Wflag) {
	    int savev	= vflag;
	    vflag	= 0;

/*
	    int	dx	= M2PX(new.x - na->position.x);
	    int	dy	= M2PX(new.y - na->position.y);

	    TCLTK("$map move n%i %i %i ; $map move nl%i %i %i",
			THISNODE, dx, dy, THISNODE, dx, dy);
 */
	    TCLTK("$map coords n%i %i %i ; $map coords nl%i %i %i",
			THISNODE, M2PX(new.x), M2PX(new.y),
			THISNODE, M2PX(new.x), M2PX(new.y)+na->iconheight/2);
	    vflag	= savev;
	}
#endif
	na->position.x	= new.x;
	na->position.y	= new.y;
	na->position.z	= new.z;
	result		= 0;
    }

    if(gattr.trace_events) {
	char	fmt[64];

	sprintf(fmt, "{x=%.1f,y=%.1f,z=%.1f}", new.x, new.y, new.z);
	if(result == 0)
	    TRACE(0, "\t%s(%s) = 0\n", __func__, fmt);
	else
	    TRACE(1, "\t%s(%s) = -1 %s\n",
			__func__, fmt, cnet_errname[(int)cnet_errno]);
    }
    return result;
}

double CNET_get_mapscale(void)
{
    if(gattr.trace_events)
	TRACE(0, "\t%s() = %.2f\n", __func__, gattr.mapscale);

    return gattr.mapscale;
}

int CNET_get_mapmargin(void)
{
    if(gattr.trace_events)
	TRACE(0, "\t%s() = %i\n", __func__, (int)gattr.mapmargin);

    return (int)gattr.mapmargin;
}

#endif
