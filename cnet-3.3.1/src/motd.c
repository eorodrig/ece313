#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


/*  The motd() function will be called early from main() to give you
    a chance to provide any message-of-the-day to students.  Examples may
    include any recently discovered problems (?) or modifications to
    project requirements.
 */

#define	MOTD_FILE	"/cslinux/examples/CITS3230/cnet.motd"

void motd(void)			// called from cnetmain.c:main()
{
#if	MOTD_WANTED
    char	line[BUFSIZ];
    FILE	*fp;

    if((fp = fopen(MOTD_FILE, "r"))) {
	while(fgets(line, sizeof(line), fp))
	    fputs(line, stdout);
	fclose(fp);
    }
#endif
}
