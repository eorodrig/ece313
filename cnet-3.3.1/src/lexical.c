#include "cnetprivate.h"
#include "lexical.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

typedef struct _hl {
    char	*keyword;
    TOKEN	token;
    struct _hl	*next;
} HASHLIST;

typedef struct {
    int		size;
    int		nitems;
    HASHLIST	**table;
} HASHTABLE;

static	HASHTABLE	*ht;

static unsigned int hash(char *str)
{
    unsigned int	h = 0;

    while(*str) {
	h = (h<<3) + *str ;
	++str;
    }
    return(h);
}

static void new_hashtable(int size)
{
    ht			= malloc(sizeof(HASHTABLE));
    CHECKALLOC(ht);
    ht->size		= (size > 0) ? size : 1023;
    ht->table		= malloc(ht->size * sizeof(HASHLIST *));
    CHECKALLOC(ht->table);
    ht->nitems		= 0;
    memset(ht->table, 0, ht->size * sizeof(HASHLIST *));
}

static void add_keyword(char *keyword, TOKEN newtoken)
{
    int		h;
    HASHLIST	*hl;

    h			= hash(keyword) % ht->size;
    hl			= ht->table[h];
    while(hl) {
	if(strcmp(hl->keyword, keyword) == 0) {
	    hl->token	= newtoken;
	    return;
	}
	hl	= hl->next;
    }
    hl			= NEW(HASHLIST);
    CHECKALLOC(hl);
    hl->keyword		= strdup(keyword);
    CHECKALLOC(hl->keyword);
    hl->token		= newtoken;
    hl->next		= ht->table[h];
    ht->table[h]	= hl;
    ++ht->nitems;
}

static TOKEN hash_get(char *word)
{
    int		h;
    HASHLIST	*hl;

    h			= hash(word) % ht->size;
    hl			= ht->table[h];
    while(hl) {
	if(strcmp(hl->keyword, word) == 0)
	    return(hl->token);
	hl	= hl->next;
    }
    return(T_BAD);
}

// ---------------------------------------------------------------------


