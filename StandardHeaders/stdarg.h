
/*
totally untested, but I think this should work.  EDIT: I think this will not work, the casting is wrong when adding to pointers

these definitions do no runtime checks
  and allows variable argments to be read from 
  pushing the argument to stack in reverse order 
  with no alignment considerations
*/

#ifndef __STD_ARG
#define __STD_ARG

typedef unsigned char * va_list;

#define va_start(vaList,num) vaList=((unsigned char *)(&num))+((unsigned char *)(sizeof(int)))
#define va_arg(vaList,type) (*((type *)((vaList+((unsigned char *)(sizeof(type)))),vaList)))
#define va_end(vaList) ((void)0)

#endif
