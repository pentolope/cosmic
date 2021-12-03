

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int16_t ex_stdin_block_appropriate();

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
	if (stream->file_descriptor==0){
		return ex_stdin_block_appropriate();
	}
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
			print_target->str[print_target->writeCount]=c;
			print_target->strMaxHit=(print_target->writeCount+1u)>print_target->strMax;
		}
	} else {
		fputc(c,print_target->stream);
	}
	print_target->writeCount++;
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
		memmove((void*)(0x80800000lu),(void*)(0x80800000lu+n2 *3lu),n1 *3lu);
		for (uint16_t i=n1;i<n0;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
	}
}

void _putchar_screen(char c){
	if (_terminalCharacterState.isAnsiEscapeOccuring){
		uint16_t l=strlen(_terminalCharacterState.ansiBuffer);
		if (c=='m'){
			_terminalCharacterState.isAnsiEscapeOccuring=0;
			if (_terminalCharacterState.ansiBuffer[0]=='['){
				uint8_t p=0;
				uint8_t v=0;
				switch (_terminalCharacterState.ansiBuffer[1]){
					case '0':if (l==2){
						_terminalCharacterState.current_foreground=182;
						_terminalCharacterState.current_background=0;
					}break;
					case COLOR_TO_BACKGROUND:p=1;break;
					case COLOR_TO_TEXT:p=2;break;
				}
				if (p!=0 & l==3){
					switch (_terminalCharacterState.ansiBuffer[2]){
						case COLOR_GRAY_OR_BLACK:v=p==1?0:182;break;
						case COLOR_RED:v=224;break;
						case COLOR_GREEN:v=28;break;
						case COLOR_YELLOW:v=252;break;
						case COLOR_BLUE:v=3;break;
						case COLOR_MAGENTA:v=227;break;
						case COLOR_CYAN:v=31;break;
						case COLOR_WHITE:v=255;break;
						default:p=0;break;
					}
					if (p==1){
						_terminalCharacterState.current_background=v;
					} else if (p==2){
						_terminalCharacterState.current_foreground=v;
					}
				}
			}
		} else if (l==7){
			// too long, abort it. It's not going to handle this well and I kinda don't care
			_terminalCharacterState.isAnsiEscapeOccuring=0;
		} else {
			_terminalCharacterState.ansiBuffer[l++]=c;
			if (_terminalCharacterState.ansiBuffer[0]=='[' & _terminalCharacterState.ansiBuffer[1]=='1' & _terminalCharacterState.ansiBuffer[2]=='A'){
				_terminalCharacterState.isAnsiEscapeOccuring=0;
				unsigned n=(((*(volatile uint8_t*)(0x80804ffflu)&(1<<4))!=0)?80u:71u);
				if (_terminalCharacterState.cursor>=n) _terminalCharacterState.cursor -=n;
			}
		}
	} else if (c==27){
		_terminalCharacterState.isAnsiEscapeOccuring=1;
		memset(_terminalCharacterState.ansiBuffer,0,8);
	} else if (c>=' ' & c<='~'){
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

void _setchar_screen(uint16_t position, uint8_t character, uint8_t foreground, uint8_t background){
	const uint32_t a=0x80800000lu+position*3lu;
	*(volatile uint8_t*)(a+0)=character;
	*(volatile uint8_t*)(a+1)=foreground;
	*(volatile uint8_t*)(a+2)=background;
}


void ps2_command(uint8_t id){
	// no ps2 commands used in simulator
}

int16_t ps2_read_byte(){
	/* mod_held :
	bit 0:left shift pressed
	bit 1:right shift pressed
	*/
	static uint8_t mod_held;
	/* in_flags :
	bit 0:break
	bit 1:special 0
	bit 2:special 1
	*/
	static uint8_t in_flags;
	uint8_t c0,c1;
	if (*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x02lu))!=0){
		c0=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x00lu));
		*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x00lu))=0;
	} else {
		return -1;
	}
	/* byte_to_action0 and byte_to_action1 :
	==0:unexpected byte
	==1:flag "break"
	==2:flag "special 0"
	==3:flag "special 1"
	==4:ack command responce
	==5:resend command responce
	==6:echo command responce
	==7:self test past responce
	>=8:probably finished
	==255:ignore
	*/
	static const uint8_t byte_to_action0[128]={ // shift disabled
		[0x00]=0,
		[0x01]=255,
		[0x02]=0,
		[0x03]=255,
		[0x04]=255,
		[0x05]=255,
		[0x06]=255,
		[0x07]=255,
		[0x08]=0,
		[0x09]=255,
		[0x0a]=255,
		[0x0b]=255,
		[0x0c]=255,
		[0x0d]=9,//tab
		[0x0e]=96,//`
		[0x0f]=0,
		[0x10]=0,
		[0x11]=255,
		[0x12]=14,//left shift
		[0x13]=0,
		[0x14]=255,
		[0x15]=113,//q
		[0x16]=49,//1
		[0x17]=0,
		[0x18]=0,
		[0x19]=0,
		[0x1a]=122,//z
		[0x1b]=115,//s
		[0x1c]=97,//a
		[0x1d]=119,//w
		[0x1e]=50,//2
		[0x1f]=255,
		[0x20]=0,
		[0x21]=99,//c
		[0x22]=120,//x
		[0x23]=100,//d
		[0x24]=101,//e
		[0x25]=52,//4
		[0x26]=51,//3
		[0x27]=255,
		[0x28]=0,
		[0x29]=32,//space
		[0x2a]=118,//v
		[0x2b]=102,//f
		[0x2c]=116,//t
		[0x2d]=114,//r
		[0x2e]=53,//5
		[0x2f]=255,
		[0x30]=0,
		[0x31]=110,//n
		[0x32]=98,//b
		[0x33]=104,//h
		[0x34]=103,//g
		[0x35]=121,//y
		[0x36]=54,//6
		[0x37]=0,
		[0x38]=0,
		[0x39]=0,
		[0x3a]=109,//m
		[0x3b]=106,//j
		[0x3c]=117,//u
		[0x3d]=55,//7
		[0x3e]=56,//8
		[0x3f]=0,
		[0x40]=0,
		[0x41]=44,//,
		[0x42]=107,//k
		[0x43]=105,//i
		[0x44]=111,//o
		[0x45]=48,//0
		[0x46]=57,//9
		[0x47]=0,
		[0x48]=0,
		[0x49]=46,//.
		[0x4a]=47,// /
		[0x4b]=108,//l
		[0x4c]=59,//;
		[0x4d]=112,//p
		[0x4e]=45,//-
		[0x4f]=0,
		[0x50]=0,
		[0x51]=0,
		[0x52]=39,//'
		[0x53]=0,
		[0x54]=91,//[
		[0x55]=61,//=
		[0x56]=0,
		[0x57]=0,
		[0x58]=255,
		[0x59]=15,//right shift
		[0x5a]=10,//enter
		[0x5b]=93,//]
		[0x5c]=0,
		[0x5d]=92,// \   (...)
		[0x5e]=0,
		[0x5f]=0,
		[0x60]=0,
		[0x61]=0,
		[0x62]=0,
		[0x63]=0,
		[0x64]=0,
		[0x65]=0,
		[0x66]=8,//backspace
		[0x67]=0,
		[0x68]=0,
		[0x69]=255,
		[0x6a]=0,
		[0x6b]=18,//left arrow
		[0x6c]=255,
		[0x6d]=0,
		[0x6e]=0,
		[0x6f]=0,
		[0x70]=255,
		[0x71]=127,//del
		[0x72]=19,//down arrow
		[0x73]=255,
		[0x74]=20,//right arrow
		[0x75]=17,//up arrow
		[0x76]=27,//esc
		[0x77]=255,
		[0x78]=255,
		[0x79]=43,//+ kp
		[0x7a]=255,
		[0x7b]=45,//- kp
		[0x7c]=42,//* kp
		[0x7d]=255,
		[0x7e]=255,
		[0x7f]=0
	};
	static const uint8_t byte_to_action1[128]={ // shift enabled
		[0x00]=0,
		[0x01]=255,
		[0x02]=0,
		[0x03]=255,
		[0x04]=255,
		[0x05]=255,
		[0x06]=255,
		[0x07]=255,
		[0x08]=0,
		[0x09]=255,
		[0x0a]=255,
		[0x0b]=255,
		[0x0c]=255,
		[0x0d]=9,//tab
		[0x0e]=126,//~
		[0x0f]=0,
		[0x10]=0,
		[0x11]=255,
		[0x12]=14,//left shift
		[0x13]=0,
		[0x14]=255,
		[0x15]=81,//Q
		[0x16]=33,//!
		[0x17]=0,
		[0x18]=0,
		[0x19]=0,
		[0x1a]=90,//Z
		[0x1b]=83,//S
		[0x1c]=65,//A
		[0x1d]=87,//W
		[0x1e]=64,//@
		[0x1f]=255,
		[0x20]=0,
		[0x21]=67,//C
		[0x22]=88,//X
		[0x23]=68,//D
		[0x24]=69,//E
		[0x25]=36,//$
		[0x26]=35,//#
		[0x27]=255,
		[0x28]=0,
		[0x29]=32,//space
		[0x2a]=86,//V
		[0x2b]=70,//F
		[0x2c]=84,//T
		[0x2d]=82,//R
		[0x2e]=37,//%
		[0x2f]=255,
		[0x30]=0,
		[0x31]=78,//N
		[0x32]=66,//B
		[0x33]=72,//H
		[0x34]=71,//G
		[0x35]=89,//Y
		[0x36]=94,//^
		[0x37]=0,
		[0x38]=0,
		[0x39]=0,
		[0x3a]=77,//M
		[0x3b]=74,//J
		[0x3c]=85,//U
		[0x3d]=38,//&
		[0x3e]=42,//*
		[0x3f]=0,
		[0x40]=0,
		[0x41]=60,//<
		[0x42]=75,//K
		[0x43]=73,//I
		[0x44]=79,//O
		[0x45]=41,//)
		[0x46]=40,//(
		[0x47]=0,
		[0x48]=0,
		[0x49]=62,//>
		[0x4a]=63,//?
		[0x4b]=76,//L
		[0x4c]=58,//:
		[0x4d]=80,//P
		[0x4e]=95,//_
		[0x4f]=0,
		[0x50]=0,
		[0x51]=0,
		[0x52]=34,//"
		[0x53]=0,
		[0x54]=123,//{
		[0x55]=43,//+
		[0x56]=0,
		[0x57]=0,
		[0x58]=255,
		[0x59]=15,//right shift
		[0x5a]=10,//enter
		[0x5b]=125,//}
		[0x5c]=0,
		[0x5d]=124,// |
		[0x5e]=0,
		[0x5f]=0,
		[0x60]=0,
		[0x61]=0,
		[0x62]=0,
		[0x63]=0,
		[0x64]=0,
		[0x65]=0,
		[0x66]=8,//backspace
		[0x67]=0,
		[0x68]=0,
		[0x69]=255,
		[0x6a]=0,
		[0x6b]=18,//left arrow
		[0x6c]=255,
		[0x6d]=0,
		[0x6e]=0,
		[0x6f]=0,
		[0x70]=255,
		[0x71]=127,//del
		[0x72]=19,//down arrow
		[0x73]=255,
		[0x74]=20,//right arrow
		[0x75]=17,//up arrow
		[0x76]=27,//esc
		[0x77]=255,
		[0x78]=255,
		[0x79]=43,//+ kp
		[0x7a]=255,
		[0x7b]=45,//- kp
		[0x7c]=42,//* kp
		[0x7d]=255,
		[0x7e]=255,
		[0x7f]=0
	};
	if ((c0 & 0x80)!=0){
		switch (c0){
			default:
			case 0x83:in_flags=0;break;
			case 0xaa:ps2_command(0);break;
			case 0xee:ps2_command(4);break;
			case 0xe1:in_flags |=4;break;
			case 0xe0:in_flags |=2;break;
			case 0xf0:in_flags |=1;break;
			case 0xfa:ps2_command(2);break;
			case 0xfc:
			case 0xfd:ps2_command(1);break;
			case 0xfe:ps2_command(3);break;
		}
		return -2;
	} else {
		c1=(((mod_held & 3)!=0)?byte_to_action1:byte_to_action0)[c0];
	}
	if (c1==255 | c1==0){
		in_flags=0;
	} else if (c1==14){
		mod_held=(in_flags & 1)?(mod_held & 0xfe):(mod_held | 0x01);
		in_flags=0;
	} else if (c1==15){
		mod_held=(in_flags & 1)?(mod_held & 0xfd):(mod_held | 0x02);
		in_flags=0;
	} else {
		if (in_flags & 1){
			in_flags=0;
		} else {
			return c1;
		}
	}
	return -2;
}

