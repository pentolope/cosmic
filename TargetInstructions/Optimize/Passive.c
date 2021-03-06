
typedef struct InstructionInformation{
	enum InstructionTypeID id;
	uint32_t cv; // the constant value associated with RL2_,RL1_,BL1_,STPA,STPS,STWV,STWN,STRV,STRN,ALCR,STOF
	uint8_t regIN[9]; // no more then 8 inputs are listed
	uint8_t regOUT[5]; // no more then 4 outputs are listed
	
	bool usesReg_D; // these are for if it uses that register in a special way that cannot be renamed
	bool usesReg_E;
	
	bool noRename; // if true, all registers for regIN and regOUT cannot be renamed
	bool isSymbolicInternal;
	
	bool isMemoryAccess;
	bool isMemoryAccessVolatile;
	
	bool doesMoveStack; // if true, this instruction may/will move the stack pointer
	bool doesRelyOnStack; // if true, this instruction requires the stack pointer to not change
	
	bool doesDestroyReg; // when true, registers not in regOUT have indetermine value when this instruction finishes
	bool doesBlockReorder; // when true, this instruction will always prevent instructions from being reordered on the other side of this instruction 
	// note that there are several cases that will block a reorder even if doesBlockReorder==false
	
	bool requireRegValuePhi; // true for LABL
	bool stopLinearRegTrace; // true for LABL,AJMP,JJMP,JTEN,JEND
	
	// these 2 below are for peephole opt
	bool inputTrivialSwapable; // true for AND_,OR__,XOR_,ADDN,MULS
	bool isAllowedInVTE;
} InstructionInformation;

#ifdef OPT_DEBUG_SANITY
bool doingSanityCheck = false;
#endif




#if 0

#include <windows.h>



/*
0=default
1=default
2=yellow
3=green
4=default
*/

const char* consolePrintBufferGeneralMessage="";
void printSingleInstructionOptCode(const InstructionSingle);
void consolePrintBufferPoint(const InstructionBuffer*const ib, int32_t index0, int32_t index1, uint8_t mode,const char* message){
	static bool isFirst=true;
	static char printf_buffer[120000];
	if (isFirst){
		setvbuf(stdout,printf_buffer,_IOFBF,120000);
		isFirst=false;
	}
	//const int32_t nLines=250;
	const int32_t nLines=123;
	//const int32_t nLines=79;
	const char* resetLineString="\r                                                            \r";
	if (ib->numberOfSlotsTaken==0){
		return;
	}
	if (ib->buffer[0].id==I_LOFF | ib->buffer[0].id==I_NSNB){
		return;
	}
	if (!(((index0!=-1 & index0<nLines) | (index1!=-1 & index1<nLines)) | (index0==-1 & index1==-1))){
		/*
		resetColor();
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("\x1b[1A");
		printf("SKIPPING - indexes not in range\n");
		resetColor();
		*/
		return;
	}
	for (int32_t l=0;l<nLines+2;l++){
		printf("\x1b[1A");
	}
	resetColor();
	makeColor(COLOR_TO_BACKGROUND,COLOR_BLUE);
	makeColor(COLOR_TO_TEXT,COLOR_GREEN);
	printf("%s  %s\n%s  %s\n",resetLineString,consolePrintBufferGeneralMessage,resetLineString,message);
	resetColor();
	switch (mode){
		case 1:break;
		case 2:makeColor(COLOR_TO_TEXT,COLOR_YELLOW);break;
		case 3:makeColor(COLOR_TO_TEXT,COLOR_GREEN);break;
		case 4:break;
	}
	for (int32_t a=0;a<ib->numberOfSlotsTaken & a<nLines;a++){
		printf("%s",resetLineString);
		if (a==index0){
			if (mode==4) makeColor(COLOR_TO_TEXT,COLOR_GREEN);
			else makeColor(COLOR_TO_TEXT,COLOR_CYAN);
		}
		if (a==index1 & index0!=-1){
			makeColor(COLOR_TO_TEXT,COLOR_MAGENTA);
		}
		if (a==index1 & index0==-1){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
		}
		printf("%c",(a==index0 | a==index1)?'>':' ');
		printSingleInstructionOptCode(ib->buffer[a]);
		if (a==index0 | a==index1){
			resetColor();
			switch (mode){
				case 1:break;
				case 2:makeColor(COLOR_TO_TEXT,COLOR_YELLOW);break;
				case 3:makeColor(COLOR_TO_TEXT,COLOR_GREEN);break;
				case 4:break;
			}
		}
		printf("\n");
	}
	resetColor();
	for (int32_t a=ib->numberOfSlotsTaken;a<nLines;a++){
		printf("%s\n",resetLineString);
	}
	fflush(stdout);
	if ((index0==-1 || ib->buffer[index0].id==I_NOP_) & (index1==-1 || ib->buffer[index1].id==I_NOP_) & (index0!=-1 | index1!=-1)){
		Sleep(10);
	} else {
		Sleep(mode*20+20);
		Sleep(20);
	}
	if (mode==3) Sleep(100);
}
void consolePrintBufferGeneralMessageSet(const char* message){
	consolePrintBufferGeneralMessage=message;
}




#else

#define consolePrintBufferPoint(ib,index0,index1,mode,message) ((void)0)
#define consolePrintBufferGeneralMessageSet(message) ((void)0)

#endif




const InstructionInformation resetII = {.regIN={16,16,16,16,16,16,16,16,16},.regOUT={16,16,16,16,16}};


