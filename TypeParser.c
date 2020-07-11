

#include "Common.c"


/*
NOTES:

 - bitfields are not implemented at all. Attempting to use them could cause weird behaviour, because there is nothing checking for them.
 - "const" and "volatile" will appear directly before the thing that is declared const or volatile.
 - in some rare cases, if the type does not have an identifier and it's a very complex type, 
     it may not recognize the starting point correctly, 
     and soon after would throw an error when it sees that something is invalid.
 - I am unsure if the way the enum assigns values automatically is the way it should be done. As of now, it assigns values in increasing order (starting at 1) with a value that is not already taken.
     Further, it only accepts base 10 numbers with no suffix
 - I think everything is working, except for the things I just mentioned of course.
 - This file is to be included by "TypeConverter.c"

 TODO: 
 - write check to recursively apply "const" and "volatile" to struct and union members
 - write sanity checker for type strings
*/



/*
this (or the next 2) is probably the function that you are looking for if you are looking for a public-like function that deals with parsing types.
returns a new string on the heap of the result, which is seperated by spaces (without an ending space).
struct, enum, union will preface the name or a bracket (and the name may be followed by a bracket)
"long int" does NOT become "int long", and other things of that sort (I got that working!)

Remember, what comes out of here is not finished. It needs to have the structure declarations taken out and typedefs resolved by "breakDownTypeAndAdd()"
*/
char* convertType(int32_t startIndex, int32_t endIndex);

/*
finds the endIndex for a C style declaration, primarily for convertType(). Does not include the brackets for function declarations
doesn't check must about the type declaration to see it's validity
*/
int32_t findEndIndexForConvertType(char *string, int32_t startIndex){
	bool wasCloseParenLast = false; // this isn't set to false for spaces and newlines when they are last
	char c;
	uint16_t enclosementLevel = 0; // unsigned is kinda important
	int32_t i=startIndex;
	while (string[i]!=0){
		c = string[i];
		if (c=='{' & wasCloseParenLast){
			if (enclosementLevel!=0){
				break; // this causes the fail condition at the end of the function
			}
			return i; // this should be a function declaration
		} else if (c=='{' | c=='[' | c=='('){
			++enclosementLevel;
		} else if (c=='}' | c==']' | c==')'){
			if (enclosementLevel==0){
				break; // this causes the fail condition at the end of the function
			}
			if (c==')'){
				wasCloseParenLast = true;
			}
			--enclosementLevel;
		} else if (c==';' & enclosementLevel==0){
			return i; // this is for most things
		} else if (c=='=' & enclosementLevel==0){
			return i; // this is for things that are initialized at the declaration
		}
		if (c!=' ' & c!='\n' & c!=')'){
			wasCloseParenLast = false;
		}
		i++;
	}	
	printf("error when trying to find end for that type declaration\n"); // for an index, it is probably best to use the startIndex
	exit(1);
}

// doesn't check for validity of identifier (if it exists)
// expects that the type string has had typedef expansion on it already
// in various places, it is needed to know if a type string has an identifier at the beginning
bool doesThisTypeStringHaveAnIdentifierAtBeginning(const char* string){
	int32_t indexOfSpace = getIndexOfFirstSpaceInString(string);
	if (indexOfSpace==-1){
		// not going to check for things that must have spaces in them
		if (doStringsMatch(string,"char") || 
			doStringsMatch(string,"short") ||
			doStringsMatch(string,"int") ||
			doStringsMatch(string,"long") ||
			doStringsMatch(string,"float") ||
			doStringsMatch(string,"double") ||
			doStringsMatch(string,"void") ||
			doStringsMatch(string,"_Bool")){
			
			return false;
		}
		printf("Internal Error: This type string has an identifier but no type. That is just wrong. [%s]\n",string);
		exit(1);
	} else {
		return !(
			isSectionOfStringEquivalent(string,0,"char ") ||
			isSectionOfStringEquivalent(string,0,"short ") ||
			isSectionOfStringEquivalent(string,0,"int ") ||
			isSectionOfStringEquivalent(string,0,"long ") ||
			isSectionOfStringEquivalent(string,0,"float ") ||
			isSectionOfStringEquivalent(string,0,"double ") ||
			isSectionOfStringEquivalent(string,0,"void ") ||
			isSectionOfStringEquivalent(string,0,"signed ") ||
			isSectionOfStringEquivalent(string,0,"unsigned ") ||
			isSectionOfStringEquivalent(string,0,"_Bool ") ||
			isSectionOfStringEquivalent(string,0,"struct ") ||
			isSectionOfStringEquivalent(string,0,"union ") ||
			isSectionOfStringEquivalent(string,0,"enum ") ||
			isSectionOfStringEquivalent(string,0,"const ") ||
			isSectionOfStringEquivalent(string,0,"volatile ") ||
			isSectionOfStringEquivalent(string,0,"* ") ||
			isSectionOfStringEquivalent(string,0,"[ ") ||
			isSectionOfStringEquivalent(string,0,"( ")
			);
	}
}



bool isArrayBracketsInvalidOnTypeString(const char* string);

uint16_t numberInterpreterForEnumEnumerationNormalizer(char* string, int32_t indexOfStartOfNumber){
	int32_t indexOfEndSpace = indexOfStartOfNumber;
	for (int32_t i=indexOfStartOfNumber;string[i];i++){
		if (string[i]==' '){
			indexOfEndSpace=i;
			break;
		}
	}
	uint16_t number = 0;
	for (int32_t i=indexOfStartOfNumber;i<indexOfEndSpace;i++){
		if (string[i]<'0' | string[i]>'9'){
			printf("That is not a base 10 digit in that enumeration initilization number\n");
			exit(1);
		}
		number = number * 10 + (string[i]-'0');
	}
	return number;
}

// the input string has cosmic_realloc() called on it, which is then returned
char* giveNumberToSingleEnumEnumerator(char* string, int32_t indexOfEnumerator, uint16_t numberToPlace){
	char buffer[10] = {0};
	snprintf(buffer,9," = %u",(unsigned int)numberToPlace);
	int16_t lengthToAdd = strlen(buffer);
	int32_t newLength = lengthToAdd + strlen(string);
	string = cosmic_realloc(string,newLength+1);
	int32_t indexOfSpaceAfterEnumerator = indexOfEnumerator;
	for (int32_t i=indexOfEnumerator;string[i];i++){
		if (string[i]==' '){
			indexOfSpaceAfterEnumerator = i; // this should happen
			break;
		}
	}
	// this copies the null terminator
	for (int32_t i=newLength;i>=indexOfSpaceAfterEnumerator;i--){
		string[i]=string[i-lengthToAdd];
	}
	for (int16_t i=0;i<lengthToAdd;i++){
		string[i+indexOfSpaceAfterEnumerator]=buffer[i];
	}
	return string;
}


