#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

typedef struct {
    int			destnode;	// index into NODES[]
    int			destlink;	// link at NODES[destnode]
    CnetTime		arrives;	// time in usec the frame arrives
    bool		corrupted;
    size_t		len;		// len of data iff not corrupted
    char		*frame;

    CnetLinkType	linktype;	// one of LT_*
    int			linkindex;	// into WANS[], LANS[], or WLANS[]
    double		rx_signal;	// WLAN - signal strength of arrival
    double		rx_angle;	// WLAN - angle of arrival
} FRAMEARRIVAL;

extern	void	assign_nicaddr(int thisnode, int thisnic);

extern	int	corrupt_frame(NICATTR *nic, char *frame, size_t len);
extern	int	lose_frame   (NICATTR *nic, size_t len);
