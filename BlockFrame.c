

#include "Common.c"
#include "FunctionParamHelper.c"

//void debugPrintOfBlockFrameArray();

struct TypeMemberEntry{
	const char* typeString; // must have identifier
	// however, when this is a part of an enum entry, the string above is not a type string.
	// instead, it should have an identifier followed by " = " followed by a decimal number
	
	uint32_t offset; // this is the offset from the base of the type that this component is at. It is NOT always word alligned.
};

struct BlockFrameArray{
	uint16_t maxStackSize; // this value is updated on BlockFrame removal (at all indexes) and reset when a BlockFrame is added at index 0
	uint16_t initialStackSize; // this value is updated on BlockFrame removal and is the size of the index 0 blockframe
	
	struct BlockFrameEntry{
		uint16_t addedStackValue;
		
		uint32_t numberOfAllocatedVariableEntries;
		uint32_t numberOfValidVariableEntries;
		struct BlockFrameVariableEntry{
			const char* typeString; // must have identifier
			uint16_t thisSizeof; // because of how this is used, this number shall always be word alligned
			// thisSizeof is not valid if staticLinkbackID!=0
			uint32_t staticLinkbackID; // if 0, this variable is not static. Otherwise, this is the linkback ID that should be used to look up the variable in the global frame
			uint16_t offsetInThisBlockFrame; // memory offset. does not include the other block frames below it. Always word alligned.
			bool isRegister; // this is if the keyword is specified. Which I generally ignore, by the way.
		} *variableEntries;
		
		uint32_t numberOfTypeEntries;
		struct BlockFrameTypeEntry{
			uint8_t typeOfThis;
			/*
			typeOfType == 0 is struct
			typeOfType == 1 is union
			typeOfType == 2 is enum		
			*/
			const char* name;
			struct TypeMemberEntry *arrayOfMemberEntries;
			uint16_t numberOfMemberEntries; // should not be 0
			uint32_t thisSizeof; // because types that are keywords are not resolved here and struct/union total sizes are always word alligned, this is the only value that is needed here
		} *typeEntries; // typeEntries does not include typedef entries, those are seperate
		
	} *entries;
	uint32_t numberOfAllocatedSlots;
	uint32_t numberOfValidSlots;
	
	struct GlobalBlockFrame{
		uint32_t sizeOfGlobalVariables; // this is a memory size, and will always be word-aligned
		
		uint32_t numberOfAllocatedGlobalVariableEntrySlots;
		uint32_t numberOfValidGlobalVariableEntrySlots;
		struct GlobalVariableEntry{
			const char* typeString; // must have identifier
			uint32_t labelID; // number given when this entry is created that is able to identify the start location in data section of assembly output
			uint32_t staticLinkbackID; // if 0, then this is not a linkback entry. Otherwise, this value is the one used to access this variable, and it cannot be recognized by the identifier
			uint32_t thisSizeof; // because of how this is used, this number shall always be word alligned
			uint32_t indexOfDeclaration;
			bool usedStatic;
			bool usedExtern;
			bool hadInitializer;
			bool isCurrentlyTentative;
		} *globalVariableEntries;
		
		uint32_t numberOfAllocatedGlobalTypeEntrySlots;
		uint32_t numberOfValidGlobalTypeEntrySlots;
		struct GlobalTypeEntry{
			uint8_t typeOfThis;
			/*
			typeOfType == 0 is struct
			typeOfType == 1 is union
			typeOfType == 2 is enum		
			*/
			const char* name;
			struct TypeMemberEntry *arrayOfMemberEntries;
			uint16_t numberOfMemberEntries; // should not be 0
			uint32_t thisSizeof; // because types that are keywords are not resolved here and struct/union total sizes are always word alligned, this is the only value that is needed here
		} *globalTypeEntries;
		
		uint32_t numberOfAllocatedGlobalFunctionEntrySlots;
		uint32_t numberOfValidGlobalFunctionEntrySlots;
		struct GlobalFunctionEntry{
			const char* typeString; // must have identifier
			struct FunctionTypeAnalysis functionTypeAnalysis; // this is the version without singular void parameter and 
			uint32_t labelID; // number given when this entry is created that is able to identify the start location in assembly output
			uint32_t indexOfDeclaration; // is one of the prototypes if !hasBeenDefined, otherwise it is the definition location
			bool usedStatic;
			bool usedExtern;
			bool usedInline;
			bool hasBeenDefined;
			bool usesVaArgs;
		} *globalFunctionEntries;
	} globalBlockFrame;
} blockFrameArray;



// this number is used to make unique numbers for assembly labels (one is added directly prior to each use)
// it starts at 4 because labels 0,1,2,3,4 are reserved for several things
uint32_t globalLabelID = 4;



// it returns the block index the new BlockFrame was added at, because sometimes that is important
uint32_t addBlockFrame(){
	if (blockFrameArray.numberOfValidSlots==0){
		blockFrameArray.maxStackSize=0;
	}
	if (blockFrameArray.numberOfValidSlots>=blockFrameArray.numberOfAllocatedSlots){
		if (blockFrameArray.numberOfAllocatedSlots==0){
			blockFrameArray.numberOfAllocatedSlots = 10;
			blockFrameArray.entries = cosmic_malloc(10*sizeof(struct BlockFrameEntry));
		} else {
			blockFrameArray.numberOfAllocatedSlots += 10;
			blockFrameArray.entries = cosmic_realloc(blockFrameArray.entries,blockFrameArray.numberOfAllocatedSlots*sizeof(struct BlockFrameEntry));
		}
	}
	uint32_t thisBlockIndex = blockFrameArray.numberOfValidSlots++;
	memZero(&(blockFrameArray.entries[thisBlockIndex]),sizeof(struct BlockFrameEntry));
	return thisBlockIndex;
}

void removeBlockFrame(){
	if (blockFrameArray.numberOfValidSlots==0){
		printf("Internal error: No BlockFrame to pop.\n");
		exit(1);
	}
	{
		// this section updates blockFrameArray.maxStackSize and blockFrameArray.initialStackSize
		blockFrameArray.initialStackSize=blockFrameArray.entries[0].addedStackValue;
		uint32_t totalStackSize=0;
		
		for (uint32_t i=0;i<blockFrameArray.numberOfValidSlots;i++){
			totalStackSize+=blockFrameArray.entries[i].addedStackValue;
		}
		if (totalStackSize>64000){
			printf("Maxiumum stack size exceeded (size of local variables too close to size of stack pointer)\n");
			exit(1);
		}
		if (totalStackSize>blockFrameArray.maxStackSize){
			blockFrameArray.maxStackSize = totalStackSize;
		}
	}
	uint32_t thisBlockIndex = --blockFrameArray.numberOfValidSlots;
	struct BlockFrameEntry* blockFrameEntryPtr = &(blockFrameArray.entries[thisBlockIndex]);
	if (blockFrameEntryPtr->numberOfAllocatedVariableEntries!=0){
		for (uint32_t i=0;i<blockFrameEntryPtr->numberOfValidVariableEntries;i++){
			cosmic_free((char*)blockFrameEntryPtr->variableEntries[i].typeString);
		}
		cosmic_free(blockFrameEntryPtr->variableEntries);
	}
	if (blockFrameEntryPtr->numberOfTypeEntries!=0){
		for (uint32_t i=0;i<blockFrameEntryPtr->numberOfTypeEntries;i++){
			for (uint16_t j=0;j<blockFrameEntryPtr->typeEntries[i].numberOfMemberEntries;j++){
				cosmic_free((char*)blockFrameEntryPtr->typeEntries[i].arrayOfMemberEntries[j].typeString);
			}
			cosmic_free(blockFrameEntryPtr->typeEntries[i].arrayOfMemberEntries);
			cosmic_free((char*)blockFrameEntryPtr->typeEntries[i].name);
		}
		cosmic_free(blockFrameEntryPtr->typeEntries);
	}
}



