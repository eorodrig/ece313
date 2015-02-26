#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

/*
    Here we'd like a fast Poisson distribution function to support the MTBF
    of nodes and links and for AL message generation rates.  Expected values
    are presented in microseconds and the actual Poisson failure variates
    returned as microseconds:

	poisson_usecs(microseconds)	-> microseconds

    Calculate random interarrival times for a Poisson process.  These
    have an exponential density             1/lambda * exp(- t/lambda),
    where lambda is the mean interarrival time.  The cumulative distribution
    function for this is                  y = 1 - exp(- t/lambda).
    The inverse of this is                t = -lambda ln(1 - y).
    According to Knuth, Vol 2, pp116, 128, we can plug a uniform random
    variable in for y, and t will have the exponential distribution we want.
    The following adjustments have been made:

    1) Even though y and 1-y have the same uniform distribution, we
       don't try to save the subtraction by substituting y for 1-y.
       This is so that the parameter to ln is never zero.  Our random
       variable y is such that 0 <= y < 1, so 0 < 1 - y <= 1.
    2) We never want a zero value.  This is achieved by subtracting
       one from the desired mean at the beginning.  This way we get
       an exponential distribution with mean lambda-1.  Then we add
       one to this random variable just before returning it.  This
       gives us a mean of lambda, and we never have a zero value.
    3) We save a negation by writing (1 - lambda) instead of -(lambda - 1).


    Thanks to Jimmy Wilkinson <wilkins@CS.cofc.EDU> for providing this code.
*/


CnetTime poisson_usecs(CnetTime mean_usecs, MT *mt)
{
    double	p;

    p	= (((1.0 - (double)mean_usecs) * log(1.0 - mt19937_real53(mt)) + 1.0));
    return((CnetTime)p);
}
