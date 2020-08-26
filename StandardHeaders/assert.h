
#ifndef __STD_ASSERT
#define __STD_ASSERT
#ifdef NDEBUG
#define assert(exp) ((void)0)
#else
static void _assert(const char* exp,const char* file,unsigned int line);
#define assert(exp) ((exp)?(void)0:_assert(#exp,__FILE__,__LINE__))
#include <stdlib.h>
static void _assert(const char* exp,const char* file,unsigned int line){
	fprintf(stderr,"Assertion failed: `%s` in `%s` at line %u\n",exp,file,line);
	abort(); // 134 will be return value
}
#endif
#endif
