#ifndef ALLOC_SHIM_H
#define ALLOC_SHIM_H

#include <stdlib.h>

#ifdef TEST_ALLOC

void *test_malloc(size_t);
void *test_realloc(void *, size_t);
void *test_calloc(size_t, size_t);

#define malloc(s) test_malloc(s)
#define realloc(p, s) test_realloc((p), (s))
#define calloc(n, s) test_calloc((n), (s))

#endif

#endif
