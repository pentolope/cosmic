

typedef struct InstructionSingle{
	enum InstructionTypeID {
		I_ERR_ = 0, // I_ERR_ is not an instruction, if it is encountered, the compiler should terminate execution
		I_NOP_,
		I_PU1_,
		I_PU2_,
		I_PUA1,
		I_PUA2,
		I_POP1,
		I_POP2,
		I_BL1_,
		I_RL1_,
		I_RL2_,
		I_CALL,
		I_RET_,
		I_STPA,
		I_STPS,

		I_STWN,
		I_STRN,
		I_STWV,
		I_STRV,

		I_ALOC,
		I_ALCR,
		I_STOF,
		
		I_AJMP,
		I_CJMP,
		
		I_JJMP,
		I_JTEN,
		I_JEND,

		I_MOV_,

		I_AND_,
		I_OR__,
		I_XOR_,
		I_SSUB,

		I_ADDN,
		I_ADDC,
		I_SUBN,
		I_SUBC,

		I_MULS,
		I_MULL,
		I_DIVM,

		I_SHFT,
		I_BSWP,

		I_MWWN,
		I_MRWN,
		I_MWWV,
		I_MRWV,
		I_MWBN,
		I_MRBN,
		I_MWBV,
		I_MRBV,

		I_LABL,
		
		I_PHIS,
		I_PHIE,
		
		I_D32U,
		I_R32U,
		I_D32S,
		I_R32S,
		
		I_FCST,
		I_FCEN,
		
		I_SYRB,
		I_SYRW,
		I_SYRD,
		I_SYRE,

		I_SYCB,
		I_SYCW,
		I_SYCD,
		I_SYCL,

		I_SYC0,
		I_SYC1,
		I_SYC2,
		I_SYC3,
		I_SYC4,
		I_SYC5,
		I_SYC6,
		I_SYC7,
		I_SYC8,
		I_SYC9,
		
		I_SYCX,
		I_SYCY,
		I_SYCA,
		I_SYCU,
		I_SYCO,
		I_SYCQ,
		I_SYCC,
		I_SYCN,
		I_SYCM,
		
		I_SYCZ,
		I_SYCS,
		I_SYCT,
		
		// the following should not be encontered by the instruction optimizer
		I_NSNB,
		I_NSCB,
		I_NSNW,
		I_NSCW,
		I_ZNXB,
		I_ZNXW,

		I_BYTE,
		I_WORD,
		I_DWRD,

		I_SYDB,
		I_SYDW,
		I_SYDD,
		I_SYDE,
		
		I_DEPL, // I_DEPL is used for when a depth limit is reached for peephole optimization templates, it is not an instruction
		I_PEPH, // used in peephole optimization templates for poping or pushing a value
		
		I_STPI, // for placing initializer offsets. Is converted to I_STPA after the variable's local offset is calculated.
		
		I_LOFF, // used temporarily by static initializers to indicate an offset of the data that comes next
		
		I_INSR // this is the insert placeholder for core entries, it is not an instruction. It uses the 'B' struct in the 'arg' union
	} id;
	union {
		struct {
			uint8_t a_0;
		} B;
		struct {
			uint8_t a_0;
			uint8_t a_1;
		} BB;
		struct {
			uint8_t a_0;
			uint8_t a_1;
			uint8_t a_2;
		} BBB;
		struct {
			uint8_t a_0;
			uint16_t a_1;
		} BW;
		struct {
			uint8_t a_0;
			uint8_t a_1;
			uint16_t a_2;
		} BBW;
		struct {
			uint8_t a_0;
			uint8_t a_1;
			uint32_t a_2;
		} BBD;
		struct {
			uint8_t a_0;
			uint16_t a_1;
			uint32_t a_2;
		} BWD;
		struct {
			uint16_t a_0;
		} W;
		struct {
			uint32_t a_0;
			bool mark; // [for use on LABL only] may be used in the optimizer to avoid infinite loops (as of writing this it is unused)
		} D;
	} arg;
} InstructionSingle;


/*
I_PHIS - phi start
I_PHIE - phi end

phi nodes are used to access register values across jumps and labels.
the I_PHIS 'instruction', according to the optimizer, contains one register of  input that is not renamable and not reorderable
the I_PHIE 'instruction', according to the optimizer, contains one register of output that is not renamable and not reorderable
*/



typedef struct CompressedInstructionBuffer{
	uint8_t* byteCode;
	uint32_t allocLen;
} CompressedInstructionBuffer;


// InstructionBuffer has initializers (that are generated by the python script), so don't change it around
typedef struct InstructionBuffer{
	InstructionSingle* buffer;
	uint32_t numberOfSlotsAllocated;
	uint32_t numberOfSlotsTaken;
} InstructionBuffer;

