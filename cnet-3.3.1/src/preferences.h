
// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

/*  The string constant CNETPATH should provide a colon-delimited list
    of directory names where cnet will search for <cnet.h> at run-time.
    This value will be the default, but any user can over-ride it by
    having their own $CNETPATH environment variable.
    CNETPATH must be defined.
 */

#define CNETPATH        ".:/usr/local/lib/cnet:/cslinux/adhoc/lib/cnet"

/*  Next, indicate the type of interface we want.  The only choices are:
	USE_ASCII	or
	USE_TCLTK
 */

#define	USE_TCLTK

/*  The following preprocessor tests should automatically determine
    your operating system.  You should define constants, such as the
    full pathname of the C compiler, for your system.
 */

#if	defined(__linux__) && defined(__ELF__)
#define	USE_LINUX		1
#define	OS_DEFINE		"-DLINUX"
#define	CNETCPP			"/usr/bin/gcc"
#define	CNETCC			"/usr/bin/gcc"
#define	CNETLD			"/usr/bin/gcc"
#define	CNETTAR			"/bin/tar"
#define	LIB_CNETSUPPORT_A	"libcnetsupport-linux.a"
#define	L_CNETSUPPORT		"-lcnetsupport-linux"

#elif TARGET_IPHONE_SIMULATOR  || (defined(__APPLE__) && defined(__arm__))
#define	USE_IPOD		1
#define	CNET4IPOD		1
#define	OS_DEFINE		"-DCNET4IPOD"

#elif	defined(__APPLE__) && defined(__MACH__)
#define	USE_MACOSX		1
#define	OS_DEFINE		"-DMACOSX"
#define	CNETCPP			"/usr/bin/cpp"
#define	CNETCC			"/usr/bin/gcc"
#define	CNETLD			"/usr/bin/gcc"
#define	CNETTAR			"/usr/bin/tar"
#define	LIB_CNETSUPPORT_A	"libcnetsupport-osx.a"
#define	L_CNETSUPPORT		"-lcnetsupport-osx"

#elif  defined(__FreeBSD__)
#define	USE_FREEBSD		1
#define	OS_DEFINE		"-DFREEBSD"
#define	CNETCPP			"/usr/bin/cpp"
#define	CNETCC			"/usr/bin/gcc"
#define	CNETLD			"/usr/bin/ld"
#define	CNETTAR			"/usr/bin/tar"

#elif  defined(__NetBSD__)
#define	USE_NETBSD		1
#define	OS_DEFINE		"-DNETBSD"
#define	CNETCPP			"/usr/bin/cpp"
#define	CNETCC			"/usr/bin/cc"
#define	CNETLD			"/usr/bin/ld"
#define	CNETTAR			"/usr/bin/tar"
 
#elif	defined(sun) || defined(__sun__)
#if	defined(SVR4) || defined(__svr4__)
#define	USE_SOLARIS		1
#define	OS_DEFINE		"-DSOLARIS"
#define	CNETCPP			"/usr/ccs/lib/cpp"
#define	CNETCC			"/usr/ucb/cc"
#define	CNETLD			"/usr/ccs/bin/ld"
#define	CNETTAR			"/usr/bin/tar"

#else
#define	USE_SUNOS		1
#define	OS_DEFINE		"-DSUNOS"
#define	CNETCPP			"/lib/cpp"
#define	CNETCC			"/bin/cc"
#define	CNETLD			"/bin/ld"
#define	CNETTAR			"/bin/tar"
#endif

#elif	defined(__osf__) && (defined(__alpha__) || defined(__alpha))
#define	USE_OSF1		1
#define	OS_DEFINE		"-DOSF1"
#define	CNETCPP			"/bin/cpp"
#define	CNETCC			"/bin/cc"
#define	CNETLD			"/bin/ld"
#define	CNETTAR			"/bin/tar"

#elif	defined(sgi) || defined(__sgi) || defined(__sgi__)
#define	USE_IRIX5		1
#define	OS_DEFINE		"-DIRIX5"
#define	CNETCPP			"/usr/lib/cpp"
#define	CNETCC			"/bin/cc"
#define	CNETLD			"/bin/ld"
#define	CNETTAR			"/bin/tar"

