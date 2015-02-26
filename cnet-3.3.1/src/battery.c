#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_PROVIDES_MOBILITY

int CNET_set_battery(double new_volts, double new_mAH)
{
    int	 result	= -1;

    if(NP->nodetype != NT_MOBILE)
	ERROR(ER_NOTSUPPORTED);
    else if(new_volts < 0.0 || new_mAH < 0.0)
	ERROR(ER_BADARG);
    else {
	NP->nattr.battery_volts	= new_volts;
	NP->nattr.battery_mAH	= new_mAH;
	result			= 0;
    }

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE(0, "\t%s(new_mAH=%.2f, new_mAH=%.2f) = 0\n",
			__func__, new_volts, new_mAH);
	else
	    TRACE(1, "\t%s(new_mAH=%.2f, new_mAH=%.2f) = -1\n",
			__func__, new_volts, new_mAH,
			cnet_errname[(int)cnet_errno]);
    }
    return(result);
}

int CNET_get_battery(double *volts, double *mAH)
{
    int	 result	= -1;

    if(NP->nodetype != NT_MOBILE)
	ERROR(ER_NOTSUPPORTED);
    else {
	if(volts)
	    *volts	= NP->nattr.battery_volts;
	if(mAH)
	    *mAH	= NP->nattr.battery_mAH;
	result			= 0;
    }

    if(gattr.trace_events) {
	char	res1[32], res2[32];

	if(volts)
	    sprintf(res1, "%.2f", NP->nattr.battery_volts);
	else
	    strcpy(res1, "NULL");
	if(mAH)
	    sprintf(res2, "%.2f", NP->nattr.battery_mAH);
	else
	    strcpy(res2, "NULL");

	if(result == 0)
	    TRACE(0, "\t%s(*volts, *mAH) = 0 (%s,%s)\n",
			__func__, res1, res2);
	else
	    TRACE(1, "\t%s(*volts, *mAH) = -1\n",
			__func__,
			cnet_errname[(int)cnet_errno]);
    }
    return(result);
}
#endif
