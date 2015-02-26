#include "cnetsupport.h"
#include <math.h>

//  c.f.  http://en.wikipedia.org/wiki/Haversine_formula

#if	!defined(M_PI)
#define	M_PI			3.14159265358979323846264338327950288	// pi
#endif

#define	deg2rad(deg)		(deg * M_PI / 180.0)
#define	rad2deg(rad)		(rad * 180.0 / M_PI)

double haversine(double lat1, double lon1, double lat2, double lon2)
{
    double theta = lon1 - lon2;
    double dist	 = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) +
	       cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));

    dist	= acos(dist);
    dist	= rad2deg(dist);
    dist	= dist * 60 * 1151.5;
    return dist;			// metres
}

#undef	rad2deg
#undef	deg2rad

//  vim: ts=8 sw=4
