#include "cnetsupport.h"
#include "cnetsupportprivate.h"

// The cnet network simulator (v3.3.1)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#define	MAGIC	0xDEADBEEF

typedef struct _e {
    char	*data;
    size_t	len;
    struct _e	*next;
} ELEMENT;

typedef struct {
    int32_t	magic;
    ELEMENT	*head;
    ELEMENT	*tail;
    int		nitems;
} _QUEUE;

#define check(q) if(q == NULL || q->magic != MAGIC) {	\
	fprintf(stderr, "invalid QUEUE passed to %s()\n", __func__); exit(1); }

/* -------------------------------------------------------------------- */

QUEUE queue_new(void)
{
    _QUEUE *q = calloc(1, sizeof(_QUEUE));

    CHECKALLOC(q);
    q->magic	= MAGIC;
    return ((QUEUE)q);
}

void queue_free(QUEUE Q)
{
    _QUEUE	*q	= (_QUEUE *)Q;
    ELEMENT	*e1, *e2;

    check(q);
    e1	= q->head;
    while(e1) {
	e2	= e1;
	e1	= e1->next;
	free(e2->data);
	free(e2);
    }
    free(q);
}

void queue_add(QUEUE Q, void *data, size_t len)
{
    _QUEUE	*q	= (_QUEUE *)Q;
    ELEMENT	*newe;

    check(q);
    newe		= calloc(1, sizeof(ELEMENT));
    CHECKALLOC(newe);
    newe->data		= malloc(len);
    CHECKALLOC(newe->data);
    memcpy(newe->data, data, len);
    newe->len		= len;
    newe->next		= NULL;

    if(q->head == NULL) {
	q->head		= newe;
	q->tail		= newe;
    }
    else {
	q->tail->next	= newe;
	q->tail		= newe;
    }
    q->nitems++;
}

void *queue_remove(QUEUE Q, size_t *len)
{
    _QUEUE	*q	= (_QUEUE *)Q;
    ELEMENT     *olde;
    void	*rtn;

    check(q);
    if(q->head == NULL)
        return NULL;

    olde	= q->head;
    q->head	= q->head->next;
    if(q->tail == olde) {
        q->head = NULL;
        q->tail = NULL;
    }
    q->nitems--;

    rtn		= olde->data;
    if(len)
	*len	= olde->len;
    free(olde);
    return(rtn);
}

void *queue_peek(QUEUE Q, size_t *len)
{
    _QUEUE	*q	= (_QUEUE *)Q;

    check(q);
    if(q->head == NULL)
        return NULL;

    if(len)
	*len	= q->head->len;
    return(q->head->data);
}

int queue_nitems(QUEUE Q)
{
    _QUEUE	*q	= (_QUEUE *)Q;

    check(q);
    return(q->nitems);
}

//  vim: ts=8 sw=4
