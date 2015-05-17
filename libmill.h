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

#ifndef MILL_H_INCLUDED
#define MILL_H_INCLUDED

#include <alloca.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>	

/******************************************************************************/
/*  ABI versioning support                                                    */
/******************************************************************************/

/*  Don't change this unless you know exactly what you're doing and have      */
/*  read and understand the following documents:                              */
/*  www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html     */
/*  www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html  */

/*  The current interface version. */
#define MILL_VERSION_CURRENT 0

/*  The latest revision of the current interface. */
#define MILL_VERSION_REVISION 0

/*  How many past interface versions are still supported. */
#define MILL_VERSION_AGE 0

/******************************************************************************/
/*  Symbol visibility                                                         */
/******************************************************************************/

#if defined MILL_NO_EXPORTS
#   define MILL_EXPORT
#else
#   if defined _WIN32
#      if defined MILL_EXPORTS
#          define MILL_EXPORT __declspec(dllexport)
#      else
#          define MILL_EXPORT __declspec(dllimport)
#      endif
#   else
#      if defined __SUNPRO_C
#          define MILL_EXPORT __global
#      elif (defined __GNUC__ && __GNUC__ >= 4) || \
             defined __INTEL_COMPILER || defined __clang__
#          define MILL_EXPORT __attribute__ ((visibility("default")))
#      else
#          define MILL_EXPORT
#      endif
#   endif
#endif

/******************************************************************************/
/*  Coroutines                                                                */
/******************************************************************************/

MILL_EXPORT extern volatile int mill_unoptimisable1;
MILL_EXPORT extern volatile void *mill_unoptimisable2;

MILL_EXPORT void *mill_go_prologue(void);
MILL_EXPORT void mill_go_epilogue(void);

#define go(fn) \
    do {\
        void *mill_sp = mill_go_prologue();\
        if(mill_sp) {\
            int mill_anchor[mill_unoptimisable1];\
            mill_unoptimisable2 = &mill_anchor;\
            char mill_filler[(char*)&mill_anchor - (char*)(mill_sp)];\
            mill_unoptimisable2 = &mill_filler;\
            fn;\
            mill_go_epilogue();\
        }\
    } while(0)

MILL_EXPORT void yield(void);

MILL_EXPORT void msleep(unsigned long ms);

#define FDW_IN 1
#define FDW_OUT 2
#define FDW_ERR 4
MILL_EXPORT int fdwait(int fd, int events);

MILL_EXPORT void *cls(void);
MILL_EXPORT void setcls(void *val);

/******************************************************************************/
/*  Channels                                                                  */
/******************************************************************************/

typedef struct chan *chan;

struct mill_cp;
struct mill_ep;

/* This structure represents a single clause in a choose statement.
   Similarly, both chs() and chr() each create a single clause. */
struct mill_clause {
    /* Double-linked list of clauses waiting for a channel endpoint. */
    struct mill_clause *prev;
    struct mill_clause *next;
    /* The coroutine which created the clause. */
    struct mill_cr *cr;
    /* Channel endpoint the clause is waiting for. */
    struct mill_ep *ep;
    /* For out clauses, pointer to the value to send. For in clauses it is
       either point to the value to receive to or NULL. In the latter case
       the value should be received into coroutines in buffer. */
    void *val;
    /* The index to jump to when the clause is executed. */
    int idx;
    /* If 0, there's no peer waiting for the clause at the moment.
       If 1, there is one. */
    int available;
    /* Linked list of clauses in the choose statement. */
    struct mill_clause *next_clause;
};

#define chmake(type, bufsz) mill_chmake(sizeof(type), bufsz)

#define chs(channel, type, value) \
    do {\
        type mill_val = (value);\
        mill_chs((channel), &mill_val, sizeof(type));\
    } while(0)

#define chr(channel, type) \
    (*(type*)mill_chr((channel), alloca(sizeof(type)), sizeof(type)))

