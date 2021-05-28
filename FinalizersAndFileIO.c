




// doesFileExist() isn't used now, but I might want it later
#if 0
bool doesFileExist(char* filePath){
	FILE *file;
	if((file = fopen(filePath,"r"))!=NULL){
		fclose(file);
		return true;
	}
	return false;
}
#endif



struct BinContainer{
	struct SymbolEntry{
		char* name;
		uint32_t label;
		uint8_t type;
		
		bool hasMatch; // this is if the identifier is in the other BinContainer
		uint32_t match; // this is the index in the other BinContainer
	}* symbols;
	uint32_t len_symbols;
	InstructionBuffer functions;
	InstructionBuffer staticData;
};



// also prepends and appends a newline
char* loadFileContentsAsSourceCode(const char* filePath){
	FILE *inputFile = fopen(filePath,"r");
	if (inputFile==NULL){
		err_10_1_(strMerge3("Could not load file \"",filePath,"\""));
	}
	fpos_t startingPositionOfFile;
	fgetpos(inputFile,&startingPositionOfFile);
	int32_t lengthOfFile = 0;
	while (fgetc(inputFile)!=EOF){
		lengthOfFile++;
		if (lengthOfFile>268435456L){ // 2**28
			fclose(inputFile);
			err_10_1_(strMerge3("Input file \"",filePath,"\" is too large"));
		}
	}
	fsetpos(inputFile,&startingPositionOfFile);
	char *characterArrayOfFile = cosmic_malloc((lengthOfFile+3)*sizeof(char));
	characterArrayOfFile[0] = '\n';
	characterArrayOfFile[lengthOfFile+1] = '\n';
	characterArrayOfFile[lengthOfFile+2] = 0;
	bool hasGivenStrangeCharacterWarning=false;
	for (int32_t i=1;i<lengthOfFile+1;i++){
		int currentCharacter = fgetc(inputFile);
		assert(currentCharacter!=EOF); // length should have already been determined
		if ((currentCharacter<32 & currentCharacter!='\r' & currentCharacter!='\n' & currentCharacter!='\t') | currentCharacter>126){
			if (!hasGivenStrangeCharacterWarning){
				err_00__0("Strange character found when opening source file. Is it really a C source code file?");
				hasGivenStrangeCharacterWarning=true;
			}
			currentCharacter=' ';
		}
		characterArrayOfFile[i]=currentCharacter;
	}
	fclose(inputFile);
	return characterArrayOfFile;
}



FILE* safe_fputc_file;
const char* safe_fputc_file_path;

void safe_fputc(int value){
	if (fputc(value,safe_fputc_file)!=value) err_10_1_(strMerge3("Error in writing \'",safe_fputc_file_path,"\'"));
}


