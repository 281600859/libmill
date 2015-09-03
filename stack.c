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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include "slist.h"
#include "stack.h"
#include "utils.h"

/* Size of stack for all coroutines. In bytes. Default size is slightly smaller
   than page size to account for malloc's chunk header. */
size_t mill_stack_size = 256 * 1024 - 256;

/* Maximum number of unused cached stacks. Keep in mind that we can't
   deallocate the stack you are running on. Thus we need at least one cached
   stack. */
int mill_max_cached_stacks = 64;

/* A stack of unused coroutine stacks. This allows for extra-fast allocation
   of a new stack. The FIFO nature of this structure minimises cache misses.
   When the stack is cached its mill_slist_item is placed on its top rather
   then on the bottom. That way we minimise page misses. */
static int mill_num_cached_stacks = 0;
static struct mill_slist mill_cached_stacks = {0};

static void *mill_allocstackmem(void) {
#if defined HAVE_POSIX_MEMALIGN && HAVE_MPROTECT
    /* Find out what the memory page size is. */
    long pgsz = sysconf(_SC_PAGE_SIZE);
    mill_assert(pgsz > 0);
    size_t pagesize = (size_t)pgsz;
    mill_assert(mill_stack_size > pagesize);
    /* Amount of memory allocated must be multiply of the page size otherwise
       the behaviour of posx_memalign() is undefined. */
    size_t sz = (mill_stack_size + pagesize - 1) & ~(pagesize - 1);
    /* Allocate one additional guard page. */
    sz += pagesize;
    /* Allocate the stack so that it's memory-page-aligned. */
    void *ptr;
    int rc = posix_memalign(&ptr, pagesize, sz);
    if(rc != 0) {
        errno = rc;
        return NULL;
    }
    /* The bottom page is used as a stack guard. This way stack overflow will
       cause segfault rather than randomly overwrite the heap. */
    rc = mprotect(ptr, pagesize, PROT_NONE);
    if(rc != 0) {
        int err = errno;
        free(ptr);
        errno = err;
        return NULL;
    }
    return (void*)(((char*)ptr) + sz);
#else
    return malloc(mill_stack_size);
#endif
}

int mill_preparestacks(int count, size_t stack_size) {
    /* Purge the cached stacks. */
    while(1) {
        struct mill_slist_item *item = mill_slist_pop(&mill_cached_stacks);
        if(!item)
            break;
        free(((char*)(item + 1)) - mill_stack_size);
    }
    /* Now that there are no stacks allocated, we can adjust the stack size. */
    mill_stack_size = stack_size;
    /* Make sure that the stacks won't get deallocated even if they aren't used
       at the moment. */
    if(count > mill_max_cached_stacks)
        mill_max_cached_stacks = count;
    /* Allocate the new stacks. */
    int i;
    for(i = 0; i != count; ++i) {
        void *ptr = mill_allocstackmem();
        if(!ptr)
            break;
        struct mill_slist_item *item = ((struct mill_slist_item*)ptr) - 1;
        mill_slist_push_back(&mill_cached_stacks, item);
    }
    return i;
}

void *mill_allocstack(void) {
    if(!mill_slist_empty(&mill_cached_stacks)) {
        --mill_num_cached_stacks;
        return (void*)(mill_slist_pop(&mill_cached_stacks) + 1);
    }
    void *ptr = mill_allocstackmem();
    if(!ptr)
        mill_panic("not enough memory to allocate coroutine stack");
    return ptr;
}

void mill_freestack(void *stack) {
    /* Put the stack to the list of cached stacks. */
    struct mill_slist_item *item = ((struct mill_slist_item*)stack) - 1;
    mill_slist_push_back(&mill_cached_stacks, item);
    if(mill_num_cached_stacks < mill_max_cached_stacks) {
        ++mill_num_cached_stacks;
        return;
    }
    /* We can't deallocate the stack we are running on at the moment.
       Standard C free() is not required to work when it deallocates its
       own stack from underneath itself. Instead, we'll deallocate one of
       the unused cached stacks. */
    item = mill_slist_pop(&mill_cached_stacks);  
    free(((char*)(item + 1)) - mill_stack_size);
}