// the input string probably has cosmic_realloc() called on it, which is then returned
char* giveNumbersToSingleEnumList(char* string, int32_t indexOfEnum){
	if (string[indexOfEnum]!='{'){
		printf("giveNumbersToSingleEnumList() called wrong\n");
		exit(1);
	}
	if (string[indexOfEnum+2]=='}'){
		printf("enum cannot have no enumerations in it\n"); // a little hard to get an index in the main string into here...
		exit(1);
	}
	int32_t endBracketIndex = getIndexOfMatchingEnclosement(string,indexOfEnum);
	uint16_t numberOfEnumerators = 1;
	{
		int32_t lastCommaIndex = 0;
		for (int32_t i=indexOfEnum;i<endBracketIndex;i++){
			if (string[i]==','){
				numberOfEnumerators++;
				lastCommaIndex = i;
			}
		}
		if (lastCommaIndex!=0 & lastCommaIndex+2==endBracketIndex){
			// Ha! you have an enum definition with a trailing comma. I can fix that.
			for (int32_t i=lastCommaIndex+2;string[i-1];i++){
				// the "string[i-1]" is to copy down the null terminator as well
				string[i-2]=string[i];
			}
			return giveNumbersToSingleEnumList(string,indexOfEnum);
		}
	}
	struct EnumEnumerator{
		int32_t startIndex;
		uint16_t number;
		bool didHaveNumberAlreadyThere;
	};
	struct EnumEnumerator* enumEnumerators = cosmic_malloc(
		numberOfEnumerators*sizeof(struct EnumEnumerator));
	enumEnumerators[0].startIndex = indexOfEnum+2;
	uint16_t walkingIndex = 1;
	for (int32_t i=indexOfEnum;i<endBracketIndex;i++){
		if (string[i]==','){
			enumEnumerators[walkingIndex++].startIndex = i+2;
		}
	}
	for (uint16_t enumeratorIndex=0;enumeratorIndex<numberOfEnumerators;enumeratorIndex++){
		enumEnumerators[enumeratorIndex].number = 0;
		enumEnumerators[enumeratorIndex].didHaveNumberAlreadyThere = false;
		for (int32_t i=enumEnumerators[enumeratorIndex].startIndex;i<endBracketIndex;i++){
			char c=string[i];
			if (c=='='){
				enumEnumerators[enumeratorIndex].didHaveNumberAlreadyThere = true;
				enumEnumerators[enumeratorIndex].number = numberInterpreterForEnumEnumerationNormalizer(string,i+2);
				break;
			}
			if (c==',' | c=='}'){
				break; // this data was already placed as default
			}
		}
	}
	for (uint16_t enumeratorIndex_1=0;enumeratorIndex_1<numberOfEnumerators;enumeratorIndex_1++){
		if (!enumEnumerators[enumeratorIndex_1].didHaveNumberAlreadyThere){
			uint16_t suggestedNumber = 1;
			bool hadConflict;
			do {
				hadConflict = false;
				for (uint16_t enumeratorIndex_2=0;enumeratorIndex_2<numberOfEnumerators;enumeratorIndex_2++){
					if ((enumeratorIndex_2<enumeratorIndex_1 ||
						enumEnumerators[enumeratorIndex_2].didHaveNumberAlreadyThere) && 
						enumEnumerators[enumeratorIndex_2].number==suggestedNumber){
						
						suggestedNumber++;
						hadConflict = true;
					}
				}
			} while (hadConflict);
			// now the suggestedNumber does not conflict with any other numbers already in place, so we can place it
			enumEnumerators[enumeratorIndex_1].number = suggestedNumber;
		}
	}
	// because numberOfEnumerators and enumeratorIndex are a unsigned numbers, the terminating condition "enumeratorIndex<numberOfEnumerators" works even when going backward, like below
	// by the way, the reason we go backwards is because then the indexes don't change, so they don't have to be recalculated
	for (uint16_t enumeratorIndex=numberOfEnumerators-1;enumeratorIndex<numberOfEnumerators;enumeratorIndex--){
		if (!enumEnumerators[enumeratorIndex].didHaveNumberAlreadyThere){
			string = giveNumberToSingleEnumEnumerator(string,enumEnumerators[enumeratorIndex].startIndex,enumEnumerators[enumeratorIndex].number);
		}
	}
	cosmic_free(enumEnumerators);
	return string;
}


// the input string is potentially destroyed, and an output string is given (cosmic_realloc() may be used on the input string)
char* giveNumbersForAllEnumEnumerators(char* string){
	int32_t length = strlen(string);
	int32_t startIndex = 6;
	if (length>5 && specificStringEqualCheck(string,0,5,"enum ")){
		for (int32_t i=7;i<length;i++){
			if (string[i]=='{'){
				if (string[++i]!='{') break;
				string = giveNumbersToSingleEnumList(string,i);
				length = strlen(string);
				startIndex = getIndexOfMatchingEnclosement(string,i); // this should happen
				break;
			}
		}
	}
	for (int32_t mainIndex=startIndex;mainIndex<length;mainIndex++){
		if (specificStringEqualCheck(string,mainIndex-6,mainIndex," enum ")){
			for (int32_t i=mainIndex;i<length;i++){
				if (string[i]==' '){
					if (string[++i]!='{') break;
					string = giveNumbersToSingleEnumList(string,i);
					length = strlen(string);
					mainIndex = getIndexOfMatchingEnclosement(string,i); // this should happen
					break;
				}
			}
		}
	}
	return string;
}





// the input string has cosmic_realloc() called on it, which is then returned
char* giveNameToSingleAnonymous(char* string, int32_t startIndexAtBracket, uint8_t typeOfStructure){
	const char* literalToInsert;
	if (typeOfStructure==0){
		literalToInsert = "__ANONYMOUS_S_";
	} else if (typeOfStructure==1){
		literalToInsert = "__ANONYMOUS_U_";
	} else {
		literalToInsert = "__ANONYMOUS_E_";
	}
	const int16_t amountToAddToLength = 23;// 23=14+8+1
	int32_t prevLength = strlen(string);
	int32_t newLength = prevLength+amountToAddToLength;
	string = cosmic_realloc(string,newLength+1);
	// this loop also copies the null terminator
	for (int32_t i=prevLength;i>(startIndexAtBracket-2);i--){
		string[i+amountToAddToLength]=string[i];
	}
	for (int32_t i=0;i<14;i++){
		string[i+startIndexAtBracket]=literalToInsert[i];
	}
	static uint32_t uniqueAnonymousCounter;
	writeHexInString(string+(startIndexAtBracket+14),++uniqueAnonymousCounter);
	return string;
}


// the input string is potentially destroyed, and an output string is given (cosmic_realloc() may be used on the input string)
char* giveNamesToAllAnonymous(char* string){
	int32_t length = strlen(string);
	if (length>9 && specificStringEqualCheck(string,0,9,"struct { ")){
		return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,7,0));
	}
	if (length>8 && specificStringEqualCheck(string,0,8,"union { ")){
		return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,6,1));
	}
	if (length>7 && specificStringEqualCheck(string,0,7,"enum { ")){
		return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,5,2));
	}
	// the above checks are for if the struct,union,enum part is at the very beginning, because those wouldn't be detected by the one below due to the space at the beginning
	for (int32_t i=10;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && specificStringEqualCheck(string,i-10,i," struct { ")){
			return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,i-2,0));
		}
	}
	for (int32_t i=9;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && specificStringEqualCheck(string,i-9,i," union { ")){
			return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,i-2,1));
		}
	}
	for (int32_t i=8;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && specificStringEqualCheck(string,i-8,i," enum { ")){
			return giveNamesToAllAnonymous(giveNameToSingleAnonymous(string,i-2,2));
		}
	}
	return string;
}

int32_t advancedSourceFindSub(char* string, int32_t sourceStart, int32_t sourceEnd){
	int32_t i0=sourceStart;
	int32_t i1=0;
	if (sourceContainer.string[i0]=='['){
		++i0;
		while (true){
			bool didSourceAdvance=false;
			bool didStringAdvance=false;
			while (sourceContainer.string[i0]==' ' | sourceContainer.string[i0]=='\n'){
				++i0;didSourceAdvance=true;
			}
			while (string[i1]==' ' | string[i1]=='\n'){
				++i1;didStringAdvance=true;
			}
			if (i0>sourceEnd | sourceContainer.string[i0]==0 | (didSourceAdvance&!didStringAdvance)){
				return -1;
			}
			assert(string[i1]!=0);
			if (string[i1]!=sourceContainer.string[i0]){
				return -1;
			}
			if (string[i1]==']'){
				return i0;
			}
			++i0;++i1;
		}
	}
	return -1;
}