void fillInstructionInformation(InstructionInformation*const II, const InstructionBuffer*const ib, uint32_t index){
	consolePrintBufferPoint(ib,index,-1,1,"Detailed Information Calculated for this Instruction");
	
	const InstructionSingle*const buffer = ib->buffer;
	*II=resetII;
	II->id=buffer[index].id;
	const InstructionSingle IS=buffer[index];
	switch (IS.id){
		case I_SYRE:
		case I_SYCB:
		case I_SYCW:
		case I_SYCD:
		case I_SYCL:
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
		case I_SBLD:
		case I_SBRD:
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
		case I_SCBW:
		case I_SCWD:
		case I_SCZD:
		case I_SCZQ:
		case I_SCDQ:
		case I_SCQD:
		case I_SCDW:
		case I_SCWB:
		case I_SCDB:
		case I_SCQB:
		II->isSymbolicInternal=true;
		{
			enum InstructionTypeID id;
			do {
				assert(index!=0);
				id=buffer[--index].id;
			} while (id!=I_SYRB & id!=I_SYRW & id!=I_SYRD & id!=I_SYRQ);
			const InstructionSingle IS_2 = buffer[index];
			switch ((uint8_t)id){ // cast is to avoid warnings for missing enumeration values
				case I_SYRB:
				case I_SYRW:
				II->regOUT[0]=IS_2.arg.B1.a_0;
				break;
				case I_SYRD:
				II->regOUT[0]=IS_2.arg.B2.a_0;
				II->regOUT[1]=IS_2.arg.B2.a_1;
				break;
				case I_SYRQ:
				II->regOUT[0]=IS_2.arg.B4.a_0;
				II->regOUT[1]=IS_2.arg.B4.a_1;
				II->regOUT[2]=IS_2.arg.B4.a_2;
				II->regOUT[3]=IS_2.arg.B4.a_3;
				break;
				// there are no other possibilities
			}
		}
		break;
		case I_PHIS:
		II->regIN[0]=IS.arg.B1.a_0;
		II->doesBlockReorder=true;
		II->noRename=true;
		break;
		case I_PHIE:
		II->regOUT[0]=IS.arg.B1.a_0;
		II->doesBlockReorder=true;
		II->noRename=true;
		break;
		case I_JJMP:
		II->regIN[0]=IS.arg.BBD.a_0;
		II->regIN[1]=IS.arg.BBD.a_1;
		case I_JTEN:
		case I_JEND:
		II->doesBlockReorder=true;
		II->stopLinearRegTrace=true;
		case I_NOP_:
		break;
		case I_FCST:
		case I_FCEN:
		II->doesBlockReorder=true;
		II->doesDestroyReg=true;
		break;
		case I_PU1_:
		case I_PUA1:
		II->doesMoveStack=true;
		II->doesRelyOnStack=true;
		II->regIN[0]=IS.arg.B1.a_0;
		break;
		case I_PU2_:
		case I_PUA2:
		II->doesMoveStack=true;
		II->doesRelyOnStack=true;
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		break;
		case I_POP1:
		II->doesMoveStack=true;
		II->doesRelyOnStack=true;
		II->regOUT[0]=IS.arg.B1.a_0;
		break;
		case I_POP2:
		II->doesMoveStack=true;
		II->doesRelyOnStack=true;
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regOUT[1]=IS.arg.B2.a_1;
		break;
		case I_BL1_:
		II->cv=IS.arg.B2.a_1;
		II->regOUT[0]=IS.arg.B2.a_0;
		II->isAllowedInVTE=true;
		break;
		case I_RL1_:
		II->cv=IS.arg.BW.a_1;
		II->regOUT[0]=IS.arg.BW.a_0;
		II->isAllowedInVTE=true;
		break;
		case I_RL2_:
		II->cv=IS.arg.BBD.a_2;
		II->regOUT[0]=IS.arg.BBD.a_0;
		II->regOUT[1]=IS.arg.BBD.a_1;
		break;
		case I_SYRB:
		case I_SYRW:
		II->regOUT[0]=IS.arg.B1.a_0;
		break;
		case I_SYRD:
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regOUT[1]=IS.arg.B2.a_1;
		break;
		case I_SYRQ:
		II->regOUT[0]=IS.arg.B4.a_0;
		II->regOUT[1]=IS.arg.B4.a_1;
		II->regOUT[2]=IS.arg.B4.a_2;
		II->regOUT[3]=IS.arg.B4.a_3;
		break;
		case I_CALL:
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		II->doesRelyOnStack=true;
		case I_RET_:
		II->doesMoveStack=true;
		II->doesDestroyReg=true;
		II->doesBlockReorder=true;
		break;
		case I_LABL:
		II->stopLinearRegTrace=true;
		II->doesBlockReorder=true;
		II->requireRegValuePhi=true;
		break;
		case I_STWV:
		II->isMemoryAccessVolatile=true;
		case I_STWN:
		II->isMemoryAccess=true;
		II->regIN[0]=IS.arg.B2.a_0;
		II->cv=IS.arg.B2.a_1;
		break;
		case I_STRV:
		II->isMemoryAccessVolatile=true;
		case I_STRN:
		II->isMemoryAccess=true;
		II->regOUT[0]=IS.arg.B2.a_0;
		II->cv=IS.arg.B2.a_1;
		break;
		case I_ALOC:
		II->doesMoveStack=true;
		II->doesRelyOnStack=true;
		II->doesBlockReorder=true;
		break;
		case I_ALCR:
		II->doesMoveStack=true;
		case I_STOF:
		II->doesRelyOnStack=true;
		case I_STPA:
		case I_STPS:
		II->cv=IS.arg.BW.a_1;
		II->regOUT[0]=IS.arg.BW.a_0;
		break;
		case I_AJMP:
		II->doesBlockReorder=true;
		II->stopLinearRegTrace=true;
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		break;
		case I_CJMP:
		II->doesBlockReorder=true;
		II->regIN[0]=IS.arg.B3.a_0;
		II->regIN[1]=IS.arg.B3.a_1;
		II->regIN[2]=IS.arg.B3.a_2;
		break;
		case I_MOV_:
		case I_SHFT:
		case I_BSWP:
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regIN[0]=IS.arg.B2.a_1;
		II->isAllowedInVTE=true;
		break;
		case I_AND_:
		case I_OR__:
		case I_XOR_:
		case I_ADDN:
		II->inputTrivialSwapable=true;
		case I_SUBN:
		case I_SUBC:
		II->regOUT[0]=IS.arg.B3.a_0;
		II->regIN[0]=IS.arg.B3.a_1;
		II->regIN[1]=IS.arg.B3.a_2;
		II->isAllowedInVTE=true;
		break;
		case I_SSUB:
		II->regOUT[0]=IS.arg.B3.a_0;
		II->regOUT[1]=IS.arg.B3.a_1;
		II->regIN[0]=IS.arg.B3.a_0;
		II->regIN[1]=IS.arg.B3.a_1;
		II->regIN[2]=IS.arg.B3.a_2;
		II->isAllowedInVTE=true;
		break;
		case I_DIVM:
		II->regOUT[1]=IS.arg.B2.a_1;
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		II->isAllowedInVTE=true;
		break;
		case I_MULS:
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		II->isAllowedInVTE=true;
		II->inputTrivialSwapable=true;
		break;
		case I_MULL:
		II->usesReg_D=true;
		II->usesReg_E=true;
		II->regIN[0]=IS.arg.B2.a_0;
		II->regIN[1]=IS.arg.B2.a_1;
		II->regIN[2]=13;
		II->regIN[3]=14;
		II->regOUT[0]=13;
		II->regOUT[1]=14;
		II->isAllowedInVTE=true;
		break;
		case I_MWWV:
		II->isMemoryAccessVolatile=true;
		case I_MWWN:
		II->isMemoryAccess=true;
		II->regIN[0]=IS.arg.B3.a_1;
		II->regIN[1]=IS.arg.B3.a_2;
		II->regIN[2]=IS.arg.B3.a_0;
		break;
		case I_MRWV:
		II->isMemoryAccessVolatile=true;
		case I_MRWN:
		II->isMemoryAccess=true;
		II->regOUT[0]=IS.arg.B3.a_0;
		II->regIN[0]=IS.arg.B3.a_1;
		II->regIN[1]=IS.arg.B3.a_2;
		break;
		case I_MWBV:
		II->isMemoryAccessVolatile=true;
		case I_MWBN:
		II->isMemoryAccess=true;
		II->usesReg_D=true;
		II->regIN[0]=13;
		II->regIN[1]=IS.arg.B2.a_1;
		II->regIN[2]=IS.arg.B2.a_0;
		break;
		case I_MRBV:
		II->isMemoryAccessVolatile=true;
		case I_MRBN:
		II->isMemoryAccess=true;
		II->usesReg_D=true;
		II->regOUT[0]=IS.arg.B2.a_0;
		II->regIN[0]=13;
		II->regIN[1]=IS.arg.B2.a_1;
		break;
		case I_LSU0:
		II->regOUT[0]=IS.arg.B4.a_0;
		II->regOUT[1]=IS.arg.B4.a_1;
		II->regIN[0]=IS.arg.B4.a_0;
		II->regIN[1]=IS.arg.B4.a_1;
		II->regIN[2]=IS.arg.B4.a_2;
		II->regIN[3]=IS.arg.B4.a_3;
		II->isAllowedInVTE=true;
		break;
		case I_LAD2:
		II->regOUT[0]=IS.arg.B4.a_0;
		II->regOUT[1]=IS.arg.B4.a_1;
		II->regIN[0]=IS.arg.B4.a_2;
		II->regIN[1]=IS.arg.B4.a_3;
		II->isAllowedInVTE=true;
		break;
		case I_LAD1:
		II->regOUT[0]=IS.arg.B5.a_0;
		II->regOUT[1]=IS.arg.B5.a_1;
		II->regIN[0]=IS.arg.B5.a_2;
		II->regIN[1]=IS.arg.B5.a_3;
		II->regIN[2]=IS.arg.B5.a_4;
		II->isAllowedInVTE=true;
		break;
		case I_LAD0:
		II->regOUT[0]=IS.arg.B6.a_0;
		II->regOUT[1]=IS.arg.B6.a_1;
		II->regIN[0]=IS.arg.B6.a_2;
		II->regIN[1]=IS.arg.B6.a_3;
		II->regIN[2]=IS.arg.B6.a_4;
		II->regIN[3]=IS.arg.B6.a_5;
		II->isAllowedInVTE=true;
		break;
		case I_LAD3:
		case I_LSU3:
		case I_LMU3:
		II->regOUT[0]=IS.arg.B8.a_0;
		II->regOUT[1]=IS.arg.B8.a_1;
		II->regOUT[2]=IS.arg.B8.a_2;
		II->regOUT[3]=IS.arg.B8.a_3;
		II->regIN[0]=IS.arg.B8.a_0;
		II->regIN[1]=IS.arg.B8.a_1;
		II->regIN[2]=IS.arg.B8.a_2;
		II->regIN[3]=IS.arg.B8.a_3;
		II->regIN[4]=IS.arg.B8.a_4;
		II->regIN[5]=IS.arg.B8.a_5;
		II->regIN[6]=IS.arg.B8.a_6;
		II->regIN[7]=IS.arg.B8.a_7;
		break;
		case I_LLS6:
		case I_LRS6:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=6;
		II->regIN[1]=7;
		II->regIN[2]=10;
		II->regOUT[0]=2;
		II->regOUT[1]=3;
		break;
		case I_LLS7:
		case I_LRS7:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=6;
		II->regIN[1]=7;
		II->regIN[2]=8;
		II->regIN[3]=9;
		II->regIN[4]=10;
		II->regOUT[0]=2;
		II->regOUT[1]=3;
		II->regOUT[2]=4;
		II->regOUT[3]=5;
		break;
		case I_D32U:
		case I_D32S:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=2;
		II->regIN[1]=3;
		II->regIN[2]=4;
		II->regIN[3]=5;
		II->regOUT[0]=8;
		II->regOUT[1]=9;
		break;
		case I_R32U:
		case I_R32S:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=2;
		II->regIN[1]=3;
		II->regIN[2]=4;
		II->regIN[3]=5;
		II->regOUT[0]=6;
		II->regOUT[1]=7;
		break;
		case I_LAD4:
		case I_LSU4:
		case I_LMU4:
		case I_LDI4:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=4;
		II->regIN[1]=5;
		II->regIN[2]=6;
		II->regIN[3]=7;
		II->regOUT[0]=2;
		II->regOUT[1]=3;
		break;
		case I_D64U:
		case I_D64S:
		case I_R64U:
		case I_R64S:
		case I_LAD5:
		case I_LSU5:
		case I_LMU5:
		case I_LDI5:
		II->noRename=true;
		II->doesDestroyReg=true;
		II->regIN[0]=6;
		II->regIN[1]=7;
		II->regIN[2]=8;
		II->regIN[3]=9;
		II->regIN[4]=10;
		II->regIN[5]=11;
		II->regIN[6]=12;
		II->regIN[7]=13;
		II->regOUT[0]=2;
		II->regOUT[1]=3;
		II->regOUT[2]=4;
		II->regOUT[3]=5;
		break;
		case I_ERR_:
		case I_INSR:
		case I_DEPL:
		default:
		printf("Internal Error: fillInstructionInformation() got bad opcode\n");
		exit(1);
	}
}




