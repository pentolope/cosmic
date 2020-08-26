
#define __STD_REAL
#define __BUILDING_SIM_LOADER


#define INCLUDE_BACKEND

static unsigned long _exit_ret_address;
static unsigned int _exit_ret_val_ptr;

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <stdlib.c>
#include <stdio.c>
#include <printf.c>
#include <string.c>


#define err_00__0(message)     _err_(message)
#define err_00__1(message)     _err_(message)
#define err_10_00(message)     _err_(message)
#define err_10_01(message)     _err_(message)
#define err_10_1_(message)     _err_(message)
#define err_010_0(message,s)   _err_(message)
#define err_010_1(message,s)   _err_(message)
#define err_011_0(message,s,e) _err_(message)
#define err_011_1(message,s,e) _err_(message)
#define err_11000(message,s)   _err_(message)
#define err_11001(message,s)   _err_(message)
#define err_1101_(message,s)   _err_(message)
#define err_11100(message,s,e) _err_(message)
#define err_11101(message,s,e) _err_(message)
#define err_1111_(message,s,e) _err_(message)

static void _err_(const char* message){
	fprintf(stderr,"%s\n",message);
	exit(1);
}


#include "Alloc.c"
#include "TargetInstructions/InstructionBase.c"
//#include "TargetInstructions/GeneratedInstructionInitialization.c"

#include "IntrinsicBuiltFiles/_intrinsic_c_functions.c"