// this next struct is used when searching for variables, functions, types, and type components through an identifier


struct IdentifierSearchResult{
	bool didExist;
	enum IdentifierSearchResultEnum{
		IdentifierSearchResultIsVariable,
		IdentifierSearchResultIsTypeMember,
		IdentifierSearchResultIsFunction
	} typeOfResult;
	union {
		struct VariableReference{
			uint32_t blockFrameEntryIndex; // if isGlobal==true, this value is not used
			uint32_t variableEntryIndex;
			bool isGlobal;
		} variableReference;
		
		struct TypeMemberReference{ // needed for enum values
			uint32_t blockFrameEntryIndex; // if isGlobal==true, this value is not used
			uint32_t typeEntryIndex;
			uint32_t memberEntryIndex;
			bool isGlobal;
		} typeMemberReference;
		
		struct FunctionReference{
			uint32_t functionEntryIndex;
		} functionReference;
		
	} reference;
};



bool areIdentifierStringsEquivalent(const char* string1,const char* string2){
	uint32_t i=0;
	do {
		char c1 = string1[i];
		char c2 = string2[i];
		bool terminate1 = (c1==' ') | (c1==0);
		bool terminate2 = (c2==' ') | (c2==0);
		if (terminate1 & terminate2){
			return true;
		}
		if (terminate1 | terminate2 | (c1!=c2)){
			return false;
		}
		i++;
	} while (true);
}



/*
for the string "identifier", a type string with an identifier may be used. Termination will occur at a space or null.
searchGlobal==false will make this function not search global variables, types, and functions.
searchGlobal==true will make this function search local and global variables, types, and functions.
however, even if searchGlobal==false, the "isGlobal" boolean will be true if the variable inside a BlockFrame is a static variable.
*/
void searchForIdentifier(struct IdentifierSearchResult *isr,const char* identifier, bool searchGlobal, bool searchLocal, bool searchVariables, bool searchTypes, bool searchFunctions){
	if (searchLocal){
		// the order BlockFrameEntries are searched in does matter (for block overriden identifiers), it should be from end to start
		uint32_t i=blockFrameArray.numberOfValidSlots;
		while (i--!=0){
			struct BlockFrameEntry* tempBlockFrameEntryPtr = &(blockFrameArray.entries[i]);
			if (searchVariables){
				for (uint32_t j=0;j<tempBlockFrameEntryPtr->numberOfValidVariableEntries;j++){
					struct BlockFrameVariableEntry* tempBlockFrameVariableEntryPtr = &(tempBlockFrameEntryPtr->variableEntries[j]);
					if (areIdentifierStringsEquivalent(tempBlockFrameVariableEntryPtr->typeString,identifier)){
						if (tempBlockFrameVariableEntryPtr->staticLinkbackID==0){
							isr->didExist = true;
							isr->typeOfResult = IdentifierSearchResultIsVariable;
							isr->reference.variableReference.blockFrameEntryIndex = i;
							isr->reference.variableReference.variableEntryIndex = j;
							isr->reference.variableReference.isGlobal = false;
							return;
						} else {
							for (uint32_t k=0;k<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;k++){
								struct GlobalVariableEntry* tempGlobalVariableEntryPtr = &(blockFrameArray.globalBlockFrame.globalVariableEntries[k]);
								if (tempGlobalVariableEntryPtr->staticLinkbackID==tempBlockFrameVariableEntryPtr->staticLinkbackID){
									isr->didExist = true;
									isr->typeOfResult = IdentifierSearchResultIsVariable;
									isr->reference.variableReference.variableEntryIndex = k;
									isr->reference.variableReference.isGlobal = true;
									return;
								}
							}
							printf("Internal Error: static linkback failed\n");
							exit(1);
						}
					}
				}
			}
			if (searchTypes){
				for (uint32_t j=0;j<tempBlockFrameEntryPtr->numberOfTypeEntries;j++){
					struct BlockFrameTypeEntry* tempBlockFrameTypeEntryPtr = &(tempBlockFrameEntryPtr->typeEntries[j]);
					if (tempBlockFrameTypeEntryPtr->typeOfThis==2){
						// enum component search
						for (uint16_t k=0;k<tempBlockFrameTypeEntryPtr->numberOfMemberEntries;k++){
							struct TypeMemberEntry* tempTypeMemberEntryPtr = &(tempBlockFrameTypeEntryPtr->arrayOfMemberEntries[k]);
							if (areIdentifierStringsEquivalent(tempTypeMemberEntryPtr->typeString,identifier)){
								isr->didExist = true;
								isr->typeOfResult = IdentifierSearchResultIsTypeMember;
								isr->reference.typeMemberReference.blockFrameEntryIndex = i;
								isr->reference.typeMemberReference.typeEntryIndex = j;
								isr->reference.typeMemberReference.memberEntryIndex = k;
								isr->reference.typeMemberReference.isGlobal = false;
								return;
							}
						}
					}
				}
			}
		}
	}
	
	if (searchGlobal){
		if (searchVariables){
			for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;i++){
				struct GlobalVariableEntry* tempGlobalVariableEntryPtr = &(blockFrameArray.globalBlockFrame.globalVariableEntries[i]);
				if (tempGlobalVariableEntryPtr->staticLinkbackID==0 && areIdentifierStringsEquivalent(tempGlobalVariableEntryPtr->typeString,identifier)){
					isr->didExist = true;
					isr->typeOfResult = IdentifierSearchResultIsVariable;
					isr->reference.variableReference.variableEntryIndex = i;
					isr->reference.variableReference.isGlobal = true;
					return;
				}
			}
		}
		if (searchTypes){
			for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots;i++){
				struct GlobalTypeEntry* tempGlobalTypeEntryPtr = &(blockFrameArray.globalBlockFrame.globalTypeEntries[i]);
				if (tempGlobalTypeEntryPtr->typeOfThis==2){
					// enum component search
					for (uint16_t k=0;k<tempGlobalTypeEntryPtr->numberOfMemberEntries;k++){
						struct TypeMemberEntry* tempTypeMemberEntryPtr = &(tempGlobalTypeEntryPtr->arrayOfMemberEntries[k]);
						if (areIdentifierStringsEquivalent(tempTypeMemberEntryPtr->typeString,identifier)){
							isr->didExist = true;
							isr->typeOfResult = IdentifierSearchResultIsTypeMember;
							isr->reference.typeMemberReference.typeEntryIndex = i;
							isr->reference.typeMemberReference.memberEntryIndex = k;
							isr->reference.typeMemberReference.isGlobal = true;
							return;
						}
					}
				}
			}
		}
		if (searchFunctions){
			for (uint32_t i=0;i<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;i++){
				struct GlobalFunctionEntry* tempGlobalFunctionEntryPtr = &(blockFrameArray.globalBlockFrame.globalFunctionEntries[i]);
				if (areIdentifierStringsEquivalent(tempGlobalFunctionEntryPtr->typeString,identifier)){
					isr->didExist = true;
					isr->typeOfResult = IdentifierSearchResultIsFunction;
					isr->reference.functionReference.functionEntryIndex = i;
					return;
				}
			}
		}
	}
	isr->didExist = false;
	return;
}