void init_lexical(void)
{
    input.cc	= 0;
    input.ll	= 0;
    input.lc	= 0;

//  KEYWORDS MAY BE ADDED IN ANY ORDER
    new_hashtable(0);

//  NODE THINGS
    add_keyword("accesspoint",		T_ACCESSPOINT);
    add_keyword("host",			T_HOST);
    add_keyword("mobile",		T_MOBILE);
    add_keyword("router",		T_ROUTER);

    add_keyword("accesspointtype",	T_ACCESSPOINTTYPE);
    add_keyword("hosttype",		T_HOSTTYPE);
    add_keyword("mobiletype",		T_MOBILETYPE);
    add_keyword("routertype",		T_ROUTERTYPE);

//  LINK THINGS
    add_keyword("lan",			T_LAN);
    add_keyword("link",			T_WAN);
    add_keyword("wan",			T_WAN);
    add_keyword("wireless",		T_WLAN);
    add_keyword("wlan",			T_WLAN);

    add_keyword("wantype",		T_WANTYPE);
    add_keyword("lantype",		T_LANTYPE);
    add_keyword("wlantype",		T_WLANTYPE);

//  GLOBAL ATTRIBUTES
    add_keyword("drawlinks",		T_DRAWLINKS);
    add_keyword("drawnodes",		T_DRAWNODES);
    add_keyword("extension",		T_EXTENSION);
    add_keyword("mapcolor",		T_MAPCOLOUR);
    add_keyword("mapcolour",		T_MAPCOLOUR);
    add_keyword("mapgrid",		T_MAPGRID);
    add_keyword("mapheight",		T_MAPHEIGHT);
    add_keyword("maphex",		T_MAPHEX);
    add_keyword("mapimage",		T_MAPIMAGE);
    add_keyword("mapmargin",		T_MAPMARGIN);
    add_keyword("mapscale",		T_MAPSCALE);
    add_keyword("maptile",		T_MAPTILE);
    add_keyword("mapwidth",		T_MAPWIDTH);
    add_keyword("positionerror",	T_POSITIONERROR);
    add_keyword("showcostperbyte",	T_SHOWCOSTPERBYTE);
    add_keyword("showcostperframe",	T_SHOWCOSTPERFRAME);
    add_keyword("tracefile",		T_TRACEFILE);

//  NODE ATTRIBUTES
    add_keyword("address",		T_NODEATTR_ADDRESS);
    add_keyword("args",			T_NODEATTR_REBOOTARGV);
    add_keyword("argv",			T_NODEATTR_REBOOTARGV);
    add_keyword("compile",		T_NODEATTR_COMPILE);
    add_keyword("icontitle",		T_NODEATTR_ICONTITLE);
    add_keyword("logfile",		T_NODEATTR_OUTPUTFILE);
    add_keyword("maxmessagesize",	T_NODEATTR_MAXMESSAGESIZE);
    add_keyword("messagerate",		T_NODEATTR_MESSAGERATE);
    add_keyword("minmessagesize",	T_NODEATTR_MINMESSAGESIZE);
    add_keyword("nodemtbf",		T_NODEATTR_MTBF);
    add_keyword("nodemttr",		T_NODEATTR_MTTR);
    add_keyword("outputfile",		T_NODEATTR_OUTPUTFILE);
    add_keyword("rebootargs",		T_NODEATTR_REBOOTARGV);
    add_keyword("rebootargv",		T_NODEATTR_REBOOTARGV);
    add_keyword("rebootfunc",		T_NODEATTR_REBOOTFUNC);
    add_keyword("sourcefile",		T_NODEATTR_COMPILE);
    add_keyword("sourcefiles",		T_NODEATTR_COMPILE);
    add_keyword("trace",		T_NODEATTR_TRACE);
    add_keyword("var",			T_NODEATTR_VAR);
    add_keyword("x",			T_NODEATTR_X);
    add_keyword("y",			T_NODEATTR_Y);
    add_keyword("z",			T_NODEATTR_Z);
    add_keyword("winopen",		T_NODEATTR_WINOPEN);
    add_keyword("winx",			T_NODEATTR_WINX);
    add_keyword("winy",			T_NODEATTR_WINY);

//  LINK ATTRIBUTES
    add_keyword("bandwidth",		T_WANATTR_BANDWIDTH);
    add_keyword("buffered",		T_WANATTR_BUFFERED);
    add_keyword("costperbyte",		T_WANATTR_COSTPERBYTE);
    add_keyword("costperframe",		T_WANATTR_COSTPERFRAME);
    add_keyword("jitter",		T_WANATTR_JITTER);
    add_keyword("lan-bandwidth",	T_LANATTR_BANDWIDTH);
    add_keyword("lan-buffered",		T_LANATTR_BUFFERED);
    add_keyword("lan-costperbyte",	T_LANATTR_COSTPERBYTE);
    add_keyword("lan-costperframe",	T_LANATTR_COSTPERFRAME);
    add_keyword("lan-jitter",		T_LANATTR_JITTER);
    add_keyword("lan-linkmtbf",		T_LANATTR_MTBF);
    add_keyword("lan-linkmttr",		T_LANATTR_MTTR);
    add_keyword("lan-mtu",		T_LANATTR_MTU);
    add_keyword("lan-probframecorrupt",	T_LANATTR_PROBFRAMECORRUPT);
    add_keyword("lan-probframeloss",	T_LANATTR_PROBFRAMELOSS);
    add_keyword("lan-propagationdelay",	T_LANATTR_PROPAGATION);
    add_keyword("lan-slottime",		T_LANATTR_SLOTTIME);
    add_keyword("lansegment",		T_LANSEGMENT);
    add_keyword("linkmtbf",		T_WANATTR_MTBF);
    add_keyword("linkmttr",		T_WANATTR_MTTR);
    add_keyword("mtu",			T_WANATTR_MTU);
    add_keyword("nicaddr",		T_NICADDR);
    add_keyword("probframecorrupt",	T_WANATTR_PROBFRAMECORRUPT);
    add_keyword("probframeloss",	T_WANATTR_PROBFRAMELOSS);
    add_keyword("propagationdelay",	T_WANATTR_PROPAGATION);
    add_keyword("wan-bandwidth",	T_WANATTR_BANDWIDTH);
    add_keyword("wan-buffered",		T_WANATTR_BUFFERED);
    add_keyword("wan-costperbyte",	T_WANATTR_COSTPERBYTE);
    add_keyword("wan-costperframe",	T_WANATTR_COSTPERFRAME);
    add_keyword("wan-jitter",		T_WANATTR_JITTER);
    add_keyword("wan-linkmtbf",		T_WANATTR_MTBF);
    add_keyword("wan-linkmttr",		T_WANATTR_MTTR);
    add_keyword("wan-mtu",		T_WANATTR_MTU);
    add_keyword("wan-probframecorrupt",	T_WANATTR_PROBFRAMECORRUPT);
    add_keyword("wan-probframeloss",	T_WANATTR_PROBFRAMELOSS);
    add_keyword("wan-propagationdelay",	T_WANATTR_PROPAGATION);
    add_keyword("wlan-bandwidth",	T_WLANATTR_BANDWIDTH);
    add_keyword("wlan-buffered",	T_WLANATTR_BUFFERED);
    add_keyword("wlan-costperbyte",	T_WLANATTR_COSTPERBYTE);
    add_keyword("wlan-costperframe",	T_WLANATTR_COSTPERFRAME);
    add_keyword("wlan-frequency",	T_WLANATTR_FREQUENCY);
    add_keyword("wlan-idlecurrent",	T_WLANATTR_IDLECURRENT);
    add_keyword("wlan-jitter",		T_WLANATTR_JITTER);
    add_keyword("wlan-linkmtbf",	T_WLANATTR_MTBF);
    add_keyword("wlan-linkmttr",	T_WLANATTR_MTTR);
    add_keyword("wlan-mtu",		T_WLANATTR_MTU);
    add_keyword("wlan-probframecorrupt",T_WLANATTR_PROBFRAMECORRUPT);
    add_keyword("wlan-probframeloss",	T_WLANATTR_PROBFRAMELOSS);
    add_keyword("wlan-rxantennagain",	T_WLANATTR_RXANTENNAGAIN);
    add_keyword("wlan-rxcableloss",	T_WLANATTR_RXCABLELOSS);
    add_keyword("wlan-rxcurrent",	T_WLANATTR_RXCURRENT);
    add_keyword("wlan-rxsensitivity",	T_WLANATTR_RXSENSITIVITY);
    add_keyword("wlan-rxsignaltonoise",	T_WLANATTR_RXSIGNALTONOISE);
    add_keyword("wlan-sleepcurrent",	T_WLANATTR_SLEEPCURRENT);
    add_keyword("wlan-txantennagain",	T_WLANATTR_TXANTENNAGAIN);
    add_keyword("wlan-txcableloss",	T_WLANATTR_TXCABLELOSS);
    add_keyword("wlan-txcurrent",	T_WLANATTR_TXCURRENT);
    add_keyword("wlan-txpower",		T_WLANATTR_TXPOWER);

//  UNITS
    add_keyword("Bps",			T_BYTESPS);
    add_keyword("GHz",			T_GHz);
    add_keyword("KB",			T_KBYTES);
    add_keyword("KBps",			T_KBYTESPS);
    add_keyword("KByte",		T_KBYTES);
    add_keyword("KBytes",		T_KBYTES);
    add_keyword("Kbps",			T_KBITSPS);
    add_keyword("MB",			T_MBYTES);
    add_keyword("MBps",			T_MBYTESPS);
    add_keyword("MByte",		T_MBYTES);
    add_keyword("MBytes",		T_MBYTES);
    add_keyword("MHz",			T_MHz);
    add_keyword("Mbps",			T_MBITSPS);
    add_keyword("bps",			T_BITSPS);
    add_keyword("bytes",		T_BYTES);
    add_keyword("c",			T_C);
    add_keyword("dBm",			T_dBm);
    add_keyword("m",			T_METRES);
    add_keyword("mA",			T_mA);
    add_keyword("mW",			T_mW);
    add_keyword("ms",			T_MSEC);
    add_keyword("msec",			T_MSEC);
    add_keyword("msecs",		T_MSEC);
    add_keyword("px",			T_PIXELS);
    add_keyword("s",			T_SEC);
    add_keyword("sec",			T_SEC);
    add_keyword("secs",			T_SEC);
    add_keyword("us",			T_USEC);
    add_keyword("usec",			T_USEC);
    add_keyword("usecs",		T_USEC);

//  COMPASS POINTS
    add_keyword("east",			T_EAST);
    add_keyword("north",		T_NORTH);
    add_keyword("northeast",		T_NORTHEAST);
    add_keyword("northwest",		T_NORTHWEST);
    add_keyword("south",		T_SOUTH);
    add_keyword("southeast",		T_SOUTHEAST);
    add_keyword("southwest",		T_SOUTHWEST);
    add_keyword("west",			T_WEST);

//  OTHER KEYWORDS
    add_keyword("false",		T_FALSE);
    add_keyword("from",			T_FROM);
    add_keyword("join",			T_JOIN);
    add_keyword("of",			T_OF);
    add_keyword("to",			T_TO);
    add_keyword("toggle",		T_TOGGLE);
    add_keyword("true",			T_TRUE);

    if(vflag)
	REPORT("%i keywords\n", ht->nitems);
}

