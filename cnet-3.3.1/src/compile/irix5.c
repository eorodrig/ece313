#include <dlfcn.h>
#include <nlist.h>
#include <libelf.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

static int add_compile_args(int ac, char *av[])
{
#if	!USE_GCC_COMPILER
    av[ac++] =	"-woff";
    av[ac++] =	"636,653,740,771";
#endif
    return(ac);
}


static int add_link_args(int ac, char *av[])
{
    av[ac++] =	"-elf";
    av[ac++] =	"-shared";
    av[ac++] =	"-d";
    return(ac);
}


static int elf_sizes(char *fn, unsigned long *datalen, unsigned long *bsslen)
{
    int		fd;
    int		found = 0;
    char	*sname;
    Elf		*elf;
    Elf32_Ehdr	*ehdr;
    Elf_Scn	*scn;
    Elf32_Shdr	*shdr;

    if((fd = open(fn, O_RDONLY, 0)) < 0) {
	F(stderr,"%s: cannot open %s\n", argv0, fn);
	return(1);
    }

    elf_version(EV_CURRENT);
    elf = elf_begin(fd, ELF_C_READ, NULL);
    if(elf == NULL || elf_kind(elf) != ELF_K_ELF) {
	F(stderr,"%s: %s is not in ELF format\n", argv0, fn);
	close(fd);
	return(1);
    }

    ehdr = elf32_getehdr(elf);
    scn	= NULL;

    while((scn = elf_nextscn(elf, scn)) != 0) {
	shdr	= elf32_getshdr(scn);
	sname	= elf_strptr(elf, ehdr->e_shstrndx, (size_t)shdr->sh_name);

	if(sname != NULL) {
	    if(strcmp(sname, ELF_DATA) == 0) {
		*datalen	= (unsigned long)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS) == 0) {
		*bsslen		= (unsigned long)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	}
    }
    elf_end(elf);
    close(fd);
    return(0);
}


static void data_segments(int n, void *handle, char *so_filenm)
{
    extern int	 	nlist(const char *, struct nlist *);

    typedef struct _c {
	char		*so_filenm;
	unsigned long	length_data[NDATASEGS];
	char		*incore_data[NDATASEGS];
	char		*original_data[NDATASEGS];
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = NULL;
    CACHE		*cp  = chd;
    struct nlist	nls[3];
    NODE		*np	= &NODES[n];
    int			i;

    while(cp != NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    nls[0].n_name	= ELF_DATA;
    nls[1].n_name	= ELF_BSS;
    nls[2].n_name	= NULL;

    if(nlist(so_filenm, nls) != 0) {
	F(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	exit(1);
    }

    cp			= malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    if(elf_sizes(so_filenm, &(cp->length_data[0]), &(cp->length_data[1])) != 0)
	exit(1);

// FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT
    for(i=0 ; i<NDATASEGS ; ++i) {
	if(nls[i].n_value) {
	    cp->incore_data[i]		= (char *)nls[i].n_value;
	    cp->original_data[i]	= malloc(cp->length_data[i]);
	    memcpy(cp->original_data[i],
			    cp->incore_data[i], cp->length_data[i]);
	}
	else {
	    cp->length_data[i] = 0;
	    cp->incore_data[i] = cp->original_data[i] = NULL;
	}
    }
    cp->next		= chd;
    chd			= cp;

    if(dflag) {
	F(stderr,"%s:\n", so_filenm);
	F(stderr,"\t  initialized=0x%08lx,   len(initialized)=%ld\n",
				nls[0].n_value, cp->length_data[0]);
	F(stderr,"\tuninitialized=0x%08lx, len(uninitialized)=%ld\n",
				nls[1].n_value, cp->length_data[1]);
    }

found:

// FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT
    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	if(np->length_data[i]) {
	    np->private_data[i]	= malloc(cp->length_data[i]);
	    memcpy(np->private_data[i],
			    cp->original_data[i], cp->length_data[i]);
	}
	else
	    np->private_data[i]	= NULL;
    }
}
