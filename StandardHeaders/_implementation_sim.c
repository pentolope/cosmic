

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define COLOR_GRAY_OR_BLACK '0' // gray when on text, black when on background
#define COLOR_RED           '1'
#define COLOR_GREEN         '2'
#define COLOR_YELLOW        '3'
#define COLOR_BLUE          '4'
#define COLOR_MAGENTA       '5'
#define COLOR_CYAN          '6'
#define COLOR_WHITE         '7'

#define COLOR_TO_TEXT        '9'
#define COLOR_TO_BACKGROUND  '4'


struct _print_target{
	_Bool isStr;
	_Bool strMaxHit;
	char* str;
	FILE* stream;
	unsigned long strMax;
	unsigned long writeCount;
};


void _print_target_char(struct _print_target*,char);

const char* _file_modes[16]={NULL,"r","rb","w","wb","a","ab","r+","rb+","r+b","w+","wb+","w+b","a+","ab+","a+b"};

_Bool _isStrEql(const char* s0,const char* s1){
	unsigned long i=0;
	char c0,c1;
	while ((c0=s0[i])!=0 & (c1=s1[i])!=0){
		i++;
		if (c0!=c1) return false;
	}
	return c0==c1;
}

void _putchar_screen(char);

int fflush(FILE* stream){
	// simulator doesn't use buffers
	return 0;
}

int fputc(int c, FILE* stream){
	if (stream->buffType<3) return EOF;
	if (stream<=__open_files+2){
		_putchar_screen(c);
	} else {
		if (stream->buffType==3 | stream->buffType==4){
			*((volatile char*)(0x05000000))=c;
		} else {
			*((volatile char*)(0x05000000+stream->buffPos++))=c;
			if (*((volatile unsigned long*)0x04020006)<=stream->buffPos){
				*((volatile unsigned long*)0x04020006)=stream->buffPos+1;
			}
		}
		return c;
	}
}

int fgetc(FILE* stream){
	if (stream->buffType==0 | (stream->buffType>=3 & stream->buffType<=6)) return EOF;
	if (*((volatile unsigned long*)0x04020006)<=stream->buffPos){
		stream->errFlags=1;
		return EOF;
	}
	return *((volatile char*)(0x05000000+stream->buffPos++));
}

int fgetpos(FILE* stream,fpos_t* pos){
	pos->position=stream->buffPos;
	return 0;
}

int fsetpos(FILE* stream,fpos_t* pos){
	stream->buffPos=pos->position;
	stream->errFlags=0;
	return 0;
}

FILE* fopen(const char* file,const char* mode){
	FILE* fileObjPtr;
	{
		unsigned int i=2; // open slots start at 3 because 0,1,2 are taken by default
		while (++i<4){
			if (__open_files[i].buffType==0){
				fileObjPtr=__open_files+i;
				goto L0;
			}
		}
		return NULL; // no more open file slots
	}
	L0:;
	fileObjPtr->buffPos=0;
	{
		
		unsigned char mode_id=0;
		while (++mode_id<16){
			if (_isStrEql(_file_modes[mode_id],mode)){
				fileObjPtr->buffType=mode_id;
				goto L1;
			}
		}
		return NULL; // invalid mode string
	}
	L1:;
	{
		const char* mode_a=_file_modes[fileObjPtr->buffType];
		unsigned long i=0;
		while ((*((volatile char*)0x04020001)=mode_a[i++])!=0){
		}
		i=0;
		while ((*((volatile char*)0x04020002)=file[i++])!=0){
		}
		if (*((volatile char*)0x04020003)==0){
			return NULL;
		}
	}
	return fileObjPtr;
}

int fclose(FILE* stream){
	if (stream==NULL) return EOF;
	if (stream==__open_files+0 | stream==__open_files+1 | stream==__open_files+2) return EOF;
	*((volatile char*)0x04020003)=0;
	stream->buffType=0;
	return 0;
}



void _print_target_char(struct _print_target* print_target,char c){
	if (print_target->isStr){
		if (!print_target->strMaxHit){
			print_target->str[print_target->writeCount++]=c;
			print_target->strMaxHit=print_target->writeCount>print_target->strMax;
		} else {
			print_target->writeCount++;
		}
	} else {
		fputc(c,print_target->stream);
	}
}



