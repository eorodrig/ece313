#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filehdr.h>
#include <scnhdr.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

static struct {
    const char	*name;
    long	size;
} section[NDATASEGS] = {
    { ".data",		0L },
    { ".sdata",		0L },
    { ".bss",		0L },
    { ".sbss",		0L },
};

static int read_sections(char *filenm)
{
    struct filehdr	filehdr;
    struct scnhdr	scnhdr;
    int			i, n, fd, nfound;

    if((fd = open(filenm,O_RDONLY,0)) < 0)
	return(1);

    if(read(fd, &filehdr, sizeof(filehdr)) <= 0	||
       lseek(fd, (off_t)filehdr.f_opthdr, SEEK_CUR) <= 0) {
	close(fd);
	return(1);
    }

    nfound	= 0;
    for(i=0 ; i<filehdr.f_nscns && nfound<NDATASEGS ; i++) {
	if(read(fd, &scnhdr, sizeof(scnhdr)) <= 0)
	    break;

	for(n=0 ; n<NDATASEGS ; ++n)
	    if(strncmp(scnhdr.s_name, section[n].name, 8) == 0) {
		section[n].size	= scnhdr.s_size;
		++nfound;
		break;
	    }
    }
    close(fd);
    return(nfound == 0);
}

static int add_compile_args(int ac, char *av[])
{
    av[ac++] =	"-fpic";
    av[ac++] =	"-fPIC";
#if	!USE_GCC_COMPILER
    av[ac++] = "-std1";
#endif
    return(ac);
}

static int add_link_args(int ac, char *av[])
{
    av[ac++] =	"-d";
    av[ac++] =	"-shared";
    av[ac++] =	"-expect_unresolved";
    av[ac++] =	"*";
    return(ac);
}


static void data_segments(int n, void *handle, char *so_filenm)
{
    typedef struct _c {
	char		*so_filenm;
	unsigned long	length_data[NDATASEGS];
	char		*incore_data[NDATASEGS];
	char		*original_data[NDATASEGS];
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = NULL;
    CACHE		*cp  = chd;

    NODE		*np	= &NODES[n];
    int			i;

    while(cp != NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    if(read_sections(so_filenm) != 0) {
	F(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	++nerrors;
	return;
    }

    cp			 = malloc(sizeof(CACHE));
    cp->so_filenm	 = strdup(so_filenm);

    for(i=0 ; i<NDATASEGS ; ++i) {
	cp->length_data[i]	= section[i].size;
	if(0 == cp->length_data[i])
	    continue;
	cp->incore_data[i]	= (char *)dlsym(handle, section[i].name);
	cp->original_data[i]	= malloc(cp->length_data[i]);
	memcpy(cp->original_data[i], cp->incore_data[i], cp->length_data[i]);
    }

    cp->next		 = chd;
    chd			 = cp;

    if(dflag) {
	F(stderr,"%s:\n", so_filenm);
	for(i=0 ; i<NDATASEGS ; ++i)
	    F(stderr,"\t  %6s=0x%012lx len(%6s)=%ld\n",
			section[i].name, (long)cp->incore_data[i],
			section[i].name,  cp->length_data[i]);
    }

found:

// FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT
    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	if(0 == cp->length_data[i])
	    continue;
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	np->private_data[i]	= malloc(cp->length_data[i]);
	memcpy(np->private_data[i], cp->original_data[i], cp->length_data[i]);
    }
}