bool advancedSourceFind(char* string, int32_t sourceStart, int32_t sourceEnd, int32_t* sourceStartOut, int32_t* sourceEndOut){
	for (int32_t subStart=sourceStart;subStart<sourceEnd;subStart++){
		int32_t ret=advancedSourceFindSub(string,subStart,sourceEnd);
		if (ret!=-1){
			*sourceStartOut=subStart+1;
			*sourceEndOut=ret;
			return false;
		}
	}
	return true;
}


// the input string is potentially destroyed, and an output string is given (cosmic_realloc() may be used on the input string)
char* resolveConstantExpressionInTypeString(char* string,int32_t sourceStart,int32_t sourceEnd){
	ExpressionTreeGlobalBuffer pack;
	for (int32_t i=0;string[i];i++){
		if (string[i]=='[') goto StartResolve;
	}
	return string; // nothing to resolve
	
	StartResolve:
	packExpressionTreeGlobalBuffer(&pack);
	for (int32_t i0=0;string[i0];i0++){
		if (string[i0]=='['){
			assert(string[i0+1]==' ');
			if (string[i0+2]==']') continue;
			for (int32_t i1=i0+1;string[i1];i1++){
				if (string[i1]==']'){
					char* t0=copyStringSegmentToHeap(string,0,i0+2);
					char* t1=copyStringSegmentToHeap(string,i0+2,i1+1);
					char* t2=copyStringSegmentToHeap(string,i1-1,strlen(string));
					int32_t internalSourceStart;
					int32_t internalSourceEnd;
					if (advancedSourceFind(t1,sourceStart,sourceEnd,&internalSourceStart,&internalSourceEnd)){
						// this is basically an internal error, it shouldn't happen
						printInformativeMessageAtSourceContainerIndex(true,"Type parser failed to figure out the original position of the contents of an array bracket pair",sourceStart,sourceEnd);
						exit(1);
					}
					int16_t root=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,internalSourceStart,internalSourceEnd,true);
					assert(root!=-1); // empty brackets should have already been check for
					uint32_t arrSize=expressionToConstantValue("unsigned long",root);
					char numberBuffer[14] = {0};
					snprintf(numberBuffer,13,"%lu",(unsigned long)arrSize);
					char* t3=strMerge3(t0,numberBuffer,t2);
					cosmic_free(t0);
					cosmic_free(t1);
					cosmic_free(t2);
					cosmic_free(string);
					string=t3;
					break;
				}
			}
		}
	}
	unpackExpressionTreeGlobalBuffer(&pack);
	return string;
}