// does not care about LABL,JJMP,FCST, instead it only looks for things targeting the label (such as SYCL,JTEN)
bool doesLabelHaveUsage(const InstructionBuffer* ib,const uint32_t labelNumber){
	InstructionSingle* ISP;
	InstructionSingle* buffer = ib->buffer;
	uint32_t i = ib->numberOfSlotsTaken;
	while (i--!=0){
		ISP = buffer+i;
		if (ISP->id==I_SYCL | ISP->id==I_JTEN){
			if (ISP->arg.D.a_0==labelNumber) return true;
		}
	}
	return false;
}



// for BL1_,RL1_,RL2_,STPA . It does not check registers, only data
bool isDataInInstructionIdentical(const InstructionSingle* IS_0,const InstructionSingle* IS_1){
	enum InstructionTypeID id;
	if ((id=IS_0->id)!=IS_1->id) return false;
	if (id==I_BL1_){
		return IS_0->arg.B2.a_1==IS_1->arg.B2.a_1;
	} else if (id==I_RL1_){
		return IS_0->arg.BW.a_1==IS_1->arg.BW.a_1;
	} else if (id==I_RL2_){
		return IS_0->arg.BBD.a_2==IS_1->arg.BBD.a_2;
	} else if (id==I_STPA){
		return IS_0->arg.BW.a_1==IS_1->arg.BW.a_1;
	}
	return false;
}


uint16_t findLengthOfSymbolicCalc(const InstructionBuffer* ib,const uint32_t target){
	{
	enum InstructionTypeID id;
	assert((id=ib->buffer[target].id,id==I_SYRB|id==I_SYRW|id==I_SYRD|id==I_SYRQ));
	}
	uint16_t length=0;
	InstructionInformation II;
	do {
		fillInstructionInformation(&II,ib,target+ ++length);
	} while (II.isSymbolicInternal);
	return length;
}

static inline bool doesRegListContain(const uint8_t* regList,const uint8_t item){
	uint8_t r;
	while ((r=*(regList++))!=16){
		if (r==item) return true;
	}
	return false;
}

static inline bool doRegListsHaveCommon(const uint8_t* regList0,const uint8_t* regList1){
	if (regList0[0]==16 | regList1[0]==16) return false;
	if (regList0[1]==16 & regList1[1]==16) return regList0[0]==regList1[0];
	uint8_t itemInRegList0;
	while ((itemInRegList0=*(regList0++))!=16){
		if (doesRegListContain(regList1,itemInRegList0)) return true;
	}
	return false;
}

// the return value indicates that the target instruction can be reordered to any position down to and including the index returned
uint32_t findLowerReorderBoundry(const InstructionBuffer* ib,const uint32_t target){
	InstructionInformation targetII;
	InstructionInformation prevII;
	fillInstructionInformation(&targetII,ib,target);
	if (targetII.isSymbolicInternal){
		printf("Internal Warning: findLowerReorderBoundry() was told to reorder a symbolic internal, which shouldn't happen\n");
		return target;
	}
	if (targetII.doesBlockReorder | targetII.doesDestroyReg | target==0) return target;
	bool doesTargetHaveOutput = targetII.regOUT[0]!=16;
	uint32_t bound=target;
	uint32_t nextBound;
	do {
		nextBound=bound-1;
		fillInstructionInformation(&prevII,ib,nextBound);
		if ((
		prevII.doesBlockReorder |
		(prevII.doesDestroyReg & doesTargetHaveOutput) |
		(prevII.isMemoryAccess & targetII.isMemoryAccess) |
		(targetII.doesRelyOnStack & prevII.doesMoveStack) |
		(targetII.doesMoveStack & prevII.doesRelyOnStack)
		) || (
		doRegListsHaveCommon(targetII.regIN,prevII.regOUT) |
		doRegListsHaveCommon(targetII.regOUT,prevII.regIN) |
		doRegListsHaveCommon(targetII.regOUT,prevII.regOUT))
		){
			return bound;
		}
		bound=nextBound;
	} while (nextBound!=0);
	return 0;
}

