#include <fcntl.h>
#include <elf.h>
#include <nlist.h>
#include <dlfcn.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

// Sections of this NetBSD code were contributed by Mark Davies,
// Victoria University of Wellington, NZ <mark@mcs.vuw.ac.nz>
// Wed, 15 Aug 2001.

static int add_compile_args(int ac, char *av[])
{
    av[ac++]	= "-rdynamic";
    av[ac++]	= "-fPIC";
    return(ac);
}


static int add_link_args(int ac, char *av[])
{
    av[ac++] =	"-shared";
    return(ac);
}


#define	ELF_DATA_SYMBOL		".data"
#define	ELF_BSS_SYMBOL		".bss"
#define	ELF_END_SYMBOL		"_end"


typedef struct {
    unsigned int	datalen;
    unsigned int	bsslen;
    unsigned int	dataaddr;
    unsigned int	bssaddr;
    unsigned int	endaddr;
} ELFinfo;


/*  The following function borrows heavily from the source of elf2aout.c
    from the NetBSD-1.5 sources.
 */
static char *saveRead(int file, off_t offset, off_t len, char *name)
{
        char   *tmp;
        int     count;
        off_t   off;
        if ((off = lseek(file, offset, SEEK_SET)) < 0) {
                F(stderr, "%s: fseek: %s\n", name, strerror(errno));
                exit(1);
        }
        if (!(tmp =  malloc(len))) {
                F(stderr, "%s: Can't allocate %ld bytes.", name,
			(long)len);
		exit(1);
	}
        count = read(file, tmp, len);
        if (count != len) {
                F(stderr, "%s: read: %s.\n",
                    name, count ? strerror(errno) : "End of file reached");
                exit(1);
        }
        return tmp;
}

static int get_ELFinfo(char *filenm, ELFinfo *ei)
{
    int		fd, i;
    int		found=0, rtn=1;
    int         nsymbols, nstrings;
    int		strtabix, symtabix;
    char	*sname, *strings;
    

    Elf32_Ehdr	ex;
    Elf32_Shdr	*sh;
    char   	*shstrtab;
    Elf32_Sym   *symbols;


    if((fd = open(filenm, O_RDONLY, 0)) < 0) {
	F(stderr,"%s: cannot open %s\n", argv0, filenm);
	return(1);
    }
    // Read the header, which is at the beginning of the file...
    i = read(fd, &ex, sizeof ex);
    if (i != sizeof ex) {
      F(stderr, "%s: ex: %s.\n",
	      argv0, i ? strerror(errno) : "End of file reached");
      close(fd);
      return(1);
    }

    // Read the section headers...
    sh = (Elf32_Shdr *) saveRead(fd, ex.e_shoff,
				 ex.e_shnum * sizeof(Elf32_Shdr), "sh");
    // Read in the section string table.
    shstrtab = saveRead(fd, sh[ex.e_shstrndx].sh_offset,
			sh[ex.e_shstrndx].sh_size, "shstrtab");

    for (i = 0; i < ex.e_shnum; i++) {
        sname = shstrtab + sh[i].sh_name;
 
        if(sname != NULL) {
	    if(strcmp(sname, ELF_DATA_SYMBOL) == 0) {
		ei->dataaddr	= (unsigned int)sh[i].sh_addr;
		ei->datalen	= (unsigned int)sh[i].sh_size;
		if(++found == 2)
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS_SYMBOL) == 0) {
		ei->bssaddr	= (unsigned int)sh[i].sh_addr;
		ei->bsslen	= (unsigned int)sh[i].sh_size;
		if(++found == 2)
		    break;
	    }
	}
    }
    if(found != 2)
	goto done;


 
    /* Look for the symbol table and string table... Also map section
     * indices to symbol types for a.out */
    for (i = 0; i < ex.e_shnum; i++) {
      sname = shstrtab + sh[i].sh_name;
      if (!strcmp(sname, ".symtab"))
	symtabix = i;
      else
	if (!strcmp(sname, ".strtab"))
	  strtabix = i;
      
    }
    symbols = (Elf32_Sym*)saveRead(fd, sh[symtabix].sh_offset,
				   sh[symtabix].sh_size, "symbol table");
    strings = saveRead(fd, sh[strtabix].sh_offset, sh[strtabix].sh_size,
		       "string table");
    nsymbols = sh[symtabix].sh_size / sizeof(Elf32_Sym);
    nstrings = sh[strtabix].sh_size;
    if(!symbols || !strings || !nsymbols || !nstrings)
        goto done;

    for(i=1; i<nsymbols; i++) {
        if(symbols[i].st_name < 0 || symbols[i].st_name >= nstrings)
            goto done;
        if(symbols[i].st_name == 0) {
            continue;
        }
        sname = strings + symbols[i].st_name;
        if(strcmp(sname, ELF_END_SYMBOL) == 0) {
            ei->endaddr = symbols[i].st_value;
            break;
        }
    }
    rtn	= 0;
done:
    close(fd);
    return(rtn);
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

    ELFinfo		ei;
    unsigned int	endat;
    unsigned int	offset;

    while(cp != NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    cp			= malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    errno		= 0;
    if(get_ELFinfo(so_filenm, &ei) != 0) {
	F(stderr,"cannot get ELFinfo\n");
	if(errno != 0)
	    perror(argv0);
	exit(1);
    }
    endat	= (int)dlsym(handle, "_end"),
    offset	= (endat - ei.endaddr);

// FIRST, THE INITIALIZED SEGMENT
    cp->length_data[0] = ei.datalen;
    if(ei.datalen != 0) {
	cp->incore_data[0]		= (char *)offset+ei.dataaddr;
	cp->original_data[0]	= malloc(cp->length_data[0]);
	memcpy(cp->original_data[0], cp->incore_data[0], cp->length_data[0]);
    }
    else
	cp->incore_data[0] = cp->original_data[0] = NULL;

// THEN, THE BSS (UNINITIALIZED) SEGMENT
    cp->length_data[1] = ei.bsslen;
    if(ei.bsslen != 0) {
	cp->incore_data[1]		= (char *)offset+ei.bssaddr;
	cp->original_data[1]	= malloc(cp->length_data[1]);
	memcpy(cp->original_data[1], cp->incore_data[1], cp->length_data[1]);
    }
    else
	cp->incore_data[1] = cp->original_data[1] = NULL;

    cp->next		= chd;
    chd			= cp;

    if(dflag) {
	F(stderr,"%s:\n", so_filenm);
	F(stderr,"\t  initialized=0x%08lx,   len(initialized)=%ld\n",
				(long)offset+ei.dataaddr, cp->length_data[0]);
	F(stderr,"\tuninitialized=0x%08lx, len(uninitialized)=%ld\n",
				(long)offset+ei.bssaddr,  cp->length_data[1]);
    }

found:

// FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT

    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	if(np->length_data[i]) {
	    np->private_data[i]	= malloc(cp->length_data[i]);
	    memcpy(np->private_data[i],cp->original_data[i],cp->length_data[i]);
	}
	else
	    np->private_data[i]	= NULL;
    }
}
