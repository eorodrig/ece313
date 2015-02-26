#include "cnetsupport.h"
#include "cnetsupportprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

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

#define	MAGIC	0xBABEFACE

typedef struct {
    int32_t	magic;
    void	**items;
    size_t	*lens;
    int		nitems;
} _VECTOR;

#define check(v) if(v == NULL || v->magic != MAGIC) {	\
	fprintf(stderr, "invalid VECTOR passed to %s()\n", __func__); exit(1); }

/* -------------------------------------------------------------------- */

VECTOR vector_new(void)
{
    _VECTOR *v	= calloc(1, sizeof(_VECTOR));

    CHECKALLOC(v);
    v->magic	= MAGIC;
    v->items	= calloc(1, sizeof(void *));
    CHECKALLOC(v->items);
    v->lens	= calloc(1, sizeof(size_t));
    CHECKALLOC(v->lens);
    return ((VECTOR)v);
}

void vector_free(VECTOR V)
{
    _VECTOR	*v	= (_VECTOR *)V;
    int		n;

    check(v);
    for(n=0 ; n<v->nitems ; ++n)
	free(v->items[n]);
    free(v->items);
    free(v);
}

void vector_append(VECTOR V, void *newitem, size_t len)
{
    _VECTOR	*v	= (_VECTOR *)V;

    check(v);
    v->items		= realloc(v->items, (v->nitems+1)*sizeof(void *));
    CHECKALLOC(v->items);
    v->lens		= realloc(v->lens, (v->nitems+1)*sizeof(size_t));
    CHECKALLOC(v->lens);
    v->items[v->nitems]	= calloc(1, len);
    CHECKALLOC(v->items[v->nitems]);
    memcpy(v->items[v->nitems], newitem, len);
    v->lens[v->nitems] = len;
    v->nitems++;
}

void vector_replace(VECTOR V, int position, void *newitem, size_t len)
{
    _VECTOR	*v	= (_VECTOR *)V;

    check(v);
    if(position >= 0 && position < v->nitems) {
	free(v->items[position]);
	v->items[position]	= calloc(1, len);
	CHECKALLOC(v->items[position]);
	memcpy(v->items[position], newitem, len);
	v->lens[position]	= len;
    }
}

void *vector_remove(VECTOR V, int position, size_t *len)
{
    _VECTOR	*v	= (_VECTOR *)V;

    check(v);
    if(position >= 0 && position < v->nitems) {
	void	*old	= v->items[position];
	size_t	shift	= (v->nitems - position - 1);

	if(len)
	    *len	= v->lens[position];
	if(shift != 0) {
	    memcpy(&v->items[position], &v->items[position+1],
			shift*sizeof(void *));
	    memcpy(&v->lens[position], &v->lens[position+1],
			shift*sizeof(size_t));
	}
	v->nitems--;
	return	old;
    }
    return NULL;
}

void *vector_peek(VECTOR V, int position, size_t *len)
{
    _VECTOR	*v	= (_VECTOR *)V;

    check(v);
    if(position >= 0 && position < v->nitems) {
	if(len)
	    *len	= v->lens[position];
	return v->items[position];
    }
    return NULL;
}

int vector_nitems(VECTOR V)
{
    _VECTOR	*v	= (_VECTOR *)V;

    check(v);
    return(v->nitems);
}

//  vim: ts=8 sw=4
