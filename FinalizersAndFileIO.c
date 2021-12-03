




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

struct FileContentSegment{
	struct FileContentSegment* next;
	uint16_t position;
	char buffer[8192];
};

struct FileContentStruct {
	struct FileContentSegment* startSegment;
	struct FileContentSegment* walkSegment;
	uint32_t length;
};

// also prepends and appends a newline
char* loadFileContentsAsSourceCode(const char* filePath){
	FILE *inputFile = fopen(filePath,"r");
	if (inputFile==NULL){
		err_10_1_(strMerge3("Could not load file \"",filePath,"\""));
	}
	struct FileContentStruct fcs={0};
	fcs.walkSegment=fcs.startSegment=cosmic_malloc(sizeof(struct FileContentSegment));
	goto mid;
	while (1){
		fcs.walkSegment->next=cosmic_malloc(sizeof(struct FileContentSegment));
		fcs.walkSegment=fcs.walkSegment->next;
		mid:;
		fcs.walkSegment->next=NULL;
		fcs.walkSegment->position=0;
		uint32_t r;
		r=fread(fcs.walkSegment->buffer,1,8192,inputFile);
		fcs.walkSegment->position=r;
		fcs.length+=r;
		if (r!=8192){
			if (feof(inputFile)!=0){
				break;
			} else {
				fclose(inputFile);
				err_10_1_(strMerge3("An unknown error occured when reading input file \"",filePath,"\""));
			}
		}
		if (fcs.length>268435456lu){ // 2**28
			fclose(inputFile);
			err_10_1_(strMerge3("Input file \"",filePath,"\" is too large"));
		}
	}
	if (fclose(inputFile)!=0){
		err_10_1_(strMerge3("fclose on input file \"",filePath,"\" failed"));
	}
	fcs.walkSegment=fcs.startSegment;
	char* result=cosmic_malloc(fcs.length+3u);
	result[0]='\n';
	uint32_t position=1;
	do {
		memcpy(result+position,fcs.walkSegment->buffer,fcs.walkSegment->position);
		position+=fcs.walkSegment->position;
		fcs.walkSegment=fcs.walkSegment->next;
	} while (fcs.walkSegment!=NULL);
	result[position++]='\n';
	result[position++]=0;
	assert(position==fcs.length+3u);
	position--;
	fcs.walkSegment=fcs.startSegment;
	do {
		struct FileContentSegment* next=fcs.walkSegment->next;
		cosmic_free(fcs.walkSegment);
		fcs.walkSegment=next;
	} while (fcs.walkSegment!=NULL);
	bool hasGivenStrangeCharacterWarning=false;
	for (uint32_t i=0;i<position;i++){
		char currentCharacter=result[i];
		if ((currentCharacter<32 & currentCharacter!='\r' & currentCharacter!='\n' & currentCharacter!='\t') | currentCharacter>126){
			result[i]=' ';
			if (!hasGivenStrangeCharacterWarning){
				hasGivenStrangeCharacterWarning=true;
				err_00__1(strMerge3("Strange character(s) found in \"",filePath,"\". Is it really a C source code file?"));
			}
		}
	}
	return result;
}


FILE* safe_fputc_file;
const char* safe_fputc_file_path;

void safe_fputc(int value){
	if (fputc(value,safe_fputc_file)!=value) err_10_1_(strMerge3("Error in writing \'",safe_fputc_file_path,"\'"));
}

void safe_fput_32(uint32_t value){
	safe_fputc((uint8_t)(value>> 0));
	safe_fputc((uint8_t)(value>> 8));
	safe_fputc((uint8_t)(value>>16));
	safe_fputc((uint8_t)(value>>24));
}

void safe_fwrite(const void* array, uint32_t length){
	if (fwrite(array,1,length,safe_fputc_file)!=length) err_10_1_(strMerge3("Error in writing \'",safe_fputc_file_path,"\'"));
}