#ifdef INCLUDE_BACKEND
uint32_t backendInstructionSize(const InstructionSingle IS){
	switch (IS.id){
		case I_NOP_:
		case I_JEND:
		case I_SYRE:
		case I_SYDE:
		case I_SYC0:
		case I_SYC1:
		case I_SYC2:
		case I_SYC3:
		case I_SYC4:
		case I_SYC5:
		case I_SYC6:
		case I_SYC7:
		case I_SYC8:
		case I_SYC9:
		case I_SYCZ:
		case I_SYCS:
		case I_SYCT:
		case I_SYCX:
		case I_SYCY:
		case I_SYCA:
		case I_SYCU:
		case I_SYCO:
		case I_SYCQ:
		case I_SYCC:
		case I_SYCN:
		case I_SYCM:
		case I_PHIS:
		case I_PHIE:
		case I_FCST:
		case I_FCEN:
		case I_NSNB:
		case I_NSCB:
		case I_NSNW:
		case I_NSCW:
		case I_LABL:
		case I_SYCD:
		case I_SYCW:
		case I_SYCB:
		case I_SYCL:
		return 0;
		case I_SYDB:
		case I_BYTE:
		return 1;
		case I_RET_:
		case I_PU1_:
		case I_PU2_:
		case I_PUA1:
		case I_PUA2:
		case I_POP1:
		case I_POP2:
		case I_CALL:
		case I_MULS:
		case I_MULL:
		case I_DIVM:
		case I_SHFT:
		case I_BSWP:
		case I_MOV_:
		case I_AND_:
		case I_OR__:
		case I_XOR_:
		case I_ADDN:
		case I_ADDC:
		case I_SSUB:
		case I_SUBN:
		case I_SUBC:
		case I_MWBN:
		case I_MRBN:
		case I_MWBV:
		case I_MRBV:
		case I_MWWN:
		case I_MRWN:
		case I_MWWV:
		case I_MRWV:
		case I_STWN:
		case I_STRN:
		case I_STWV:
		case I_STRV:
		case I_CJMP:
		case I_JJMP:
		case I_AJMP:
		case I_SYRB:
		case I_BL1_:
		case I_SYDW:
		case I_WORD:
		return 2;
		case I_SYRW:
		case I_RL1_:
		case I_STPS:
		case I_JTEN:
		case I_SYDD:
		case I_DWRD:
		return 4;
		case I_STPA:
		case I_ALOC:
		return 6;
		case I_RL2_:
		case I_SYRD:
		return 8;
		case I_D32U:
		case I_R32U:
		case I_D32S:
		case I_R32S:
		return 14;
		case I_ALCR:return 4u+(((unsigned)IS.arg. BW.a_1&0xFF00u)!=0u)*2u;
		case I_STOF:return 8u+(((unsigned)IS.arg.BBW.a_2&0xFF00u)!=0u)*2u;
		case I_ZNXB:return IS.arg.D.a_0;
		case I_ZNXW:return IS.arg.D.a_0*2u;
		default:;
	}
	assert(false); // invalid operator id
	exit(1);
}


