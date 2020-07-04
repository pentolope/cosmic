





// this prototype will be needed when function inlining becomes a thing
int32_t functionStatementsWalk(
	char* returnTypeString,
	int32_t indexOfStart, // usually, right after bracket
	InstructionBuffer* parentInstructionBufferToInsertTo,
	uint32_t labelNumberForBreak,
	uint32_t labelNumberForContinue,
	uint32_t labelNumberForInlinedReturn,
	bool stopAfterSingleExpression);



bool doSymbolicConstantGenForExp = false; // InitializerMapping controls this. typically it is false. (as of writting this is still being implemented)



void ensureExpNodeInit(ExpressionTreeNode* thisNode){
	const ExpTreePostWalkData n0 = {0};
	const InstructionBuffer n1 = {0};
	thisNode->post=n0;
	thisNode->ib=n1;
}



void findErrorRangeForExpressionNode(ExpressionTreeNode* thisNode, int32_t* start, int32_t* end){
	int32_t possibleStart=thisNode->startIndexInString;
	int32_t possibleEnd=thisNode->endIndexInString;
	if (thisNode->pre.hasLeftNode) {
		findErrorRangeForExpressionNode(expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode,start,end);
		if (*start<possibleStart) possibleStart=*start;
		if (*end>possibleEnd) possibleEnd=*end;
	}
	if (thisNode->pre.hasRightNode) {
		findErrorRangeForExpressionNode(expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode,start,end);
		if (*start<possibleStart) possibleStart=*start;
		if (*end>possibleEnd) possibleEnd=*end;
	}
	*start=possibleStart;
	*end=possibleEnd;
}

void printInformativeMessageForExpression(bool isError,const char* message, ExpressionTreeNode* thisNode){
	int32_t indexes[2];
	findErrorRangeForExpressionNode(thisNode,indexes,indexes+1);
	printInformativeMessageAtSourceContainerIndex(isError,message,*indexes,*(indexes+1));
}

void printInformativeMessageForExpressionIndex(bool isError,const char* message, int16_t nodeIndex){
	printInformativeMessageForExpression(isError,message,expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex);
}


/*
typeString is usually a struct or union, but does not need to be
returns true if there is a const member
if a struct or union it encounters is not defined, an error is thrown at thisNode
this is intended to be used for assignment when it is assigning struct/union to 
  prevent assigning to a struct/union that is not const but a member of the struct/union is
*/
bool checkTypeStringForConstMembers(const char* typeString,ExpressionTreeNode* thisNode){
	bool hasConst;
	const char* typeStringNQ=stripQualifiersC(typeString,NULL,&hasConst);
	if (hasConst){
		return true;
	}
	struct TypeSearchResult tsr;
	const char* name;
	uint8_t t;
	if (isSectionOfStringEquivalent(typeStringNQ,0,"struct ")){
		t=0;  name=typeStringNQ+7;
	} else if (isSectionOfStringEquivalent(typeStringNQ,0,"union ")){
		t=1;  name=typeStringNQ+6;
	} else {
		return false;
	}
	searchForType(&tsr,name,t);
	if (!tsr.didExist){
		// I would be somewhat surprised if this is possible to reach anyway
		printf("The type name for the following error message is \"%s\"\n",typeStringNQ);
		printInformativeMessageForExpression(true,"The definition of a struct or union is required to do this",thisNode);
		exit(1);
	}
	struct TypeMemberEntry* tme;
	uint16_t len;
	if (tsr.isGlobal){
		struct GlobalTypeEntry* gte = blockFrameArray.globalBlockFrame.globalTypeEntries+tsr.typeEntryIndex;
		tme=gte->arrayOfMemberEntries;
		len=gte->numberOfMemberEntries;
	} else {
		struct BlockFrameTypeEntry* bfte = blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries+tsr.typeEntryIndex;
		tme=bfte->arrayOfMemberEntries;
		len=bfte->numberOfMemberEntries;
	}
	for (uint16_t i=0;i<len;i++){
		if (checkTypeStringForConstMembers(tme[i].typeString,thisNode)) return true;
	}
	return false;
}

void applyPrependWithInputReplace(const char* begin,char** inTS){
	char* tempTS=concatStrings(begin,*inTS);
	cosmic_free(*inTS);
	*inTS=tempTS;
}

void genTypeStringNQ(ExpressionTreeNode* thisNode){
	thisNode->post.typeStringNQ=stripQualifiers(thisNode->post.typeString,&thisNode->post.isRValueVolatile,&thisNode->post.isRValueConst);
}

void applyRvaluePointerToLvalueTransform(ExpressionTreeNode* thisNode){
	if (!(thisNode->post.typeStringNQ[0]=='*' & !thisNode->post.isLValue)){
		printf("Internal Error: cannot apply rvalue pointer to lvalue transform\n");
		exit(1);
	}
	char* new=copyStringToHeapString(thisNode->post.typeStringNQ+2);
	cosmic_free(thisNode->post.typeString);
	thisNode->post.typeString=new;
	genTypeStringNQ(thisNode);
	thisNode->post.isLValueConst=thisNode->post.isRValueConst;
	thisNode->post.isLValueVolatile=thisNode->post.isRValueVolatile;
	thisNode->post.isLValue=true;
}

void applyConvertToRvalue(ExpressionTreeNode* thisNode){
	if (!thisNode->post.isLValue){
		printf("Internal Error: value is already an Rvalue\n");
		exit(1);
	}
	thisNode->post.isLValue=false;
	bool decayedArray=applyToTypeStringBaseArrayDecayToSelf(thisNode->post.typeString);
	genTypeStringNQ(thisNode);
	if (decayedArray)return;
	uint32_t size = getSizeofForTypeString(thisNode->post.typeStringNQ,true);
	if (size==0){
		printInformativeMessageForExpression(true,"During lvalue to rvalue conversion, this object\'s type has unresolvable size",thisNode);
		exit(1);
	}
	if (isTypeStringOfStructOrUnion(thisNode->post.typeStringNQ)){
		printf("Internal Error: cannot convert struct or union to rvalue in that way\n");
		exit(1);
	}
	if (thisNode->ib.numberOfSlotsAllocated!=0){
		InstructionBuffer* ibMem;
		bool applySignedCharConvert=false;
		if (size==1){
			if (doStringsMatch(thisNode->post.typeStringNQ,"signed char")) applySignedCharConvert=true;
			if (thisNode->post.isLValueVolatile) ibMem=&ib_mem_byte_read_v;
			else ibMem=&ib_mem_byte_read_n;
		} else if (size==2){
			if (thisNode->post.isLValueVolatile) ibMem=&ib_mem_word_read_v;
			else ibMem=&ib_mem_word_read_n;
		} else if (size==4){
			if (thisNode->post.isLValueVolatile) ibMem=&ib_mem_dword_read_v;
			else ibMem=&ib_mem_dword_read_n;
		} else {
			printf("Internal Error: bad size for lvalue->rvalue\n");
			exit(1);
		}
		singleMergeIB(&thisNode->ib,ibMem);
		if (applySignedCharConvert) singleMergeIB(&thisNode->ib,&ib_char_s_TO_int_s);
	} else {
		printf("Error: constant expression tried to convert to rvalue. That transformation is not implemented\n");
		exit(1);
	}
}

uint16_t getTypeIdForCast(const char* ts){
	if (doStringsMatch(ts,"_Bool"))
		return 1;
	else if (doStringsMatch(ts,"signed char"))
		return 2;
	else if (doStringsMatch(ts,"char"))
		return 3;
	else if (doStringsMatch(ts,"int") || 
			doStringsMatch(ts,"short") || 
			isSectionOfStringEquivalent(ts,0,"enum "))
		return 4;
	else if (doStringsMatch(ts,"unsigned int") || 
			doStringsMatch(ts,"unsigned short"))
		return 5;
	else if (doStringsMatch(ts,"long"))
		return 6;
	else if (doStringsMatch(ts,"unsigned long"))
		return 7;
	else if (doStringsMatch(ts,"* void"))
		return 8;
	else if (isSectionOfStringEquivalent(ts,0,"* "))
		return 9;
	printf("Internal Error: type cast could not recognize type `%s`\n",ts);
	exit(1);
}


/*
probably not perfect, this is used for warning messages (mainly for auto type casts).
 typeString1 is from
 typeString2 is to
 hasHadPointer should generally be false when being called outside this function
*/
void warnForTypeCastChangeQualifiersOrTypeStructure(
		ExpressionTreeNode* thisNode,
		const char* typeString1,
		const char* typeString2,
		bool hasHadPointer,
		bool isForTypeCast,
		bool warnForQualifiers){

	bool c1;
	bool v1;
	bool c2;
	bool v2;
	const char* typeString1NQ=stripQualifiersC(typeString1,&v1,&c1);
	const char* typeString2NQ=stripQualifiersC(typeString2,&v2,&c2);
	if (isForTypeCast&warnForQualifiers){
		if ( v1&!v2) printInformativeMessageForExpression(false,"A volatile qualifier is being removed in this type",thisNode);
		if (!v1& v2) printInformativeMessageForExpression(false,"A volatile qualifier is being added in this type",thisNode);
		
		//this check below may cause some annoying warnings, but I think it's actually good
		if ( c1&!c2&hasHadPointer) printInformativeMessageForExpression(false,"A const qualifier is being removed in this type",thisNode);
	}
	if (doStringsMatch(typeString1NQ,typeString2NQ)) return;
	bool isSU1=isTypeStringOfStructOrUnion(typeString1NQ);
	bool isSU2=isTypeStringOfStructOrUnion(typeString2NQ);
	if (hasHadPointer){
		if (isSU1 & isSU2){
			if (isForTypeCast) printInformativeMessageForExpression(false,"Pointers to differing structs or unions are being type casted here",thisNode);
			else printInformativeMessageForExpression(false,"Pointers to differing structs or unions are being compared here",thisNode);
		} else if (isSU1 | isSU2){
			if (isForTypeCast) printInformativeMessageForExpression(false,"Pointers to a struct or union and a non struct or union are being type casted here",thisNode);
			else printInformativeMessageForExpression(false,"Pointers to a struct or union and a non struct or union are being compared here",thisNode);
		}
	}
	if (isSU1 | isSU2) return;
	if (doStringsMatch(typeString1NQ,"void")) return;
	if (doStringsMatch(typeString2NQ,"void")) return;
	uint16_t tID1=getTypeIdForCast(typeString1NQ);
	uint16_t tID2=getTypeIdForCast(typeString2NQ);
	if (tID1==9 & tID2==9){
		warnForTypeCastChangeQualifiersOrTypeStructure(thisNode,typeString1NQ+2,typeString2NQ+2,true,isForTypeCast,warnForQualifiers);
		return;
	}
	if (hasHadPointer){
		if (isForTypeCast) printInformativeMessageForExpression(false,"Pointers to differing types are being type casted here",thisNode);
		else printInformativeMessageForExpression(false,"Pointers to differing types are being compared here",thisNode);
	}
}

