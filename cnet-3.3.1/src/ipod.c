#include "cnetprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	CNET_SUPPORTS_IPOD

#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define	TEMPLATE	"/tmp/c4i-XXXXXX"
#define	C4I_ARGS	"args"

extern	int	kill(pid_t pid, int sig);

//  ---------------------------------------------------------------------

static void rmrf(const char *dirname)
{
    DIR		*dirp = opendir(dirname);

//  IF WE CAN OPEN THE DIRECTORY....
    if(dirp) {
	char	fullpath[MAXPATHLEN], *fn = fullpath;

	strcpy(fn, dirname);
	while(*fn)
	    ++fn;
	if(*(fn-1) != '/') {
	    *fn++	= '/';
	    *fn		= '\0';
	}
//  FOREACH ITEM IN THE DIRECTORY....
	for(struct dirent *dp = readdir(dirp); dp ; dp = readdir(dirp)) {
	    struct stat     sbuf;

	    strcpy(fn, dp->d_name);
//  IF IT'S A DIRECTORY, RECURSIVELY REMOVE IT
	    if(stat(fullpath, &sbuf) == 0 && S_ISDIR(sbuf.st_mode)) {
		if(dp->d_name[0] != '.')
		    rmrf(fullpath);
	    }
//  OTHERWISE, TRY TO UNLINK THE FILE
	    else
		unlink(fullpath);
	}
	closedir(dirp);
    }
//  REMOVE THE DIRECTORY
    rmdir(dirname);
}

static int catfile(FILE *outfp, const char *filenm)
{
    FILE	*fp = fopen(filenm, "r");

    if(fp) {
	char	line[BUFSIZ];
	int	nerrs = 0;

	while(fgets(line, sizeof(line), fp)) {
	    if(strstr(line, "-force_cpusubtype_ALL") == NULL) {
		fputs(line, outfp);
		++nerrs;
	    }
	}
	fclose(fp);
	return nerrs;
    }
    return 0;
}

static int check_bundle(time_t NOW)
{
    DIR		*dirp = opendir(".");
    int		result	= 1;

//  IF WE CAN OPEN THE DIRECTORY....
    if(dirp) {
	struct stat     sbuf;
	char		bundle[MAXPATHLEN] = { 0 };
	time_t		newest	= 0;

//  FOREACH ITEM IN THE CURRENT DIRECTORY...
	for(struct dirent *dp = readdir(dirp); dp ; dp = readdir(dirp)) {
	    if(dp->d_name[0] == '.')
		continue;

//  HAVE WE FOUND A RECENT DIRECTORY?
	    if(stat(dp->d_name, &sbuf) == 0 && S_ISDIR(sbuf.st_mode))
		if(newest < sbuf.st_mtime) {
		    newest	= sbuf.st_mtime;
		    strcpy(bundle, dp->d_name);
		}
	}
	closedir(dirp);

//  OK, DID WE FIND A RECENT BUNDLE?
	if(newest >= NOW) {
	    char	pathname[MAXPATHLEN];
	    int		nerrs	= 0;

//  DISPLAY AND UNLINK THE .stderr FILE
	    sprintf(pathname, "%s/.stderr", bundle);
	    if(catfile(stderr, pathname) != 0)
		++nerrs;
	    unlink(pathname);

//  DISPLAY AND UNLINK THE .stdout FILE
	    sprintf(pathname, "%s/.stdout", bundle);
	    if(catfile(stdout, pathname) != 0)
		++nerrs;
	    unlink(pathname);

//  IF WE HAVE ANY ERRORS, REMOVE THE .app DIRECTORY
	    if(nerrs)
		rmrf(bundle);
	    else
		result	= 0;
	}
    }
    return result;
}

//  ---------------------------------------------------------------------

static int send2remote(int sd, const char *tarfile, int *nb)
{
    struct stat	sbuf;
    int		nbytes	= 0;
    int		result	= 1;
    int		w;

    *nb	= 0;
    if(stat(tarfile, &sbuf) == 0) {
	int	fd	= open(tarfile, O_RDONLY, 0);

	nbytes	= htonl(sbuf.st_size);
	if(write(sd, &nbytes, sizeof(nbytes)) == sizeof(nbytes)) {
	    char	buf[BUFSIZ];
	    int		got;

	    while((got = read(fd, buf, sizeof(buf))) > 0) {
//  THIS FOOLISHNESS HUSHES UBUNTU, WHICH SETS -Werror=unused-but-set-variable
		w = write(sd, buf, got);
		w += 0;
	    }
	    if(got == 0)
		result	= 0;
	}
	close(fd);
	*nb	= sbuf.st_size;
    }
    return result;
}