// inside the variableReference, isGlobal must not be true
uint16_t getReversedOffsetForLocalVariable(struct VariableReference* variableReference){
	uint16_t totalOffset = 0;
	for (uint32_t i=0;i<variableReference->blockFrameEntryIndex;i++){
		totalOffset += blockFrameArray.entries[i].addedStackValue;
	}
	return totalOffset + blockFrameArray.entries[variableReference->blockFrameEntryIndex]
		.variableEntries[variableReference->variableEntryIndex].offsetInThisBlockFrame;
}





/*
typeString should have types broken down and typedefs resolved prior to calling this function

typically, staticLinkbackID==0
typeString should have an identifier
typeString is copied in this function

return value description:
 0 - global variable added without problems (can also mean that it wasn't added but was a permissable redeclaration after a tentative declaration)
 1 - identifier collision exists with non-variable
 2 - identifier collision exists with variable
*/
uint8_t addGlobalVariable(
		const char* typeString,
		uint32_t staticLinkbackID,
		uint32_t labelID,
		uint32_t indexOfDeclaration,
		bool usedStatic,
		bool usedExtern,
		bool hadInitializer){

	bool doesIdentifierAlreadyExist;
	uint32_t indexOfEntryIfIdentifierExists;
	{
		// identifier collisions are weird at the global level...
		struct IdentifierSearchResult isr;
		searchForIdentifier(&isr,typeString,true,false,false,true,true);
		if (isr.didExist){
			return 1; // identifier collision exists with non-variable
		}
		searchForIdentifier(&isr,typeString,true,false,true,false,false);
		doesIdentifierAlreadyExist = isr.didExist;
		if (isr.didExist){
			indexOfEntryIfIdentifierExists = isr.reference.variableReference.variableEntryIndex;
			// TODO: support tentative declarations. This should not always fail
			printf("Warning: global variable was attempted to be redefined, and I do not currently support tentative declarations.\n");
			return 2; // identifier collision exists with variable
		}
		// getting here means that the variable has no identifier collisions
	}
	struct GlobalVariableEntry* globalVariableEntryPtr;
	if (doesIdentifierAlreadyExist){
		globalVariableEntryPtr = &(blockFrameArray.globalBlockFrame.globalVariableEntries[indexOfEntryIfIdentifierExists]);
		
		// currently, this is not possible because tentative declarations are not supported.
		// TODO
	} else {
		uint32_t thisSizeof = getSizeofForTypeString(typeString,false);
		if (thisSizeof==0){
			printf("sizeof failed while adding variable to global frame\n");
			exit(1);
		}
		thisSizeof+=thisSizeof&1; // word alligned sizeof
		
		if (blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots>=blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalVariableEntrySlots){
			blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalVariableEntrySlots+=50;
			blockFrameArray.globalBlockFrame.globalVariableEntries = cosmic_realloc(blockFrameArray.globalBlockFrame.globalVariableEntries,
					blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalVariableEntrySlots*sizeof(struct GlobalVariableEntry));
		}
		globalVariableEntryPtr = &(blockFrameArray.globalBlockFrame.globalVariableEntries[blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots++]);
		
		globalVariableEntryPtr->usedStatic = usedStatic;
		globalVariableEntryPtr->usedExtern = usedExtern;
		globalVariableEntryPtr->typeString = copyStringToHeapString(typeString);
		globalVariableEntryPtr->staticLinkbackID = staticLinkbackID;
		globalVariableEntryPtr->labelID = labelID;
		globalVariableEntryPtr->thisSizeof = thisSizeof;
		globalVariableEntryPtr->hadInitializer=hadInitializer;
		globalVariableEntryPtr->indexOfDeclaration=indexOfDeclaration;
		globalVariableEntryPtr->isCurrentlyTentative = false; // this will change when tentative declarations become supported
	}
	return 0;
}



