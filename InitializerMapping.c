

/*

running initializerMapRoot() will destroy any previous initializer mappings
	doesn't even need to have the type defined, it doesn't look at blockFrameArray

initializerImplementRoot() does need to have the type defined, 
	and it will walk the initializer map that is currently created 

There is constant and static expression walking and evaluation in this file too

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
			if (operatorTypeID==2) *thisConstVal&=0xFFFFu;
		}
		break;
		case 18:{
			*thisConstVal=leftConstVal*rightConstVal;
			if (operatorTypeID==2) *thisConstVal&=0xFFFFu;
		}
		break;
		case 21:{
			if (operatorTypeID==1){
				*thisConstVal=leftConstVal+rightConstVal;
			} else if (operatorTypeID==2){
				*thisConstVal=(leftConstVal+rightConstVal)&0xFFFFu;
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 22:{
			if (operatorTypeID==1){
				*thisConstVal=leftConstVal-rightConstVal;
			} else if (operatorTypeID==2){
				*thisConstVal=(leftConstVal-rightConstVal)&0xFFFFu;
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 14:
		case 62:{
			*thisConstVal=extraVal;
		}
		break;
		case 59:{
			if (operatorTypeID==1){
				printInformativeMessageForExpression(true,"Global variables are not quite ready to be in constant expressions",thisNode);
				//exit(1);
			} else if (operatorTypeID==2){
				printInformativeMessageForExpression(true,"Local variables cannot be in constant expressions",thisNode);
				//exit(1);
			} else if (operatorTypeID==3){
				*thisConstVal=extraVal;
			} else { //operatorTypeID==4
				printInformativeMessageForExpression(true,"Function pointers cannot be in constant expressions",thisNode);
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
		GiveHasNotAddedMessage:
		printInformativeMessageForExpression(true,"Unimplemented Error: I have not (or cannot) add support for this operand in constant expressions",thisNode);
		exit(1);
	}
}

void applySymbolicOperator(ExpressionTreeNode* thisNode){
	ExpressionTreeNode* ln;
	ExpressionTreeNode* rn;
	uint16_t operatorTypeID = thisNode->post.operatorTypeID;
	uint32_t extraVal = thisNode->post.extraVal;
	InstructionBuffer* ib=&thisNode->ib;
	InstructionBuffer* ibLeft;
	InstructionBuffer* ibRight;
	InstructionSingle writeIS; 
	if (thisNode->pre.hasLeftNode){
		ln=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
		ibLeft=&(ln->ib);
	}
	if (thisNode->pre.hasRightNode){
		rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
		if (!(thisNode->operatorID==5 | thisNode->operatorID==6)) ibRight=&(rn->ib);
	}
	switch (thisNode->operatorID){
		case 10:
		case 14:
		singleMergeIB(ib,ibRight);
		break;
		case 11:{
			if (operatorTypeID==2){
				writeIS.id=I_SYCD;
				writeIS.arg.D.a_0=0;
				addInstruction(ib,writeIS);
				singleMergeIB(ib,ibRight);
				writeIS.id=I_SYC3;
				addInstruction(ib,writeIS);
			} else {
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=0;
				addInstruction(ib,writeIS);
				singleMergeIB(ib,ibRight);
				writeIS.id=I_SYC2;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 18:{
			if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC5;
				addInstruction(ib,writeIS);
			} else {
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC4;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 21:{
			if (operatorTypeID==1){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC1;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC0;
				addInstruction(ib,writeIS);
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 22:{
			if (operatorTypeID==1){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC3;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC2;
				addInstruction(ib,writeIS);
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 59:{
			if (operatorTypeID==1){
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				// should be unreachable
				printInformativeMessageForExpression(true,"Local variables cannot be in static expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==3){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else { //operatorTypeID==4
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
				printInformativeMessageForExpression(true,"Function pointers in static expressions are... possibly functional right now",thisNode);
			}
		}
		break;
		case 61:{
			if (operatorTypeID==1){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else {
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 62:
		{
			if (operatorTypeID==1){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else {
		case 17:
				writeIS.id=I_SYCD;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			}
		}
		break;
		default:
		GiveHasNotAddedMessage:
		printInformativeMessageForExpression(true,"Unimplemented Error: I have not (or cannot) add support for this operand in static expressions",thisNode);
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
	if (oID==36 | oID==37){
		printInformativeMessageForExpression(true,"Cannot have ternary inside constant expression",thisNode);
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


void expressionToSymbolic(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if ((oID>=38 & oID<=48) | oID==1 | oID==2 | oID==8 | oID==9){
		printInformativeMessageForExpression(true,"Cannot have value modifying operator inside static expression",thisNode);
		exit(1);
	}
	if (oID==65 | oID==66){
		printInformativeMessageForExpression(true,"Cannot have function call inside static expression",thisNode);
		exit(1);
	}
	if (oID==36 | oID==37){
		printInformativeMessageForExpression(true,"Cannot have ternary inside static expression",thisNode);
		exit(1);
	}
	if (thisNode->pre.hasLeftNode)
		expressionToSymbolic(thisNode->pre.leftNode);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionToSymbolic(thisNode->pre.rightNode);
	ensureExpNodeInit(thisNode);
	initInstructionBuffer(&thisNode->ib);
	applyAutoTypeConversion(thisNode);
	applySymbolicOperator(thisNode);
	if (thisNode->pre.hasLeftNode)
		destroyInstructionBuffer(&expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.leftNode ].ib);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6 | oID==17))
		destroyInstructionBuffer(&expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.rightNode].ib);
}




struct ConstValueTypePair{
	uint32_t value;
	uint32_t size; // of typeString
};


// this function has specific expectations for being called
bool shouldAvoidWarningsForInitializerExpressionElement(ExpressionTreeNode* rootNode){
	return rootNode->operatorID==62 && (!compileSettings.warnForZeroCastInStructOrUnionInInitializer & rootNode->post.extraVal==0);
}


void expressionToConstantBase(struct ConstValueTypePair* cvtp,const char* typeStringCast, int16_t nodeIndex){ // potentialNoWarn is intended to be removed
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

void expressionToSymbolicRoot(InstructionBuffer* parent_ib,const char* typeStringCast,uint32_t offset,int16_t nodeIndex, bool potentialNoWarn){
	assert(!doSymbolicConstantGenForExp);
	doSymbolicConstantGenForExp=true;
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint32_t sizeOfCastType=getSizeofForTypeString(typeStringCast,true);
	if (sizeOfCastType==0 | sizeOfCastType==3 | sizeOfCastType>4){
		printInformativeMessageForExpression(true,"type size for the destination type of this static expression is invalid",thisNode);
		exit(1);
	}
	expressionToSymbolic(nodeIndex);
	applyTypeCast(thisNode,typeStringCast,15*!(potentialNoWarn && shouldAvoidWarningsForInitializerExpressionElement(thisNode)));
	InstructionBuffer* ib=&thisNode->ib;
	InstructionBuffer ibTemp;
	initInstructionBuffer(&ibTemp);
	InstructionSingle writeIS;
	writeIS.id=I_LOFF;
	writeIS.arg.D.a_0=offset;
	addInstruction(&ibTemp,writeIS);
	//printf("typeString:`%s`\n",typeStringCast);
	if (sizeOfCastType==1){
		writeIS.id=I_SYDB;
	} else if (sizeOfCastType==2){
		writeIS.id=I_SYDW;
	} else if (sizeOfCastType==4){
		writeIS.id=I_SYDD;
	} else {
		assert(false);
	}
	addInstruction(&ibTemp,writeIS);
	singleMergeIB(&ibTemp,ib);
	destroyInstructionBuffer(ib);
	writeIS.id=I_SYDE;
	addInstruction(&ibTemp,writeIS);
	
	//printInformativeMessageForExpression(false,"output for this expression follows:",thisNode);
	//printInstructionBufferWithMessageAndNumber(&ibTemp,"output",0);
	
	//printf("test exit!\n");
	//exit(0);
	destroyInstructionBuffer(&ibTemp);
	cosmic_free(thisNode->post.typeString);
	doSymbolicConstantGenForExp=false;
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


char* getTypeStringOfFirstNonStructUnionMember(const char* typeString){
	struct TypeSearchResult tsr;
	typeString=stripQualifiersC(typeString,NULL,NULL);
	assert(isTypeStringOfStructOrUnion(typeString));
	bool isUnion=isSectionOfStringEquivalent(typeString,0,"union");
	searchForType(&tsr,typeString+(7-isUnion),isUnion);
	assert(tsr.didExist);
	char* firstMemberTypeString;
	if (tsr.isGlobal) firstMemberTypeString=(char*)blockFrameArray.globalBlockFrame.globalTypeEntries[tsr.typeEntryIndex].arrayOfMemberEntries[0].typeString;
	else firstMemberTypeString=(char*)blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries[tsr.typeEntryIndex].arrayOfMemberEntries[0].typeString;
	firstMemberTypeString=applyToTypeStringRemoveIdentifierToNew(firstMemberTypeString);// this is why the cast to (char*) is okay [a new string gets allocated]
	if (isTypeStringOfStructOrUnion(firstMemberTypeString)){
		char* sub=getTypeStringOfFirstNonStructUnionMember(firstMemberTypeString);
		cosmic_free(firstMemberTypeString);
		return sub;
	}
	return firstMemberTypeString;
}


void initializerImplementStaticExpression(
		int32_t entryIndex,
		struct RawMemoryForInitializer* rmfi,
		const char* typeStringCast,
		uint32_t memoryOffset,
		bool potentialNoWarn){
	
	if (isTypeStringOfStructOrUnion(stripQualifiersC(typeStringCast,NULL,NULL))){
		// this causes static initizations of structs/unions from expression -> initializing that struct/union 's first member from expression
		char* firstMemberTypeString=getTypeStringOfFirstNonStructUnionMember(typeStringCast);
		initializerImplementStaticExpression(entryIndex,rmfi,firstMemberTypeString,memoryOffset,true);
		cosmic_free(firstMemberTypeString);
	} else {
		struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
		/*
		struct ConstValueTypePair cvtp;
		expressionToConstantBase(&cvtp,typeStringCast,ime->expNode,potentialNoWarn);
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
		*/
		expressionToSymbolicRoot(NULL,typeStringCast,memoryOffset,ime->expNode,potentialNoWarn); // todo: remove null
	}
}