static int receive_files(int sd, int *nb)
{
    int		pid, status;
    int		p[2];
    int		nbytes	= 0;
    int		result	= 1;

    *nb	= 0;
    if(pipe(p) != 0)
	return 1;

    switch (pid = fork()) {
    case -1 :
	close(p[0]);
	close(p[1]);
	break;

//  CHILD PROCESS RUNS tar TO RECEIVE TARFILE
    case 0 : {
	char	*av[8];
	int	ac = 0;
	char	*tmp;
	int	devnull;

	av[ac]	 = strdup(findenv("CNETTAR", CNETTAR));
	CHECKALLOC(av[ac]);
	++ac;
	
	av[ac++] = (tmp = strrchr(av[0],'/')) ? tmp+1 : av[0];
	av[ac++] = "zxpf";
	av[ac++] = "-";
	av[ac  ] = NULL;

//  tar RECEIVES ITS INPUT VIA THE PIPE
	dup2(p[0], 0);
	close(p[0]);
	close(p[1]);

//  REDIRECT tar's OUTPUT TO /dev/null
        devnull      = open("/dev/null", O_WRONLY, 0);
        dup2(devnull, 1);
        dup2(devnull, 2);
        close(devnull);

	execvp(av[0], &av[1]);
	exit(EXIT_FAILURE);
        break;
    }
    default : {
	char	buf[BUFSIZ];
	int	got, w;

	close(p[0]);
//  RECEIVE THE TARFILE OVER SOCKET, SEND TO tar PROCESS OVER PIPE
	while((got = read(sd, buf, sizeof(buf))) > 0) {
//  THIS FOOLISHNESS HUSHES UBUNTU, WHICH SETS -Werror=unused-but-set-variable
	    w = write(p[1], buf, got);
	    w += 0;
	    nbytes	+= got;
	}
	close(p[1]);
	*nb	= nbytes;

//  WAIT FOR THE tar PROCESS TO TERMINATE
	while(wait(&status) != pid)
	    ;
	if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
	    result = 0;
	kill(pid, SIGKILL);
	break;
    }
    }
    return result;
}

//  ---------------------------------------------------------------------