// allocates a new string
// does not care if there is an identifier
// this function is called in this file and should not be used elsewhere
char* checkAndApplyTypeReorderAndNormalizationAnalysisToTypeStringToNew(char* stringIn, uint16_t* errorValue, int32_t sourceStart, int32_t sourceEnd){
	*errorValue=0;
	char* stringInternal = copyStringToHeapString(stringIn);
	for (int32_t i=0;stringInternal[i];i++){
		if (stringInternal[i]=='\"' | stringInternal[i]=='\''){
			*errorValue=2;
			return NULL;
		}
	}
	if (isArrayBracketsInvalidOnTypeString(stringInternal)){
		*errorValue=3;
		return NULL;
	}
	stringInternal=resolveConstantExpressionInTypeString(stringInternal,sourceStart,sourceEnd);
	int32_t length = strlen(stringInternal);
	
	struct SegmentOfTypeInTypeString{
		int32_t startInString;
		int32_t endInString;
		bool hasHadSwap;
	};
	
	uint16_t numberOfSegments = 1;
	for (int32_t i=0;i<length;i++){
		if (stringInternal[i]==' '){
			numberOfSegments++;
		}
	}
	struct SegmentOfTypeInTypeString *segments = cosmic_malloc(
		numberOfSegments*sizeof(struct SegmentOfTypeInTypeString));
	{
		// there is a copy of this block later in this function
		uint16_t walkingSegmentIndex = 0;
		int32_t nextStartIndex = 0;
		for (int32_t i=0;i<length;i++){
			if (stringInternal[i]==' '){
				segments[walkingSegmentIndex  ].hasHadSwap = false;
				segments[walkingSegmentIndex  ].startInString = nextStartIndex;
				segments[walkingSegmentIndex++].endInString = i;
				nextStartIndex = i+1;
			}
		}
		segments[walkingSegmentIndex].hasHadSwap = false;
		segments[walkingSegmentIndex].startInString = nextStartIndex;
		segments[walkingSegmentIndex].endInString = length;
	}
	// now time to check and apply reorder operations
	
	// by the way, I have "char" and "unsigned char" as the same type, even though they are technically different according to the standard.
	// This might cause some slight and silent problems when checking if types are identical, but I don't think it should be too bad
	
	for (uint16_t i=3;i<numberOfSegments;i++){
		struct SegmentOfTypeInTypeString *offsetSegments = &(segments[i-3]);
		if (!offsetSegments[0].hasHadSwap & 
			!offsetSegments[1].hasHadSwap & 
			!offsetSegments[2].hasHadSwap &
			!offsetSegments[3].hasHadSwap){
			
			if ((specificStringEqualCheck(stringInternal,offsetSegments[3].startInString,offsetSegments[3].endInString,"int") &&
				specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
				(specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"unsigned") || 
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"unsigned")))
				){
				
				offsetSegments[0].hasHadSwap = true;
				offsetSegments[1].hasHadSwap = true;
				offsetSegments[2].hasHadSwap = true;
				offsetSegments[3].hasHadSwap = true;
				struct SegmentOfTypeInTypeString tempValue = offsetSegments[0];
				offsetSegments[0] = offsetSegments[3];
				offsetSegments[3] = tempValue;
				tempValue = offsetSegments[1];
				offsetSegments[1] = offsetSegments[2];
				offsetSegments[2] = tempValue;
			}
		}
	}
	
	for (uint16_t i=2;i<numberOfSegments;i++){
		struct SegmentOfTypeInTypeString *offsetSegments = &(segments[i-2]);
		if (!offsetSegments[0].hasHadSwap & 
			!offsetSegments[1].hasHadSwap & 
			!offsetSegments[2].hasHadSwap){
			
			bool doSwap = false; // this is to help split up the logic so it isn't all in one crazy if expression
			
			if ((specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"signed") ||
				specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"unsigned")) &&
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") &&
				(specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"short") ||
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long"))
				){
				
				doSwap = true;
			} else if ((specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"signed") ||
				specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"unsigned")) &&
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long")
				){
				
				doSwap = true;
			} else if (specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int")
				){
				
				doSwap = true;
			}
			if (doSwap){
				offsetSegments[0].hasHadSwap = true;
				offsetSegments[1].hasHadSwap = true;
				offsetSegments[2].hasHadSwap = true;
				struct SegmentOfTypeInTypeString tempValue = offsetSegments[0];
				offsetSegments[0] = offsetSegments[2];
				offsetSegments[2] = tempValue;
			}
		}
	}
	
	for (uint16_t i=1;i<numberOfSegments;i++){
		struct SegmentOfTypeInTypeString *offsetSegments = &(segments[i-1]);
		if (!offsetSegments[0].hasHadSwap & !offsetSegments[1].hasHadSwap){
			bool doSwap = false; // this is to help split up the logic so it isn't all in one crazy if expression
			
			if ((specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"signed") ||
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"unsigned")) &&
				(specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"char") ||
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"short") ||
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") ||
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long"))
				){
				
				doSwap = true;
			} else if (specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") &&
				(specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"short") ||
				specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long"))
				){
				
				doSwap = true;
			} else if (specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"double")
				){
				
				doSwap = true;
			} else if (specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
				specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long")
				){
				
				// in this case, a swap isn't actually necessary, because they are the same word. However, for the future checks of validity, the segments should say it was swapped
				offsetSegments[0].hasHadSwap = true;
				offsetSegments[1].hasHadSwap = true;
			}
			
			if (doSwap){
				offsetSegments[0].hasHadSwap = true;
				offsetSegments[1].hasHadSwap = true;
				struct SegmentOfTypeInTypeString tempValue = offsetSegments[0];
				offsetSegments[0] = offsetSegments[1];
				offsetSegments[1] = tempValue;
			}
		}
	}
	// reorder operations are now done, however it is time for a validity check on the order of type keywords
	for (uint16_t i=0;i<numberOfSegments;i++){
		if (!segments[i].hasHadSwap &&
			(specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"unsigned") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"char") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"short") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"int") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"long") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"double") ||
			specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"float"))
			){
			
			// then either this is a single type specifier, or something was out of order (and an error should be thrown).
			bool isWrong = false; // using this to split up the logic so the if expressions are not super huge
			int16_t offset_i = i-1;
			// this for loop is to avoid having huge duplicated code for the if statement's expression. It loops twice.
			for (uint8_t tempI=0;tempI<2;tempI++){
				if (tempI==1){
					offset_i = i+1;
				}
				bool isSafe = !((i==0 & tempI==0) | (i==(numberOfSegments-1) & tempI==1)); // this is to prevent out of bounds access
				if (isSafe &&
					(specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"unsigned") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"signed") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"char") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"short") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"int") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"long") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"double") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"float"))
					){
					
					isWrong = true;
				}
			}
			if (isWrong){
				*errorValue=1;
				return NULL;
			}
		}
	}
	// now we know that everything is in the correct order and is valid.
	// however, this is still some more to do.
	{
		// first, let's apply what has been done to a new string
		char* tempString = cosmic_malloc(length+1);
		int32_t walkingIndex = 0;
		for (int32_t i=0;i<length;i++){
			tempString[i]=' '; // set all to spaces so that placement is easy
		}
		for (uint16_t segmentIndex=0;segmentIndex<numberOfSegments;segmentIndex++){
			int32_t startAt = segments[segmentIndex].startInString;
			int32_t endAt = segments[segmentIndex].endInString;
			for (int32_t stringIndex=startAt;stringIndex<endAt;stringIndex++){
				tempString[walkingIndex++]=stringInternal[stringIndex];
			}
			tempString[walkingIndex++]=' ';
		}
		// tempString doesn't need null termination
		
		// the following copies tempString to stringInternal
		// the reason why is to help prevent heap fragmentation a little
		for (int32_t i=0;i<length;i++){
			stringInternal[i]=tempString[i];
		}
		cosmic_free(tempString);
	}
	// now let's normalize the representations, so that type equivalence can be checked easier
	{
		uint16_t i=0;
		bool isSegmentRebuildNecessary = true; // we need to do a rebuild the first time
		while (i<numberOfSegments | isSegmentRebuildNecessary){
			if (isSegmentRebuildNecessary){
				{
					uint16_t newNumberOfSegments = 1;
					for (int32_t i0=0;i0<length;i0++){
						if (stringInternal[i0]==' '){
							newNumberOfSegments++;
						}
					}
					if (newNumberOfSegments!=numberOfSegments){
						numberOfSegments = newNumberOfSegments;
						cosmic_free(segments);
						segments = cosmic_malloc(numberOfSegments*sizeof(struct SegmentOfTypeInTypeString));
					}
				}
				{
					// this rebuilds segments (copied from before in this function)
					uint16_t walkingSegmentIndex = 0;
					int32_t nextStartIndex = 0;
					for (int32_t i0=0;i0<length;i0++){
						if (stringInternal[i0]==' '){
							segments[walkingSegmentIndex  ].hasHadSwap = false;
							segments[walkingSegmentIndex  ].startInString = nextStartIndex;
							segments[walkingSegmentIndex++].endInString = i0;
							nextStartIndex = i0+1;
						}
					}
					segments[walkingSegmentIndex].hasHadSwap = false;
					segments[walkingSegmentIndex].startInString = nextStartIndex;
					segments[walkingSegmentIndex].endInString = length;
				}
				isSegmentRebuildNecessary = false;
				i=0;
				// we may now continue with the normalization from the start
				continue;
			}
			// check for implied int from just "signed" and "unsigned"
			if ((specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"unsigned") || 
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed")) &&
				(i+1>=numberOfSegments ||
				!(specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"char") ||
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"short") ||
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"int") ||
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"long")))
				){
				if (specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed")){
					// if this is for "signed", then the "signed" should be removed at the same time as the "int" is inserted
					int32_t startPoint = segments[i].startInString;
					stringInternal[startPoint  ]='i';
					stringInternal[startPoint+1]='n';
					stringInternal[startPoint+2]='t';
					// this does copy the null terminator because of stringI<=length
					for (int32_t stringI=startPoint+6;stringI<=length;stringI++){
						stringInternal[stringI-3] = stringInternal[stringI];
					}
					length=length-3;
					// shrinking the allocation isn't exactly necessary, so it isn't done here
				} else {
					// this is for "unsigned"
					int32_t newLength = length+4;
					stringInternal = cosmic_realloc(stringInternal,newLength+1);
					int32_t stopPoint = segments[i].endInString;
					// this does copy the null terminator because stringI=length
					for (int32_t stringI=length;stringI>=stopPoint;stringI--){
						stringInternal[stringI+4] = stringInternal[stringI];
					}
					stringInternal[stopPoint+1]='i';
					stringInternal[stopPoint+2]='n';
					stringInternal[stopPoint+3]='t';
					length=newLength;
				}
				
				isSegmentRebuildNecessary = true;
				continue;
			}
			// check for unnessary "int" after other type keywords
			if (i!=0 &&
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"int") &&
				(specificStringEqualCheck(stringInternal,segments[i-1].startInString,segments[i-1].endInString,"short") ||
				specificStringEqualCheck(stringInternal,segments[i-1].startInString,segments[i-1].endInString,"long"))
				){
				
				// this does copy the null terminator because of stringI<=length
				for (int32_t stringI=segments[i].endInString+1;stringI<=length;stringI++){
					stringInternal[stringI-4] = stringInternal[stringI];
				}
				length=length-4;
				// shrinking the allocation isn't exactly necessary, so it isn't done here
				isSegmentRebuildNecessary = true;
				continue;
			}
			// check for unnessary "signed"
			if (i+1<numberOfSegments &&
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed") &&
				(specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"long") ||
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"int") ||
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"short"))
				){
				
				for (int32_t stringI=segments[i].endInString+1;stringI<=length;stringI++){
					stringInternal[stringI-7] = stringInternal[stringI];
				}
				length=length-7;
				// shrinking the allocation isn't exactly necessary, so it isn't done here
				isSegmentRebuildNecessary = true;
				continue;
			}
			// check for unnessary "unsigned"
			if (i+1<numberOfSegments &&
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"unsigned") &&
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"char")
				){
				
				for (int32_t stringI=segments[i].endInString+1;stringI<=length;stringI++){
					stringInternal[stringI-9] = stringInternal[stringI];
				}
				length=length-9;
				// shrinking the allocation isn't exactly necessary, so it isn't done here
				isSegmentRebuildNecessary = true;
				continue;
			}
			// check for "volatile" or "const" at end of typestring or end of member or end of parameter and move it up
			char tempC;
			if (((i==numberOfSegments-1 & i!=0) |
				(i+1<numberOfSegments && 
				((tempC=stringInternal[segments[i+1].startInString]), (tempC==';' | tempC==',' | tempC==')')))) &&
				(specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"volatile") ||
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"const"))
				){
				
				bool isHandlingConst=specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"const");
				int32_t iTo=i;
				while (iTo--!=0){
					int32_t startToTest;
					int32_t endToTest;
					if (iTo!=0){
						startToTest=segments[iTo-1].startInString;
						endToTest=segments[iTo-1].endInString;
						if (specificStringEqualCheck(stringInternal,startToTest,endToTest,"struct") ||
							specificStringEqualCheck(stringInternal,startToTest,endToTest,"union") ||
							specificStringEqualCheck(stringInternal,startToTest,endToTest,"enum")
							){
							
							if (--iTo!=0) continue;
							else break;
						}
					}
					startToTest=segments[iTo].startInString;
					endToTest=segments[iTo].endInString;
					if (stringInternal[startToTest]=='}'){
						int32_t otherBracket = getIndexOfMatchingEnclosement(stringInternal,startToTest);
						int32_t temp1=iTo;
						for (uint16_t temp2=0;temp2<numberOfSegments;temp2++){
							if (segments[temp2].startInString==otherBracket){
								iTo=temp2+1;
								break;
							}
						}
						if (temp1==iTo){
							printf("Internal Error: typeString corrupted (no open curly bracket or empty brackets while moving volatile)\n");
							exit(1);
						}
						continue;
					}
					if (isSegmentOfStringTypeLike(stringInternal,startToTest,endToTest)){
						continue;
					}
					break;
				}
				++iTo;
				// I'm doing this the lazy way, I don't want to write a better one right now
				char* temp0=copyStringSegmentToHeap(stringInternal,0,segments[iTo].startInString);
				char* temp1=copyStringSegmentToHeap(stringInternal,segments[iTo].startInString,segments[i].startInString-1);
				char* temp2=copyStringSegmentToHeap(stringInternal,segments[i].endInString,strlen(stringInternal));
				char* temp3;
				if (isHandlingConst){
					temp3=strMerge2(temp0,"const");
				} else {
					temp3=strMerge2(temp0,"volatile");
				}
				char* temp4=strMerge3(temp3," ",temp1);
				char* newStringInternal=strMerge2(temp4,temp2);
				cosmic_free(stringInternal);cosmic_free(temp0);cosmic_free(temp1);cosmic_free(temp2);cosmic_free(temp3);cosmic_free(temp4);
				stringInternal=newStringInternal;
				isSegmentRebuildNecessary = true;
				continue;
			}
			i++;
		}
	}
	// and nearly done with normalizing everything
	// 1. add unique names to anonymous struct, enum, union. 
	// 2. add values to any enum's enumerator-lists there may be
	cosmic_free(segments);
	stringInternal = giveNamesToAllAnonymous(stringInternal);
	stringInternal = giveNumbersForAllEnumEnumerators(stringInternal);
	// and done with normalizing everything
	char* stringOut = copyStringToHeapString(stringInternal); // lets ensure a proper sized allocation area for this string
	cosmic_free(stringInternal);
	return stringOut;
}




