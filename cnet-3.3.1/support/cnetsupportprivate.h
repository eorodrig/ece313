// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	!defined(_CNETSUPPORTPRIVATE_H_)
#define	_CNETSUPPORTPRIVATE_H_		1

#if	!defined(USE_MACOSX)
extern	char	*strdup(const char *);
extern	int	strcasecmp(const char *, const char *);
#endif

#define CHECKALLOC(ptr)	do { if(ptr == NULL) { fprintf(stderr, "allocation failed - %s:%s(), line %d\n", __FILE__, __func__, __LINE__); exit(2); }} while(false)

#endif

//  vim: ts=8 sw=4