// symVal includes JTEN
// symVal includes location of related label for D32U,D32S,R32U,R32S
void backendInstructionWrite(uint8_t** word,uint32_t symVal,uint16_t func_stack_size,uint8_t func_stack_initial,const InstructionSingle IS){
	uint8_t b0=((unsigned)IS.arg.BB.a_0&0xFu)|(((unsigned)IS.arg.BB.a_1&0xFu)<<4);
	uint8_t b1=((unsigned)IS.arg.BBB.a_0&0xFu)|(((unsigned)IS.arg.BBB.a_1&0xFu)<<4);
	uint8_t b2=(unsigned)IS.arg.BBB.a_2&0xFu;
	uint16_t w;
	switch (IS.id){
		case I_NOP_:
		case I_JEND:
		case I_SYRE:
		case I_SYDE:
		case I_SYC0:
		case I_SYC1:
		case I_SYC2:
		case I_SYC3:
		case I_SYC4:
		case I_SYC5:
		case I_SYC6:
		case I_SYC7:
		case I_SYC8:
		case I_SYC9:
		case I_SYCZ:
		case I_SYCS:
		case I_SYCT:
		case I_SYCX:
		case I_SYCY:
		case I_SYCA:
		case I_SYCU:
		case I_SYCO:
		case I_SYCQ:
		case I_SYCC:
		case I_SYCN:
		case I_SYCM:
		case I_PHIS:
		case I_PHIE:
		case I_FCST:
		case I_FCEN:
		case I_NSNB:
		case I_NSCB:
		case I_NSNW:
		case I_NSCW:
		case I_LABL:
		case I_SYCD:
		case I_SYCW:
		case I_SYCB:
		case I_SYCL:
		return;
		
		case I_SYDB:*((*word)++)=symVal;return;
		case I_BYTE:*((*word)++)=IS.arg.B.a_0;return;
		
		case I_PU1_:case I_PUA1:*((*word)++)=0xF0;*((*word)++)=(unsigned)IS.arg.B.a_0&0xFu;return;
		case I_POP1:*((*word)++)=0xF2;*((*word)++)=IS.arg.B.a_0&0xFu;return;
		case I_PU2_:case I_PUA2:*((*word)++)=0xF1;*((*word)++)=b0;return;
		case I_POP2:*((*word)++)=0xF3;*((*word)++)=b0;return;
		case I_MOV_:*((*word)++)=0xF4;*((*word)++)=b0;return;
		case I_BSWP:*((*word)++)=0xF5;*((*word)++)=b0;return;
		case I_SHFT:*((*word)++)=0xF6;*((*word)++)=b0;return;
		case I_MULS:*((*word)++)=0xF7;*((*word)++)=b0;return;
		case I_MULL:*((*word)++)=0xF8;*((*word)++)=b0;return;
		case I_DIVM:*((*word)++)=0xF9;*((*word)++)=b0;return;
		case I_CALL:*((*word)++)=0xFA;*((*word)++)=b0;return;
		case I_MWBN:case I_MWBV:*((*word)++)=0xFC;*((*word)++)=b0;return;
		case I_MRBN:case I_MRBV:*((*word)++)=0xFD;*((*word)++)=b0;return;
		case I_JJMP:*((*word)++)=0xFE;*((*word)++)=b0;return;
		case I_AJMP:*((*word)++)=0xFE;*((*word)++)=b0;return;
		case I_RET_:*((*word)++)=0xFB;*((*word)++)=0;return;
		case I_AND_:*((*word)++)=0x40|b2;*((*word)++)=b1;return;
		case I_OR__:*((*word)++)=0x50|b2;*((*word)++)=b1;return;
		case I_XOR_:*((*word)++)=0x60|b2;*((*word)++)=b1;return;
		case I_SSUB:*((*word)++)=0x70|b2;*((*word)++)=b1;return;
		case I_MWWN:case I_MWWV:*((*word)++)=0x80|b2;*((*word)++)=b1;return;
		case I_MRWN:case I_MRWV:*((*word)++)=0x90|b2;*((*word)++)=b1;return;
		case I_ADDN:*((*word)++)=0xA0|b2;*((*word)++)=b1;return;
		case I_ADDC:*((*word)++)=0xB0|b2;*((*word)++)=b1;return;
		case I_SUBN:*((*word)++)=0xC0|b2;*((*word)++)=b1;return;
		case I_SUBC:*((*word)++)=0xD0|b2;*((*word)++)=b1;return;
		case I_CJMP:*((*word)++)=0xE0|b2;*((*word)++)=b1;return;
		
		case I_SYRB:*((*word)++)=(((unsigned)symVal>>4)&0xFu);*((*word)++)=(((unsigned)symVal&0xFu)<<4)|((unsigned)IS.arg.B.a_0&0xFu);return;
		case I_BL1_:*((*word)++)=(((unsigned)IS.arg.BB.a_1>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BB.a_1&0xFu)<<4)|((unsigned)IS.arg.BB.a_0&0xFu);return;
		case I_STRN:case I_STRV:*((*word)++)=0x20u|(((unsigned)IS.arg.BB.a_1>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BB.a_1&0xFu)<<4)|((unsigned)IS.arg.BB.a_0&0xFu);return;
		case I_STWN:case I_STWV:*((*word)++)=0x30u|(((unsigned)IS.arg.BB.a_1>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BB.a_1&0xFu)<<4)|((unsigned)IS.arg.BB.a_0&0xFu);return;
		
		case I_SYDW:*((*word)++)=IS.arg.W.a_0;*((*word)++)=(unsigned)IS.arg.W.a_0>>8;return;
		case I_WORD:*((*word)++)=IS.arg.W.a_0;*((*word)++)=(unsigned)IS.arg.W.a_0>>8;return;
		
		case I_SYRW:
		*((*word)++)=(((unsigned)symVal>>4)&0xFu);*((*word)++)=(((unsigned)symVal&0xFu)<<4)|((unsigned)IS.arg.B.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)symVal>>12)&0xFu);*((*word)++)=(((unsigned)symVal>>4)&0xF0u)|((unsigned)IS.arg.B.a_0&0xFu);
		return;
		case I_RL1_:
		*((*word)++)=(((unsigned)IS.arg.BW.a_1>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BW.a_1&0xFu)<<4)|((unsigned)IS.arg.BW.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)IS.arg.BW.a_1>>12)&0xFu);*((*word)++)=(((unsigned)IS.arg.BW.a_1>>4)&0xF0u)|((unsigned)IS.arg.BW.a_0&0xFu);
		return;
		case I_STPS:
		w=func_stack_size-IS.arg.BW.a_1;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|((unsigned)IS.arg.BW.a_0&0xFu);
		*((*word)++)=0xA1;*((*word)++)=(((unsigned)IS.arg.BW.a_0<<4)&0xF0u)|((unsigned)IS.arg.BW.a_0&0xFu);
		return;
		
		case I_JTEN:case I_SYDD:*((*word)++)=symVal;*((*word)++)=(unsigned)symVal>>8;*((*word)++)=symVal>>16;*((*word)++)=symVal>>24;return;
		case I_DWRD:*((*word)++)=IS.arg.D.a_0;*((*word)++)=(unsigned)IS.arg.D.a_0>>8;*((*word)++)=IS.arg.D.a_0>>16;*((*word)++)=IS.arg.D.a_0>>24;return;
		return;
		case I_STPA:
		w=func_stack_size-IS.arg.BW.a_1;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|((unsigned)IS.arg.BW.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)w>>12)&0xFu);*((*word)++)=(((unsigned)w>>4)&0xF0u)|((unsigned)IS.arg.BW.a_0&0xFu);
		*((*word)++)=0xA1;*((*word)++)=(((unsigned)IS.arg.BW.a_0<<4)&0xF0u)|((unsigned)IS.arg.BW.a_0&0xFu);
		return;
		case I_ALOC:
		w=func_stack_size-func_stack_initial;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|1u;
		*((*word)++)=0x10u|(((unsigned)w>>12)&0xFu);*((*word)++)=(((unsigned)w>>4)&0xF0u)|1u;
		*((*word)++)=0xFF;*((*word)++)=1;
		return;
		case I_RL2_:
		*((*word)++)=(((unsigned)IS.arg.BBD.a_2>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BBD.a_2&0xFu)<<4)|((unsigned)IS.arg.BBD.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)IS.arg.BBD.a_2>>12)&0xFu);*((*word)++)=(((unsigned)IS.arg.BBD.a_2>>4)&0xF0u)|((unsigned)IS.arg.BBD.a_0&0xFu);
		w=IS.arg.BBD.a_2>>16;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|((unsigned)IS.arg.BBD.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)w>>12)&0xFu);*((*word)++)=(((unsigned)w>>4)&0xF0u)|((unsigned)IS.arg.BBD.a_0&0xFu);
		return;
		case I_SYRD:
		*((*word)++)=(((unsigned)symVal>>4)&0xFu);*((*word)++)=(((unsigned)symVal&0xFu)<<4)|((unsigned)IS.arg.BBD.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)symVal>>12)&0xFu);*((*word)++)=(((unsigned)symVal>>4)&0xF0u)|((unsigned)IS.arg.BBD.a_0&0xFu);
		w=symVal>>16;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|((unsigned)IS.arg.BBD.a_0&0xFu);
		*((*word)++)=0x10u|(((unsigned)w>>12)&0xFu);*((*word)++)=(((unsigned)w>>4)&0xF0u)|((unsigned)IS.arg.BBD.a_0&0xFu);
		return;
		case I_D32U:
		case I_R32U:
		case I_D32S:
		case I_R32S:
		*((*word)++)=0;*((*word)++)=0xA;*((*word)++)=0xF0;*((*word)++)=0xA;
		
		*((*word)++)=(((unsigned)symVal>>4)&0xFu);*((*word)++)=(((unsigned)symVal&0xFu)<<4)|0xAu;
		*((*word)++)=0x10u|(((unsigned)symVal>>12)&0xFu);*((*word)++)=(((unsigned)symVal>>4)&0xF0u)|0xAu;
		w=symVal>>16;
		*((*word)++)=(((unsigned)w>>4)&0xFu);*((*word)++)=(((unsigned)w&0xFu)<<4)|0xBu;
		*((*word)++)=0x10u|(((unsigned)w>>12)&0xFu);*((*word)++)=(((unsigned)w>>4)&0xF0u)|0xBu;
		
		*((*word)++)=0xFA;*((*word)++)=0xBA;
		return;
		case I_ALCR:
		*((*word)++)=(((unsigned)IS.arg.BW.a_1>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BW.a_1&0xFu)<<4)|((unsigned)IS.arg.BW.a_0&0xFu);
		if ((((unsigned)IS.arg. BW.a_1&0xFF00u)!=0u)){
			*((*word)++)=0x10u|(((unsigned)IS.arg.BW.a_1>>12)&0xFu);*((*word)++)=(((unsigned)IS.arg.BW.a_1>>4)&0xF0u)|((unsigned)IS.arg.BW.a_0&0xFu);
		}
		*((*word)++)=0xFF;*((*word)++)=(unsigned)IS.arg.BW.a_0&0xFu;
		return;
		case I_STOF:
		*((*word)++)=0;*((*word)++)=(unsigned)IS.arg.BBW.a_1&0xFu;
		*((*word)++)=0xFF;*((*word)++)=(unsigned)IS.arg.BBW.a_1&0xFu;
		w=(unsigned)IS.arg.BBW.a_0&0xFu;
		*((*word)++)=(((unsigned)IS.arg.BBW.a_2>>4)&0xFu);*((*word)++)=(((unsigned)IS.arg.BBW.a_2&0xFu)<<4)|((unsigned)w);
		if (((unsigned)IS.arg.BBW.a_2&0xFF00u)!=0u){
			*((*word)++)=0x10u|(((unsigned)IS.arg.BBW.a_2>>12)&0xFu);*((*word)++)=(((unsigned)IS.arg.BBW.a_2>>4)&0xF0u)|((unsigned)w);
		}
		*((*word)++)=0xA0|((unsigned)IS.arg.BBW.a_1&0xFu);*((*word)++)=(unsigned)w|((unsigned)w<<4);
		return;
		case I_ZNXB:
		for (uint32_t i=0;i<IS.arg.D.a_0;i++){
			*((*word)++)=0;
		}
		return;
		case I_ZNXW:
		for (uint32_t i=0;i<IS.arg.D.a_0;i++){
			*((*word)++)=0;*((*word)++)=0;
		}
		return;
		default:;
	}
	assert(false); // invalid operator id
	exit(1);
}
#endif //ifdef INCLUDE_BACKEND