// destroys global_static_data
void finalOutputFromCompile(const char* filePath){
	if (filePath!=NULL){
		CompressedInstructionBuffer cis_global_static_data=compressInstructionBuffer(&global_static_data);
		destroyInstructionBuffer(&global_static_data);
		uint32_t symbolEntryLength=0;
		for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
			struct GlobalVariableEntry gve=blockFrameArray.globalBlockFrame.globalVariableEntries[i];
			if (!gve.usedStatic) symbolEntryLength++;
		}
		for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
			struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
			if (!gfe.usedStatic^gfe.usedInline) symbolEntryLength++;
		}
		safe_fputc_file_path=filePath;
		safe_fputc_file = fopen(safe_fputc_file_path,"wb");
		if (safe_fputc_file==NULL) err_10_1_(strMerge3("Could not open \'",safe_fputc_file_path,"\' for output"));
		safe_fputc((uint8_t)(symbolEntryLength));
		safe_fputc((uint8_t)(symbolEntryLength>>8));
		safe_fputc((uint8_t)(symbolEntryLength>>16));
		safe_fputc((uint8_t)(symbolEntryLength>>24));
		for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
			struct GlobalVariableEntry gve=blockFrameArray.globalBlockFrame.globalVariableEntries[i];
			if (!gve.usedStatic){
				safe_fputc((uint8_t)(gve.labelID));
				safe_fputc((uint8_t)(gve.labelID>>8));
				safe_fputc((uint8_t)(gve.labelID>>16));
				safe_fputc((uint8_t)(gve.labelID>>24));
				char* t=applyToTypeStringGetIdentifierToNew(gve.typeString);
				for (uint32_t i2=0;t[i2];i2++) safe_fputc(t[i2]);
				cosmic_free(t);
				safe_fputc(gve.usedExtern+gve.hadInitializer*2);
				/*
				0- no extern, no initializer
				1-yes extern, no initializer
				2- no extern,yes initializer
				3-yes extern,yes initializer
				*/
			}
		}
		for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
			struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
			if (!gfe.usedStatic^gfe.usedInline){
				safe_fputc((uint8_t)(gfe.labelID));
				safe_fputc((uint8_t)(gfe.labelID>>8));
				safe_fputc((uint8_t)(gfe.labelID>>16));
				safe_fputc((uint8_t)(gfe.labelID>>24));
				char* t=applyToTypeStringGetIdentifierToNew(gfe.typeString);
				for (uint32_t i2=0;t[i2];i2++) safe_fputc(t[i2]);
				cosmic_free(t);
				safe_fputc(gfe.usedExtern+4);
				/*
				4- no extern (and therefore   defined)
				5-yes extern (and therefore undefined)
				*/
			}
		}
		for (uint32_t i=0;i<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i++){
			CompressedInstructionBuffer cib=globalInstructionBuffersOfFunctions.slots[i];
			uint32_t lenWithoutTerminate=cib.allocLen-1;
			for (uint32_t i2=0;i2<lenWithoutTerminate;i2++){
				safe_fputc(cib.byteCode[i2]);
			}
		}
		safe_fputc(0);
		for (uint32_t i=0;i<cis_global_static_data.allocLen;i++){
			safe_fputc(cis_global_static_data.byteCode[i]);
		}
		// null terminator for static data is already placed by the loop above
		if (fclose(safe_fputc_file)!=0) err_10_1_(strMerge3("Could not close \'",safe_fputc_file_path,"\' after writting"));
		safe_fputc_file=NULL;
		safe_fputc_file_path=NULL;
		cosmic_free(cis_global_static_data.byteCode);
	} else {
		destroyInstructionBuffer(&global_static_data);
	}
}




// should be done before applyFinalizeToGlobalStaticData()
void doFunctionDefinitionCheck(){
	uint32_t countToCheck=0;
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
		struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
		if (!gfe.usedExtern&!gfe.hasBeenDefined) countToCheck++;
	}
	if (countToCheck==0) return;
	uint32_t* labelMarkoff=cosmic_malloc(countToCheck*sizeof(uint32_t));
	uint32_t walkingIndex=0;
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
		struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
		if (!gfe.usedExtern&!gfe.hasBeenDefined) labelMarkoff[walkingIndex++]=gfe.labelID;
	}
	doLabelMarkoffInAllCompressedInstructionBuffers(labelMarkoff,countToCheck);
	doLabelMarkoffInInstructionBuffer(&global_static_data,labelMarkoff,countToCheck);
	bool gaveError=false;
	for (uint32_t i=0;i<countToCheck;i++){
		if (labelMarkoff[i]==0){
			gaveError=true;
			walkingIndex=0;
			bool foundEntry=false;
			for (uint32_t i2=0;i2<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i2++){
				struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i2];
				if (!gfe.usedExtern&!gfe.hasBeenDefined){
					if (walkingIndex++==i){
						foundEntry=true;
						err_11000("This function is declared, used, not defined, and not declared with 'extern'. I can\'t make your functions for you.",gfe.indexOfDeclaration);
						break;
					}
				}
			}
			assert(foundEntry);
		}
	}
	cosmic_free(labelMarkoff);
	if (gaveError) exit(1);
}

