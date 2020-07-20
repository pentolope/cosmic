

#include "Common.c"

struct {
	struct StringLiteralEntry{
		uint32_t label;
		uint32_t lengthOfData; // in bytes, may not be word alligned
		bool isWideLiteral;
		uint8_t* data;
	}* entries;
	uint32_t numberOfValidEntries;
	uint32_t numberOfAllocatedSlots;
} globalStringLiteralEntries;

InstructionBuffer global_static_data={0};



// only one of asOctal and asHex should be true, obviously
// returns 200 if the character is not in range or neither boolean was true
uint8_t ucharAsciiAsNumberDigit(uint8_t character, bool asOctal, bool asHex){
	if (asOctal){
		if ((character>='0') & (character<='8')) return character-'0';
		else return 200;
	} else if (asHex){
		if ((character>='0') & (character<='9')) return character-'0';
		else if ((character>='a') & (character<='f')) return (character-'a')+10;
		else if ((character>='A') & (character<='F')) return (character-'A')+10;
		else return 200;
	}
	return 200;
}

// does include a null terminator
uint32_t findLengthOfDataOfStringLiteral(int32_t indexOfFirstQuote, int32_t indexOfLastQuote, bool isWideLiteral){
	if (isWideLiteral){
		printf("Wide literals not completely supported yet\n");
		exit(1);
	}
	uint32_t numberOfCharactersLong = (indexOfLastQuote-indexOfFirstQuote)-1;
	for (int32_t i=indexOfFirstQuote+1;i<indexOfLastQuote;i++){
		uint8_t c0 = (uint8_t)(sourceContainer.string[i]);
		if (c0=='\\'){
			uint8_t c1 = (uint8_t)(sourceContainer.string[i+1]);
			if (c1=='0'){
				numberOfCharactersLong-=3,i+=3;
			} else if (c1=='x'){
				if (isWideLiteral) numberOfCharactersLong-=5,i+=5;
				else numberOfCharactersLong-=3,i+=3;
			} else {
				numberOfCharactersLong-=1,i+=1;
			}
		}
	}
	return numberOfCharactersLong;
}


void writeStringLiteralData(
		uint8_t* dataDestination, 
		int32_t indexOfFirstQuote, 
		int32_t indexOfLastQuote, 
		uint32_t dataDestinationSize, 
		bool isWideLiteral){
	
	assert(dataDestinationSize>=findLengthOfDataOfStringLiteral(indexOfFirstQuote,indexOfLastQuote,isWideLiteral));
	uint32_t walkingIndex = 0;
	for (int32_t i=indexOfFirstQuote+1;i<indexOfLastQuote;i++){
		uint8_t c0 = (uint8_t)(sourceContainer.string[i  ]);
		uint8_t valueToSet = c0;
		if (c0=='\\'){
			uint8_t c1 = (uint8_t)(sourceContainer.string[i+1]);
			if (c1=='0'){
				uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
				uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
				c2 = ucharAsciiAsNumberDigit(c2,true,false);
				c3 = ucharAsciiAsNumberDigit(c3,true,false);
				if (c2==200 | c3==200){
					err_1101_("escape sequence not in octal range",i);
				}
				valueToSet=c2*8+c3;
				i+=2;
			} else if (c1=='x'){
				uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
				uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
				c2 = ucharAsciiAsNumberDigit(c2,false,true);
				c3 = ucharAsciiAsNumberDigit(c3,false,true);
				if (c2==200 | c3==200){
					err_1101_("escape sequence not in hexadecimal range",i);
				}
				if (isWideLiteral){
					uint8_t c4 = (uint8_t)(sourceContainer.string[i+4]);
					uint8_t c5 = (uint8_t)(sourceContainer.string[i+5]);
					c4 = ucharAsciiAsNumberDigit(c4,false,true);
					c5 = ucharAsciiAsNumberDigit(c5,false,true);
					if (c4==200 | c5==200){
						err_1101_("escape sequence not in hexadecimal range",i);
					}
					// not implemented further, as it is currently not supported
					i+=2;
				} else {
					valueToSet=c2*16+c3;
				}
				i+=2;
			} else if (c1=='a'){
				valueToSet=7;
			} else if (c1=='b'){
				valueToSet=8;
			} else if (c1=='f'){
				valueToSet=12;
			} else if (c1=='n'){
				valueToSet=10;
			} else if (c1=='r'){
				valueToSet=13;
			} else if (c1=='t'){
				valueToSet=9;
			} else if (c1=='v'){
				valueToSet=11;
			} else if (c1=='\''){
				valueToSet=39;
			} else if (c1=='\"'){
				valueToSet=34;
			} else if (c1=='\\'){
				valueToSet=92;
			} else if (c1=='?'){
				valueToSet=63;
			} else {
				err_1101_("unknown escape sequence",i);
			}
			i+=1;
		}
		dataDestination[walkingIndex++]=valueToSet;
	}
	dataDestination[walkingIndex-1]=0;
	assert(walkingIndex<=dataDestinationSize);
	assert(walkingIndex==findLengthOfDataOfStringLiteral(indexOfFirstQuote,indexOfLastQuote,isWideLiteral));
}