#else
#error	Could not determine your operating system in preferences.h
#endif

#if	!defined(LIB_CNETSUPPORT_A)
#define	LIB_CNETSUPPORT_A	"libcnetsupport.a"
#define	L_CNETSUPPORT		"-lcnetsupport"
#endif

/*  A number of cnet's features may be selectively supported or omitted.
    Most people will probably just want all features, but cnet's code
    explicitly isolates the features so that the iPod implementation
    can add its support for these features over time.
    To include a particular feature, #define its constant to 1.
    You may see the effect of setting each constant to 1 in <cnet.h>
 */

#if	defined(USE_IPOD)
#define	CNET_PROVIDES_APPLICATION_LAYER		1
#define	CNET_PROVIDES_KEYBOARD			1
#define	CNET_PROVIDES_WANS			0
#define	CNET_PROVIDES_LANS			0
#define	CNET_PROVIDES_WLANS			1
#define	CNET_PROVIDES_MOBILITY			0

#else
#define	CNET_PROVIDES_APPLICATION_LAYER		1
#define	CNET_PROVIDES_KEYBOARD			1
#define	CNET_PROVIDES_WANS			1
#define	CNET_PROVIDES_LANS			1
#define	CNET_PROVIDES_WLANS			1
#define	CNET_PROVIDES_MOBILITY			1
#endif

/*  cnet may be used to cross-compile its protocols for execution
    on the iPhone or iPod Touch mobile platform.  The cross-compilation
    is performed by communicating with a remote cnet4ipodd cross-compilation
    server.  To provide iPod support, set CNET_SUPPORTS_IPOD to 1
    (otherwise 0) and set both CNET4IPOD_HOST and CNET4IPOD_PORT to
    identify the server.
    Note:  these default settings are only accessible within CSSE at UWA.
 */

#define	CNET_SUPPORTS_IPOD			0
#define	CNET4IPOD_HOST				"130.95.1.70"
#define	CNET4IPOD_PORT				"2380"

#if	defined(USE_IPOD)
#undef	USE_TCLTK
#define	USE_ASCII
#endif

/*  When compiled for Tcl/Tk, the Tcl/Tk script file may be taken from
    CNETTCLTK or provided with the -F command-line option.
    If an absolute filename, it is sought as is; if a relative filename,
    it is sought in one of the directories in CNETPATH.
 */

#define	CNETTCLTK	"cnet.tcl"

/*  A small number of common errors are detected by cnet at run-time.
    To assist students in quickly finding an explanation for the error,
    we just refer them to cnet's FAQ.
 */

#define	WWW_FAQ		"http://www.csse.uwa.edu.au/cnet/faq.html"

/*  cnet's compiled protocol files are linked to dynamically shared object
    files which normally have a .so extension.  The value of CNETFILEEXT
    may be defined to provide a more useful name.
 */

#define	CNETFILEEXT			".cnet"

/*  I prefer the rigourous treatment of gcc -std=c99 -pedantic -Werror -Wall
    and recommend it as a good habit to adopt. If you're convinced, define
    the 4 constants to true here.
 */

#define	GCC_STDC99_WANTED		true
#define	GCC_PEDANTIC_WANTED		false
#define	GCC_WERROR_WANTED		true
#define	GCC_WALL_WANTED			true

/*  Each node may have a "compile string" provided in the topology file
    or with the -C command-line argument.  This string may include
    C compiler directives and the names of several source files
    containing the protocols. It is typically just set to a single filename.
    If not provided, DEFAULT_COMPILE_STRING is used.
    DEFAULT_COMPILE_STRING must be defined.
 */

#define	DEFAULT_COMPILE_STRING		"protocol.c"

/*  Each node is rebooted by calling a named entry point in its shared
    object.  This may be specified in the topology file or with the -R
    command-line argument.  It is the name of a function which must be
    in the shared object and be externally visible.
    If not provided, DEFAULT_REBOOT_FUNCTION is used.
    DEFAULT_REBOOT_FUNCTION must be defined.
 */

