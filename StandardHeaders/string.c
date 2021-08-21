
#include <string.h>


unsigned long strlen(const char* str){
	unsigned long l=0;
	while (str[l++]){}
	return l-1;
}

void* memset(void* dest, int v, unsigned long n){
	for (unsigned long i=0;i<n;i++){((char*)dest)[i]=v;}
	return dest;
}

void* memmove(void* dest, const void* src, unsigned long n){
	unsigned long i;
	if (dest!=src){
		if (dest<src){
			for (i=0;i!=n;i++){((char*)dest)[i]=((char*)src)[i];}
		} else {
			for (i=n-1;i!=-1;i--){((char*)dest)[i]=((char*)src)[i];}
		}
	}
	return dest;
}

void* memcpy(void* dest, const void* src, unsigned long n){
	for (unsigned long i=0;i<n;i++){((char*)dest)[i]=((char*)src)[i];}
	return dest;
}
	
char* strcpy(char* dest, const char* src){
	char* o_dest=dest;
	while (*(dest++)=*(src++)){}
	return o_dest;
}

char* strncpy(char* dest, const char* src, unsigned long count){
	char* o_dest=dest;
	if (count==0) goto r;
	while (--count,(*(dest++)=*(src++))){
		if (count==0) goto r;
	}
	while (count!=0){
		--count;
		*(dest++)=0;
	}
	r:;return o_dest;
}

char* strcat(char* dest, const char* src){
	const unsigned long s=strlen(dest);
	strcpy(dest+s,src);
	return dest;
}

char* strncat(char* dest, const char* src, unsigned long count){
	const unsigned long s=strlen(dest);
	strncpy(dest+s,src,count);
	return dest;
}


char* strchr(const char* str,int character){
	int c;
	character=(char)character;
	while (1){
		c=*str;
		if (c==character) return (char*)str;
		if (c==0) return (void*)0;
		str++;
	}
}

char* strrchr(const char* str,int character){
	const char* r=(void*)0;
	int c;
	character=(char)character;
	while (1){
		c=*str;
		if (c==character) r=str;
		if (c==0) return (char*)r;
		str++;
	}
}

int strcmp(const char* str1,const char* str2){
	while (1){
		unsigned int c1,c2;
		c1=*(str1++);
		c2=*(str2++);
		if (c1==c2 & c1!=0) continue;
		return c1-c2;
	}
}

int strncmp(const char* str1,const char* str2, unsigned long count){
	while (count--!=0){
		unsigned int c1,c2;
		c1=*(str1++);
		c2=*(str2++);
		if (c1==c2 & c1!=0) continue;
		return c1-c2;
	}
	return 0;
}

int memcmp(const char* mem1,const char* mem2, unsigned long count){
	for (unsigned long i=0;i<count;i++){
		if (mem1[i]!=mem2[i]) return mem1[i]-mem2[i];
	}
	return 0;
}

void* memchr(const void* ptr, int value, unsigned long num){
	for (unsigned long i=0;i<num;i++){
		if ((unsigned char)value==((unsigned char*)ptr)[i]) return (unsigned char*)ptr+i;
	}
	return (void*)0;
}

unsigned long strspn(const char* dest,const char* src){
	unsigned long lr=0;
	while (1){
		if (dest[lr]==0 || strchr(src,dest[lr])==NULL) return lr;
		lr++;
	}
}

unsigned long strcspn(const char* dest,const char* src){
	unsigned long lr=0;
	while (1){
		if (dest[lr]==0 || strchr(src,dest[lr])!=NULL) return lr;
		lr++;
	}
}

char* strpbrk(const char* dest,const char* breakset){
	return strcspn(dest,breakset)+(char*)dest;
}

char* strstr(const char* str,const char* substr){
	unsigned long i;
	unsigned int c0,c1;
	if ((c1=*substr)==0) return (char*)str;
	while ((c0=*str)!=0){
		if (c0==c1){
			i=1;
			while (1){
				if (substr[i]==0) return (char*)str;
				if (str[i]!=substr[i]) break;
				i++;
			}
		}
		str++;
	}
	return (void*)0;
}

char* strtok(char* str, const char* delim){
	if (str==(void*)0){
		str=_static_var_strtok;
	} else {
		_static_var_strtok=str;
	}
	if (str==(void*)0){
		return (void*)0;
	}
	char* start=strspn(str,delim)+str;
	if (*start==0){
		_static_var_strtok=start;
		return (void*)0;
	}
	char* end=strcspn(start,delim)+start;
	if (*end==0){
		_static_var_strtok=end;
		return start;
	}
	*end=0;// this assignment would violate `restrict` semantics on `str`
	_static_var_strtok=end+1;
	return start;
}