// destroys string data
void applyFinalizeToGlobalStaticData(){
	if (globalStringLiteralEntries.numberOfValidEntries!=0){
		for (uint32_t i0=1;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			uint32_t i1=i0;
			while (i1!=0){
				struct StringLiteralEntry* sle_ptr_1=globalStringLiteralEntries.entries+(i1-0);
				struct StringLiteralEntry* sle_ptr_2=globalStringLiteralEntries.entries+(i1-1);
				if (sle_ptr_2->lengthOfData<=sle_ptr_1->lengthOfData) break;
				struct StringLiteralEntry sle;
				sle=*sle_ptr_2;
				*sle_ptr_2=*sle_ptr_1;
				*sle_ptr_1=sle;
				i1--;
			}
		}
		for (uint32_t i0=1;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			uint32_t i1=i0;
			while (i1!=0){
				struct StringLiteralEntry* sle_ptr_1=globalStringLiteralEntries.entries+(i1-0);
				struct StringLiteralEntry* sle_ptr_2=globalStringLiteralEntries.entries+(i1-1);
				if (sle_ptr_2->lengthOfData!=sle_ptr_1->lengthOfData) break;
				uint8_t stringCompareResult = stringDataCompare(sle_ptr_1->data,sle_ptr_2->data,sle_ptr_2->lengthOfData);
				if (stringCompareResult!=1) break;
				struct StringLiteralEntry sle;
				sle=*sle_ptr_2;
				*sle_ptr_2=*sle_ptr_1;
				*sle_ptr_1=sle;
				i1--;
			}
		}
		{
		uint32_t* labelRenamesFrom;
		uint32_t* labelRenamesTo;
		{
		uint32_t sizeForAllocation=(globalStringLiteralEntries.numberOfValidEntries-1)*sizeof(uint32_t);
		labelRenamesFrom=cosmic_malloc(sizeForAllocation);
		labelRenamesTo=cosmic_malloc(sizeForAllocation);
		}
		uint32_t labelRenamesCount=0;
		for (uint32_t i0=1;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			struct StringLiteralEntry* sle_ptr_1=globalStringLiteralEntries.entries+(i0-0);
			struct StringLiteralEntry* sle_ptr_2=globalStringLiteralEntries.entries+(i0-1);
			while (i0<globalStringLiteralEntries.numberOfValidEntries){
				if (sle_ptr_2->lengthOfData!=sle_ptr_1->lengthOfData) break;
				uint8_t stringCompareResult = stringDataCompare(sle_ptr_1->data,sle_ptr_2->data,sle_ptr_2->lengthOfData);
				if (stringCompareResult!=0) break;
				labelRenamesFrom[labelRenamesCount]=sle_ptr_1->label;
				labelRenamesTo[labelRenamesCount]=sle_ptr_2->label;
				labelRenamesCount++;
				cosmic_free(sle_ptr_1->data);
				for (uint32_t i1=i0+1;i1<globalStringLiteralEntries.numberOfValidEntries;i1++){
					globalStringLiteralEntries.entries[i1-1]=globalStringLiteralEntries.entries[i1-0];
				}
				globalStringLiteralEntries.numberOfValidEntries--;
			}
		}
		applyLabelRenamesInAllCompressedInstructionBuffers(labelRenamesFrom,labelRenamesTo,labelRenamesCount);
		applyLabelRenamesInInstructionBuffer(&global_static_data,labelRenamesFrom,labelRenamesTo,labelRenamesCount);
		cosmic_free(labelRenamesFrom);
		cosmic_free(labelRenamesTo);
		}
	}
	RemoveUnusedStart:;
	{
	InstructionSingle* buffer=global_static_data.buffer;
	uint32_t labelCount=0;
	uint32_t i0;
	for (i0=0;i0<global_static_data.numberOfSlotsTaken;i0++){
		if (buffer[i0].id==I_LABL) labelCount++;
	}
	uint32_t labelTotal=labelCount;
	if (labelTotal==0) goto RemoveUnusedEnd;
	uint32_t* labelMarkoff=cosmic_malloc(labelTotal*sizeof(uint32_t));
	labelCount=0;
	for (i0=0;i0<global_static_data.numberOfSlotsTaken;i0++){
		if (buffer[i0].id==I_LABL) labelMarkoff[labelCount++]=buffer[i0].arg.D.a_0;
	}
	doLabelMarkoffInAllCompressedInstructionBuffers(labelMarkoff,labelCount);
	doLabelMarkoffInInstructionBuffer(&global_static_data,labelMarkoff,labelCount);
	uint32_t i1;
	for (i1=0;i1<labelTotal;i1++){
		if (labelMarkoff[i1]!=0){
			// then unused
			labelCount=0;
			for (i0=0;i0<global_static_data.numberOfSlotsTaken;i0++){
				if (buffer[i0].id==I_LABL){
					if (labelCount==i1){
						uint32_t unusedLabelNumber=labelMarkoff[i1];
						assert(unusedLabelNumber==buffer[i0].arg.D.a_0);
						bool foundVariable=false;
						for (uint32_t i2=0;i2<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i2++){
							if (blockFrameArray.globalBlockFrame.globalVariableEntries[i2].labelID==unusedLabelNumber){
								if (!blockFrameArray.globalBlockFrame.globalVariableEntries[i2].usedStatic){
									// if the declaration used 'static', then it cannot be verified that this can be removed, it may be used elsewhere at link time
									goto ContinueUnusedSearch;
								}
								err_010_0("This static variable is effectively unused. It\'s data is being removed. Remove the keyword 'static' to avoid removal.",blockFrameArray.globalBlockFrame.globalVariableEntries[i2].indexOfDeclaration);
								foundVariable=true;
								break;
							}
						}
						assert(foundVariable);
						assert(i0!=0 && (buffer[i0-1].id==I_NSNB));
						uint32_t bytesLeft=buffer[i0-1].arg.D.a_0;
						buffer[i0-1].id=I_NOP_;
						while (bytesLeft!=0){
							enum InstructionTypeID id=buffer[i0].id;
							if (id==I_SYDB | id==I_BYTE){bytesLeft-=1;}
							else if (id==I_SYDW | id==I_WORD){assert(bytesLeft>=2);bytesLeft-=2;}
							else if (id==I_SYDD | id==I_DWRD){assert(bytesLeft>=4);bytesLeft-=4;}
							else if (id==I_ZNXB){assert(bytesLeft>=buffer[i0].arg.D.a_0);bytesLeft-=buffer[i0].arg.D.a_0;}
							buffer[i0++].id=I_NOP_;
							if (i0>=global_static_data.numberOfSlotsTaken){
								assert(bytesLeft==0);
								break;
							}
						}
						removeNop(&global_static_data);
						cosmic_free(labelMarkoff);
						goto RemoveUnusedStart;
					} else {
						labelCount++;
					}
				}
			}
		}
		ContinueUnusedSearch:;
	}
	cosmic_free(labelMarkoff);
	}
	RemoveUnusedEnd:;
	if (globalStringLiteralEntries.numberOfValidEntries!=0){
		{
		bool didStringRemoveAtLeastOne=false;
		uint32_t* stringLabelMarkoff=cosmic_malloc(globalStringLiteralEntries.numberOfValidEntries*sizeof(uint32_t));
		uint32_t i0;
		for (i0=0;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			stringLabelMarkoff[i0]=globalStringLiteralEntries.entries[i0].label;
		}
		doLabelMarkoffInAllCompressedInstructionBuffers(stringLabelMarkoff,globalStringLiteralEntries.numberOfValidEntries);
		doLabelMarkoffInInstructionBuffer(&global_static_data,stringLabelMarkoff,globalStringLiteralEntries.numberOfValidEntries);
		for (i0=globalStringLiteralEntries.numberOfValidEntries;i0--!=0;){
			if (stringLabelMarkoff[i0]!=0){
				didStringRemoveAtLeastOne=true;
				cosmic_free(globalStringLiteralEntries.entries[i0].data);
				for (uint32_t i1=i0+1;i1<globalStringLiteralEntries.numberOfValidEntries;i1++){
					globalStringLiteralEntries.entries[i1-1]=globalStringLiteralEntries.entries[i1-0];
				}
				globalStringLiteralEntries.numberOfValidEntries--;
			}
		}
		cosmic_free(stringLabelMarkoff);
		if (didStringRemoveAtLeastOne) goto RemoveUnusedStart;
		}
		// todo: allow string literals to be stored inside of other string literals (put that here)
		{
		InstructionBuffer ib_temp;
		initInstructionBuffer(&ib_temp);
		uint32_t totalByteCount=0;
		for (uint32_t i0=0;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			totalByteCount+=globalStringLiteralEntries.entries[i0].lengthOfData;
		}
		InstructionSingle IS_temp;
		IS_temp.id=I_NSNB;
		IS_temp.arg.D.a_0=totalByteCount;
		addInstruction(&ib_temp,IS_temp);
		for (uint32_t i0=0;i0<globalStringLiteralEntries.numberOfValidEntries;i0++){
			struct StringLiteralEntry sle=globalStringLiteralEntries.entries[i0];
			IS_temp.id=I_LABL;
			IS_temp.arg.D.a_0=sle.label;
			addInstruction(&ib_temp,IS_temp);
			IS_temp.id=I_BYTE;
			for (uint32_t i1=0;i1<sle.lengthOfData;i1++){
				IS_temp.arg.B1.a_0=sle.data[i1];
				addInstruction(&ib_temp,IS_temp);
			}
			cosmic_free(sle.data);
		}
		cosmic_free(globalStringLiteralEntries.entries);
		globalStringLiteralEntries.numberOfValidEntries=0;
		globalStringLiteralEntries.entries=NULL;
		dataBytecodeReduction(&ib_temp);
		singleMergeIB(&global_static_data,&ib_temp);
		destroyInstructionBuffer(&ib_temp);
		}
	}
	dataBytecodeReduction(&global_static_data); // probably won't find anything, but it doesn't really hurt to try
}

