



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


