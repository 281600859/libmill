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

#include "slist.h"

#include <assert.h>
#include <stddef.h>

void mill_slist_init(struct mill_slist *self) {
    self->first = NULL;
    self->last = NULL;
}

void mill_slist_term(struct mill_slist *self) {
    assert(!self->first);
    assert(!self->last);
}

int mill_slist_empty(struct mill_slist *self) {
    return self->first ? 0 : 1;
}

struct mill_slist_item *mill_slist_begin(struct mill_slist *self) {
    return self->first;
}

struct mill_slist_item *mill_slist_next(struct mill_slist_item *it) {
    return it->next;
}

void mill_slist_push(struct mill_slist *self, struct mill_slist_item *item) {
    item->next = self->first;
    self->first = item;
    if(!self->last)
        self->last = item;
}

void mill_slist_push_back(struct mill_slist *self,
      struct mill_slist_item *item) {
    item->next = NULL;
    if(!self->last) {
        self->last = item;
        self->first = item;
        return;
    }
    self->last->next = item;
    self->last = item;
}

void mill_slist_insert(struct mill_slist *self, struct mill_slist_item *item,
      struct mill_slist_item *it) {
    if(!it) {
        mill_slist_push(self, item);
        return;
    }
    item->next = it->next;
    if(it == self->last)
        self->last = item;
    it->next = item;
}

struct mill_slist_item *mill_slist_pop(struct mill_slist *self) {
    if(!self->first)
        return NULL;
    struct mill_slist_item *it = self->first;
    self->first = self->first->next;
    if(!self->first)
        self->last = NULL;
    return it;
}

