

void insert_IB_load_byte(InstructionBuffer *ib_ToAppendTo,uint8_t literalValue){
	uint32_t insertSpot = ib_ToAppendTo->numberOfSlotsTaken;
	addInstruction(ib_ToAppendTo,ib_load_byte.buffer[0]);
	ib_ToAppendTo->buffer[insertSpot].arg.B2.a_1 = literalValue;
	addInstruction(ib_ToAppendTo,ib_load_byte.buffer[1]);
}
void insert_IB_load_word(InstructionBuffer *ib_ToAppendTo,uint16_t literalValue){
	uint32_t insertSpot = ib_ToAppendTo->numberOfSlotsTaken;
	addInstruction(ib_ToAppendTo,ib_load_word.buffer[0]);
	ib_ToAppendTo->buffer[insertSpot].arg.BW.a_1 = literalValue;
	addInstruction(ib_ToAppendTo,ib_load_word.buffer[1]);
}
void insert_IB_load_dword(InstructionBuffer *ib_ToAppendTo,uint32_t literalValue){
	uint32_t insertSpot = ib_ToAppendTo->numberOfSlotsTaken;
	addInstruction(ib_ToAppendTo,ib_load_dword.buffer[0]);
	ib_ToAppendTo->buffer[insertSpot].arg.BBD.a_2 = literalValue;
	addInstruction(ib_ToAppendTo,ib_load_dword.buffer[1]);
}
void insert_IB_STPA(InstructionBuffer *ib_ToAppendTo,uint16_t reversedOffset){
	uint32_t insertSpot = ib_ToAppendTo->numberOfSlotsTaken;
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[0]);
	ib_ToAppendTo->buffer[insertSpot].arg.BW.a_1 = reversedOffset;
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[2]);
}
void insert_IB_STPI(InstructionBuffer *ib_ToAppendTo,uint16_t reversedOffset){
	uint32_t insertSpot = ib_ToAppendTo->numberOfSlotsTaken;
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[0]);
	ib_ToAppendTo->buffer[insertSpot].id = I_STPI;
	ib_ToAppendTo->buffer[insertSpot].arg.BW.a_1 = reversedOffset;
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_STPA.buffer[2]);
}

// insert_IB_call_nonComplex_function should be called after arguments are pushed
void insert_IB_call_nonComplex_function(InstructionBuffer *ib_ToAppendTo,uint32_t labelNumber,uint16_t stackSize,bool returnsNonVoid){
	InstructionSingle iS;
	if (returnsNonVoid){
		iS.id = I_STOF;
		iS.arg.BW.a_0 = 2;
		iS.arg.BW.a_1 = stackSize;
		addInstruction(ib_ToAppendTo,iS);
		iS.id = I_PUA1;
		iS.arg.B1.a_0 = 2;
		addInstruction(ib_ToAppendTo,iS);
		stackSize+=2;
	}
	iS.id = I_RL1_;
	iS.arg.BW.a_0 = 2;
	iS.arg.BW.a_1 = stackSize;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_PUA1;
	iS.arg.B1.a_0 = 2;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_SYRD;
	iS.arg.B2.a_0 = 10;
	iS.arg.B2.a_1 = 11;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_SYCL;
	iS.arg.D.a_0 = labelNumber;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_SYRE;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_CALL;
	iS.arg.B2.a_0 = 10;
	iS.arg.B2.a_1 = 11;
	addInstruction(ib_ToAppendTo,iS);
}

// insert_IB_call_complex_function should be called after arguments are pushed, then address was pushed
void insert_IB_call_complex_function(InstructionBuffer *ib_ToAppendTo,uint16_t stackSize,bool returnsNonVoid){
	InstructionSingle iS;
	iS.id = I_POP2;
	iS.arg.B2.a_0 = 10;
	iS.arg.B2.a_1 = 11;
	addInstruction(ib_ToAppendTo,iS);
	if (returnsNonVoid){
		iS.id = I_STOF;
		iS.arg.BW.a_0 = 2;
		iS.arg.BW.a_1 = stackSize;
		addInstruction(ib_ToAppendTo,iS);
		iS.id = I_PUA1;
		iS.arg.B1.a_0 = 2;
		addInstruction(ib_ToAppendTo,iS);
		stackSize+=2;
	}
	iS.id = I_RL1_;
	iS.arg.BW.a_0 = 2;
	iS.arg.BW.a_1 = stackSize;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_PUA1;
	iS.arg.B1.a_0 = 2;
	addInstruction(ib_ToAppendTo,iS);
	iS.id = I_CALL;
	iS.arg.B2.a_0 = 10;
	iS.arg.B2.a_1 = 11;
	addInstruction(ib_ToAppendTo,iS);
}
