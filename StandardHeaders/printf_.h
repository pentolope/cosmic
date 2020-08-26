
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

#ifdef __STD_REAL

struct _print_target{
	_Bool isStr;
	_Bool strMaxHit;
	char* str;
	FILE* stream;
	uint32_t strMax;
	int writeCount;
};

static int vsprintf(char *dest, const char *fmt, va_list args);
static int fprintf(FILE* stream, const char* format, ...);
static int printf(const char* format, ...);
static int snprintf(char* str, unsigned long size, const char* format, ...);

#else // #ifdef __STD_REAL

extern int vsprintf(char *dest, const char *fmt, va_list args);
extern int fprintf(FILE* stream, const char* format, ...);
extern int printf(const char* format, ...);
extern int snprintf(char* str, unsigned long size, const char* format, ...);

#endif // #ifdef __STD_REAL

#endif