int16_t ex_stdin(){
	// returns EOF (-1) when no other characters are avalible (therefore it is non-blocking)
	while (1){
		int16_t v=ps2_read_byte();
		if (v==-2) continue;
		return v;
	}
}

void flasher_switch(){
	uint8_t v1,v2;
	const uint32_t a=0x80800000lu+_terminalCharacterState.cursor*3lu;
	v1=*(volatile uint8_t*)(a+1);
	v2=*(volatile uint8_t*)(a+2);
	*(volatile uint8_t*)(a+1)=v2;
	*(volatile uint8_t*)(a+2)=v1;
}

// waits until ex_stdin() returns non-EOF
// shows cursor position with visual flashing
// performs no lasting visual changes from the character it reads (the caller will have to print the character to the screen if they want it to be shown)
int16_t ex_stdin_block_character(){
	int16_t c;
	uint32_t flasher_inc=0;
	bool flasher_state=1;
	flasher_switch();
	while ((c=ex_stdin())==-1){
		if (flasher_inc++>=0x16000){
			flasher_inc=0;
			flasher_state=!flasher_state;
			flasher_switch();
		}
	}
	if (flasher_state){
		flasher_switch();
	}
	return c;
}

uint16_t getTerminalCursorLimit();
void _inc_cursor(int m);
void _setchar_screen(uint16_t position, uint8_t character, uint8_t foreground, uint8_t background);
char ex_stdin_line_buffer[256];
int16_t ex_stdin_line_position=-1;
bool stdin_do_line_block=1;

