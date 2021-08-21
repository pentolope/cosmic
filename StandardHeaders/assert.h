
#ifndef __STD_ASSERT
#define __STD_ASSERT
#ifdef NDEBUG
#define assert(exp) ((void)0)
#else // #ifdef NDEBUG
#define assert(exp) ((exp)?(void)0:_assert(#exp,__FILE__,__LINE__))

#ifdef __STD_REAL
void _assert(const char* exp,const char* file,unsigned int line);
#include <stdlib.h>
void _assert(const char* exp,const char* file,unsigned int line){
	fprintf(stderr,"Assertion failed: `%s` in `%s` at line %u\n",exp,file,line);
	abort(); // 134 will be return value
}
#else // #ifdef __STD_REAL
extern void _assert(const char* exp,const char* file,unsigned int line);
#endif // #ifdef __STD_REAL

#endif // #ifdef NDEBUG
#endif // #ifndef __STD_ASSERT