// the return value indicates that the target instruction can be reordered to any position up to and including the index returned
uint32_t findUpperReorderBoundry(const InstructionBuffer* ib,const uint32_t target){
	uint32_t bound=target;
	InstructionInformation targetII;
	fillInstructionInformation(&targetII,ib,target);
	if (targetII.isSymbolicInternal){
		printf("Internal Warning: findUpperReorderBoundry() was told to reorder a symbolic internal, which shouldn't happen\n");
		return target;
	}
	uint32_t lastInstruction = ib->numberOfSlotsTaken-1;
	if (targetII.doesBlockReorder | targetII.doesDestroyReg | target==lastInstruction) return target;
	bool doesTargetHaveInput = targetII.regIN[0]!=16;
	bool isTargetSymbolicStarter = targetII.id==I_SYRB | targetII.id==I_SYRW | targetII.id==I_SYRD | targetII.id==I_SYRQ;
	{
	InstructionInformation nextII;
	uint32_t nextBound;
	do {
		nextBound=bound+1;
		fillInstructionInformation(&nextII,ib,nextBound);
		if (
		!nextII.isSymbolicInternal & ((
		nextII.doesBlockReorder |
		(nextII.doesDestroyReg & doesTargetHaveInput) |
		(nextII.isMemoryAccess & targetII.isMemoryAccess) |
		((nextII.doesRelyOnStack | targetII.doesRelyOnStack) & 
		(nextII.doesMoveStack | targetII.doesMoveStack))
		) || (
		doRegListsHaveCommon(targetII.regIN,nextII.regOUT) |
		doRegListsHaveCommon(targetII.regOUT,nextII.regIN) |
		doRegListsHaveCommon(targetII.regOUT,nextII.regOUT))
		)){
			break;
		}
		bound=nextBound;
	} while (nextBound!=lastInstruction);
	}
	{
	InstructionInformation symPrev;
	if (isTargetSymbolicStarter){
		while (true){
			fillInstructionInformation(&symPrev,ib,bound);
			if (bound==target | (!symPrev.isSymbolicInternal & symPrev.id!=I_SYRB & symPrev.id!=I_SYRW & symPrev.id!=I_SYRD & symPrev.id!=I_SYRQ)) return bound;
			bound--;
		}
	}
	return bound;
	}
}




/*
returns true if it succeeded in getting the boundry
the lower and upper bounds are inclusive
the startInstruction should have the targetRegister in it's output and not it's input
*/
bool findRegRenameBoundaryFromOrigin(const InstructionBuffer* ib, uint32_t startInstruction, uint8_t targetRegister, uint32_t* upperBound){
	InstructionInformation II;
	fillInstructionInformation(&II,ib,startInstruction);
	assert(doesRegListContain(II.regOUT,targetRegister) & !doesRegListContain(II.regIN,targetRegister)); // ensure called on origin
	const bool checkReg_D=targetRegister==13;
	const bool checkReg_E=targetRegister==14;
	if ((checkReg_D & II.usesReg_D) | (checkReg_E & II.usesReg_E) | II.noRename){
		// the startInstruction needs to be checked for this
		return false;
	}
	const uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t lastMentioned=startInstruction;
	uint32_t i=startInstruction;
	while (++i!=numberOfSlotsTaken){
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		const bool containedRegIN = doesRegListContain(II.regIN,targetRegister);
		if ((checkReg_D & II.usesReg_D)|(checkReg_E & II.usesReg_E)|(containedRegIN & II.noRename)){
			return false; // cannot rename
		}
		const bool containedRegOUT = doesRegListContain(II.regOUT,targetRegister);
		if (II.doesDestroyReg | II.stopLinearRegTrace){
			if (!II.noRename & (containedRegOUT | containedRegIN)){
				*upperBound=i;
				return true;
			} else {
				*upperBound=lastMentioned;
				return true;
			}
		}
		if (containedRegOUT & !containedRegIN){
			*upperBound=lastMentioned;
			return true;
		}
		if (containedRegOUT |  containedRegIN){
			lastMentioned=i;
		}
	}
	*upperBound=lastMentioned;
	return true;
}


// assumes the label exists. if it doesn't, it will run off the end of the buffer.
// starts search at the start, which is important for a specific type of JMP optimization.
uint32_t findLabelLocation(const InstructionBuffer* ib,const uint32_t labelNumber){
	const InstructionSingle* IS_i=ib->buffer-1;
	uint32_t i=0;// i is offset from where IS_i is
	while (true){
		++i;
		if (i>=ib->numberOfSlotsTaken){
			printInstructionBufferWithMessageAndNumber(ib,"WRONG",labelNumber);
			fflush(stdout);
			assert(false);
		}
		const enum InstructionTypeID id=(++IS_i)->id;
		if (id==I_LABL | id==I_FCST){
			if (IS_i->arg.D.a_0==labelNumber) return i-1;
		}
		if (id==I_JJMP){
			if (IS_i->arg.BBD.a_2==labelNumber) return i-1;
		}
	}
}


struct RegOriginInfo{
	// these two are input
	uint32_t targetLocation;
	uint8_t targetRegister;
	
	// these two are output
	bool didHitModification;
	uint32_t originLocation;
};


void getRegOriginInfo(const InstructionBuffer* ib, struct RegOriginInfo* regOriginInfo){
	uint32_t i=regOriginInfo->targetLocation;
	regOriginInfo->didHitModification=false;
	InstructionInformation II;
	uint8_t targetRegister=regOriginInfo->targetRegister;
	fillInstructionInformation(&II,ib,i);
	if (II.stopLinearRegTrace){
		// tracing a register on an AJMP or JJMP should not immediately fail to find the origin
		assert(i!=0);
		--i;
	} else if (II.doesDestroyReg){
		// when targeting an instruction that destroys registers, it should not fail if the register is in the target instructions inputs
		if (doesRegListContain(II.regIN,targetRegister)){
			assert(i!=0);
			--i;
		}
	}
	do {
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regOUT,targetRegister)){
			if (doesRegListContain(II.regIN,targetRegister)){
				regOriginInfo->didHitModification=true;
			} else {
				regOriginInfo->originLocation=i;
				return;
			}
		}
		if (II.doesDestroyReg|II.stopLinearRegTrace) break;
	} while (i--!=0);
	// if targetRegister is 0,1,F, then finding the origin is wrong, and this function should not have been called
#ifdef OPT_DEBUG_SANITY
	if (doingSanityCheck) printf("{ Caught during sanity check }\n");
#endif
	printf("Internal Error: register (%%%01X) has no origin\n",targetRegister);
	printInstructionBufferWithMessageAndNumber(ib,"location:",regOriginInfo->targetLocation);
	exit(1);
}