// reads an entire line and performs visual updating
int16_t ex_stdin_block_line(){
	uint16_t length=0;
	uint16_t position=0;
	ex_stdin_line_buffer[0]=0;
	uint16_t cursorLimit=getTerminalCursorLimit();
	int16_t c=-1;
	while (c!='\n'){
		c=ex_stdin_block_character();
		switch (c){
			case 8:// backspace
			if (position!=0){
				_inc_cursor(-1);
				memmove(ex_stdin_line_buffer+(position-1u),ex_stdin_line_buffer+position,(length-position)+1u);
				length--;
				position--;
			}
			break;
			case 17://up arrow
			// not implemented
			//_inc_cursor(-80);
			break;
			case 18://left arrow
			if (position!=0){
				_inc_cursor(-1);
				position--;
			}
			break;
			case 19:// down arrow
			// not implemented
			//_inc_cursor(80);
			break;
			case 20://right arrow
			if (position<length){
				_inc_cursor(1);
				position++;
			}
			break;
			case '\n':
			if (position!=length) _inc_cursor(length-position);
			position=length;
			break;
			default:
			if (length<253 & c>=' ' & c<='~'){
				_inc_cursor(1);
				memmove(ex_stdin_line_buffer+(position+1u),ex_stdin_line_buffer+position,(length-position)+1u);
				ex_stdin_line_buffer[position]=c;
				length++;
				position++;
			}
		}
		for (uint16_t i=0;i<=length;i++){
			int32_t target=(int32_t)_terminalCharacterState.cursor+((int32_t)i-(int32_t)position);
			if (target>=0 & target<cursorLimit){
				_setchar_screen((uint16_t)target,(i==length)?' ':ex_stdin_line_buffer[i],_terminalCharacterState.current_foreground,_terminalCharacterState.current_background);
			}
		}
		if (c=='\n'){
			_putchar_screen('\n');
			ex_stdin_line_buffer[length++]='\n';
			ex_stdin_line_buffer[length]=0;
			ex_stdin_line_position=1;
			return ex_stdin_line_buffer[0];
		}
	}
}