#define	DEFAULT_REBOOT_FUNCTION		"reboot_node"

/*  Some texts (Tanenbaum, 2nd ed.) suggest that frame corruption errors
    should be reported via the Physical Layer - the datalink software
    should not have to detect corruption with checksum algorithms.  If you
    would like the *default* action to be that CNET_read_physical() in
    the receiver returns -1 and sets cnet_errno=ER_CORRUPTFRAME for you,
    define REPORT_PHYSICAL_CORRUPTION to 0 or 1 here.
    This can be toggled at run-time with the -e option.
    REPORT_PHYSICAL_CORRUPTION must be defined.
 */

#define	REPORT_PHYSICAL_CORRUPTION	false

/*  The standard Physical Layer implementation (in physicallayer.c) corrupts
    and loses data frames subject to certain probabilities.  Corruption is
    always implemented by modifying bytes in the data frames, but may also
    truncate (shorten) some frames as well.
 */

#define	MAY_TRUNCATE_FRAMES		false

/*  You may wish to limit the number of frames that may be "pending"
    on each network link. If so, set MAX_PENDING_FRAMES here to some
    smaller value, like 8.  MAX_PENDING_FRAMES must be defined.
 */

#define	MAX_PENDING_FRAMES		1000

#define	DELAY_WRITE_LOOPBACK		10		// microseconds
#define	DELAY_WRITE_DIRECT		1000		// microseconds

#define	WAN_BANDWIDTH			56000		// 56Kbps
#define	WAN_PROPAGATION			100000		// microseconds

/*  We use the full slotlength for LT_LAN, assuming that wired segments
    can be their full 2.5km long.
 */

#define	LAN_BANDWIDTH			10000000	// 10Mbps
#define	LAN_PROPAGATION			52		// microseconds
#define	LAN_SLOTTIME			52		// microseconds

#define	LAN_ENCODE_TIME			0		// microseconds >= 0
#define	LAN_DECODE_TIME			1		// microseconds > 0

/*  LT_WLAN links are modelled on the Orinocco PCMCIA Silver/Gold cards.
    See:
		http://www.swisswireless.org/wlan_calc_en.html
		http://www.terabeam.com/support/calculations/som.php
 */

#define	WLAN_PROPAGATION		5		// 5usec
#define	WLAN_BANDWIDTH			11000000	// 11Mbps
#define	WLAN_FREQUENCY_GHz		2.45

#define	WLAN_TX_POWER_dBm		14.771
#define	WLAN_TX_CABLE_LOSS_dBm		0.0
#define	WLAN_TX_ANTENNA_GAIN_dBi	2.14

#define	WLAN_RX_ANTENNA_GAIN_dBi	2.14
#define	WLAN_RX_CABLE_LOSS_dBm		0.0
#define	WLAN_RX_SENSITIVITY_dBm		(-82.0)
#define	WLAN_RX_SIGNAL_TO_NOISE_dBm	16.0

#define	WLAN_SLEEP_mA			9.0
#define	WLAN_IDLE_mA			156.0
#define	WLAN_TX_mA			285.0
#define	WLAN_RX_mA			185.0

#define	WLAN_ENCODE_TIME		0		// microseconds >= 0
#define	WLAN_DECODE_TIME		1		// microseconds > 0

#if	defined(USE_TCLTK)
#define	WLAN_RX_HISTORY			5

#define	WLAN_1_BAR	(WLAN_RX_SENSITIVITY_dBm + WLAN_RX_SIGNAL_TO_NOISE_dBm)
#define	WLAN_2_BARS			(WLAN_1_BAR  + 1.0)
#define	WLAN_3_BARS			(WLAN_2_BARS + 2.0)
#define	WLAN_4_BARS			(WLAN_3_BARS + 3.0)
#define	WLAN_5_BARS			(WLAN_4_BARS + 4.0)
#endif


/*  The arcs drawn to represent the transmission footprint of WLAN links
    may be either just an outline, or a filled region.
    Set FILL_WLAN_ARCS to either 1 or 0 as desired.
 */

