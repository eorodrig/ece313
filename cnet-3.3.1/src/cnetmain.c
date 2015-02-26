#include "cnetprivate.h"
#include "scheduler.h"
#include <getopt.h>
#include <sys/resource.h>

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

static	int		mflag	= 0;		// wall-clock minutes
static	char		*Fflag	= NULL;		// Filename of Tcl/Tk code
static	const char	*Rflag	= DEFAULT_REBOOT_FUNCTION; // rebootfunc name
static	bool		sflag	= false;	// cumulative statistics

#if	CNET_SUPPORTS_IPOD
#define	OPTLIST		"A:BcC:D:dEe:F:f:giI:Jm:NnOo:PpQqRr:S:stTu:vWx:z"
#else
#define	OPTLIST		"A:BcC:D:dEe:F:f:giI:Jm:NnOo:pQqRr:S:stTu:vWx:z"
#endif

static void usage(int status)
{
    F(stderr,
	"Usage: %s [options] {TOPOLOGYFILE | - | -r nnodes} [args-to-%s]\n",
		argv0, Rflag);
    F(stderr, "options are:\n");

    F(stderr,
    "  -A str\tprovide the Application Layer compilation string\n");
    F(stderr,
    "  -B\t\tdisable frame/packet buffering in NICs\n");
    F(stderr,
    "  -c\t\tclocks in all nodes are synchronized\n");
    F(stderr,
    "  -C str\tprovide the Central Layers' compilation string\n");
    F(stderr,
    "  -d\t\tprint some debugging information to stderr\n");
    F(stderr,
    "  -D token\tdefine C-preprocessor tokens for compilation\n");
    F(stderr,
    "  -e period\texecute for the indicated period\n");
    F(stderr,
    "  -E\t\treport corrupt frame arrival with ER_CORRUPTFRAME\n");
    F(stderr,
    "  -f period\tset the period of some periodic activities\n");
    F(stderr,
    "  -F filename\tprovide the filename of the Tcl/Tk code\n");
    F(stderr,
    "  -g\t\tgo - commence execution immediately\n");
    F(stderr,
    "  -i\t\tprint instantaneous non-zero statistics to stdout\n");
    F(stderr,
    "  -I directory\tprovide a C-preprocessor include path\n");
    F(stderr,
    "  -J file[s]\tjust compile one or more C files\n");
    F(stderr,
    "  -m mins\trun for the indicated length of wall-clock time\n");
    F(stderr,
    "  -n\t\tcompile and link the protocols, but do not run\n");
    F(stderr,
    "  -N\t\treveal the number of nodes in the variable NNODES\n");
    F(stderr,
    "  -o filename\tprovide a filename into which each node's printfs are mirrored\n");
    F(stderr,
    "  -O\t\topen each node's window on startup\n");
    F(stderr,
    "  -p\t\tread, check, and print the topology, then exit\n");
#if	CNET_SUPPORTS_IPOD
    F(stderr,
    "  -P file[s]\tcompile one or more C files (%s) for the iPod platform\n",
#if	defined(CNET4IPODHOST) && defined(CNET4IPODPORT)
	"remotely"
#else
	"locally"
#endif
	);
#endif
    F(stderr,
    "  -q\t\trequest quiet execution, except during EV_DEBUGs\n");
    F(stderr,
    "  -Q\t\tignore Application Layer message sequencing errors\n");
    F(stderr,
    "  -r N\t\trequest a random, connected, topology of N nodes\n");
    F(stderr,
    "  -R func\tname the function to reboot each node (default is %s)\n",
		DEFAULT_REBOOT_FUNCTION);
    F(stderr,
    "  -s\t\tprint cumulative non-zero statistics to stdout\n");
    F(stderr,
    "  -S seed\tprovide a positive starting random seed\n");
    F(stderr,
    "  -t\t\ttrace all cnet events and calls to cnet functions\n");
    F(stderr,
    "  -T\t\tuse (fast) discrete-event simulation\n");
    F(stderr,
    "  -U token\tundefine C-preprocessor tokens for compilation\n");
    F(stderr,
    "  -u period\tspecify the update period for GUI windows\n");
    F(stderr,
    "  -v\t\tprint version (%s) and verbose debugging to stderr\n",
			CNET_VERSION);
    F(stderr,
    "  -W\t\tdo not use the windowing interface\n");
    F(stderr,
    "  -x str\tprovide the compilation string of a code extension\n");
    F(stderr,
    "  -z\t\tprint all statistics to stdout, even if zero\n");

    vflag	= 0;
    F(stderr, "\nUsing the C header file %s\n",
			find_cnetfile("cnet.h", false, true));
    F(stderr, "Protocols will be compiled with %s\n",
			findenv("CNETCC", CNETCC));
    F(stderr, "Protocols will be linked with %s\n",
			findenv("CNETLD", CNETLD));
#if	defined(USE_TCLTK)
    if(Wflag)
	F(stderr, "Using the Tcl/Tk file %s\n",
		find_cnetfile(Fflag ? Fflag : findenv("CNETTCLTK", CNETTCLTK),
			      false, true));
#endif

    F(stderr, "\nPlease report any bugs to %s\n", AUTHOR_EMAIL);
    exit(status);
}