int16_t ex_stdin_block_appropriate(){
	if (ex_stdin_line_position!=-1){
		if (ex_stdin_line_buffer[ex_stdin_line_position]!=0){
			return ex_stdin_line_buffer[ex_stdin_line_position++];
		} else {
			ex_stdin_line_position=-1;
		}
	}
	if (stdin_do_line_block){return ex_stdin_block_line();} else {return ex_stdin_block_character();}
}

uint16_t getTerminalCursorLimit(){
	uint8_t mode_info=*(volatile uint8_t*)(0x80804ffflu);
	uint8_t font_height=(mode_info &15)+3;
	unsigned n0=480u/(unsigned)font_height;
	if ((mode_info &(1<<4))!=0){
		return n0*80u;
	} else {
		return n0*71u;
	}
}

void _inc_cursor(int m){
	if (m==0) return;
	if (m<0){
		if (_terminalCharacterState.cursor>=-m) _terminalCharacterState.cursor+=m;
	} else {
		_terminalCharacterState.cursor+=m;
		_putchar_ensure_cursor_normal();
	}
}


#define PRINTF_FLAG_NEG       (1U<< 0)
#define PRINTF_FLAG_PLUS      (1U<< 1)
#define PRINTF_FLAG_PADZERO   (1U<< 2)
#define PRINTF_FLAG_SPACE     (1U<< 3)
#define PRINTF_HAS_WIDTH      (1U<< 4)
#define PRINTF_HAS_PRECISION  (1U<< 5)
#define PRINTF_SIG_UPPERCASE  (1U<< 6)
#define PRINTF_VAL_CHAR       (1U<< 7)
#define PRINTF_VAL_SHORT      (1U<< 8)
#define PRINTF_VAL_LONG       (1U<< 9)
#define PRINTF_VAL_SIGNED     (1U<<10)

#define PRINTF_REINTERP_TO_SIGNED_INT(v) (*(int*)(&(v)))
#define PRINTF_REINTERP_TO_UNSIGN_INT(v) (*(unsigned int*)(&(v)))
#define PRINTF_REINTERP_TO_SIGNED_LONG(v) (*(long*)(&(v)))
#define PRINTF_REINTERP_TO_UNSIGN_LONG(v) (*(unsigned long*)(&(v)))


