#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	!defined(CNET4IPOD)

#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dlfcn.h>

#define	PATH_SEPARATOR		':'
#define	N_ALLOCATED		64

#if	defined(USE_LINUX)
#include "compile/linux.c"

#elif	defined(USE_MACOSX)
#define	CNET_DATA_SYM		"_CNET_DATA_SYM"
#include "compile/macosx.c"

#elif	defined(USE_FREEBSD)
#include "compile/freebsd.c"

#elif	defined(USE_NETBSD)
#include "compile/netbsd.c"

#elif	defined(USE_SOLARIS)
#include "compile/solaris.c"

#elif	defined(USE_SUNOS)
#include "compile/sunos.c"

#elif	defined(USE_OSF1)
#include "compile/osf1.c"

#elif	defined(USE_IRIX5)
#include "compile/irix5.c"

#endif


// --------------------------------------------------------------------

static void dynamic_load(int n, char *so_file)
{
#if	defined(USE_MACOSX)
    void *handle	= dlopen(so_file, RTLD_LAZY|RTLD_GLOBAL);
#elif	defined(RTLD_NOW)
    void *handle	= dlopen(so_file, RTLD_NOW|RTLD_GLOBAL);
#else
    void *handle	= dlopen(so_file, RTLD_LAZY|RTLD_GLOBAL);
#endif

//  ENSURE THAT WE CAN SUCCESSFULLY OPEN AND LINK THE SHARED OBJECT
    if(handle == NULL) {
	WARNING("NULL HANDLE from \"%s\"\n\t%s!\n", so_file, dlerror());
	++nerrors;
	return;
    }

//  IF LOADING A NODE'S CODE, ENSURE THAT IT PROVIDES AN EV_REBOOT HANDLER
    if(n >= 0) {
	NODE	*np	= &NODES[n];

#if	defined(PREPEND_REBOOT_UNDERSCORE)
	char	*_func = malloc(strlen(np->nattr.rebootfunc) + 4);

	CHECKALLOC(_func);
	sprintf(_func, "_%s", np->nattr.rebootfunc);
	np->nattr.rebootfunc	= _func;
#endif

	*(void **)(&(np->handler[(int)EV_REBOOT])) =
			dlsym(handle, np->nattr.rebootfunc);

	if(np->handler[(int)EV_REBOOT] == NULL) {
	    WARNING("cannot find %s() in %s\n", np->nattr.rebootfunc, so_file);
	    ++nerrors;
	}
	else
	    data_segments(n, handle, so_file);
    }

//  OTHERSIZE, WE'RE DYNAMICALLY LOADING A CODE EXTENSION
    else
	;
//  WE DON'T dlclose(handle)
}

char *find_cnetfile(const char *filenm, int wantdir, bool fatal)
{
    char	ch;
    char	*pathname;
    char	*env, *orig, *p;

//  TRY TO FIND filenm USING $CNETPATH
    orig = env	= strdup( findenv("CNETPATH", CNETPATH) );
    CHECKALLOC(env);

    while(*env) {
	while(*env == PATH_SEPARATOR)
	    ++env;
	p	= env;
	while(*p && *p != PATH_SEPARATOR)
	    ++p;
	ch	= *p;
	*p	= '\0';
	sprintf(chararray, "%s/%s", env, filenm);	// find this!
	if(access(chararray, R_OK) == 0) {
	    pathname	= strdup(wantdir ? env : chararray);
	    CHECKALLOC(pathname);
	    FREE(orig);
	    return pathname;
	}
	*p	= ch;
	env	= p;
    }
    FREE(orig);

    if(fatal)
	FATAL("cannot locate \"%s\"\n", filenm);
    return NULL;
}