struct TypeToken{
	int32_t stringIndexStart;
	int32_t stringIndexEnd;
	int16_t enclosementMatch;
	bool isNonSymbol;
	bool doSkip;
	bool isKeywordOrTypedefed;
	char firstCharacter;
};

struct TypeTokenArray{
	struct TypeToken* typeTokens;
	int16_t* indexesForRearrangement;
	int16_t length;
};

bool isCharNonSymbolForTypeToken(char c){
	return ((c>='0') & (c<='9')) | ((c>='A') & (c<='Z')) | ((c>='a') & (c<='z')) | (c=='_') | (c=='.'); // the . is for the ... for variadic arguments
}

void writeIndexToNextSlotInTypeTokenArray(struct TypeTokenArray typeTokenArray, int16_t index){
	for (int16_t i=0;i<typeTokenArray.length;i++){
		if (typeTokenArray.indexesForRearrangement[i]==-1){
			typeTokenArray.indexesForRearrangement[i] = index;
			return;
		} else if (typeTokenArray.indexesForRearrangement[i]==index){
			printf("Type Parse Error: index:%d:got duped (%c)\n",index,typeTokenArray.typeTokens[index].firstCharacter);
			exit(1);
		}
	}
	printf("Type parsing must have failed, because I ran out of slots\n");
	exit(1);
}

int16_t findStartForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t boundStart, int16_t boundEnd){
	bool doContainerStart = false;
	bool doStartSkip = false;
	if (typeTokenArray.typeTokens[boundStart].isKeywordOrTypedefed){
		int32_t startIndexOfStartBound = typeTokenArray.typeTokens[boundStart].stringIndexStart;
		int16_t lengthOfBoundStartToken = typeTokenArray.typeTokens[boundStart].stringIndexEnd-startIndexOfStartBound;
		if (lengthOfBoundStartToken==4){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"enum")){
				doContainerStart = true;
				doStartSkip = true;
			}
		} else if (lengthOfBoundStartToken==5){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"union")){
				doContainerStart = true;
				doStartSkip = true;
			} else if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"const")){
				return findStartForTypeTokens(typeTokenArray,boundStart+1,boundEnd);
			}
		} else if (lengthOfBoundStartToken==6){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"struct")){
				doContainerStart = true;
				doStartSkip = true;
			}
		} else if (lengthOfBoundStartToken==8){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"volatile")){
				return findStartForTypeTokens(typeTokenArray,boundStart+1,boundEnd);
			}
		}
	}
	// doContainerStart is used to more easily parse containers (struct, enum, union)
	if (doContainerStart){
		// this is to ensure that the containerStart is done only when actually needed (and to detangle the logic)
		if (boundStart+1>=boundEnd){
			printf("Type parse error ID:1");
			exit(1);
		}
		bool isOnFirstToken;
		if (typeTokenArray.typeTokens[boundStart+1].firstCharacter=='{'){
			doContainerStart = true;
			isOnFirstToken = true;
		} else if (boundStart+2>=boundEnd){
			doContainerStart = false;
		} else if (typeTokenArray.typeTokens[boundStart+2].firstCharacter=='{'){
			doContainerStart = true;
			isOnFirstToken = false;
		} else {
			doContainerStart = false;
		}
		if (doContainerStart){
			// this is to ensure that the containerStart is done only when actually needed (and to detangle the logic)
			int16_t checkThisIndex;
			if (isOnFirstToken){
				checkThisIndex = typeTokenArray.typeTokens[boundStart+1].enclosementMatch+1;
			} else {
				checkThisIndex = typeTokenArray.typeTokens[boundStart+2].enclosementMatch+1;
			}
			if (checkThisIndex>=boundEnd){
				return boundStart;
			} else{
				return findStartForTypeTokens(typeTokenArray,checkThisIndex,boundEnd);
			}
		} else {
			/*
			this switches the order of the original tokens for a particular case to ensure that the order comes out correctly (because the parser reads it's backside backwards)
			the particular case is "struct ST"
			in constrast to the other cases that do not go here, which are:
			"struct {}"
			"struct ST{}"
			
			(Note that in these three quotes, "struct" can be replaced with "enum" and "union")
			*/
			struct TypeToken tempTypeToken = typeTokenArray.typeTokens[boundStart];
			typeTokenArray.typeTokens[boundStart] = typeTokenArray.typeTokens[boundStart+1];
			typeTokenArray.typeTokens[boundStart+1] = tempTypeToken;
		}
	}
	/*
	then this does not start on a container (struct, enum, union)
	therefore, search for identifier
	*/
	int16_t doStartAt;
	if (doStartSkip){
		doStartAt = boundStart+2;
	} else {
		doStartAt = boundStart;
	}
	for (int16_t i=doStartAt;i<boundEnd;i++){
		struct TypeToken *thisTypeTokenPtr = typeTokenArray.typeTokens+i;
		if ((thisTypeTokenPtr->firstCharacter=='[') | (thisTypeTokenPtr->firstCharacter=='{')){
			i = thisTypeTokenPtr->enclosementMatch;
		} else if ((!thisTypeTokenPtr->isKeywordOrTypedefed) & (thisTypeTokenPtr->isNonSymbol)){
			return i;
		}
	}
	/*
	then this has no identifier
	therefore, find the most inclosed token that isn't an enclosement
	*/
	int16_t mostInclosedToken = boundStart;
	int16_t amountOfEnclosementOnMostEnclosedToken = 0;
	int16_t amountOfEnclosementOnThisToken = 0;
	for (int16_t i=boundStart;i<boundEnd;i++){
		struct TypeToken *thisTypeTokenPtr = &(typeTokenArray.typeTokens[i]);
		if (thisTypeTokenPtr->firstCharacter=='['){
			int16_t prev=i;
			i = thisTypeTokenPtr->enclosementMatch;
			if (i==boundEnd-1 && 
				amountOfEnclosementOnThisToken>=amountOfEnclosementOnMostEnclosedToken){
				
				return prev;
			}
		} else if (thisTypeTokenPtr->firstCharacter=='{'){
			i = thisTypeTokenPtr->enclosementMatch;
			if (i==boundEnd-1){
				printf("Type Parser Warning: I\'m not sure if that is supposed to happen\n");
			}
		} else if (thisTypeTokenPtr->firstCharacter=='('){
			amountOfEnclosementOnThisToken++;
		} else if (thisTypeTokenPtr->firstCharacter==')'){
			amountOfEnclosementOnThisToken--;
		} else if (amountOfEnclosementOnThisToken>=amountOfEnclosementOnMostEnclosedToken){
			mostInclosedToken = i;
			amountOfEnclosementOnMostEnclosedToken = amountOfEnclosementOnThisToken;
		}
		if (amountOfEnclosementOnThisToken<0){
			printf("Type parse error ID:8");
			exit(1);
		}
	}
	// if nothing is enclosed, then this defaults to the last valid token (because amountOfEnclosementOnMostEnclosedToken==0)
	return mostInclosedToken;
}