// prints padding and sign for integer-like specifiers
void _print_sub1(struct _print_target* print_target, unsigned int len_value, unsigned int sign_type, unsigned int mode, unsigned int format_signals, unsigned int width, unsigned int precision){
	/*
	mode 0: right justified before value printed
	mode 1: left justified before value printed
	mode 2: left justified after value printed
	*/
	/*
	sign_type 0: no sign
	sign_type 1: neg sign
	sign_type 2: pos sign
	sign_type 3: space
	*/
	unsigned precision_to_print=0;
	unsigned width_to_print=0;
	if (((format_signals & PRINTF_HAS_PRECISION)!=0) & (precision > (len_value - (sign_type!=0u)))){
		precision_to_print = precision - (len_value - (sign_type!=0u));
		len_value += precision_to_print;
	}
	if (((format_signals & PRINTF_HAS_WIDTH)!=0) & (width > len_value)){
		width_to_print = width - len_value;
		//len_value += width_to_print;//no need to update len_value
	}
	switch (mode){
		case 0:
		if (format_signals & PRINTF_FLAG_PADZERO){
			unsigned c=0u;
			switch (sign_type){
				case 1:c='-';break;
				case 2:c='+';break;
				case 3:c=' ';break;
			}
			if (c!=0u) _print_target_char(print_target,c);
		}
		case 2:
		if (format_signals & PRINTF_FLAG_PADZERO){
			for (unsigned i=0;i<width_to_print;i++){
				_print_target_char(print_target,'0');
			}
			return;
		} else {
			for (unsigned i=0;i<width_to_print;i++){
				_print_target_char(print_target,' ');
			}
		}
		if (mode==2) return;
		case 1:
		{
			unsigned c=0u;
			switch (sign_type){
				case 1:c='-';break;
				case 2:c='+';break;
				case 3:c=' ';break;
			}
			if (c!=0u) _print_target_char(print_target,c);
		}
		for (unsigned i=0;i<precision_to_print;i++){
			_print_target_char(print_target,'0');
		}
	}
}

#if 0
// prints padding for floating point specifiers (which is why this is disabled)
void _print_sub2(struct _print_target* print_target, unsigned int len_value, unsigned int format_signals, unsigned int width){
}
#endif

// prints a string
void _print_sub3(struct _print_target* print_target, const char* s, unsigned int format_signals, unsigned int width, unsigned int precision){
	if (s==NULL){
		// lets be nice to that poor soul who called printf with NULL for a string
		s="(null)";
	}
	unsigned long len0=strlen(s);
	unsigned int len1=len0;
	if (len0!=len1) len1=0xFFFFu;
	if ((format_signals & PRINTF_HAS_PRECISION)!=0 & precision<len1){
		len1=precision;
	}
	unsigned int len2=0;
	if ((format_signals & PRINTF_HAS_WIDTH)!=0 & width>len1){
		len2=width - len1;
	}
	if (format_signals & PRINTF_FLAG_NEG){
		for (unsigned int i=0;i<len1;i++) _print_target_char(print_target,s[i]);
		for (unsigned int i=0;i<len2;i++) _print_target_char(print_target,' ');
	} else {
		for (unsigned int i=0;i<len2;i++) _print_target_char(print_target,' ');
		for (unsigned int i=0;i<len1;i++) _print_target_char(print_target,s[i]);
	}
}

// prints a (un)signed integer in decimal format
void _print_sub4(struct _print_target* print_target, unsigned int value, unsigned int format_signals, unsigned int width, unsigned int precision){
	if (format_signals & PRINTF_HAS_PRECISION){
		format_signals &= ~PRINTF_FLAG_PADZERO; // it seems like having a precision specified will override zero pad
	}
	unsigned int uvalue;
	int is_neg;
	if (format_signals & PRINTF_VAL_SIGNED){
		is_neg=PRINTF_REINTERP_TO_SIGNED_INT(value)<0;
		uvalue=is_neg? - PRINTF_REINTERP_TO_SIGNED_INT(value) : PRINTF_REINTERP_TO_SIGNED_INT(value);
	} else {
		is_neg=0;
		uvalue=value;
	}
	unsigned int sign_type=is_neg;
	unsigned int buf[5];
	buf[4]=((uvalue        )%10u)+'0';
	buf[3]=((uvalue /   10u)%10u)+'0';
	buf[2]=((uvalue /  100u)%10u)+'0';
	buf[1]=((uvalue / 1000u)%10u)+'0';
	buf[0]=((uvalue /10000u)%10u)+'0';
	unsigned int skip=(((format_signals & PRINTF_HAS_PRECISION)!=0 & (precision==0u & value==0u))!=0);
	for (unsigned int i=0;i<4;i++){
		if (buf[i]=='0') skip++;
		else break;
	}
	unsigned int len=5u -skip;
	if (is_neg | (format_signals & PRINTF_FLAG_SPACE) | (format_signals & PRINTF_FLAG_PLUS)){
		len++;
		if ((format_signals & PRINTF_FLAG_PLUS)!=0 & !is_neg) sign_type=2u;
		else if ((format_signals & PRINTF_FLAG_SPACE)!=0 & sign_type==0u) sign_type=3u;
	}
	unsigned int has_width_or_precision=((format_signals & PRINTF_HAS_PRECISION) | (format_signals & PRINTF_HAS_WIDTH))!=0;
	if (has_width_or_precision){
		_print_sub1(print_target,len,sign_type,(format_signals & PRINTF_FLAG_NEG)!=0,format_signals,width,precision);
	} else {
		unsigned c=0u;
		switch (sign_type){
			case 1:c='-';break;
			case 2:c='+';break;
			case 3:c=' ';break;
		}
		if (c!=0u) _print_target_char(print_target,c);
	}
	for (unsigned int i=skip;i<5;i++){
		_print_target_char(print_target,buf[i]);
	}
	if (has_width_or_precision & ((format_signals & PRINTF_FLAG_NEG)!=0)){
		_print_sub1(print_target,len,sign_type,2,format_signals,width,precision);
	}
}

