

#ifndef __STD_LIB
#define __STD_LIB

#ifndef NULL
#define NULL ((void*)0)
#endif


#ifdef __STD_REAL

static void abort(void);
static void exit(int);

#else // #ifdef __STD_REAL

extern void abort(void);
extern void exit(int);

#endif // #ifdef __STD_REAL

#include "alloc.h"
#include "printf_.h"

#endif




