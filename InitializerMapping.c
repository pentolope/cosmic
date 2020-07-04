

/*

running initializerMapRoot() will destroy any previous initializer mappings
initializerMapRoot() doesn't even need to have the type defined, it doesn't even look at blockFrameArray


There is constant expression walking and evaluation in this file too

*/



// not all operators are done yet
void applyConstantOperator(ExpressionTreeNode* thisNode){
	ExpressionTreeNode* ln;
	ExpressionTreeNode* rn;
	uint16_t operatorTypeID = thisNode->post.operatorTypeID;
	uint32_t extraVal = thisNode->post.extraVal;
	uint32_t leftConstVal;
	uint32_t rightConstVal;
	uint32_t* thisConstVal=&thisNode->post.constVal;
	if (thisNode->pre.hasLeftNode){
		ln=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
		leftConstVal=ln->post.constVal;
	}
	if (thisNode->pre.hasRightNode){
		rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
		rightConstVal=rn->post.constVal;
	}
	switch (thisNode->operatorID){
		case 10:{
			*thisConstVal=rightConstVal;
		}
		break;
		case 11:{
			*thisConstVal=-((int32_t)rightConstVal);
		}
		break;
		case 17:
		case 62:{
			*thisConstVal=extraVal;
		}
		break;
		case 59:{
			if (operatorTypeID==1){
				printInformativeMessageForExpression(true,"Local variables are not ready to be in constant expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==2){
				printInformativeMessageForExpression(true,"Global variables are not ready to be in constant expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==3){
				*thisConstVal=extraVal;
			} else { //operatorTypeID==4
				printInformativeMessageForExpression(true,"Function pointers not allowed in constant expressions (the memory location is symbolic at this point!)",thisNode);
				exit(1);
			}
		}
		break;
		case 61:{
			if (operatorTypeID==1){
				*thisConstVal=extraVal;
			} else {
				printInformativeMessageForExpression(false,"You need to implement the updates to do this!",thisNode);
			}
		}
		break;
		default:
		printInformativeMessageForExpression(true,"Unimplemented Error: I have not (or cannot) add support for this operand in constant expressions",thisNode);
		exit(1);
	}
}

void expressionToConstant(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if ((oID>=38 & oID<=48) | oID==1 | oID==2 | oID==8 | oID==9){
		printInformativeMessageForExpression(true,"Cannot have value modifying operator inside constant expression",thisNode);
		exit(1);
	}
	if (oID==65 | oID==66){
		printInformativeMessageForExpression(true,"Cannot have function call inside constant expression",thisNode);
		exit(1);
	}
	// should I add short-circuiting to this evaluation?
	if (thisNode->pre.hasLeftNode)
		expressionToConstant(thisNode->pre.leftNode);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionToConstant(thisNode->pre.rightNode);
	ensureExpNodeInit(thisNode);
	applyAutoTypeConversion(thisNode);
	applyConstantOperator(thisNode);
}

struct ConstValueTypePair{
	uint32_t value;
	uint32_t size; // of typeString
};

void expressionToConstantBase(struct ConstValueTypePair* cvtp,const char* typeStringCast, int16_t nodeIndex){
	expressionToConstant(nodeIndex);
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	applyTypeCast(thisNode,typeStringCast,15);
	cvtp->value=thisNode->post.constVal;
	cvtp->size=getSizeofForTypeString(thisNode->post.typeString,true);
	if (cvtp->size==0){
		printInformativeMessageForExpression(true,"Failed to get size for the type of this constant expression",thisNode);
		exit(1);
	}
	cosmic_free(thisNode->post.typeString);
}

uint32_t expressionToConstantValue(const char* typeStringCast,int16_t nodeIndex){
	struct ConstValueTypePair cvtp;
	expressionToConstantBase(&cvtp,typeStringCast,nodeIndex);
	return cvtp.value;
}


struct InitializerMap{
	struct InitializerMapEntry{
		// for the next 3, the value is -1 if it doesn't exist
		int32_t chainEntry;
		int32_t subEntry;
		int32_t descriptionBranchEntry;
		int32_t strStart;
		int32_t strMid;
		int32_t strEnd;
		uint8_t typeOfEntry;
		int16_t expNode;
	} *entries;
	int32_t numberOfEntriesAllocated;
	int32_t numberOfEntriesTaken;
	int32_t rootEntry;
	uint8_t typeOfInit;
	
	
} initializerMap;
/*

typeOfInit==0 : no initializer
typeOfInit==1 : bracket pair
typeOfInit==2 : string
typeOfInit==3 : character
typeOfInit==4 : expression


typeOfEntry==0 : {invalid}
typeOfEntry==1 : bracket pair
	subEntry points to first content of bracket pair, which are chained on chainEntry
typeOfEntry==2 : string
	if typeOfInit==2, this is the root and is a modifiable string literal. 
	Otherwise, it is not the root, and is an unmodifiable string literal.
typeOfEntry==3 : character
	if typeOfInit==3, this is the root. Otherwise, it is not the root.
typeOfEntry==4 : expression
	if typeOfInit==4, this is the root. Otherwise, it is not the root.

typeOfEntry==5 : location-array
typeOfEntry==6 : location-member
typeOfEntry==7 : {internal only} used to skip the . for location

*/


int32_t partitionEntryInInitializerMap(){
	if (initializerMap.numberOfEntriesTaken>=initializerMap.numberOfEntriesAllocated){
		if (initializerMap.numberOfEntriesAllocated<64){
			initializerMap.numberOfEntriesAllocated=64;
		} else {
			initializerMap.numberOfEntriesAllocated*=2;
		}
		initializerMap.entries=cosmic_realloc(initializerMap.entries,initializerMap.numberOfEntriesAllocated*sizeof(struct InitializerMapEntry));
	}
	return initializerMap.numberOfEntriesTaken++;
}

int32_t emptyIndexRecedeForInitializerMap(int32_t strStart){
	int32_t strWalk=strStart+1;
	while (true){
		char c=sourceContainer.string[--strWalk];
		assert(strWalk!=0); // hit string start
		if (!(c==' ' | c=='\n')){
			return strWalk;
		}
	}
}

int32_t emptyIndexAdvanceForInitializerMap(int32_t strStart){
	int32_t strWalk=strStart-1;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (!(c==' ' | c=='\n')){
			return strWalk;
		}
	}
}

int32_t findLocatorEndForInitializerMap(int32_t strStart){
	char strStartChar=sourceContainer.string[strStart];
	int32_t strWalk=strStart;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=';' & c!=0);
		if (strStartChar=='.'){
			if (!(c==' ' | c=='\n')){
				return strWalk;
			}
		} else if (strStartChar=='['){
			assert(c!='['); // hit nested brackets
			if (c==']'){
				return strWalk+1;
			}
		} else if (strStartChar=='='){
			return strStart;
		} else if (c==' ' | c=='\n' | c=='.' | c=='[' | c=='='){
			return strWalk;
		} else {
			assert(c!='\'' & c!='\"' & c!='{' & c!='}'); // should not hit string or character literals or brackets
		}
	}
}