uint8_t instructionContentCatagory(enum InstructionTypeID id){
	switch (id){
		case I_NOP_:
		case I_FCEN:
		case I_RET_:
		case I_ALOC:
		case I_D32U:
		case I_R32U:
		case I_D32S:
		case I_R32S:
		case I_JEND:
		case I_SYRE:
		case I_SYDB:
		case I_SYDW:
		case I_SYDD:
		case I_SYDE:
		case I_SYC0:
		case I_SYC1:
		case I_SYC2:
		case I_SYC3:
		case I_SYC4:
		case I_SYC5:
		case I_SYC6:
		case I_SYC7:
		case I_SYC8:
		case I_SYC9:
		case I_SYCZ:
		case I_SYCS:
		case I_SYCT:
		case I_SYCX:
		case I_SYCY:
		case I_SYCA:
		case I_SYCU:
		case I_SYCO:
		case I_SYCQ:
		case I_SYCC:
		case I_SYCN:
		case I_SYCM:
		return 0;
		case I_PU1_:
		case I_PUA1:
		case I_POP1:
		case I_SYRB:
		case I_SYRW:
		case I_PHIS:
		case I_PHIE:
		return 1;
		case I_BYTE:
		case I_SYCB:
		return 2;
		case I_PU2_:
		case I_PUA2:
		case I_POP2:
		case I_CALL:
		case I_AJMP:
		case I_MULS:
		case I_MULL:
		case I_DIVM:
		case I_SHFT:
		case I_BSWP:
		case I_MWBN:
		case I_MRBN:
		case I_MWBV:
		case I_MRBV:
		case I_MOV_:
		case I_SYRD:
		return 3;
		case I_AND_:
		case I_OR__:
		case I_XOR_:
		case I_SSUB:
		case I_ADDN:
		case I_ADDC:
		case I_SUBN:
		case I_SUBC:
		case I_MWWN:
		case I_MRWN:
		case I_MWWV:
		case I_MRWV:
		case I_CJMP:
		return 4;
		case I_BL1_:
		case I_STWN:
		case I_STRN:
		case I_STWV:
		case I_STRV:
		return 5;
		case I_WORD:
		case I_SYCW:
		return 6;
		case I_RL1_:
		case I_ALCR:
		case I_STPA:
		case I_STPS:
		return 7;
		case I_STOF:
		return 8;
		case I_LABL:
		case I_JTEN:
		case I_SYCD:
		case I_SYCL:
		case I_DWRD:
		case I_NSNB:
		case I_NSCB:
		case I_NSNW:
		case I_NSCW:
		case I_ZNXB:
		case I_ZNXW:
		return 9;
		case I_RL2_:
		case I_JJMP:
		return 10;
		case I_FCST:
		return 11;
		default:;
	}
	assert(false); // invalid operator id
	exit(1);
}