int	nextch;
TOKEN	token;

static void cpp_line(void)
{
    char *p = input.line+1,
	 *q = chararray;

    while(*p && !(isdigit((int)*p)))
	p++;
    if(!isdigit((int)*p))
	return;

    input.lc = 0;
    while(*p && isdigit((int)*p))
	input.lc = input.lc*10 + (*p++ -'0');
    --input.lc;

    while(*p && *p != '"') p++;
    p++;
    while(*p && *p != '"') *q++ = *p++;
    *q = '\0';
    input.name = strdup(*chararray ? chararray : "<stdin>");
    CHECKALLOC(input.name);
}


static void get(void)		// get the next buffered char from input.line
{
    if(input.cc == input.ll) {
	input.cc = input.ll = 0;
	*input.line = '\0';
	while(fgets(input.line,BUFSIZ,input.fp) != NULL) {
	    char *p = input.line;

	    while(*p++ &= 0177);		// cleanup line
	    input.ll = --p - input.line;

	    if(*input.line == '#') {
		cpp_line(); continue;
	    }
	    break;
	}
	input.lc++;
    }
    nextch = input.line[input.cc++];
}

#define unget()		(nextch = input.line[--input.cc])

static void skip_whitespace(void)	//  Skip whitespace and C-comments
{
    while(isspace(nextch) || nextch == '/') {
	if(nextch == '/') {
	    get();
	    if(nextch == '/') {		// An ISO-C99 or C++ style comment
		input.cc = input.ll = 0;
		get();
		continue;
	    }
	    if(nextch != '*') {
		unget();
		break;
	    }
	    get();
	    while(!feof(input.fp)) {
		if(nextch == '*') {
		    get();
		    if(nextch == '/') {
			get();
			break;
		    }
		}
	        get();
	    }
	    continue;
	}
	get();
    }
}