void CLEANUP(int status)
{
    if(nerrors)
	F(stderr,"\n%i error%s found.\n", nerrors, PLURAL(nerrors));

    if(gattr.tfp)				// close trace-file if open
	fclose(gattr.tfp);

    if(status == 0) {
	extern void	invoke_shutdown(int node);
	extern void	flush_allstats(bool);

	for(int n=0 ; n<_NNODES ; ++n)
	    invoke_shutdown(n);
	if(sflag)
	    flush_allstats(true);
    }
    exit(status);
}


// -------------------------- Overtime errors --------------------------


static void signal_catcher(int sig)
{
    if(sig == SIGALRM || sig == SIGXCPU) {
	F(stderr, "%s: %s limit exceeded\n(see %s)\n",
			argv0,
			sig == SIGALRM ? "time" : "simulation execution",
			WWW_FAQ);
	CLEANUP(0);
    }

    F(stderr,"%s: caught signal number %i", argv0, sig);
    if(THISNODE >= 0)			// only if we've been running
	F(stderr," while (last) handling %s.%s\n",
			NP->nodename, cnet_evname[(int)HANDLING]);
    else
	fputc('\n',stderr);

    if(sig == SIGINT)
	CLEANUP(0);

    if(sig == SIGBUS || sig == SIGSEGV)
	F(stderr, "(see %s)\n", WWW_FAQ);

    if(malloc(1024) == NULL)
	F(stderr,"%s: Out of memory!\n",argv0);
    _exit(EXIT_FAILURE);
}


#if	defined(USE_TCLTK)
static void tcl_game_over(ClientData client_data)
{
    signal_catcher(SIGALRM);
}
#endif


static void init_traps(void)
{
    if(mflag > 0) {
#if	defined(USE_TCLTK)
	if(Wflag)
	    Tcl_CreateTimerHandler(mflag * 60000,	// yes, in millisecs
				(Tcl_TimerProc *)tcl_game_over, (ClientData)0);
	else
#endif
	{
	    signal(SIGALRM,	signal_catcher);
	    alarm((unsigned int)(mflag*60));	// seconds
	}
    }

    signal(SIGBUS,	signal_catcher);
    signal(SIGQUIT,	signal_catcher);

    signal(SIGINT,	signal_catcher);
    signal(SIGSEGV,	signal_catcher);
    signal(SIGILL,	signal_catcher);
    signal(SIGFPE,	signal_catcher);
}

// ----------------------------------------------------------------

