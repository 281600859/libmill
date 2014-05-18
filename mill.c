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

#include "mill.h"

#include <stdio.h>

/******************************************************************************/
/* Generic stuff.                                                             */
/******************************************************************************/

#define mill_cont(ptr, type, member) \
    (ptr ? ((type*) (((char*) ptr) - offsetof(type, member))) : NULL)

void mill_base_init (
    struct mill_base *self,
    mill_handler_fn handler,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    self->handler = handler;
    self->state = 0;
    self->tag = tag;
    self->err = 0;
    self->parent = parent;
    self->next = 0;
    self->loop = loop;
}

void mill_base_term (struct mill_base *self)
{
    assert (0);
}

void mill_base_emit (struct mill_base *self, int err)
{
    self->err = err;
    mill_loop_emit (self->loop, self);
}

/******************************************************************************/
/* The event loop.                                                            */
/******************************************************************************/

void mill_loop_init (struct mill_loop *self)
{
    int rc;

    rc = uv_loop_init (&self->uv_loop);
    assert (rc == 0);
    self->first = 0;
    self->last = 0;
}

void mill_loop_term (struct mill_loop *self)
{
    //int rc;
    //
    //rc = uv_loop_close (&self->uv_loop);
    //assert (rc == 0);
}

void mill_loop_run (struct mill_loop *self)
{
    int rc;

    while (1) {
        rc = uv_run (&self->uv_loop, UV_RUN_ONCE);
        assert (rc >= 0);
        while (self->first) {
            if (!self->first->parent)
                return;
            self->first->parent->handler (self->first->parent, self->first);
            self->first = self->first->next;
        }
        self->last = 0;
    }
}

void mill_loop_emit (
    struct mill_loop *self,
    struct mill_base *ev)
{
    if (self->first == 0)
        self->first = ev;
    else
        self->last->next = ev;
    ev->next = 0;
    self->last = ev;
}

/******************************************************************************/
/* Alarm.                                                                     */
/******************************************************************************/

static void alarm_handler(struct mill_base *base, struct mill_base *event)
{
    struct mill_coroutine_alarm *self = (struct mill_coroutine_alarm*) base;
    assert (0);
}

static void alarm_cb (uv_timer_t *timer)
{
    struct mill_coroutine_alarm *self = mill_cont (timer,
        struct mill_coroutine_alarm, timer);
    mill_base_emit (&self->mill_base, 0);
}

void mill_call_alarm (
    struct mill_coroutine_alarm *self,
    int milliseconds,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    mill_base_init (&self->mill_base, alarm_handler, parent, loop, tag);
    uv_timer_init(&loop->uv_loop, &self->timer);
    uv_timer_start(&self->timer, alarm_cb, milliseconds, 0);
}

/******************************************************************************/
/* TCP socket.                                                                */
/******************************************************************************/

int tcpsocket_init (
    struct tcpsocket *self,
    struct mill_loop *loop)
{
    self->loop = &loop->uv_loop;
    self->accept = 0;
    self->term = 0;
    return uv_tcp_init (&loop->uv_loop, &self->s);
}

static void tcpsocket_term_handler (
    struct mill_base *base,
    struct mill_base *event)
{
    struct mill_coroutine_tcpsocket_term *self =
        (struct mill_coroutine_tcpsocket_term*) base;
    assert (0);
}

static void tcpsocket_term_cb (
    uv_handle_t* handle)
{
    struct tcpsocket *self = mill_cont (handle, struct tcpsocket, s);

    assert (self->term);
    mill_base_emit (&self->term->mill_base, 0);
    self->term = 0;
}

void mill_call_tcpsocket_term (
    struct mill_coroutine_tcpsocket_term *self,
    struct tcpsocket *s,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    mill_base_init (&self->mill_base, tcpsocket_term_handler,
        parent, loop, tag);
    self->s = s;
    uv_close ((uv_handle_t*) &s->s, tcpsocket_term_cb);
}