// returns the delta
uint8_t decompressInstruction(const uint8_t* byteCode,InstructionSingle* IS_parent){
	uint8_t delta=0;
	InstructionSingle IS;
	switch(instructionContentCatagory(IS.id=byteCode[0])){
		case 0:
		delta=1;
		break;
		case 1:
		case 2:
		IS.arg.B.a_0=(uint16_t)byteCode[1]&15U;
		delta=2;
		break;
		case 3:
		IS.arg.BB.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BB.a_1=(uint16_t)byteCode[1]/16U;
		delta=2;
		break;
		case 4:
		IS.arg.BBB.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BBB.a_1=(uint16_t)byteCode[1]/16U;
		IS.arg.BBB.a_2=(uint16_t)byteCode[2]&15U;
		delta=3;
		break;
		case 5:
		IS.arg.BB.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BB.a_1=(uint16_t)byteCode[2];
		delta=3;
		break;
		case 6:
		IS.arg.W.a_0=(uint16_t)byteCode[1]|(uint16_t)byteCode[2]*256U;
		delta=3;
		break;
		case 7:
		IS.arg.BW.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BW.a_1=(uint16_t)byteCode[2]|(uint16_t)byteCode[3]*256U;
		delta=4;
		break;
		case 8:
		IS.arg.BBW.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BBW.a_1=(uint16_t)byteCode[1]/16U;
		IS.arg.BBW.a_2=(uint16_t)byteCode[2]|(uint16_t)byteCode[3]*256U;
		delta=4;
		break;
		case 9:
		IS.arg.D.a_0=
		(uint32_t)((uint16_t)byteCode[1]|(uint16_t)byteCode[2]*256U)|
		(uint32_t)((uint16_t)byteCode[3]|(uint16_t)byteCode[4]*256U)*65536LU;
		delta=5;
		break;
		case 10:
		IS.arg.BBD.a_0=(uint16_t)byteCode[1]&15U;
		IS.arg.BBD.a_1=(uint16_t)byteCode[1]/16U;
		IS.arg.BBD.a_2=
		(uint32_t)((uint16_t)byteCode[2]|(uint16_t)byteCode[3]*256U)|
		(uint32_t)((uint16_t)byteCode[4]|(uint16_t)byteCode[5]*256U)*65536LU;
		delta=6;
		break;
		case 11:
		IS.arg.BWD.a_0=(uint16_t)byteCode[1];
		IS.arg.BWD.a_1=(uint16_t)byteCode[2]|(uint16_t)byteCode[3]*256U;
		IS.arg.BWD.a_2=
		(uint32_t)((uint16_t)byteCode[4]|(uint16_t)byteCode[5]*256U)|
		(uint32_t)((uint16_t)byteCode[6]|(uint16_t)byteCode[7]*256U)*65536LU;
		delta=8;
		break;
	}
	*IS_parent=IS;
	return delta;
}

#ifndef IS_BUILDING_RUN

void printInstructionBufferWithMessageAndNumber(const InstructionBuffer*,const char*,const uint32_t);
void printSingleInstructionOptCode(const InstructionSingle);
void addInstruction(InstructionBuffer*, const InstructionSingle);
void initInstructionBuffer(InstructionBuffer*);

// byteCodeEnd may be NULL. byteCodeEnd represents the position of the terminating zero
InstructionBuffer decompressInstructionBuffer(const uint8_t* byteCodeStart,const uint8_t** byteCodeEnd){
	InstructionBuffer ib;
	initInstructionBuffer(&ib);
	const uint8_t* byteCode=byteCodeStart;
	uint32_t i=0;
	while (byteCode[i]){
		InstructionSingle IS;
		i+=decompressInstruction(byteCode+i,&IS);
		addInstruction(&ib,IS);
	}
	if (byteCodeEnd!=NULL) *byteCodeEnd=byteCode+i;
	return ib;
}

void addInstruction(InstructionBuffer* ib, const InstructionSingle instructionSingle){
	if (ib->numberOfSlotsTaken>=ib->numberOfSlotsAllocated){
		assert(ib->numberOfSlotsAllocated!=0); // Cannot add instruction to destroyed instruction buffer
		ib->numberOfSlotsAllocated*=2;
		ib->buffer = cosmic_realloc(ib->buffer,ib->numberOfSlotsAllocated*sizeof(InstructionSingle));
	}
	ib->buffer[ib->numberOfSlotsTaken++]=instructionSingle;
}