// destroys global_static_data
void finalOutputFromCompile(const char* filePath){
	if (filePath!=NULL){
		uint32_t totalFunctionAndStaticDataBackendSize=0;
		uint32_t totalFunctionAndStaticDataLength=1;
		uint32_t labelCount=0;
		{
			uint8_t* byteCode;
			enum InstructionTypeID id;
			for (uint32_t i=0;i<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i++){
				totalFunctionAndStaticDataLength+=globalInstructionBuffersOfFunctions.slots[i].allocLen-1u;
				byteCode=globalInstructionBuffersOfFunctions.slots[i].byteCode;
				while ((id=*byteCode)!=0){
					totalFunctionAndStaticDataBackendSize+=backendInstructionSizeFromByteCode(byteCode);
					labelCount+=(id==I_LABL | id==I_JJMP | id==I_FCST);
					byteCode+=getStorageDeltaForInstruction(id);
				}
			}
		}
		{
			InstructionSingle* buffer=global_static_data.buffer;
			uint32_t numberOfSlotsTaken=global_static_data.numberOfSlotsTaken;
			for (uint32_t i=0;i<numberOfSlotsTaken;i++){
				totalFunctionAndStaticDataBackendSize+=backendInstructionSize(buffer+i);
				labelCount+=(buffer[i].id==I_LABL);
			}
		}
		CompressedInstructionBuffer cis_global_static_data=compressInstructionBuffer(&global_static_data);
		totalFunctionAndStaticDataLength+=cis_global_static_data.allocLen;
		destroyInstructionBuffer(&global_static_data);
		uint32_t symbolEntryLength=0;
		{
			struct GlobalVariableEntry* gve;
			for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
				gve=blockFrameArray.globalBlockFrame.globalVariableEntries+i;
				symbolEntryLength+=(bool)(!gve->usedStatic);
			}
		}
		{
			struct GlobalFunctionEntry* gfe;
			for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
				gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries+i;
				symbolEntryLength+=(bool)(!gfe->usedStatic^gfe->usedInline);
			}
		}
		safe_fputc_file_path=filePath;
		safe_fputc_file = fopen(safe_fputc_file_path,"wb");
		if (safe_fputc_file==NULL) err_10_1_(strMerge3("Could not open \'",safe_fputc_file_path,"\' for output"));
		safe_fput_32(totalFunctionAndStaticDataBackendSize);
		safe_fput_32(totalFunctionAndStaticDataLength);
		safe_fput_32(labelCount);
		safe_fput_32(symbolEntryLength);
		for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
			struct GlobalVariableEntry gve=blockFrameArray.globalBlockFrame.globalVariableEntries[i];
			if (!gve.usedStatic){
				safe_fput_32(gve.labelID);
				const char* firstSpace=strchr(gve.typeString,' ');
				assert(firstSpace!=NULL);
				safe_fwrite(gve.typeString,firstSpace - gve.typeString);
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
				safe_fput_32(gfe.labelID);
				const char* firstSpace=strchr(gfe.typeString,' ');
				assert(firstSpace!=NULL);
				safe_fwrite(gfe.typeString,firstSpace - gfe.typeString);
				safe_fputc(gfe.usedExtern+4);
				/*
				4- no extern (and therefore   defined)
				5-yes extern (and therefore undefined)
				*/
			}
		}
		for (uint32_t i=0;i<globalInstructionBuffersOfFunctions.numberOfSlotsTaken;i++){
			CompressedInstructionBuffer cib=globalInstructionBuffersOfFunctions.slots[i];
			safe_fwrite(cib.byteCode,cib.allocLen-1u);
		}
		safe_fputc(0);
		safe_fwrite(cis_global_static_data.byteCode,cis_global_static_data.allocLen);
		// null terminator for static data is already placed from the safe_fwrite above
		if (fclose(safe_fputc_file)!=0) err_10_1_(strMerge3("Could not close \'",safe_fputc_file_path,"\' after writing"));
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
	uint32_t* labelMarkoff=cosmic_malloc((countToCheck+5)*sizeof(uint32_t));
	labelMarkoff[0]=0x00000015;
	labelMarkoff[1]=0x00000016;
	labelMarkoff[2]=0x00000017;
	labelMarkoff[3]=0x00000018;
	labelMarkoff[4]=0x00000019;
	uint32_t walkingIndex=5;
	for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
		struct GlobalFunctionEntry gfe=blockFrameArray.globalBlockFrame.globalFunctionEntries[i];
		if (!gfe.usedExtern&!gfe.hasBeenDefined) labelMarkoff[walkingIndex++]=gfe.labelID;
	}
	doLabelMarkoffInAllCompressedInstructionBuffers(labelMarkoff,countToCheck+5);
	doLabelMarkoffInInstructionBuffer(&global_static_data,labelMarkoff,countToCheck+5);
	bool gaveError=false;
	for (uint32_t i=5;i<countToCheck;i++){
		if (labelMarkoff[i]==0){
			gaveError=true;
			walkingIndex=5;
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
	if (!gaveError){
		InstructionBuffer ib;
		if (labelMarkoff[0]==0){
			initInstructionBuffer(&ib);
			singleMergeIB(&ib,&ib_intrinsic_back_memmove_forward_unaligned);
			addEntryToInstructionBuffersOfFunctions(&ib);
		}
		if (labelMarkoff[1]==0){
			initInstructionBuffer(&ib);
			singleMergeIB(&ib,&ib_intrinsic_back_memmove_backward_unaligned);
			addEntryToInstructionBuffersOfFunctions(&ib);
		}
		if (labelMarkoff[2]==0){
			initInstructionBuffer(&ib);
			singleMergeIB(&ib,&ib_intrinsic_back_memcpy_aligned);
			addEntryToInstructionBuffersOfFunctions(&ib);
		}
		if (labelMarkoff[3]==0){
			initInstructionBuffer(&ib);
			singleMergeIB(&ib,&ib_intrinsic_back_strlen);
			addEntryToInstructionBuffersOfFunctions(&ib);
		}
		if (labelMarkoff[4]==0){
			initInstructionBuffer(&ib);
			singleMergeIB(&ib,&ib_intrinsic_back_memset);
			addEntryToInstructionBuffersOfFunctions(&ib);
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
									// if the declaration didn't use 'static', then it cannot be verified that this can be removed, it may be used elsewhere at link time
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
	for (uint16_t i=0;i<12;i++){
		binaryFile_noEOF_fgetc();// for now, I am just going to ignore this new data. it is redundant, anyway.
	}
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