// if destroyAllCompilerInfo() is run after the preprocesser and compiler, then nearly all global state effected by the preprocesser and compiler will be reset when it finishes
void destroyAllCompilerInfo(){
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
		struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
		destroyFunctionTypeAnalysis(&gfe.functionTypeAnalysis);
		cosmic_free((char*)gfe.typeString);
	}
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots;i++){
		struct GlobalTypeEntry gte=blockFrameArray.globalBlockFrame.globalTypeEntries[i];
		for (uint32_t i2=0;i2<gte.numberOfMemberEntries;i2++){
			cosmic_free((char*)gte.arrayOfMemberEntries[i2].typeString);
		}
		cosmic_free(gte.arrayOfMemberEntries);
		cosmic_free((char*)gte.name);
	}
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
		cosmic_free((char*)blockFrameArray.globalBlockFrame.globalVariableEntries[i].typeString);
	}
	for (uint32_t i=0;i<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i++){
		cosmic_free(globalInstructionBuffersOfFunctions.slots[i].byteCode);
	}
	for (uint16_t i=0;i<sourceFileListing.allocationCount;i++){
		cosmic_free(sourceFileListing.sourceFileOriginals[i].fileName);
		cosmic_free(sourceFileListing.sourceFileOriginals[i].originalContentOfFile);
	}
	for (uint32_t i=0;i<globalTypedefEntries.numberOfAllocatedSlots;i++){
		struct TypedefEntry te=globalTypedefEntries.entries[i];
		if (te.isThisSlotTaken) cosmic_free(te.typeSpecifier);
	}
	cosmic_free(blockFrameArray.globalBlockFrame.globalFunctionEntries);
	cosmic_free(blockFrameArray.globalBlockFrame.globalTypeEntries);
	cosmic_free(blockFrameArray.globalBlockFrame.globalVariableEntries);
	cosmic_free(blockFrameArray.entries);
	cosmic_free(globalInstructionBuffersOfFunctions.slots);
	cosmic_free(initializerMap.entries);
	cosmic_free(expressionTreeGlobalBuffer.expressionTreeNodes);
	cosmic_free(globalTypedefEntries.entries);
	cosmic_free((char*)sourceContainer.string);
	cosmic_free(sourceContainer.sourceChar);
	globalInstructionBuffersOfFunctions.slots=NULL;
	globalInstructionBuffersOfFunctions.numberOfSlotsTaken=0;
	globalInstructionBuffersOfFunctions.numberOfSlotsAllocated=0;
	blockFrameArray.globalBlockFrame.globalTypeEntries=NULL;
	blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots=0;
	blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalTypeEntrySlots=0;
	blockFrameArray.globalBlockFrame.globalFunctionEntries=NULL;
	blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots=0;
	blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalFunctionEntrySlots=0;
	blockFrameArray.globalBlockFrame.globalVariableEntries=NULL;
	blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots=0;
	blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalVariableEntrySlots=0;
	blockFrameArray.entries=NULL;
	blockFrameArray.numberOfAllocatedSlots=0;
	blockFrameArray.numberOfValidSlots=0;
	expressionTreeGlobalBuffer.expressionTreeNodes=NULL;
	expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray=0;
	expressionTreeGlobalBuffer.walkingIndexForNextOpenSlot=0;
	globalTypedefEntries.entries=NULL;
	globalTypedefEntries.numberOfAllocatedSlots=0;
	initializerMap.entries=NULL;
	initializerMap.numberOfEntriesTaken=0;
	initializerMap.numberOfEntriesAllocated=0;
	sourceFileListing.allocationCount=0;
	sourceContainer.string=NULL;
	sourceContainer.sourceChar=NULL;
	sourceContainer.allocationLength=0;
}