// the first is modified, the second is not
void singleMergeIB(InstructionBuffer* ib1, const InstructionBuffer* ib2){
	uint32_t numberOfSlotsTakenOn2=ib2->numberOfSlotsTaken;
	const InstructionSingle* buffer2=ib2->buffer;
	for (uint32_t i=0;i<numberOfSlotsTakenOn2;i++){
		addInstruction(ib1,buffer2[i]);
	}
}

void dualMergeIB(InstructionBuffer* ib0,const InstructionBuffer* ib1,const InstructionBuffer* ib2){
	singleMergeIB(ib0,ib1);
	singleMergeIB(ib0,ib2);
}

void tripleMergeIB(InstructionBuffer* ib0,const InstructionBuffer* ib1,const InstructionBuffer* ib2,const InstructionBuffer* ib3){
	singleMergeIB(ib0,ib1);
	singleMergeIB(ib0,ib2);
	singleMergeIB(ib0,ib3);
}

void quadMergeIB(InstructionBuffer* ib0,const InstructionBuffer* ib1,const InstructionBuffer* ib2,const InstructionBuffer* ib3,const InstructionBuffer* ib4){
	singleMergeIB(ib0,ib1);
	singleMergeIB(ib0,ib2);
	singleMergeIB(ib0,ib3);
	singleMergeIB(ib0,ib4);
}

static const InstructionBuffer resetInstructionBuffer={0};
#define INIT_INSTRUCTION_BUFFER_SIZE 1024

void initInstructionBuffer(InstructionBuffer* ib){
	*ib=resetInstructionBuffer;
	ib->numberOfSlotsAllocated = INIT_INSTRUCTION_BUFFER_SIZE;
	ib->buffer = cosmic_malloc(ib->numberOfSlotsAllocated*sizeof(InstructionSingle));
}

void destroyInstructionBuffer(InstructionBuffer* ib){
	if (ib->buffer!=NULL){
		cosmic_free(ib->buffer);
		ib->buffer = NULL;
		ib->numberOfSlotsTaken = 0;
		ib->numberOfSlotsAllocated = 0;
	}
}


// returns the delta
// assumes that the space is avalible in byteCode
uint8_t compressInstruction(uint8_t* byteCode,const InstructionSingle IS){
	switch (instructionContentCatagory(byteCode[0]=IS.id)){
		case 0:
		return 1;
		case 1:
		case 2:
		byteCode[1]=IS.arg.B.a_0;
		return 2;
		case 3:
		byteCode[1]=IS.arg.BB.a_0|(IS.arg.BB.a_1*16);
		return 2;
		case 4:
		byteCode[1]=IS.arg.BBB.a_0|(IS.arg.BBB.a_1*16);
		byteCode[2]=IS.arg.BBB.a_2;
		return 3;
		case 5:
		byteCode[1]=IS.arg.BB.a_0;
		byteCode[2]=IS.arg.BB.a_1;
		return 3;
		case 6:
		byteCode[1]=IS.arg.W.a_0&255U;
		byteCode[2]=(IS.arg.W.a_0&(255U*256U))/256U;
		return 3;
		case 7:
		byteCode[1]=IS.arg.BW.a_0;
		byteCode[2]=IS.arg.BW.a_1&255U;
		byteCode[3]=(IS.arg.BW.a_1&(255U*256U))/256U;
		return 4;
		case 8:
		byteCode[1]=IS.arg.BBW.a_0|(IS.arg.BBW.a_1*16);
		byteCode[2]=IS.arg.BBW.a_2&255U;
		byteCode[3]=(IS.arg.BBW.a_2&(255U*256U))/256U;
		return 4;
		case 9:
		byteCode[1]=(uint16_t)IS.arg.D.a_0&255U;
		byteCode[2]=((uint16_t)IS.arg.D.a_0&(255U*256U))/256U;
		byteCode[3]=*((uint16_t*)&IS.arg.D.a_0+1)&255U;
		byteCode[4]=(*((uint16_t*)&IS.arg.D.a_0+1)&(255U*256U))/256U;
		return 5;
		case 10:
		byteCode[1]=IS.arg.BBD.a_0|(IS.arg.BBD.a_1*16);
		byteCode[2]=(uint16_t)IS.arg.BBD.a_2&255U;
		byteCode[3]=((uint16_t)IS.arg.BBD.a_2&(255U*256U))/256U;
		byteCode[4]=*((uint16_t*)&IS.arg.BBD.a_2+1)&255U;
		byteCode[5]=(*((uint16_t*)&IS.arg.BBD.a_2+1)&(255U*256U))/256U;
		return 6;
		case 11:
		byteCode[1]=IS.arg.BWD.a_0;
		byteCode[2]=IS.arg.BWD.a_1&255U;
		byteCode[3]=(IS.arg.BWD.a_1&(255U*256U))/256U;
		byteCode[4]=(uint16_t)IS.arg.BWD.a_2&255U;
		byteCode[5]=((uint16_t)IS.arg.BWD.a_2&(255U*256U))/256U;
		byteCode[6]=*((uint16_t*)&IS.arg.BWD.a_2+1)&255U;
		byteCode[7]=(*((uint16_t*)&IS.arg.BWD.a_2+1)&(255U*256U))/256U;
		return 8;
	}
	assert(false);
	exit(1);
}

uint8_t getStorageDeltaForInstruction(enum InstructionTypeID id){
	// if something changes, do not just change this, there are other places
	const uint8_t deltas[]={ // could be static
		[ 0]=1,[ 1]=2,[ 2]=2,[ 3]=2,[ 4]=3,[ 5]=3,[ 6]=3,[ 7]=4,[ 8]=4,[ 9]=5,[10]=6,[11]=8
	};
	return deltas[instructionContentCatagory(id)];
}