int main(int argc, char **argv)
{
    extern void init_globals(bool, const char *, bool, const char *, bool);
    extern void	TCLTK_init(void);
    extern void	init_application_layer(char *, bool, int);
    extern void	init_physical_layer(bool, bool, int);
    extern void	init_stats_layer(bool, bool);
    extern void	init_stdio_layer(char *);
    extern void	init_trace(void);

    extern void check_topology(int, int, int, char **);
    extern void compile_topology(char **);
    extern void motd(void);
    extern void parse_topology(char *, char **);
    extern void save_topology(const char *);
    extern void random_topology(const char *, int);
    extern void	schedule(bool single);

    int		opt;
    int		ndefines = 0;
    char	*defines[64] = { NULL };// hoping that this is enough
    char	*topfile= NULL;		// topology filename

    const char	*Cflag	= DEFAULT_COMPILE_STRING;  // protocol source files

    char	*Aflag	= NULL,		// application layer file string
		*eflag	= NULL,		// period/length of execution
		*fflag	= NULL,		// period of reporting
		*oflag	= NULL,		// prefix of output filenames
		*rflag	= NULL,		// random topology
		*uflag	= UPDATE_PERIOD;// GUI update period

    bool	Bflag	= true,		// NICs are buffered
    		cflag	= false,	// synchronize time_of_day clocks
    		Eflag	= REPORT_PHYSICAL_CORRUPTION,
    		gflag	= false,	// start execution (go) immediately
		iflag	= false,	// display instantaneous statistics
		Jflag	= false,	// just compile *.c -> *.o
		Nflag	= false,	// divulge NNODES
		nflag	= false,	// compile, link, exit
		Oflag	= false,	// open all node windows
		pflag	= false,	// display topology, exit
#if	CNET_SUPPORTS_IPOD
		Pflag	= false,	// compile for the iPod platform
#endif
		Qflag	= false,	// ignore AL sequencing errors
		Tflag	= false,	// schedule as fast as possible
		tflag	= false,	// trace event handlers
		zflag	= false;	// display zero statistics

    int		Sflag	= 0;		// random seed
    {
	struct timeval NOW;
 
	gettimeofday(&NOW, NULL);
	Sflag	= (getpid() << 16) ^ NOW.tv_sec ^ NOW.tv_usec;
    }

    argv0		= (argv0 = strrchr(argv[0],'/')) ? argv0+1 : argv[0];

    opterr	= 0;
    while((opt = getopt(argc, argv, OPTLIST)) != -1) {
	switch (opt) {

// USE MY APPLICATION LAYER
	case 'A' :  Aflag = strdup(optarg);
		    CHECKALLOC(Aflag);
		    break;

// DISABLE FRAME/PACKET BUFFERING IN NICs
	case 'B' :  Bflag = !Bflag;
		    break;

// KEEP CLOCKS SYNCHRONIZED
	case 'c' :  cflag = !cflag;
		    break;

// COMPILE THIS PROTOCOL SOURCE STRING
	case 'C' :  Cflag = strdup(optarg);
		    CHECKALLOC(Cflag);
		    break;

// DEBUG PRINTING TO STDERR
	case 'd' :  dflag = !dflag;
		    break;

// ACCEPT SOME CPP SWITCHES
	case 'D' :
	case 'U' :
	case 'I' :  defines[ndefines] = strdup(argv[optind-1]);
		    CHECKALLOC(defines[ndefines]);
		    ++ndefines;
		    break;

// EXECUTE FOR A SPECIFIC PERIOD
	case 'e' :  if(!isdigit(*optarg)) {
			F(stderr,"%s: invalid period for -e '%s'\n",
					argv0, optarg);
			argc	= 0;
		    }
		    else {
			eflag = strdup(optarg);
			CHECKALLOC(eflag);
		    }
		    break;

// REPORT OPTION_CHECKSUM ERRORS WITH ER_CORRUPTFRAME
	case 'E' :  Eflag = !Eflag;
		    break;

// PROVIDE TCL/TK FILENAME
	case 'F' :  Fflag = strdup(optarg);
		    CHECKALLOC(Fflag);
		    break;

// PERIOD OF SCHEDULER REPORTING
	case 'f' :  if(!isdigit(*optarg)) {
			F(stderr,"%s: invalid period for -f '%s'\n",
					argv0, optarg);
			argc	= 0;
		    }
		    else {
			fflag = strdup(optarg);
			CHECKALLOC(fflag);
		    }
		    break;

// COMMENCE EXECUTION (go) IMMEDIATELY
	case 'g' :  gflag = !gflag;
		    break;

// PRINT INSTANTANEOUS EXECUTION STATISTICS
	case 'i' :  iflag = !iflag;
		    break;

// JUST COMPILE INDIVIDUAL C FILES INTO OBJECT FILES
	case 'J' :  Jflag = !Jflag;
		    break;

// RUN FOR mflag MINUTES OF WALL-CLOCK TIME
	case 'm' :  mflag	= atoi(optarg);
		    if(mflag < 0) {
			F(stderr,"%s : invalid # of minutes\n",argv0);
			argc = 0;
		    }
		    break;

// COMPILE AND LINK but DO NOT RUN
	case 'n' :  nflag = !nflag;
		    break;

// REVEAL THE NUMBER OF NNODES TO STUDENTS' PROTOCOLS
	case 'N' :  Nflag = !Nflag;
		    break;

// OUTPUT FILENAME
	case 'o' :  oflag = strdup(optarg);
		    CHECKALLOC(oflag);
		    break;

//  OPEN ALL NODE WINDOWS
	case 'O' :  Oflag = !Oflag;
		    break;

//  PRINT TOPOLOGY
	case 'p' :  pflag = !pflag;
		    break;

#if	CNET_SUPPORTS_IPOD
//  COMPILE FOR THE iPod PLATFORM
	case 'P' :  Pflag = !Pflag;
		    goto args_done;
		    break;
#endif

// QUIET EXECUTION (EXCEPT DURING EV_DEBUGs)
	case 'q' :  qflag = !qflag;
		    break;

// IGNORE APPLICATION LAYER SEQUENCING ERRORS
	case 'Q' :  Qflag = !Qflag;
		    break;

// RANDOM TOPOLOGY - ALL FOLLOWING ARGUMENTS ARE PASSED TO reboot_node()
	case 'r' :  rflag = strdup(optarg);
		    CHECKALLOC(rflag);
		    goto args_done;
		    break;

// PROVIDE reboot_func() FUNCTION NAME
	case 'R' :  Rflag = strdup(optarg);
		    CHECKALLOC(Rflag);
		    break;

// PRINT CUMULATIVE EXECUTION STATISTICS
	case 's' :  sflag = !sflag;
		    break;

// PROVIDE RANDOM SEED
	case 'S' :  if(!isdigit(*optarg)) {
			F(stderr,"%s: invalid random seed '%s'\n",
					argv0, optarg);
			argc	= 0;
		    }
		    else
			gattr.Sflag	=
			Sflag		= (unsigned long)atoi(optarg);
		    break;

// TRACE EVENTS
	case 't' :  tflag = !tflag;
		    break;

// TOGGLE USE OF (FAST) DISCRETE-EVENT SIMULATION
	case 'T' :  Tflag = !Tflag;
		    break;

// SPECIFY THE PERIOD OF GUI WINDOW UPDATES 
	case 'u' :  if(!isdigit(*optarg)) {
			F(stderr,"%s: invalid period for -u '%s'\n",
					argv0, optarg);
			argc	= 0;
		    }
		    else {
			uflag = strdup(optarg);
			CHECKALLOC(uflag);
		    }
		    break;

// VERBOSE DEBUG PRINTING TO STDERR
	case 'v' :  ++vflag;
		    break;

// TOGGLE USE OF THE WINDOWING ENVIRONMENT
	case 'W' :  Wflag = false;
		    break;

// ACCEPT MULTIPLE CODE EXTENSIONS
	case 'x' :  if(gattr.nextensions < MAXEXTENSIONS) {
			gattr.extensions[gattr.nextensions] = strdup(optarg);
			CHECKALLOC(gattr.extensions[gattr.nextensions]);
			++gattr.nextensions;
		    }
		    break;

// DISPLAY STATISTICS EVEN IF ZERO
	case 'z' :  zflag = !zflag;
		    break;

	default :   F(stderr,"%s : illegal option -%c\n", argv0,optopt);
		    argc = 0;
		    break;
	}
    }

    if(argc <= 0 || optind == argc)
	usage(1);

args_done:
    defines[ndefines]	= NULL;

    argc	-= optind;
    argv	+= optind;

    if(vflag) {
	dflag	= true;
	REPORT("%s\n", CNET_VERSION);
    }
    if(zflag)
	sflag	= true;

#if	CNET_SUPPORTS_IPOD
//  JUST COMPILE FOR THE iPod PLATFORM?
    if(Pflag) {
	extern int  compile4ipod(char **defines, int argc, char **argv);

	if(argc == 0)
	    usage(1);

	return compile4ipod(defines, argc, argv);
    }
#endif

// JUST COMPILE INDIVIDUAL C FILES INTO OBJECT FILES
    if(Jflag) {
	extern void	just_compile(char **defines, int argc, char **argv);
	extern char	*compile_string(char **, const char *, bool);

	if(argc == 0)
	    usage(1);
	for(int a=0 ; a<argc ; ++a)
	    compile_string(defines, argv[a], false);
	return EXIT_SUCCESS;
    }

    if(dflag)
	REPORT("seed %i\n", Sflag);

#if	defined(USE_TCLTK) && !defined(USE_MACOSX)
    if(Wflag && findenv("DISPLAY", NULL) == NULL) {
	WARNING("DISPLAY variable not defined (setting -W option)\n");
	Wflag	= false;
    }
#endif

    motd();
    init_globals(Bflag, Cflag, Oflag, Rflag, tflag);

    if(rflag)
	random_topology(rflag, Sflag);
    else {
	char	*dot;

	if((dot=strrchr(argv[0],'.')) && strcmp(dot,".c") == 0) {
	    F(stderr,
		"%s: hmmm, '%s' looks like a C file, not a topology file.\n",
				    argv0, argv[0]);
	    CLEANUP(1);
	}

	topfile	= strdup(argv[0]);
	CHECKALLOC(topfile);

	parse_topology(topfile, defines);
    }

#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK_init();		// now, to determine the mapsize
#endif

    check_topology(cflag, Sflag, argc, &argv[0]);
    if(pflag) {
	save_topology(NULL);
	exit(EXIT_SUCCESS);
    }

    compile_topology(defines);
    if(nflag)
	exit(EXIT_SUCCESS);

    if(eflag == NULL) {
	eflag	= SIMULATION_LENGTH;
	if(mflag == 0)
	    mflag	= DEFAULT_mflag_MINUTES;
    }

    init_application_layer(Aflag, Qflag, Sflag);
    init_physical_layer(Eflag, Nflag, Sflag);
    init_stats_layer(iflag, zflag);
    init_stdio_layer(oflag);
    init_trace();
    init_scheduler(eflag, fflag, Nflag, Sflag, sflag, Tflag, uflag);

#if	defined(USE_TCLTK)
    if(Wflag) {
	extern void	init_mainwindow(const char *,
					const char *, bool, bool, bool);
	extern void	init_statswindows(void);
	extern void	init_nodewindow(int);

	int	n;
	char	winname[64];

	if(rflag)
	    sprintf(winname, "random(%s)", rflag);
	init_mainwindow(rflag ? winname : topfile, Fflag, gflag, Tflag, tflag);
	init_statswindows();

	for(n=DEFAULT ; n<_NNODES ; ++n)
	    init_nodewindow(n);

	if(gflag) {
	    init_traps();
	    cnet_state = STATE_RUNNING;
	}
	else
	    cnet_state = STATE_PAUSED;

	while(cnet_state != STATE_UNKNOWN) {
	    switch (cnet_state) {

	    case STATE_PAUSED: {
		static int traps_set = false;

		tcltk_notify_start();
		if(traps_set == false) {
		    init_traps();
		    traps_set	= true;
		}
		break;
	    }
	    case STATE_RUNNING:
		schedule(false);
		break;
	    case STATE_SINGLESTEP: {
		cnet_state = STATE_RUNNING;
		schedule(true);
		cnet_state = STATE_PAUSED;
		break;
	    }
	    case STATE_GAMEOVER:
		signal_catcher(SIGXCPU);	// -e period has elapsed
		break;
	    default :
		cnet_state = STATE_UNKNOWN;
		break;
	    }
	}
    }
    else
#endif
    {
	extern	int	fileno(FILE *fp);

	init_traps();
	if(dflag && !qflag && isatty(fileno(stdout)))
	    REPORT("running\n");
	cnet_state = STATE_RUNNING;
	schedule(false);
    }
    CLEANUP(0);
    return EXIT_SUCCESS;
}