int32_t parenSkipForInitializerMap(int32_t strStart){
	bool inLiteral=false;
	int32_t strWalk=strStart;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (inLiteral){
			if (c=='\\') ++strWalk;
			else if (c=='\"' | c=='\'') inLiteral=false;
		} else {
			if (c=='(') strWalk=parenSkipForInitializerMap(strWalk);
			else if (c==')') return strWalk;
			else if (c=='\"' | c=='\'') inLiteral=true;
		}
	}
}

int32_t findTypicalEndForInitializerMap(int32_t strStart){
	bool inLiteral=false;
	int32_t strWalk=strStart-1;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (c=='\'' | c=='\"'){
			inLiteral^=true;
			continue;
		}
		if (inLiteral){
			if (c=='\\') ++strWalk;
			continue;
		}
		if (c==';'){
			return strWalk;
		} else if (c=='('){
			strWalk=parenSkipForInitializerMap(strWalk);
		} else if (c=='{' & strWalk!=strStart){
			strWalk=findTypicalEndForInitializerMap(strWalk);
		} else if (c==',' | c=='}'){
			if (!(c==',' & sourceContainer.string[strStart]=='{')){
				return strWalk;
			}
		} else if (c=='=' && sourceContainer.string[strStart]!='{'){
			int32_t t = findTypicalEndForInitializerMap(emptyIndexAdvanceForInitializerMap(strWalk+1));
			return t;
		}
	}
}

uint8_t catagorizeInitializerMapLocator(int32_t strStart){
	char strStartChar0=sourceContainer.string[strStart];
	assert(strStartChar0!=' ' & strStartChar0!='\n'); // bad starting place
	if (strStartChar0=='.'){
		return 7;
	} else if (strStartChar0=='['){
		return 5;
	} else {
		return 6;
	}
}