/*
typeString should have types broken down and typedefs resolved prior to calling this function
typeString should have had function decomposition already
typeString should have an identifier

return value description:
 0 - function was added (or already existed and was the same)
 1 - function prototype conflict
 2 - function was already defined (cannot be defined again)
 3 - identifier conflict with non-function
 4 - is being defined but had a missing identifier on a parameter
 5 - extern functions cannot be given a definition
*/
uint8_t addGlobalFunction(
		const char* typeString,
		uint32_t indexOfDeclaration,
		bool isDefinitionBeingGiven,
		bool usedStatic,
		bool usedExtern,
		bool usedInline){
	
	if (usedExtern&isDefinitionBeingGiven) return 5;
	if (usedExtern&usedInline){
		usedInline=false;
		err_010_0("functions declared 'extern' cannot be inlined. The 'inline' keyword is being ignored for this declaration",indexOfDeclaration);
	}
	bool hasPreviousPrototype;
	bool wasPreviouslyDefinedSoDontChange = false;
	uint32_t indexOfPreviousPrototype;
	struct FunctionTypeAnalysis functionTypeAnalysis_this;
	analyseFunctionTypeString(&functionTypeAnalysis_this,typeString,false);
	if (isDefinitionBeingGiven){
		for (uint16_t i=0;i<functionTypeAnalysis_this.numberOfParameters;i++){
			if (!functionTypeAnalysis_this.params[i].hasIdentifier && !doStringsMatch(functionTypeAnalysis_this.params[i].typeString,"...")){
				if (!(doStringsMatch(functionTypeAnalysis_this.params[i].typeString,"void") & functionTypeAnalysis_this.numberOfParameters==1)){
					destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
					return 4;
				}
			}
		}
	}
	// this checks for function prototype conflicts
	struct IdentifierSearchResult isr;
	searchForIdentifier(&isr,typeString,true,false,true,true,false);
	if (isr.didExist){
		destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
		return 3; // identifier conflict with non-function
	}
	searchForIdentifier(&isr,typeString,true,false,false,false,true);
	hasPreviousPrototype = isr.didExist;
	if (isr.didExist){
		indexOfPreviousPrototype = isr.reference.functionReference.functionEntryIndex;
		struct GlobalFunctionEntry* globalFunctionEntryPtr = &(blockFrameArray.globalBlockFrame.globalFunctionEntries[indexOfPreviousPrototype]);
		if (globalFunctionEntryPtr->usedStatic!=usedStatic |
			globalFunctionEntryPtr->usedExtern!=usedExtern |
			globalFunctionEntryPtr->usedInline!=usedInline){
			
			return 1; // storage class specifier conflict
		}
		if (globalFunctionEntryPtr->hasBeenDefined){
			if (isDefinitionBeingGiven){
				destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
				return 2; // function was already defined
			} else {
				wasPreviouslyDefinedSoDontChange = true;
			}
		}
		struct FunctionTypeAnalysis functionTypeAnalysis_previous;
		analyseFunctionTypeString(&functionTypeAnalysis_previous,globalFunctionEntryPtr->typeString,false);
		if (functionTypeAnalysis_this.numberOfParameters!=functionTypeAnalysis_previous.numberOfParameters){
			destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
			destroyFunctionTypeAnalysis(&functionTypeAnalysis_previous);
			return 1;
		}
		for (uint16_t i=0;i<functionTypeAnalysis_this.numberOfParameters;i++){
			if (!doStringsMatch(functionTypeAnalysis_this.params[i].noIdentifierTypeString,functionTypeAnalysis_previous.params[i].noIdentifierTypeString)){
				destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
				destroyFunctionTypeAnalysis(&functionTypeAnalysis_previous);
				return 1;
			}
		}
		if (!doStringsMatch(functionTypeAnalysis_this.returnType,functionTypeAnalysis_previous.returnType)){
			destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
			destroyFunctionTypeAnalysis(&functionTypeAnalysis_previous);
			return 1;
		}
		bool previousUsesVaArgs = false;
		for (uint16_t i=0;i<functionTypeAnalysis_this.numberOfParameters;i++){
			if (previousUsesVaArgs){
				return 1; // not exactly what the error is, it is actually that vaArg needs to be the last argument
			}
			if (doStringsMatch(functionTypeAnalysis_this.params[i].noIdentifierTypeString,"...")){
				if (functionTypeAnalysis_this.params[i].hasIdentifier){
					return 1; // not exactly what the error is, it is actually just that "..." cannot have an identifier
				}
				previousUsesVaArgs = true;
			}
		}
		destroyFunctionTypeAnalysis(&functionTypeAnalysis_previous);
	}
	bool usesVaArgs = false;
	for (uint16_t i=0;i<functionTypeAnalysis_this.numberOfParameters;i++){
		if (usesVaArgs){
			return 1; // not actually what the error is, it is actally that vaArg needs to be the last argument
		}
		if (doStringsMatch(functionTypeAnalysis_this.params[i].noIdentifierTypeString,"...")){
			if (functionTypeAnalysis_this.params[i].hasIdentifier){
				return 1; // not actually what the error is, it is actually just that "..." cannot have an identifier
			}
			usesVaArgs = true;
		}
	}
	destroyFunctionTypeAnalysis(&functionTypeAnalysis_this);
	
	// now we know that the function does not conflict
	if (!wasPreviouslyDefinedSoDontChange){
		uint32_t indexOfCurrentEntry;
		if (hasPreviousPrototype){
			indexOfCurrentEntry = indexOfPreviousPrototype;
		} else {
			if (blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots>=blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalFunctionEntrySlots){
				blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalFunctionEntrySlots+=50;
				blockFrameArray.globalBlockFrame.globalFunctionEntries = cosmic_realloc(blockFrameArray.globalBlockFrame.globalFunctionEntries,
						blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalFunctionEntrySlots*sizeof(struct GlobalFunctionEntry));
			}
			indexOfCurrentEntry = blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots++;
		}
		struct GlobalFunctionEntry* globalFunctionEntryPtr = &(blockFrameArray.globalBlockFrame.globalFunctionEntries[indexOfCurrentEntry]);
		globalFunctionEntryPtr->hasBeenDefined = isDefinitionBeingGiven;
		if (isDefinitionBeingGiven & hasPreviousPrototype){
			cosmic_free((char*)globalFunctionEntryPtr->typeString);
			globalFunctionEntryPtr->typeString = copyStringToHeapString(typeString);
		}
		if (hasPreviousPrototype){
			destroyFunctionTypeAnalysis(&(globalFunctionEntryPtr->functionTypeAnalysis));
		} else {
			globalFunctionEntryPtr->typeString = copyStringToHeapString(typeString);
			globalFunctionEntryPtr->labelID = ++globalLabelID;
			globalFunctionEntryPtr->usesVaArgs = usesVaArgs;
		}
		globalFunctionEntryPtr->usedStatic=usedStatic;
		globalFunctionEntryPtr->usedExtern=usedExtern;
		globalFunctionEntryPtr->usedInline=usedInline;
		globalFunctionEntryPtr->indexOfDeclaration=indexOfDeclaration;
		analyseFunctionTypeString(&(globalFunctionEntryPtr->functionTypeAnalysis),typeString,true);
	}
	return 0; // function entry is valid
}