//  ESTABLISH A TCP CONNECTION TO THE cnet4ipod SERVER
static int tcp_connect(const char *hostname, int port)
{
    struct hostent	*he;
    int			sd;

    if((he = gethostbyname(hostname)) == NULL)
	return -1;

    if((sd = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
	struct sockaddr_in	addr;
	uint32_t		ipaddr;		// always in network order

	memset(&addr, 0, sizeof(addr));
	memcpy(&ipaddr, (char *)he->h_addr_list[0], sizeof(ipaddr));

	addr.sin_family		= AF_INET;
	addr.sin_addr.s_addr	= ipaddr;	// already in network order
	addr.sin_port		= htons(port);

//  AND ATTEMPT TO CONNECT TO THE REMOTE HOST
	if(connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	    close(sd);
	    sd = -1;
	}
    }
    return sd;
}

static int remote_compile4ipod(const char *aout,
				char **defines, int argc, char **argv)
{
    char	tarfile[32];
    int		result = 1;

    sprintf(tarfile, "%s.tgz", TEMPLATE);
#if	defined(USE_MACOSX)
    close( mkstemps(tarfile, 4) );
#else
    extern int	mkstemp(char *template);

    close( mkstemp(tarfile) );
#endif

//  BUILD A TAR ARCHIVE OF ALL FILES TO BE SENT
    int		pid, status;

    switch (pid = fork()) {
    case -1 :
	FATAL("cannot fork\n");
	break;

    case 0 : {
	char	*av[128];
	int	ac = 0;
	char	*tmp;

//  CREATE THE C4I_ARGS FILE
	FILE	*fp = fopen(C4I_ARGS, "w");
	if(fp == NULL) {
	    F(stderr, "%s: cannot create %s\n", argv0, C4I_ARGS);
	    exit(EXIT_FAILURE);
	}

//  WRITE THE USER and APPLICATION'S NAME TO THE ARGS FILE
	F(fp, "%s\n%s\n", getlogin(), aout);

//  WRITE ALL (ANY) DEFINES TO THE ARGS FILE
	for(int d=0 ; defines[d] ; ++d)
	    F(fp, "%s\n", defines[d]);

//  WRITE ALL C FILENAMES TO THE ARGS FILE
	for(int a=0 ; a<argc ; ++a)
	    F(fp, "%s\n", argv[a]);

	fclose(fp);

//  BUILD THE ARGUMENTS FOR tar
	av[ac]	 = strdup(findenv("CNETTAR", CNETTAR));
	CHECKALLOC(av[ac]);
	++ac;

	av[ac++] = (tmp = strrchr(av[0],'/')) ? tmp+1 : av[0];
	av[ac++] = "zcf";
	av[ac++] = tarfile;

	av[ac++] = C4I_ARGS;

//  ADD THE ACTUAL C FILES
	for(int a=0 ; a<argc ; ++a)
	    av[ac++] =	argv[a];

	av[ac  ] =	NULL;

	execvp(av[0], &av[1]);
	FATAL("cannot exec %s\n", av[0]);
	exit(EXIT_FAILURE);
        break;
    }
//  WAIT FOR THE tar PROCESS TO COMPLETE
    default :
	while(wait(&status) != pid)
	    ;
	if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
	    result = 0;
	kill(pid, SIGKILL);
	break;
    }

//  TAR FILE BUILT SUCCESSFULLY
    if(result == 0) {
	const char	*host = findenv("CNET4IPOD_HOST",CNET4IPOD_HOST);
	int		port  = atoi(findenv("CNET4IPOD_PORT",CNET4IPOD_PORT));
	int		sd;

	if(port <= 0)
	    port	= atoi(CNET4IPOD_PORT);

	sd		= tcp_connect(host, port);
	if(sd < 0) {
	    F(stderr, "%s: cannot connect to cnetipodd (%s:%i)\n",
					argv0, host, port);
	    exit(EXIT_FAILURE);
	}
	else {
	    int		nbytes;
	    time_t	NOW = time(NULL);	// remember start time

	    result	= send2remote(sd, tarfile, &nbytes);
	    if(vflag) {
		if(result == 0)
		    F(stderr, "sent %i bytes to %s:%i\n",
						nbytes, host, port);
		else
		    F(stderr, "failed to send files\n");
	    }
	    if(result == 0) {
		result = receive_files(sd, &nbytes);
		if(vflag)
		    F(stderr, "received %i bytes from %s:%i\n",
						nbytes, host, port);
	    }
	    shutdown(sd, SHUT_RDWR);
	    close(sd);

	    if(result == 0)
		result = check_bundle(NOW);	// find & check newest bundle
	}
    }
//  CLEANUP
    unlink(C4I_ARGS);
    unlink(tarfile);
    return result;
}

//  ---------------------------------------------------------------------

int compile4ipod(char **defines, int argc, char **argv)
{
    char	*aout, *tmp;
    int		result	= 1;

//  CHECK ALL PRESENTED C SOURCE FILES
    for(int a=0 ; a<argc ; ++a) {
	struct stat	sbuf;
	char		*dot;

//  ENSURE THAT ARGUMENTS IS A READABLE FILE
	if(stat(argv[a], &sbuf) != 0 || !S_ISREG(sbuf.st_mode)) {
	    F(stderr, "%s: cannot read file %s\n", argv0, argv[a]);
	    return result;
	}

//  ENSURE THAT THE FILE IS IN THE CURRENT DIRECTORY
	if(argv[a][0]=='-' || argv[a][0]=='.' || strchr(argv[a], '/') != NULL) {
	    F(stderr, "%s: '%s' is not a relative C file\n",
				argv0, argv[a]);
	    return result;
	}
//  ENSURE THAT WE HAVE A C SOURCE FILE
	dot	= strrchr(argv[a], '.');
	if(dot == NULL || (strcmp(dot, ".c") != 0 && strcmp(dot, ".h") != 0)) {
	    F(stderr, "%s: '%s' is not a relative C file\n",
				argv0, argv[a]);
	    return result;
	}
    }

//  FIND THE APPLICATION'S NAME
    tmp		= strchr(argv[0], '.');
    *tmp	= '\0';
    aout	= strdup(argv[0]);
    CHECKALLOC(aout);
    *tmp	= '.';

    if(vflag)
	F(stderr, "using the remote iPod cross-compiler\n");
    result	= remote_compile4ipod(aout, defines, argc, argv);

    if(result == 0)
	F(stdout, "iPod application is named %s.app\n", aout);
    free(aout);

    return result;
}

#endif	// CNET_SUPPORTS_IPOD

//  vim: ts=8 sw=4
