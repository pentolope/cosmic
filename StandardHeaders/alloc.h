
#ifndef __STD_ALLOC
#define __STD_ALLOC

#ifdef __STD_REAL

static void* malloc(unsigned long size);
static void* calloc(unsigned long nitems, unsigned long size);
static void free(void* ptr);
static void* realloc(void* ptr, unsigned long size);

#include <alloc.c>

#else // #ifdef __STD_REAL

extern void* malloc(unsigned long size);
extern void* calloc(unsigned long nitems, unsigned long size);
extern void free(void* ptr);
extern void* realloc(void* ptr, unsigned long size);

#endif // #ifdef __STD_REAL

#endif // #ifndef __STD_ALLOC