/*
typeString should have types broken down and typedefs resolved prior to calling this function

typeString should have an identifier
typeString is copied in this function
this should not be used for global variables
isRegister and isStatic refer to if the respective keywords were specified
*/
void addVariableToBlockFrame(
		const char* typeString,
		uint32_t indexOfDeclaration,
		bool isRegister,
		bool isStatic){

	int32_t indexOfFirstSpace = getIndexOfFirstSpaceInString(typeString);
	if (indexOfFirstSpace==-1){
		printf("Internal error: Cannot add non-identifier to BlockFrameEntry.variableEntries\n");
		exit(1);
	}
	if (typeString[1+indexOfFirstSpace]=='('){
		printInformativeMessageAtSourceContainerIndex(true,"Function declaration is not allowed here",indexOfDeclaration,0);
		exit(1);
	}
	assert(!(isRegister & isStatic));
	{
		// global identifiers are not checked (when the search for an identifier happens, local variables are searched first, and local may override global)
		struct IdentifierSearchResult isr;
		searchForIdentifier(&isr,typeString,false,true,true,true,false);
		if (isr.didExist){
			bool isAcceptable=false;
			// isr could be global because of static
			if (isr.typeOfResult==IdentifierSearchResultIsVariable){
				isAcceptable=isr.reference.variableReference.isGlobal || isr.reference.variableReference.blockFrameEntryIndex!=blockFrameArray.numberOfValidSlots-1;
			} else {
				isAcceptable=isr.reference.typeMemberReference.isGlobal || isr.reference.typeMemberReference. blockFrameEntryIndex!=blockFrameArray.numberOfValidSlots-1;
			}
			if (isAcceptable){
printInformativeMessageAtSourceContainerIndex(false,
	"Identical identifier was already declared in a previous scope.\n  The previous declaration will be overriden while this variable is in scope",indexOfDeclaration,0);
			} else {
printInformativeMessageAtSourceContainerIndex(true,
	"Identical identifier was already declared in the same scope",indexOfDeclaration,0);
exit(1);
			}
		}
		// getting here means that the variable has no identifier collisions
	}
	uint32_t thisSizeof = getSizeofForTypeString(typeString,false);
	
	struct BlockFrameEntry* blockFrameEntryPtr = &(blockFrameArray.entries[blockFrameArray.numberOfValidSlots-1]);
	if (blockFrameEntryPtr->numberOfValidVariableEntries>=blockFrameEntryPtr->numberOfAllocatedVariableEntries){
		blockFrameEntryPtr->numberOfAllocatedVariableEntries += 10;
		blockFrameEntryPtr->variableEntries = cosmic_realloc(
			blockFrameEntryPtr->variableEntries,
			blockFrameEntryPtr->numberOfAllocatedVariableEntries*sizeof(struct BlockFrameVariableEntry));
	}
	struct BlockFrameVariableEntry* blockFrameVariableEntryPtr = &(blockFrameEntryPtr->variableEntries[blockFrameEntryPtr->numberOfValidVariableEntries++]);
	
	blockFrameVariableEntryPtr->isRegister = isRegister;
	blockFrameVariableEntryPtr->typeString = copyStringToHeapString(typeString);
	
	if (thisSizeof==0){
		printf("sizeof failed while adding variable `%s` to block frame\n",typeString);
		exit(1);
	}
	thisSizeof += thisSizeof&1; // word align by adding padding if the size isn't word aligned
	static uint32_t staticLinkbackID;
	if (isStatic){
		blockFrameArray.globalBlockFrame.sizeOfGlobalVariables += thisSizeof;
		uint32_t thisStaticID = ++staticLinkbackID;
		blockFrameVariableEntryPtr->staticLinkbackID = thisStaticID;
		
		// the variable is now sufficently set up to call this function with no problems
		addGlobalVariable(typeString,thisStaticID,++globalLabelID,indexOfDeclaration,true,false,true); 
		// the global entry is added in addition to the auto scope entry.
		// Also, because thisStaticID!=0, the global and local entry can't be accessed normally
		// hasInitializer is true because then it will behave well with tentative declaration logic
	} else {
		if ((((uint32_t)blockFrameEntryPtr->addedStackValue)+((uint32_t)thisSizeof))>64000){
			printf("size of that type is too large for the stack\n");
			exit(1);
		}
		blockFrameEntryPtr->addedStackValue += (uint16_t)thisSizeof;
		blockFrameVariableEntryPtr->offsetInThisBlockFrame = blockFrameEntryPtr->addedStackValue;
		blockFrameVariableEntryPtr->staticLinkbackID = 0;
	}
	blockFrameVariableEntryPtr->thisSizeof = (uint16_t)thisSizeof;
}







struct TypeSearchResult{
	uint32_t blockFrameEntryIndex; // if isGlobal==true, this value is not used
	uint32_t typeEntryIndex;
	bool didExist;
	bool isGlobal;
};




/*
"name" should be null terminated with no spaces
"type" is as follows:
0 - struct
1 - union
2 - enum
*/
void searchForType(struct TypeSearchResult* typeSearchResult,const char* name, uint8_t type){
	uint32_t j=blockFrameArray.numberOfValidSlots;
	while (j--!=0){
		// the order BlockFrameEntries are searched in does matter (for later when inlining functions becomes a thing), it should be last to first
		struct BlockFrameEntry* tempBlockFrameEntryPtr = &(blockFrameArray.entries[j]);
		for (uint32_t k=0;k<tempBlockFrameEntryPtr->numberOfTypeEntries;k++){
			struct BlockFrameTypeEntry* tempBlockFrameTypeEntryPtr = &(tempBlockFrameEntryPtr->typeEntries[k]);
			if (tempBlockFrameTypeEntryPtr->typeOfThis==type && doStringsMatch(name,tempBlockFrameTypeEntryPtr->name)){
				typeSearchResult->blockFrameEntryIndex = j;
				typeSearchResult->typeEntryIndex = k;
				typeSearchResult->didExist = true;
				typeSearchResult->isGlobal = false;
				return;
			}
		}
	}
	for (j=0;j<blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots;j++){
		struct GlobalTypeEntry* tempGlobalTypeEntryPtr = &(blockFrameArray.globalBlockFrame.globalTypeEntries[j]);
		if (tempGlobalTypeEntryPtr->typeOfThis==type && doStringsMatch(name,tempGlobalTypeEntryPtr->name)){
			typeSearchResult->typeEntryIndex = j;
			typeSearchResult->didExist = true;
			typeSearchResult->isGlobal = true;
			return;
		}
	}
	typeSearchResult->didExist = false;
}



/*
returns the TypeComponentEntry pointer
typeString should not have an identifier, and should start with "struct" or "union" followed by the name (and nothing else)
componentName should not have any spaces in it

the return value is NOT always word alligned
*/
struct TypeMemberEntry* getOffsetAndTypeStringOfMemberInType(const char* typeString,const char* componentName){
	assert(getIndexOfNthSpace(typeString,1)==-1);
	int32_t indexOfFirstSpaceInTypeString = getIndexOfNthSpace(typeString,0);
	uint8_t typeNumber;
	if (isSectionOfStringEquivalent(typeString,0,"struct ")){
		typeNumber = 0;
	} else if (isSectionOfStringEquivalent(typeString,0,"union ")){
		typeNumber = 1;
	} else {
		assert(false);
	}
	struct TypeSearchResult typeSearchResult;
	searchForType(&typeSearchResult,typeString+(indexOfFirstSpaceInTypeString+1),typeNumber);
	assert(typeSearchResult.didExist);
	if (typeSearchResult.isGlobal){
		struct GlobalTypeEntry* globalTypeEntryPtr = 
			&(blockFrameArray.globalBlockFrame
			.globalTypeEntries[typeSearchResult.typeEntryIndex]);
		for (uint16_t i=0;i<globalTypeEntryPtr->numberOfMemberEntries;i++){
			if (areIdentifierStringsEquivalent(
					globalTypeEntryPtr->arrayOfMemberEntries[i].typeString,componentName)){
				return &(globalTypeEntryPtr->arrayOfMemberEntries[i]);
			}
		}
	} else {
		struct BlockFrameTypeEntry* blockFrameTypeEntryPtr = 
			&(blockFrameArray.entries[typeSearchResult.blockFrameEntryIndex]
			.typeEntries[typeSearchResult.typeEntryIndex]);
		for (uint16_t i=0;i<blockFrameTypeEntryPtr->numberOfMemberEntries;i++){
			if (areIdentifierStringsEquivalent(
					blockFrameTypeEntryPtr->arrayOfMemberEntries[i].typeString,componentName)){
				return &(blockFrameTypeEntryPtr->arrayOfMemberEntries[i]);
			}
		}
	}
	return NULL;
}



