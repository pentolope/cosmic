
#ifndef __STD_ASSERT
#define __STD_ASSERT
#ifdef NDEBUG
#define assert(exp) ((void)0)
#else
#include <stdlib.h>
void _assert(const char* exp,const char* file,unsigned int line){
	fprintf(stderr,"Assertion failed: `%s` in `%s` at line %u\n",exp,file,line);
	exit(134); // technically, assert should call abort(), not exit()
}
#define assert(exp) ((exp)?(void)0:_assert(#exp,__FILE__,__LINE__))
#endif
#endif
