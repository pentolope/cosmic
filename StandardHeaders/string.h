
#ifndef __STD_STRING
#define __STD_STRING


#ifdef __STD_REAL

static unsigned long strlen(const char*);
static void memset(void*,int v,unsigned long);
static void memcpy(void*,const void*,unsigned long);
static void strcpy(char*,const char*);
static char* strchr(const char*,int);

#else // #ifdef __STD_REAL

extern unsigned long strlen(const char*);
extern void memset(void*,int v,unsigned long);
extern void memcpy(void*,const void*,unsigned long);
extern void strcpy(char*,const char*);
extern char* strchr(const char*,int);

#endif // #ifdef __STD_REAL


#endif