static	char *cp;

static void escape_str(void)
{
    get();
    switch(nextch) {
	case 'b' :
	case 'f' :
	case 'n' :
	case 'r' :
	case 't' :  *cp++ = '\\';
		    *cp++ = nextch;
		    break;
    }
}

static void parse_etheraddr(void)
{
    while(isxdigit(nextch) || nextch == ':') {
	*cp++	= nextch;
	get();
    }
    unget();
    *cp		= '\0';
    token	= (CNET_parse_nicaddr(input.nicaddr, chararray) == 0)
			    ? T_NICADDRVALUE : T_BAD;
}

static void parse_number(void)
{
    bool	negative	= false;

    if(nextch == '-') {
	negative	= true;
	get();
    }

    cp			= chararray;
    input.ivalue	= 0;
    while(isdigit(nextch)) {
	input.ivalue = input.ivalue*10 + (nextch-'0');
	*cp++ = nextch;
	get();
    }
    *cp	= '\0';
    if(nextch == '.') {
	*cp++ = '.';
	get();
	while(isdigit(nextch)) {
	    *cp++ = nextch;
	    get();
	}
	if(nextch == 'E' || nextch == 'e') {
	    *cp++ = 'E';
	    if(nextch == '-' || nextch == '+') {
		*cp++	= nextch;
		get();
	    }
	    while(isdigit(nextch)) {
		*cp++ = nextch;
		get();
	    }
	}
	*cp	= '\0';
	unget();
	if(sscanf(chararray, "%lf", &input.dvalue) == 1) {
	    if(negative)
		input.dvalue = -input.dvalue;
	    token	= T_REALCONST;
	}
	else
	    token	= T_BAD;
    }
    else if(nextch == ':')
	parse_etheraddr();
    else {
	unget();
	if(negative)
	    input.ivalue = -input.ivalue;
	token = T_INTCONST;
    }
}

void gettoken(void)
{
    get();
    skip_whitespace();

    *chararray		= '\0';
    input.ivalue	= 0;
    input.dvalue	= 0.0;
    if(isalpha(nextch)) {
	cp = chararray;
	while(isalnum(nextch) || nextch =='_'|| nextch=='-' || nextch=='.') {
	    *cp++ = nextch;
	    get();
	}
	*cp = '\0';
	unget();
//  SEE IF THE CURRENT WORD IS A RESERVED WORD
	token	= hash_get(chararray);
	if(token == T_BAD)
	    token = T_NAME;
	return;
    }
    else if(isdigit(nextch) || nextch == '-') {
	parse_number();
	return;
    }
    else if(isxdigit(nextch)) {
	cp	= chararray;
	parse_etheraddr();
	return;
    }
    else if(nextch == '"') {
	get();
	cp = chararray;
	while(nextch != '"' && !feof(input.fp)) {
	    if(nextch == '\\')
		escape_str();
	    else
		*cp++ = nextch;
	    get();
	}
	*cp	= '\0';
	token	= feof(input.fp) ? T_BAD : T_STRCONST;

    } else
    switch (nextch) {
	case '{' :	token = T_LCURLY;		break;
	case '}' :	token = T_RCURLY;		break;
	case '=' :	token = T_EQ;			break;
	case ';' :
	case ',' :	token = T_COMMA;		break;
	default  :	token = feof(input.fp) ? T_EOF : T_BAD;
							break;
    }
}
