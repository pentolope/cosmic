


#include <stdio.h>
#include <alloc.h>
#include <printf_.h>

static void _print_target_char(struct _print_target*,char);

static const char* _file_modes[16]={NULL,"r","rb","w","wb","a","ab","r+","rb+","r+b","w+","wb+","w+b","a+","ab+","a+b"};

static _Bool _isStrEql(const char* s0,const char* s1){
	unsigned long i=0;
	char c0,c1;
	while ((c0=s0[i])!=0 & (c1=s1[i])!=0){
		i++;
		if (c0!=c1) return false;
	}
	return c0==c1;
}

static char* _copyStr(const char* s0){
	unsigned long i=0;
	while (s0[i++]!=0){
	}
	char* s1=malloc(i);
	i=0;
	char c;
	do {
		s1[i]=(c=s0[i]);
		i++;
	} while (c!=0);
	return s1;
}

#ifdef __BUILDING_SIM_LOADER

static int __fopen__LL(const char* file,unsigned char mode_id){
	const char* mode=_file_modes[mode_id];
	unsigned long i=0;
	while ((*((volatile char*)0x04020001)=mode[i++])!=0){
	}
	i=0;
	while ((*((volatile char*)0x04020002)=file[i++])!=0){
	}
	if (*((volatile char*)0x04020003)==0){
		return EOF;
	}
	return 0;
}

static int __fclose__LL(FILE* stream){
	*((volatile char*)0x04020003)=0;
	return 0;
}

static int __fflush__LL(FILE* stream){
	// simulator doesn't use buffers
	return 0;
}

static void _putchar_screen(char);

static int __fputc__LL(int c, FILE* stream){
	if (stream<=__open_files+2){
		_putchar_screen(c);
	} else {
		if (stream->ignorePosition){
			*((volatile char*)(0x05000000))=c;
		} else {
			*((volatile char*)(0x05000000+stream->position++))=c;
			if (*((volatile unsigned long*)0x04020006)<=stream->position){
				*((volatile unsigned long*)0x04020006)=stream->position+1;
			}
		}
		return c;
	}
}

static int __fgetc__LL(FILE* stream){
	if (*((volatile unsigned long*)0x04020006)<=stream->position){
		return EOF;
	}
	return *((volatile char*)(0x05000000+stream->position++));
}


#else

// the other definitions for what will run on the machine (as opposed to the simulator) will be here


#endif



static int fflush(FILE* stream){
	return __fflush__LL(stream);
}

static int fputc(int c, FILE* stream){
	if (stream->mode_id<3) return EOF;
	return __fputc__LL(c,stream);
}

static int fgetc(FILE* stream){
	if (stream->mode_id==0 | (stream->mode_id>=3 & stream->mode_id<=6)) return EOF;
	return __fgetc__LL(stream);
}

static int fgetpos(FILE* stream,fpos_t* pos){
	pos->position=stream->position;
	return 0;
}

static int fsetpos(FILE* stream,fpos_t* pos){
	stream->position=pos->position;
	return 0;
}

static FILE* fopen(const char* file,const char* mode){
	FILE* fileObjPtr;
	{
		unsigned int i=2; // open slots start at 3 because 0,1,2 are taken by default
		while (++i<256){
			if (__open_files[i].mode_id==0){
				fileObjPtr=__open_files+i;
				goto L0;
			}
		}
		return NULL; // no more open file slots
	}
	L0:;
	fileObjPtr->path=NULL;
	fileObjPtr->position=0;
	{
		
		unsigned char mode_id=0;
		while (++mode_id<16){
			if (_isStrEql(_file_modes[mode_id],mode)){
				fileObjPtr->mode_id=mode_id;
				fileObjPtr->ignorePosition=mode_id==3 | mode_id==4;
				goto L1;
			}
		}
		return NULL; // invalid mode string
	}
	L1:;
	if (__fopen__LL(file,fileObjPtr->mode_id)){
		return NULL;
	}
	fileObjPtr->path=_copyStr(file);
	return fileObjPtr;
}

static int fclose(FILE* stream){
	if (stream==NULL) return EOF;
	if (stream==__open_files+0 | stream==__open_files+1 | stream==__open_files+2) return EOF;
	fflush(stream);
	if (__fclose__LL(stream)!=0) return EOF;
	free((char*)stream->path);
	stream->mode_id=0;
	return 0;
}

