void initializerImplementNonstaticExpression(
		int32_t entryIndex,
		InstructionBuffer* ib,
		const char* typeStringCast,
		uint16_t stackOffset,
		bool isBehindList,
		bool potentialNoWarn){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+ime->expNode;
	expressionToAssembly(ime->expNode,NULL,0);
	bool isSU=isTypeStringOfStructOrUnion(thisNode->post.typeStringNQ);
	uint32_t typeSize=getSizeofForTypeString(typeStringCast,true);
	const char* typeStringCastNQ=stripQualifiersC(typeStringCast,NULL,NULL);
	bool isToSU=isTypeStringOfStructOrUnion(typeStringCastNQ);
	uint8_t warnValue=15*!(potentialNoWarn && shouldAvoidWarningsForInitializerExpressionElement(thisNode));
	if (isSU!=isToSU){
		if (isBehindList&!isSU){
			// this causes the first member of a struct/union to be initialized when attempting to initialize it from a non-struct/union expression when in a list initializer
			isToSU=false;isSU=false; // not needed, but to be clear what this ends up doing
			
			char* firstMemberTypeString=getTypeStringOfFirstNonStructUnionMember(typeStringCast);
			applyTypeCast(thisNode,firstMemberTypeString,warnValue);
			typeSize=getSizeofForTypeString(firstMemberTypeString,true);
			cosmic_free(firstMemberTypeString);
			goto InsertJmp;
		} else {
			const char* message;
			if (isSU) message="Cannot initialize non-struct and non-union with struct or union expression";
			else message="Cannot initialize struct or union with non-struct and non-union expression";
			printInformativeMessageAtSourceContainerIndex(true,message,ime->strStart,0);
			exit(1);
		}
	}
	if (!isSU) applyTypeCast(thisNode,typeStringCast,warnValue);
	InsertJmp:;
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
		if (typeSize==1){
			singleMergeIB(ib,&ib_mem_byte_write_n);
		} else if (typeSize==4){
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
	char c;
	while ((c=*(crackedType++))!='{'){
		assert(c!=0);
		if (c=='|') return NULL;
	}
	return crackedType;
}

char* advanceToNextMemberInCrackedType(char* crackedType){
	char c;
	while ((c=*crackedType)!=';'){
		assert(c!=0);
		if (c=='<') crackedType=getIndexOfMatchingEnclosement(crackedType,0)+crackedType;
		crackedType++;
	}
	return crackedType+1;
}

char* gotoMemberAtIndex(char* crackedType,uint16_t memberIndex){
	crackedType=advanceToMembersInCrackedType(crackedType);
	if (crackedType==NULL) return NULL;
	for (uint16_t i=0;i<memberIndex;i++){
		crackedType=advanceToNextMemberInCrackedType(crackedType);
		if (*crackedType=='}') return NULL;
	}
	return crackedType;
}

char* advanceToNameInCrackedType(char* crackedType){
	char c;
	while (true){
		c=*crackedType;
		if (c=='<'){
			crackedType+=getIndexOfMatchingEnclosement(crackedType,0)+1;
			break;
		} else if (c=='('){
			crackedType+=getIndexOfMatchingEnclosement(crackedType,0)+1;
			crackedType=advanceToNameInCrackedType(crackedType);
			break;
		} else if (c=='?'|c=='!'|c=='#'){
			crackedType+=8;
		} else if (c=='j' | c=='h'){
			while (*crackedType!='|'){
				assert(*crackedType!=0);
				crackedType++;
			}
			crackedType++;
			break;
		} else if (c!='*'){
			crackedType++;
			break;
		}
		crackedType++;
	}
	return crackedType;
}

uint16_t findMemberIndexForName(char* crackedType,int32_t strStart,int32_t strEnd){
	char* subCrackedType;
	uint16_t i=0;
	while ((subCrackedType=gotoMemberAtIndex(crackedType,i))!=NULL){
		subCrackedType=advanceToNameInCrackedType(subCrackedType);
		int32_t strWalk=strStart;
		while (true){
			if (*subCrackedType==';'){
				if (strWalk==strEnd) return i;
				else break;
			}
			if (strWalk>strEnd) break;
			if (*(subCrackedType++)!=sourceContainer.string[strWalk++]) break;
		}
		i++;
	}
	return 0xFFFFu;
}


uint32_t designatorSourceStart;
uint32_t designatorSourceEnd;
char* designatedCrackedType;
uint32_t baseDesignation;
uint32_t designationOffsetTotal;
bool doDesignationOffsetCalc;


// fallbackIndex is used only if there are no designators
void calcPotentialDesignatorAndSize(int32_t entryIndex,char* crackedType, uint32_t fallbackIndex){
	bool hasSubEntry;
	bool hasQualifier;
	bool isArray;
	bool isFirst=true;
	char cts;
	char* crackedTypeNQ;
	char* subCrackedType;
	uint32_t thisBaseDesignation=fallbackIndex;
	designationOffsetTotal=0;
	if (entryIndex==-1){
		cts= crackedType[0];
		if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
		isArray=cts!='<';
		crackedTypeNQ=crackedType+hasQualifier;
		if (isArray){
			uint32_t suggestedLength=readHexInString(crackedTypeNQ+1);
			if (thisBaseDesignation>=suggestedLength){
				if (cts=='#'){
					printInformativeMessageAtSourceContainerIndex(true,"Array is too small for this list",designatorSourceStart,designatorSourceEnd);
					exit(1);
				}
				*crackedTypeNQ='!';
				writeHexInString(crackedTypeNQ+1,thisBaseDesignation+1);
			}
			subCrackedType=crackedTypeNQ+9;
		} else {
			subCrackedType = gotoMemberAtIndex(crackedTypeNQ,thisBaseDesignation);
			if (subCrackedType==NULL){
				printInformativeMessageAtSourceContainerIndex(true,"No more members to initialize",designatorSourceStart,designatorSourceEnd);
				exit(1);
			}
		}
		baseDesignation=thisBaseDesignation;
		goto LabelForNoDesignators;
	}
	do {
		struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
		assert(ime->typeOfEntry==5 | ime->typeOfEntry==6);
		cts= crackedType[0];
		if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
		if ((ime->typeOfEntry==5 & cts!='?' & cts!='!' & cts!='#') |
			(ime->typeOfEntry==6 & cts!='<')){
			
			printInformativeMessageAtSourceContainerIndex(true,"This designator and the type do not match",ime->strStart,ime->strEnd);
			exit(1);
		}
		crackedTypeNQ=crackedType+hasQualifier;
		isArray=cts!='<';
		if (isArray){
			thisBaseDesignation=expressionToConstantValue("unsigned long",ime->expNode);
			uint32_t suggestedLength=readHexInString(crackedTypeNQ+1);
			if (thisBaseDesignation>=suggestedLength){
				if (cts=='#'){
					printInformativeMessageAtSourceContainerIndex(true,"This designator\'s index is out of bounds",ime->strStart,ime->strEnd);
					exit(1);
				}
				*crackedTypeNQ='!';
				writeHexInString(crackedTypeNQ+1,thisBaseDesignation+1);
			}
			subCrackedType=crackedTypeNQ+9;
		} else {
			thisBaseDesignation = findMemberIndexForName(crackedTypeNQ,ime->strStart,ime->strEnd);
			if ((uint16_t)thisBaseDesignation==0xFFFFu){
				printInformativeMessageAtSourceContainerIndex(true,"No member of this name in this type",ime->strStart,ime->strEnd);
				exit(1);
			}
			subCrackedType=gotoMemberAtIndex(crackedTypeNQ,thisBaseDesignation);
		}
		if (isFirst) baseDesignation=thisBaseDesignation;
		entryIndex=ime->chainEntry; // these expressions are preparing for the potential next loop body
		isFirst=false;
		LabelForNoDesignators:
		crackedType=subCrackedType;
		if (doDesignationOffsetCalc){
			if (isArray){
				uint32_t unitSize;
				{
					char* unitType=advancedCrackedTypeToTypeString(crackedTypeNQ+9);
					unitSize=getSizeofForTypeString(unitType,true);
					cosmic_free(unitType);
				}
				designationOffsetTotal+=unitSize*thisBaseDesignation;
			} else {
				struct TypeMemberEntry* tmeArray;
				{
					char* unitType=advancedCrackedTypeToTypeString(crackedTypeNQ);
					struct TypeSearchResult tsr;
					char diffSU=*(crackedTypeNQ+1);
					assert(diffSU=='j' | diffSU=='h');
					assert(getIndexOfNthSpace(unitType,0)!=-1);
					assert(getIndexOfNthSpace(unitType,1)==-1);
					searchForType(&tsr,(getIndexOfNthSpace(unitType,0)+1)+unitType,diffSU=='h');
					assert(tsr.didExist);
					if (tsr.isGlobal) tmeArray=blockFrameArray.globalBlockFrame.globalTypeEntries[tsr.typeEntryIndex].arrayOfMemberEntries;
					else tmeArray=blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries[tsr.typeEntryIndex].arrayOfMemberEntries;
					cosmic_free(unitType);
				}
				designationOffsetTotal+=tmeArray[thisBaseDesignation].offset;
			}
		}
	} while (entryIndex!=-1);
	designatedCrackedType=crackedType;
}



void initializerPreImplementList(
		int32_t entryIndex,
		char* crackedType){
	
	doDesignationOffsetCalc=false;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	char cts= crackedType[0];
	bool hasQualifier;
	if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,designatedCrackedType);
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
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,designatedCrackedType);
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
	
	doDesignationOffsetCalc=true;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	
	char cts= crackedType[0];
	bool hasQualifier;
	if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		bool isFirst=true;
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			uint32_t subOffset=memoryOffset+designationOffsetTotal;
			bool isFirstAndLast=isFirst&(imeChain->chainEntry==-1);
			if (imeChain->typeOfEntry==1){
				initializerImplementList(rmfi,ib,subOffset,chainIndex,designatedCrackedType,isStatic);
			} else {
				char* subTypeString=advancedCrackedTypeToTypeString(designatedCrackedType);
				if (doesThisTypeStringHaveAnIdentifierAtBeginning(subTypeString)){
					applyToTypeStringRemoveIdentifierToSelf(subTypeString);
				}
				if (isStatic) {initializerImplementStaticExpression(chainIndex,rmfi,subTypeString,subOffset,isFirstAndLast);}
				else {initializerImplementNonstaticExpression(chainIndex,ib,subTypeString,subOffset,true,isFirstAndLast);}
				cosmic_free(subTypeString);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				isFirst=false;
				continue;
			}
			break;
		}
	} else if (isArray){
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			assert(walkingIndex<readHexInString(crackedType+1+hasQualifier));
			uint32_t subOffset=memoryOffset+designationOffsetTotal;
			if (imeChain->typeOfEntry==1){
				initializerImplementList(rmfi,ib,subOffset,chainIndex,crackedType+9+hasQualifier,isStatic);
			} else {
				char* subTypeString=advancedCrackedTypeToTypeString(designatedCrackedType);
				if (doesThisTypeStringHaveAnIdentifierAtBeginning(subTypeString)){
					applyToTypeStringRemoveIdentifierToSelf(subTypeString);
				}
				if (isStatic) {initializerImplementStaticExpression(chainIndex,rmfi,subTypeString,subOffset,false);}
				else {initializerImplementNonstaticExpression(chainIndex,ib,subTypeString,subOffset,true,false);}
				cosmic_free(subTypeString);
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
			initializerImplementList(rmfi,ib,memoryOffset,ime->subEntry,crackedType,isStatic);
		} else {
			char* unitType=advancedCrackedTypeToTypeString(crackedType);
			if (isStatic) {initializerImplementStaticExpression(chainIndex,rmfi,unitType,memoryOffset,false);}
			else {initializerImplementNonstaticExpression(chainIndex,ib,unitType,memoryOffset,true,false);}
			cosmic_free(unitType);
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
			printInformativeMessageAtSourceContainerIndex(true,strMerge2(strMerge2("typeString \'",*typeStringPtr),"\' is or contains an incomplete struct or union"),ime->strStart,0);
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
				if (true){
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,0);
				insert_IB_load_dword(ib,typeSize-temp);
				dualMergeIB(ib,&ib_i_32add,&ib_mem_word_write_n);
				addVoidPop(ib);
				temp-=2;
				} else {
				temp-=2;
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,temp);
				singleMergeIB(ib,&ib_mem_word_write_n);
				addVoidPop(ib);
				}
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
			initializerImplementStaticExpression(root,rmfi,*typeStringPtr,0,false);
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
			
			initializerImplementNonstaticExpression(root,ib,*typeStringPtr,false,false,false);
			return false;
		}
	}
	
}






