#ifdef OPT_DEBUG_SANITY
void sanityCheck(const InstructionBuffer* ib){
	doingSanityCheck=true;
	InstructionInformation II;
	if (0!=ib->numberOfSlotsTaken){
		fillInstructionInformation(&II,ib,0);
		if (II.regIN[0]!=16){
			// if the first instruction takes input, then it's inputs have no origin
			printf("{ Caught during sanity check }\n");
			printf("Internal Error: register (%%%01X) has no origin\n",II.regIN[0]);
			printInstructionBufferWithMessageAndNumber(ib,"location:",0);
			exit(1);
		}
	}
	for (uint32_t i=0;i<ib->numberOfSlotsTaken;i++){
		fillInstructionInformation(&II,ib,i);
		struct RegOriginInfo regOriginInfo;
		regOriginInfo.targetLocation=i;
		uint8_t ri=0;
		uint8_t r=0;
		while ((r=II.regIN[ri++])!=16){
			if (r!=0 & r!=1){
				regOriginInfo.targetRegister=r;
				getRegOriginInfo(ib,&regOriginInfo);
			}
			if (r==15){
				printf("{ Caught during sanity check }\n");
				printf("{%d}",r);
				printInstructionBufferWithMessageAndNumber(ib,"Instruction uses %%F:",i);
				exit(1);
			}
		}
		ri=0;
		while ((r=II.regOUT[ri++])!=16){
			// the optimizer is not told that ALOC sets %1 so that this check is easier
			if (r==0 | r==1){
				printf("{ Caught during sanity check }\n");
				printf("{%d}",r);
				printInstructionBufferWithMessageAndNumber(ib,"Instruction destroys the Base Pointer or Frame Pointer:",i);
				exit(1);
			}
			if (r==15){
				printf("{ Caught during sanity check }\n");
				printf("{%d}",r);
				printInstructionBufferWithMessageAndNumber(ib,"Instruction uses %%F:",i);
				exit(1);
			}
			
		}
		{
			uint8_t r2;
			uint8_t r3;
			uint8_t ri2=0;
			uint8_t ri3=0;
			while ((r2=II.regOUT[ri2++])!=16){
				while ((r3=II.regOUT[ri3++])!=16){
					if (ri2!=ri3 & r2==r3){
						printf("{ Caught during sanity check }\n");
						printInstructionBufferWithMessageAndNumber(ib,"Instructions cannot use identical registers for different outputs:",i);
						exit(1);
					}
				}
			}
		}
		if ((II.id==I_LAD0 | II.id==I_LAD1 | II.id==I_LSU0) & (II.regOUT[0]==II.regIN[1] | II.regOUT[0]==II.regIN[3])){
			printf("{ Caught during sanity check }\n");
			printInstructionBufferWithMessageAndNumber(ib,"Instructions LAD0,LAD1,LSU0 cannot use that identical register for r0:",i);
			exit(1);
		}
		if (II.id==I_LAD3 | II.id==I_LSU3 | II.id==I_LMU3){
			uint8_t r2;
			uint8_t r3;
			uint8_t ri2=0;
			uint8_t ri3=0;
			while ((r2=II.regIN[ri2++])!=16){
				while ((r3=II.regIN[ri3++])!=16){
					if ((unsigned)ri2%4u!=(unsigned)ri3%4u & r2==r3){
						printf("{ Caught during sanity check }\n");
						printInstructionBufferWithMessageAndNumber(ib,"Instructions LAD3,LSU3,LMU3 must have order not violated:",i);
						exit(1);
					}
					if ((unsigned)ri2/4u!=(unsigned)ri3/4u & r2==r3){
						printf("{ Caught during sanity check }\n");
						printInstructionBufferWithMessageAndNumber(ib,"Instructions LAD3,LSU3,LMU3 must have unique registers for first four and last four registers:",i);
						exit(1);
					}
				}
			}
		}
	}
	for (uint32_t i=0;i<ib->numberOfSlotsTaken;i++){
		fillInstructionInformation(&II,ib,i);
		assert(!II.isSymbolicInternal);
		if (II.id==I_SYRQ |
			II.id==I_SYRD |
			II.id==I_SYRW |
			II.id==I_SYRB
			){
			assert(i+1<ib->numberOfSlotsTaken); // symbolic constant had no I_SYRE
			assert(ib->buffer[i+1].id!=I_SYRE); // symbolic constant had no calculation
			for (uint32_t i2=i+1;i2<ib->numberOfSlotsTaken;i2++){
				fillInstructionInformation(&II,ib,i2);
				if (II.isSymbolicInternal){
					if (II.id==I_SYRE){
						i=i2;
						goto SafeExit;
					}
				} else {
					break;
				}
			}
			printf("{ Caught during sanity check }\n");
			printInstructionBufferWithMessageAndNumber(ib,"symbolic constant had no I_SYRE or value calculation was interupted by bad instruction:",i);
			exit(0); // symbolic constant had no I_SYRE or value calculation was interupted by bad instruction
			SafeExit:;
		}
	}
	doingSanityCheck=false;
}
#endif


/*
for AJMP and CJMP . returns true if it could trace to a labelNumber
labelNumber and labelLoadLocation may be NULL
*/
bool traceJumpLabel(const InstructionBuffer* ib,const uint32_t locationOfJump, uint32_t* labelNumber, uint32_t* labelLoadLocation){
	struct RegOriginInfo regOriginInfo0;
	struct RegOriginInfo regOriginInfo1;
	InstructionSingle IS_jmp = ib->buffer[locationOfJump];
	regOriginInfo0.targetLocation=locationOfJump;
	regOriginInfo1.targetLocation=locationOfJump;
	if (IS_jmp.id==I_AJMP){
		regOriginInfo0.targetRegister=IS_jmp.arg.B2.a_0;
		regOriginInfo1.targetRegister=IS_jmp.arg.B2.a_1;
	} else if (IS_jmp.id==I_CJMP){
		regOriginInfo0.targetRegister=IS_jmp.arg.B3.a_0;
		regOriginInfo1.targetRegister=IS_jmp.arg.B3.a_1;
	} else {
		assert(false); // called wrong
	}
	getRegOriginInfo(ib,&regOriginInfo0);
	getRegOriginInfo(ib,&regOriginInfo1);
	if (
		regOriginInfo0.didHitModification |
		regOriginInfo1.didHitModification |
		regOriginInfo0.originLocation!=regOriginInfo1.originLocation){
		
		return false;
	}
	InstructionSingle origin0 = ib->buffer[regOriginInfo0.originLocation  ];
	InstructionSingle origin1 = ib->buffer[regOriginInfo0.originLocation+1];
	InstructionSingle origin2 = ib->buffer[regOriginInfo0.originLocation+2];
	if (origin0.id==I_SYRD & 
		origin0.arg.B2.a_0==regOriginInfo0.targetRegister & 
		origin0.arg.B2.a_1==regOriginInfo1.targetRegister &
		origin1.id==I_SYCL & 
		origin2.id==I_SYRE){
		
		if (labelNumber!=NULL) *labelNumber = origin1.arg.D.a_0;
		if (labelLoadLocation!=NULL) *labelLoadLocation = regOriginInfo0.originLocation;
		return true;
	}
	return false;
}


// if the jump at startAt uses the labelNumber, it will return it
// should start at 0
// if the end is reached, it returns numberOfSlotsTaken
// does not bother to check JJMP
uint32_t findNextJumpUsingLabel(const InstructionBuffer* ib, uint32_t labelNumber, uint32_t startAt){
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* buffer=ib->buffer;
	uint32_t tempLabelNumber;
	for (uint32_t i=startAt;i<numberOfSlotsTaken;i++){
		enum InstructionTypeID id = buffer[i].id;
		if (id==I_AJMP){
			if (traceJumpLabel(ib,i,&tempLabelNumber,NULL) && tempLabelNumber==labelNumber){
				return i;
			}
		} else if (id==I_CJMP){
			if (traceJumpLabel(ib,i,&tempLabelNumber,NULL) && tempLabelNumber==labelNumber){
				return i;
			}
		}
	}
	return numberOfSlotsTaken;
}


// indexOfFirstMention may safely be NULL. if it returns false, indexOfFirstMention is garenteed to be unmodified
bool isRegMentionedAtOrAfterTarget(const InstructionBuffer* ib,const uint32_t target,const uint8_t reg, uint32_t* indexOfFirstMention){
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionInformation II;
	for (uint32_t i=target;i<numberOfSlotsTaken;i++){
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regIN,reg) | doesRegListContain(II.regOUT,reg)){
			if (indexOfFirstMention!=NULL) *indexOfFirstMention=i;
			return true;
		}
	}
	return false;
}


// indexOfFirstUsage may safely be NULL. if it returns false, indexOfFirstUsage is garenteed to be unmodified
bool isValueInRegUsedAfterTarget(const InstructionBuffer* ib,const uint32_t target,const uint8_t reg, uint32_t* indexOfFirstUsage){
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionInformation II;
	for (uint32_t i=target+1;i<numberOfSlotsTaken;i++){
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regIN,reg)){
			if (indexOfFirstUsage!=NULL) *indexOfFirstUsage=i;
			return true;
		}
		if (doesRegListContain(II.regOUT,reg) | II.doesDestroyReg | II.stopLinearRegTrace){
			return false;
		}
	}
	return false;
}


