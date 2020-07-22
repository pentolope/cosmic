



void printInstructionBuffer(const InstructionBuffer* instructionBuffer){
	for (uint32_t i=0;i<instructionBuffer->numberOfSlotsTaken;i++){
		printf("%04X%04X |",(uint16_t)(i>>16),(uint16_t)i);
		printSingleInstructionOptCode(instructionBuffer->buffer[i]);
		printf("\n");
	}
}

void printInstructionBufferWithMessageAndNumber(const InstructionBuffer* instructionBuffer,const char* string,const uint32_t number){
	printf("\n%s [%04X%04X]\n",string,(uint16_t)(number>>16),(uint16_t)number);
	printInstructionBuffer(instructionBuffer);
	printf("\n");
}


