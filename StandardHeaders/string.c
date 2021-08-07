






#include <string.h>


static unsigned long strlen(const char* str){
	unsigned long l=0;
	while (str[l++]){
	}
	return l-1;
}

static void memset(void* dest, int v, unsigned long n){
	for (unsigned long i=0;i<n;i++){
		((char*)dest)[i]=v;
	}
}

static void memcpy(void* dest, const void* src, unsigned long n){
	for (unsigned long i=0;i<n;i++){
		((char*)dest)[i]=((const char*)src)[i];
	}
}
	
static void strcpy(char* dest, const char* src){
	while (*(dest++)=*(src++)){
	}
}

static char* strchr(const char* str,int character){
	int c;
	character=(char)character;
	while (1){
		c=*str;
		if (c==character) return (char*)str;
		if (c==0) return (void*)0;
		str++;
	}
}

static int strcmp(const char* str1,const char* str2){
	while (1){
		char c1;char c2;
		c1=*(str1++);
		c2=*(str2++);
		if (c1==c2 & c1!=0) continue;
		return c1-c2;
	}
}

static void* memchr(const void* ptr, int value, unsigned long num){
	unsigned char* ptr0=(unsigned char*)ptr;
	while (num--!=0){
		unsigned char* ptr1=ptr0;
		if ((unsigned char)value==*(ptr0++)) return ptr1;
	}
	return (void*)0;
}