static void tcpsocket_connect_handler (
    struct mill_base *base,
    struct mill_base *event)
{
    struct mill_coroutine_tcpsocket_connect *self =
        (struct mill_coroutine_tcpsocket_connect*) base;
    assert (0);
}

static void tcpsocket_connect_cb (
    uv_connect_t* req,
    int status)
{
    struct mill_coroutine_tcpsocket_connect *self = mill_cont (req,
        struct mill_coroutine_tcpsocket_connect, conn);

    assert (status == 0);
    mill_base_emit (&self->mill_base, 0);
}

void mill_call_tcpsocket_connect (
    struct mill_coroutine_tcpsocket_connect *self,
    struct tcpsocket *s,
    struct sockaddr *addr,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    int rc;

    mill_base_init (&self->mill_base, tcpsocket_connect_handler,
        parent, loop, tag);
    rc = uv_tcp_connect (&self->conn, &s->s, addr, tcpsocket_connect_cb);
    assert (rc == 0);
}

int tcpsocket_bind (
    struct tcpsocket *s,
    struct sockaddr *addr,
    int flags)
{
    return uv_tcp_bind(&s->s, addr, flags);
}

static void tcpsocket_listen_cb (
    uv_stream_t *s,
    int status)
{
    int rc;
    struct tcpsocket *self;
    uv_tcp_t uvs;

    self = mill_cont (s, struct tcpsocket, s);

    /* If nobody is accepting connections at the moment
       we'll simply drop them. */
    if (!self->accept) {
        rc = uv_tcp_init (s->loop, &uvs);
        assert (rc == 0);
        rc = uv_accept (s, (uv_stream_t*) &uvs);
        assert (rc == 0);
        uv_close ((uv_handle_t*) &s, NULL); // TODO: This is an async op!
        return; 
    }

    /* Actual accept. */
    rc = uv_accept (s, (uv_stream_t*) &self->accept->s->s);
    assert (rc == 0);
    mill_base_emit (&self->accept->mill_base, 0);
    self->accept = 0;
}

int tcpsocket_listen (
    struct tcpsocket *self,
    int backlog,
    struct mill_loop *loop)
{
    int rc;

    rc = uv_listen((uv_stream_t*) &self->s, backlog, tcpsocket_listen_cb);
    assert (rc == 0);
}

static void tcpsocket_accept_handler (
    struct mill_base *base,
    struct mill_base *event)
{
    struct mill_coroutine_tcpsocket_accept *self =
        (struct mill_coroutine_tcpsocket_accept*) base;
    assert (0);
}

void mill_call_tcpsocket_accept (
    struct mill_coroutine_tcpsocket_accept *self,
    struct tcpsocket *ls,
    struct tcpsocket *s,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    mill_base_init (&self->mill_base, tcpsocket_accept_handler,
        parent, loop, tag);
    self->s = s;
    ls->accept = self;
}

static void tcpsocket_send_handler(
    struct mill_base *base,
    struct mill_base *event)
{
    struct mill_coroutine_tcpsocket_send *self =
        (struct mill_coroutine_tcpsocket_send*) base;
    assert (0);
}

static void tcpsocket_send_cb (
    uv_write_t* req,
    int status)
{
printf ("send_cb\n");
    struct mill_coroutine_tcpsocket_send *self = mill_cont (req,
        struct mill_coroutine_tcpsocket_send, req);

    assert (status == 0);
    mill_base_emit (&self->mill_base, 0);
}

void mill_call_tcpsocket_send (
    struct mill_coroutine_tcpsocket_send *self,
    struct tcpsocket *s,
    const void *buf,
    size_t len,
    struct mill_base *parent,
    struct mill_loop *loop,
    int tag)
{
    int rc;

    mill_base_init (&self->mill_base, tcpsocket_send_handler,
        parent, loop, tag);
    self->buf.base = (void*) buf;
    self->buf.len = len;
    rc = uv_write (&self->req, (uv_stream_t*) &s->s, &self->buf, 1,
        tcpsocket_send_cb);
    assert (rc == 0);
}

