
#ifndef __STD_PRINTF
#define __STD_PRINTF

/*
#define PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_DISABLE_SUPPORT_LONG_LONG

#include "mpaland/printf.c"
*/

#include <stdio.h>
#include <stdarg.h>


int vsprintf(char *dest, const char *fmt, va_list args){
	return 0;
}

int fprintf(FILE* stream, const char* format, ...){
	return 0;
}

int printf(const char* format, ...){
	long i=0;
	while (format[i++]){
	}
	return fprintf(&stdout,format);
}

int snprintf(char* str, unsigned long size, const char* format, ...){
	return 0;
}


#endif
