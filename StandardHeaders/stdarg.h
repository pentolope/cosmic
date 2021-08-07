

#ifndef __STD_ARG
#define __STD_ARG

typedef unsigned long va_list;
/* these definitions do no runtime checks */
#define va_allign_(n) (((n)+((n)&1)))
#define va_start(vaList,lastArg) ((vaList=(unsigned long)&lastArg+va_allign_(sizeof lastArg)))
#define va_arg(vaList,type) (((vaList+=va_allign_(sizeof(type))),(*((type*)(vaList-va_allign_(sizeof(type)))))))
#define va_end(vaList) (((void)0))


#endif