void mainWalkForTypeTokens(struct TypeTokenArray, int16_t, int16_t, int16_t, bool);
void splitterStarterForTypeTokens(struct TypeTokenArray, int16_t, char);

void enumHandlerForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t bracketIndex){
	int16_t endBracketIndex = typeTokenArray.typeTokens[bracketIndex].enclosementMatch;
	for (int16_t i=bracketIndex;i<=endBracketIndex;i++){
		writeIndexToNextSlotInTypeTokenArray(typeTokenArray,i);
	}
}

int16_t squareBracketHandlerForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t squareBracketIndex0){
	int16_t squareBracketIndex1=typeTokenArray.typeTokens[squareBracketIndex0].enclosementMatch;
	int16_t squareBracketIndex2;
	int16_t squareBracketIndex3;
	if (squareBracketIndex0<squareBracketIndex1){
		squareBracketIndex2=squareBracketIndex0;
		squareBracketIndex3=squareBracketIndex1;
	} else {
		squareBracketIndex2=squareBracketIndex1;
		squareBracketIndex3=squareBracketIndex0;
	}
	for (int16_t i=squareBracketIndex2;i<=squareBracketIndex3;i++){
		writeIndexToNextSlotInTypeTokenArray(typeTokenArray,i);
	}
	return squareBracketIndex1;
}

void bracketHandlerForMainWalkForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t boundStart, int16_t bracketIndex){
	bool isDescriptionOneAway = false;
	if (bracketIndex-1<boundStart){
		printf("Type parse error ID:5");
		exit(1);
	}
	if (typeTokenArray.typeTokens[bracketIndex-1].isKeywordOrTypedefed){
		isDescriptionOneAway = true;
	}
	if (!isDescriptionOneAway){
		if (bracketIndex-2<boundStart){
			printf("Type parse error ID:6");
			exit(1);
		}
		if (!(typeTokenArray.typeTokens[bracketIndex-2].isKeywordOrTypedefed)){
			printf("Type parse error ID:7");
			exit(1);
		}
	}
	int16_t descriptorIndex;
	if (isDescriptionOneAway){
		descriptorIndex = bracketIndex-1;
	} else {
		descriptorIndex = bracketIndex-2;
	}
	uint8_t outID = 0;
	
	
	int32_t startIndexInString = typeTokenArray.typeTokens[descriptorIndex].stringIndexStart;
	int16_t lengthInString = typeTokenArray.typeTokens[descriptorIndex].stringIndexEnd-startIndexInString;
	if (lengthInString==4){
		if (isSectionOfStringEquivalent(sourceContainer.string,startIndexInString,"enum")){
			outID = 1;
		}
	} else if (lengthInString==5){
		if (isSectionOfStringEquivalent(sourceContainer.string,startIndexInString,"union")){
			outID = 2;
		}
	} else if (lengthInString==6){
		if (isSectionOfStringEquivalent(sourceContainer.string,startIndexInString,"struct")){
			outID = 3;
		}
	}
	if (outID==0){
		printf("Type parse error ID:8");
		exit(1);
	}
	if (outID!=1){
		splitterStarterForTypeTokens(typeTokenArray,bracketIndex,';');
	} else {
		enumHandlerForTypeTokens(typeTokenArray,bracketIndex);
	}
}

void mainWalkForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t boundStart, int16_t boundEnd, int16_t externalStart, bool doExternalStart){
	if (boundStart+1==boundEnd){
		writeIndexToNextSlotInTypeTokenArray(typeTokenArray,boundStart);
		return;
	}
	if (boundStart+1>boundEnd){
		printf("Type parse error ID:2\n");
		exit(1);
	}
	bool hasParenStart = false;
	bool hasParenEnd = false;
	int16_t startIndex;
	int16_t parenStartIndex;
	if (doExternalStart){
		startIndex = typeTokenArray.typeTokens[externalStart].enclosementMatch+1;
	} else {
		startIndex = findStartForTypeTokens(typeTokenArray,boundStart,boundEnd);
	}
	for (int16_t i=startIndex;i<boundEnd;i++){
		char firstCharacter = typeTokenArray.typeTokens[i].firstCharacter;
		if ((firstCharacter=='*') & (i!=startIndex)){
			printf("Type parse error ID:3 (this is the condition to prevent incorrect asterisk)\n");
			exit(1);
		}
		if (firstCharacter==')'){
			hasParenEnd=true;
			break;
		}
		if (firstCharacter=='['){
			i=squareBracketHandlerForTypeTokens(typeTokenArray,i);
		} else if (firstCharacter=='('){
			splitterStarterForTypeTokens(typeTokenArray,i,',');
			i = typeTokenArray.typeTokens[i].enclosementMatch;
		} else if (firstCharacter=='{'){
			int16_t matchingIndex = typeTokenArray.typeTokens[i].enclosementMatch;
			bracketHandlerForMainWalkForTypeTokens(typeTokenArray,boundStart,i);
			i = matchingIndex;
		} else {
			writeIndexToNextSlotInTypeTokenArray(typeTokenArray,i);
		}
	}
	if (doExternalStart){
		startIndex = externalStart-1;
	} else {
		startIndex = startIndex-1;
	}
	for (int16_t i=startIndex;i>=boundStart;i--){
		char firstCharacter = typeTokenArray.typeTokens[i].firstCharacter;
		if (firstCharacter=='('){
			hasParenStart=true;
			parenStartIndex=i;
			break;
		}
		if (firstCharacter=='}'){
			int16_t matchingIndex = typeTokenArray.typeTokens[i].enclosementMatch;
			mainWalkForTypeTokens(typeTokenArray,boundStart,i+1,0,false);
			break;
		}
		if (firstCharacter==']'){
			i=squareBracketHandlerForTypeTokens(typeTokenArray,i);
		} else {
			writeIndexToNextSlotInTypeTokenArray(typeTokenArray,i);
		}
	}
	if (hasParenStart | hasParenEnd){
		if (hasParenStart & hasParenEnd){
			mainWalkForTypeTokens(typeTokenArray,boundStart,boundEnd,parenStartIndex,true);
		} else {
			printf("Type parse error ID:4"); // this one should just never happen, regardless of input
			exit(1);
		}
	}
}

