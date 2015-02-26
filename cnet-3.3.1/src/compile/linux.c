#include <fcntl.h>
#include <libelf.h>
//  An old Linux system may require this, instead:
//	#include <libelf/libelf.h>
#include <dlfcn.h>


// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.


// Some of the following code borrows heavily from the sourcefile nlist.c
// of the public release of libelf-0.6.4,
// a free ELF object file access library,
// written by Michael "Tired" Riepe <michael@stud.uni-hannover.de>
// available from	http://www.stud.uni-hannover.de/~michael/software/
//
// Special thanks, too, to Dean Scarff <scarfd01@student.uwa.edu.au> for
// providing patches for 64-bit Linux.


#define	ELF_DATA_SYMBOL		".data"
#define	ELF_BSS_SYMBOL		".bss"
#define	ELF_END_SYMBOL		"_end"

typedef struct {
    uintptr_t		datalen;
    uintptr_t		bsslen;
    uintptr_t		dataaddr;
    uintptr_t		bssaddr;
    uintptr_t		endaddr;
} ELFinfo;

//  -------------------------------------------------------------------------

static int add_compile_args(int ac, char *av[])
{
    av[ac++]	= "-rdynamic";
    av[ac++]	= "-fPIC";
    return ac;
}

static int add_link_args(int ac, char *av[])
{
    av[ac++] =	"-shared";
    av[ac++]	= "-fPIC";
    return ac;
}

//  -------------------------------------------------------------------------

static int get_ELFinfo(char *filenm, ELFinfo *ei)
{
    int			fd;
    int			nfound=0, rtn=1;
    int			nsymbols, nstrings;
    char		*sname, *strings;

    Elf			*elf;
    char		*eident;
    size_t		eidentlen = 0;

    union {
	Elf32_Ehdr	*c32;
	Elf64_Ehdr	*c64;
    } ehdr;
    union {
	Elf32_Shdr	*c32;
	Elf64_Shdr	*c64;
    } shdr;
    union {
	Elf32_Sym	*c32;
	Elf64_Sym	*c64;
    } symbols;

    Elf_Scn		*scn;
    Elf_Scn		*strtab = NULL;
    Elf_Scn		*symtab = NULL;
    Elf_Data		*strdata, *symdata;

    memset(ei, 0, sizeof(ELFinfo));

    if((fd = open(filenm, O_RDONLY, 0)) < 0) {
	F(stderr, "%s: cannot open %s\n", argv0, filenm);
	return 1;
    }
    elf_version(EV_CURRENT);
    elf = elf_begin(fd, ELF_C_READ, NULL);
    if(elf == NULL || elf_kind(elf) != ELF_K_ELF) {
	F(stderr, "%s: %s is not in ELF format\n", argv0, filenm);
	close(fd);
	return 1;
    }

    eident	= elf_getident(elf, &eidentlen);
    if(eident == NULL || eidentlen <= EI_CLASS) {
	F(stderr, "%s: %s missing ELF CLASS ident\n",
			argv0, filenm);
	close(fd);
	return 1;
    }
    if(!((ELFCLASS32|ELFCLASS64) & eident[EI_CLASS])) {
	F(stderr, "%s: %s unrecognised ELF class '%hhd'\n",
			argv0, filenm, eident[EI_CLASS]);
	close(fd);
	return 1;
    }

//  Test whether the file has the 64-bit ELF class.
#define	ELF64P			(ELFCLASS64 == eident[EI_CLASS])
//  Behave like "x[i].y" for the class-conditional unions.
#define	ELF_SUB_DOT(x, i, y)	((ELF64P) ? (x).c64[(i)].y : (x).c32[(i)].y)
//  Behave like "x->y" for the class-conditional unions.
#define	ELFMA(x, y)		((ELF64P) ? (x).c64->y : (x).c32->y)

    if(ELF64P)
	ehdr.c64 = elf64_getehdr(elf);
    else
	ehdr.c32 = elf32_getehdr(elf);

    scn	= NULL;
    while((scn = elf_nextscn(elf, scn)) != 0) {
	if(ELF64P)
	    shdr.c64 = elf64_getshdr(scn);
	else
	    shdr.c32 = elf32_getshdr(scn);

	sname	= elf_strptr(elf, ELFMA(ehdr, e_shstrndx),
			     (size_t) ELFMA(shdr, sh_name));

	if(sname != NULL) {
	    if(strcmp(sname, ELF_DATA_SYMBOL) == 0) {
		ei->dataaddr	= (uintptr_t) ELFMA(shdr, sh_addr);
		ei->datalen	= (uintptr_t) ELFMA(shdr, sh_size);
		if(++nfound == 2)	// that's all we need
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS_SYMBOL) == 0) {
		ei->bssaddr	= (uintptr_t) ELFMA(shdr, sh_addr);
		ei->bsslen	= (uintptr_t) ELFMA(shdr, sh_size);
		if(++nfound == 2)	// that's all we need
		    break;
	    }
	}
    }

