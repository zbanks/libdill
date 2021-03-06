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

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cr.h"
#include "handle.h"
#include "libdill.h"
#include "utils.h"

#define CHECKHANDLE(h, err) \
    if(dill_slow((h) < 0 || (h) >= dill_nhandles ||\
          dill_handles[(h)].next != -2)) {\
        errno = EBADF; return (err);}\
    struct dill_handle *hndl = &dill_handles[(h)];

#define CHECKHANDLEVOID(h) \
    if(dill_slow((h) < 0 || (h) >= dill_nhandles ||\
          dill_handles[(h)].next != -2)) {\
        errno = EBADF; return;}\
    struct dill_handle *hndl = &dill_handles[(h)];

#define CHECKHANDLEASSERT(h) \
    dill_assert(!dill_slow((h) < 0 && (h) < dill_nhandles &&\
          dill_handles[(h)].next == -2));\
    struct dill_handle *hndl = &dill_handles[(h)];

static struct dill_handle *dill_handles = NULL;
static int dill_nhandles = 0;
static int dill_unused = -1;

static void dill_handle_atexit(void) {
    if(dill_handles)
        free(dill_handles);
}

int dill_handle(const void *type, void *data, const struct hvfptrs *vfptrs,
      const char *created) {
    if(dill_slow(!type || !data || !vfptrs)) {errno = EINVAL; return -1;}
    /* Check mandatory virtual functions. */
    if(dill_slow(!vfptrs->close)) {errno = EINVAL; return -1;}
    /* If there's no space for the new handle expand the array. */
    if(dill_slow(dill_unused == -1)) {
        /* Start with 256 handles, double the size when needed. */
        int sz = dill_nhandles ? dill_nhandles * 2 : 256;
        struct dill_handle *hndls =
            realloc(dill_handles, sz * sizeof(struct dill_handle));
        if(dill_slow(!hndls)) {errno = ENOMEM; return -1;}
        /* Clean-up function to delete the array at exit. It is not strictly
           necessary but valgrind will be happy about it. */
        if(dill_slow(!dill_handles)) {
            int rc = atexit(dill_handle_atexit);
            dill_assert(rc == 0);
        }
        /* Add newly allocated handles to the list of unused handles. */
        int i;
        for(i = dill_nhandles; i != sz - 1; ++i)
            hndls[i].next = i + 1;
        hndls[sz - 1].next = -1;
        dill_unused = dill_nhandles;
        /* Adjust the array. */
        dill_handles = hndls;
        dill_nhandles = sz;
    }
    /* Return first handle from the list of unused hadles. */
    int h = dill_unused;
    dill_unused = dill_handles[h].next;
    dill_handles[h].type = type;
    dill_handles[h].data = data;
    dill_handles[h].refcount = 1;
    dill_handles[h].vfptrs = *vfptrs;
    dill_handles[h].created = created;
    dill_handles[h].next = -2;
    return h;
}

int hdup(int h) {
    CHECKHANDLE(h, -1);
    ++hndl->refcount;
    return h;
}

void *hdata(int h, const void *type) {
    CHECKHANDLE(h, NULL);
    if(dill_slow(type && hndl->type != type)) {errno = ENOTSUP; return NULL;}
    return hndl->data;
}

void hdump(int h) {
    CHECKHANDLEVOID(h);
    fprintf(stderr, "Handle:{%d} Type:%p Data:%p Refcount:%d Created: %s\n",
        h, hndl->type, hndl->data, hndl->refcount, hndl->created);
    if(hndl->vfptrs.dump)
        hndl->vfptrs.dump(h);
}

int hclose(int h) {
    CHECKHANDLE(h, -1);
    /* If there are multiple duplicates of this handle just remove one
       reference. */
    if(hndl->refcount > 1) {
        --hndl->refcount;
        return 0;
    }
    /* This will guarantee that blocking functions cannot be called anywhere
       inside the context of the close. */
    int was_stopping = dill_running->stopping;
    dill_running->stopping = 1;
    /* Send stop signal to the handle. */
    dill_assert(hndl->vfptrs.close);
    hndl->vfptrs.close(h);
    dill_running->stopping = was_stopping;
    /* Better be paraniod and delete the function pointer here. */
    hndl->vfptrs.close = NULL;
    /* Return the handle to the shared pool. */
    hndl->next = dill_unused;
    dill_unused = h;
    return 0;
}

void dill_handle_done(int h) {
    CHECKHANDLEASSERT(h);
    hndl->data = NULL;
}

void goredump(void) {
    if(dill_slow(!dill_handles)) return;
    int i;
    for(i = 0; i != dill_nhandles; ++i) {
        if(dill_handles[i].next == -2)
            hdump(i);
    }
}

