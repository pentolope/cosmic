


// probably temporary implementation

#include <printf_.h>

static void _print_target_char(struct _print_target* print_target,char c){
	if (print_target->isStr){
		if (!print_target->strMaxHit){
			print_target->str[print_target->writeCount++]=c;
			print_target->strMaxHit=(uint32_t)print_target->writeCount>print_target->strMax;
		} else {
			print_target->writeCount++;
		}
	} else {
		//*((volatile char*)0x04030000)=c;
		fputc(c,print_target->stream);
	}
}



static struct {
	uint8_t ansiBuffer[8];
	bool isAnsiEscapeOccuring;
	uint8_t current_foreground;
	uint8_t current_background;
	uint16_t cursor;
} _terminalCharacterState={
	.current_foreground=224,
	.current_background=0
};

static void _putchar_ensure_cursor_normal(){
	uint8_t mode_info=*(volatile uint8_t*)(0x80807ffflu);
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


static void _putchar_screen(char c){
	if (c>=' ' & c<='~'){
		const uint32_t a=0x80800000lu+_terminalCharacterState.cursor*3lu;
		*(volatile uint8_t*)(a+0)=c;
		*(volatile uint8_t*)(a+1)=_terminalCharacterState.current_foreground;
		*(volatile uint8_t*)(a+2)=_terminalCharacterState.current_background;
		_terminalCharacterState.cursor++;
		_putchar_ensure_cursor_normal();
	} else if (c=='\r'){
		unsigned n=(((*(volatile uint8_t*)(0x80807ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
	} else if (c=='\n'){
		unsigned n=(((*(volatile uint8_t*)(0x80807ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
		_terminalCharacterState.cursor+=n;
		_putchar_ensure_cursor_normal();
	}
}

static void _putstr(struct _print_target* print_target,const char* str){
	if (str==NULL) str="(null)";
	while (*str!=0){
		_print_target_char(print_target,*(str++));
	}
}

static void _put_udeci(struct _print_target* print_target,unsigned long uv){
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

static void _put_sdeci(struct _print_target* print_target,long sv){
	unsigned long uv=sv;
	bool s = (sv&0x80000000)!=0;
	unsigned long uvc=(uv^(s*0xFFFFFFFF))+s;
	if (s) _print_target_char(print_target,'-');
	_put_udeci(print_target,uvc);
}

static void _putinthex(struct _print_target* print_target,unsigned int v){
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


static void _print(struct _print_target* print_target,const char* format,va_list args){
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


static int vsprintf(char *dest, const char *format, va_list args){
	struct _print_target print_target={0};
	print_target.isStr=1;
	print_target.str=dest;
	print_target.strMax=0xFFFFFFFF;
	
	_print(&print_target,format,args);
	
	_print_target_char(&print_target,0);
	return print_target.writeCount;
}

static int fprintf(FILE* stream, const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stream;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

static int printf(const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stdout;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

static int snprintf(char* dest, unsigned long size, const char* format, ...){
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