// does not check the start
bool isValueInRegUsedAnywhereThroughoutRangeWithExtentionCheck(const InstructionBuffer* ib,const uint32_t start,const uint32_t end,const uint8_t reg){
	uint32_t walk = start;
	uint32_t numberOfSlotsTaken = ib->numberOfSlotsTaken;
	InstructionInformation II;
	while (walk<=end){
		while (++walk<numberOfSlotsTaken){
			fillInstructionInformation(&II,ib,walk);
			if (II.isSymbolicInternal) continue;
			bool containedIn  = doesRegListContain(II.regIN ,reg);
			bool containedOut = doesRegListContain(II.regOUT,reg);
			if (containedIn | containedOut){
				if ((containedOut & !containedIn) & (walk>end)) return false;
				else return true;
			}
			if (II.doesDestroyReg | II.stopLinearRegTrace) break;
		}
	}
	return false;
}


bool wouldRegRenameViolateMultiOutputLaws(const InstructionBuffer* ib,const uint32_t start,const uint32_t end,const uint8_t regFrom,const uint8_t regTo){
	InstructionInformation II;
	for (uint32_t i=start;i<=end;i++){
		fillInstructionInformation(&II,ib,i);
		uint8_t r0;
		uint8_t ri0=0;
		while ((r0=II.regOUT[ri0++])!=16){
			if (r0==regFrom){
				uint8_t r1;
				uint8_t ri1=0;
				while ((r1=II.regOUT[ri1++])!=16){
					if (ri0!=ri1 & r1==regTo){
						return true;
					}
				}
				if (II.id==I_LAD3 | II.id==I_LSU3 | II.id==I_LMU3){
					if (II.regIN[4]==regTo | II.regIN[5]==regTo | II.regIN[6]==regTo | II.regIN[7]==regTo){
						return true;
					}
				}
				if ((II.id==I_LAD0 | II.id==I_LAD1 | II.id==I_LSU0) & (ri0==1) & (II.regIN[1]==regTo | II.regIN[3]==regTo)){
					return true;
				}
			}
		}
		
	}
	return false;
}


// does not check the start, uses isValueInRegUsedAnywhereThroughoutRangeWithExtentionCheck()
// returns true if it found the desired amount
// never writes more then requested
// `count!=0` must be true
bool findUnusedRegsInRange(const InstructionBuffer* ib,const uint32_t start,const uint32_t end, uint8_t* list,const uint8_t count){
	uint8_t countFound=0;
	for (uint8_t r=2;r<15;r++){
		if (!isValueInRegUsedAnywhereThroughoutRangeWithExtentionCheck(ib,start,end,r)){
			list[countFound]=r;
			if (++countFound==count) return true;
		}
	}
	return false;
}

// similiar to findUnusedRegsInRange(), however it uses isValueInRegUsedAfterTarget()
bool findUnusedRegsAfterTarget(const InstructionBuffer* ib,const uint32_t start, uint8_t* list,const uint8_t count){
	uint8_t countFound=0;
	for (uint8_t r=2;r<15;r++){
		if (!isValueInRegUsedAfterTarget(ib,start,r,NULL)){
			list[countFound]=r;
			if (++countFound==count) return true;
		}
	}
	return false;
}

// returns 16 if it couldn't find one
uint8_t findUnusedRegisterAfter(const InstructionBuffer* ib,const uint32_t index,const uint8_t notThis0,const uint8_t notThis1){
	for (uint8_t r=2;r<15;r++){
		if ((r!=notThis0 & r!=notThis1) && !isValueInRegUsedAfterTarget(ib,index,r,NULL)){
			return r;
		}
	}
	return 16;
}

struct RegRenameInfo{
	// these three are input
	uint32_t target;
	uint8_t regFrom;
	uint8_t regTo;
	
	// these four are output
	bool didSucceed;
	uint8_t suggestedReg;
	uint32_t lowerBound;
	uint32_t upperBound;
};

/*
regFrom should be used at the target
the target does not need to be the origin
if regTo==16, then this function will try to find an unused register (that is not the regFrom) and will set regRenameInfo->suggestedReg to the unused register
*/
void getRegRenameInfo(const InstructionBuffer* ib, struct RegRenameInfo* regRenameInfo){
	struct RegOriginInfo regOriginInfo;
	regOriginInfo.targetLocation=regRenameInfo->target;
	regOriginInfo.targetRegister=regRenameInfo->regFrom;	
	getRegOriginInfo(ib,&regOriginInfo);
	uint32_t upperBound;
	regRenameInfo->didSucceed=true;
	if (findRegRenameBoundaryFromOrigin(ib,regOriginInfo.originLocation,regOriginInfo.targetRegister,&upperBound)){
		regRenameInfo->lowerBound=regOriginInfo.originLocation;
		regRenameInfo->upperBound=upperBound;
		uint8_t desiredRegTo=regRenameInfo->regTo;
		if (desiredRegTo==16){
			for (uint8_t rTry=2;rTry<13;rTry++){ // would things be generally better or worse if 13 is increased to 15?
				if (
				rTry!=regOriginInfo.targetRegister && 
				!isValueInRegUsedAnywhereThroughoutRangeWithExtentionCheck(ib,regOriginInfo.originLocation,upperBound,rTry) &&
				!wouldRegRenameViolateMultiOutputLaws(ib,regOriginInfo.originLocation,upperBound,regOriginInfo.targetRegister,rTry)){
					
					regRenameInfo->suggestedReg=rTry;
					return;
				}
			}
		} else if (
				!isValueInRegUsedAnywhereThroughoutRangeWithExtentionCheck(ib,regOriginInfo.originLocation,upperBound,desiredRegTo) &&
				!wouldRegRenameViolateMultiOutputLaws(ib,regOriginInfo.originLocation,upperBound,regOriginInfo.targetRegister,desiredRegTo)){
			return;
		}
	}
	regRenameInfo->didSucceed=false;
}


/*
returns true if it succeeds. 
Writes the constant's value to 'value'
if value==NULL, then the constant is not written
*/
bool getValueInRegisterIfTraceableToRawConstants(const InstructionBuffer* ib,const uint32_t start, uint8_t reg, uint16_t* value){
	uint32_t i=start;
	InstructionInformation II;
	while (true){
		fillInstructionInformation(&II,ib,i);
		if (doesRegListContain(II.regOUT,reg)){
			if (II.id==I_MOV_){
				reg=II.regIN[0];
				i--;
				continue;
			}
			if (II.id==I_BL1_ | II.id==I_RL1_){
				if (value!=NULL) *value = (uint16_t)(II.cv);
				return true;
			}
			return false;
		}
		if (i==0 | II.doesDestroyReg | II.stopLinearRegTrace) return false;
		i--;
	}
}

struct RepeatedConstInfo{
	uint32_t i0;
	enum InstructionTypeID id;
	InstructionBuffer* ib;
	InstructionInformation II_0;
	InstructionInformation II_1;
	uint32_t indexBuff[2];
	uint16_t cvBuff[4];
	uint8_t types[2];
	uint8_t normalizedReg0[4];
	uint8_t normalizedReg1[4];
};

bool attemptRepeatedConstantOpt_sub0(const InstructionBuffer* ib,uint32_t* location,const uint32_t start,uint8_t reg,enum InstructionTypeID id){
	uint32_t i=start-1;
	InstructionInformation II;
	while (true){
		fillInstructionInformation(&II,ib,i);
		if (doesRegListContain(II.regOUT,reg)){
			if (II.id==id){
				*location=i;
				return true;
			}
			if (II.id!=I_MOV_) return false;
			reg=II.regIN[0];
		}
		if (doesRegListContain(II.regIN,reg)) return false; // this is because we don't want to perform the optimization if the intermediate value is needed for something else anyway
		if (II.doesMoveStack | II.doesRelyOnStack) return false; // this is because the optimization uses the stack as temporary storage
		assert(!(i==0 | II.doesDestroyReg | II.stopLinearRegTrace));
		i--;
	}
}