void splitterStarterForTypeTokens(struct TypeTokenArray typeTokenArray, int16_t boundStart, char splittingChar){
	int16_t boundEnd = typeTokenArray.typeTokens[boundStart].enclosementMatch;
	int16_t previousStart = boundStart+1;
	writeIndexToNextSlotInTypeTokenArray(typeTokenArray,boundStart);
	if (previousStart!=boundEnd){
		for (int16_t i=boundStart+1;i<boundEnd;i++){
			char c = typeTokenArray.typeTokens[i].firstCharacter;
			if (c == splittingChar){
				mainWalkForTypeTokens(typeTokenArray,previousStart,i,0,false);
				previousStart = i+1;
				writeIndexToNextSlotInTypeTokenArray(typeTokenArray,i);
			} else if ((c=='(') | (c=='{')){
				i = typeTokenArray.typeTokens[i].enclosementMatch;
			}
		}
		if (splittingChar==','){
			mainWalkForTypeTokens(typeTokenArray,previousStart,boundEnd,0,false);
		}
	}
	writeIndexToNextSlotInTypeTokenArray(typeTokenArray,boundEnd);
}

void keywordAndTypedefDetectInTypeTokens(struct TypeTokenArray typeTokenArray){
	for (int16_t i=0;i<typeTokenArray.length;i++){
		int32_t startIndex = typeTokenArray.typeTokens[i].stringIndexStart;
		int32_t endIndex = typeTokenArray.typeTokens[i].stringIndexEnd;
		int16_t lengthOfThisToken = endIndex-startIndex;
		bool isKeyword = false;
		if (lengthOfThisToken==1){
			// do nothing, this is to consume the case because it is common for lengthOfThisToken==1
		} else if (lengthOfThisToken==3){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"int") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"...")){ 
				// the ... is for multiple arguments in a function
				
				isKeyword = true;
			}
		} else if (lengthOfThisToken==4){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"long") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"void") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"char")){
					
				isKeyword = true;
			}
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"enum")){
				if (i+1>=typeTokenArray.length){
					printf("keyword \'enum\' must have something after it\n");
					exit(1);
				}
				isKeyword = true;
			}
		} else if (lengthOfThisToken==5){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"short") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"const") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"float") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"_Bool")){
				
				isKeyword = true;
			}
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"union")){
				if (i+1>=typeTokenArray.length){
					printf("keyword \'union\' must have something after it\n");
					exit(1);
				}
				isKeyword = true;
			}
		} else if (lengthOfThisToken==6){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"signed") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"double")){
				isKeyword = true;
			}
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"struct")){
				if (i+1>=typeTokenArray.length){
					printf("keyword \'struct\' must have something after it\n");
					exit(1);
				}
				isKeyword = true;
			}
		} else if (lengthOfThisToken==8){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndex,"volatile") ||
				isSectionOfStringEquivalent(sourceContainer.string,startIndex,"unsigned")){
				isKeyword = true;
			}
		}
		bool isTypedefed = isSectionOfStringTypedefed(sourceContainer.string,startIndex,endIndex);
		typeTokenArray.typeTokens[i].isKeywordOrTypedefed = isKeyword | isTypedefed;
	}
}

int16_t specificEnclosementMatchTypeTokens(struct TypeTokenArray typeTokenArray, int16_t startIndex){
	char startChar = typeTokenArray.typeTokens[startIndex].firstCharacter;
	char endingEnclosements[]=")]}";
	char startingEnclosements[]="([{";
	uint8_t typeOfEnclosement;
	if (startChar==startingEnclosements[0]){
		typeOfEnclosement=0;
	} else if (startChar==startingEnclosements[1]){
		typeOfEnclosement=1;
	} else if (startChar==startingEnclosements[2]){
		typeOfEnclosement=2;
	} else {
		printf("specificEnclosementMatchTypeTokens() called wrong\n");
		exit(1);
	}
	int16_t i=startIndex+1;
	char c;
	while ((c=typeTokenArray.typeTokens[i].firstCharacter)!=endingEnclosements[typeOfEnclosement]){
		if ((c==endingEnclosements[0]) |
			(c==endingEnclosements[1]) |
			(c==endingEnclosements[2]) |
			(i+1>=typeTokenArray.length)){
			
			printf("While parsing type, got an enclosement error\n");
			exit(1);
		}
		if ((c==startingEnclosements[0]) |
			(c==startingEnclosements[1]) |
			(c==startingEnclosements[2])){
			
			i = specificEnclosementMatchTypeTokens(typeTokenArray,i);
		}
		i++;
	}
	typeTokenArray.typeTokens[startIndex].enclosementMatch=i;
	typeTokenArray.typeTokens[i].enclosementMatch=startIndex;	
	return i;
}

void enclosementMatchTypeTokens(struct TypeTokenArray typeTokenArray){
	char endingEnclosements[]=")]}";
	char startingEnclosements[]="([{";
	for (int16_t i=0;i<typeTokenArray.length;i++){
		typeTokenArray.typeTokens[i].enclosementMatch=0;
	}
	for (int16_t i=0;i<typeTokenArray.length;i++){
		char c = typeTokenArray.typeTokens[i].firstCharacter;
		if ((c==endingEnclosements[0]) |
			(c==endingEnclosements[1]) |
			(c==endingEnclosements[2])){
			
			printf("While parsing type, got an enclosement error\n");
			exit(1);
		}
		if ((c==startingEnclosements[0]) |
			(c==startingEnclosements[1]) |
			(c==startingEnclosements[2])){
			
			i=specificEnclosementMatchTypeTokens(typeTokenArray,i);
		}
	}
}