/*
typeStringTo should not have an identifier.
typeStringTo may have qualifiers.

there are 3 configuable types of warnings this can produce. Each bit in warnState signifies if that warning shall be given:
 1 - warn if pointer to pointer cast of unsame pointer (and neither is a void pointer)
 2 - warn if pointer to non-pointer (unless pointer is a void pointer)
 4 - warn if non-pointer to pointer (unless pointer is a void pointer)
 8 - warn if qualifiers are being changed or type structure is being changed

if a pointer to non-pointer cast is performed and the non-pointer type cannot hold the entire value of a pointer, then a warning is always given
if either type is a struct or union that will cause an error message (unless either type is void)
if typeStringTo is "void", then it is guaranteed to succeed and no warnings will be given.
*/
void applyTypeCast(ExpressionTreeNode* thisNode,const char* typeStringTo_input, uint16_t warnState){
	char* typeStringTo=applyToTypeStringBaseArrayDecayToNew(typeStringTo_input);
	if (applyToTypeStringBaseArrayDecayToSelf(thisNode->post.typeString)){
		assert(thisNode->post.isLValue); // how did an rvalue array exist at this stage?
		thisNode->post.isLValue=false;
	}
	genTypeStringNQ(thisNode);
	bool typeStringToHasVolatile;
	bool typeStringToHasConst;
	char* typeStringToNQ = stripQualifiers(typeStringTo,&typeStringToHasVolatile,&typeStringToHasConst);
	char* typeStringFromNQ = thisNode->post.typeStringNQ;
	bool wPtrToPtr = (warnState&0x0001)!=0;
	bool wPtrToNonPtr = (warnState&0x0002)!=0;
	bool wNonPtrToPtr = (warnState&0x0004)!=0;
	bool wQualifierStructure = (warnState&0x0008)!=0;
	bool sNonPtrToPtr=false;
	bool sPtrToNonPtr=false;
	bool sPtrToPtr=false;
	uint16_t typeIdForFrom;
	uint16_t typeIdForTo;
	uint32_t* thisConstVal=&thisNode->post.constVal;
	bool isFromVoid=doStringsMatch(typeStringFromNQ,"void");
	if (doStringsMatch(typeStringToNQ,"void")){
		if (isFromVoid){
			cosmic_free(typeStringTo);
			return;
		}
		uint16_t sizeOnStack=4;
		if (!thisNode->post.isLValue){
			sizeOnStack=getSizeofForTypeString(typeStringFromNQ,true);
			sizeOnStack+=(sizeOnStack&1);
			if (sizeOnStack==0){
				printf("Internal Error\n");
				exit(1);
			}
		}
		uint16_t sizeRemoved=0;
		while (sizeRemoved!=sizeOnStack){
			addVoidPop(&thisNode->ib);
			sizeRemoved+=2;
		}
		thisNode->post.isLValue=false;
		goto quickEnd;
	}
	if (isFromVoid){
		printInformativeMessageForExpression(true,"casting from void to non-void is not allowed",thisNode);
		exit(1);
	}
	if (isTypeStringOfStructOrUnion(typeStringFromNQ) ||
		isTypeStringOfStructOrUnion(typeStringToNQ)) {
		
		cosmic_free(typeStringTo);
		printf("Internal Error: cast got struct or union\n");
		exit(1);
	}
	if (thisNode->post.isLValue) applyConvertToRvalue(thisNode);
	if (doStringsMatch(thisNode->post.typeString,typeStringTo)) {
		cosmic_free(typeStringTo);
		return;
	}
	if (wQualifierStructure) warnForTypeCastChangeQualifiersOrTypeStructure(thisNode,thisNode->post.typeString,typeStringTo,false,true,true);
	typeIdForFrom=getTypeIdForCast(typeStringFromNQ);
	typeIdForTo=getTypeIdForCast(typeStringToNQ);
	bool isFromPtr=typeIdForFrom==8 | typeIdForFrom==9;
	bool isToPtr=typeIdForTo==8 | typeIdForTo==9;
	sNonPtrToPtr=!isFromPtr &  isToPtr;
	sPtrToNonPtr= isFromPtr & !isToPtr;
	if (typeIdForFrom==typeIdForTo | (isFromPtr&isToPtr)){
		sPtrToPtr = typeIdForFrom==9 & typeIdForTo==9 & !wQualifierStructure;
		goto quickEnd;
	}
	if (thisNode->ib.numberOfSlotsAllocated!=0){
		InstructionBuffer* ib_apply_0=NULL;
		InstructionBuffer* ib_apply_1=NULL;
		bool doLoad255=false;
		if (typeIdForFrom==1){
			if (typeIdForTo>=6) {
				ib_apply_1=&ib_int_u_TO_long;
			}
		} else if (typeIdForFrom==2){
			if (typeIdForTo==1){
				ib_apply_1=&ib_int_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo==3){
				doLoad255=true;
				ib_apply_1=&ib_b_and_int;
				*thisConstVal=0x00FF&(*thisConstVal);
			} else if (typeIdForTo==4 | typeIdForTo==5){
				ib_apply_1=&ib_char_s_TO_int_s;
				*thisConstVal=(int16_t)((char)(*thisConstVal));
			} else {
				ib_apply_0=&ib_char_s_TO_int_s;
				ib_apply_1=&ib_int_s_TO_long_s;
				*thisConstVal=(int32_t)((char)(*thisConstVal));
			}
		} else if (typeIdForFrom==3){
			if (typeIdForTo==1){
				ib_apply_1=&ib_int_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo>=6 & typeIdForTo<=9){
				ib_apply_1=&ib_int_u_TO_long;
			}
		} else if (typeIdForFrom==4){
			if (typeIdForTo==1){
				ib_apply_1=&ib_int_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo==2 | typeIdForTo==3){
				doLoad255=true;
				ib_apply_1=&ib_b_and_int;
				*thisConstVal=0x00FF&(*thisConstVal);
			} else if (typeIdForTo>=6 & typeIdForTo<=9){
				ib_apply_1=&ib_int_s_TO_long_s;
				*thisConstVal=(int32_t)((int16_t)(*thisConstVal));
			}
		} else if (typeIdForFrom==5){
			if (typeIdForTo==1){
				ib_apply_1=&ib_int_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo==2 | typeIdForTo==3){
				doLoad255=true;
				ib_apply_1=&ib_b_and_int;
				*thisConstVal=0x00FF&(*thisConstVal);
			} else if (typeIdForTo>=6 & typeIdForTo<=9){
				ib_apply_1=&ib_int_u_TO_long;
			}
		} else if (typeIdForFrom>=6 & typeIdForFrom<=8){
			if (typeIdForTo==1){
				ib_apply_1=&ib_long_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo==2 | typeIdForTo==3){
				doLoad255=true;
				ib_apply_0=&ib_long_TO_int;
				ib_apply_1=&ib_b_and_int;
				*thisConstVal=0x00FF&(*thisConstVal);
			} else if (typeIdForTo==4 | typeIdForTo==5){
				ib_apply_1=&ib_long_TO_int;
				*thisConstVal=0xFFFF&(*thisConstVal);
			}
		} else {
			if (typeIdForTo==1){
				ib_apply_1=&ib_long_TO_bool;
				*thisConstVal=(bool)(*thisConstVal);
			} else if (typeIdForTo==2 | typeIdForTo==3){
				doLoad255=true;
				ib_apply_0=&ib_long_TO_int;
				ib_apply_1=&ib_b_and_int;
				*thisConstVal=0x00FF&(*thisConstVal);
			} else if (typeIdForTo==4 | typeIdForTo==5){
				ib_apply_1=&ib_long_TO_int;
				*thisConstVal=0xFFFF&(*thisConstVal);
			}
		}
		if (ib_apply_0!=NULL) singleMergeIB(&thisNode->ib,ib_apply_0);
		if (doLoad255) insert_IB_load_byte(&thisNode->ib,255);
		if (ib_apply_1!=NULL) singleMergeIB(&thisNode->ib,ib_apply_1);
	}
	quickEnd:
	if (wPtrToPtr&sPtrToPtr) warnForTypeCastChangeQualifiersOrTypeStructure(thisNode,thisNode->post.typeString,typeStringTo,false,true,false);
	if (wPtrToNonPtr&sPtrToNonPtr) printInformativeMessageForExpression(false,"Casting this pointer to a non-pointer type",thisNode);
	if (wNonPtrToPtr&sNonPtrToPtr) printInformativeMessageForExpression(false,"Casting this non-pointer to a pointer type",thisNode);
	cosmic_free(thisNode->post.typeString);
	thisNode->post.typeString=typeStringTo;
	genTypeStringNQ(thisNode);
}

/*
typeStringTo should not have an identifier.
typeStringTo may have qualifiers.
*/
void pushAsParam(ExpressionTreeNode* thisNode,const char* typeStringTo){
	if (thisNode->ib.numberOfSlotsAllocated==0){
		// this should be unreachable
		printf("Internal Error: cannot push param in constant expression");
		exit(1);
	}
	if (isTypeStringOfStructOrUnion(thisNode->post.typeStringNQ)){
		if (!doStringsMatch(thisNode->post.typeStringNQ,stripQualifiersC(typeStringTo,NULL,NULL))){
			printInformativeMessageForExpression(true,"This struct or union is not the same type as the function specifies",thisNode);
			exit(1);
		}
		uint32_t size = getSizeofForTypeString(thisNode->post.typeStringNQ,true);
		if (size>64000){
			printInformativeMessageForExpression(true,"This struct or union cannot fit on the stack",thisNode);
			exit(1);
		}
		thisNode->post.sizeOnStack+=(uint16_t)size;
		if (thisNode->post.isLValue){
			addStructArgPush(&thisNode->ib,size,thisNode->post.isLValueVolatile);
		} // otherwise, the struct is already completely on the stack. The optimizer should be barely okay with not being told about this
	} else {
		applyTypeCast(thisNode,typeStringTo,15);
		applyTypeCast(thisNode,singleTypicalIntegralTypePromote(typeStringTo,NULL),15); // should this and the previous warn?
		uint16_t size = getSizeofForTypeString(thisNode->post.typeStringNQ,true);
		assert((size&1)==0);
		//if (size&1) size++;
		if (size==2) {
			thisNode->post.sizeOnStack+=2;
			addPopArgPush1(&thisNode->ib);
		} else {
			thisNode->post.sizeOnStack+=4;
			addPopArgPush2(&thisNode->ib);
		}
	}
}


uint16_t getNumberOfParametersOnFunctionCall(ExpressionTreeNode* expTreeNode){
	uint16_t n=0;
	if (!(expTreeNode->operatorID==65 | expTreeNode->operatorID==66)){
		printf("Internal Error: getNumberOfParametersOnFunctionCall() called wrong\n");
		exit(1);
	}
	if (expTreeNode->pre.hasRightNode){
		expTreeNode=expressionTreeGlobalBuffer.expressionTreeNodes+expTreeNode->pre.rightNode;
		n++;
		while (expTreeNode->pre.hasChainNode){
			expTreeNode=expressionTreeGlobalBuffer.expressionTreeNodes+expTreeNode->pre.chainNode;
			n++;
		}
	}
	return n;
}

