/*

  Copyright (c) 2016 Martin Sustrik

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

#ifndef DILL_HANDLE_INCLUDED
#define DILL_HANDLE_INCLUDED

#include "libdill.h"

struct dill_handle {
    /* Implemetor-specified type of the handle. */
    const void *type;
    /* Opaque implemetor-specified pointer. */
    void *data;
    /* Number of duplicates of this handle. */
    int refcount;
    /* Table of virtual functions. */
    struct hvfptrs vfptrs;
    /* The location where the handle was created. The string it points to must
       be static. */
    const char *created;
    /* Index of the next handle in the linked list of unused handles. -1 means
       'end of the list'. -2 means 'active handle'. */
    int next;
};

void dill_handle_done(int h);

#endif

