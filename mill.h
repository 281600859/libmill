/*
    Copyright (c) 2014 Martin Sustrik  All rights reserved.

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

#ifndef mill_h_included
#define mill_h_included

#include <assert.h>
#include <stdlib.h>
#include <uv.h>

/******************************************************************************/
/*  Coroutine metadata.                                                       */
/******************************************************************************/

/* Constant tag is used to make sure that the coframe is valid. If its type
   member doesn't point to something that begins with MILL_TYPE_TAG, the
   coframe can be considered invalid. */
#define mill_type_tag 0x4efd36df

/*  Special events. */
#define mill_event_init ((void*) 0)
#define mill_event_term ((void*) -1)

/* 'coframe' points to the coframe of the coroutine being evaluated.
   'event' either points to the coframe of the child coroutine that have just
   terminated or is one of the special events listed above. */
typedef void (*mill_fn_handler) (void *cfptr, void *event);

struct mill_type {
    int tag;
    mill_fn_handler handler;
};

/******************************************************************************/
/* Coframe head -- common to all coroutines.                                  */
/******************************************************************************/

/* These flags are used to construct 'flags' member. */
#define mill_flag_deallocate 1
#define mill_flag_canceled 2

struct mill_cfh {
    const struct mill_type *type;
    int state;
    int flags;
    int err;
    struct mill_cfh *parent;
    struct mill_cfh *children;
    struct mill_cfh *next;
    struct mill_cfh *prev;
    struct mill_loop *loop;
};

void mill_cfh_init (
    struct mill_cfh *self,
    const struct mill_type *type,
    struct mill_loop *loop,
    struct mill_cfh *parent,
    int flags);

void mill_cfh_getresult (struct mill_cfh *cfh, void **who, int *err);

/******************************************************************************/
/* The event loop. */
/******************************************************************************/

struct mill_loop
{
    /* Underlying libuv loop. */
    uv_loop_t uv_loop;

    /* Libuv hook that processes the mill events. */
    uv_idle_t idle;

    /* Local event queue. Items in this list are processed immediately,
       before control is returned to libuv. */
    struct mill_cfh *first;
    struct mill_cfh *last;
};

void mill_loop_init (struct mill_loop *self);
void mill_loop_term (struct mill_loop *self);
void mill_loop_run (struct mill_loop *self);
void mill_loop_emit (struct mill_loop *self, struct mill_cfh *base);

/******************************************************************************/
/*  Mill keywords.                                                            */
/******************************************************************************/

/*  wait  */

#define mill_wait(statearg, whoarg, errarg)\
    do {\
        cf->mill_cfh.state = (statearg);\
        return;\
        mill_state##statearg:\
        mill_cfh_getresult (&cf->mill_cfh, (whoarg), (errarg));\
    } while (0)

/*  raise  */

#define mill_raise(errarg)\
    do {\
        cf->mill_cfh.err = (errarg);\
        goto mill_finally;\
    } while (0)

/*  return  */

#define mill_return mill_raise (0)

/*  cancel  */

void mill_cancel (void *cfptr);

/*  cancelall  */

#define mill_cancelall(statearg)\
    do {\
        mill_cfh_cancelall (&cf->mill_cfh);\
        while (1) {\
            if (!mill_cfh_haschildren (&cf->mill_cfh))\
                break;\
            cf->mill_cfh.state = (statearg);\
            return;\
            mill_state##statearg:\
            ;\
        }\
    } while (0)

/*  typeof  */

void *mill_typeof (void *cfptr);

/******************************************************************************/
/* coroutine msleep (int milliseconds)                                        */
/******************************************************************************/

extern const struct mill_type mill_type_msleep;

struct mill_cf_msleep {

    /* Generic coframe header. */
    struct mill_cfh mill_cfh;

    /* Local variables. */
    uv_timer_t timer;
};

void *mill_call_msleep (
    void *cf,
    const struct mill_type *type,
    struct mill_loop *loop,
    void *parent,
    int millseconds);

int msleep (int milliseconds);

#endif