int32_t initializerMapLocator(int32_t strStart){
	struct InitializerMapEntry ime={-1,-1,-1,strStart,strStart,
		findLocatorEndForInitializerMap(strStart),catagorizeInitializerMapLocator(strStart),-1};
	if (ime.typeOfEntry==7){
		return initializerMapLocator(ime.strEnd);
	} else if (strStart==ime.strEnd){
		return -1;
	}
	int32_t thisEntry=partitionEntryInInitializerMap();
	ime.chainEntry=initializerMapLocator(emptyIndexAdvanceForInitializerMap(ime.strEnd));
	initializerMap.entries[thisEntry]=ime;
	return thisEntry;
}

uint8_t catagorizeInitializerMapTypical(char* string,int32_t index){
	const char c0=string[index  ];
	const char c1=string[index+1];
	if (c0=='{'){
		return 1;
	} else if (c0=='\"' | (c0=='L' & c1=='\"')){
		if (initializerMap.typeOfInit==0){
			if (string[emptyIndexAdvanceForInitializerMap(getEndOfToken(string,index))]==';') return 2;
			return 4;
		} else if (initializerMap.typeOfInit==4) return 4;
		return 2;
	} else if (c0=='\'' | (c0=='L' & c1=='\'')){
		return 3;
	}
	return 4;
}

int32_t initializerMapTypical(int32_t strStart){
	struct InitializerMapEntry ime={-1,-1,-1,strStart,strStart,findTypicalEndForInitializerMap(strStart),0,-1};
	{
	char strStartChar0=sourceContainer.string[strStart];
	assert(!(strStartChar0==' ' | strStartChar0=='\n')); // bad starting place
	if (strStartChar0=='.' | strStartChar0=='['){
		while (true){
			char c=sourceContainer.string[++ime.strMid];
			if (c=='='){
				ime.strMid=emptyIndexAdvanceForInitializerMap(ime.strMid+1);
				assert(ime.strMid<ime.strEnd); // overran strEnd while looking for locator end
				ime.descriptionBranchEntry=initializerMapLocator(strStart);
				break;
			} else {
				assert(c!=';' & c!='{' & c!='}' & c!=0); // should not hit these while looking for locator
			}
		}
	}
	ime.typeOfEntry=catagorizeInitializerMapTypical(sourceContainer.string,ime.strMid);
	}
	int32_t thisEntry=partitionEntryInInitializerMap();
	int32_t strEndNextIndex=emptyIndexAdvanceForInitializerMap(ime.strEnd+1);
	char strEndChar=sourceContainer.string[ime.strEnd];
	if (ime.typeOfEntry==1){
		assert(strEndChar=='}'); // bad end
		ime.subEntry=initializerMapTypical(emptyIndexAdvanceForInitializerMap(ime.strMid+1));
		if (sourceContainer.string[strEndNextIndex]==','){
			ime.chainEntry=initializerMapTypical(emptyIndexAdvanceForInitializerMap(strEndNextIndex+1));
		}
	} else {
		assert(strEndChar=='}' | strEndChar==';' | strEndChar==','); // bad end
		if (strEndChar==','){
			ime.chainEntry=initializerMapTypical(strEndNextIndex);
		}
	}
	initializerMap.entries[thisEntry]=ime;
	return thisEntry;
}

// returns true if the end was found exactly, throws error if end was overstepped
bool endCheckInitializerMap(int32_t entryIndex,int32_t strStart,int32_t strEnd){
	if (entryIndex==-1) return false;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	int32_t thisStrEnd=ime->strEnd;
	bool foundEnd=thisStrEnd==strEnd;
	assert(thisStrEnd<=strEnd); // check if initializerMapRoot() overstepped strEnd
	foundEnd|=endCheckInitializerMap(ime->subEntry              ,strStart,strEnd);
	foundEnd|=endCheckInitializerMap(ime->chainEntry            ,strStart,strEnd);
	foundEnd|=endCheckInitializerMap(ime->descriptionBranchEntry,strStart,strEnd);
	return foundEnd;
}