bool attemptRepeatedConstantOpt_sub1(struct RepeatedConstInfo* repeatedConstInfoPtr){
	struct RepeatedConstInfo repeatedConstInfo;
	repeatedConstInfo.i0=repeatedConstInfoPtr->i0;
	repeatedConstInfo.id=repeatedConstInfoPtr->id;
	repeatedConstInfo.ib=repeatedConstInfoPtr->ib;
	fillInstructionInformation(&repeatedConstInfo.II_0,repeatedConstInfo.ib,repeatedConstInfo.i0);
	if (repeatedConstInfo.II_0.regIN[0]==repeatedConstInfo.II_0.regIN[1])
		return false; // no need to check, it will and should fail
	if (attemptRepeatedConstantOpt_sub0(repeatedConstInfo.ib,repeatedConstInfo.indexBuff+0,repeatedConstInfo.i0,repeatedConstInfo.II_0.regIN[0],repeatedConstInfo.id)){
		repeatedConstInfo.types[0]=1;
	} else if (attemptRepeatedConstantOpt_sub0(repeatedConstInfo.ib,repeatedConstInfo.indexBuff+0,repeatedConstInfo.i0,repeatedConstInfo.II_0.regIN[1],repeatedConstInfo.id)){
		repeatedConstInfo.types[0]=0;
	} else return false;
	if (repeatedConstInfo.II_0.regIN[repeatedConstInfo.types[0]^1]!=repeatedConstInfo.II_0.regOUT[0]){
		if (isValueInRegUsedAfterTarget(repeatedConstInfo.ib,repeatedConstInfo.i0,repeatedConstInfo.II_0.regIN[repeatedConstInfo.types[0]^1],NULL))
			return false; // this is because we don't want to perform the optimization if the intermediate value is needed for something else
	}
	if (getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.i0-1,repeatedConstInfo.II_0.regIN[repeatedConstInfo.types[0]],repeatedConstInfo.cvBuff+0)){
		fillInstructionInformation(&repeatedConstInfo.II_1,repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]);
		if (getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]-1,repeatedConstInfo.II_1.regIN[0],repeatedConstInfo.cvBuff+1)){
			repeatedConstInfo.types[1]=1;
		} else if (getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]-1,repeatedConstInfo.II_1.regIN[1],repeatedConstInfo.cvBuff+1)){
			repeatedConstInfo.types[1]=0;
		} else return false;
	} else return false;
	*repeatedConstInfoPtr=repeatedConstInfo;
	return true;
}

bool attemptRepeatedConstantOpt_sub2(const InstructionBuffer* ib,uint32_t* location,const uint32_t start,uint8_t reg,uint8_t expectedOutIndex){
	uint32_t i=start-1;
	InstructionInformation II;
	while (true){
		fillInstructionInformation(&II,ib,i);
		if (doesRegListContain(II.regOUT,reg)){
			if ((II.id==I_LAD0 | II.id==I_LAD1 | II.id==I_LAD2) & II.regOUT[expectedOutIndex]==reg & II.regOUT[expectedOutIndex^1]!=reg){
				*location=i;
				return true;
			}
			if (II.id!=I_MOV_) return false;
			reg=II.regIN[0];
		}
		if (doesRegListContain(II.regIN,reg)) return false; // this is because we don't want to perform the optimization if the intermediate value is needed for something else anyway
		if (II.doesMoveStack | II.doesRelyOnStack) return false; // this is because the optimization uses the stack as temporary storage
		assert(!(i==0 | II.doesDestroyReg | II.stopLinearRegTrace));
		i--;
	}
}


bool attemptRepeatedConstantOpt_sub3(struct RepeatedConstInfo* repeatedConstInfoPtr){
	struct RepeatedConstInfo repeatedConstInfo;
	repeatedConstInfo.i0=repeatedConstInfoPtr->i0;
	repeatedConstInfo.id=repeatedConstInfoPtr->id;
	repeatedConstInfo.ib=repeatedConstInfoPtr->ib;
	repeatedConstInfo.cvBuff[0]=0;
	repeatedConstInfo.cvBuff[1]=0;
	repeatedConstInfo.cvBuff[2]=0;
	repeatedConstInfo.cvBuff[3]=0;
	fillInstructionInformation(&repeatedConstInfo.II_0,repeatedConstInfo.ib,repeatedConstInfo.i0);
	repeatedConstInfo.normalizedReg0[0]=repeatedConstInfo.II_0.regIN[0];
	repeatedConstInfo.normalizedReg0[1]=repeatedConstInfo.II_0.regIN[1];
	repeatedConstInfo.normalizedReg0[2]=repeatedConstInfo.II_0.regIN[2];
	repeatedConstInfo.normalizedReg0[3]=repeatedConstInfo.II_0.regIN[3];
	if (
	repeatedConstInfo.normalizedReg0[0]==repeatedConstInfo.normalizedReg0[1] |
	repeatedConstInfo.normalizedReg0[2]==repeatedConstInfo.normalizedReg0[3] | 
	repeatedConstInfo.normalizedReg0[0]==repeatedConstInfo.normalizedReg0[2] | 
	repeatedConstInfo.normalizedReg0[1]==repeatedConstInfo.normalizedReg0[3])
		return false; // no need to check, it will and should fail
	if (!(
	attemptRepeatedConstantOpt_sub2(repeatedConstInfo.ib,repeatedConstInfo.indexBuff+0,repeatedConstInfo.i0,repeatedConstInfo.normalizedReg0[repeatedConstInfo.types[0]=0],0) ||
	attemptRepeatedConstantOpt_sub2(repeatedConstInfo.ib,repeatedConstInfo.indexBuff+0,repeatedConstInfo.i0,repeatedConstInfo.normalizedReg0[repeatedConstInfo.types[0]=2],0)))
		return false;
	repeatedConstInfo.indexBuff[1]=repeatedConstInfo.indexBuff[0];
	for (uint8_t ri=0;ri<4;ri++){
		uint8_t r=repeatedConstInfo.normalizedReg0[ri];
		if ((r!=16 && !doesRegListContain(repeatedConstInfo.II_0.regOUT,r)) && isValueInRegUsedAfterTarget(repeatedConstInfo.ib,repeatedConstInfo.i0,r,NULL))
			return false; // this is because we don't want to perform the optimization if the intermediate value is needed for something else
	}
	if (repeatedConstInfo.types[0]!=0){
		uint8_t r;
		r=repeatedConstInfo.normalizedReg0[2];
		repeatedConstInfo.normalizedReg0[2]=repeatedConstInfo.normalizedReg0[0];
		repeatedConstInfo.normalizedReg0[0]=r;
		r=repeatedConstInfo.normalizedReg0[3];
		repeatedConstInfo.normalizedReg0[3]=repeatedConstInfo.normalizedReg0[1];
		repeatedConstInfo.normalizedReg0[1]=r;
	}
	if (
	!(repeatedConstInfo.normalizedReg0[1]==16 || attemptRepeatedConstantOpt_sub2(repeatedConstInfo.ib,repeatedConstInfo.indexBuff+1,repeatedConstInfo.i0,repeatedConstInfo.normalizedReg0[1],1)))
		return false;
	if (
	repeatedConstInfo.indexBuff[1]!=repeatedConstInfo.indexBuff[0])
		return false;
	if (
	!getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.i0-1,repeatedConstInfo.normalizedReg0[2],repeatedConstInfo.cvBuff+0))
		return false;
	if (
	!(repeatedConstInfo.normalizedReg0[3]==16 || 
	 getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.i0-1,repeatedConstInfo.normalizedReg0[3],repeatedConstInfo.cvBuff+1)))
		return false;
	fillInstructionInformation(&repeatedConstInfo.II_1,repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]);
	repeatedConstInfo.normalizedReg1[0]=repeatedConstInfo.II_1.regIN[0];
	repeatedConstInfo.normalizedReg1[1]=repeatedConstInfo.II_1.regIN[1];
	repeatedConstInfo.normalizedReg1[2]=repeatedConstInfo.II_1.regIN[2];
	repeatedConstInfo.normalizedReg1[3]=repeatedConstInfo.II_1.regIN[3];
	if (repeatedConstInfo.II_1.id==I_LAD2){
		repeatedConstInfo.normalizedReg1[1]=16;
		repeatedConstInfo.normalizedReg1[2]=repeatedConstInfo.II_1.regIN[1];
	}
	if (!(
	getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]-1,repeatedConstInfo.normalizedReg1[repeatedConstInfo.types[1]=0],repeatedConstInfo.cvBuff+2) ||
	getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]-1,repeatedConstInfo.normalizedReg1[repeatedConstInfo.types[1]=2],repeatedConstInfo.cvBuff+2)))
		return false;
	if (repeatedConstInfo.types[1]!=0){
		uint8_t r;
		r=repeatedConstInfo.normalizedReg1[2];
		repeatedConstInfo.normalizedReg1[2]=repeatedConstInfo.normalizedReg1[0];
		repeatedConstInfo.normalizedReg1[0]=r;
		r=repeatedConstInfo.normalizedReg1[3];
		repeatedConstInfo.normalizedReg1[3]=repeatedConstInfo.normalizedReg1[1];
		repeatedConstInfo.normalizedReg1[1]=r;
	}
	if (
	!(repeatedConstInfo.normalizedReg1[1]==16 || 
	 getValueInRegisterIfTraceableToRawConstants(repeatedConstInfo.ib,repeatedConstInfo.indexBuff[0]-1,repeatedConstInfo.normalizedReg1[1],repeatedConstInfo.cvBuff+3)))
		return false;
	
	*repeatedConstInfoPtr=repeatedConstInfo;
	return true;
}



