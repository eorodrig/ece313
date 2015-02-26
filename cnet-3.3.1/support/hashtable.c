#include "cnetsupport.h"
#include "cnetsupportprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	MAGIC	0xBEEFCAFE

typedef struct _hl {
    char	*name;
    void	*item;
    size_t	len;
    struct _hl	*next;
} HASHLIST;

typedef struct {
    int32_t	magic;
    int		nbuckets;
    int		nitems;
    HASHLIST	**table;
} _HASHTABLE;

#define check(ht) if(ht == NULL || ht->magic != MAGIC) {	\
    fprintf(stderr, "invalid HASHTABLE passed to %s()\n", __func__); exit(1); }

static unsigned int hash(char *str)
{
    unsigned int	h = 0;

    while(*str) {
	h = (h<<3) + *str ;
	++str;
    }
    return(h);
}

HASHTABLE hashtable_new(int nbuckets)
{
    _HASHTABLE	*new;

    new			= calloc(1, sizeof(_HASHTABLE));
    CHECKALLOC(new);
    new->magic		= MAGIC;
    new->nbuckets	= (nbuckets > 0) ? nbuckets : 1023;
    new->table		= calloc(new->nbuckets, sizeof(HASHLIST *));
    CHECKALLOC(new->table);
    new->nitems		= 0;

    return((HASHTABLE)new);
}

void hashtable_add(HASHTABLE HT, char *name, void *item, size_t len)
{
    _HASHTABLE	*ht	= (_HASHTABLE *)HT;
    HASHLIST	*hl;
    int		h;

    check(ht);
    h			= hash(name) % ht->nbuckets;
    hl			= ht->table[h];
    while(hl) {
	if(hl->name == NULL)
	    goto add;
	if(strcmp(hl->name, name) == 0) {
	    free(hl->item);
	    hl->item	= malloc(len);
	    CHECKALLOC(hl->item);
	    memcpy(hl->item, item, len);
	    return;
	}
	hl	= hl->next;
    }
    hl			= calloc(1, sizeof(HASHLIST));
    CHECKALLOC(hl);
    hl->next		= ht->table[h];
    ht->table[h]	= hl;
add:
    hl->name		= strdup(name);
    hl->item		= malloc(len);
    CHECKALLOC(hl->item);
    memcpy(hl->item, item, len);
    ++ht->nitems;
}

void *hashtable_find(HASHTABLE HT, char *name, size_t *len)
{
    _HASHTABLE	*ht	= (_HASHTABLE *)HT;
    HASHLIST	*hl;
    int		h;

    check(ht);
    h		= hash(name) % ht->nbuckets;
    hl		= ht->table[h];
    while(hl) {
	if(hl->name && strcmp(hl->name, name) == 0) {
	    if(len)
		*len	= hl->len;
	    return(hl->item);
	}
	hl	= hl->next;
    }
    if(len)
	*len	= 0;
    return(NULL);
}

void *hashtable_remove(HASHTABLE HT, char *name, size_t *len)
{
    _HASHTABLE	*ht	= (_HASHTABLE *)HT;
    HASHLIST	*hl;
    int		h;

    check(ht);
    h		= hash(name) % ht->nbuckets;
    hl		= ht->table[h];
    while(hl) {
	if(hl->name && strcmp(hl->name, name) == 0) {
	    void	*item	= hl->item;

	    free(hl->name);
	    hl->name	= NULL;
	    hl->item	= NULL;
	    --ht->nitems;

	    if(len)
		*len	= hl->len;
	    return(item);
	}
	hl	= hl->next;
    }
    if(len)
	*len	= 0;
    return(NULL);
}

void hashtable_free(HASHTABLE HT)
{
    _HASHTABLE	*ht	= (_HASHTABLE *)HT;
    HASHLIST	*hl1, *hl2;
    int		h;

    check(ht);
    for(h=0 ; h<ht->nbuckets ; ++h) {
	hl1	= ht->table[h];
	while(hl1) {
	    hl2	= hl1;
	    hl1	= hl1->next;
	    if(hl2->name) {
		free(hl2->name);
		free(hl2->item);
	    }
	    free(hl2);
	}
    }
    free(ht);
}

int hashtable_nitems(HASHTABLE HT)
{
    _HASHTABLE	*ht	= (_HASHTABLE *)HT;

    check(ht);
    return ht->nitems;
}

//  vim: ts=8 sw=4