void expressionPrecheckForInitializer(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if (oID==59){
		struct IdentifierSearchResult isr;
		char* identifier = copyStringSegmentToHeap(sourceContainer.string,thisNode->startIndexInString,thisNode->endIndexInString);
		searchForIdentifier(&isr,identifier,true,true,true,true,true);
		if (!isr.didExist){
			printInformativeMessageForExpression(true,"This identifier does not exist",thisNode);
			exit(1);
		}
		cosmic_free(identifier);
	}
	/*
	the times where it is valid to call a function here, 
	the function call identifier will be checked elsewhere 
	and it will be okay to not precheck it as other identifiers must be.
	That is because there are times where it is valid to use an identifier 
	but this precheck must be done on that identifier.
	*/
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionPrecheckForInitializer(thisNode->pre.rightNode);
	if (thisNode->pre.hasLeftNode)
		expressionPrecheckForInitializer(thisNode->pre.leftNode);
	if (thisNode->pre.hasChainNode)
		expressionPrecheckForInitializer(thisNode->pre.chainNode);
	
}

void genExpressionsForInitializerMap(int32_t entryIndex){
	if (entryIndex==-1) return;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	uint8_t typeOfEntry=ime->typeOfEntry;
	if (typeOfEntry>=2 & typeOfEntry<=5){
		int32_t start=ime->strMid;
		int32_t end=ime->strEnd-1;
		if (typeOfEntry==5){
			++start;
			--end;
		}
		start=emptyIndexAdvanceForInitializerMap(start);
		end=emptyIndexRecedeForInitializerMap(end)+1;
		ime->expNode=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,start,end,false);
		if (ime->expNode!=-1){
			expressionPrecheckForInitializer(ime->expNode);
		} else {
			printInformativeMessageAtSourceContainerIndex(true,"Expression expected here",start,end);
			exit(1);
		}
	}
	genExpressionsForInitializerMap(ime->subEntry);
	genExpressionsForInitializerMap(ime->chainEntry);
	genExpressionsForInitializerMap(ime->descriptionBranchEntry);
}


// returns root. clears any previous initializer mappings, and clear previous expressions. if strEnd==-1 then it doesn't check for overstepping strEnd
int32_t initializerMapRoot(int32_t strStart,int32_t strEnd){
	clearPreviousExpressions();
	initializerMap.numberOfEntriesTaken=0;
	initializerMap.typeOfInit=0;//this set is needed for catagorizeInitializerMapTypical() in case a string is first and it needs to know if it is running to find the typeOfInit or typeOfEntry
	initializerMap.typeOfInit=catagorizeInitializerMapTypical(sourceContainer.string,strStart);
	int32_t root = initializerMapTypical(strStart);
	if (initializerMap.typeOfInit==1){
		strEnd=emptyIndexRecedeForInitializerMap(strEnd-1);
		assert(strStart<strEnd); // bracket location calc failure
	}
	assert(initializerMap.entries[root].typeOfEntry==initializerMap.typeOfInit); // failed to agree on catagory
	assert(!(strEnd!=-1 && !endCheckInitializerMap(root,strStart,strEnd))); // understepped the end of the initializer
	if (initializerMap.typeOfInit!=2){
		genExpressionsForInitializerMap(root);
	}
	return root;
}

#include "TypeStringCracking.c"











void initializerImplementStaticExpression(
		int32_t entryIndex,
		struct RawMemoryForInitializer* rmfi,
		char* typeStringCast,
		uint32_t memoryOffset){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct ConstValueTypePair cvtp;
	expressionToConstantBase(&cvtp,typeStringCast,ime->expNode);
	bool didSetFail;
	if (cvtp.size==1){
		didSetFail=setByteRawMemoryForInitializer(rmfi,1,memoryOffset,cvtp.value);
	} else if (cvtp.size==2){
		didSetFail=setWordRawMemoryForInitializer(rmfi,2,memoryOffset,cvtp.value);
	} else if (cvtp.size==4){
		didSetFail=setDwordRawMemoryForInitializer(rmfi,4,memoryOffset,cvtp.value);
	} else {
		assert(false); //bad size
	}
	if (didSetFail){
		printf("Warning: data overlap in initializer\n");
	}
}

