#include "cnetsupport.h"
#include "cnetsupportprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if   !defined(__APPLE__)
extern	char	*strdup(const char *);
#endif

void remove_nl(char *line)
{
    while(*line) {
	if(*line == '\n' || *line == '\r') {
	    *line	= '\0';
	    break;
	}
	++line;
    }
}

void remove_comment(char *line)
{
    bool inquote = false;

    while(*line) {
	if(*line == CHAR_DQUOTE)
	    inquote = !inquote;
	else if(*line == '\n' || *line == '\r' ||
		(!inquote && *line == CHAR_COMMENT)) {
	    *line	= '\0';
	    break;
	}
	++line;
    }
}

// -------------------------------------------------------------------

char *fgetmultiline(FILE *fp, int *lc)
{
    char	line[BUFSIZ];
    char	*result	= NULL;
    size_t	tlen	= 0;

    while(fgets(line, sizeof line, fp) != NULL) {
	char	*s = line;

	if(lc)
	    *lc += 1;

	while(*s) {
	    if(*s == '\n' || *s == '\r') {
		*s	= '\0';
		break;
	    }
	    ++s;
	}
	size_t len = s - line;

	result	= realloc(result, tlen+len+1);
	CHECKALLOC(result);

	strcpy(&result[tlen], line);
	tlen	+= len;

	if(result[tlen-1] != '\\')
	    break;
	--tlen;
	result[tlen]	= '\0';
    }
    return result;
}

// -------------------------------------------------------------------

#define	atend(line)	(*line == '\0' || *line == '\n' || *line == '\r')
#define	inside(line)	(*line != '\0' && *line != '\n' && *line != '\r')

char **tokenize(char *line, int *ntokens, const char *separators)
{
    char	ch, *t;
    char	**tokens;
    int		nfound;

    if(separators == NULL)
	separators	= " \t";
    tokens	= malloc((strlen(line)+2)*sizeof(char *));
    CHECKALLOC(tokens);
    nfound	= 0;

//  FIRSTLY, REPLACE FIRST NEWLINE OR CARRIAGE RETURN WITH A NULL
    t	= line;
    while(inside(t)) {
	if(*t == '\n' || *t == '\r') {
	    *t	= '\0';
	    break;
	}
	++t;
    }

    t	= line;
    while(inside(line)) {
// SKIP LEADING SEPARATORS
	while(inside(line) && strchr(separators, *line))
	    ++line;
	if(atend(line))
	    break;
// COLLECT TOKEN - EITHER A SINGLE WORD OR A QUOTED STRING
	t	= line;
	if(*line == CHAR_DQUOTE) {
	    ++t, ++line;
	    while(inside(line) && *line != CHAR_DQUOTE)
		++line;
	}
	else {
	    while(inside(line) && !strchr(separators, *line))
		++line;
	}
	ch		= *line;
	*line		= '\0';
	tokens[nfound]	= strdup(t);
	++nfound;
	*line		= ch;

	if(ch == CHAR_DQUOTE)
	    ++line;
    }
    tokens[nfound]	= NULL;
    if(ntokens)
	*ntokens	= nfound;
    return(tokens);
}

void free_tokens(char **tokens)
{
    if(tokens) {
	char	**t = tokens;

	while(*t)
	    free(*t++);
	free(tokens);
    }
}

//  vim: ts=8 sw=4