// prints a (un)signed long in decimal format
void _print_sub5(struct _print_target* print_target, unsigned long value, unsigned int format_signals, unsigned int width, unsigned int precision){
	if (format_signals & PRINTF_VAL_SIGNED){
		if ((int)(PRINTF_REINTERP_TO_SIGNED_LONG(value))==(PRINTF_REINTERP_TO_SIGNED_LONG(value))){
			// you know what a fully correct and faster option is for this value and sign? the one that prints an integer instead of a long, because this value can fit in an integer
			_print_sub4(print_target,(int)value,format_signals,width,precision);
			return;
		}
	} else {
		if ((unsigned int)value==value){
			// you know what a fully correct and faster option is for this value and sign? the one that prints an unsigned integer instead of an unsigned long, because this value can fit in an unsigned integer
			_print_sub4(print_target,(unsigned)value,format_signals,width,precision);
			return;
		}
	}
	if (format_signals & PRINTF_HAS_PRECISION){
		format_signals &= ~PRINTF_FLAG_PADZERO; // it seems like having a precision specified will override zero pad
	}
	unsigned long uvalue0,uvalue1;
	unsigned int uvalue2;
	int is_neg;
	if (format_signals & PRINTF_VAL_SIGNED){
		is_neg=PRINTF_REINTERP_TO_SIGNED_LONG(value)<0;
		uvalue0=is_neg ? - PRINTF_REINTERP_TO_SIGNED_LONG(value) : PRINTF_REINTERP_TO_SIGNED_LONG(value);
	} else {
		is_neg=0;
		uvalue0=value;
	}
	unsigned int sign_type=is_neg;
	unsigned int buf[10];
	uvalue1=uvalue0 /10LU;
	buf[9]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue0=uvalue1;uvalue1=uvalue0 /10LU;
	buf[8]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue0=uvalue1;uvalue1=uvalue0 /10LU;
	buf[7]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue0=uvalue1;uvalue1=uvalue0 /10LU;
	buf[6]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue0=uvalue1;uvalue1=uvalue0 /10LU;
	buf[5]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue0=uvalue1;uvalue1=uvalue0 /10LU;
	buf[4]=(uvalue0 - (uvalue1 *10LU))+'0';
	uvalue2=uvalue1; // 6 digits have been generated, value is now garenteed to be in range to use 16 bit operations
	buf[3]=((uvalue2       )%10u)+'0';
	buf[2]=((uvalue2 /  10u)%10u)+'0';
	buf[1]=((uvalue2 / 100u)%10u)+'0';
	buf[0]=((uvalue2 /1000u)%10u)+'0';
	unsigned int skip=((((format_signals & PRINTF_HAS_PRECISION)!=0 & (precision==0u & value==0)))!=0);
	for (unsigned int i=0;i<9;i++){
		if (buf[i]=='0') skip++;
		else break;
	}
	unsigned int len=10u -skip;
	if (is_neg | (format_signals & PRINTF_FLAG_SPACE) | (format_signals & PRINTF_FLAG_PLUS)){
		len++;
		if ((format_signals & PRINTF_FLAG_PLUS)!=0 & !is_neg) sign_type=2u;
		else if ((format_signals & PRINTF_FLAG_SPACE)!=0 & sign_type==0u) sign_type=3u;
	}
	unsigned int has_width_or_precision=((format_signals & PRINTF_HAS_PRECISION) | (format_signals & PRINTF_HAS_WIDTH))!=0;
	if (has_width_or_precision){
		_print_sub1(print_target,len,sign_type,(format_signals & PRINTF_FLAG_NEG)!=0,format_signals,width,precision);
	} else {
		unsigned c=0u;
		switch (sign_type){
			case 1:c='-';break;
			case 2:c='+';break;
			case 3:c=' ';break;
		}
		if (c!=0u) _print_target_char(print_target,c);
	}
	for (unsigned int i=skip;i<10;i++){
		_print_target_char(print_target,buf[i]);
	}
	if (has_width_or_precision & ((format_signals & PRINTF_FLAG_NEG)!=0)){
		_print_sub1(print_target,len,sign_type,2,format_signals,width,precision);
	}
}