void initializerImplementNonstaticExpression(
		int32_t entryIndex,
		InstructionBuffer* ib,
		char* typeStringCast,
		uint16_t stackOffset){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+ime->expNode;
	expressionToAssembly(ime->expNode,NULL,0);
	uint32_t typeSize=getSizeofForTypeString(typeStringCast,true);
	char* typeStringCastNQ=stripQualifiers(typeStringCast,NULL,NULL);
	bool isSU=isTypeStringOfStructOrUnion(thisNode->post.typeStringNQ);
	bool isToSU=isTypeStringOfStructOrUnion(typeStringCastNQ);
	if (isSU!=isToSU){
		const char* message;
		if (!isSU& isToSU) message="Cannot initialize non-struct and non-union with struct or union expression";
		else message="Cannot initialize struct or union with non-struct and non-union expression";
		printInformativeMessageAtSourceContainerIndex(true,message,ime->strStart,0);
		exit(1);
	}
	if (!isSU) applyTypeCast(thisNode,typeStringCast,15);
	singleMergeIB(ib,&thisNode->ib);
	destroyInstructionBuffer(&thisNode->ib);
	if (isSU){
		/*
		TODO: the following ignores volatile and const. when reading memory, it should obey this.
		For writing memory, 
			I would kinda expect that volatile can be ignored (it is garenteed to be writing to stack memory), 
			and const should be ignored (because this is the initializer, 
			if the initializer can't write to it then initializing const values wouldn't be possible)
		*/
		if (!doStringsMatch(thisNode->post.typeStringNQ,typeStringCastNQ)){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize differing struct or union types",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (typeSize==0){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize incomplete struct or union",ime->strStart,0);
			exit(1);
		}
		insert_IB_STPI(ib,stackOffset);
		singleMergeIB(ib,&ib_stack_swp_22);
		while (typeSize!=0){
			singleMergeIB(ib,&ib_mem_word_copy_n_n);
			typeSize-=2;
		}
		addQuadVoidPop(ib);
	} else {
		assert(!(typeSize==0 | typeSize==3 | typeSize>4));
		insert_IB_STPI(ib,stackOffset);
		if (typeSize==4){
			singleMergeIB(ib,&ib_mem_dword_write_n);
			addVoidPop(ib);
		} else {
			singleMergeIB(ib,&ib_mem_word_write_n);
		}
		addVoidPop(ib);
	}
	cosmic_free(thisNode->post.typeString);
}

char* advanceToMembersInCrackedType(char* crackedType){
	while (*(crackedType++)!='{'){
	}
	return crackedType;
}

char* advanceToNextMemberInCrackedType(char* crackedType){
	while (*crackedType!=';'){
		if (*crackedType=='<'){
			crackedType=getIndexOfMatchingEnclosement(crackedType,0)+crackedType;
		}
		crackedType++;
	}
	return crackedType+1;
}

char* gotoMemberAtIndex(char* crackedType,uint16_t memberIndex){
	crackedType=advanceToMembersInCrackedType(crackedType);
	for (uint16_t i=0;i<memberIndex;i++){
		crackedType=advanceToNextMemberInCrackedType(crackedType);
		if (*crackedType=='}'){
			return NULL;
		}
	}
	return crackedType;
}

uint16_t getMemberCount(char* crackedType){
	uint16_t count=0;
	while (gotoMemberAtIndex(crackedType,count)!=NULL){
		count++;
	}
	return count;
}

uint16_t findMemberIndexForName(char* crackedType,int32_t strStart,int32_t strEnd){
	char* subCrackedType;
	uint16_t i=0;
	while ((subCrackedType=gotoMemberAtIndex(crackedType,i))!=NULL){
		char c;
		while (true){
			c=*subCrackedType;
			if (c=='<'){
				subCrackedType+=getIndexOfMatchingEnclosement(subCrackedType,0)+1;
				break;
			} else if (c=='?'|c=='!'|c=='#'){
				subCrackedType+=8;
			} else if (c!='*'){
				subCrackedType++;
				break;
			}
			subCrackedType++;
		}
		int32_t strWalk=strStart;
		while (true){
			if (*subCrackedType==';'){
				if (strWalk==strEnd){
					return i;
				} else {
					break;
				}
			}
			if (strWalk>strEnd){
				break;
			}
			if (*(subCrackedType++)!=sourceContainer.string[strWalk++]){
				break;
			}
		}
		i++;
	}
	return 0xFFFF;
}


uint32_t nestedDesignatorBuffer[64];