void printSingleInstructionOptCode(const InstructionSingle instructionSingle){
switch(instructionSingle.id){
case I_NOP_:printf("NOP_");return;
case I_PU1_:printf("PU1_ %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_PU2_:printf("PU2_ %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_PUA1:printf("PUA1 %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_PUA2:printf("PUA2 %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_POP1:printf("POP1 %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_POP2:printf("POP2 %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_BL1_:printf("BL1_ %%%01X $%02X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_RL1_:printf("RL1_ %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_RL2_:printf("RL2_ %%%01X %%%01X !%04X%04X",(uint16_t)instructionSingle.arg.BBD.a_0,(uint16_t)instructionSingle.arg.BBD.a_1,(uint16_t)(instructionSingle.arg.BBD.a_2>>16),(uint16_t)instructionSingle.arg.BBD.a_2);return;
case I_CALL:printf("CALL %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_RET_:printf("RET_");return;
case I_STPA:printf("STPA %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_STPS:printf("STPS %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_STWN:printf("STWN %%%01X $%02X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_STRN:printf("STRN %%%01X $%02X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_STWV:printf("STWV %%%01X $%02X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_STRV:printf("STRV %%%01X $%02X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_ALOC:printf("ALOC");return;
case I_ALCR:printf("ALCR %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_STOF:printf("STOF %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_AJMP:printf("AJMP %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_CJMP:printf("CJMP %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_JJMP:printf("JJMP %%%01X %%%01X :%04X%04X",(uint16_t)instructionSingle.arg.BBD.a_0,(uint16_t)instructionSingle.arg.BBD.a_1,(uint16_t)(instructionSingle.arg.BBD.a_2>>16),(uint16_t)instructionSingle.arg.BBD.a_2);return;
case I_JTEN:printf("JTEN @%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_JEND:printf("JEND");return;
case I_MOV_:printf("MOV_ %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_AND_:printf("AND_ %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_OR__:printf("OR__ %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_XOR_:printf("XOR_ %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_SSUB:printf("SSUB %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_ADDN:printf("ADDN %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_SUBN:printf("SUBN %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_SUBC:printf("SUBC %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_MULS:printf("MULS %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_MULL:printf("MULL %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_DIVM:printf("DIVM %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_SHFT:printf("SHFT %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_BSWP:printf("BSWP %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_MWWN:printf("MWWN %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_MRWN:printf("MRWN %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_MWWV:printf("MWWV %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_MRWV:printf("MRWV %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B3.a_0,(uint16_t)instructionSingle.arg.B3.a_1,(uint16_t)instructionSingle.arg.B3.a_2);return;
case I_MWBN:printf("MWBN %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_MRBN:printf("MRBN %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_MWBV:printf("MWBV %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_MRBV:printf("MRBV %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_LABL:printf("LABL :%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_PHIS:printf("PHIS %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_PHIE:printf("PHIE %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_FCST:printf("FCST $%02X #%04X :%04X%04X",(uint16_t)instructionSingle.arg.BWD.a_0,(uint16_t)instructionSingle.arg.BWD.a_1,(uint16_t)(instructionSingle.arg.BWD.a_2>>16),(uint16_t)instructionSingle.arg.BWD.a_2);return;
case I_FCEN:printf("FCEN");return;
case I_D32U:printf("D32U");return;
case I_R32U:printf("R32U");return;
case I_D32S:printf("D32S");return;
case I_R32S:printf("R32S");return;
case I_D64U:printf("D64U");return;
case I_R64U:printf("R64U");return;
case I_D64S:printf("D64S");return;
case I_R64S:printf("R64S");return;
case I_LAD0:printf("LAD0 %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B6.a_0,(uint16_t)instructionSingle.arg.B6.a_1,(uint16_t)instructionSingle.arg.B6.a_2,(uint16_t)instructionSingle.arg.B6.a_3,(uint16_t)instructionSingle.arg.B6.a_4,(uint16_t)instructionSingle.arg.B6.a_5);return;
case I_LAD1:printf("LAD1 %%%01X %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B5.a_0,(uint16_t)instructionSingle.arg.B5.a_1,(uint16_t)instructionSingle.arg.B5.a_2,(uint16_t)instructionSingle.arg.B5.a_3,(uint16_t)instructionSingle.arg.B5.a_4);return;
case I_LAD2:printf("LAD2 %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B4.a_0,(uint16_t)instructionSingle.arg.B4.a_1,(uint16_t)instructionSingle.arg.B4.a_2,(uint16_t)instructionSingle.arg.B4.a_3);return;
case I_LAD3:printf("LAD3 %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B8.a_0,(uint16_t)instructionSingle.arg.B8.a_1,(uint16_t)instructionSingle.arg.B8.a_2,(uint16_t)instructionSingle.arg.B8.a_3,(uint16_t)instructionSingle.arg.B8.a_4,(uint16_t)instructionSingle.arg.B8.a_5,(uint16_t)instructionSingle.arg.B8.a_6,(uint16_t)instructionSingle.arg.B8.a_7);return;
case I_LAD4:printf("LAD4");return;
case I_LAD5:printf("LAD5");return;
case I_LSU0:printf("LSU0 %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B4.a_0,(uint16_t)instructionSingle.arg.B4.a_1,(uint16_t)instructionSingle.arg.B4.a_2,(uint16_t)instructionSingle.arg.B4.a_3);return;
case I_LSU3:printf("LSU3 %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B8.a_0,(uint16_t)instructionSingle.arg.B8.a_1,(uint16_t)instructionSingle.arg.B8.a_2,(uint16_t)instructionSingle.arg.B8.a_3,(uint16_t)instructionSingle.arg.B8.a_4,(uint16_t)instructionSingle.arg.B8.a_5,(uint16_t)instructionSingle.arg.B8.a_6,(uint16_t)instructionSingle.arg.B8.a_7);return;
case I_LSU4:printf("LSU4");return;
case I_LSU5:printf("LSU5");return;
case I_LMU3:printf("LMU3 %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B8.a_0,(uint16_t)instructionSingle.arg.B8.a_1,(uint16_t)instructionSingle.arg.B8.a_2,(uint16_t)instructionSingle.arg.B8.a_3,(uint16_t)instructionSingle.arg.B8.a_4,(uint16_t)instructionSingle.arg.B8.a_5,(uint16_t)instructionSingle.arg.B8.a_6,(uint16_t)instructionSingle.arg.B8.a_7);return;
case I_LMU4:printf("LMU4");return;
case I_LMU5:printf("LMU5");return;
case I_LDI4:printf("LDI4");return;
case I_LDI5:printf("LDI5");return;
case I_LLS6:printf("LLS6");return;
case I_LLS7:printf("LLS7");return;
case I_LRS6:printf("LRS6");return;
case I_LRS7:printf("LRS7");return;
case I_SYRB:printf("SYRB %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_SYRW:printf("SYRW %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_SYRD:printf("SYRD %%%01X %%%01X",(uint16_t)instructionSingle.arg.B2.a_0,(uint16_t)instructionSingle.arg.B2.a_1);return;
case I_SYRQ:printf("SYRQ %%%01X %%%01X %%%01X %%%01X",(uint16_t)instructionSingle.arg.B4.a_0,(uint16_t)instructionSingle.arg.B4.a_1,(uint16_t)instructionSingle.arg.B4.a_2,(uint16_t)instructionSingle.arg.B4.a_3);return;
case I_SYRE:printf("SYRE");return;
case I_NSNB:printf("NSNB !%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_ZNXB:printf("ZNXB !%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_BYTE:printf("BYTE $%02X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_WORD:printf("WORD #%04X",(uint16_t)instructionSingle.arg.W.a_0);return;
case I_DWRD:printf("DWRD !%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_SYDB:printf("SYDB");return;
case I_SYDW:printf("SYDW");return;
case I_SYDD:printf("SYDD");return;
case I_SYDQ:printf("SYDQ");return;
case I_SYDE:printf("SYDE");return;
case I_SYCB:printf("SYCB $%02X",(uint16_t)instructionSingle.arg.B1.a_0);return;
case I_SYCW:printf("SYCW #%04X",(uint16_t)instructionSingle.arg.W.a_0);return;
case I_SYCD:printf("SYCD !%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_SYCL:printf("SYCL @%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_SYW0:printf("SYW0");return;
case I_SYW1:printf("SYW1");return;
case I_SYW2:printf("SYW2");return;
case I_SYW3:printf("SYW3");return;
case I_SYW4:printf("SYW4");return;
case I_SYW5:printf("SYW5");return;
case I_SYW6:printf("SYW6");return;
case I_SYW7:printf("SYW7");return;
case I_SYW8:printf("SYW8");return;
case I_SYW9:printf("SYW9");return;
case I_SBLW:printf("SBLW");return;
case I_SBRW:printf("SBRW");return;
case I_SYD0:printf("SYD0");return;
case I_SYD1:printf("SYD1");return;
case I_SYD2:printf("SYD2");return;
case I_SYD3:printf("SYD3");return;
case I_SYD4:printf("SYD4");return;
case I_SYD5:printf("SYD5");return;
case I_SYD6:printf("SYD6");return;
case I_SYD7:printf("SYD7");return;
case I_SYD8:printf("SYD8");return;
case I_SYD9:printf("SYD9");return;
case I_SBLD:printf("SBLD");return;
case I_SBRD:printf("SBRD");return;
case I_SYQ0:printf("SYQ0");return;
case I_SYQ1:printf("SYQ1");return;
case I_SYQ2:printf("SYQ2");return;
case I_SYQ3:printf("SYQ3");return;
case I_SYQ4:printf("SYQ4");return;
case I_SYQ5:printf("SYQ5");return;
case I_SYQ6:printf("SYQ6");return;
case I_SYQ7:printf("SYQ7");return;
case I_SYQ8:printf("SYQ8");return;
case I_SYQ9:printf("SYQ9");return;
case I_SBLQ:printf("SBLQ");return;
case I_SBRQ:printf("SBRQ");return;
case I_SCBW:printf("SCBW");return;
case I_SCWD:printf("SCWD");return;
case I_SCDQ:printf("SCDQ");return;
case I_SCQD:printf("SCQD");return;
case I_SCDW:printf("SCDW");return;
case I_SCWB:printf("SCWB");return;
case I_SCDB:printf("SCDB");return;
case I_SCQB:printf("SCQB");return;
case I_SCZD:printf("SCZD");return;
case I_SCZQ:printf("SCZQ");return;
case I_ERR_:return;case I_DEPL:printf("DEPL");return;
case I_PEPH:printf("PEPH");return;
case I_STPI:printf("STPI %%%01X #%04X",(uint16_t)instructionSingle.arg.BW.a_0,(uint16_t)instructionSingle.arg.BW.a_1);return;
case I_LOFF:printf("LOFF !%04X%04X",(uint16_t)(instructionSingle.arg.D.a_0>>16),(uint16_t)instructionSingle.arg.D.a_0);return;
case I_INSR:printf("INSR %%%01X",(uint16_t)instructionSingle.arg.B1.a_0);return;
default:printf("ERRR");return;
}
}






uint32_t numberOfLabels;
uint32_t* labelNumbers;
uint32_t* labelAddresses;

uint32_t mainLabel;
uint32_t mainAddress;
void* binaryLocation;


bool doStringsMatch(const char*const string1,const char*const string2){
	int32_t i=0;
	char c1;
	char c2;
	while ((c1=string1[i])!=0 & (c2=string2[i])!=0){
		i++;
		if (c1!=c2) return false;
	}
	return c1==c2;
}


char* strMerge3(const char*const s0,const char*const s1,const char*const s2){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	const uint32_t l2 = strlen(s2);
	char*const sf = cosmic_malloc(l0+l1+l2+1);
	char* wa = sf;
	memcpy(wa,s0,l0);
	wa+=l0;
	memcpy(wa,s1,l1);
	wa+=l1;
	memcpy(wa,s2,l2);
	wa+=l2;
	*wa=0;
	return sf;
}


struct StringBuilder{
	struct StringBuilder* nextStringBuilder;
	char buffer[256];
	uint16_t nextCharIndex;
};

void stringBuilderAppendChar(struct StringBuilder* stringBuilderIn,char c){
	struct StringBuilder* stringBuilderWork=stringBuilderIn;
	while (stringBuilderWork->nextStringBuilder!=NULL){
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	if (stringBuilderWork->nextCharIndex==256){
		stringBuilderWork->nextStringBuilder=cosmic_calloc(1,sizeof(struct StringBuilder));
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	stringBuilderWork->buffer[stringBuilderWork->nextCharIndex++]=c;
}

char* stringBuilderToString(struct StringBuilder* stringBuilderIn){
	uint32_t length=1;
	struct StringBuilder* stringBuilderWork=stringBuilderIn;
	while (stringBuilderWork->nextStringBuilder!=NULL){
		length+=stringBuilderWork->nextCharIndex;
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	do {
		length+=stringBuilderWork->nextCharIndex;
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	} while (stringBuilderWork!=NULL);
	char* string = cosmic_calloc(length,1);
	stringBuilderWork=stringBuilderIn;
	uint32_t i0=0;
	do {
		for (uint16_t i1=0;i1<stringBuilderWork->nextCharIndex;i1++){
			string[i0++]=stringBuilderWork->buffer[i1];
		}
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	} while (stringBuilderWork!=NULL);
	return string;
}

struct StringBuilder* stringBuilderCreate(){
	return cosmic_calloc(1,sizeof(struct StringBuilder));
}

void stringBuilderDestroy(struct StringBuilder* stringBuilderIn){
	if (stringBuilderIn==NULL) return;
	stringBuilderDestroy(stringBuilderIn->nextStringBuilder);
	cosmic_free(stringBuilderIn);
}

void stringBuilderClear(struct StringBuilder* stringBuilderIn){
	stringBuilderDestroy(stringBuilderIn->nextStringBuilder);
	memset(stringBuilderIn,0,sizeof(struct StringBuilder));
}





static uint32_t getLabelAddress(uint32_t labelToSearch){
	if (labelToSearch==0){
		printf("Label address of 0 is invalid\n");
		exit(1);
	}
	for (uint32_t i=0;i<numberOfLabels;i++){
		if (labelNumbers[i]==labelToSearch) return labelAddresses[i];
	}
	printf("Unresolved Label %04X%04X\n",(unsigned)(labelToSearch>>16),(unsigned)labelToSearch);
	exit(1);
}


static uint16_t getIntrinsicID(enum InstructionTypeID id){
	switch (id){
		case I_D32U:return 0x01;
		case I_R32U:return 0x02;
		case I_D32S:return 0x03;
		case I_R32S:return 0x04;
		case I_D64U:return 0x05;
		case I_R64U:return 0x06;
		case I_D64S:return 0x07;
		case I_R64S:return 0x08;
		case I_LAD4:return 0x09;
		case I_LAD5:return 0x0A;
		case I_LSU4:return 0x0B;
		case I_LSU5:return 0x0C;
		case I_LMU4:return 0x0D;
		case I_LMU5:return 0x0E;
		case I_LDI4:return 0x0F;
		case I_LDI5:return 0x10;
		case I_LLS6:return 0x11;
		case I_LLS7:return 0x12;
		case I_LRS6:return 0x13;
		case I_LRS7:return 0x14;
		default:;
	}
	return 0;
}



#define NUM_STD_SYMBOLS_FUNC 20
#define NUM_STD_SYMBOLS_VAR 3

static const char* stdSymbolsFunc[NUM_STD_SYMBOLS_FUNC] = {
	"malloc",
	"calloc",
	"realloc",
	"free",
	"vsprintf",
	"fprintf",
	"printf",
	"snprintf",
	"fclose",
	"fopen",
	"fsetpos",
	"fgetpos",
	"fgetc",
	"fputc",
	"fflush",
	"abort",
	"exit",
	"strlen",
	"memset",
	"memcpy"
};

static const void* stdAddressesFunc[NUM_STD_SYMBOLS_FUNC] = {
	&malloc,
	&calloc,
	&realloc,
	&free,
	&vsprintf,
	&fprintf,
	&printf,
	&snprintf,
	&fclose,
	&fopen,
	&fsetpos,
	&fgetpos,
	&fgetc,
	&fputc,
	&fflush,
	&abort,
	&exit,
	&strlen,
	&memset,
	&memcpy
};

static uint32_t stdLabelsFunc[NUM_STD_SYMBOLS_FUNC] = {0};

static const char* stdSymbolsVar[NUM_STD_SYMBOLS_VAR] = {
	"stdout",
	"stderr",
	"stdin"
};

static const void* stdAddressesVar[NUM_STD_SYMBOLS_VAR] = {
	&stdout,
	&stderr,
	&stdin
};

static uint32_t stdLabelsVar[NUM_STD_SYMBOLS_VAR] = {0};



static struct {
	uint16_t vs[100]; // [value stack]
	int16_t vsl; // [value stack location]
	int16_t pvsl; // [previous value stack location]
} symbolicStack;


static void symbolicResolutionSingle(const uint8_t* workingByteCode,InstructionSingle* IS_temp){
	decompressInstruction(workingByteCode,IS_temp);
	if (symbolicStack.vsl<10){
		printf("ByteCode corrupted, symbolic stack overflow\n");
		exit(1);
	}
	symbolicStack.pvsl=symbolicStack.vsl;
	bool wasLoad=false;
	uint16_t inSize;
	uint16_t outSize;
	switch (IS_temp->id){
		case I_SYRE:
		case I_SYDE:
		return;
		case I_SYCB:
		symbolicStack.vsl-=1;
		symbolicStack.vs[symbolicStack.vsl  ]=IS_temp->arg.B1.a_0;
		wasLoad=true;
		break;
		case I_SYCW:
		symbolicStack.vsl-=1;
		symbolicStack.vs[symbolicStack.vsl  ]=IS_temp->arg.W.a_0;
		wasLoad=true;
		break;
		case I_SYCD:
		symbolicStack.vsl-=2;
		symbolicStack.vs[symbolicStack.vsl+1]=IS_temp->arg.D.a_0>>16;
		symbolicStack.vs[symbolicStack.vsl+0]=IS_temp->arg.D.a_0>> 0;
		wasLoad=true;
		break;
		case I_SYCL:
		symbolicStack.vsl-=2;
		{
		uint32_t labelAddressesTemp = getLabelAddress(IS_temp->arg.D.a_0);
		symbolicStack.vs[symbolicStack.vsl+1]=labelAddressesTemp>>16;
		symbolicStack.vs[symbolicStack.vsl+0]=labelAddressesTemp>> 0;
		}
		wasLoad=true;
		break;
		case I_SYW0:
		case I_SYW1:
		case I_SYW2:
		case I_SYW3:
		case I_SYW4:
		case I_SYW5:
		case I_SYW6:
		case I_SYW7:
		case I_SYW8:
		case I_SYW9:
		case I_SBLW:
		case I_SBRW:
		case I_SCDW:
		case I_SCDB:
		inSize=2;
		outSize=1;
		break;
		case I_SYD0:
		case I_SYD1:
		case I_SYD2:
		case I_SYD3:
		case I_SYD4:
		case I_SYD5:
		case I_SYD6:
		case I_SYD7:
		case I_SYD8:
		case I_SYD9:
		case I_SCQD:
		inSize=4;
		outSize=2;
		break;
		case I_SYQ0:
		case I_SYQ1:
		case I_SYQ2:
		case I_SYQ3:
		case I_SYQ4:
		case I_SYQ5:
		case I_SYQ6:
		case I_SYQ7:
		case I_SYQ8:
		case I_SYQ9:
		inSize=8;
		outSize=4;
		break;
		case I_SBLD:
		case I_SBRD:
		inSize=3;
		outSize=2;
		break;
		case I_SBLQ:
		case I_SBRQ:
		inSize=5;
		outSize=4;
		break;
		case I_SCWD:
		case I_SCZD:
		inSize=1;
		outSize=2;
		break;
		case I_SCDQ:
		case I_SCZQ:
		inSize=2;
		outSize=4;
		break;
		case I_SCBW:
		case I_SCWB:
		inSize=1;
		outSize=1;
		break;
		case I_SCQB:
		inSize=4;
		outSize=1;
		break;
		default:;assert(false);
	}
	if (!wasLoad){
		symbolicStack.vsl+=inSize;
		symbolicStack.vsl-=outSize;
		if (symbolicStack.pvsl+inSize>100){
			printf("ByteCode corrupted, symbolic stack underflow\n");
			exit(1);
		}
		uint16_t vs_in[8];
		uint32_t val_out; // todo: when updating to allow qword, change to uint64_t
		for (uint16_t i=0;i<inSize;i++){
			vs_in[i]=symbolicStack.vs[symbolicStack.pvsl+i];
		}
		switch (IS_temp->id){
			case I_SYW0:val_out=vs_in[1]+vs_in[0];break;
			case I_SYW1:val_out=vs_in[1]-vs_in[0];break;
			case I_SYW2:val_out=vs_in[1]*vs_in[0];break;
			case I_SYW3:val_out=(  signed)vs_in[1]/(  signed)vs_in[0];break;
			case I_SYW4:val_out=(unsigned)vs_in[1]/(unsigned)vs_in[0];break;
			case I_SYW5:val_out=(  signed)vs_in[1]%(  signed)vs_in[0];break;
			case I_SYW6:val_out=(unsigned)vs_in[1]%(unsigned)vs_in[0];break;
			case I_SYW7:val_out=vs_in[1]^vs_in[0];break;
			case I_SYW8:val_out=vs_in[1]&vs_in[0];break;
			case I_SYW9:val_out=vs_in[1]|vs_in[0];break;
			case I_SBLW:val_out=(unsigned)vs_in[1]<<vs_in[0];break;
			case I_SBRW:val_out=(unsigned)vs_in[1]>>vs_in[0];break;
			case I_SCWB:val_out=vs_in[0]!=0u;break;
			case I_SCDB:val_out=(vs_in[1]|vs_in[0])!=0u;break;
			case I_SCQB:val_out=(vs_in[2]|vs_in[3]|vs_in[0]|vs_in[1])!=0u;break;
			case I_SCZD:case I_SCDW:val_out=vs_in[0];break;
			case I_SCBW:val_out=(((unsigned)vs_in[0]&0xA0u)*0x01FEu)|(unsigned)vs_in[0];break;
			case I_SCWD:val_out=(((uint32_t)vs_in[0]&0xA000u)*(uint32_t)0x0001FFFELU)|(uint32_t)vs_in[0];break;
			case I_SYD0:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))+(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD1:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))-(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD2:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))*(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD3:val_out=((( int32_t)vs_in[2]<<0)|(( int32_t)vs_in[3]<<16))/((( int32_t)vs_in[0]<<0)|(( int32_t)vs_in[1]<<16));break;
			case I_SYD4:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))/(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD5:val_out=((( int32_t)vs_in[2]<<0)|(( int32_t)vs_in[3]<<16))%((( int32_t)vs_in[0]<<0)|(( int32_t)vs_in[1]<<16));break;
			case I_SYD6:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))%(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD7:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))^(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD8:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))&(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD9:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))|(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SCQD:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16));break;
			case I_SBLD:val_out=(((uint32_t)vs_in[1]<<0)|((uint32_t)vs_in[2]<<16))<<vs_in[0];break;
			case I_SBRD:val_out=(((uint32_t)vs_in[1]<<0)|((uint32_t)vs_in[2]<<16))>>vs_in[0];break;
			
			case I_SYQ0:
			case I_SYQ1:
			case I_SYQ2:
			case I_SYQ3:
			case I_SYQ4:
			case I_SYQ5:
			case I_SYQ6:
			case I_SYQ7:
			case I_SYQ8:
			case I_SYQ9:
			case I_SBLQ:
			case I_SBRQ:
			
			case I_SCDQ:
			case I_SCZQ:
			assert(false);// qword not ready yet
			
			default:;assert(false);
		}
		switch (outSize){
			case 1:
			symbolicStack.vs[symbolicStack.vsl  ]=(uint16_t)val_out;
			break;
			case 2:
			symbolicStack.vs[symbolicStack.vsl+1]=val_out>>16;
			symbolicStack.vs[symbolicStack.vsl+0]=val_out>> 0;
			break;
			case 4:assert(false);// qword not ready yet [will have to use insertInstructionAt()]
			
			default:assert(false);
		}
	}
}






void loadFileContentsAsByteCode(const char* filePath){
	printf("0/7\r");
	binaryFileLoadState.binaryFile=fopen(filePath,"rb");
	if (binaryFileLoadState.binaryFile==NULL){
		err_10_1_(strMerge3("Error: Could not load file \"",filePath,"\""));
	}
	binaryFileLoadState.corruptionErrorMessage=strMerge3("Error: Input file \"",filePath,"\" is corrupted");
	{
		mainLabel=0;
		struct StringBuilder* stringBuilder=stringBuilderCreate();
		uint32_t len_symbols;
		len_symbols =(uint32_t)binaryFile_noEOF_fgetc();
		len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<8;
		len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<16;
		len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<24;
		
		for (uint32_t i0=0;i0<len_symbols;i0++){
			uint32_t label;
			label =(uint32_t)binaryFile_noEOF_fgetc();
			label|=(uint32_t)binaryFile_noEOF_fgetc()<<8;
			label|=(uint32_t)binaryFile_noEOF_fgetc()<<16;
			label|=(uint32_t)binaryFile_noEOF_fgetc()<<24;
			char c;
			while (6<=(c=binaryFile_noEOF_fgetc())){
				stringBuilderAppendChar(stringBuilder,c);
			}
			uint8_t type=c;
			char* name=stringBuilderToString(stringBuilder);
			stringBuilderClear(stringBuilder);
			if (doStringsMatch(name,"main")){
				// todo add check for correct type
				mainLabel=label;
			}
			for (uint16_t i1=0;i1<NUM_STD_SYMBOLS_FUNC;i1++){
				if (doStringsMatch(name,stdSymbolsFunc[i1])){
					// todo add check for correct type
					stdLabelsFunc[i1]=label;
					goto SymbolAdvanceToNext;
				}
			}
			for (uint16_t i1=0;i1<NUM_STD_SYMBOLS_VAR;i1++){
				if (doStringsMatch(name,stdSymbolsVar[i1])){
					// todo add check for correct type
					stdLabelsVar[i1]=label;
					goto SymbolAdvanceToNext;
				}
			}
			SymbolAdvanceToNext:;
			cosmic_free(name);
		}
		stringBuilderDestroy(stringBuilder);
		if (mainLabel==0){
			printf("Could not find \'main\' symbol\n");
			exit(1);
		}
	}
	printf("1/7\r");
	{
		fpos_t fileBufferStartPosition;
		if (fgetpos(binaryFileLoadState.binaryFile,&fileBufferStartPosition)){
			err_10_1_(binaryFileLoadState.corruptionErrorMessage);
		}
		numberOfLabels=NUM_STD_SYMBOLS_FUNC+NUM_STD_SYMBOLS_VAR;
		uint32_t rawSize=0;
		uint16_t id_temp;
		static const uint8_t icc_to_delta[] = {[0]=0,[1]=1,[2]=2,[3]=2,[4]=2,[5]=3,[6]=3,[7]=3,[8]=4,[9]=5,[10]=6,[11]=8,[12]=3,[13]=4,[14]=4,[15]=5};
		uint8_t workingByteCode[10];
		uint32_t intrinsicWalk=0;
		while ((id_temp=_intrinsic_c_functions[intrinsicWalk++])!=0){
			workingByteCode[0]=id_temp;
			numberOfLabels+=id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP;
			const uint8_t icc = instructionContentCatagory(id_temp);
			const uint8_t deltaByteCode=icc_to_delta[icc];
			for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
				workingByteCode[iDelta]=_intrinsic_c_functions[intrinsicWalk++];
			}
			InstructionSingle IS;
			decompressInstruction(workingByteCode,&IS);
			rawSize+=backendInstructionSize(IS);
		}
		intrinsicWalk=0;
		bool doubleLoopSwitch=false;
		do {
			while ((id_temp=binaryFile_noEOF_fgetc())!=0){
				workingByteCode[0]=id_temp;
				numberOfLabels+=id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP;
				const uint8_t icc = instructionContentCatagory(id_temp);
				const uint8_t deltaByteCode=icc_to_delta[icc];
				for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
					workingByteCode[iDelta]=binaryFile_noEOF_fgetc();
				}
				InstructionSingle IS;
				decompressInstruction(&workingByteCode,&IS);
				rawSize+=backendInstructionSize(IS);
			}
			if (doubleLoopSwitch) break;
			doubleLoopSwitch=true;
		} while (true);
		doubleLoopSwitch=false;
		uint32_t rawSizeBeforeAlign=rawSize;
		rawSize+=rawSize&1;
		// size and number of labels is now determined
		binaryFile_isEOF_fgetc();
		printf("2/7\r");
		if (fsetpos(binaryFileLoadState.binaryFile,&fileBufferStartPosition)){
			err_10_1_(binaryFileLoadState.corruptionErrorMessage);
		}
		binaryLocation =malloc(rawSize);
		labelNumbers   =malloc(numberOfLabels*sizeof(uint32_t));
		labelAddresses =malloc(numberOfLabels*sizeof(uint32_t));
		if (binaryLocation==NULL | labelNumbers==NULL | labelAddresses==NULL){
			fprintf(stderr,"Could not allocate sufficient space for binary\n");
			exit(1);
		}
		uint32_t rawWalkSize=(uint32_t)binaryLocation;
		uint32_t labelWalkCount=0;
		for (uint16_t i1=0;i1<NUM_STD_SYMBOLS_FUNC;i1++){
			labelNumbers[labelWalkCount]=stdLabelsFunc[i1];
			labelAddresses[labelWalkCount]=(uint32_t)stdAddressesFunc[i1];
			labelWalkCount++;
		}
		for (uint16_t i1=0;i1<NUM_STD_SYMBOLS_VAR;i1++){
			labelNumbers[labelWalkCount]=stdLabelsVar[i1];
			labelAddresses[labelWalkCount]=(uint32_t)stdAddressesVar[i1];
			labelWalkCount++;
		}
		printf("3/7\r");
		while ((id_temp=_intrinsic_c_functions[intrinsicWalk++])!=0){
			workingByteCode[0]=id_temp;
			const uint8_t icc = instructionContentCatagory(id_temp);
			const uint8_t deltaByteCode=icc_to_delta[icc];
			for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
				workingByteCode[iDelta]=_intrinsic_c_functions[intrinsicWalk++];
			}
			InstructionSingle IS;
			decompressInstruction(&workingByteCode,&IS);
			rawWalkSize+=backendInstructionSize(IS);
			if (id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP){
				labelAddresses[labelWalkCount]=rawWalkSize;
				if (id_temp==I_LABL){
					labelNumbers[labelWalkCount++]=IS.arg.D.a_0;
				} else if (id_temp==I_FCST){
					labelNumbers[labelWalkCount++]=IS.arg.BWD.a_2;
				} else {
					labelNumbers[labelWalkCount++]=IS.arg.BBD.a_2;
				}
			}
		}
		intrinsicWalk=0;
		
		doubleLoopSwitch=false;
		do {
			while ((id_temp=binaryFile_noEOF_fgetc())!=0){
				workingByteCode[0]=id_temp;
				const uint8_t icc = instructionContentCatagory(id_temp);
				const uint8_t deltaByteCode=icc_to_delta[icc];
				for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
					workingByteCode[iDelta]=binaryFile_noEOF_fgetc();
				}
				InstructionSingle IS;
				decompressInstruction(&workingByteCode,&IS);
				rawWalkSize+=backendInstructionSize(IS);
				if (id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP){
					labelAddresses[labelWalkCount]=rawWalkSize;
					if (id_temp==I_LABL){
						labelNumbers[labelWalkCount++]=IS.arg.D.a_0;
					} else if (id_temp==I_FCST){
						labelNumbers[labelWalkCount++]=IS.arg.BWD.a_2;
					} else {
						labelNumbers[labelWalkCount++]=IS.arg.BBD.a_2;
					}
				}
			}
			if (doubleLoopSwitch) break;
			doubleLoopSwitch=true;
		} while (true);
		doubleLoopSwitch=false;
		// label addresses now determined
		printf("4/7\r");
		if (fsetpos(binaryFileLoadState.binaryFile,&fileBufferStartPosition)){
			err_10_1_(binaryFileLoadState.corruptionErrorMessage);
		}
		mainAddress=getLabelAddress(mainLabel);
		
		uint8_t* binaryLocationWalk=binaryLocation;
		uint16_t func_stack_size;
		uint8_t func_stack_initial;
		while ((id_temp=_intrinsic_c_functions[intrinsicWalk++])!=0){
			workingByteCode[0]=id_temp;
			uint8_t icc = instructionContentCatagory(id_temp);
			uint8_t deltaByteCode=icc_to_delta[icc];
			for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
				workingByteCode[iDelta]=_intrinsic_c_functions[intrinsicWalk++];
			}
			InstructionSingle IS;
			decompressInstruction(&workingByteCode,&IS);
			
			uint16_t intrinsicID=getIntrinsicID(IS.id);
			uint32_t symVal;
			if (intrinsicID!=0){
				symVal=getLabelAddress(intrinsicID);
			} else {
				uint8_t symbolSizeBytes=0;
				uint8_t symbolSizeWords=0;
				switch (IS.id){
					case I_FCST:
					func_stack_initial=IS.arg.BWD.a_0;
					func_stack_size=IS.arg.BWD.a_1;
					break;
					case I_JTEN:
					symVal=getLabelAddress(IS.arg.D.a_0);
					break;
					case I_SYRD:
					case I_SYDD:
					symbolSizeBytes=4;
					symbolSizeWords=2;
					break;
					case I_SYDB:
					case I_SYRB:
					symbolSizeBytes=1;
					symbolSizeWords=1;
					break;
					case I_SYRW:
					case I_SYDW:
					symbolSizeBytes=2;
					symbolSizeWords=1;
					break;
					case I_SYRQ:
					case I_SYDQ:
					symbolSizeBytes=8;
					symbolSizeWords=4;
					break;
					default:;
				}
				if (symbolSizeWords!=0){
					symbolicStack.vsl=100;
					InstructionSingle IS_temp;
					do {
						workingByteCode[0]=_intrinsic_c_functions[intrinsicWalk++];
						if (workingByteCode[0]==0){
							printf("ByteCode corrupted, end inside symbolic\n");
							exit(1);
						}
						icc = instructionContentCatagory(workingByteCode[0]);
						deltaByteCode=icc_to_delta[icc];
						for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
							workingByteCode[iDelta]=_intrinsic_c_functions[intrinsicWalk++];
						}
						symbolicResolutionSingle(&workingByteCode,&IS_temp);
					} while (IS_temp.id!=I_SYRE & IS_temp.id!=I_SYDE);
					if (100-symbolicStack.vsl!=symbolSizeWords){
						printf("ByteCode corrupted, symbolic result size mismatch\n");
						exit(1);
					}
					switch (symbolSizeBytes){
						case 1:symVal=(uint32_t)(symbolicStack.vs[99]&255);break;
						case 2:symVal=(uint32_t)symbolicStack.vs[99];break;
						case 4:symVal=((uint32_t)symbolicStack.vs[99]<<16)|symbolicStack.vs[98];break;
						//case 8:break;
						default:assert(false);
					}
				}
			}
			backendInstructionWrite(&binaryLocationWalk,symVal,func_stack_size,func_stack_initial,IS);
		}
		intrinsicWalk=0;
		
		printf("5/7\r");
		doubleLoopSwitch=false;
		do {
			while ((id_temp=binaryFile_noEOF_fgetc())!=0){
			workingByteCode[0]=id_temp;
			uint8_t icc = instructionContentCatagory(id_temp);
			uint8_t deltaByteCode=icc_to_delta[icc];
			for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
				workingByteCode[iDelta]=binaryFile_noEOF_fgetc();
			}
			InstructionSingle IS;
			decompressInstruction(&workingByteCode,&IS);
			
			uint16_t intrinsicID=getIntrinsicID(IS.id);
			uint32_t symVal;
			if (intrinsicID!=0){
				symVal=getLabelAddress(intrinsicID);
			} else {
				uint8_t symbolSizeBytes=0;
				uint8_t symbolSizeWords=0;
				switch (IS.id){
					case I_FCST:
					func_stack_initial=IS.arg.BWD.a_0;
					func_stack_size=IS.arg.BWD.a_1;
					break;
					case I_JTEN:
					symVal=getLabelAddress(IS.arg.D.a_0);
					break;
					case I_SYRD:
					case I_SYDD:
					symbolSizeBytes=4;
					symbolSizeWords=2;
					break;
					case I_SYDB:
					case I_SYRB:
					symbolSizeBytes=1;
					symbolSizeWords=1;
					break;
					case I_SYRW:
					case I_SYDW:
					symbolSizeBytes=2;
					symbolSizeWords=1;
					break;
					case I_SYRQ:
					case I_SYDQ:
					symbolSizeBytes=8;
					symbolSizeWords=4;
					break;
					default:;
				}
				if (symbolSizeWords!=0){
					symbolicStack.vsl=100;
					InstructionSingle IS_temp;
					do {
						workingByteCode[0]=binaryFile_noEOF_fgetc();
						if (workingByteCode[0]==0){
							printf("ByteCode corrupted, end inside symbolic\n");
							exit(1);
						}
						icc = instructionContentCatagory(workingByteCode[0]);
						deltaByteCode=icc_to_delta[icc];
						for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
							workingByteCode[iDelta]=binaryFile_noEOF_fgetc();
						}
						symbolicResolutionSingle(&workingByteCode,&IS_temp);
					} while (IS_temp.id!=I_SYRE & IS_temp.id!=I_SYDE);
					if (100-symbolicStack.vsl!=symbolSizeWords){
						printf("ByteCode corrupted, symbolic result size mismatch\n");
						exit(1);
					}
					switch (symbolSizeBytes){
						case 1:symVal=(uint32_t)(symbolicStack.vs[99]&255);break;
						case 2:symVal=(uint32_t)symbolicStack.vs[99];break;
						case 4:symVal=((uint32_t)symbolicStack.vs[99]<<16)|symbolicStack.vs[98];break;
						//case 8:break;
						default:assert(false);
					}
				}
			}
			backendInstructionWrite(&binaryLocationWalk,symVal,func_stack_size,func_stack_initial,IS);
			}
			if (doubleLoopSwitch) break;
			doubleLoopSwitch=true;
			printf("6/7\r");
		} while (true);
		doubleLoopSwitch=false;
		assert(binaryLocationWalk==(uint8_t*)binaryLocation+rawSizeBeforeAlign); // otherwise, the size that was initially calculated was wrong
		free(labelNumbers);
		free(labelAddresses);
		labelNumbers=NULL;
		labelAddresses=NULL;
	}
	cosmic_free((char*)binaryFileLoadState.corruptionErrorMessage);
	fclose(binaryFileLoadState.binaryFile);
	binaryFileLoadState.corruptionErrorMessage=NULL;
	binaryFileLoadState.binaryFile=NULL;
	printf("Load Successful\n");
}



static void _exec_springboard1(){
	// this is a very neat trick.
	// after _exec_springboard returns, the loaded binary's main is run instead of going back to the loader's _exec_springboard0
	__FUNCTION_RET_INSTRUCTION_ADDRESS=mainAddress;
}

static int _exec_springboard0(int argc, char** argv){
	*((int*)__FUNCTION_RET_VALUE_PTR)=0;
	_exit_ret_address=__FUNCTION_RET_INSTRUCTION_ADDRESS;
	_exit_ret_val_ptr=__FUNCTION_RET_VALUE_PTR;
	_exec_springboard1();
}


int main(int argc, char** argv){
	*((int*)__FUNCTION_RET_VALUE_PTR)=0;
	_exit_ret_address=__FUNCTION_RET_INSTRUCTION_ADDRESS;
	_exit_ret_val_ptr=__FUNCTION_RET_VALUE_PTR;
	if (argc==0) return 0xFFFF;
	loadFileContentsAsByteCode(argv[1]); // should I have this be [0] or [1] ? if I choose [1], then after this expression do "++argv,--argc"
	int ret = _exec_springboard0(argc-1,argv+1);
	free(binaryLocation);
	return ret;
}






