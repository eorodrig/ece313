/*  This is the standard support header file for the cnet simulator.
    DO NOT COPY this file, simply #include it into your cnet protocol files.
 */

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#if	!defined(_CNETSUPPORT_H_)
#define	_CNETSUPPORT_H_		1

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*  The QUEUE data structure manages a FIFO queue of arbitrary data items.
    QUEUE does not know what it is managing, so you must indicate the
    length, in bytes, of each added item.  QUEUE then uses malloc/free to
    keep a copy of each added item.

    Removing the head item from the QUEUE returns a pointer to the
    previously allocated copy, which you should use and, eventually, free.
    Peeking at the head of a QUEUE does not remove the item.
    An attempt to remove from, or peek at, an empty queue returns NULL.

    A typical use is:

	QUEUE	myq	= queue_new();
	TYPE	mydata, *myptr;
	size_t	len;

	queue_add(myq, &mydata, sizeof mydata);
	    .....
	queue_add(myq, &mydata, sizeof mydata);
	    .....
	myptr	= queue_remove(myq, &len);
	if(myptr) {
	    .....
	    free(myptr);
	}
	queue_free(myq);
 */

typedef	void *	QUEUE;
extern	QUEUE	queue_new(void);
extern	void	queue_free(QUEUE q);
extern	void	queue_add(QUEUE q, void *item, size_t len);
extern	void	*queue_remove(QUEUE q, size_t *len);
extern	void	*queue_peek(QUEUE q, size_t *len);
extern	int	queue_nitems(QUEUE q);

/*  The HASHTABLE data structure manages the association of a string
    (a name or key) with an arbitrary data item.
    hashtable_new creates a new HASHTABLE, with the indicated number of
    buckets - indicating 0 buckets requests the default.

    HASHTABLE does not know what it is managing, so you must indicate
    the length, in bytes, of each added item.  HASHTABLE then uses
    malloc/free to manage a copy of each added item.  If a second item
    is added with an existing name, the space occupied by the first item
    is first deallocated.

    Finding an item in a HASHTABLE leaves the item in the HASHTABLE,
    and returns a pointer to the found item, and informs you of its length.
    hashtable_remove deallocates the space occopied by a requested item.
    hashtable_free deallocates the space occopied by all items and the
    HASHTABLE itself.

    A typical use is:

	HASHTABLE	myht	= hashtable_new(0);
	TYPE		mydata, *myptr;
	size_t		len;

	hashtable_add(myht, "apple",  &mydata, sizeof mydata);
	hashtable_add(myht, "banana", &mydata, sizeof mydata);
	    ....
	myptr = hashtable_find(myht, "orange", &len);
	if(myptr) {
	    ....
	}
	hashtable_free(myht);
 */

typedef	void *	HASHTABLE;
extern	HASHTABLE	hashtable_new(int nbuckets);
extern	void	hashtable_free(HASHTABLE ht);
extern	void	hashtable_add(HASHTABLE ht, char *name, void *item, size_t len);
extern	void	*hashtable_remove(HASHTABLE ht, char *name, size_t *len);
extern	void	*hashtable_find(HASHTABLE ht, char *name, size_t *len);
extern	int	hashtable_nitems(HASHTABLE ht);

/*  The VECTOR datatype and functions manage a vector of
    heterogenous-sized items.  Each data item may be of a different
    datatype and a different size and, thus, these functions should not
    be used in preference to a standard array in C. A cnet node may
    have an unlimited number of such vectors.  A new vector is firstly
    allocated by calling vector_new, and should eventually be deallocated
    with a call to vector_free. The opaque value returned by vector_new
    should be passed to all other functions, to identify which vector
    is being used.

    Items, of any datatype and length, may be appended to a vector with
    vector_append. The functions do not know what they are storing, and
    so you must also provide the length, in bytes, of each appended
    item. A copy is made of all items appended to a vector. The number
    of items in a vector may be determined at any time by calling
    vector_nitems. The first item in a vector is in position 0.

    An existing item in a vector may be replaced by a new item by calling
    vector_replace and providing the new length. Removing an item from a
    vector returns a pointer to the previously allocated copy, which you
    should use and, eventually, free. All remaining items in the vector
    "slide-down" by one position to fill the hole created by the removal.
    Peeking at any item of a vector does not remove the item.

    A typical use of these functions is:

	VECTOR    myv;
	MYTYPE    mydata, *myptr;
	size_t    len;
	int       n;

	myv     = vector_new();
	vector_append(myv, &mydata, sizeof(mydata));
	.....
	vector_append(myv, &mydata, sizeof(mydata));
	.....
	n = vector_nitems(myv);
	while(n > 0) {
	    myptr   = vector_remove(myv, 0, &len);
	    --n;
	    .....
	    free(myptr);
	}
	.....
	vector_free(myv);
 */

typedef	void *	VECTOR;
extern	VECTOR	vector_new(void);
extern	void	vector_free(VECTOR v);
extern	void	vector_append(VECTOR v, void *item, size_t len); 
extern	void	vector_replace(VECTOR v, int position, void *item, size_t len); 
extern	void	*vector_peek(VECTOR v, int position, size_t *len);
extern	void	*vector_remove(VECTOR v, int position, size_t *len);
extern	int	vector_nitems(VECTOR v);


/*  tokenize() breaks a string into a number of tokens that are separated
    by characters from an indicated string.
    Initial separators, at the beginning of a line, are first skipped.
    A token appearing within double-quotes may contain any of the separators.
    tokenize() returns a NULL-terminated vector of strings (tokens),
    with the number of tokens provided in ntokens.
    A typical use is:

	char	*line;
	int	lc = 0;

	while((line = fgetmultiline(fp, &lc)) != NULL) {
	    int  ntokens;
	    char **tokens = tokenize(line, &ntokens, " \t");

	    for(int n=0 ; n<ntokens ; ++n)
		// access or copy tokens[n] ...
	    free_tokens(tokens);
	    free(line);
	}
 */

#define	CHAR_COMMENT	'#'
#define	CHAR_DQUOTE	'"'

extern	char	*fgetmultiline(FILE *fp, int *lc);

extern	char	**tokenize(char *line, int *ntokens, const char *separators);
extern	void	free_tokens(char **tokens);
extern	void	remove_comment(char *line);
extern	void	remove_nl(char *line);

#define	MD5_BYTELEN	16
#define	MD5_STRLEN	32

extern	void	*MD5_buffer(const char *input, size_t len, void *md5_result);
extern	char	*MD5_format(void *md5_result);
extern	char	*MD5_str(const char *str);
extern	char	*MD5_file(const char *filenm);


//  RETURNS THE DISTANCE, IN METRES, BETWEEN ANY TWO LAT/LON LOCATIONS
extern	double	haversine(double lat1, double lon1, double lat2, double lon2);

#endif

//  vim: ts=8 sw=4
