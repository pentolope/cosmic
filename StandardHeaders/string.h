
#ifndef __STD_STRING
#define __STD_STRING

unsigned long strlen(const char* str){
	unsigned long l=0;
	while (str[l++]){
	}
	return l-1;
}

// untested
void memset(void* dest, int v, size_t n){
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

// untested
void memcpy(void* dest, const void* src, unsigned long n){
	if (n==0) return; // do while loops are used
	unsigned long i=0;
	if (((unsigned long)dest&1)^((unsigned long)src&1)){
		// no hope of word alligning
		do {
			((char*)dest)[i]=((const char*)src)[i];
		} while (i++<n);
	} else if (!(((unsigned long)dest&1) | ((unsigned long)src&1) | (n&1))){
		// everything is word alligned
		n/=2;
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

// untested, but quite sure it will work
void strcpy(char* dest, const char* src){
	while (*(dest++)=*(src++)){
	}
}

#endif