struct IntegralTypePromoteResult{
	const char* typeString; // not heap allocated
	uint16_t rankValue;
	uint16_t rankValueNoSign;
	bool hasVolatile;
	bool hasConst;
	bool isSignedChar;
	bool isUnsignedChar; // or _Bool
	bool didSucceed;
};


// for use by applyAutoTypeConversion() . It's not exactly a rank, but it is like one. typeString should not have identifier
struct IntegralTypePromoteResult getRankOfPromotedTypeString(const char* typeString){
	struct IntegralTypePromoteResult itpr={0};
	const char* typeStringNQ=stripQualifiersC(typeString,&itpr.hasVolatile,&itpr.hasConst);
	itpr.didSucceed=!(itpr.hasVolatile&itpr.hasConst);
	if (!itpr.didSucceed){
		// this, at the time of writing, is possible to reach but needs to be invalid
		printf("Internal Error: itpr detected typestring with volatile and const applied to a single segment\n");
		exit(1);
	}
	if (
		doStringsMatch(typeStringNQ,"int") ||
		(itpr.isUnsignedChar|=doStringsMatch(typeStringNQ,"char")) ||
		(itpr.isSignedChar=doStringsMatch(typeStringNQ,"signed char")) ||
		doStringsMatch(typeStringNQ,"short") ||
		(itpr.isUnsignedChar|=doStringsMatch(typeStringNQ,"_Bool")) ||
		isSectionOfStringEquivalent(typeStringNQ,0,"enum ")
		){
		itpr.rankValue=0;
		itpr.typeString="int";
		if (itpr.hasVolatile) itpr.typeString="volatile int";
		else if (itpr.hasConst) itpr.typeString="const int";
	} else if (
		doStringsMatch(typeStringNQ,"unsigned int") ||
		doStringsMatch(typeStringNQ,"unsigned short")
		){
		itpr.rankValue=1;
		itpr.typeString="unsigned int";
		if (itpr.hasVolatile) itpr.typeString="volatile unsigned int";
		else if (itpr.hasConst) itpr.typeString="const unsigned int";
	} else if (
		doStringsMatch(typeStringNQ,"long")
		){
		itpr.rankValue=2;
		itpr.typeString="long";
		if (itpr.hasVolatile) itpr.typeString="volatile long";
		else if (itpr.hasConst) itpr.typeString="const long";
	} else if (
		doStringsMatch(typeStringNQ,"unsigned long")
		){
		itpr.rankValue=3;
		itpr.typeString="unsigned long";
		if (itpr.hasVolatile) itpr.typeString="volatile unsigned long";
		else if (itpr.hasConst) itpr.typeString="const unsigned long";
	} else itpr.didSucceed=false;
	itpr.rankValueNoSign=itpr.rankValue^(itpr.rankValue&1);
	return itpr;
}

// for use by applyAutoTypeConversion() . It's not exactly a rank, but it is like one.
const char* getTypeStringForCommonRank(uint16_t r0,uint16_t r1){
	uint16_t final=4;
	uint16_t r0r=r0^(r0&1);
	uint16_t r1r=r1^(r1&1);
	if (r0==r1){
		final=r0;
	} else if ((r0&1)==(r1&1)){
		if (r0>r1) final=r0;
		else final=r1;
	} else if ((r0&1)==1) {
		if (r0r>=r1r){
			final=r0;
		} else if (r0==1 & r1==2) {
			final=r1;
		} else {
			final=r0+1;
		}
	} else {
		if (r1r>=r0r){
			final=r1;
		} else if (r1==1 & r0==2) {
			final=r0;
		} else {
			final=r1+1;
		}
	}
	switch (final){
		case 0:return "int";
		case 1:return "unsigned int";
		case 2:return "long";
		case 3:return "unsigned long";
	}
	printf("Internal Error: integer rank invalid\n");
	exit(1);
}



struct AdvancedTypeInfo{
	bool isPtr;
	bool isSU; // isStructOrUnion
	ExpTreePostWalkData* post;
	ExpressionTreeNode* node;
	const char* tsDerefNQ;
	struct IntegralTypePromoteResult itpr;
};

struct AdvancedTypeInfo genAdvancedTypeInfo(ExpressionTreeNode* thisNode){
	genTypeStringNQ(thisNode);
	struct AdvancedTypeInfo this={0};
	this.node=thisNode;
	this.post=&(thisNode->post);
	this.isSU=isTypeStringOfStructOrUnion(this.post->typeStringNQ);
	this.isPtr=this.post->typeStringNQ[0]=='*';
	if (this.isPtr) this.tsDerefNQ=stripQualifiers(this.post->typeStringNQ+2,NULL,NULL);
	if (!this.isSU&!this.isPtr){
		this.itpr=getRankOfPromotedTypeString(this.post->typeString);
		if (!this.itpr.didSucceed){
			printf("Internal Error: itpr should succeed on <%s>\n",this.post->typeString);
			exit(1);
		}
	}
	return this;
}

void applyAutoTypeConversion(ExpressionTreeNode* thisNode);