// prints an unsigned integer in hexadecimal format
void _print_sub6(struct _print_target* print_target, unsigned int value, unsigned int format_signals, unsigned int width, unsigned int precision){
	if (format_signals & PRINTF_HAS_PRECISION){
		format_signals &= ~PRINTF_FLAG_PADZERO; // it seems like having a precision specified will override zero pad
	}
	unsigned int buf[4];
	buf[3]=((value >> 0u)&0xFu)+'0';
	buf[2]=((value >> 4u)&0xFu)+'0';
	buf[1]=((value >> 8u)&0xFu)+'0';
	buf[0]=((value >>12u)&0xFu)+'0';
	if (format_signals & PRINTF_SIG_UPPERCASE){
		buf[3]+=('A' - ('9' + 1))*(buf[3]>(unsigned)(9 + '0'));
		buf[2]+=('A' - ('9' + 1))*(buf[2]>(unsigned)(9 + '0'));
		buf[1]+=('A' - ('9' + 1))*(buf[1]>(unsigned)(9 + '0'));
		buf[0]+=('A' - ('9' + 1))*(buf[0]>(unsigned)(9 + '0'));
	} else {
		buf[3]+=('a' - ('9' + 1))*(buf[3]>(unsigned)(9 + '0'));
		buf[2]+=('a' - ('9' + 1))*(buf[2]>(unsigned)(9 + '0'));
		buf[1]+=('a' - ('9' + 1))*(buf[1]>(unsigned)(9 + '0'));
		buf[0]+=('a' - ('9' + 1))*(buf[0]>(unsigned)(9 + '0'));
	}
	unsigned int skip=((((format_signals & PRINTF_HAS_PRECISION)!=0 & (precision==0u & value==0u)))!=0);
	for (unsigned int i=0;i<3;i++){
		if (buf[i]=='0') skip++;
		else break;
	}
	unsigned int len=4u -skip;
	unsigned int has_width_or_precision=((format_signals & PRINTF_HAS_PRECISION) | (format_signals & PRINTF_HAS_WIDTH))!=0;
	if (has_width_or_precision){
		_print_sub1(print_target,len,0,(format_signals & PRINTF_FLAG_NEG)!=0,format_signals,width,precision);
	}
	for (unsigned int i=skip;i<4;i++){
		_print_target_char(print_target,buf[i]);
	}
	if (has_width_or_precision & ((format_signals & PRINTF_FLAG_NEG)!=0)){
		_print_sub1(print_target,len,0,2,format_signals,width,precision);
	}
}