//  WE COULD USE THE STANDARD nlist() FUNCTION HERE, BUT WE ALREADY HAVE
//  THE SHARED LIBRARY OPENED, SO WE JUST USE THE "RAW" LIBELF ROUTINES.

    scn	= NULL;				// rewind ELF file
    while((scn = elf_nextscn(elf, scn)) != 0) {
	if(ELF64P)
	    shdr.c64 = elf64_getshdr(scn);
	else
	    shdr.c32 = elf32_getshdr(scn);

	sname	= elf_strptr(elf, ELFMA(ehdr, e_shstrndx),
				     (size_t) ELFMA(shdr, sh_name));

	if(ELFMA(shdr, sh_type) != SHT_SYMTAB &&
	   ELFMA(shdr, sh_type) != SHT_DYNSYM) {
	    continue;
	}
	symtab = scn;
	strtab = elf_getscn(elf, ELFMA(shdr, sh_link));
	if(ELFMA(shdr, sh_type) == SHT_SYMTAB)
	    break;
    }
    symdata = elf_getdata(symtab, NULL);
    strdata = elf_getdata(strtab, NULL);
    if(!symdata || !strdata)
	goto done;

    if(ELF64P)
	symbols.c64 = (Elf64_Sym *)symdata->d_buf;
    else
	symbols.c32 = (Elf32_Sym *)symdata->d_buf;

    strings	 = (char *)strdata->d_buf;
    nsymbols	 = symdata->d_size;
    nsymbols	/= ELF64P ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);

    nstrings = strdata->d_size;
    if(!((ELF64P && symbols.c64) || symbols.c32) || !strings || !nsymbols || !nstrings)
	goto done;

    for(int i=1; i<nsymbols; i++) {
	Elf64_Word stnamei = ELF_SUB_DOT(symbols, i, st_name);

	if(stnamei < 0 || stnamei >= nstrings)
	    goto done;
	else if(stnamei == 0)
	    continue;

	sname = strings + stnamei;
	if(strcmp(sname, ELF_END_SYMBOL) == 0) {
	    ei->endaddr	= ELF_SUB_DOT(symbols, i, st_value);
	    break;
	}
    }
    rtn	= 0;
done:
    elf_end(elf);
    close(fd);
    return rtn;
#undef	ELF64P
#undef	ELF_SUB_DOT
#undef	ELFMA
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

    ELFinfo		ei = {0, 0, 0, 0, 0};
    uintptr_t		endat;
//  ptrdiff_t		offset;
    uintptr_t		offset;

    while(cp != NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    cp			= calloc(1, sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    errno		= 0;
    if(get_ELFinfo(so_filenm, &ei) != 0) {
	F(stderr,"%s: cannot read ELFinfo from %s\n", argv0, so_filenm);
	if(errno != 0)
	    perror(argv0);
	exit(EXIT_FAILURE);
    }
    endat	= (uintptr_t)dlsym(handle, "_end"),
    offset	= (endat - ei.endaddr);

// FIRST, THE INITIALIZED SEGMENT
    cp->length_data[0] = ei.datalen;
    if(ei.datalen != 0) {
	cp->incore_data[0]	= (char *)offset + ei.dataaddr;
	cp->original_data[0]	= calloc(1, cp->length_data[0]);
	memcpy(cp->original_data[0], cp->incore_data[0], cp->length_data[0]);
    }
    else
	cp->incore_data[0] = cp->original_data[0] = NULL;

// THEN, THE BSS (UNINITIALIZED) SEGMENT
    cp->length_data[1] = ei.bsslen;
    if(ei.bsslen != 0) {
	cp->incore_data[1]	= (char *)offset+ei.bssaddr;
	cp->original_data[1]	= calloc(1, cp->length_data[1]);
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
    for(int i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	if(np->length_data[i]) {
	    np->private_data[i]	= calloc(1, cp->length_data[i]);
	    memcpy(np->private_data[i],cp->original_data[i],cp->length_data[i]);
	}
	else
	    np->private_data[i]	= NULL;
    }
}