void compile_topology(char **defines)
{
    extern char	*compile_string(char **, const char *, bool);

    char	*so_file;

#if	defined(PREPEND_DOT_TO_LDPATH)
    extern int	putenv(char *);
    const char	*oldenv;
    char	*newenv;

    if((oldenv = findenv("LD_LIBRARY_PATH", NULL)) == NULL) {
	newenv	= strdup("LD_LIBRARY_PATH=\".\"");
	CHECKALLOC(newenv);
    }
    else {
	sprintf(chararray,"LD_LIBRARY_PATH=\"%s:.\"", oldenv);
	newenv	= strdup(chararray);
	CHECKALLOC(newenv);
    }
    putenv(newenv);
#endif

//  FIND AND LOAD, OR COMPILE AND LOAD, EACH CODE EXTENSION
    for(int n=0 ; n<gattr.nextensions ; n++) {
	char	filename[MAXPATHLEN];

	sprintf(filename, "%s%s",
		    gattr.extensions[n], findenv("CNETFILEEXT",CNETFILEEXT));
	so_file	= find_cnetfile(filename, false, false);

	if(so_file && access(so_file, X_OK) == 0) {
	    if(dflag)
		REPORT("using extension \"%s\"\n", so_file);
	}
	else {
	    so_file	= compile_string(defines, gattr.extensions[n], true);
	    if(nerrors)
		CLEANUP(1);
	}
	dynamic_load(-1, so_file);
	if(nerrors)
	    CLEANUP(1);
    }

//  COMPILE AND LOAD THE COMPILATION STRING OF EACH NODE
    for(int n=0 ; n<_NNODES ; n++) {
	so_file	= compile_string(defines, NODES[n].nattr.compile, true);
	if(nerrors)
	    CLEANUP(1);
	dynamic_load(n, so_file);
	if(nerrors)
	    CLEANUP(1);
    }
}


// --------------------------------------------------------------------

#if	CHECK_RECEIVE_SPELLING
static bool spell_check(char *filenm)
{
    FILE	*fp;
    char	*s;
    bool	passes	= true;
    int		lc	= 0;

    if((fp = fopen(filenm,"r")) != NULL) {
	while(fgets(chararray, BUFSIZ, fp) != NULL) {
	    lc++;
	    s	= chararray;
	    while(*s) {
		if(isupper((int)*s))
		    *s	= tolower((int)*s);
		++s;
	    }
	    if(strstr(chararray, "reciev") || strstr(chararray, "interup")) {
		WARNING("spelling mistake on line %i of %s\n", lc, filenm);
		++nerrors;
		passes	= false;
	    }
	}
	fclose(fp);
    }
    return passes;
}
#endif


// --------------------------------------------------------------------

static int fork_exec(int ac, char *av[])
{
    pid_t	pid;
    int		status = -1;

    switch (pid = fork()) {
    case -1 :
	FATAL("cannot fork\n");
	break;

    case 0 :
	if(dflag) {
	    REPORT("%s", av[0]);
	    for(int a=2 ; a<ac ; a++)
		REPORT(" %s", av[a]);
	    REPORT("\n");
	}

	execvp(av[0], &av[1]);
	FATAL("cannot exec %s\n", av[0]);
	exit(EXIT_FAILURE);
        break;

    default :
	while(wait(&status) != pid)
	    ;
	break;
    }
    return status;
}

static int make_ofile(char **Cflags, char *o_file, char *c_file)
{
    struct stat	stat_c, stat_o;

//  FIRSTLY, ENSURE THAT THE SOURCEFILE EXISTS AND NEEDS RECOMPILING
    if(stat(c_file, &stat_c) == -1) {
	WARNING("cannot find sourcefile %s\n", c_file);
	++nerrors;
	return -1;
    }
    if(stat(o_file, &stat_o) == 0 && stat_c.st_mtime <= stat_o.st_mtime)
	return 0;

#if	CHECK_RECEIVE_SPELLING
    if(spell_check(c_file) == false)
	return -1;
#endif

    char	*tmp;
    char	*av[N_ALLOCATED];	// hope that this is enough!
    int		ac=0;

    av[ac++]	= strdup(findenv("CNETCC", CNETCC));
    av[ac++]	= (tmp = strrchr(av[0],'/')) ? tmp+1 : av[0];

    if(strstr(tmp, "gcc")) {
#if	GCC_STDC99_WANTED
	av[ac++] = "-std=c99";
#endif
#if	GCC_PEDANTIC_WANTED
	av[ac++] = "-pedantic-errors";
#endif
#if	GCC_WERROR_WANTED
	av[ac++] = "-Werror";
#endif
#if	GCC_WALL_WANTED
	av[ac++] = "-Wall";
#endif
    }
    ac	     =	add_compile_args(ac, av);
    av[ac++] =	OS_DEFINE;

//  ADD COMPILER SWITCHES
    while(*Cflags)
	av[ac++] = *Cflags++;

//  ADD THE DIRECTORY CONTAINING cnet.h
    static char	*include_dir	= NULL;
    if(include_dir == NULL)
	include_dir	= find_cnetfile("cnet.h", true, true);
    sprintf(chararray, "-I%s", include_dir);
    av[ac++] =	strdup(chararray);

    av[ac++] =	"-c";
    av[ac++] =	c_file;
    av[ac  ] =	NULL;

    if(!qflag && !vflag)
	REPORT("compiling %s\n", c_file);

    int status = fork_exec(ac, av);
    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	exit(EXIT_FAILURE);

    return 0;
}

