#include <mach-o/loader.h>
#include <CoreServices/CoreServices.h>
#include <sys/param.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	XQUARTZ_INCLUDE		"/opt/X11/include"

static int add_compile_args(int ac, char *av[])
{
    struct stat	sbuf;

    if(stat(XQUARTZ_INCLUDE, &sbuf) == 0 && S_ISDIR(sbuf.st_mode))
	av[ac++] = "-I" XQUARTZ_INCLUDE;

    av[ac++]	= "-fno-common";
    return ac;
}

static int add_link_args(int ac, char *av[])
{
    av[ac++]	= "-bundle";
    av[ac++]	= "-flat_namespace";
    av[ac++]	= "-undefined";
    av[ac++]	= "suppress";
    return ac;
}

static void data_segments(int n, void *handle, char *so_filenm)
{
    struct mach_header	header32;

    int			fd, ncmds;
    off_t		offset;
    long		length_data = 0L;

    typedef struct _c {
	char		*so_filenm;
	long		length_data;
	char		*incore_data;
	char		*original_data;
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = NULL;
    CACHE		*cp  = chd;

    NODE		*np	= &NODES[n];

    while(cp != NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    if((fd = open(so_filenm, O_RDONLY, 0)) < 0 ||
	read(fd, &header32, sizeof(header32)) != sizeof(header32)) {
	++nerrors;
	return;
    }

    if(header32.magic == MH_MAGIC_64) {
	struct mach_header_64	header64;

	lseek(fd, (off_t)0, SEEK_SET);		// rewind, read 64-bit header
	read(fd, &header64, sizeof(header64));
	ncmds	= header64.ncmds;
    }
    else
	ncmds	= header32.ncmds;

    offset	= lseek(fd, (off_t)0, SEEK_CUR);

    for(int nc=0 ; nc<ncmds ; ++nc) {
	struct load_command	loadcmd;

	read(fd, &loadcmd, sizeof(loadcmd));

	if(loadcmd.cmd == LC_SEGMENT_64) {		// 64-bit arch
	    struct segment_command_64	segment64;

	    offset	= lseek(fd, offset, SEEK_SET);
	    read(fd, &segment64, sizeof(segment64));

	    if(strcmp(segment64.segname, SEG_DATA) == 0) {
		length_data	= 0L;
		for(int ns=0 ; ns<segment64.nsects ; ++ns) {
		    struct section_64	section64;
		    uint32_t		pow2, padded;

		    read(fd, &section64, sizeof(section64));

		    pow2	= (1<<section64.align);
		    padded	= ((section64.size+(pow2- 1))/pow2) * pow2;
		    length_data += padded;

		    if(dflag)
			REPORT("%s.%s size=%u align=%u pad=%u\n",
				    section64.segname, section64.sectname,
				    section64.size, section64.align, padded);
		}
		break;
	    }
	}
	else if(loadcmd.cmd == LC_SEGMENT) {		// 32-bit arch
	    struct segment_command	segment32;

	    offset	= lseek(fd, offset, SEEK_SET);
	    read(fd, &segment32, sizeof(segment32));

	    if(strcmp(segment32.segname, SEG_DATA) == 0) {
		length_data	= 0L;
		for(int ns=0 ; ns<segment32.nsects ; ++ns) {
		    struct section	section32;
		    uint32_t		pow2, padded;

		    read(fd, &section32, sizeof(section32));

		    pow2	= (1<<section32.align);
		    padded	= ((section32.size+(pow2- 1))/pow2) * pow2;
		    length_data += padded;

		    if(dflag)
			REPORT("%s.%s size=%u align=%u pad=%u\n",
				    section32.segname, section32.sectname,
				    section32.size, section32.align, padded);
		}
		break;
	    }
	}
	offset	+= loadcmd.cmdsize;
	offset	= lseek(fd, offset, SEEK_SET);
    }
    close(fd);

    cp			= malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    cp->length_data	= length_data;
    cp->incore_data	= dlsym(handle, CNET_DATA_SYM);
    cp->original_data	= malloc(cp->length_data);
    memcpy(cp->original_data, cp->incore_data, cp->length_data);
    cp->next		= chd;
    chd			= cp;

    if(dflag)
	REPORT("%s dataseg=0x%08lx len(dataseg)=%ld\n",
			    so_filenm, (long)cp->incore_data, cp->length_data);
found:
    np->length_data[0]		= cp->length_data;
    np->incore_data[0]		= cp->incore_data;
    np->original_data[0]	= cp->original_data;

    np->private_data[0]		= malloc(cp->length_data);
    memcpy(np->private_data[0], cp->original_data, cp->length_data);
}