uint32_t insertDesignatorSizes(
		int32_t entryIndex,
		char* crackedType,
		uint8_t nestLevel){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	if (nestLevel==0){
		for (uint16_t i=0;i<64;i++){
			nestedDesignatorBuffer[i]=0xFFFFFFFF;
		}
	} else if (nestLevel>=64){
		printInformativeMessageAtSourceContainerIndex(true,"Too many designators",ime->strStart,ime->strEnd);
		exit(1);
	}
	char cts= crackedType[0];
	if ((ime->typeOfEntry==5 & cts!='?' & cts!='!' & cts!='#') |
		(ime->typeOfEntry==6 & cts!='<')){
		
		printInformativeMessageAtSourceContainerIndex(true,"This designator and the type do not match",ime->strStart,ime->strEnd);
		exit(1);
	}
	assert(ime->typeOfEntry==5 | ime->typeOfEntry==6);
	if (ime->typeOfEntry==5){
		uint32_t arrayIndex=expressionToConstantValue("unsigned long",ime->expNode);
		uint32_t suggestedLength=readHexInString(crackedType+1);
		if (arrayIndex>=suggestedLength){
			if (cts=='#'){
				printInformativeMessageAtSourceContainerIndex(true,"This designator\'s index is out of bounds",ime->strStart,ime->strEnd);
				exit(1);
			}
			*crackedType='!';
			writeHexInString(crackedType+1,arrayIndex+1);
		}
		if (ime->chainEntry!=-1){
			insertDesignatorSizes(ime->chainEntry,crackedType+9,nestLevel+1);
		}
		nestedDesignatorBuffer[nestLevel]=arrayIndex;
		return arrayIndex;
	} else {
		uint16_t memberIndex = findMemberIndexForName(crackedType,ime->strStart,ime->strEnd);
		if (memberIndex==0xFFFF){
			printInformativeMessageAtSourceContainerIndex(true,"No member of this name in this type",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (ime->chainEntry!=-1){
			insertDesignatorSizes(ime->chainEntry,gotoMemberAtIndex(crackedType,memberIndex),nestLevel+1);
		}
		nestedDesignatorBuffer[nestLevel]=memberIndex;
		return memberIndex;
	}
}

void initializerPreImplementList(
		int32_t entryIndex,
		char* crackedType){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	char cts= crackedType[0];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		while (true){
			if (imeChain->descriptionBranchEntry!=-1){
				walkingIndex=insertDesignatorSizes(imeChain->descriptionBranchEntry,crackedType,0);
			}
			char* crackedTypeMember = gotoMemberAtIndex(crackedType,walkingIndex);
			if (crackedTypeMember==NULL){
				printInformativeMessageAtSourceContainerIndex(true,"No more members to initialize",imeChain->strStart,imeChain->strEnd);
				exit(1);
			}
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,crackedTypeMember);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isArray){
		while (true){
			if (imeChain->descriptionBranchEntry!=-1){
				walkingIndex=insertDesignatorSizes(imeChain->descriptionBranchEntry,crackedType,0);
			} else {
				uint32_t suggestedLength=readHexInString(crackedType+1);
				if (walkingIndex>=suggestedLength){
					if (cts=='#'){
						printInformativeMessageAtSourceContainerIndex(true,"Array is too small for this list",ime->strStart,ime->strEnd);
						exit(1);
					}
					*crackedType='!';
					writeHexInString(crackedType+1,walkingIndex+1);
				}
			}
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,crackedType+9);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isBase){
		if (imeChain->chainEntry!=-1){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot use multiple expressions to initialize a type that isn't an array, struct, or union",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (imeChain->descriptionBranchEntry!=-1){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot use designator to initialize a type that isn't an array, struct, or union",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (imeChain->typeOfEntry==1){
			initializerPreImplementList(ime->subEntry,crackedType);
		}
	} else {
		assert(false);// corrupted crackedType
	}
}

void initializerImplementList(
		struct RawMemoryForInitializer* rmfi,
		InstructionBuffer* ib,
		uint32_t memoryOffset,
		int32_t entryIndex,
		char* crackedType,
		bool isStatic){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	char cts= crackedType[0];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		while (true){
			if (imeChain->descriptionBranchEntry!=-1){
				walkingIndex=insertDesignatorSizes(imeChain->descriptionBranchEntry,crackedType,0);
			}
			char* crackedTypeMember = gotoMemberAtIndex(crackedType,walkingIndex);
			assert(crackedTypeMember!=NULL);
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,crackedTypeMember);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isArray){
		while (true){
			if (imeChain->descriptionBranchEntry!=-1){
				walkingIndex=insertDesignatorSizes(imeChain->descriptionBranchEntry,crackedType,0);
			} else {
				uint32_t suggestedLength=readHexInString(crackedType+1);
				if (walkingIndex>=suggestedLength){
					assert(cts!='#');
					*crackedType='!';
					writeHexInString(crackedType+1,walkingIndex+1);
				}
			}
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,crackedType+9);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isBase){
		assert(imeChain->chainEntry==-1);
		assert(imeChain->descriptionBranchEntry==-1);
		if (imeChain->typeOfEntry==1){
			initializerPreImplementList(ime->subEntry,crackedType);
		}
	} else {
		assert(false);
	}
}