void genAllBinContainerMatch(struct BinContainer bc0,struct BinContainer bc1){
	for (uint32_t i0=0;i0<bc0.len_symbols;i0++){
		bool* hasMatch=&bc0.symbols[i0].hasMatch;
		*hasMatch=false;
		uint32_t* match=&bc0.symbols[i0].match;
		char* name=bc0.symbols[i0].name;
		for (uint32_t i1=0;i1<bc1.len_symbols;i1++){
			if (doStringsMatch(name,bc1.symbols[i1].name)){
				*hasMatch=true;
				*match=i1;
				break;
			}
		}
	}
	for (uint32_t i1=0;i1<bc1.len_symbols;i1++){
		bool* hasMatch=&bc1.symbols[i1].hasMatch;
		*hasMatch=false;
		uint32_t* match=&bc1.symbols[i1].match;
		char* name=bc1.symbols[i1].name;
		for (uint32_t i0=0;i0<bc0.len_symbols;i0++){
			if (doStringsMatch(name,bc0.symbols[i0].name)){
				*hasMatch=true;
				*match=i0;
				break;
			}
		}
	}
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



struct BinContainer loadFileContentsAsBinContainer(const char* filePath){
	binaryFileLoadState.binaryFile=fopen(filePath,"rb");
	if (binaryFileLoadState.binaryFile==NULL){
		err_10_1_(strMerge3("Could not load file \"",filePath,"\""));
	}
	binaryFileLoadState.corruptionErrorMessage=strMerge3("Input file \"",filePath,"\" is corrupted");
	struct BinContainer binContainerOut;
	{
		struct StringBuilder* stringBuilder=stringBuilderCreate();
		binContainerOut.len_symbols =(uint32_t)binaryFile_noEOF_fgetc();
		binContainerOut.len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<8;
		binContainerOut.len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<16;
		binContainerOut.len_symbols|=(uint32_t)binaryFile_noEOF_fgetc()<<24;
		binContainerOut.symbols=cosmic_malloc(binContainerOut.len_symbols*sizeof(struct SymbolEntry));
		
		for (uint32_t i0=0;i0<binContainerOut.len_symbols;i0++){
			struct SymbolEntry se;
			se.label =(uint32_t)binaryFile_noEOF_fgetc();
			se.label|=(uint32_t)binaryFile_noEOF_fgetc()<<8;
			se.label|=(uint32_t)binaryFile_noEOF_fgetc()<<16;
			se.label|=(uint32_t)binaryFile_noEOF_fgetc()<<24;
			char c;
			while (6<=(c=binaryFile_noEOF_fgetc())){
				stringBuilderAppendChar(stringBuilder,c);
			}
			se.type=c;
			se.name=stringBuilderToString(stringBuilder);
			stringBuilderClear(stringBuilder);
			binContainerOut.symbols[i0]=se;
		}
		stringBuilderDestroy(stringBuilder);
	}
	binContainerOut.functions  =decompressInstructionBufferOnLoad();
	binContainerOut.staticData =decompressInstructionBufferOnLoad();
	binaryFile_isEOF_fgetc();
	
	cosmic_free((char*)binaryFileLoadState.corruptionErrorMessage);
	fclose(binaryFileLoadState.binaryFile);
	binaryFileLoadState.corruptionErrorMessage=NULL;
	binaryFileLoadState.binaryFile=NULL;
	
	return binContainerOut;
}