struct TypeTokenArray typeTokenizeForAnalysis(int32_t startIndex, int32_t endIndex){
	// This function has optimization potential (doing multiple cosmic_malloc()'s for the TypeToken* are not actually necessary)
	struct TypeToken * typeTokensOfStringSegment_1 = cosmic_malloc(
		sizeof(struct TypeToken)*(endIndex-startIndex));
	for (int32_t i=startIndex;i<endIndex;i++){
		struct TypeToken* thisToken = typeTokensOfStringSegment_1+(i-startIndex);
		thisToken->stringIndexStart = i;
		thisToken->stringIndexEnd = i+1;
		thisToken->doSkip = false;
		thisToken->enclosementMatch = 0;
		char firstCharacter = sourceContainer.string[i];
		thisToken->firstCharacter = firstCharacter;
		thisToken->isNonSymbol = isCharNonSymbolForTypeToken(firstCharacter);
	}
	for (int32_t i=1;i<(endIndex-startIndex);i++){
		if (typeTokensOfStringSegment_1[i-1].isNonSymbol && typeTokensOfStringSegment_1[i].isNonSymbol){
			typeTokensOfStringSegment_1[i].doSkip = true;
		}
	}
	int16_t walkingIndex=0;
	for (int32_t i=0;i<(endIndex-startIndex);i++){
		if (!typeTokensOfStringSegment_1[i].doSkip){
			walkingIndex++;
		}
	}
	int16_t part2Length = walkingIndex;
	struct TypeToken* typeTokensOfStringSegment_2 = cosmic_malloc(
		sizeof(struct TypeToken)*part2Length);
	walkingIndex=0;
	for (int32_t i=0;i<(endIndex-startIndex);i++){
		if (typeTokensOfStringSegment_1[i].doSkip){
			typeTokensOfStringSegment_2[walkingIndex-1].stringIndexEnd++;
		} else {
			typeTokensOfStringSegment_2[walkingIndex++]=typeTokensOfStringSegment_1[i];
		}
	}
	cosmic_free(typeTokensOfStringSegment_1);
	for (int16_t i=0;i<part2Length;i++){
		typeTokensOfStringSegment_2[i].doSkip=typeTokensOfStringSegment_2[i].firstCharacter==' ' || typeTokensOfStringSegment_2[i].firstCharacter=='\n';
	}
	walkingIndex=0;
	for (int16_t i=0;i<part2Length;i++){
		if (!typeTokensOfStringSegment_2[i].doSkip){
			walkingIndex++;
		}
	}
	int16_t part3Length = walkingIndex;
	struct TypeToken* typeTokensOfStringSegment_3 = cosmic_malloc(
		sizeof(struct TypeToken)*part3Length);
	walkingIndex=0;
	for (int16_t i=0;i<part2Length;i++){
		if (!typeTokensOfStringSegment_2[i].doSkip){
			typeTokensOfStringSegment_3[walkingIndex++]=typeTokensOfStringSegment_2[i];
		}
	}
	cosmic_free(typeTokensOfStringSegment_2);
	struct TypeTokenArray returnVal;
	returnVal.typeTokens = typeTokensOfStringSegment_3;
	returnVal.length = part3Length;
	returnVal.indexesForRearrangement = cosmic_malloc(sizeof(int16_t)*(part3Length+1));
	for (int16_t i=0;i<(part3Length+1);i++){
		returnVal.indexesForRearrangement[i]=-1;
	}
	return returnVal;
}

char* convertType(int32_t startIndex, int32_t endIndex){
	struct TypeTokenArray typeTokenArray = typeTokenizeForAnalysis(startIndex,endIndex);
	enclosementMatchTypeTokens(typeTokenArray);
	keywordAndTypedefDetectInTypeTokens(typeTokenArray);
	mainWalkForTypeTokens(typeTokenArray,0,typeTokenArray.length,0,false);
	// now we generate the string
	int16_t walkingIndex = 0;
	int32_t walkingCount = 0;
	while (typeTokenArray.indexesForRearrangement[walkingIndex]!=-1){
		struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+typeTokenArray.indexesForRearrangement[walkingIndex];
		walkingCount += (thisTypeTokenPtr->stringIndexEnd-thisTypeTokenPtr->stringIndexStart)+1;
		walkingIndex++;
	}
	char *resultString = cosmic_malloc(walkingCount);
	int32_t endIndexForReturnString = walkingCount-1;
	walkingIndex = 0;
	walkingCount = 0;
	while (typeTokenArray.indexesForRearrangement[walkingIndex]!=-1){
		struct TypeToken thisTypeToken = typeTokenArray.typeTokens[typeTokenArray.indexesForRearrangement[walkingIndex]];
		for (int32_t i=thisTypeToken.stringIndexStart;i<thisTypeToken.stringIndexEnd;i++){
			resultString[walkingCount++]=sourceContainer.string[i];
		}
		resultString[walkingCount++]=' ';
		walkingIndex++;
	}
	resultString[endIndexForReturnString]=0;
	cosmic_free(typeTokenArray.typeTokens);
	cosmic_free(typeTokenArray.indexesForRearrangement);
	uint16_t errorValueForTypeNormalizer;
	char *normalizedResultString = checkAndApplyTypeReorderAndNormalizationAnalysisToTypeStringToNew(resultString,&errorValueForTypeNormalizer,startIndex,endIndex);
	if (normalizedResultString==NULL){
		if (errorValueForTypeNormalizer==1){
			printInformativeMessageAtSourceContainerIndex(true,"type keywords cannot be out of proper order",startIndex,endIndex);
		} else if (errorValueForTypeNormalizer==2){
			printInformativeMessageAtSourceContainerIndex(true,"types cannot contain the character `\'` or `\"`",startIndex,endIndex);
		} else if (errorValueForTypeNormalizer==3){
			printInformativeMessageAtSourceContainerIndex(true,"types cannot contain nested array brackets",startIndex,endIndex);
		} else {
			assert(false);
		}
		exit(1);
	}
	assert(errorValueForTypeNormalizer==0);
	cosmic_free(resultString);
	return normalizedResultString;
}

// some old testing code is below. The code above has been updated many times since the test code below has been run
#if 0
void temp(char *s){
	int32_t length;
	for (length=0;s[length];length++){
	}
	printf("\n$%s$\n^\n$%s$\n\n\n\n",convertType(s,0,length),s);
}


int main(){
	temp("int i");
	temp("int  (*i) (int k,long int)");
	temp("void (*)()");
	temp("long int (*compar)(const void *, const void*)");
	temp("int (*i)[20]");
	temp("int *i[20]");
	temp("int *(i[20])");
	temp("char *(*(**foo [][8])())[]");
	temp("char (*var)[1]");
	temp("void qsort(void *base, long nitems, long size, int (*compar)(const void *, const void*))");
	temp("struct Me var");
	temp("struct Me var[2]");
	temp("struct ST{int i;} var");
	temp("struct ST{int i;}");
	temp("struct ST var");
	temp("struct ST *var");
	temp("struct ST *fun(struct STY, struct STY id)");
	temp("struct ST fun(struct STY, struct STY id)");
	temp("struct ST{int i1;} fun(struct {long i2;long int i4;}, struct STO{long long i3;} id)");
	temp("enum {this,that} id");
	temp("long int i");
	temp("long long i");
	temp("long long int i");
	temp("short i");
	temp("short int i");
	temp("long double i");
	temp("unsigned i");
	temp("signed i");
	temp("signed int i ");
	temp("unsigned int i ");
	temp("signed char i ");
	temp("unsigned char i ");
	temp("struct {long int component;}");
	temp("union {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("struct {long int component;}");
	temp("enum {component=13}");
	temp("enum {c1=1,c2,c3=2,c4=5}");
	temp("const int i");
	temp("union ARGU{struct {char a_0;} R;struct {char a_0;char a_1;} RR; struct {char a_0;char a_2;} RRR;struct {char a_0;char a_1;} RB;struct {char a_0;unsigned int a_1;} RW;struct {char a_0;char a_1;unsigned long a_2;} RRD;struct {char a_0;char a_1;unsigned long a_2;} RRL;struct {char a_0;char a_1;unsigned long a_2;} RRW;struct {unsigned long a_0;} P;}");
	temp("union ARGU{struct {char a_0;} R;}");
	
	printf("this should fail\n");
	temp("char var*[1]");
	
	printf("this should fail\n");
	temp("unsigned int long i");
	
	printf("The C standard says this should be valid, but it fails with this parser. It seems to be due to not having an identifer, so the starting place is off. However, I think this type is crazy, and I do not think I am going to work with it.");
	temp("struct tag (*[5])(float)");
	
	printf("\nDONE\n");
	return 0;
}
#endif