static int make_sofile(char **LDflags, char *so_file, char **o_files)
{
    struct stat	stat_o, stat_so;

    char	**ofs;

    if(stat(so_file, &stat_so) == 0) {	// hmmm, so_file exists
	ofs	= o_files;
	while(*ofs) {
	    if(stat(*ofs, &stat_o) == -1 || stat_so.st_mtime <= stat_o.st_mtime)
		break;
	    ++ofs;
	}
	if(*ofs == NULL)
	    return 0;
    }
    unlink(so_file);

    char	*tmp;
    char	*av[N_ALLOCATED];	// hope that this is enough!
    int		ac=0;

    av[ac++]	= strdup(findenv("CNETLD", CNETLD));
    av[ac++]	= (tmp = strrchr(av[0],'/')) ? tmp+1 : av[0];

    ac		= add_link_args(ac, av);
    av[ac++]	= "-o";
    av[ac++]	= so_file;

#if	defined(USE_MACOSX)
    char	tmpcnet[32];

    strcpy(tmpcnet, "/tmp/cnet-XXXXXX");
    mktemp(tmpcnet);
    strcat(tmpcnet, ".c");

    FILE	*fp = fopen(tmpcnet, "w");

    F(fp, "int %s = 1;\n", CNET_DATA_SYM);
    fclose(fp);
    av[ac++]	= tmpcnet;
#endif

//  ADD THE OBJECT FILES
    ofs		= o_files;
    while(*ofs)
	av[ac++] = *ofs++;

//  ADD ANY -l OR -L LINKER FLAGS
    ofs	 =	LDflags;
    while(*ofs)
	av[ac++] = *ofs++;

//  ADD THE CNET-PROVIDED SUPPORT LIBRARY
    tmp		= find_cnetfile(LIB_CNETSUPPORT_A, true, true);
    sprintf(chararray, "-L%s", tmp);
    av[ac++]	= strdup(chararray);
    av[ac++]	= L_CNETSUPPORT;

#if	!defined(USE_MACOSX)
    av[ac++]	= "-lm";		// add the mathematics library
#endif
    av[ac  ]	= NULL;

    if(!qflag && !vflag)
	REPORT("linking %s\n", so_file);
    sleep(1);				// force *.so to be newer than all *.o

    int status	= fork_exec(ac, av);

#if	defined(UNLINK_SO_LOCATIONS)
    unlink("so_locations");
#endif
#if	defined(USE_MACOSX)
    unlink(tmpcnet);
#endif

    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	unlink(so_file);
	exit(EXIT_FAILURE);
    }
    return 0;
}


// --------------------------------------------------------------------

