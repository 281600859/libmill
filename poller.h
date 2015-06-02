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

#ifndef MILL_POLLER_INCLUDED
#define MILL_POLLER_INCLUDED

/* Pull in FDW_IN/OUT/ERR constants. */
#include "libmill.h"

#include "slist.h"

#include <stdint.h>

struct mill_timer;

typedef void(*mill_timer_cb)(struct mill_timer *self);

struct mill_timer {
    /* Item of the global list of timers. */
    struct mill_slist_item item;
    /* The timepoint when the timer expires. */
    uint64_t expiry;
    /* Callback to invoke on timer expiration. */
    mill_timer_cb cb;
};

/* Installs new timer. */
void mill_timer(struct mill_timer *self, long ms, mill_timer_cb cb);

struct mill_poll;

typedef void(*mill_poll_cb)(struct mill_poll *self, int events);

struct mill_poll {
    /* Callback to invoke when the file descriptor signals. */
    mill_poll_cb cb;
};

/* Starts waiting for events from a specified file descriptor. */
void mill_poll(struct mill_poll *self, int fd, int events, mill_poll_cb cb);

/* Wait till at least one callback is invoked. */
void mill_wait(void);

#endif