// returns the label number
// this is only for unmodifiable strings
uint32_t addEntryForStringData(int32_t indexOfFirstQuote, int32_t indexOfLastQuote, bool isWideLiteral){
	if (globalStringLiteralEntries.numberOfValidEntries>=globalStringLiteralEntries.numberOfAllocatedSlots){
		globalStringLiteralEntries.numberOfAllocatedSlots += 50;
		globalStringLiteralEntries.entries = cosmic_realloc(globalStringLiteralEntries.entries,sizeof(struct StringLiteralEntry)*globalStringLiteralEntries.numberOfAllocatedSlots);
	}
	struct StringLiteralEntry sle;
	struct StringLiteralEntry* sle_ptr = globalStringLiteralEntries.entries+globalStringLiteralEntries.numberOfValidEntries++;
	sle.isWideLiteral=isWideLiteral;
	sle.lengthOfData=findLengthOfDataOfStringLiteral(indexOfFirstQuote,indexOfLastQuote,isWideLiteral);
	sle.data=cosmic_malloc(sle.lengthOfData);
	writeStringLiteralData(sle.data,indexOfFirstQuote,indexOfLastQuote,sle.lengthOfData,isWideLiteral);
	sle.label = ++globalLabelID;
	*sle_ptr=sle;
	return sle.label;
}


void dataBytecodeReduction(InstructionBuffer* ib){
	ReductionOptFullRestart:;
	bool didApplySingle=false;
	bool isThisWordMisaligned=false;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i_this=0;
	uint32_t i_next=1;
	InstructionSingle* ptr_this=ib->buffer;
	InstructionSingle* ptr_next=ptr_this+1;
	while (i_next<numberOfSlotsTaken){
		InstructionSingle IS_this=*ptr_this;
		InstructionSingle IS_next=*ptr_next;
		if (IS_this.id==I_BYTE & IS_next.id==I_BYTE & !isThisWordMisaligned){
			ptr_next->id=I_WORD;
			ptr_next->arg.W.a_0=((unsigned)IS_next.arg.B1.a_0<<8)|((unsigned)IS_this.arg.B1.a_0);
			ptr_this->id=I_NOP_;
			numberOfSlotsTaken=ib->numberOfSlotsTaken;
			didApplySingle=true;goto ReductionOptSingleRestart;
		}
		if (IS_next.id==I_WORD & IS_this.id==I_WORD){
			ptr_next->id=I_DWRD;
			ptr_next->arg.D.a_0=((uint32_t)IS_next.arg.W.a_0<<16)|((uint32_t)IS_this.arg.W.a_0);
			ptr_this->id=I_NOP_;
			numberOfSlotsTaken=ib->numberOfSlotsTaken;
			didApplySingle=true;goto ReductionOptSingleRestart;
		}
		if (IS_next.id==I_ZNXB){
			if (IS_this.id==I_ZNXB){
				ptr_next->arg.D.a_0+=ptr_this->arg.D.a_0;
				ptr_this->id=I_NOP_;
				numberOfSlotsTaken=ib->numberOfSlotsTaken;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
			if (IS_this.id==I_BYTE & IS_this.arg.B1.a_0==0){
				ptr_next->arg.D.a_0++;
				ptr_this->id=I_NOP_;
				numberOfSlotsTaken=ib->numberOfSlotsTaken;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
			if (IS_this.id==I_WORD & (IS_this.arg.W.a_0&0xFF00u)==0u){
				ptr_this->id=I_BYTE;
				ptr_this->arg.B1.a_0=ptr_this->arg.W.a_0;
				ptr_next->arg.D.a_0++;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
			if (IS_this.id==I_DWRD & (IS_this.arg.D.a_0&0xFFFF0000lu)==0lu){
				ptr_this->id=I_WORD;
				ptr_this->arg.W.a_0=ptr_this->arg.D.a_0;
				ptr_next->arg.D.a_0+=2;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
		}
		if (IS_this.id==I_ZNXB){
			if (IS_next.id==I_BYTE & IS_next.arg.B1.a_0==0){
				ptr_this->arg.D.a_0++;
				ptr_next->id=I_NOP_;
				numberOfSlotsTaken=ib->numberOfSlotsTaken;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
			if (IS_next.id==I_WORD & (IS_next.arg.W.a_0&0x00FFu)==0u){
				ptr_next->id=I_BYTE;
				ptr_next->arg.B1.a_0=ptr_next->arg.W.a_0>>8;
				ptr_this->arg.D.a_0++;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
			if (IS_next.id==I_DWRD & (IS_next.arg.D.a_0&0x0000FFFFlu)==0u){
				ptr_next->id=I_WORD;
				ptr_next->arg.W.a_0=ptr_next->arg.D.a_0>>16;
				ptr_this->arg.D.a_0+=2;
				didApplySingle=true;goto ReductionOptSingleRestart;
			}
		}
		if      (IS_this.id==I_BYTE | IS_this.id==I_SYDB) isThisWordMisaligned^=1;
		else if (IS_this.id==I_ZNXB) isThisWordMisaligned^=IS_this.arg.D.a_0&1;
		else if ((IS_this.id==I_WORD | IS_this.id==I_DWRD) & isThisWordMisaligned){printInstructionBufferWithMessageAndNumber(ib,"",i_this);assert(false);}
		
		i_this=i_next++;
		ptr_this=ptr_next++;
		ReductionOptSingleRestart:;
	}
	if (didApplySingle) {removeNop(ib);goto ReductionOptFullRestart;}
}




// returns 0 if equal, 1 if s0 is lower, 2 if s1 is lower
uint8_t stringDataCompare(uint8_t* s0,uint8_t* s1,uint32_t length){
	for (uint32_t i=0;i<length;i++){
		char c0=s0[i];
		char c1=s1[i];
		if (c0!=c1) return (c0>c1)+1;
	}
	return 0;
}


