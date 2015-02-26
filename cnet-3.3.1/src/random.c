#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

void CNET_srand(unsigned int seed)
{
    if(gattr.Sflag != UNKNOWN)
	seed	= gattr.Sflag ^ (THISNODE+1);
    NP->mt = mt19937_init((unsigned long)seed);
}

long CNET_rand(void)
{
    return mt19937_int31(NP->mt);
}

CnetRandom CNET_newrand(unsigned int seed)
{
    if(gattr.Sflag != UNKNOWN)
	seed	= gattr.Sflag ^ (THISNODE+2) ^ ++NP->nnewrands;
    return (CnetRandom)mt19937_init((unsigned long)seed);
}

long CNET_nextrand(CnetRandom mt)
{
    return mt19937_int31((MT *)mt);
}
