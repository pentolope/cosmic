
#ifndef __STD_PRINTF
#define __STD_PRINTF

#include <stdio.h>
#include <stdarg.h>

#ifdef __STD_REAL

int vsprintf(char *dest, const char *fmt, va_list args);
int fprintf(FILE* stream, const char* format, ...);
int printf(const char* format, ...);
int snprintf(char* str, unsigned long size, const char* format, ...);

#else // #ifdef __STD_REAL

extern int vsprintf(char *dest, const char *fmt, va_list args);
extern int fprintf(FILE* stream, const char* format, ...);
extern int printf(const char* format, ...);
extern int snprintf(char* str, unsigned long size, const char* format, ...);

#endif // #ifdef __STD_REAL

#endif
