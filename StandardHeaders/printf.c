


// temporary implementation


struct {
	uint8_t font[665];
	uint8_t ansiBuffer[8];
	bool isAnsiEscapeOccuring;
	uint8_t current_forground;
	uint8_t current_background;
	uint16_t cursor;
	struct CharacterState{
		uint8_t character;
		uint8_t foreground;
		uint8_t background;
	} state[880];
} _terminalCharacterState={
	.current_forground=224,
	.current_background=1,
	.font={0,0,0,0,0,0,0,0,0,0,250,0,0,0,0,0,192,0,192,0,0,0,40,254,40,254,40,0,0,100,146,254,146,76,0,66,164,72,16,36,74,132,108,146,146,170,68,10,0,0,0,0,192,0,0,0,0,0,56,68,130,0,0,0,0,130,68,56,0,0,0,0,80,32,80,0,0,0,0,8,28,8,0,0,0,0,0,1,2,0,0,0,0,8,8,8,0,0,0,0,0,2,0,0,0,2,4,8,16,32,64,128,0,124,138,146,162,124,0,0,34,66,254,2,2,0,0,66,134,138,146,98,0,0,68,146,146,146,108,0,0,24,40,72,254,8,0,0,242,146,146,146,140,0,0,124,146,146,146,12,0,0,128,128,134,152,224,0,0,108,146,146,146,108,0,0,96,146,146,146,108,0,0,0,0,36,0,0,0,0,0,2,36,0,0,0,0,16,40,68,130,0,0,0,40,40,40,40,40,0,0,0,130,68,40,16,0,0,0,128,138,144,96,0,124,130,154,186,138,122,0,0,126,144,144,144,126,0,0,254,146,146,146,108,0,0,124,130,130,130,130,0,0,254,130,130,68,56,0,0,254,146,146,130,130,0,0,254,144,144,128,128,0,0,124,130,138,138,12,0,0,254,16,16,16,254,0,0,130,130,254,130,130,0,4,130,130,130,252,128,0,0,254,16,40,68,130,0,0,254,2,2,2,2,0,254,64,32,16,32,64,254,254,64,32,16,8,4,254,0,124,130,130,130,124,0,0,254,144,144,144,96,0,0,124,130,138,132,122,0,0,254,144,152,148,98,0,0,100,146,146,146,76,0,0,128,128,254,128,128,0,0,252,2,2,2,252,0,0,248,4,2,4,248,0,252,2,2,12,2,2,252,130,68,40,16,40,68,130,0,224,16,14,16,224,0,0,134,138,146,162,194,0,0,0,254,130,0,0,0,128,64,32,16,8,4,2,0,0,0,130,254,0,0,0,16,32,64,32,16,0,1,1,1,1,1,1,1,0,0,128,128,64,0,0,0,6,42,42,42,28,2,0,254,18,18,18,12,0,0,28,34,34,34,34,0,0,12,18,18,18,254,0,0,28,42,42,42,26,0,0,16,126,144,144,0,0,0,24,37,37,37,30,0,0,254,16,16,16,30,0,0,0,0,94,0,0,0,0,2,1,1,94,0,0,0,254,8,20,34,0,0,0,0,0,254,0,0,0,62,32,32,30,32,32,30,0,62,32,32,32,30,0,0,28,34,34,34,28,0,0,126,72,72,48,0,0,0,0,24,36,36,63,0,0,62,16,32,32,32,0,0,18,42,42,42,36,0,0,32,32,124,34,32,0,0,60,2,2,2,60,0,0,56,4,2,4,56,0,60,2,2,12,2,2,60,0,34,20,8,20,34,0,0,56,5,5,5,62,0,0,70,74,82,98,0,0,0,16,108,130,130,0,0,0,0,0,254,0,0,0,0,0,130,130,108,16,0,0,8,16,16,8,8,16}
};



void _putchar_write(uint16_t position){
	struct CharacterState characterState=_terminalCharacterState.state[position];
	if (characterState.character<' ' | characterState.character>'~'){
		characterState.character=' ';
	}
	uint32_t mem=0x04000000+320;
	uint16_t heightPosition=position/40u;
	uint16_t widthPosition=position%40u;
	mem+=widthPosition*8+(uint32_t)heightPosition*320*9;
	uint8_t* fontPosition=(uint8_t*)_terminalCharacterState.font+(characterState.character-' ')*7;
	*(uint8_t*)(mem+7+7*320lu)=characterState.background;
	*(uint8_t*)(mem+7+8*320lu)=characterState.background;
	for (uint16_t i=0;i<7u;i++){
		uint32_t memi=mem+i;
		uint8_t fv=*(fontPosition+i);
		bool b;
		*(uint8_t*)(memi+320*0)=characterState.background;
		b=(fv&(1u<<7))!=0;
		*(uint8_t*)(memi+320*1)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<6))!=0;
		*(uint8_t*)(memi+320*2)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<5))!=0;
		*(uint8_t*)(memi+320*3)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<4))!=0;
		*(uint8_t*)(memi+320*4)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<3))!=0;
		*(uint8_t*)(memi+320*5)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<2))!=0;
		*(uint8_t*)(memi+320*6)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<1))!=0;
		*(uint8_t*)(memi+320*7)=b*characterState.foreground | !b*characterState.background;
		b=(fv&(1u<<0))!=0;
		*(uint8_t*)(memi+320*8)=b*characterState.foreground | !b*characterState.background;
		*(uint8_t*)(mem+7+i*320lu)=characterState.background;
	}
}

void _putchar_ensure_cursor_normal(){
	while (_terminalCharacterState.cursor>=880){
		_terminalCharacterState.cursor-=80;
		for (uint16_t i=80;i<880;i++){
			_terminalCharacterState.state[i-80]=_terminalCharacterState.state[i];
		}
		for (uint16_t i=800;i<880;i++){
			_terminalCharacterState.state[i].character=' ';
		}
		for (uint16_t i=0;i<880;i++){
			_putchar_write(i);
		}
	}
}

void _putchar_screen(char c){
	if (c>=' ' & c<='~'){
		struct CharacterState characterState;
		characterState.character=c;
		characterState.foreground=_terminalCharacterState.current_forground;
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
}





