bool getValueInSTPifTracableToSTP(const InstructionBuffer* ib,const uint32_t target, uint8_t reg, uint16_t* value){
	uint32_t i=target;
	InstructionInformation II;
	while (true){
		fillInstructionInformation(&II,ib,i);
		if (doesRegListContain(II.regOUT,reg)){
			if (II.id==I_MOV_){
				reg=II.regIN[0];
				i--;
				continue;
			}
			if (II.id==I_STPA | II.id==I_STPS){
				if (value!=NULL) *value=ib->buffer[i].arg.BW.a_1;
				return true;
			}
			return false;
		}
		if (i==0 | II.doesDestroyReg | II.stopLinearRegTrace) return false;
		i--;
	}
}


struct StackDataForMemoryAccess{
	uint16_t offset; // if !isDiscrete , the offset is in the alternative format 
	// and must be converted back using the stack size { stack_size-offset*2 }
	
	bool isWord;
	bool isDiscrete;
	bool isRead;
	bool isOnStack;
	bool isOnStackKnown;
};

uint8_t attemptToGetStackInfoForMemoryAccess(const InstructionBuffer* ib,const uint32_t target, struct StackDataForMemoryAccess* sdfma){
	InstructionInformation II;
	fillInstructionInformation(&II,ib,target);
	assert(II.isMemoryAccess);
	assert(sdfma!=NULL);
	sdfma->isDiscrete=II.regIN[1]!=16;
	sdfma->isWord=!II.usesReg_D;
	sdfma->isRead=II.regOUT[0]!=16;
	if (II.regIN[1]==16){
		sdfma->offset=ib->buffer[target].arg.B2.a_1;
		sdfma->isOnStack=true;
		sdfma->isOnStackKnown=true;
		return true;
	} else {
		uint16_t value0;
		uint16_t value1;
		sdfma->isOnStack=false;
		if ((sdfma->isOnStackKnown=(getValueInRegisterIfTraceableToRawConstants(ib,target-1,II.regIN[1],&value1)))){
			if ((sdfma->isOnStack=(value1==0))){
				if (getValueInSTPifTracableToSTP(ib,target-1,II.regIN[0],&value0)){
					sdfma->offset=value0;
					return true;
				}
			}
		}
		return false;
	}
}


// this refers to if the register at target can be asserted to hold a value of either 0 or 1
// target will be checked for the reg in it's outputs, and the target will be moved back until reg is in target's outputs
bool isTraceableToBool(const InstructionBuffer* ib,uint32_t target,uint8_t reg){
	InstructionInformation II;
	do {
		fillInstructionInformation(&II,ib,target);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regOUT,reg)) break;
		assert(target!=0);
	} while (target--,1);
	uint8_t r;
	uint8_t ri=0;
	uint16_t cv;
	switch (II.id){
		case I_AND_:
		case I_OR__:
		case I_XOR_:
		case I_MULS:
		case I_MOV_:
		while ((r=II.regIN[ri++])!=16){
			if (!isTraceableToBool(ib,target-1,r)){
				return false;
			}
		}
		case I_SUBC:
		return true;
		
		case I_SSUB:
		return reg==II.regOUT[0];
		
		case I_LAD2:
		return reg==II.regOUT[1];
		
		case I_BL1_:
		cv=ib->buffer[target].arg.B2.a_1;
		return cv==1 | cv==0;
		
		case I_RL1_:
		cv=ib->buffer[target].arg.BW.a_1;
		return cv==1 | cv==0;
		
		default:
		return false;
	}
}


struct LocationAndIfRequireDepl{
	uint32_t target;
	uint8_t reg;
	bool require_depl;
};
// will stop at modifications. DOES check target.
// findRegValueSourceSignalCross() is used in generateValueTraceEntries() and attemptPeepHoleOpt()
uint32_t findRegValueSourceSignalCross(const InstructionBuffer* ib, struct LocationAndIfRequireDepl* ladpl){
	uint8_t reg=ladpl->reg;
	uint32_t i=ladpl->target;
	ladpl->require_depl=false;
	InstructionInformation II;
	do {
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (II.doesMoveStack | II.doesRelyOnStack | II.doesBlockReorder | II.doesDestroyReg) ladpl->require_depl=true;
		if (doesRegListContain(II.regOUT,reg)) return i;
		assert(i!=0);
	} while (i--,1);
}

bool isValueInRegUsedAfterTargetExceptAt(const InstructionBuffer *ib,const uint32_t target,const uint32_t exceptAt,const uint8_t reg){
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	if (exceptAt>=numberOfSlotsTaken){
		goto SecondLoop;
	}
	InstructionInformation II;
	for (uint32_t i=target+1;i<exceptAt;i++){
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regIN,reg)){
			return true;
		}
		if (doesRegListContain(II.regOUT,reg) | II.doesDestroyReg | II.stopLinearRegTrace){
			return false;
		}
	}
	fillInstructionInformation(&II,ib,exceptAt);
	if ((II.doesDestroyReg | II.stopLinearRegTrace) || doesRegListContain(II.regOUT,reg)) return false;
	SecondLoop:;
	for (uint32_t i=exceptAt+1;i<numberOfSlotsTaken;i++){
		fillInstructionInformation(&II,ib,i);
		if (II.isSymbolicInternal) continue;
		if (doesRegListContain(II.regIN,reg)){
			return true;
		}
		if (doesRegListContain(II.regOUT,reg) | II.doesDestroyReg | II.stopLinearRegTrace){
			return false;
		}
	}
	return false;
}