/*
Not needed to be called outside of breakDownTypeAndAdd() and fullTypeParseAvoidAdd()

input typeString is not modified
input typeString should have at least one segment (no zero length strings or strings with only spaces)
input typeString doesn't actually need to have any typedefs that need to be substituted

returns a string with all typedefs substituted in. 
If a new string was allocated, the boolean at the given pointer is set to true.
If a new string was not allocated (therefore the return value is the same string as the input), the boolean at the given pointer is set to false.
*/
char* resolveTypdefsInTypeString(char* typeString, bool* didAllocateNewString){
	int32_t mainStart = 0;
	bool didFindOne;
	bool didFindOneYet = false;
	do {
		didFindOne=false;
		if (typeString[mainStart]==0){
			break;
		}
		uint16_t numberOfSegments = 1;
		int32_t lengthOfInput = 0;
		{
			int32_t i;
			for (i=mainStart;typeString[i];i++){
				if (typeString[i]==' '){
					numberOfSegments++;
				}
			}
			lengthOfInput = i;
		}
		bool wasPreviousTypeIndicator = false;
		for (uint16_t segNum = 0;segNum<numberOfSegments;segNum++){
			int32_t startIndex;
			int32_t endIndex;
			if (segNum!=0){
				startIndex = getIndexOfNthSpace(typeString+mainStart,segNum-1)+1+mainStart;
			} else {
				startIndex = mainStart;
			}
			if (segNum+1==numberOfSegments){
				endIndex = lengthOfInput;
			} else {
				endIndex = getIndexOfNthSpace(typeString+mainStart,segNum)+mainStart;
			}
			if (startIndex==-1 | endIndex==-1){
				printf("Internal Error:typedef substitution segment error\n");
				exit(1);
			}
			// startIndex and endIndex should never be -1 . Does it need to be checked?
			if (
			specificStringEqualCheck(typeString,startIndex,endIndex,"enum") || 
			specificStringEqualCheck(typeString,startIndex,endIndex,"struct") || 
			specificStringEqualCheck(typeString,startIndex,endIndex,"union")){
				wasPreviousTypeIndicator = true;
			} else if (!wasPreviousTypeIndicator){
				int32_t typedefEntryIndex = indexOfTypedefEntryForSectionOfString(typeString,startIndex,endIndex);
				if (typedefEntryIndex!=-1){
					struct TypedefEntry* typedefEntryPtr = &(globalTypedefEntries.entries[typedefEntryIndex]);
					int32_t lengthOfTypedefSubstitution = strlen(typedefEntryPtr->typeSpecifier)-(typedefEntryPtr->lengthOfIdentifier+1);
					int32_t lengthOfSegment = endIndex-startIndex;
					int32_t lengthOfNewString = (lengthOfInput-lengthOfSegment)+lengthOfTypedefSubstitution;
					int32_t indexToCopyEndOfOriginal = (endIndex-lengthOfSegment)+lengthOfTypedefSubstitution;
					char* newString = cosmic_malloc(lengthOfNewString+1);
					for (int32_t i=0;i<lengthOfNewString;i++){
						char c;
						if (i<startIndex){
							c = typeString[i];
						} else if (i>=(startIndex+lengthOfTypedefSubstitution)){
							c = typeString[i-lengthOfTypedefSubstitution+lengthOfSegment];
						} else {
							c = typedefEntryPtr->typeSpecifier[(i-startIndex)+typedefEntryPtr->lengthOfIdentifier+1];
						}
						newString[i]=c;
					}
					newString[lengthOfNewString]=0;
					mainStart=startIndex+lengthOfTypedefSubstitution;
					if (didFindOneYet) cosmic_free(typeString);
					typeString=newString;
					didFindOne=true;
					didFindOneYet=true;
					break;
				}
			} else {
				wasPreviousTypeIndicator=false;
			}
		}
	} while (didFindOne);
	*didAllocateNewString = didFindOneYet;
	return typeString;
}





#include "Sizeof.c"