void _print(struct _print_target* print_target,const char* format,va_list args){
	unsigned int c0;
	while ((c0=*(format++))!=0){
		if (c0!='%'){
			_print_target_char(print_target,c0);
		} else {
			// %[flags][width][.precision][length]specifier
			unsigned int format_signals=0;
			unsigned int width=0;
			unsigned int precision=0;
			// flags
			while (1){
				switch ((c0=*(format++))){
					case '-':format_signals |=PRINTF_FLAG_NEG;     break;
					case '+':format_signals |=PRINTF_FLAG_PLUS;    break;
					case '0':format_signals |=PRINTF_FLAG_PADZERO; break;
					case ' ':format_signals |=PRINTF_FLAG_SPACE;   break;
					// hash not supported
					default:goto FlagsDone;
				}
			}
			FlagsDone:;
			if (c0==0) return;
			// width
			if (c0=='*'){
				int temp_width=0;
				format_signals |=PRINTF_HAS_WIDTH;
				temp_width=va_arg(args,int);
				width=temp_width;
				if (temp_width<0){
					format_signals |=PRINTF_FLAG_NEG;// is the application of this flag correct?
					width=-temp_width;
				}
				if ((c0=*(format++))==0) return;
			} else {
				while (c0>='0' & c0<='9'){
					format_signals |=PRINTF_HAS_WIDTH;
					width=width *10+(c0 -'0');
					if ((c0=*(format++))==0) return;
				}
			}
			// precision
			if (c0=='.'){
				format_signals |=PRINTF_HAS_PRECISION;
				if ((c0=*(format++))==0) return;
				if (c0=='*'){
					int temp_precision=0;
					temp_precision=va_arg(args,int);
					precision=temp_precision;
					if (temp_precision<0){
						precision=-temp_precision;
					}
					if ((c0=*(format++))==0) return;
				} else {
					while (c0>='0' & c0<='9'){
						precision=precision *10+(c0 -'0');
						if ((c0=*(format++))==0) return;
					}
				}
			}
			unsigned int c1;
			c1=*format;
			// length
			format_signals |=PRINTF_VAL_SIGNED; // default assignment. needed for `n` specifier
			switch (c0){
				case 'l':
				if (c1=='l'){
					// this should be long long, but that isn't supported right now
					format_signals |=PRINTF_VAL_LONG;
					if ((*(format++))==0) return;
				} else {
					format_signals |=PRINTF_VAL_LONG;
				}
				if ((c0=*(format++))==0) return;
				break;
				case 'h':
				if (c1=='h'){
					format_signals |=PRINTF_VAL_CHAR;
					if ((*(format++))==0) return;
				} else {
					format_signals |=PRINTF_VAL_SHORT;
				}
				if ((c0=*(format++))==0) return;
				break;
				case 'z':
				format_signals &= ~ PRINTF_VAL_SIGNED;
				case 'j':
				case 't':
				format_signals |=PRINTF_VAL_LONG;
				if ((c0=*(format++))==0) return;
				break;
				//case 'L':break;//not supported
				default:break;
			}
			if ((format_signals & PRINTF_FLAG_NEG) && (format_signals & PRINTF_FLAG_PADZERO)){
				format_signals &= ~ PRINTF_FLAG_PADZERO; // left justify overrides zero padding if both are used
			}
			// specifier
			switch (c0){
				case '%':
				_print_target_char(print_target,'%');
				break;
				case 'd':
				case 'i':
				{
				format_signals |=PRINTF_VAL_SIGNED;
				if (format_signals & PRINTF_VAL_LONG){
					long v=va_arg(args,long);
					_print_sub5(print_target,PRINTF_REINTERP_TO_UNSIGN_LONG(v),format_signals,width,precision);
				} else {
					int v=va_arg(args,int);
					_print_sub4(print_target,PRINTF_REINTERP_TO_UNSIGN_INT(v),format_signals,width,precision);
				}
				}
				break;
				case 'u':
				{
				format_signals &= ~ PRINTF_VAL_SIGNED;
				if (format_signals & PRINTF_VAL_LONG){
					unsigned long v=va_arg(args,unsigned long);
					_print_sub5(print_target,v,format_signals,width,precision);
				} else {
					unsigned int v=va_arg(args,unsigned int);
					_print_sub4(print_target,v,format_signals,width,precision);
				}
				}
				break;
				case 'X':
				format_signals |=PRINTF_SIG_UPPERCASE;
				case 'x':
				{
				format_signals &= ~ PRINTF_VAL_SIGNED;
				unsigned int v=va_arg(args,unsigned int);
				_print_sub6(print_target,v,format_signals,width,precision);
				}
				break;
				case 'c':
				{
				char v=va_arg(args,int);
				_print_target_char(print_target,v);
				}
				break;
				case 's':
				{
				const char* v=va_arg(args,const char*);
				_print_sub3(print_target,v,format_signals,width,precision);
				}
				break;
				case 'n':
				{
				void* v0=va_arg(args,void*);
				if (format_signals & PRINTF_VAL_CHAR){
					if (format_signals & PRINTF_VAL_SIGNED){
						*((signed char*)v0)=print_target->writeCount;
					} else {
						*((unsigned char*)v0)=print_target->writeCount;
					}
				} else if (format_signals & PRINTF_VAL_SHORT){
					if (format_signals & PRINTF_VAL_SIGNED){
						*((signed short*)v0)=print_target->writeCount;
					} else {
						*((unsigned short*)v0)=print_target->writeCount;
					}
				} else if (format_signals & PRINTF_VAL_LONG){
					if (format_signals & PRINTF_VAL_SIGNED){
						*((signed long*)v0)=print_target->writeCount;
					} else {
						*((unsigned long*)v0)=print_target->writeCount;
					}
				} else {
					if (format_signals & PRINTF_VAL_SIGNED){
						*((signed int*)v0)=print_target->writeCount;
					} else {
						*((unsigned int*)v0)=print_target->writeCount;
					}
				}
				}
				break;
				default:break;// either not supported or invalid
			}
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

char* fgets(char* str,int num, FILE* stream){
	if (num<=0) return NULL; // what is it supposed to do?
	int i=0;
	int c;
	while (1){
		if (i==num -1){
			str[i]=0;
			return str;
		}
		c=fgetc(stream);
		if (c==EOF){
			if (i!=0){
				str[i]=0;
			}
			return NULL;
		}
		str[i++]=c;
		if (c=='\n'){
			str[i]=0;
			return str;
		}
	}
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

