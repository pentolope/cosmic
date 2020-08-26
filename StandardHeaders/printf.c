


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
	uint8_t font[665];
	uint8_t ansiBuffer[8];
	bool isAnsiEscapeOccuring;
	uint8_t current_foreground;
	uint8_t current_background;
	uint16_t cursor;
	struct _CharacterState{
		uint8_t character;
		uint8_t foreground;
		uint8_t background;
	} state[880];
} _terminalCharacterState={
	.current_foreground=224,
	.current_background=0,
	.font={0,0,0,0,0,0,0,0,0,0,250,0,0,0,0,0,192,0,192,0,0,0,40,254,40,254,40,0,0,100,146,254,146,76,0,66,164,72,16,36,74,132,108,146,146,170,68,10,0,0,0,0,192,0,0,0,0,0,56,68,130,0,0,0,0,130,68,56,0,0,0,0,80,32,80,0,0,0,0,8,28,8,0,0,0,0,0,1,2,0,0,0,0,8,8,8,0,0,0,0,0,2,0,0,0,2,4,8,16,32,64,128,0,124,138,146,162,124,0,0,34,66,254,2,2,0,0,66,134,138,146,98,0,0,68,146,146,146,108,0,0,24,40,72,254,8,0,0,242,146,146,146,140,0,0,124,146,146,146,12,0,0,128,128,134,152,224,0,0,108,146,146,146,108,0,0,96,146,146,146,108,0,0,0,0,36,0,0,0,0,0,2,36,0,0,0,0,16,40,68,130,0,0,0,40,40,40,40,40,0,0,0,130,68,40,16,0,0,0,128,138,144,96,0,124,130,154,186,138,122,0,0,126,144,144,144,126,0,0,254,146,146,146,108,0,0,124,130,130,130,130,0,0,254,130,130,68,56,0,0,254,146,146,130,130,0,0,254,144,144,128,128,0,0,124,130,138,138,12,0,0,254,16,16,16,254,0,0,130,130,254,130,130,0,4,130,130,130,252,128,0,0,254,16,40,68,130,0,0,254,2,2,2,2,0,254,64,32,16,32,64,254,254,64,32,16,8,4,254,0,124,130,130,130,124,0,0,254,144,144,144,96,0,0,124,130,138,132,122,0,0,254,144,152,148,98,0,0,100,146,146,146,76,0,0,128,128,254,128,128,0,0,252,2,2,2,252,0,0,248,4,2,4,248,0,252,2,2,12,2,2,252,130,68,40,16,40,68,130,0,224,16,14,16,224,0,0,134,138,146,162,194,0,0,0,254,130,0,0,0,128,64,32,16,8,4,2,0,0,0,130,254,0,0,0,16,32,64,32,16,0,1,1,1,1,1,1,1,0,0,128,128,64,0,0,0,6,42,42,42,28,2,0,254,18,18,18,12,0,0,28,34,34,34,34,0,0,12,18,18,18,254,0,0,28,42,42,42,26,0,0,16,126,144,144,0,0,0,24,37,37,37,30,0,0,254,16,16,16,30,0,0,0,0,94,0,0,0,0,2,1,1,94,0,0,0,254,8,20,34,0,0,0,0,0,254,0,0,0,62,32,32,30,32,32,30,0,62,32,32,32,30,0,0,28,34,34,34,28,0,0,126,72,72,48,0,0,0,0,24,36,36,63,0,0,62,16,32,32,32,0,0,18,42,42,42,36,0,0,32,32,124,34,32,0,0,60,2,2,2,60,0,0,56,4,2,4,56,0,60,2,2,12,2,2,60,0,34,20,8,20,34,0,0,56,5,5,5,62,0,0,70,74,82,98,0,0,0,16,108,130,130,0,0,0,0,0,254,0,0,0,0,0,130,130,108,16,0,0,8,16,16,8,8,16}
};




static void _putchar_write(uint16_t position){
	struct _CharacterState characterState=_terminalCharacterState.state[position];
	if (characterState.character<' ' | characterState.character>'~'){
		characterState.character=' ';
	}
	uint16_t heightPosition=position/40u;
	uint16_t widthPosition=position%40u;
	uint32_t mem=0x04000000+320;
	mem+=widthPosition*8+(uint32_t)heightPosition*320*9;
	uint8_t* fontPosition=(uint8_t*)_terminalCharacterState.font+(characterState.character-' ')*7;
	*(volatile uint8_t*)(mem+7+7*320lu)=characterState.background;
	*(volatile uint8_t*)(mem+7+8*320lu)=characterState.background;
	for (uint16_t i=0;i<7u;i++){
		uint32_t memi=mem+i;
		uint8_t fv=*(fontPosition+i);
		bool b;
		*(volatile uint8_t*)(memi+320*0)=characterState.background;
		b=(fv&(1<<7))!=0;
		*(volatile uint8_t*)(memi+320*1)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<6))!=0;
		*(volatile uint8_t*)(memi+320*2)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<5))!=0;
		*(volatile uint8_t*)(memi+320*3)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<4))!=0;
		*(volatile uint8_t*)(memi+320*4)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<3))!=0;
		*(volatile uint8_t*)(memi+320*5)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<2))!=0;
		*(volatile uint8_t*)(memi+320*6)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<1))!=0;
		*(volatile uint8_t*)(memi+320*7)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1<<0))!=0;
		*(volatile uint8_t*)(memi+320*8)=b*characterState.foreground | !b*characterState.background;
		*(volatile uint8_t*)(mem+7+i*320)=characterState.background;
	}
}

static void _putchar_ensure_cursor_normal(){
	while (_terminalCharacterState.cursor>=880){
		_terminalCharacterState.cursor-=40;
		uint16_t i;
		for (i=0;i<840;i++){
			_terminalCharacterState.state[i]=_terminalCharacterState.state[i+40];
			_putchar_write(i);
		}
		for (;i<880;i++){
			_terminalCharacterState.state[i].character=' ';
			_putchar_write(i);
		}
	}
}


static void _putchar_screen(char c){
	*((volatile char*)0x0400FFFF)=1; // set sim to do visual buffering
	
	if (c>=' ' & c<='~'){
		struct _CharacterState characterState;
		characterState.character=c;
		characterState.foreground=_terminalCharacterState.current_foreground;
		characterState.background=_terminalCharacterState.current_background;
		_terminalCharacterState.state[_terminalCharacterState.cursor]=characterState;
		_putchar_write(_terminalCharacterState.cursor++);
		_putchar_ensure_cursor_normal();
	} else if (c=='\r'){
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%40u;
	} else if (c=='\n'){
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%40u;
		_terminalCharacterState.cursor+=40;
		_putchar_ensure_cursor_normal();
	}
	
	*((volatile char*)0x0400FFFF)=0; // set sim to release visual buffering
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