#define chdone(channel, type, value) \
    do {\
        type mill_val = (value);\
        mill_chdone((channel), &mill_val, sizeof(type));\
    } while(0)

MILL_EXPORT chan mill_chmake(size_t sz, size_t bufsz);
MILL_EXPORT chan chdup(chan ch);
MILL_EXPORT void mill_chs(chan ch, void *val, size_t sz);
MILL_EXPORT void *mill_chr(chan ch, void *val, size_t sz);
MILL_EXPORT void mill_chdone(chan ch, void *val, size_t sz);
MILL_EXPORT void chclose(chan ch);

#define mill_concat(x,y) x##y

#define choose \
    {\
        int mill_idx = -2;\
        while(1) {\
            if(mill_idx != -2) {\
                if(0)

#define mill_in(chan, type, name, idx) \
                    break;\
                }\
                goto mill_concat(mill_label, idx);\
            }\
            struct mill_clause mill_concat(mill_clause, idx);\
            mill_choose_in(\
                &mill_concat(mill_clause, idx),\
                (chan),\
                sizeof(type),\
                idx);\
            if(0) {\
                type name;\
                mill_concat(mill_label, idx):\
                if(mill_idx == idx) {\
                    name = *(type*)mill_choose_val();\
                    mill_concat(mill_dummylabel, idx)

#define in(chan, type, name) mill_in((chan), type, name, __COUNTER__)

#define mill_out(chan, type, val, idx) \
                    break;\
                }\
                goto mill_concat(mill_label, idx);\
            }\
            struct mill_clause mill_concat(mill_clause, idx);\
            type mill_concat(mill_val, idx) = (val);\
            mill_choose_out(\
                &mill_concat(mill_clause, idx),\
                (chan),\
                &mill_concat(mill_val, idx),\
                sizeof(type),\
                idx);\
            if(0) {\
                mill_concat(mill_label, idx):\
                if(mill_idx == idx) {\
                    mill_concat(mill_dummylabel, idx)

#define out(chan, type, val) mill_out((chan), type, (val), __COUNTER__)

#define mill_otherwise(idx) \
                    break;\
                }\
                goto mill_concat(mill_label, idx);\
            }\
            mill_choose_otherwise();\
            if(0) {\
                mill_concat(mill_label, idx):\
                if(mill_idx == -1) {\
                    mill_concat(mill_dummylabel, idx)

#define otherwise mill_otherwise(__COUNTER__)

#define end \
                    break;\
                }\
            }\
            mill_idx = mill_choose_wait();\
        }

MILL_EXPORT void mill_choose_in(struct mill_clause *clause,
    chan ch, size_t sz, int idx);
MILL_EXPORT void mill_choose_out(struct mill_clause *clause,
    chan ch, void *val, size_t sz, int idx);
MILL_EXPORT void mill_choose_otherwise(void);
MILL_EXPORT int mill_choose_wait(void);
MILL_EXPORT void *mill_choose_val(void);

/******************************************************************************/
/*  Experimental                                                              */
/******************************************************************************/

typedef struct tcplistener *tcplistener;
typedef struct tcpconn *tcpconn;

MILL_EXPORT tcplistener tcplisten(const struct sockaddr *addr,
    socklen_t addrlen);
MILL_EXPORT tcpconn tcpaccept(tcplistener listener);
MILL_EXPORT void tcplistener_close(tcplistener listener);
MILL_EXPORT tcpconn tcpconnect(const struct sockaddr *addr, socklen_t addrlen);
MILL_EXPORT void tcpconn_close(tcpconn conn);
MILL_EXPORT void tcpwrite(tcpconn conn, const void *buf, size_t len);
MILL_EXPORT int tcpflush(tcpconn conn);
MILL_EXPORT ssize_t tcpread(tcpconn conn, void *buf, size_t len);
MILL_EXPORT ssize_t tcpreaduntil(tcpconn conn, void *buf, size_t len,
    char until);

#endif