CompressedInstructionBuffer compressInstructionBuffer(const InstructionBuffer* ib){
	CompressedInstructionBuffer cib;
	cib.allocLen=1;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	for (uint32_t i=0;i<numberOfSlotsTaken;i++){
		cib.allocLen+=getStorageDeltaForInstruction(ib->buffer[i].id);
	}
	cib.byteCode=cosmic_malloc(cib.allocLen*sizeof(uint8_t));
	uint8_t* byteCode=cib.byteCode;
	for (uint32_t i=0;i<numberOfSlotsTaken;i++){
		byteCode+=compressInstruction(byteCode,ib->buffer[i]);
	}
	*byteCode=0;
	assert(cib.byteCode+(cib.allocLen-1)==byteCode);
	return cib;
}


struct InstructionBuffersOfFunctions{
	CompressedInstructionBuffer* slots;
	uint32_t numberOfSlotsAllocated;
	uint32_t numberOfSlotsTaken;
} globalInstructionBuffersOfFunctions;

// input is destroyed
void addEntryToInstructionBuffersOfFunctions(InstructionBuffer* ib){
	if (globalInstructionBuffersOfFunctions.numberOfSlotsTaken>=globalInstructionBuffersOfFunctions.numberOfSlotsAllocated){
		globalInstructionBuffersOfFunctions.numberOfSlotsAllocated+=100;
		globalInstructionBuffersOfFunctions.slots = cosmic_realloc(
			globalInstructionBuffersOfFunctions.slots,
			globalInstructionBuffersOfFunctions.numberOfSlotsAllocated*sizeof(InstructionBuffer));
	}
	CompressedInstructionBuffer cib=compressInstructionBuffer(ib);
	globalInstructionBuffersOfFunctions.slots[globalInstructionBuffersOfFunctions.numberOfSlotsTaken++]=cib;
	destroyInstructionBuffer(ib);
}

void applyLabelRenamesInAllCompressedInstructionBuffers(const uint32_t* fromArr,const uint32_t* toArr,const uint32_t arrLen){
	if (arrLen==0) return;
	for (uint32_t i0=0;i0<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i0++){
		uint8_t* byteCode=globalInstructionBuffersOfFunctions.slots[i0].byteCode;
		while (*byteCode!=0){
			bool didChange=false;
			enum InstructionTypeID id=*byteCode;
			InstructionSingle IS;
			uint8_t delta;
			if (id==I_LABL | id==I_SYCL | id==I_JTEN | id==I_FCST){
				delta=decompressInstruction(byteCode,&IS);
				for (uint32_t i1=0;i1<arrLen;i1++){
					if (IS.arg.D.a_0==fromArr[i1]){IS.arg.D.a_0=toArr[i1];didChange=true;break;}
				}
			} else if (id==I_JJMP){
				delta=decompressInstruction(byteCode,&IS);
				for (uint32_t i1=0;i1<arrLen;i1++){
					if (IS.arg.BBD.a_2==fromArr[i1]){IS.arg.BBD.a_2=toArr[i1];didChange=true;break;}
				}
			} else {
				delta=getStorageDeltaForInstruction(id);
			}
			if (didChange) compressInstruction(byteCode,IS);
			byteCode+=delta;
		}
	}
}

void applyLabelRenamesInInstructionBuffer(InstructionBuffer* ib,const uint32_t* fromArr,const uint32_t* toArr,const uint32_t arrLen){
	if (arrLen==0) return;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* buffer=ib->buffer;
	for (uint32_t i=0;i<numberOfSlotsTaken;i++){
		enum InstructionTypeID id=buffer[i].id;
		uint32_t lablVal;
		uint32_t* lablPtr;
		if (id==I_LABL | id==I_SYCL | id==I_JTEN | id==I_FCST){
			lablPtr=&buffer[i].arg.D.a_0;
			Search:
			lablVal=*lablPtr;
			for (uint32_t i1=0;i1<arrLen;i1++){
				if (lablVal==fromArr[i1]){*lablPtr=toArr[i1];break;}
			}
		} else if (id==I_JJMP){
			lablPtr=&buffer[i].arg.BBD.a_2;
			goto Search;
		}
	}
}

// label markoffs do not check LABL or FCST
void doLabelMarkoffInAllCompressedInstructionBuffers(uint32_t* labelMarkoff,const uint32_t arrLen){
	if (arrLen==0) return;
	for (uint32_t i0=0;i0<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i0++){
		uint8_t* byteCode=globalInstructionBuffersOfFunctions.slots[i0].byteCode;
		while (*byteCode!=0){
			enum InstructionTypeID id=*byteCode;
			InstructionSingle IS;
			uint32_t lablVal;
			uint8_t delta;
			if (id==I_SYCL | id==I_JTEN){
				delta=decompressInstruction(byteCode,&IS);
				lablVal=IS.arg.D.a_0;
				Search:
				for (uint32_t i1=0;i1<arrLen;i1++){
					if (lablVal==labelMarkoff[i1]){labelMarkoff[i1]=0;break;}
				}
			} else if (id==I_JJMP){
				delta=decompressInstruction(byteCode,&IS);
				lablVal=IS.arg.BBD.a_2;
				goto Search;
			} else {
				delta=getStorageDeltaForInstruction(id);
			}
			byteCode+=delta;
		}
	}
}

