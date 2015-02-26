#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

static	bool	output_commas	= OUTPUT_COMMAS;

bool CNET_get_commas(void)
{
    return output_commas;
}

bool CNET_set_commas(bool wanted)
{
    bool	was = output_commas;

    output_commas	= wanted;
    return was;
}

/*  Large (64 bit) integers, such as instances of CnetTime, may be formatted
    with commas to make them more readable.
    cnet only formats large integers with commas on the GUI, only if
    requested, and never in the statistics printed to stdout.
 */

#define	N_COMMABUFS	4

char *CNET_format64(int64_t value)
{
    static	char	result[N_COMMABUFS][32];
    static	int	whichbuf	= 0;

    char	*rp	= &result[whichbuf][0];

    whichbuf	= (whichbuf+1) % N_COMMABUFS;

    sprintf(rp, "%" PRId64, value);
    if(output_commas) {
	char	buf[32], *b=buf, *r=rp;
	int	i, len;

	if(rp[0] == '-') {
	    *b++	= '-';
	    ++r;
	}

	len	= strlen(r);
	for(i=0 ; i<len ; ++i) {
	    *b++	= *r++;
	    if(*r && ((len-i)%3) == 1)
		*b++	= ',';
	}
	*b	= '\0';
	strcpy(rp, buf);
    }
    return rp;
}
