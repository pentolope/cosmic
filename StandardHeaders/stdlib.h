
#ifndef __STD_LIB
#define __STD_LIB

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __STD_REAL

void abort(void);
void exit(int);

#else // #ifdef __STD_REAL

extern void abort(void);
extern void exit(int);

#endif // #ifdef __STD_REAL

#include "printf_.h"
#include "alloc.h"

#endif
