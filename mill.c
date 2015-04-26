/*

  Copyright (c) 2015 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include "mill.h"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/*  Utilities                                                                 */
/******************************************************************************/

/*  Takes a pointer to a member variable and computes pointer to the structure
    that contains it. 'type' is type of the structure, not the member. */
#define cont(ptr, type, member) \
    (ptr ? ((type*) (((char*) ptr) - offsetof(type, member))) : NULL)

/******************************************************************************/
/*  Coroutines                                                                */
/******************************************************************************/

#define STACK_SIZE 16384

struct cr {
    struct cr *next;
    jmp_buf ctx;
};

/* Fake coroutine corresponding to the main thread of execution. */
struct cr main_cr = {NULL};

/* The queue of coroutines ready to run. The first one is currently running. */
struct cr *first_cr = &main_cr;
struct cr *last_cr = &main_cr;

/* Removes current coroutine from the queue and returns it to the calller. */
static struct cr *suspend() {
    struct cr *cr = first_cr;
    first_cr = first_cr->next;
    if(!first_cr)
        last_cr = NULL;
    cr->next = NULL;
    return cr;
}

/* Schedules preiously suspended coroutine for execution. */
static void resume(struct cr *cr) {
    cr->next = NULL;
    if(last_cr)
        last_cr->next = cr;
    else
        first_cr = cr;
    last_cr = cr;
}

/* Switch to a different coroutine. */
static void ctxswitch(void) {
    /* There are no ready coroutines. Deadlock. */
    assert(first_cr);

    longjmp(first_cr->ctx, 1);
}

/* The intial part of go(). Allocates the stack for the new coroutine. */
void *mill_go_prologue() {
    if(setjmp(first_cr->ctx))
        return NULL;
    char *ptr = malloc(STACK_SIZE);
    assert(ptr);
    struct cr *cr = (struct cr*)(ptr + STACK_SIZE - sizeof (struct cr));
    cr->next = first_cr;
    first_cr = cr;
    return (void*)cr;
}

/* The final part of go(). Cleans up when the coroutine is finished. */
void mill_go_epilogue(void) {
    struct cr *cr = first_cr;
    first_cr = first_cr->next;
    char *ptr = ((char*)(cr + 1)) - STACK_SIZE;
    free(ptr);
    ctxswitch();    
}

/* Move the current coroutine to the end of the queue.
   Pass control to the new head of the queue. */
void yield(void) {
    if(first_cr == last_cr)
        return;
    if(setjmp(first_cr->ctx))
        return;
    resume(suspend());
    ctxswitch();
}

/******************************************************************************/
/*  Basic channels                                                            */
/******************************************************************************/

/* Channel endpoint. */
struct ep {
    enum {SENDER, RECEIVER} type;
    struct cr *cr;
    void **val;
    int idx;
    struct ep *next;
};

/* Unbuffered channel. */
struct chan {
    struct ep sender;
    struct ep receiver;
    int refcount;
};

static struct ep *mill_getpeer(struct ep *ep) {
    switch(ep->type) {
    case SENDER:
        return &cont(ep, struct chan, sender)->receiver;
    case RECEIVER:
        return &cont(ep, struct chan, receiver)->sender;
    default:
        assert(0);
    }
}

chan chmake(void) {
    struct chan *ch = (struct chan*)malloc(sizeof(struct chan));
    assert(ch);
    ch->sender.type = SENDER;
    ch->sender.cr = NULL;
    ch->sender.val = NULL;
    ch->sender.idx = -1;
    ch->sender.next = NULL;
    ch->receiver.type = RECEIVER;
    ch->receiver.cr = NULL;
    ch->receiver.val = NULL;
    ch->receiver.idx = -1;
    ch->receiver.next = NULL;
    ch->refcount = 2;
    return ch;
}

void chaddref(chan ch) {
    ++ch->refcount;
}

void chs(chan ch, void *val) {
    /* Only one coroutine can send at a time. */
    assert(!ch->sender.cr);

    /* If there's a receiver already waiting, we can just unblock it. */
    if(ch->receiver.cr) {
        *(ch->receiver.val) = val;
        resume(ch->receiver.cr);
        ch->receiver.cr = NULL;
        ch->receiver.val = NULL;
        return;
    }

    /* Otherwise we are going to yield till the receiver arrives. */
    if(setjmp(first_cr->ctx))
        return;
    ch->sender.cr = suspend();
    ch->sender.val = &val;

    /* Pass control to a different coroutine. */
    ctxswitch();
}

void *chr(chan ch) {
    /* Only one coroutine can receive from a channel at a time. */
    assert(!ch->receiver.cr);

    /* If there's a sender already waiting, we can just unblock it. */
    if(ch->sender.cr) {
        void *val = *(ch->sender.val);
        resume(ch->sender.cr);
        ch->sender.cr = NULL;
        ch->sender.val = NULL;
        return val;
    }

    /* Otherwise we are going to yield till the sender arrives. */
    void *val;
    if(setjmp(first_cr->ctx))
        return val;
    ch->receiver.cr = suspend();
    ch->receiver.val = &val;

    /* Pass control to a different coroutine. */
    ctxswitch();
}	

void chclose(chan ch) {
    assert(ch->refcount >= 1);
    --ch->refcount;
    if(!ch->refcount) {
        assert(!ch->sender.cr);
        assert(!ch->receiver.cr);
        free(ch);
    }
}

/*****************************************************************************/
/*  Selecting                                                                */
/*****************************************************************************/

struct ep *mill_chselect_in(struct ep *chlist, chan ch, int idx) {
    assert(!ch->receiver.cr);
    ch->receiver.cr = first_cr;
    ch->receiver.idx = idx;
    ch->receiver.next = chlist;
    return &ch->receiver;
}

struct ep *mill_chselect_out(struct ep *chlist, chan ch, int idx) {
    assert(!ch->sender.cr);
    ch->sender.cr = first_cr;
    ch->sender.idx = idx;
    ch->sender.next = chlist;
    return &ch->sender;
}

int mill_chselect_wait(struct ep *chlist) {
    /* Find out wheter there are any channels that are already available. */
    int available = 0;
    struct ep *it = chlist;
    while(it) {
        if(mill_getpeer(it)->cr)
            ++available;
        it = it->next;
    }
    printf("available: %d\n", available);
    
    /* If so, choose a random one. */
    if(available > 0) {
        int chosen = random() % available;
        printf("chosen: %d\n", chosen);
        int res = -1;
        it = chlist;
        while(it) {
            if(mill_getpeer(it)->cr) {
                if(!chosen)
                    res = it->idx;
                --chosen;
            }
            struct ep *tmp = it;
            it = it->next;
            tmp->cr = NULL;
            tmp->idx = -1;
            tmp->next = NULL;
        }
        assert(res >= 0);
        return res;
    }

    /* If not so, block and wait for an available channel. */
    if(!setjmp(first_cr->ctx)) {
        suspend();
        ctxswitch();
    }
    it = chlist;
    while(it) {
        struct ep *tmp = it;
        it = it->next;
        tmp->cr = NULL;
        tmp->idx = -1;
        tmp->next = NULL;
    }
    return -1;//!
}

