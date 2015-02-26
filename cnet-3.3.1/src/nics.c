#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


static int hex_to_int(int ch)
{
    if(ch >= '0' && ch <= '9')	return(ch-'0');
    if(ch >= 'a' && ch <= 'f')	return(10 + ch-'a');
    if(ch >= 'A' && ch <= 'F')	return(10 + ch-'A');
    return 0;
}

int CNET_parse_nicaddr(CnetNICaddr nicaddr, char *string)
{
    if(string) {
	int		c, n;

	for(n=0 ; n<LEN_NICADDR ; ++n) {
	    if(!isxdigit((int)*string))
		break;
	    c			= hex_to_int(*string++);
	    if(!isxdigit((int)*string))
		break;
	    nicaddr[n]		= c*16 + hex_to_int(*string++);

	    if(*string == ':' || *string == '-')
		++string;
	}
	if(n == LEN_NICADDR && *string == '\0')
	    return 0;
    }
    memcpy(nicaddr, NICADDR_ZERO, LEN_NICADDR);
    ERROR(ER_BADARG);
    return (-1);
}

int CNET_format_nicaddr(char *buf, CnetNICaddr nicaddr)
{
    if(buf) {
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
			nicaddr[0], nicaddr[1], nicaddr[2], 
			nicaddr[3], nicaddr[4], nicaddr[5] );
	return 0;	// what can go wrong?
    }
    ERROR(ER_BADARG);
    return (-1);
}

void assign_nicaddr(int thisnode, int thislink)
{
    NICATTR	*nic	= &NODES[thisnode].nics[thislink];

    if(memcmp(nic->nicaddr, NICADDR_ZERO, LEN_NICADDR) == 0) {
	nic->nicaddr[0]	= 1;
	nic->nicaddr[1]	= thisnode;
	nic->nicaddr[2]	= thisnode;
	nic->nicaddr[3]	= thisnode;
	nic->nicaddr[4]	= thisnode;
	nic->nicaddr[5]	= thislink;
    }
}

void init_nicattrs(NICATTR *defw, NICATTR *defl, NICATTR *defwl)
{
#if	CNET_PROVIDES_WANS
    if(defw)
	memcpy(defw,  &DEFAULTWAN,  sizeof(NICATTR));
#endif
#if	CNET_PROVIDES_LANS
    if(defl)
	memcpy(defl,  &DEFAULTLAN,  sizeof(NICATTR));
#endif
#if	CNET_PROVIDES_WLANS
    if(defwl) {
	memcpy(defwl, &DEFAULTWLAN, sizeof(NICATTR));
	defwl	= NEW(WLANINFO);
	CHECKALLOC(defwl);
	memcpy(defwl, &DEFAULTWLAN.wlaninfo, sizeof(WLANINFO));
    }
#endif
}
