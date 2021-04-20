






#include <string.h>


static unsigned long strlen(const char* str){
	unsigned long l=0;
	while (str[l++]){
	}
	return l-1;
}
#if 0
// untested
static void memset(void* dest, int v, unsigned long n){
	if (n==0) return; // do while loops are used
	unsigned long i=0;
	unsigned long udest=(unsigned long)dest;
	v&=255;
	const unsigned int uv=((unsigned int)v<<8u) | (unsigned int)v;
	if (udest&1) (*((char*)(udest++))=(char)v),(n--);
	const bool doTail=n&1;
	n-=doTail;
	const unsigned long end=udest+n;
	if (n!=0){
		udest-=2;
		do {
			udest+=2;
			*((unsigned int*)udest)=uv;
		} while (udest!=end);
	}
	if (doTail) (*((char*)(udest))=(char)v);
}

#else

static void memset(void* dest, int v, unsigned long n){
	for (unsigned long i=0;i<n;i++){
		((char*)dest)[i]=v;
	}
}

#endif


#if 0
// known to NOT work correctly
static void memcpy(void* dest, const void* src, unsigned long n){
	if (n==0) return; // do while loops are used
	unsigned long i=0;
	if (((unsigned long)dest&1)^((unsigned long)src&1)){
		// no hope of word alligning
		do {
			((char*)dest)[i]=((const char*)src)[i];
		} while (i++<n);
	} else if (!(((unsigned long)dest&1) | ((unsigned long)src&1) | (n&1))){
		// everything is word alligned
		
		//n/=2; //there is currently a problem with this operator
		n=n/2;
		do {
			((unsigned int*)dest)[i]=((const unsigned int*)src)[i];
		} while (i++<n);
	} else {
		// other situations currently default to byte copy. optimizations are possible.
		do {
			((char*)dest)[i]=((const char*)src)[i];
		} while (i++<n);
	}
}

#else

static void memcpy(void* dest, const void* src, unsigned long n){
	for (unsigned long i=0;i<n;i++){
		((char*)dest)[i]=((const char*)src)[i];
	}
}
	
#endif


// untested, but quite sure it will work
static void strcpy(char* dest, const char* src){
	while (*(dest++)=*(src++)){
	}
}


static char* strchr(const char* str,int character){
	char c;
	while (1){
		c=*str;
		if (c==character) return str;
		if (c==0) return NULL;
		str++;
	}
}





