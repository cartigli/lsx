#ifndef TEST_BUFFER_H
#define TEST_BUFFER_H

#include <stdio.h>
#include "buff.h"

#ifdef TEST_ALLOC
#include <stdlib.h>

void *test_malloc(size_t);
void *test_realloc(void *, size_t);
void *test_calloc(size_t, size_t);

// #define malloc(s) test_malloc(s)
// #define realloc(p, s) test_realloc((p), (s))
// #define calloc(n, s) test_calloc((n), (s))

extern int g_fail_at;
extern int g_alloc_n;

#endif

#endif
