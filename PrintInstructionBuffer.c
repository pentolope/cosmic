


void printSingleInstructionOptCode(const InstructionSingle instructionSingle){
	switch(instructionSingle.id){
		case I_PHIS:printf("PHIS %%%01X",instructionSingle.arg.B.a_0);break;
		case I_PHIE:printf("PHIE %%%01X",instructionSingle.arg.B.a_0);break;
		case I_STPI:printf("STPI %%%01X #%04X",instructionSingle.arg.BW.a_0,instructionSingle.arg.BW.a_1);break;
		case I_ERR_:printf("ERR_");break;
		case I_DEPL:printf("DEPL");break;
		case I_PEPH:printf("PEPH");break;
		case I_NOP_:printf("NOP_");break;
		case I_PU1_:printf("PU1_ %%%01X",instructionSingle.arg.B.a_0);break;
		case I_PU2_:printf("PU2_ %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_PUA1:printf("PUA1 %%%01X",instructionSingle.arg.B.a_0);break;
		case I_PUA2:printf("PUA2 %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_POP1:printf("POP1 %%%01X",instructionSingle.arg.B.a_0);break;
		case I_POP2:printf("POP2 %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_BL1_:printf("BL1_ %%%01X $%02X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_RL1_:printf("RL1_ %%%01X #%04X",instructionSingle.arg.BW.a_0,instructionSingle.arg.BW.a_1);break;
		case I_RL2_:printf("RL2_ %%%01X %%%01X !%08X",instructionSingle.arg.BBD.a_0,instructionSingle.arg.BBD.a_1,instructionSingle.arg.BBD.a_2);break;
		case I_CALL:printf("CALL %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_RET_:printf("RET_");break;
		case I_STPA:printf("STPA %%%01X #%04X",instructionSingle.arg.BW.a_0,instructionSingle.arg.BW.a_1);break;
		case I_STPS:printf("STPS %%%01X #%04X",instructionSingle.arg.BW.a_0,instructionSingle.arg.BW.a_1);break;
		case I_STWN:printf("STWN %%%01X $%02X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_STRN:printf("STRN %%%01X $%02X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_STWV:printf("STWV %%%01X $%02X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_STRV:printf("STRV %%%01X $%02X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_ALOC:printf("ALOC");break;
		case I_ALCR:printf("ALCR %%%01X #%04X",instructionSingle.arg.BW.a_0,instructionSingle.arg.BW.a_1);break;
		case I_STOF:printf("STOF %%%01X %%%01X #%04X",instructionSingle.arg.BBW.a_0,instructionSingle.arg.BBW.a_1,instructionSingle.arg.BBW.a_2);break;
		case I_AJMP:printf("AJMP %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_CJMP:printf("CJMP %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_JJMP:printf("JJMP %%%01X %%%01X :%08X",instructionSingle.arg.BBD.a_0,instructionSingle.arg.BBD.a_1,instructionSingle.arg.BBD.a_2);break;
		case I_JTEN:printf("JTEN @%08X",instructionSingle.arg.D.a_0);break;
		case I_JEND:printf("JEND");break;
		case I_MOV_:printf("MOV_ %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_AND_:printf("AND_ %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_OR__:printf("OR__ %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_XOR_:printf("XOR_ %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_SSUB:printf("SSUB %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_ADDN:printf("ADDN %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_ADDC:printf("ADDC %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_SUBN:printf("SUBN %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_SUBC:printf("SUBC %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_MULS:printf("MULS %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_MULL:printf("MULL %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_DIVM:printf("DIVM %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_SHFT:printf("SHFT %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_BSWP:printf("BSWP %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_MWWN:printf("MWWN %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_MRWN:printf("MRWN %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_MWWV:printf("MWWV %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_MRWV:printf("MRWV %%%01X %%%01X %%%01X",instructionSingle.arg.BBB.a_0,instructionSingle.arg.BBB.a_1,instructionSingle.arg.BBB.a_2);break;
		case I_MWBN:printf("MWBN %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_MRBN:printf("MRBN %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_MWBV:printf("MWBV %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_MRBV:printf("MRBV %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_D32U:printf("D32U");break;
		case I_R32U:printf("R32U");break;
		case I_D32S:printf("D32S");break;
		case I_R32S:printf("R32S");break;
		case I_FCST:printf("FCST $%02X #%04X :%08X",instructionSingle.arg.BWD.a_0,instructionSingle.arg.BWD.a_1,instructionSingle.arg.BWD.a_2);break;
		case I_FCEN:printf("FCEN");break;
		case I_SYRB:printf("SYRB %%%01X",instructionSingle.arg.B.a_0);break;
		case I_SYRW:printf("SYRW %%%01X",instructionSingle.arg.B.a_0);break;
		case I_SYRD:printf("SYRD %%%01X %%%01X",instructionSingle.arg.BB.a_0,instructionSingle.arg.BB.a_1);break;
		case I_SYRE:printf("SYRE");break;
		case I_NSNB:printf("NSNB !%08X",instructionSingle.arg.D.a_0);break;
		case I_NSCB:printf("NSCB !%08X",instructionSingle.arg.D.a_0);break;
		case I_NSNW:printf("NSNW !%08X",instructionSingle.arg.D.a_0);break;
		case I_NSCW:printf("NSCW !%08X",instructionSingle.arg.D.a_0);break;
		case I_ZNXB:printf("ZNXB !%08X",instructionSingle.arg.D.a_0);break;
		case I_ZNXW:printf("ZNXW !%08X",instructionSingle.arg.D.a_0);break;
		case I_BYTE:printf("BYTE $%02X",instructionSingle.arg.B.a_0);break;
		case I_WORD:printf("WORD #%04X",instructionSingle.arg.W.a_0);break;
		case I_DWRD:printf("DWRD !%08X",instructionSingle.arg.D.a_0);break;
		case I_SYDB:printf("SYDB");break;
		case I_SYDW:printf("SYDW");break;
		case I_SYDD:printf("SYDD");break;
		case I_SYDE:printf("SYDE");break;
		case I_LOFF:printf("LOFF !%08X",instructionSingle.arg.D.a_0);break;
		case I_SYCB:printf("SYCB $%02X",instructionSingle.arg.B.a_0);break;
		case I_SYCW:printf("SYCW #%04X",instructionSingle.arg.W.a_0);break;
		case I_SYCD:printf("SYCD !%08X",instructionSingle.arg.D.a_0);break;
		case I_SYC0:printf("SYC0");break;
		case I_SYC1:printf("SYC1");break;
		case I_SYC2:printf("SYC2");break;
		case I_SYC3:printf("SYC3");break;
		case I_SYC4:printf("SYC4");break;
		case I_SYC5:printf("SYC5");break;
		case I_SYC6:printf("SYC6");break;
		case I_SYC7:printf("SYC7");break;
		case I_SYC8:printf("SYC8");break;
		case I_SYC9:printf("SYC9");break;
		case I_SYCX:printf("SYCX");break;
		case I_SYCY:printf("SYCY");break;
		case I_SYCA:printf("SYCA");break;
		case I_SYCU:printf("SYCU");break;
		case I_SYCO:printf("SYCO");break;
		case I_SYCQ:printf("SYCQ");break;
		case I_SYCC:printf("SYCC");break;
		case I_SYCN:printf("SYCN");break;
		case I_SYCM:printf("SYCM");break;
		case I_SYCZ:printf("SYCZ");break;
		case I_SYCS:printf("SYCS");break;
		case I_SYCT:printf("SYCT");break;
		case I_SYCL:printf("SYCL @%08X",instructionSingle.arg.D.a_0);break;
		case I_LABL:printf("LABL :%08X",instructionSingle.arg.D.a_0);break;
		case I_INSR:printf("INSR %%%01X",instructionSingle.arg.B.a_0);break;
	}
}


void printInstructionBuffer(const InstructionBuffer* instructionBuffer){
	for (uint32_t i=0;i<instructionBuffer->numberOfSlotsTaken;i++){
		printf("%08X |",i);
		printSingleInstructionOptCode(instructionBuffer->buffer[i]);
		printf("\n");
	}
}

void printInstructionBufferWithMessageAndNumber(const InstructionBuffer* instructionBuffer,const char* string,const uint32_t number){
	printf("\n%s [%08X]\n",string,number);
	printInstructionBuffer(instructionBuffer);
	printf("\n");
}