/*
input typeString is not modified
input typeString doesn't actually need to have any types that need to be broken down
returns new string with all types broken down and added to blockFrameArray

after this function completes, typedefs will already be substituted in
*/
char* breakDownTypeAndAdd(char* typeString, bool addToGlobal){
	char* internalString = copyStringToHeapString(typeString);
	int32_t length = strlen(internalString);
	for (int32_t i=0;i<length;i++){
		if (internalString[i]=='{'){
			int32_t pair = getIndexOfMatchingEnclosement(internalString,i);
			assert(pair>=0);
			int32_t target2=i;
			for (int32_t j=i-2;j>=0;j--){
				if (internalString[j]==' '){
					target2=j;
					break;
				}
			}
			assert(target2!=i);
			int32_t target1=0;
			for (int32_t j=target2-1;j>=0;j--){
				if (internalString[j]==' '){
					target1=j+1;
					break;
				}
			}
			bool isEnum = specificStringEqualCheck(internalString,target1,target2,"enum");
			bool isUnion = specificStringEqualCheck(internalString,target1,target2,"union");
			bool isStruct = specificStringEqualCheck(internalString,target1,target2,"struct");
			assert(isEnum | isUnion | isStruct);
			char splitChar;
			int32_t endSplitSearchIndex;
			if (isEnum){
				splitChar = ',';
				endSplitSearchIndex = pair;
			} else {
				splitChar = ';';
				endSplitSearchIndex = pair-2;
			}
			int16_t numberOfSplitSegments = 1;
			for (int32_t j=i+1;j<endSplitSearchIndex;j++){
				char c = internalString[j];
				if (c==splitChar){
					numberOfSplitSegments++;
				} else if (c=='(' | c=='[' | c=='{'){
					j = getIndexOfMatchingEnclosement(internalString,j);
					assert(j>=0);
				}
			}
			int32_t* indexesOfSplitStart = cosmic_malloc(numberOfSplitSegments*sizeof(int32_t));
			int32_t* indexesOfSplitEnd = cosmic_malloc(numberOfSplitSegments*sizeof(int32_t));
			char** stringsOfSegmentsBefore = cosmic_malloc(numberOfSplitSegments*sizeof(char*));
			char** stringsOfSegmentsAfter = cosmic_malloc(numberOfSplitSegments*sizeof(char*));
			indexesOfSplitStart[0]=i+2;
			indexesOfSplitEnd[numberOfSplitSegments-1]=endSplitSearchIndex-1;
			numberOfSplitSegments=1;
			for (int32_t j=i+1;j<endSplitSearchIndex;j++){
				char c = internalString[j];
				if (c==splitChar){
					indexesOfSplitEnd[numberOfSplitSegments-1] = j-1;
					indexesOfSplitStart[numberOfSplitSegments++] = j+2;
				} else if (c=='(' | c=='[' | c=='{'){
					j = getIndexOfMatchingEnclosement(internalString,j);
				}
			}
			for (int16_t j=0;j<numberOfSplitSegments;j++){
				stringsOfSegmentsBefore[j] = copyStringSegmentToHeap(internalString,indexesOfSplitStart[j],indexesOfSplitEnd[j]);
			}
			cosmic_free(indexesOfSplitStart);
			cosmic_free(indexesOfSplitEnd);
			for (int16_t j=0;j<numberOfSplitSegments;j++){
				if (isEnum){
					stringsOfSegmentsAfter[j] = stringsOfSegmentsBefore[j]; // enum segments are not type strings
				} else {
					stringsOfSegmentsAfter[j] = breakDownTypeAndAdd(stringsOfSegmentsBefore[j],addToGlobal);
					cosmic_free(stringsOfSegmentsBefore[j]);
					assert(doesThisTypeStringHaveAnIdentifierAtBeginning(stringsOfSegmentsAfter[j]));
				}
			}
			cosmic_free(stringsOfSegmentsBefore);
			// next, create entry
			uint32_t* offsets = cosmic_malloc(numberOfSplitSegments*sizeof(uint32_t));
			uint32_t walkingOffset = 0;
			if (isEnum){
				for (int16_t j=0;j<numberOfSplitSegments;j++){
					offsets[j] = 0;
				}
				walkingOffset=2;
			} else if (isUnion){
				for (int16_t j=0;j<numberOfSplitSegments;j++){
					uint32_t thisSizeof = getSizeofForTypeString(stringsOfSegmentsAfter[j],false);
					if (thisSizeof==0){
						printf("sizeof failed in breakDownTypeAndAdd()\n");
						exit(1);
					}
					thisSizeof+=thisSizeof&1;
					offsets[j] = 0;
					if (walkingOffset<thisSizeof){
						walkingOffset = thisSizeof; // for union, walkingOffset is treated as a max value (because it will become the final size)
					}
				}
			} else {
				// then it must be a struct
				for (int16_t j=0;j<numberOfSplitSegments;j++){
					uint32_t thisSizeof = getSizeofForTypeString(stringsOfSegmentsAfter[j],false);
					if (thisSizeof==0){
						printf("sizeof failed in breakDownTypeAndAdd()\n");
						exit(1);
					}
					if ((thisSizeof&1)==0 & (walkingOffset&1)==1){
						walkingOffset+=1; // add padding if this is a multiple of allignment and the current offset is not alligned
					}
					offsets[j] = walkingOffset;
					walkingOffset+=thisSizeof;
				}
				walkingOffset+=walkingOffset&1; // final sizes are always alligned
			}
			struct TypeMemberEntry *arrayOfMemberEntries = cosmic_malloc(numberOfSplitSegments*sizeof(struct TypeMemberEntry));
			for (int16_t j=0;j<numberOfSplitSegments;j++){
				arrayOfMemberEntries[j].typeString = stringsOfSegmentsAfter[j];
				arrayOfMemberEntries[j].offset = offsets[j];
			}
			cosmic_free(offsets);
			cosmic_free(stringsOfSegmentsAfter);
			uint8_t typeOfThis;
			if (isStruct){
				typeOfThis = 0;
			} else if (isUnion){
				typeOfThis = 1;
			} else if (isEnum){
				typeOfThis = 2;
			}
			char* name = copyStringSegmentToHeap(internalString,target2+1,i-1);
			// check for entry with same name
			{
				struct TypeSearchResult typeSearchResult;
				searchForType(&typeSearchResult,name,typeOfThis);
				if (typeSearchResult.didExist){
					printf("Type name already exists\n");
					exit(1);
				}
			}
			// next, insert entry
			if (addToGlobal){
				if (blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots>=blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalTypeEntrySlots){
					blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalTypeEntrySlots+=50;
					blockFrameArray.globalBlockFrame.globalTypeEntries = cosmic_realloc(blockFrameArray.globalBlockFrame.globalTypeEntries,
							blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalTypeEntrySlots*sizeof(struct GlobalTypeEntry));
				}
				struct GlobalTypeEntry* globalTypeEntryPtr = &(blockFrameArray.globalBlockFrame.globalTypeEntries[blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots++]);
				globalTypeEntryPtr->name = name;
				globalTypeEntryPtr->arrayOfMemberEntries = arrayOfMemberEntries;
				globalTypeEntryPtr->numberOfMemberEntries = (uint16_t)numberOfSplitSegments;
				globalTypeEntryPtr->thisSizeof = walkingOffset;
				globalTypeEntryPtr->typeOfThis=typeOfThis;
			} else {
				struct BlockFrameEntry* blockFrameEntryPtr = &(blockFrameArray.entries[blockFrameArray.numberOfValidSlots-1]);
				blockFrameEntryPtr->numberOfTypeEntries++;
				blockFrameEntryPtr->typeEntries = cosmic_realloc(blockFrameEntryPtr->typeEntries,blockFrameEntryPtr->numberOfTypeEntries*sizeof(struct BlockFrameTypeEntry));
				struct BlockFrameTypeEntry* blockFrameTypeEntryPtr = &(blockFrameEntryPtr->typeEntries[blockFrameEntryPtr->numberOfTypeEntries-1]);
				blockFrameTypeEntryPtr->name = name;
				blockFrameTypeEntryPtr->arrayOfMemberEntries = arrayOfMemberEntries;
				blockFrameTypeEntryPtr->numberOfMemberEntries = (uint16_t)numberOfSplitSegments;
				blockFrameTypeEntryPtr->thisSizeof = walkingOffset;
				blockFrameTypeEntryPtr->typeOfThis=typeOfThis;
			}
			// next, repair internalString
			for (int32_t j=i;j<pair+1;j++){
				internalString[j]=26;
			}
			if (internalString[pair+1]!=0){
				internalString[pair+1]=26;
			}
			copyDownForInPlaceEdit(internalString);
			length = strlen(internalString);
			assert(internalString[length-1]==' ');
			internalString[--length]=0; // the last character is a space, this removes it
		}
	}
	bool didAllocateAnotherString;
	char* finalString = resolveTypdefsInTypeString(internalString,&didAllocateAnotherString);
	if (didAllocateAnotherString) cosmic_free(internalString);
	return finalString;
}