char *compile_string(char **defines, const char *str, bool linktoo)
{
    static char	*laststr	= NULL;
    static char	**lastdefines	= NULL;
    static char	*so_file	= NULL;

    char	*Cflags[N_ALLOCATED];
    char	*LDflags[N_ALLOCATED];
    char	*o_files[N_ALLOCATED];

    char	**ofs	= o_files;

    char	savech, *dot, *slash;
    char	*word, *cp, *c_files;
    char	*filenm;

    int		nCflags=0, nLDflags=0;

//  IS THIS COMPILATION REQUEST IDENTICAL TO THE PREVIOUS ONE?
    if(lastdefines && lastdefines == defines &&
       laststr && strcmp(str, laststr) == 0)
	return so_file;

//  A NEW REQUEST - FORGET OLD INFORMATION, REMEMBER THIS NEW INFORMATION
    laststr		= strdup(str);
    CHECKALLOC(laststr);
    lastdefines		= defines;
    so_file		= NULL;

    cp 	= c_files	= strdup(str);
    CHECKALLOC(cp);
    ofs	= o_files;

#define	ADD_DEFINE(d)	sprintf(chararray, "-D" #d "=%i", d);		\
			Cflags[nCflags]	= strdup(chararray);		\
			CHECKALLOC(Cflags[nCflags]);			\
			++nCflags

    ADD_DEFINE(CNET_PROVIDES_APPLICATION_LAYER);
    ADD_DEFINE(CNET_PROVIDES_WANS);
    ADD_DEFINE(CNET_PROVIDES_LANS);
    ADD_DEFINE(CNET_PROVIDES_WLANS);
    ADD_DEFINE(CNET_PROVIDES_MOBILITY);
    ADD_DEFINE(CNET_PROVIDES_KEYBOARD);

#undef	ADD_DEFINE

//  ADD ANY -D, -I, or -U DEFINES PROVIDED ON THE COMMAND-LINE
    if(defines)
	while(*defines) {
	    Cflags[nCflags++]	= *defines;
	    ++defines;
	}

    while(*cp) {
//  SKIP LEADING BLANKS
	while(*cp == ' ' || *cp == '\t')
	    ++cp;

	word	= cp;
	while(*cp && *cp != ' ' && *cp != '\t')
	    ++cp;

	if(cp != word) {
	    savech	= *cp;
	    *cp		= '\0';

// FIRSTLY, CHECK FOR ANY COMPILER OR LINKER SWITCHES
	    if(*word == '-') {
		switch (*(word+1)) {
		    case 'l' :
		    case 'L' :	LDflags[nLDflags]	= strdup(word);
				CHECKALLOC(LDflags[nLDflags]);
				++nLDflags;
				break;

		    default  :	Cflags[nCflags]		= strdup(word);
				CHECKALLOC(Cflags[nCflags]);
				++nCflags;
				break;
		}
		*cp	= savech;
		continue;
	    }

// PERMIT OBJECT AND ARCHIVE FILES
	    dot	= strrchr(word,'.');
	    if(dot && (strcmp(dot,".o") == 0 || strcmp(dot,".a") == 0)) {
		LDflags[nLDflags]	= strdup(word);
		CHECKALLOC(LDflags[nLDflags]);
		++nLDflags;
		continue;
	    }

// OTHERWISE ENSURE THAT WE HAVE A C SOURCE FILE
	    if(dot == NULL || strcmp(dot,".c") != 0) {
		WARNING("sourcefile '%s' must be a C file\n", word);
		++nerrors;
		break;
	    }
	    *dot		= '\0';

	    filenm		= (slash = strrchr(word,'/')) ? slash+1 : word;
	    *ofs		= malloc(strlen(filenm) + 4);
	    CHECKALLOC(*ofs);
	    sprintf(*ofs, "%s.o", filenm);
	    *dot		= '.';
	    Cflags[nCflags]	= NULL;

// NEXT, TRY AND COMPILE EACH C FILE
	    if(make_ofile(Cflags, *ofs, word) != 0)
		break;
	    ++ofs;
	    *cp			= savech;
	}
    }
    *ofs		= NULL;
    LDflags[nLDflags]	= NULL;

// FINALLY, LINK TOGETHER ALL OBJECT FILES INTO A SHARED OBJECT FILE
    if(nerrors == 0 && linktoo) {
	cp	= c_files;

	while(*cp) {
	    while(*cp == ' ' || *cp == '\t')		// skip blanks
		++cp;
	    if(*cp != '-')				// found a pathname
		break;
	    while(*cp && *cp != ' ' && *cp != '\t')	// skip switch
		++cp;
	}

	word	= cp;
	while(*cp && *cp != ' ' && *cp != '\t')	// skip to end of pathname
	    ++cp;
	*cp		= '\0';
	filenm 		= (slash = strrchr(word,'/')) ? slash+1 : word;
	dot		= strrchr(filenm,'.');
	*dot		= '\0';

	so_file		= malloc((unsigned)((cp - filenm) + 12));
	CHECKALLOC(so_file);
	sprintf(so_file,"./%s%s",filenm,findenv("CNETFILEEXT",CNETFILEEXT));

	if(make_sofile(LDflags, so_file, o_files) != 0) {
	    ++nerrors;
	    FREE(so_file);
	}
    }

//  FREE ANY DYNAMICALLY ALLOCATED SPACE
    ofs	= o_files;
    while(*ofs) {
	FREE(*ofs);
	ofs++;
    }
    ofs	= LDflags;
    while(*ofs) {
	FREE(*ofs);
	ofs++;
    }
    FREE(c_files);

    return so_file;
}

#endif