#define	FILL_WLAN_ARCS			0
#define	DRAW_WLAN_RADIUS		30
#define	DELAY_DRAW_WLANSIGNAL		50000		// microseconds

/*  You may like to adjust the period (frequency) with which statistics
    are updated on the events and statistics popup frames.
    Parseable strings include:  100usec, 200ms, 3s, 4min, 5hr, 6000events
 */

#define	UPDATE_PERIOD			"1s"		// every second
#define	SIMULATION_LENGTH		"1000000h"	// a very long time
#define	DEFAULT_mflag_MINUTES		5		// default -m value


/*  Selecting the right mouse button over a link displays a menu from
    which that link may be severed or reconnected.  You may provide here
    a short and a long value, in seconds, for which the link will remain
    severed, after which it is reconnected.
 */

#define	LINK_SEVER_SHORT		10		// both in secs
#define	LINK_SEVER_LONG			60

/*  The network map can be drawn to highlight heavily used links by drawing
    them fatter than lesser used links. CANVAS_FATLINK specifies the width
    (in pixels) of heavily used links. If you wish all links to simply be
    drawn with all the same width, #define CANVAS_FATLINK to 1.
 */

#define	CANVAS_FATLINK			4

/*  Each node has an internal clock which may or may not be synchronized
    with the internal clocks of other nodes. NODE_CLOCK_SKEW indicates the
    maximum number of microseconds that each node may randomly differ from
    the "average" time. Defining NODE_CLOCK_SKEW to 0 requests that all
    nodes are initialized with the same time.
    Similarly, the -c command-line switch ensures clock synchronization.
 */

#define	NODE_CLOCK_SKEW			600000000	// for +/- 10 mins

/*  You may want a message to appear each time a student runs
    cnet (sort of a message of the day for cnet). For example, I
    have previously ensured that students' projects are in the
    correct directory for automatic collection. If you want this,
    define MOTD_WANTED here and add your own code in motd.c:motd()
 */

#define	MOTD_WANTED			0

/*  Random topologies generated with the -r command-line option are
    placed on a rectangular grid.  Define RANDOM_DIAGONALS to be true
    if you wish diagonal links on this grid.
 */

#define	RANDOM_DIAGONALS		true

/*  I get annoyed if students spell 'receive' as 'recieve'.
    If you would like compile.c to check for this, define
    CHECK_RECEIVE_SPELLING here.
 */

#define	CHECK_RECEIVE_SPELLING		true

/*  There is a very real chance that students' protocols may choose
    variable and function names the same as those that are globally
    accessible within cnet, and I'm not sure how this would affect the
    dynamic linking on (future) implementations.  If you would like global
    names hidden (via hidenames.h), define HIDE_GLOBAL_NAMES to true here.
 */

#define	HIDE_GLOBAL_NAMES		true

/*  These constants constrain the drawing of datalink frames on WAN links.
    All sizes are in pixels.
 */

#define	MAX_DRAWFRAMES			16
#define	DRAWFRAME_WIDTH			600
#define	DRAWFRAME_HEIGHT		72
#define	DRAWFRAME_COLOUR		"yellow"
#define	DRAWFRAME_WAYPOINTS		32
#define	DRAWFRAME_ZAPDELAY		400000

#define	DEFAULT_MAPWIDTH		400
#define	DEFAULT_MAPHEIGHT		300
#define	DEFAULT_NODE_SPACING		50

/*  Large (64 bit) numbers may be displayed with commas to make them
    more readable.  Commas are only presented on the GUI, and are
    never in the statistics printed to stdout.  If you would like commas
    displayed in large numbers, define OUTPUT_COMMAS to true here.
 */

#define	OUTPUT_COMMAS			true


//  No comments on these constants yet.  Do your best...
#define	MAXEXTENSIONS			16
#define	WIDTH_DEBUG_BUTTON_STRING	12

#define	DEFAULT_ICONTITLE		"%d: %n"

#define	COLOUR_WAN			"magenta"
#define	COLOUR_LAN			"blue"
#define	COLOUR_WLAN			"red"
#define	COLOUR_SEVERED			"red"
#define	COLOUR_GRID			"#aaaaaa"