struct {
	uint8_t ansiBuffer[8];
	bool isAnsiEscapeOccuring;
	uint8_t current_foreground;
	uint8_t current_background;
	uint16_t cursor;
} _terminalCharacterState={
	.current_foreground=182,
	.current_background=0
};

void _putchar_ensure_cursor_normal(){
	uint8_t mode_info=*(volatile uint8_t*)(0x80804ffflu);
	uint8_t font_height=(mode_info &15)+3;
	unsigned n0=480u/(unsigned)font_height;
	unsigned n1=n0-1;
	unsigned n2;
	if ((mode_info &(1<<4))!=0){
		n2=80;
	} else {
		n2=71;
	}
	n0*=n2;
	n1*=n2;
	while (_terminalCharacterState.cursor>=n0){
		_terminalCharacterState.cursor-=n2;
		uint16_t i;
		for (i=0;i<n1;i++){
			const uint32_t a0=0x80800000lu+(i+ 0)*3lu;
			const uint32_t a1=0x80800000lu+(i+n2)*3lu;
			*(volatile uint8_t*)(a0+0)=*(volatile uint8_t*)(a1+0);
			*(volatile uint8_t*)(a0+1)=*(volatile uint8_t*)(a1+1);
			*(volatile uint8_t*)(a0+2)=*(volatile uint8_t*)(a1+2);
		}
		for (;i<n0;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
	}
}


void _putchar_screen(char c){
	if (c>=' ' & c<='~'){
		const uint32_t a=0x80800000lu+_terminalCharacterState.cursor*3lu;
		*(volatile uint8_t*)(a+0)=c;
		*(volatile uint8_t*)(a+1)=_terminalCharacterState.current_foreground;
		*(volatile uint8_t*)(a+2)=_terminalCharacterState.current_background;
		_terminalCharacterState.cursor++;
		_putchar_ensure_cursor_normal();
	} else if (c=='\r'){
		unsigned n=(((*(volatile uint8_t*)(0x80804ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
	} else if (c=='\n'){
		unsigned n=(((*(volatile uint8_t*)(0x80804ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
		_terminalCharacterState.cursor+=n;
		_putchar_ensure_cursor_normal();
	}
}

void _putstr(struct _print_target* print_target,const char* str){
	if (str==NULL) str="(null)";
	while (*str!=0){
		_print_target_char(print_target,*(str++));
	}
}

void _put_udeci(struct _print_target* print_target,unsigned long uv){
	bool t=false;
	const uint8_t iStart=(uv&0xFFFF0000)!=0?9:4;
	uint8_t i=iStart;
	uint8_t o[10];
	if (iStart==9){
		do {
			o[i]=uv%10;
			uv=uv/10;
		} while (i--!=0);
	} else {
		uint16_t uvs=uv;
		do {
			o[i]=uvs%10;
			uvs=uvs/10;
		} while (i--!=0);
	}
	i=iStart;
	do {
		char c=o[iStart-i];
		t|=c!=0;
		if (t) _print_target_char(print_target,c+'0');
	} while (i--!=0);
	if (!t) _print_target_char(print_target,'0');
}

void _put_sdeci(struct _print_target* print_target,long sv){
	unsigned long uv=sv;
	bool s = (sv&0x80000000)!=0;
	unsigned long uvc=(uv^(s*0xFFFFFFFF))+s;
	if (s) _print_target_char(print_target,'-');
	_put_udeci(print_target,uvc);
}

void _putinthex(struct _print_target* print_target,unsigned int v){
	for (unsigned int i=0;i<2u;i++){
		unsigned int byte0=((char*)&v)[1u-i];
		unsigned int digit0=(byte0>>4)&0xFu;
		unsigned int digit1=(byte0>>0)&0xFu;
		digit0=digit0+(digit0>=10u)*7u+48u;
		digit1=digit1+(digit1>=10u)*7u+48u;
		_print_target_char(print_target,digit0);
		_print_target_char(print_target,digit1);
	}
}


void _print(struct _print_target* print_target,const char* format,va_list args){
	while (1){
		char c;
		bool formatFinished;
		bool formatTerminateByAnother;
		bool formatTerminateByNull;
		bool formatTerminateByExcess;
		uint16_t formatState0=0;
		uint16_t formatState1;
		uint32_t val;
		switch ((c=*(format++))){
			case 0:
			return;
			case '%':
			FormatStart:;
			formatFinished=false;
			formatState0=1;
			formatState1=0;
			formatTerminateByAnother=0;
			formatTerminateByNull=0;
			formatTerminateByExcess=0;
			do {
				switch ((c=*(format++))){
					case 0:
					formatFinished=1;
					formatTerminateByNull=1;
					break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					break;
					case 'c':
					formatState0=10;
					formatFinished=1;
					break;
					case 'l':
					formatState0+=2;
					break;
					case 'd':
					formatFinished=1;
					break;
					case 'u':
					formatState0+=1;
					formatFinished=1;
					break;
					case 'x':
					case 'X':
					formatState0=7;
					break;
					case '%':
					formatFinished=1;
					if (formatState0==1){
						formatState0=8;
					} else {
						formatTerminateByAnother=1;
					}
					break;
					case 's':
					formatFinished=1;
					if (formatState0==1){
						formatState0=9;
					}
					break;
					default:
					formatFinished=1;
					formatTerminateByExcess=1;
					break;
				}
			} while (!formatFinished);
			switch (formatState0){
				case 1:
				case 2:
				case 7:
				case 10:
				val=va_arg(args,unsigned int);
				break;
				case 3:
				case 4:
				case 9:
				val=va_arg(args,unsigned long);
				break;
			}
			switch (formatState0){
				case 1:
				val=(long)((int)val); // sign extend
				case 3:
				_put_sdeci(print_target,val);
				break;
				case 2:
				case 4:
				_put_udeci(print_target,val);
				break;
				case 7:
				_putinthex(print_target,val);
				break;
				case 8:
				_print_target_char(print_target,'%');
				break;
				case 9:
				_putstr(print_target,(char*)val);
				break;
				case 10:
				_print_target_char(print_target,val&255);
				break;
			}
			if (formatTerminateByNull) {
				return;
			}
			if (formatTerminateByAnother) goto FormatStart;
			if (formatTerminateByExcess) _print_target_char(print_target,c);
			
			break;
			default:
			_print_target_char(print_target,c);
			break;
		}
	}
}


int vsprintf(char *dest, const char *format, va_list args){
	struct _print_target print_target={0};
	print_target.isStr=1;
	print_target.str=dest;
	print_target.strMax=0xFFFFFFFF;
	
	_print(&print_target,format,args);
	
	_print_target_char(&print_target,0);
	return print_target.writeCount;
}

int fprintf(FILE* stream, const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stream;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

int printf(const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stdout;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

int snprintf(char* dest, unsigned long size, const char* format, ...){
	struct _print_target print_target={0};
	print_target.isStr=1;
	print_target.str=dest;
	print_target.strMax=size;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	_print_target_char(&print_target,0);
	return print_target.writeCount;
}

unsigned long fread(void* buffer,unsigned long size,unsigned long count,FILE* file){
	unsigned long e=size*count;
	for (unsigned long i=0;i<e;i++){
		int v;
		v=fgetc(file);
		if (v==EOF) return size<=1u?i:i/size;
		((char*)buffer)[i]=v;
	}
	return count;
}

unsigned long fwrite(const void* buffer,unsigned long size,unsigned long count,FILE* file){
	unsigned long e=size*count;
	for (unsigned long i=0;i<e;i++){
		int v;
		v=fputc(((char*)buffer)[i],file);
		if (v==EOF) return size<=1u?i:i/size;
	}
	return count;
}

int feof(FILE* file){
	if (file->errFlags & 1) return -1;
	return 0;
}

void _give_not_implemented_message(const char* f_name){
	fprintf(stderr," The function `%s` is not implemented in the simulator definitions. The simulator will now exit.",f_name);
	exit(1);
}

int fseek(FILE* file,long offset,int whence){
	_give_not_implemented_message("fseek");
}
long ftell(FILE* file){
	_give_not_implemented_message("ftell");
}
int ungetc(int ch, FILE* file){
	_give_not_implemented_message("ungetc");
}
int rewind(FILE* file){
	_give_not_implemented_message("rewind");
}
void clearerr(FILE* file){
	_give_not_implemented_message("clearerr");
}
int ferror(FILE* file){
	_give_not_implemented_message("ferror");
}

