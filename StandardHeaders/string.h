
#ifndef __STD_STRING
#define __STD_STRING

#ifdef __STD_REAL

unsigned long strlen(const char* str);
void* memset(void* dest, int v, unsigned long n);
void* memmove(void* dest, const void* src, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);	
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, unsigned long count);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, unsigned long count);
char* strchr(const char* str,int character);
char* strrchr(const char* str,int character);
int strcmp(const char* str1,const char* str2);
int strncmp(const char* str1,const char* str2, unsigned long count);
int memcmp(const char* mem1,const char* mem2, unsigned long count);
void* memchr(const void* ptr, int value, unsigned long num);
unsigned long strspn(const char* dest,const char* src);
unsigned long strcspn(const char* dest,const char* src);
char* strpbrk(const char* dest,const char* breakset);
char* strstr(const char* str,const char* substr);
char* strtok(char* str, const char* delim);

char* _static_var_strtok=(void*)0; // this declaration being here like this creates some weird side effects.
// launching other programs will require saving the value of `_static_var_strtok` beforehand and restoring it afterward.

#else // #ifdef __STD_REAL

extern unsigned long strlen(const char* str);
extern void* memset(void* dest, int v, unsigned long n);
extern void* memmove(void* dest, const void* src, unsigned long n);
extern void* memcpy(void* dest, const void* src, unsigned long n);	
extern char* strcpy(char* dest, const char* src);
extern char* strncpy(char* dest, const char* src, unsigned long count);
extern char* strcat(char* dest, const char* src);
extern char* strncat(char* dest, const char* src, unsigned long count);
extern char* strchr(const char* str,int character);
extern char* strrchr(const char* str,int character);
extern int strcmp(const char* str1,const char* str2);
extern int strncmp(const char* str1,const char* str2, unsigned long count);
extern int memcmp(const char* mem1,const char* mem2, unsigned long count);
extern void* memchr(const void* ptr, int value, unsigned long num);
extern unsigned long strspn(const char* dest,const char* src);
extern unsigned long strcspn(const char* dest,const char* src);
extern char* strpbrk(const char* dest,const char* breakset);
extern char* strstr(const char* str,const char* substr);
extern char* strtok(char* str, const char* delim);

#endif // #ifdef __STD_REAL

#endif