char* fullTypeParseAndAdd(int32_t startIndex,int32_t endIndex,bool addToGlobal){
	char* typeString_1 = convertType(startIndex,endIndex);
	char* typeString_2 = breakDownTypeAndAdd(typeString_1,addToGlobal);
	cosmic_free(typeString_1);
	return typeString_2;
}


void ensureNoNewDefinitions(char* typeString,int32_t startIndex,int32_t endIndex){
	for (uint32_t i=0;typeString[i];i++){
		if (typeString[i]=='{'){
			err_1111_("This type descriptor should not be defining a new type\n",startIndex,endIndex);
		}
	}
}

char* fullTypeParseAvoidAdd(int32_t startIndex,int32_t endIndex){
	char* typeString_1 = convertType(startIndex,endIndex);
	ensureNoNewDefinitions(typeString_1,startIndex,endIndex);
	bool didAllocateAnotherString;
	char* typeString_2 = resolveTypdefsInTypeString(typeString_1,&didAllocateAnotherString);
	if (didAllocateAnotherString) cosmic_free(typeString_1);
	ensureNoNewDefinitions(typeString_2,startIndex,endIndex);
	return typeString_2;
}















/*


void debugPrintOfBlockFrameArray(){
	printf("===Starting debug print===\n");
	printf("->Global:\n");
	printf("--sizeOfGlobalVariables:%d:\n",(int)blockFrameArray.globalBlockFrame.sizeOfGlobalVariables);
	printf("-->Variables (%d,%d):\n",(int)blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots,(int)blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalVariableEntrySlots);
	for (uint32_t j=0;j<blockFrameArray.globalBlockFrame.numberOfValidGlobalVariableEntrySlots;j++){
		struct GlobalVariableEntry* ptr = &(blockFrameArray.globalBlockFrame.globalVariableEntries[j]);
		printf("---typeString:%s:\n",ptr->typeString);
		printf("---labelID:%d:\n",(int)ptr->labelID);
		printf("---staticLinkbackID:%d:\n",(int)ptr->staticLinkbackID);
		printf("---thisSizeof:%d:\n",(int)ptr->thisSizeof);
		printf("---usedStatic:%d:\n",(int)ptr->usedStatic);
		printf("---usedExtern:%d:\n",(int)ptr->usedExtern);
		printf("---hadInitializer:%d:\n",(int)ptr->hadInitializer);
		printf("---isCurrentlyTentative:%d:\n",(int)ptr->isCurrentlyTentative);
		printf("~~~\n");
	}
	printf("-->Types (%d,%d):\n",(int)blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots,(int)blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalTypeEntrySlots);
	for (uint32_t j=0;j<blockFrameArray.globalBlockFrame.numberOfValidGlobalTypeEntrySlots;j++){
		struct GlobalTypeEntry* ptr = &(blockFrameArray.globalBlockFrame.globalTypeEntries[j]);
		printf("---name:%s:\n",ptr->name);
		printf("---typeOfThis:%d:\n",(int)ptr->typeOfThis);
		printf("---thisSizeof:%d:\n",(int)ptr->thisSizeof);
		printf("---numberOfComponentEntries:%d:\n",(int)ptr->numberOfComponentEntries);
		for (uint16_t k=0;k<ptr->numberOfComponentEntries;k++){
			struct TypeComponentEntry* ptr2 = &(ptr->arrayOfComponentEntries[k]);
			printf("-<>-typeString:%s:\n",ptr2->typeString);
			printf("-<>-offset:%d:\n",(int)ptr2->offset);
			printf("~~~~\n");
		}
		printf("~~~\n");
	}
	printf("-->Functions (%d,%d):\n",(int)blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots,(int)blockFrameArray.globalBlockFrame.numberOfAllocatedGlobalFunctionEntrySlots);
	for (uint32_t j=0;j<blockFrameArray.globalBlockFrame.numberOfValidGlobalFunctionEntrySlots;j++){
		struct GlobalFunctionEntry* ptr = &(blockFrameArray.globalBlockFrame.globalFunctionEntries[j]);
		printf("---typeString:%s:\n",ptr->typeString);
		printf("---labelID:%d:\n",(int)ptr->labelID);
		printf("---hasBeenDefined:%d:\n",(int)ptr->hasBeenDefined);
		printf("---usesVaArgs:%d:\n",(int)ptr->usesVaArgs);
		printf("~~~\n");
	}
	printf("~~\n");
	printf("->BlockFrames (%d,%d):\n",(int)blockFrameArray.numberOfValidSlots,(int)blockFrameArray.numberOfAllocatedSlots);
	for (uint32_t i=0;i<blockFrameArray.numberOfValidSlots;i++){
		struct BlockFrameEntry* entryPtr = &(blockFrameArray.entries[i]);
		printf("--addedStackValue:%d:\n",(int)entryPtr->addedStackValue);
		printf("-->Variables (%d,%d):\n",(int)entryPtr->numberOfValidVariableEntries,(int)entryPtr->numberOfAllocatedVariableEntries);
		for (uint32_t j=0;j<entryPtr->numberOfValidVariableEntries;j++){
			struct BlockFrameVariableEntry* ptr = &(entryPtr->variableEntries[j]);
			printf("---typeString:%s:\n",ptr->typeString);
			printf("---staticLinkbackID:%d:\n",(int)ptr->staticLinkbackID);
			if (ptr->staticLinkbackID==0){
				printf("---thisSizeof:%d:\n",(int)ptr->thisSizeof);
				printf("---offsetInThisBlockFrame:%d:\n",(int)ptr->offsetInThisBlockFrame);
				printf("---isRegister:%d:\n",(int)ptr->isRegister);
			} else {
				printf("---thisSizeof:irrelevant:");
				printf("---offsetInThisBlockFrame:irrelevant:\n");
				printf("---isRegister:irrelevant:\n");
			}
			printf("~~~\n");
		}
		printf("-->Types (%d):\n",(int)entryPtr->numberOfTypeEntries);
		for (uint32_t j=0;j<entryPtr->numberOfTypeEntries;j++){
			struct BlockFrameTypeEntry* ptr = &(entryPtr->typeEntries[j]);
			printf("---name:%s:\n",ptr->name);
			printf("---typeOfThis:%d:\n",(int)ptr->typeOfThis);
			printf("---thisSizeof:%d:\n",(int)ptr->thisSizeof);
			printf("---numberOfComponentEntries:%d:\n",(int)ptr->numberOfComponentEntries);
			for (uint16_t k=0;k<ptr->numberOfComponentEntries;k++){
				struct TypeComponentEntry* ptr2 = &(ptr->arrayOfComponentEntries[k]);
				printf("-<>-typeString:%s:\n",ptr2->typeString);
				printf("-<>-offset:%d:\n",(int)ptr2->offset);
				printf("~~~~\n");
			}
			printf("~~~\n");
		}
		printf("~~\n");
	}
	printf("===Ending debug print===\n");
}



*/