// label markoffs do not check LABL or FCST
void doLabelMarkoffInInstructionBuffer(const InstructionBuffer* ib,uint32_t* labelMarkoff,const uint32_t arrLen){
	if (arrLen==0) return;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* buffer=ib->buffer;
	for (uint32_t i=0;i<numberOfSlotsTaken;i++){
		enum InstructionTypeID id=buffer[i].id;
		uint32_t lablVal;
		if (id==I_SYCL | id==I_JTEN){
			lablVal=buffer[i].arg.D.a_0;
			Search:
			for (uint32_t i1=0;i1<arrLen;i1++){
				if (lablVal==labelMarkoff[i1]){labelMarkoff[i1]=0;break;}
			}
		} else if (id==I_JJMP){
			lablVal=buffer[i].arg.BBD.a_2;
			goto Search;
		}
	}
}




// these functions that follow are to avoid using InstructionSingle in expressionToInstructions


void addVoidPop(InstructionBuffer* ib){
	InstructionSingle IS;
	IS.id=I_POP1;
	IS.arg.B.a_0=3;
	addInstruction(ib,IS);
}

void addQuadVoidPop(InstructionBuffer* ib){
	addVoidPop(ib);
	addVoidPop(ib);
	addVoidPop(ib);
	addVoidPop(ib);
}

void addPopArgPush1(InstructionBuffer* ib){
	InstructionSingle IS;
	IS.id=I_POP1;
	IS.arg.B.a_0=3;
	addInstruction(ib,IS);
	IS.id=I_PUA1;
	IS.arg.B.a_0=3;
	addInstruction(ib,IS);
}

void addPopArgPush2(InstructionBuffer* ib){
	InstructionSingle IS;
	IS.id=I_POP2;
	IS.arg.BB.a_0=3;
	IS.arg.BB.a_1=4;
	addInstruction(ib,IS);
	IS.id=I_PUA2;
	IS.arg.BB.a_0=4;
	IS.arg.BB.a_1=3;
	addInstruction(ib,IS);
}


/*
addStructArgPush() is for if isActuallyAddressBecauseUnionOrStruct==true

if isActuallyAddressBecauseUnionOrStruct==false, then this should not be called
  and the argument should be treated as pushed as an argument. I don't think the optimizer will have a problem with that.
*/
void addStructArgPush(InstructionBuffer* ib, uint16_t size, bool isVolatile){
	InstructionSingle IS;
	IS.id=I_POP2;
	IS.arg.BB.a_0=3;
	IS.arg.BB.a_1=4;
	addInstruction(ib,IS);
	IS.id=I_BL1_;
	IS.arg.BB.a_0=6;
	IS.arg.BB.a_1=2;
	addInstruction(ib,IS);
	while (size){
		if (isVolatile) IS.id=I_MRWV;
		else IS.id=I_MRWN;
		IS.arg.BBB.a_0=5;
		IS.arg.BBB.a_1=3;
		IS.arg.BBB.a_2=4;
		addInstruction(ib,IS);
		IS.id=I_ADDC;
		IS.arg.BBB.a_0=3;
		IS.arg.BBB.a_1=3;
		IS.arg.BBB.a_2=6;
		addInstruction(ib,IS);
		IS.id=I_ADDN;
		IS.arg.BBB.a_0=4;
		IS.arg.BBB.a_1=4;
		IS.arg.BBB.a_2=15;
		addInstruction(ib,IS);
		IS.id=I_PUA1;
		IS.arg.B.a_0=5;
		addInstruction(ib,IS);
		size-=2;
	}
}


// pops an rvalue struct from the stack. should have address pushed after struct
// isVolatile obviously refers to the write operation.
// when it is finished, the original address is pushed back to the stack
void addStructStackAssign(InstructionBuffer* ib, uint16_t size, bool isVolatile){
	InstructionSingle IS;
	IS.id=I_POP2;
	IS.arg.BB.a_0=3;
	IS.arg.BB.a_1=4;
	addInstruction(ib,IS);
	while (size){
		size-=2;
		IS.id=I_POP1;
		IS.arg.B.a_0=5;
		addInstruction(ib,IS);
		IS.id=I_RL1_;
		IS.arg.BW.a_0=6;
		IS.arg.BW.a_1=size;
		addInstruction(ib,IS);
		IS.id=I_ADDC;
		IS.arg.BBB.a_0=7;
		IS.arg.BBB.a_1=3;
		IS.arg.BBB.a_2=6;
		addInstruction(ib,IS);
		IS.id=I_ADDN;
		IS.arg.BBB.a_0=8;
		IS.arg.BBB.a_1=4;
		IS.arg.BBB.a_2=15;
		addInstruction(ib,IS);
		if (isVolatile) IS.id=I_MWWV;
		else IS.id=I_MWWN;
		IS.arg.BBB.a_0=5;
		IS.arg.BBB.a_1=7;
		IS.arg.BBB.a_2=8;
		addInstruction(ib,IS);
	}
	IS.id=I_PU2_;
	IS.arg.BB.a_0=4;
	IS.arg.BB.a_1=3;
	addInstruction(ib,IS);
}


// for after an initializer finishes
void convertSTPI_STPA(InstructionBuffer* ib, uint16_t offsetToAdd){
	for (uint32_t i=0;i<ib->numberOfSlotsTaken;i++){
		if (ib->buffer[i].id==I_STPI){
			ib->buffer[i].id=I_STPA;
			uint16_t* vp=&ib->buffer[i].arg.BBW.a_2;
			*vp=offsetToAdd-*vp;
		}
	}
}
#endif //ifndef IS_BUILDING_RUN