// returns true if it used RawMemoryForInitializer, otherwise it used InstructionBuffer
bool initializerImplementRoot(
		struct RawMemoryForInitializer* rmfi,
		InstructionBuffer* ib,
		int32_t root,
		char** typeStringPtr,
		bool isStatic){
	
	char* localTypeString=stripQualifiers(*typeStringPtr,NULL,NULL);
	struct InitializerMapEntry* ime=initializerMap.entries+root;
	bool useExpressionForString=false;
	if (initializerMap.typeOfInit==2 & localTypeString[0]=='*'){
		useExpressionForString=true;
		genExpressionsForInitializerMap(root);
	}
	if (initializerMap.typeOfInit==1){
		
		char* crackedType = crackTypeString(*typeStringPtr);
		if (crackedType==NULL){
			printInformativeMessageAtSourceContainerIndex(true,concatStrings(concatStrings("typeString \'",*typeStringPtr),"\' is or contains an incomplete struct or union"),ime->strStart,0);
			exit(1);
		}
		for (int32_t i=0;crackedType[i];i++){
			if (crackedType[i]=='?' & i!=0){
				printInformativeMessageAtSourceContainerIndex(false,"Initializing an incomplete type like this is typically invalid.\n    However, this compiler can probably still calculate the array size",ime->strStart,0);
				break;
			}
		}
		initializerPreImplementList(root,crackedType);
		for (int32_t i=0;crackedType[i];i++){
			if (crackedType[i]=='?'){
				printInformativeMessageAtSourceContainerIndex(true,"An array type remains incomplete, this initializer is insufficient for this type declaration",ime->strStart,0);
				exit(1);
			}
		}
		uint16_t arrayCount=0;
		for (int32_t i=0;crackedType[i];i++){
			char c=crackedType[i];
			if (c=='!'){
				char* new=insertSizeToEmptySizedArrayForTypeStringToNew(*typeStringPtr,arrayCount,readHexInString(crackedType+(i+1)));
				cosmic_free(*typeStringPtr);
				*typeStringPtr=new;
				arrayCount++;
			} else if (c=='#'){
				arrayCount++;
			} else if (c=='<'){
				break;
			}
		}
		
		
		uint32_t typeSize = getSizeofForTypeString(*typeStringPtr,true);
		assert(typeSize!=0); // sizeof failure not expected
		typeSize+=typeSize&1;
		if (isStatic){
			initRawMemoryForInitializer(rmfi,typeSize);
		} else {
			uint32_t temp=typeSize;
			while (temp){
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,0);
				insert_IB_load_dword(ib,typeSize-temp);
				dualMergeIB(ib,&ib_i_32add,&ib_mem_word_write_n);
				addVoidPop(ib);
				temp-=2;
			}
		}
		// this stuff will change
		initializerImplementList(rmfi,ib,0,root,crackedType,isStatic);
		cosmic_free(crackedType);
		return isStatic;
	} else if (initializerMap.typeOfInit==2 &!useExpressionForString){
		if (localTypeString[0]!='['){
			printInformativeMessageAtSourceContainerIndex(true,"Initializer invalid for declared type",ime->strStart,ime->strEnd);
			exit(1);
		}
		uint32_t walkingIndex = 0;
		int32_t start;
		bool isWideLiteral=false;
		if (sourceContainer.string[ime->strStart]=='L'){
			start=ime->strStart+2;
			isWideLiteral=true;
		} else {
			start=ime->strStart+1;
		}
		for (int32_t i=start;i<ime->strEnd;i++){
			uint8_t c0 = (uint8_t)(sourceContainer.string[i]);
			uint8_t valueToSet = c0;
			if (c0=='\\'){
				uint8_t c1 = (uint8_t)(sourceContainer.string[i+1]);
				if (c1=='0'){
					uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
					uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
					c2 = ucharAsciiAsNumberDigit(c2,true,false);
					c3 = ucharAsciiAsNumberDigit(c3,true,false);
					if (c2==200 | c3==200){
						printf("escape sequence number not in octal range\n");
						exit(1);
					}
					valueToSet=c2*8+c3;
					i+=2;
				} else if (c1=='x'){
					uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
					uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
					c2 = ucharAsciiAsNumberDigit(c2,false,true);
					c3 = ucharAsciiAsNumberDigit(c3,false,true);
					if (c2==200 | c3==200){
						printf("escape sequence number not in hexadecimal range\n");
						exit(1);
					}
					if (isWideLiteral){
						uint8_t c4 = (uint8_t)(sourceContainer.string[i+4]);
						uint8_t c5 = (uint8_t)(sourceContainer.string[i+5]);
						c4 = ucharAsciiAsNumberDigit(c4,false,true);
						c5 = ucharAsciiAsNumberDigit(c5,false,true);
						if (c4==200 | c5==200){
							printf("escape sequence number not in hexadecimal range\n");
							exit(1);
						}
						// not implemented further, as it is currently not supported
						i+=2;
					} else {
						valueToSet=c2*16+c3;
					}
					i+=2;
				} else if (c1=='a'){
					valueToSet=7;
				} else if (c1=='b'){
					valueToSet=8;
				} else if (c1=='f'){
					valueToSet=12;
				} else if (c1=='n'){
					valueToSet=10;
				} else if (c1=='r'){
					valueToSet=13;
				} else if (c1=='t'){
					valueToSet=9;
				} else if (c1=='v'){
					valueToSet=11;
				} else if (c1=='\''){
					valueToSet=39;
				} else if (c1=='\"'){
					valueToSet=34;
				} else if (c1=='\\'){
					valueToSet=92;
				} else if (c1=='?'){
					valueToSet=63;
				} else {
					printf("unrecognized escape sequence for string literal\n");
					exit(1);
				}
				i+=1;
			}
			if (setByteRawMemoryForInitializer(rmfi,1,walkingIndex++,valueToSet)){
				printf("Internal Error\n");
				exit(1);
			}
		}
		int32_t enclosementIndex=getIndexOfMatchingEnclosement(localTypeString,0);
		char* temp=stripQualifiers(localTypeString+enclosementIndex+2,NULL,NULL);
		if (!doStringsMatch(temp,"char") & !doStringsMatch(temp,"unsigned char")){
			printInformativeMessageAtSourceContainerIndex(true,"String literal is trying to initialize a type that isn\'t a char[] or char* (sign and qualifiers don\'t matter)",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (enclosementIndex==2){
			setByteRawMemoryForInitializer(rmfi,1,walkingIndex++,0); // won't fail
			if (setByteRawMemoryForInitializer(rmfi,1,walkingIndex++,0)){
				printf("Internal Error\n");
				exit(1);
			}
			char* new=insertSizeToEmptySizedArrayForTypeStringToNew(*typeStringPtr,0,rmfi->size);
			cosmic_free(*typeStringPtr);
			*typeStringPtr=new;
		} else {
			uint32_t typeSize=getSizeofForTypeString(localTypeString,true);
			assert(typeSize!=0); // typeString corruption
			if (typeSize<rmfi->size){
				printInformativeMessageAtSourceContainerIndex(true,"String literal is too large for the indicated array size",ime->strStart,ime->strEnd);
				exit(1);
			}
			while (typeSize!=rmfi->size){
				setByteRawMemoryForInitializer(rmfi,1,walkingIndex++,0); // won't fail
			}
		}
		return true;
	} else {
		if (localTypeString[0]=='['){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize array with expression",ime->strStart,ime->strEnd);
			exit(1);
		}
		uint32_t typeSize=getSizeofForTypeString(localTypeString,true);
		if (typeSize==0){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize incomplete struct or union",ime->strStart,0);
			exit(1);
		}
		typeSize+=typeSize&1;
		if (isStatic){
			if (isTypeStringOfStructOrUnion(localTypeString)){
				printInformativeMessageAtSourceContainerIndex(true,"Initializing by expression to a struct or union of static storage\n    is not possible with the current implementation of constant expressions",ime->strStart,0);
				exit(1);
			}
			initializerImplementStaticExpression(root,rmfi,*typeStringPtr,0);
			return true;
		} else {
			/*
			uint32_t temp=typeSize;
			while (temp){
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,0);
				insert_IB_load_dword(ib,typeSize-temp);
				dualMergeIB(ib,&ib_i_32add,&ib_mem_word_write_n);
				addVoidPop(ib);
				temp-=2;
			}
			*/
			
			// pretty sure that initializerImplementNonstaticExpression() always initializes the memory
			
			initializerImplementNonstaticExpression(root,ib,*typeStringPtr,0);
			return false;
		}
	}
	
}






