void recursiveApplyAutoTypeConversionForSizeof(ExpressionTreeNode* thisNode){
	uint8_t oID=thisNode->operatorID;
	if ((oID>=38 & oID<=48) | oID==1 | oID==2 | oID==8 | oID==9){
		printInformativeMessageForExpression(true,"Cannot have value modifying operator inside constant expression",thisNode);
		exit(1);
	}
	if (oID==65 | oID==66){
		// it might actually be valid to the C standard to have function calls inside sizeof expressions, so this might have to be implemented
		printInformativeMessageForExpression(true,"Cannot have function call inside constant expression",thisNode);
		exit(1);
	}
	if (thisNode->pre.hasLeftNode)
		recursiveApplyAutoTypeConversionForSizeof(expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		recursiveApplyAutoTypeConversionForSizeof(expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode);
	ensureExpNodeInit(thisNode);
	applyAutoTypeConversion(thisNode);
}


void applyAutoTypeConversion_Ternary(ExpressionTreeNode* thisNode){
	ExpressionTreeNode* ln=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
	ExpressionTreeNode* rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
	if (thisNode->operatorID==36){
		ExpressionTreeNode* rrn=expressionTreeGlobalBuffer.expressionTreeNodes+rn->pre.rightNode;
		ExpressionTreeNode* rln=expressionTreeGlobalBuffer.expressionTreeNodes+rn->pre.leftNode;
		thisNode->post.typeString=copyStringToHeapString(rn->post.typeString);
		applyTypeCast(ln,"_Bool",15);
	} else {
		// then thisNode->operatorID==37
		if (isTypeStringOfStructOrUnion(rn->post.typeStringNQ)){
			printInformativeMessageForExpression(true,"This operand may not be a struct or union",rn);
			exit(1);
		}
		if (isTypeStringOfStructOrUnion(ln->post.typeStringNQ)){
			printInformativeMessageForExpression(true,"This operand may not be a struct or union",ln);
			exit(1);
		}
		bool isLvoid=doStringsMatch(ln->post.typeStringNQ,"void");
		bool isRvoid=doStringsMatch(rn->post.typeStringNQ,"void");
		if (isLvoid^isRvoid){
			if (isLvoid){
				printInformativeMessageForExpression(true,"This side of the ternary is void type and the other is not",ln);
			} else {
				printInformativeMessageForExpression(true,"This side of the ternary is void type and the other is not",rn);
			}
			exit(1);
		}
		assert(isLvoid==isRvoid);
		if (!isLvoid){
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			if (rn->post.typeStringNQ[0]=='*' |
				ln->post.typeStringNQ[0]=='*'){
				
				if (doStringsMatch(rn->post.typeStringNQ,ln->post.typeStringNQ)){
					if (!doStringsMatch(rn->post.typeString,ln->post.typeString)){
						printInformativeMessageForExpression(false,"Type qualifiers for these types differ, and so the qualifiers are being removed",thisNode);
						char* newForLeft =copyStringToHeapString(rn->post.typeStringNQ);
						char* newForRight=copyStringToHeapString(newForLeft);
						cosmic_free(rn->post.typeString);
						cosmic_free(ln->post.typeString);
						rn->post.typeString=newForLeft;
						ln->post.typeString=newForRight;
						genTypeStringNQ(rn);
						genTypeStringNQ(ln);
					}
				} else {
					printInformativeMessageForExpression(true,"The type behind the pointer for these two types differ, and thus I cannot resolve the types to a single type",thisNode);
					exit(1);
				}
			} else {
				struct IntegralTypePromoteResult itpr_r={0};
				struct IntegralTypePromoteResult itpr_l={0};
				itpr_r=getRankOfPromotedTypeString(rn->post.typeString);
				itpr_l=getRankOfPromotedTypeString(ln->post.typeString);
				if (!itpr_r.didSucceed|!itpr_l.didSucceed){
					printf("Internal Error: itpr_r and itpr_l should succeed\n");
					exit(1);
				}
				const char* tsc=getTypeStringForCommonRank(itpr_l.rankValue,itpr_r.rankValue);
				applyTypeCast(rn,tsc,15);
				applyTypeCast(ln,tsc,15);
			}
			if (!doStringsMatch(ln->post.typeString,rn->post.typeString)){
				printf("Internal Error: those types should match\n");
				exit(1);
			}
		}
		thisNode->post.typeString=copyStringToHeapString(rn->post.typeString);
	}
}

void applyAutoTypeConversion_Sizeof(ExpressionTreeNode* thisNode){
	thisNode->post.typeString=copyStringToHeapString("const unsigned long");
	thisNode->post.isLValue=false;
	char* typeStringToGetSize;
	ExpressionTreeNode* rn;
	if (thisNode->isArgumentTypeForSizeof){
		typeStringToGetSize = fullTypeParseAvoidAdd(sourceContainer.string,thisNode->argumentIndexStart,thisNode->argumentIndexEnd);
	} else {
		recursiveApplyAutoTypeConversionForSizeof(
			rn = expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode);
		typeStringToGetSize=rn->post.typeString;
	}
	if ((thisNode->post.extraVal=getSizeofForTypeString(typeStringToGetSize,true))==0){
		if (thisNode->isArgumentTypeForSizeof){
			printInformativeMessageAtSourceContainerIndex(true,"calculating sizeof this type failed",thisNode->argumentIndexStart,thisNode->argumentIndexEnd);
		} else {
			printInformativeMessageForExpression(true,"calculating sizeof this expression failed",rn);
		}
		exit(1);
	}
	cosmic_free(typeStringToGetSize);
}

void applyAutoTypeConversion_Identifier(ExpressionTreeNode* thisNode){
	if (sourceContainer.isStringForPreprocesser){
		thisNode->post.operatorTypeID=2;
		thisNode->post.extraVal=0;
		thisNode->post.typeString=copyStringToHeapString("unsigned long");
		return;
	}
	struct IdentifierSearchResult isr;
	char* identifier = copyStringSegmentToHeap(sourceContainer.string,thisNode->startIndexInString,thisNode->endIndexInString);
	searchForIdentifier(&isr,identifier,true,true,true,true,true);
	if (!isr.didExist){
		printInformativeMessageForExpression(true,"This identifier does not exist",thisNode);
		exit(1);
	}
	cosmic_free(identifier);
	if (isr.typeOfResult==IdentifierSearchResultIsVariable){
		thisNode->post.isLValue=true;
		if (isr.reference.variableReference.isGlobal){
			// global variable
			thisNode->post.operatorTypeID=1;
			struct GlobalVariableEntry* globalVariableEntry=
				blockFrameArray.globalBlockFrame.globalVariableEntries+
				isr.reference.variableReference.variableEntryIndex;
			thisNode->post.extraVal = globalVariableEntry->labelID;
			thisNode->post.typeString = applyToTypeStringRemoveIdentifierToNew(globalVariableEntry->typeString);
		} else {
			// local variable
			thisNode->post.operatorTypeID=2;
			thisNode->post.extraVal = getReversedOffsetForLocalVariable(&isr.reference.variableReference);
			thisNode->post.typeString = applyToTypeStringRemoveIdentifierToNew(
				blockFrameArray.entries[isr.reference.variableReference.blockFrameEntryIndex]
				.variableEntries[isr.reference.variableReference.variableEntryIndex].typeString);
		}
	} else if (isr.typeOfResult==IdentifierSearchResultIsTypeMember){
		// enum item
		thisNode->post.operatorTypeID=3;
		thisNode->post.isLValue=false;
		if (isr.reference.typeMemberReference.isGlobal){
			struct GlobalTypeEntry* globalTypeEntryPtr = 
				blockFrameArray.globalBlockFrame.globalTypeEntries+isr.reference.typeMemberReference.typeEntryIndex;
			thisNode->post.extraPtr = 
				globalTypeEntryPtr->arrayOfMemberEntries+isr.reference.typeMemberReference.memberEntryIndex;
			thisNode->post.typeString = concatStrings("const enum ",globalTypeEntryPtr->name);
		} else {
			struct BlockFrameTypeEntry* blockFrameTypeEntryPtr = 
				blockFrameArray.entries[isr.reference.typeMemberReference.blockFrameEntryIndex]
				.typeEntries+isr.reference.typeMemberReference.typeEntryIndex;
			thisNode->post.extraPtr = 
				blockFrameTypeEntryPtr->arrayOfMemberEntries+isr.reference.typeMemberReference.memberEntryIndex;
			thisNode->post.typeString = concatStrings("const enum ",blockFrameTypeEntryPtr->name);
		}
		struct NumberParseResult npr;
		const char* numberString = getIndexOfNthSpace(
			((struct TypeMemberEntry*)(thisNode->post.extraPtr))->typeString,1)+
			(((struct TypeMemberEntry*)(thisNode->post.extraPtr))->typeString+1);
		parseNumber(&npr,numberString,0,
			strlen(numberString));
		thisNode->post.extraVal=npr.valueUnion.value;
		if (npr.errorCode!=0){
			printf("Internal Error: error occured when number parsing for enum:(%d)\n",npr.errorCode);
			exit(1);
		}
	} else if (isr.typeOfResult==IdentifierSearchResultIsFunction){
		// function pointer
		thisNode->post.operatorTypeID=4;
		thisNode->post.isLValue=true;
		thisNode->post.isLValueConst=true;
		struct GlobalFunctionEntry* gfep = 
			blockFrameArray.globalBlockFrame.globalFunctionEntries+
			isr.reference.functionReference.functionEntryIndex;
		thisNode->post.extraPtr = gfep;
		thisNode->post.extraVal = gfep->labelID;
		thisNode->post.typeString = applyToTypeStringRemoveIdentifierToNew(gfep->typeString);
	} else {
		printf("Internal Error: IdentifierSearchResult does not make sense\n");
		exit(1);
	}
}

void applyAutoTypeConversion_Constant(ExpressionTreeNode* thisNode){
	if (thisNode->operatorID==61){
		int32_t startIndex = thisNode->startIndexInString;
		bool isWideLiteral = false;
		if (sourceContainer.string[startIndex]=='L'){
			startIndex++;
			isWideLiteral=true;
		}
		if (sourceContainer.string[startIndex]=='\"'){
			if (sourceContainer.isStringForPreprocesser){
				thisNode->post.operatorTypeID=1;
				thisNode->post.extraVal=0xFFFFFFFF;
				thisNode->post.typeString=copyStringToHeapString("unsigned long");
			} else {
				thisNode->post.operatorTypeID=2;
				thisNode->post.extraVal = addEntryForInitData(
					startIndex,false,isWideLiteral,false);
				thisNode->post.typeString=copyStringToHeapString("const * const char");
			}
		} else if (sourceContainer.string[startIndex]=='\''){
			startIndex++;
			uint8_t c0 = (uint8_t)(sourceContainer.string[startIndex]);
			uint16_t valueToSet = c0;
			if (c0=='\\'){
				uint8_t c1 = (uint8_t)(sourceContainer.string[startIndex+1]);
				if (c1=='0'){
					uint8_t c2 = (uint8_t)(sourceContainer.string[startIndex+2]);
					uint8_t c3 = (uint8_t)(sourceContainer.string[startIndex+3]);
					c2 = ucharAsciiAsNumberDigit(c2,true,false);
					c3 = ucharAsciiAsNumberDigit(c3,true,false);
					if (c2==200 | c3==200){
						printInformativeMessageAtSourceContainerIndex(
							true,"Escape sequence number not in octal range",startIndex,startIndex+4);
						exit(1);
					}
					valueToSet=c2*8+c3;
				} else if (c1=='x'){
					uint8_t c2 = (uint8_t)(sourceContainer.string[startIndex+2]);
					uint8_t c3 = (uint8_t)(sourceContainer.string[startIndex+3]);
					c2 = ucharAsciiAsNumberDigit(c2,false,true);
					c3 = ucharAsciiAsNumberDigit(c3,false,true);
					if (c2==200 | c3==200){
						printInformativeMessageAtSourceContainerIndex(
							true,"Escape sequence number not in hexadecimal range",startIndex,startIndex+4);
						exit(1);
					}
					if (isWideLiteral){
						uint8_t c4 = (uint8_t)(sourceContainer.string[startIndex+4]);
						uint8_t c5 = (uint8_t)(sourceContainer.string[startIndex+5]);
						c4 = ucharAsciiAsNumberDigit(c4,false,true);
						c5 = ucharAsciiAsNumberDigit(c5,false,true);
						if (c4==200 | c5==200){
							printInformativeMessageAtSourceContainerIndex(
								true,"Escape sequence number not in hexadecimal range",startIndex,startIndex+6);
							exit(1);
						}
						printf("Unimplemented Error: wide character literals are not currently supported\n");
						exit(1);
						// not implemented further, as it is currently not supported
					} else {
						valueToSet=c2*16+c3;
					}
				}else if (c1=='a') valueToSet=7;
				else if (c1=='b')  valueToSet=8;
				else if (c1=='f')  valueToSet=12;
				else if (c1=='n')  valueToSet=10;
				else if (c1=='r')  valueToSet=13;
				else if (c1=='t')  valueToSet=9;
				else if (c1=='v')  valueToSet=11;
				else if (c1=='\'') valueToSet=39;
				else if (c1=='\"') valueToSet=34;
				else if (c1=='\\') valueToSet=92;
				else if (c1=='?')  valueToSet=63;
				else{
					printInformativeMessageAtSourceContainerIndex(
						true,"Unrecognized escape sequence for character literal",startIndex,startIndex+1);
					exit(1);
				}
			}
			thisNode->post.extraVal=valueToSet;
			thisNode->post.operatorTypeID=1;
			thisNode->post.typeString=copyStringToHeapString("const int");
		} else {
			printf("Internal Error: not a string or character literal\n");
			exit(1);
		}
	} else {
		// then thisNode->operatorID==62
		struct NumberParseResult npr;
		parseNumber(
			&npr,
			sourceContainer.string,
			thisNode->startIndexInString,
			thisNode->endIndexInString);
		if (npr.errorCode!=0){
			printf("Number parsing error code:(%d)\n",npr.errorCode);
			printInformativeMessageForExpression(true,"Error occured when number parsing",thisNode);
			exit(1);
		}
		uint16_t thisSizeof = getSizeofForTypeString(npr.typeString,true);
		if ((thisSizeof&1)!=0){
			printf("Internal Error: number constant typeString not stack alligned\n");
			exit(1);
		}
		if (thisSizeof==0){
			printf("Internal Error: sizeof with number constant typeString failed\n");
			exit(1);
		}
		thisNode->post.extraVal=npr.valueUnion.value;
		thisNode->post.typeString=copyStringToHeapString(npr.typeString);
		thisNode->post.operatorTypeID=thisSizeof/2U;
	}
}

#include "ExpressionTypeCommon.c"

void applyAutoTypeConversion_Typical(ExpressionTreeNode* tn){
	uint8_t oID = tn->operatorID;
	bool selfAssignModDivFlag=false;  // used in a specific situation to help code reuse
	bool compSignedIsDifferent=false; // used in a specific situation to help code reuse
	ExpressionTreeNode* ln=NULL;
	ExpressionTreeNode* rn=NULL;
	
	struct AdvancedTypeInfo l={0};
	struct AdvancedTypeInfo r={0};
	struct AdvancedTypeInfo c={0};
	char** lTS=NULL;
	char** rTS=NULL;
	
	if (tn->pre.hasLeftNode){
		ln=expressionTreeGlobalBuffer.expressionTreeNodes+tn->pre.leftNode;
		if (applyToTypeStringBaseArrayDecayToSelf(ln->post.typeString)){
			assert(ln->post.isLValue); // how did an rvalue array type exist at this stage?
			ln->post.isLValue=false;
		}
		genTypeStringNQ(ln);
		lTS=&ln->post.typeString;
		if (oID!=49) l=genAdvancedTypeInfo(ln);
	}
	if (tn->pre.hasRightNode){
		rn=expressionTreeGlobalBuffer.expressionTreeNodes+tn->pre.rightNode;
		if (!(oID==5 | oID==6)){
			if (oID!=16){
				if (applyToTypeStringBaseArrayDecayToSelf(rn->post.typeString)){
					assert(rn->post.isLValue); // how did an rvalue array type exist at this stage?
					rn->post.isLValue=false;
				}
			}
			genTypeStringNQ(rn);
			rTS=&rn->post.typeString;
			if (!(oID==49 | oID==16)) r=genAdvancedTypeInfo(rn);
		}
	}
	ExpTreePostWalkData* tp=&tn->post;
	char** thisTSP=&tp->typeString;
	
	switch (oID){
		case 1:
		case 2:{
			etc_00(&l);
			etc_07(&l);
			*thisTSP=copyStringToHeapString(*lTS);
			tp->extraVal=1;
			if (l.isPtr) tp->extraVal=etc_03(&l);
			if (l.isPtr || l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else if (l.itpr.isSignedChar) tp->operatorTypeID=3;
			else if (l.itpr.isUnsignedChar) tp->operatorTypeID=4;
			else tp->operatorTypeID=2;
			if (ln->post.isLValueVolatile) tp->operatorTypeID+=4;
		}
		break;
		case 4:{
			etc_04(&l);
			etc_07(&r);
			applyTypeCast(rn,"unsigned long",15);
			tp->extraVal=etc_03(&l);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			applyRvaluePointerToLvalueTransform(ln);
			tp->isLValue=true;
			tp->isLValueConst =    ln->post.isLValueConst;
			tp->isLValueVolatile = ln->post.isLValueVolatile;
			*thisTSP=copyStringToHeapString(*lTS);
		}
		break;
		case 5:{
			etc_06(&l);
			etc_09(&l,rn,tn);
		}
		break;
		case 6:{
			etc_08(&l);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			applyRvaluePointerToLvalueTransform(ln);
			l=genAdvancedTypeInfo(ln);
			etc_09(&l,rn,tn);
		}
		break;
		case 8:
		case 9:{
			etc_00(&r);
			*thisTSP=copyStringToHeapString(*rTS);
			tp->isLValue=true;
			tp->isLValueVolatile=rn->post.isLValueVolatile;
			tp->extraVal=1;
			if (r.isPtr) tp->extraVal=etc_03(&r);
			if (r.isPtr || r.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else if (r.itpr.isSignedChar) tp->operatorTypeID=3;
			else if (r.itpr.isUnsignedChar) tp->operatorTypeID=4;
			else tp->operatorTypeID=2;
			if (rn->post.isLValueVolatile) tp->operatorTypeID+=4;
		}
		break;
		case 10:
		case 11:
		case 13:{
			etc_07(&r);
			etc_05(&r);
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			*thisTSP=copyStringToHeapString(*rTS);
			if (r.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else tp->operatorTypeID=2;
		}
		break;
		case 12:{
			etc_07(&r);
			etc_05(&r);
			applyTypeCast(rn,"_Bool",15);
			*thisTSP=copyStringToHeapString("_Bool");
		}
		break;
		case 14:{
			etc_07(&r);
			*thisTSP=fullTypeParseAvoidAdd(sourceContainer.string,tn->argumentIndexStart,tn->argumentIndexEnd);
			applyTypeCast(rn,*thisTSP,0);
		}
		break;
		case 15:{
			etc_04(&r);
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			applyRvaluePointerToLvalueTransform(rn);
			*thisTSP=copyStringToHeapString(*rTS);
			tp->isLValue=true;
			tp->isLValueConst =    rn->post.isLValueConst;
			tp->isLValueVolatile = rn->post.isLValueVolatile;
		}
		break;
		case 16:{
			etc_02(rn);
			*thisTSP=concatStrings("* ",*rTS);
			if (rn->post.isLValueConst) applyPrependWithInputReplace("const ",thisTSP);
			if (rn->post.isLValueVolatile) applyPrependWithInputReplace("volatile ",thisTSP);
		}
		break;
		case 18:
		case 19:
		case 20:
		case 31:
		case 32:
		case 33:{
			etc_05(&r);
			etc_05(&l);
			etc_07(&r);
			etc_07(&l);
			const char* tsc=getTypeStringForCommonRank(l.itpr.rankValue,r.itpr.rankValue);
			*thisTSP=copyStringToHeapString(tsc);
			applyTypeCast(rn,tsc,7);
			applyTypeCast(ln,tsc,7);
			c=genAdvancedTypeInfo(rn);
			if (c.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else tp->operatorTypeID=2;
			if ((oID==19 | oID==20) && c.itpr.rankValueNoSign!=c.itpr.rankValue) tp->operatorTypeID+=2;
		}
		break;
		case 21:{
			etc_07(&r);
			etc_07(&l);
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			if (l.isPtr & r.isPtr){
				printInformativeMessageForExpression(true,"cannot add pointer to pointer",tn);
				exit(1);
			} else if (l.isPtr){
				*thisTSP=copyStringToHeapString(*lTS);
				applyTypeCast(rn,"unsigned long",15);
				tp->extraVal=etc_03(&l);
				tp->operatorTypeID=3;
			} else if (r.isPtr){
				*thisTSP=copyStringToHeapString(*rTS);
				applyTypeCast(ln,"unsigned long",15);
				tp->extraVal=etc_03(&r);
				tp->operatorTypeID=4;
			} else {
				const char* tsc=getTypeStringForCommonRank(l.itpr.rankValue,r.itpr.rankValue);
				*thisTSP=copyStringToHeapString(tsc);
				applyTypeCast(rn,tsc,7);
				applyTypeCast(ln,tsc,7);
				c=genAdvancedTypeInfo(rn);
				if (c.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else tp->operatorTypeID=2;
			}
		}
		break;
		case 22:{
			etc_07(&r);
			etc_07(&l);
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			if (l.isPtr & r.isPtr){
				if (!doStringsMatch(l.tsDerefNQ,r.tsDerefNQ)){
					printInformativeMessageForExpression(true,"cannot subtract differing pointer to pointer",tn);
					exit(1);
				}
				*thisTSP=copyStringToHeapString("unsigned long");
				tp->extraVal=etc_03(&r);
				tp->operatorTypeID=4;
			} else if (l.isPtr){
				*thisTSP=copyStringToHeapString(*lTS);
				applyTypeCast(rn,"unsigned long",15);
				tp->extraVal=etc_03(&l);
				tp->operatorTypeID=3;
			} else if (r.isPtr){
				printInformativeMessageForExpression(true,"cannot subtract non pointer from pointer",tn);
				exit(1);
			} else {
				const char* tsc=getTypeStringForCommonRank(l.itpr.rankValue,r.itpr.rankValue);
				*thisTSP=copyStringToHeapString(tsc);
				applyTypeCast(rn,tsc,7);
				applyTypeCast(ln,tsc,7);
				c=genAdvancedTypeInfo(rn);
				if (c.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else tp->operatorTypeID=2;
			}
		}
		break;
		case 23:
		case 24:{
			etc_05(&r);
			etc_05(&l);
			etc_07(&r);
			etc_07(&l);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			applyTypeCast(rn,"unsigned int",15);
			*thisTSP=copyStringToHeapString(*lTS);
			if (l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else tp->operatorTypeID=2;
		}
		break;
		case 25:
		case 26:
		case 27:
		case 28:
		compSignedIsDifferent=true;
		case 29:
		case 30:{
			etc_07(&r);
			etc_07(&l);
			if (rn->post.isLValue) applyConvertToRvalue(rn);
			if (ln->post.isLValue) applyConvertToRvalue(ln);
			*thisTSP=copyStringToHeapString("_Bool");
			if (r.isPtr & l.isPtr){
				tp->operatorTypeID=1;
				warnForTypeCastChangeQualifiersOrTypeStructure(tn,*lTS,*rTS,false,false,false);
			} else if (r.isPtr){
				tp->operatorTypeID=1;
				printInformativeMessageForExpression(false,"pointer comparison with this non-pointer",ln);
				applyTypeCast(ln,*rTS,0); // already gave a warning so no warnings on type cast
			} else if (l.isPtr){
				tp->operatorTypeID=1;
				printInformativeMessageForExpression(false,"pointer comparison with this non-pointer",rn);
				applyTypeCast(rn,*lTS,0); // already gave a warning so no warnings on type cast
			} else {
				/*
				the following check produces a lot of warnings, so it is off for now
				*/
				/*
				if ((l.itpr.rankValueNoSign!=l.itpr.rankValue) ^ (r.itpr.rankValueNoSign!=r.itpr.rankValue)){
					printInformativeMessageForExpression(false,"integral comparison between signed and unsigned types",tn);
				}
				*/
				const char* tsc=getTypeStringForCommonRank(l.itpr.rankValue,r.itpr.rankValue);
				applyTypeCast(rn,tsc,7);
				applyTypeCast(ln,tsc,7);
				c=genAdvancedTypeInfo(rn);
				if (c.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else tp->operatorTypeID=2;
				if (compSignedIsDifferent & (c.itpr.rankValueNoSign!=c.itpr.rankValue)) tp->operatorTypeID+=2;
			}
		}
		break;
		case 34:
		case 35:{
			etc_07(&r);
			etc_07(&l);
			*thisTSP=copyStringToHeapString("_Bool");
			applyTypeCast(rn,"_Bool",15);
			applyTypeCast(ln,"_Bool",15);
		}
		break;
		case 38:{
			etc_00(&l);
			*thisTSP=copyStringToHeapString(*lTS);
			if (l.isSU | r.isSU){
				if (!r.isSU){
					printInformativeMessageForExpression(true,"cannot assign non struct or union to struct or union",tn);
					exit(1);
				}
				if (!l.isSU){
					printInformativeMessageForExpression(true,"cannot assign struct or union to non struct or union",tn);
					exit(1);
				}
				if (!doStringsMatch(ln->post.typeStringNQ,rn->post.typeStringNQ)){
					printInformativeMessageForExpression(true,"cannot assign differing struct or union",tn);
					exit(1);
				}
				tp->extraVal=getSizeofForTypeString(ln->post.typeStringNQ,true);
				if (tp->extraVal==0){
					printInformativeMessageForExpression(true,"cannot assign struct or union with unknown size",tn);
					exit(1);
				}
				if ((tp->extraVal&1)!=0){
					printf("Internal Error: struct union size must be word alligned\n");
					exit(1);
				}
				if (checkTypeStringForConstMembers(*lTS,tn)){
					printInformativeMessageForExpression(true,"cannot assign struct or union with const members",tn);
					exit(1);
				}
				tp->isLValue=true;
				tp->isLValueStructResultOfAssignment=true; // this makes the result of assigning structs not assignable
				if (rn->post.isLValue){
					tp->operatorTypeID=9;
					tp->isLValueVolatile=rn->post.isLValueVolatile;
					tp->isLValueConst=rn->post.isLValueConst;
					if (ln->post.isLValueVolatile) tp->operatorTypeID+=1;
					if (rn->post.isLValueVolatile) tp->operatorTypeID+=2;
				} else {
					tp->operatorTypeID=13;
					tp->isLValueVolatile=ln->post.isLValueVolatile;
					if (ln->post.isLValueVolatile) tp->operatorTypeID+=1;
				}
			} else {
				applyTypeCast(rn,*lTS,15);
				if (l.isPtr || l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else if (l.itpr.isSignedChar) tp->operatorTypeID=3;
				else if (l.itpr.isUnsignedChar) tp->operatorTypeID=4;
				else tp->operatorTypeID=2;
				if (ln->post.isLValueVolatile) tp->operatorTypeID+=4;
			}
		}
		break;
		case 39:
		case 40:{
			etc_00(&l);
			etc_05(&r);
			etc_07(&r);
			etc_07(&l);
			*thisTSP=copyStringToHeapString(*lTS);
			if (l.isPtr) {
				tp->extraVal=etc_03(&l);
				applyTypeCast(rn,"unsigned long",0);
			} else {
				applyTypeCast(rn,*lTS,15);
			}
			if (l.isPtr) tp->operatorTypeID=5;
			else if (l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else if (l.itpr.isSignedChar) tp->operatorTypeID=3;
			else if (l.itpr.isUnsignedChar) tp->operatorTypeID=4;
			else tp->operatorTypeID=2;
			if (ln->post.isLValueVolatile) tp->operatorTypeID+=5;
		}
		break;
		case 42:
		case 43:
		selfAssignModDivFlag=true;
		case 41:
		case 46:
		case 47:
		case 48:{
			etc_00(&l);
			etc_05(&r);
			etc_05(&l);
			etc_07(&r);
			etc_07(&l);
			*thisTSP=copyStringToHeapString(*lTS);
			applyTypeCast(rn,*lTS,15);
			if (selfAssignModDivFlag){
				if (l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else if (l.itpr.isSignedChar | l.itpr.isUnsignedChar) tp->operatorTypeID=3;
				else tp->operatorTypeID=2;
				if (ln->post.isLValueVolatile) tp->operatorTypeID+=3;
				if (l.itpr.rankValueNoSign!=l.itpr.rankValue | l.itpr.isSignedChar) tp->operatorTypeID+=6;
			} else {
				if (l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
				else if (l.itpr.isSignedChar) tp->operatorTypeID=3;
				else if (l.itpr.isUnsignedChar) tp->operatorTypeID=4;
				else tp->operatorTypeID=2;
				if (ln->post.isLValueVolatile) tp->operatorTypeID+=4;
			}
		}
		break;
		case 44:
		case 45:{
			etc_00(&l);
			etc_05(&r);
			etc_05(&l);
			etc_07(&r);
			etc_07(&l);
			*thisTSP=copyStringToHeapString(*lTS);
			applyTypeCast(rn,"unsigned int",15);
			if (l.itpr.rankValueNoSign==2) tp->operatorTypeID=1;
			else if (l.itpr.isSignedChar) tp->operatorTypeID=3;
			else if (l.itpr.isUnsignedChar) tp->operatorTypeID=4;
			else tp->operatorTypeID=2;
			if (ln->post.isLValueVolatile) tp->operatorTypeID+=4;
		}
		break;
		case 49:{
			applyTypeCast(ln,"void",0);
			*thisTSP=copyStringToHeapString(*rTS);
			tp->isLValue =         rn->post.isLValue;
			tp->isLValueConst =    rn->post.isLValueConst;
			tp->isLValueVolatile = rn->post.isLValueVolatile;
		}
		break;
		default:
		printf("Internal Error: invalid oID");
		exit(1);
	}
	
	
	// debug
	if (tn->post.typeString==NULL){
		printf("Internal Error: TypeString still null (%d)\n",oID);
		exit(1);
	}
	
}


/*
applies type convertions to operands for any node (except function calls)
inserts some information into thisNode->post
cosmic_frees typeStrings in child nodes
*/
void applyAutoTypeConversion(ExpressionTreeNode* thisNode){
	uint8_t oID = thisNode->operatorID;
	if (oID==59) applyAutoTypeConversion_Identifier(thisNode);
	else if (oID==17) applyAutoTypeConversion_Sizeof(thisNode);
	else if (oID==61 | oID==62) applyAutoTypeConversion_Constant(thisNode);
	else if (oID==36 | oID==37) applyAutoTypeConversion_Ternary(thisNode);
	else if (oID==65 | oID==66){
		printf("Internal Error: function call given to applyAutoTypeConversion()");
		exit(1);
	} else applyAutoTypeConversion_Typical(thisNode);
	
	// debug
	if (thisNode->post.typeString==NULL){
		printf("Internal Error: TypeString still null after (%d)\n",oID);
		exit(1);
	}
	#ifdef EXP_TO_ASSEMBLY_DEBUG
	printf(">%s<%d,%d,%d,%d\n",thisNode->post.typeString,thisNode->operatorID,thisNode->post.isLValue,thisNode->post.isLValueVolatile,thisNode->post.isLValueConst);
	#endif
	genTypeStringNQ(thisNode);
	if (sourceContainer.isStringForPreprocesser) applyTypeCast(thisNode,"unsigned long",0);
	if (thisNode->pre.hasLeftNode)
		cosmic_free(expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.leftNode].post.typeString);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6 | oID==17))
		cosmic_free(expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.rightNode].post.typeString);
}


void applyOperator(ExpressionTreeNode* thisNode){
	uint8_t oID = thisNode->operatorID;
	uint16_t operatorTypeID = thisNode->post.operatorTypeID;
	uint32_t extraVal = thisNode->post.extraVal;
	//InstructionBuffer ibTemp;
	InstructionBuffer* ib_core0;
	InstructionBuffer* ib_core1;
	InstructionBuffer* ib=&thisNode->ib;
	InstructionBuffer* ibLeft;
	InstructionBuffer* ibRight;
	if (thisNode->pre.hasLeftNode){
		ibLeft=&(expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.leftNode].ib);
	}
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6)){
		ibRight=&(expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.rightNode].ib);
	}
	switch (oID){
		case 1:{
			singleMergeIB(ib,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_rvalue_before(ib,&ib_i_32add,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_rvalue_before(ib,&ib_i_32add,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16add,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 2:{
			singleMergeIB(ib,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_rvalue_before(ib,&ib_i_32sub,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_rvalue_before(ib,&ib_i_32sub,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_rvalue_before(ib,&ib_i_16sub,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 4:{
			dualMergeIB(ib,ibLeft,ibRight);
			insert_IB_load_dword(ib,extraVal);
			dualMergeIB(ib,&ib_i_32mul,&ib_i_32add);
		}
		break;
		case 5:
		case 6:{
			singleMergeIB(ib,ibLeft);
		}
		break;
		case 8:{
			singleMergeIB(ib,ibRight);
			switch (operatorTypeID){
				case 1:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_lvalue(ib,&ib_i_32add,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_lvalue(ib,&ib_i_32add,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16add,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 9:{
			singleMergeIB(ib,ibRight);
			switch (operatorTypeID){
				case 1:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_lvalue(ib,&ib_i_32sub,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					insert_IB_load_dword(ib,extraVal);
					singleMergeIB(ib,&ib_stack_swp_22);
					insert_IB_apply_to_self_dword_lvalue(ib,&ib_i_32sub,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					insert_IB_load_word(ib,1);
					singleMergeIB(ib,&ib_stack_swp_21);
					insert_IB_apply_to_self_word_lvalue(ib,&ib_i_16sub,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 10:
		case 14:
		case 15:
		case 16:{
			singleMergeIB(ib,ibRight);
		}
		break;
		case 11:{
			singleMergeIB(ib,ibRight);
			if (operatorTypeID==1){
				insert_IB_load_dword(ib,0);
				dualMergeIB(ib,&ib_stack_swp_22,&ib_i_32sub);
			} else {
				insert_IB_load_word(ib,0);
				dualMergeIB(ib,&ib_stack_swp_11,&ib_i_16sub);
			}
		}
		break;
		case 12:{
			singleMergeIB(ib,ibRight);
			insert_IB_load_word(ib,1);
			singleMergeIB(ib,&ib_b_xor_int);
		}
		break;
		case 13:{
			if (operatorTypeID==1){
				ib_core0=&ib_b_not_long;
			} else {
				ib_core0=&ib_b_not_int;
			}
			dualMergeIB(ib,ibRight,ib_core0);
		}
		break;
		case 17:{
			insert_IB_load_dword(ib,extraVal);
		}
		break;
		case 18:{
			if (operatorTypeID==1){
				ib_core0=&ib_i_32mul;
			} else {
				ib_core0=&ib_i_16mul;
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 19:{
			if (operatorTypeID==1){
				ib_core0=&ib_i_32div_s_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_i_16div_s_s;				
			} else if (operatorTypeID==3){
				ib_core0=&ib_i_32div_u_u;
			} else {
				ib_core0=&ib_i_16div_u_u;
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 20:{
			if (operatorTypeID==1){
				ib_core0=&ib_i_32mod_s_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_i_16mod_s_s;
			} else if (operatorTypeID==3){
				ib_core0=&ib_i_32mod_u_u;
			} else {
				ib_core0=&ib_i_16mod_u_u;
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 21:{
			if (operatorTypeID==1){
				tripleMergeIB(ib,ibLeft,ibRight,&ib_i_32add);
			} else if (operatorTypeID==2){
				tripleMergeIB(ib,ibLeft,ibRight,&ib_i_16add);
			} else if (operatorTypeID==3){
				dualMergeIB(ib,ibLeft,ibRight);
				insert_IB_load_dword(ib,extraVal);
				dualMergeIB(ib,&ib_i_32mul,&ib_i_32add);
			} else {
				dualMergeIB(ib,ibRight,ibLeft);
				insert_IB_load_dword(ib,extraVal);
				dualMergeIB(ib,&ib_i_32mul,&ib_i_32add);
			}
		}
		break;
		case 22:{
			if (operatorTypeID==1){
				tripleMergeIB(ib,ibLeft,ibRight,&ib_i_32sub);
			} else if (operatorTypeID==2){
				tripleMergeIB(ib,ibLeft,ibRight,&ib_i_16sub);
			} else if (operatorTypeID==3){
				dualMergeIB(ib,ibLeft,ibRight);
				insert_IB_load_dword(ib,extraVal);
				dualMergeIB(ib,&ib_i_32mul,&ib_i_32sub);
			} else {
				tripleMergeIB(ib,ibLeft,ibRight,&ib_i_32sub);
				insert_IB_load_dword(ib,extraVal);
				singleMergeIB(ib,&ib_i_32div_u_u);
			}
		}
		break;
		case 23:{
			if (operatorTypeID==1){
				ib_core0=&ib_Lshift32;
			} else {
				ib_core0=&ib_Lshift16;				
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 24:{
			if (operatorTypeID==1){
				ib_core0=&ib_Rshift32;
			} else {
				ib_core0=&ib_Rshift16;				
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 25:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_leq_long_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_comp_leq_int_s;
			} else if (operatorTypeID==3){
				ib_core0=&ib_comp_leq_long_u;
			} else {
				ib_core0=&ib_comp_leq_int_u;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 26:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_geq_long_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_comp_geq_int_s;
			} else if (operatorTypeID==3){
				ib_core0=&ib_comp_geq_long_u;
			} else {
				ib_core0=&ib_comp_geq_int_u;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 27:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_l_long_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_comp_l_int_s;
			} else if (operatorTypeID==3){
				ib_core0=&ib_comp_l_long_u;
			} else {
				ib_core0=&ib_comp_l_int_u;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 28:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_g_long_s;
			} else if (operatorTypeID==2){
				ib_core0=&ib_comp_g_int_s;
			} else if (operatorTypeID==3){
				ib_core0=&ib_comp_g_long_u;
			} else {
				ib_core0=&ib_comp_g_int_u;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 29:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_eq_long;
			} else {
				ib_core0=&ib_comp_eq_int;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 30:{
			if (operatorTypeID==1){
				ib_core0=&ib_comp_neq_long;
			} else {
				ib_core0=&ib_comp_neq_int;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 31:{
			if (operatorTypeID==1){
				ib_core0=&ib_b_and_long;
			} else {
				ib_core0=&ib_b_and_int;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 32:{
			if (operatorTypeID==1){
				ib_core0=&ib_b_xor_long;
			} else {
				ib_core0=&ib_b_xor_int;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 33:{
			if (operatorTypeID==1){
				ib_core0=&ib_b_or_long;
			} else {
				ib_core0=&ib_b_or_int;			
			}
			tripleMergeIB(ib,ibLeft,ibRight,ib_core0);
		}
		break;
		case 34:{
			insert_IB_logic_and_with_jmp(ib,ibLeft,ibRight,++globalLabelID);
		}
		break;
		case 35:{
			insert_IB_logic_or_with_jmp(ib,ibLeft,ibRight,++globalLabelID);
		}
		break;
		case 36:{
			uint32_t l0=++globalLabelID;
			uint32_t l1=++globalLabelID;
			ExpressionTreeNode* rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
			ExpressionTreeNode* rln=expressionTreeGlobalBuffer.expressionTreeNodes+rn->pre.leftNode;
			ExpressionTreeNode* rrn=expressionTreeGlobalBuffer.expressionTreeNodes+rn->pre.rightNode;
			insert_IB_statement_if_else(ib,ibLeft,&rln->ib,&rrn->ib,l0,l1);
			destroyInstructionBuffer(&rln->ib);
			destroyInstructionBuffer(&rrn->ib);
		}
		break;
		case 38:{
			switch (operatorTypeID){
				case 1:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					tripleMergeIB(ib,ibRight,ibLeft,&ib_mem_byte_write_v);
				}
				break;
				case 9:{
					quadMergeIB(ib,ibRight,&ib_stack_dupe_dword,ibLeft,&ib_stack_swp_22);
					while (extraVal!=0){
						singleMergeIB(ib,&ib_mem_word_copy_n_n);
						extraVal-=2;
					}
					addQuadVoidPop(ib);
				}
				break;
				case 10:{
					quadMergeIB(ib,ibRight,&ib_stack_dupe_dword,ibLeft,&ib_stack_swp_22);
					while (extraVal!=0){
						singleMergeIB(ib,&ib_mem_word_copy_n_v);
						extraVal-=2;
					}
					addQuadVoidPop(ib);
				}
				break;
				case 11:{
					quadMergeIB(ib,ibRight,&ib_stack_dupe_dword,ibLeft,&ib_stack_swp_22);
					while (extraVal!=0){
						singleMergeIB(ib,&ib_mem_word_copy_v_n);
						extraVal-=2;
					}
					addQuadVoidPop(ib);
				}
				break;
				case 12:{
					quadMergeIB(ib,ibRight,&ib_stack_dupe_dword,ibLeft,&ib_stack_swp_22);
					while (extraVal!=0){
						singleMergeIB(ib,&ib_mem_word_copy_v_v);
						extraVal-=2;
					}
					addQuadVoidPop(ib);
				}
				break;
				case 13:{
					dualMergeIB(ib,ibRight,ibLeft);
					addStructStackAssign(ib,extraVal,false);
				}
				break;
				case 14:{
					dualMergeIB(ib,ibRight,ibLeft);
					addStructStackAssign(ib,extraVal,true);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 39:{
			if (operatorTypeID!=5 & operatorTypeID!=10) dualMergeIB(ib,ibRight,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32add,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					singleMergeIB(ib,ibRight);
					insert_IB_load_dword(ib,extraVal);
					dualMergeIB(ib,&ib_i_32mul,ibLeft);
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32add,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 6:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32add,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 7:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 8:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 9:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16add,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 10:{
					singleMergeIB(ib,ibRight);
					insert_IB_load_dword(ib,extraVal);
					dualMergeIB(ib,&ib_i_32mul,ibLeft);
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32add,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 40:{
			if (operatorTypeID!=5 & operatorTypeID!=10) dualMergeIB(ib,ibRight,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32sub,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					singleMergeIB(ib,ibRight);
					insert_IB_load_dword(ib,extraVal);
					dualMergeIB(ib,&ib_i_32mul,ibLeft);
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32sub,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 6:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32sub,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 7:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 8:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 9:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16sub,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 10:{
					singleMergeIB(ib,ibRight);
					insert_IB_load_dword(ib,extraVal);
					dualMergeIB(ib,&ib_i_32mul,ibLeft);
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32sub,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 41:
		ib_core0=&ib_i_32mul;
		ib_core1=&ib_i_16mul;
		goto SelfAssignStart;
		case 46:
		ib_core0=&ib_b_and_long;
		ib_core1=&ib_b_and_int;
		goto SelfAssignStart;
		case 47:
		ib_core0=&ib_b_xor_long;
		ib_core1=&ib_b_xor_int;
		goto SelfAssignStart;
		case 48:
		ib_core0=&ib_b_or_long;
		ib_core1=&ib_b_or_int;
		{
			SelfAssignStart:
			dualMergeIB(ib,ibRight,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,ib_core0,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 5:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,ib_core0,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 6:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 7:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 8:{
					insert_IB_apply_to_self_word_rvalue_after(ib,ib_core1,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 42:{
			dualMergeIB(ib,ibRight,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32div_s_s,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_s_s,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_s_s,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32div_u_u,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 5:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_u_u,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 6:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_u_u,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 7:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32div_s_s,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 8:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_s_s,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 9:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_s_s,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 10:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32div_u_u,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 11:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_u_u,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 12:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16div_u_u,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 43:{
			dualMergeIB(ib,ibRight,ibLeft);
			switch (operatorTypeID){
				case 1:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32mod_s_s,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 2:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_s_s,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 3:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_s_s,&ib_mem_sbyte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 4:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32mod_u_u,&ib_mem_dword_read_n,&ib_mem_dword_write_n);
				}
				break;
				case 5:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_u_u,&ib_mem_word_read_n,&ib_mem_word_write_n);
				}
				break;
				case 6:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_u_u,&ib_mem_byte_read_n,&ib_mem_byte_write_n);
				}
				break;
				case 7:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32mod_s_s,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 8:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_s_s,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 9:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_s_s,&ib_mem_sbyte_read_v,&ib_mem_byte_write_v);
				}
				break;
				case 10:{
					insert_IB_apply_to_self_dword_rvalue_after(ib,&ib_i_32mod_u_u,&ib_mem_dword_read_v,&ib_mem_dword_write_v);
				}
				break;
				case 11:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_u_u,&ib_mem_word_read_v,&ib_mem_word_write_v);
				}
				break;
				case 12:{
					insert_IB_apply_to_self_word_rvalue_after(ib,&ib_i_16mod_u_u,&ib_mem_byte_read_v,&ib_mem_byte_write_v);
				}
				break;
				
				default:goto BadOperatorTypeID;
			}
		}
		break;
		case 44:{
			printf("Unimplemented Error: bitwise shift fused with assignment is currently not implemented on the backend\n");
			exit(1);
		}
		break;
		case 45:{
			printf("Unimplemented Error: bitwise shift fused with assignment is currently not implemented on the backend\n");
			exit(1);
		}
		break;
		case 49:{
			dualMergeIB(ib,ibLeft,ibRight);
		}
		break;
		case 59:{
			if (operatorTypeID==1 | operatorTypeID==4){
				insert_IB_address_label(ib,extraVal);
			} else if (operatorTypeID==2){
				insert_IB_STPA(ib,extraVal);
			} else if (operatorTypeID==3){
				insert_IB_load_word(ib,extraVal);
			} else {
				goto BadOperatorTypeID;
			}
		}
		break;
		case 61:{
			if (operatorTypeID==1){
				insert_IB_load_word(ib,extraVal);
			} else if (operatorTypeID==2){
				insert_IB_address_label(ib,extraVal);
			} else {
				goto BadOperatorTypeID;
			}
		}
		break;
		case 62:{
			if (operatorTypeID==1){
				insert_IB_load_word(ib,extraVal);
			} else if (operatorTypeID==2){
				insert_IB_load_dword(ib,extraVal);
			} else {
				goto BadOperatorTypeID;
			}
		}
		break;
		default:
		printf("Internal Error: invalid oID");
		exit(1);
	}
	return;
	BadOperatorTypeID:
	printf("Internal Error: invalid operatorTypeID");
	exit(1);
}





void expressionToAssembly(
		int16_t nodeIndex,
		/*
		when functionTypeAnalysis!=NULL, then paramIndex is used 
		and the return value of this expression is casted to it's appropriate
		type for it's parameter in that function.
		
		functionTypeAnalysis.params[i].noIdentifierTypeString should be used, never functionTypeAnalysis.params[i].typeString
		*/
		struct FunctionTypeAnalysis* functionTypeAnalysis,
		uint16_t paramIndex){
	
	ExpressionTreeNode* thisNode = expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	#ifdef EXP_TO_ASSEMBLY_DEBUG
	printf("+%d\n",oID);
	#endif
	ensureExpNodeInit(thisNode);
	bool isFunctionCall = oID==65 | oID==66;
	struct GlobalFunctionEntry* functionEntryPtr; // only used when oID==65
	if (isFunctionCall){
		uint16_t numberOfParameters=getNumberOfParametersOnFunctionCall(thisNode);
		numberOfParameters=getNumberOfParametersOnFunctionCall(thisNode);
		if (oID==65){
			char* functionIdentifier = copyStringSegmentToHeap(sourceContainer.string,thisNode->startIndexInString,thisNode->endIndexInString);
			struct IdentifierSearchResult identifierSearchResult;
			searchForIdentifier(&identifierSearchResult,functionIdentifier,true,true,true,true,true);
			if (identifierSearchResult.didExist){
				if (identifierSearchResult.typeOfResult!=IdentifierSearchResultIsFunction){
					printInformativeMessageAtSourceContainerIndex(true,"This identifier is not a function, but is expected to be",thisNode->startIndexInString,thisNode->endIndexInString);
					exit(1);
				}
			} else {
				printInformativeMessageAtSourceContainerIndex(true,"This identifier does not exist",thisNode->startIndexInString,thisNode->endIndexInString);
				exit(1);
			}
			cosmic_free(functionIdentifier);
			functionEntryPtr=blockFrameArray.globalBlockFrame.globalFunctionEntries+identifierSearchResult.reference.functionReference.functionEntryIndex;
			if (functionEntryPtr->functionTypeAnalysis.numberOfParameters!=numberOfParameters){
				uint8_t errorID=0;
				if (functionEntryPtr->usesVaArgs){
					if (numberOfParameters<(functionEntryPtr->functionTypeAnalysis.numberOfParameters-1U)) errorID=2;
				} else errorID=1;
				if (errorID!=0){
					char* stringForError = cosmic_calloc(100,1);
					if (errorID==1)
						snprintf(stringForError,99,
						"This function takes %u arguments, not %u",
						(unsigned int)(functionEntryPtr->functionTypeAnalysis.numberOfParameters),
						(unsigned int)numberOfParameters);
					else
						snprintf(stringForError,99,
						"This function takes at least %u arguments, not %u",
						(unsigned int)(functionEntryPtr->functionTypeAnalysis.numberOfParameters-1U),
						(unsigned int)numberOfParameters);
					printInformativeMessageAtSourceContainerIndex(true,stringForError,thisNode->startIndexInString,0);
					// pointless to free stringForError, program is about to exit
					exit(1);
				}
			}
		} else {
			printf("complex function call not ready yet\n");
			exit(1);
		}
	}
	ExpressionTreeNode* rightNode;
	ExpressionTreeNode* leftNode;
	if (thisNode->pre.hasRightNode) rightNode=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
	if (thisNode->pre.hasLeftNode) {
		leftNode=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
		expressionToAssembly(thisNode->pre.leftNode,NULL,0);
	}
	if (isFunctionCall){
		if (thisNode->pre.hasRightNode) expressionToAssembly(thisNode->pre.rightNode,&(functionEntryPtr->functionTypeAnalysis),0);
	} else if (thisNode->pre.hasRightNode&!(oID==5 | oID==6 | oID==17)) expressionToAssembly(thisNode->pre.rightNode,NULL,0);
	
	if (thisNode->pre.hasChainNode) {
		expressionToAssembly(thisNode->pre.chainNode,functionTypeAnalysis,paramIndex+1);
		ExpressionTreeNode* chainNode = expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.chainNode;
		// we now take the InstructionBuffer that the chain node allocated and claim it as our own
		thisNode->ib=chainNode->ib;
		thisNode->post.sizeOnStack=chainNode->post.sizeOnStack;
		chainNode->ib.numberOfSlotsAllocated=0;
		// now, treat the ib on the chainNode as destroyed, because it is now this node's ib
	} else if (oID!=37){
		// then there is no node chain to allocate our node for us, so this node allocates one for itself.
		// right part of ternary does not get an instruction buffer
		initInstructionBuffer(&thisNode->ib);
	}
	if (isFunctionCall){
		if (oID==65){
			uint16_t stackSize=0;
			if (thisNode->pre.hasRightNode){
				stackSize=rightNode->post.sizeOnStack;
				singleMergeIB(&thisNode->ib,&rightNode->ib);
			}
			insert_IB_call_nonComplex_function(
				&thisNode->ib,functionEntryPtr->labelID,stackSize,
				!doStringsMatch("void",
					stripQualifiersC(functionEntryPtr->functionTypeAnalysis.returnType,NULL,NULL)));
			thisNode->post.typeString=copyStringToHeapString(
				singleTypicalIntegralTypePromote(functionEntryPtr->functionTypeAnalysis.returnType,NULL)); // this does trim off the qualifiers on the base of the type. is that desired?
			genTypeStringNQ(thisNode);
		} else {
			printf("Unimplemented Error: complex function calls are not ready yet\n");
			exit(1);
		}
	} else {
		applyAutoTypeConversion(thisNode);
		if (oID==37) return; // right part of ternary needs to return now
		applyOperator(thisNode);
	}
	if (thisNode->pre.hasLeftNode) destroyInstructionBuffer(&leftNode->ib);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6 | oID==17)) destroyInstructionBuffer(&rightNode->ib);
	if (functionTypeAnalysis!=NULL){
		// then this must be pushed as a parameter
		const char* typeStringTo;
		if ((functionTypeAnalysis->numberOfParameters-functionTypeAnalysis->usesVaArgs)<=paramIndex){
			if (!functionTypeAnalysis->usesVaArgs){
				printf("Internal Error: serious vaArg related problem\n");
				exit(1);
			}
			typeStringTo=thisNode->post.typeString;
		} else {
			typeStringTo=functionTypeAnalysis->params[paramIndex].noIdentifierTypeString;
		}
		pushAsParam(thisNode,typeStringTo);
		cosmic_free(thisNode->post.typeString); // parameters free their own type string, because it's type is not needed after.
	}
	#ifdef EXP_TO_ASSEMBLY_DEBUG
	printf("-%d\n",oID);
	#endif
}


void expressionToAssemblyWithCast(
		InstructionBuffer *ib_to,
		const char* castTypeString,
		int32_t startIndexForExpression,
		int32_t endIndexForExpression){
	int16_t indexOfRoot = buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,startIndexForExpression,endIndexForExpression,true);
	bool isVoidCast=doStringsMatch("void",castTypeString);
	if (indexOfRoot==-1){
		if (isVoidCast){
			return;
		} else {
			printInformativeMessageAtSourceContainerIndex(true,"Expression is required here",startIndexForExpression,endIndexForExpression);
			exit(1);
		}
	}
	ExpressionTreeNode* rootNode=expressionTreeGlobalBuffer.expressionTreeNodes+indexOfRoot;
	expressionToAssembly(indexOfRoot,NULL,0);
	applyTypeCast(rootNode,castTypeString,15);
	singleMergeIB(ib_to,&rootNode->ib);
	destroyInstructionBuffer(&rootNode->ib);
	cosmic_free(rootNode->post.typeString);
}


// this is for expressions inside a return statement
void expressionToAssemblyWithReturn(
		InstructionBuffer *ib_to,
		int32_t startIndexForExpression,
		int32_t endIndexForExpression,
		const char* returnTypeString)
{
	int16_t indexOfRoot = buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,startIndexForExpression,endIndexForExpression,true);
	if (indexOfRoot==-1){
		printInformativeMessageAtSourceContainerIndex(true,"Expression is required here",startIndexForExpression,endIndexForExpression);
		exit(1);
	}
	int16_t indexForReturnIdentifier=partitionNewExpressionTreeNodeInGlobalBufferAndReturnIndex();
	int16_t indexForAssignment=partitionNewExpressionTreeNodeInGlobalBufferAndReturnIndex();
	expressionToAssembly(indexOfRoot,NULL,0);
	ExpressionTreeNode* returnIdentifier=expressionTreeGlobalBuffer.expressionTreeNodes+indexForReturnIdentifier;
	ExpressionTreeNode* assignment=expressionTreeGlobalBuffer.expressionTreeNodes+indexForAssignment;
	ExpressionTreeNode* previousRoot=expressionTreeGlobalBuffer.expressionTreeNodes+indexOfRoot;
	if (!isTypeStringOfStructOrUnion(returnTypeString)){
		applyTypeCast(previousRoot,returnTypeString,15);
		applyTypeCast(previousRoot,singleTypicalIntegralTypePromote(returnTypeString,NULL),15); // should this and the previous warn?
	}
	struct IdentifierSearchResult isr;
	searchForIdentifier(&isr,"__FUNCTION_RET_VALUE_PTR",false,true,true,false,false);
	if (!isr.didExist | isr.reference.variableReference.isGlobal | (isr.typeOfResult!=IdentifierSearchResultIsVariable)){
		printf("Internal Error: return could not find internal identifier\n");
		exit(1);
	}
	ensureExpNodeInit(returnIdentifier);
	ensureExpNodeInit(assignment);
	initInstructionBuffer(&returnIdentifier->ib);
	initInstructionBuffer(&assignment->ib);
	assignment->startIndexInString=(returnIdentifier->startIndexInString=previousRoot->startIndexInString);
	assignment->endIndexInString=(returnIdentifier->endIndexInString=previousRoot->endIndexInString);
	returnIdentifier->operatorID=59;
	returnIdentifier->post.operatorTypeID=2;
	returnIdentifier->post.isLValue=true;
	returnIdentifier->post.extraVal = getReversedOffsetForLocalVariable(&isr.reference.variableReference);
	returnIdentifier->post.typeString=copyStringToHeapString("unsigned int");
	genTypeStringNQ(returnIdentifier);
	insert_IB_STPA(&returnIdentifier->ib,returnIdentifier->post.extraVal);
	char* temp=concatStrings("* ",singleTypicalIntegralTypePromote(returnTypeString,NULL));
	applyTypeCast(returnIdentifier,temp,0);
	cosmic_free(temp);
	applyRvaluePointerToLvalueTransform(returnIdentifier);
	assignment->operatorID=38;
	assignment->pre.hasChainNode=false;
	assignment->pre.hasLeftNode=true;
	assignment->pre.hasRightNode=true;
	assignment->pre.leftNode=indexForReturnIdentifier;
	assignment->pre.rightNode=indexOfRoot;
	applyAutoTypeConversion(assignment);
	applyOperator(assignment);
	applyTypeCast(assignment,"void",0);
	singleMergeIB(ib_to,&assignment->ib);
	destroyInstructionBuffer(&assignment->ib);
	destroyInstructionBuffer(&returnIdentifier->ib);
	destroyInstructionBuffer(&previousRoot->ib);
	cosmic_free(assignment->post.typeString);
}

#include "InitializerMapping.c"


// this is for declarations with some sort of initialization
// this will probably be able to handle initializations at file scope
void expressionToAssemblyWithInitializer(
		InstructionBuffer *ib_to,
		int32_t startIndexForInitializer,
		int32_t endIndexForInitializer,
		int32_t startIndexForDeclaration,
		int32_t endIndexForDeclaration,
		bool usedRegister,
		bool usedStatic)
{
	if (usedRegister&usedStatic){
		printInformativeMessageAtSourceContainerIndex(true,"Cannot use 'register' and 'static' at the same time",startIndexForDeclaration,0);
		exit(1);
	}
	if (usedStatic){
		printInformativeMessageAtSourceContainerIndex(true,"Static in functions is not ready yet",startIndexForDeclaration,0);
		exit(1);
	}
	int32_t initializerRoot=initializerMapRoot(startIndexForInitializer,endIndexForInitializer);
	uint8_t initalizerCatagory=initializerMap.entries[initializerRoot].typeOfEntry;
	
	
	char* typeString = fullTypeParseAndAdd(sourceContainer.string,startIndexForDeclaration,endIndexForDeclaration,false);
	char* typeStringNI = applyToTypeStringRemoveIdentifierToNew(typeString); // what about potentially having multiple identifiers?
	char* identifier = applyToTypeStringGetIdentifierToNew(typeString);
	cosmic_free(typeString);
	InstructionBuffer ibTemp;
	struct RawMemoryForInitializer rmfi={0};
	initInstructionBuffer(&ibTemp);
	// this is not finished
	if (initializerImplementRoot(&rmfi,&ibTemp,initializerRoot,&typeStringNI,usedStatic)){
		
		
		cosmic_free(rmfi.mem);
		cosmic_free(rmfi.isSet);
	} else {
		
	}
	typeString=tripleConcatStrings(identifier," ",typeStringNI);
	
	addVariableToBlockFrame(typeString,usedRegister,usedStatic);
	struct IdentifierSearchResult isr;
	searchForIdentifier(&isr,identifier,false,true,true,false,false);
	if (!isr.didExist | isr.typeOfResult!=IdentifierSearchResultIsVariable){
		printf("Internal Error: could not find variable after adding to blockframe\n");
		exit(1);
	}
	// the following doesn't work for global, which it would be if it was static
	uint16_t revOffset = getReversedOffsetForLocalVariable(&isr.reference.variableReference);
	
	
	convertSTPI_STPA(&ibTemp,revOffset);
	singleMergeIB(ib_to,&ibTemp);
	destroyInstructionBuffer(&ibTemp);
	cosmic_free(typeString);
	cosmic_free(identifier);
	cosmic_free(typeStringNI);
	
	
	
	
	
	
	
	
}




