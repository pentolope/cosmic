

#include "Common.c"


/*
NOTES:

 - bitfields are not implemented. There is some checking performed for them.
 - "const" and "volatile" will appear directly before the thing that is declared const or volatile.
 - in some rare cases, if the type does not have an identifier and it's a very complex type, 
     it may not recognize the starting point correctly, 
     and soon after would throw an error when it sees that something is invalid.
 - The way the enum assigns values automatically is NOT the way it should be done. As of now, it assigns values in increasing order (starting at 1) with a value that is not already taken.
     Further, it only accepts base 10 numbers with no suffix (it should allow constant expressions with values from enum items previously in the list)
 - I think everything is working, except for the things I just mentioned of course.
 - This file is to be included by "TypeConverter.c"

 TODO: 
 - write check to recursively apply "const" and "volatile" to struct and union members (is this needed? I kinda don't think so because of how expressions handle types but maybe it could if a `const struct` had a `struct` in it)
 - write sanity checker for type strings
*/



/*
convertType() Returns a new array of strings on the heap of the result.
Each string has components that are seperated by spaces (without an ending space).
The array is terminated with a NULL.
struct, enum, union will preface the name (and the name may be followed by a bracket)
"long int" does NOT become "int long", and other things of that sort (I got that working!)

Remember, what comes out of here is not finished. It needs to have the structure declarations taken out and typedefs resolved by "fullTypeParseAndAdd()" or "fullTypeParseAvoidAdd()" [ those functions call convertType() ]
*/
char** convertType(int32_t startIndex, int32_t endIndex);

/*
Finds the endIndex for a C style declaration, primarily for convertType().
Does not include the brackets for function declarations.
Doesn't check must about the type declaration to see it's validity.
*/
int32_t findEndIndexForConvertType(int32_t startIndex){
	bool wasCloseParenLast = false; // this isn't set to false for spaces and newlines when they are last
	char c;
	uint16_t enclosementLevel = 0; // unsigned is kinda important
	int32_t i=startIndex;
	const char* string = sourceContainer.string;
	while ((c = string[i])!=0){
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
	err_1101_("Could not find end to the type declaration that started here",startIndex);
	return 0; // unreachable
}


// doesn't check for validity of identifier (if it exists)
// expects that the type string has had typedef expansion on it already
// in various places, it is needed to know if a type string has an identifier at the beginning
bool doesThisTypeStringHaveAnIdentifierAtBeginning(const char* string){
	int32_t indexOfSpace = getIndexOfFirstSpaceInString(string);
	if (indexOfSpace==-1){
		// not going to check for things that must have spaces in them. And all type strings with identifiers should have spaces.
		assert(doStringsMatch(string,"char") || 
			doStringsMatch(string,"short") ||
			doStringsMatch(string,"int") ||
			doStringsMatch(string,"long") ||
			doStringsMatch(string,"float") ||
			doStringsMatch(string,"double") ||
			doStringsMatch(string,"void") ||
			doStringsMatch(string,"_Bool"));
		return false;
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
			isSectionOfStringEquivalent(string,0,"( ") ||
			isSectionOfStringEquivalent(string,0,": ")
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
	Start:;
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
			goto Start;
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


// the input string is potentially destroyed, and an output string is given (effectively, cosmic_realloc() may be used on the input string)
char* giveNumbersForAllEnumEnumerators(char* string){
	int32_t length = strlen(string);
	int32_t startIndex = 6;
	if (length>5 && specificStringEqualCheck(string,0,5,"enum ")){
		for (int32_t i=7;i<length;i++){
			if (string[i]==' '){
				if (string[++i]!='{') break;
				string = giveNumbersToSingleEnumList(string,i);
				length = strlen(string);
				startIndex = getIndexOfMatchingEnclosement(string,i);
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
					mainIndex = getIndexOfMatchingEnclosement(string,i);
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


// the input string is potentially destroyed, and an output string is given (effectively, cosmic_realloc() may be used on the input string)
char* giveNamesToAllAnonymous(char* string){
	if (isSectionOfStringEquivalent(string,0,"struct { ")){
		string=giveNameToSingleAnonymous(string,7,0);
	}
	if (isSectionOfStringEquivalent(string,0,"union { ")){
		string=giveNameToSingleAnonymous(string,6,1);
	}
	if (isSectionOfStringEquivalent(string,0,"enum { ")){
		string=giveNameToSingleAnonymous(string,5,2);
	}
	// the above checks are for if the struct,union,enum part is at the very beginning, because those wouldn't be detected by the one below due to the space at the beginning
	int32_t length = strlen(string);
	for (int32_t i=10;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && isSectionOfStringEquivalent(string,i-10," struct { ")){
			string = giveNameToSingleAnonymous(string,i-2,0);
			length = strlen(string);
			i=9;
		}
	}
	for (int32_t i=9;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && isSectionOfStringEquivalent(string,i-9," union { ")){
			string = giveNameToSingleAnonymous(string,i-2,1);
			length = strlen(string);
			i=8;
		}
	}
	for (int32_t i=8;i<length;i++){
		// the first term in the if expression is there to speed it up
		if (string[i-2]=='{' && isSectionOfStringEquivalent(string,i-8," enum { ")){
			string = giveNameToSingleAnonymous(string,i-2,2);
			length = strlen(string);
			i=7;
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
			if (i0>sourceEnd | sourceContainer.string[i0]==0 | (didSourceAdvance&!didStringAdvance)) return -1;
			assert(string[i1]!=0);
			if (string[i1]!=sourceContainer.string[i0]) return -1;
			if (string[i1]==']') return i0;
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


// the input string is potentially destroyed, and an output string is given (effectively, cosmic_realloc() may be used on the input string)
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
					int16_t root=buildExpressionTreeToGlobalBufferAndReturnRootIndex(internalSourceStart,internalSourceEnd,true);
					assert(root!=-1); // empty brackets should have already been checked for
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
		if (stringInternal[i]==' ') numberOfSegments++;
	}
	struct SegmentOfTypeInTypeString *segments = cosmic_malloc(numberOfSegments*sizeof(struct SegmentOfTypeInTypeString));
	{
		// there is a copy of this block later in this function
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
	// now time to check and apply reorder operations
	
	// by the way, I have "char" and "unsigned char" as the same type, even though they are technically different according to the standard.
	// This might cause some slight and silent problems when checking if types are identical, but I don't think it should be too bad
	
	for (uint16_t i=3;i<numberOfSegments;i++){
		struct SegmentOfTypeInTypeString *offsetSegments = segments+(i-3);
		if (
		!offsetSegments[0].hasHadSwap & 
		!offsetSegments[1].hasHadSwap & 
		!offsetSegments[2].hasHadSwap &
		!offsetSegments[3].hasHadSwap){
			
			if ((
			specificStringEqualCheck(stringInternal,offsetSegments[3].startInString,offsetSegments[3].endInString,"int") &&
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") && (
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"signed") || 
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
		struct SegmentOfTypeInTypeString *offsetSegments = segments+(i-2);
		if (
		!offsetSegments[0].hasHadSwap & 
		!offsetSegments[1].hasHadSwap & 
		!offsetSegments[2].hasHadSwap){
			
			if ((
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"signed") ||
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"unsigned")) &&
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") && (
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"short") ||
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long"))
			){
			} else if ((
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"signed") ||
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"unsigned")) &&
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long")
			){
			} else if (
			specificStringEqualCheck(stringInternal,offsetSegments[2].startInString,offsetSegments[2].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int")
			){
			} else continue;
			
			offsetSegments[0].hasHadSwap = true;
			offsetSegments[1].hasHadSwap = true;
			offsetSegments[2].hasHadSwap = true;
			struct SegmentOfTypeInTypeString tempValue = offsetSegments[0];
			offsetSegments[0] = offsetSegments[2];
			offsetSegments[2] = tempValue;
		}
	}
	
	for (uint16_t i=1;i<numberOfSegments;i++){
		struct SegmentOfTypeInTypeString *offsetSegments = segments+(i-1);
		if (
		!offsetSegments[0].hasHadSwap &
		!offsetSegments[1].hasHadSwap){
			if ((
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"signed") ||
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"unsigned")) && (
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"char") ||
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"short") ||
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") ||
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long"))
			){
			} else if (
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"int") && (
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"short") ||
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long"))
			){
			} else if (
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"double")
			){
			} else if (
			specificStringEqualCheck(stringInternal,offsetSegments[1].startInString,offsetSegments[1].endInString,"long") &&
			specificStringEqualCheck(stringInternal,offsetSegments[0].startInString,offsetSegments[0].endInString,"long")
			){
				// in this case, a swap isn't actually necessary, because they are the same word. However, for the future checks of validity, the segments should say it was swapped
				offsetSegments[0].hasHadSwap = true;
				offsetSegments[1].hasHadSwap = true;
				continue;
			} else continue;
			
			offsetSegments[0].hasHadSwap = true;
			offsetSegments[1].hasHadSwap = true;
			struct SegmentOfTypeInTypeString tempValue = offsetSegments[0];
			offsetSegments[0] = offsetSegments[1];
			offsetSegments[1] = tempValue;
		}
	}
	// reorder operations are now done. now a validity check on the order of type keywords is performed
	for (uint16_t i=0;i<numberOfSegments;i++){
		if (!segments[i].hasHadSwap && (
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"unsigned") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"char") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"short") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"int") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"long") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"double") ||
		specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"float"))
		){
			
			// then either this is a single type specifier, or something was out of order (and an error should be thrown).
			int16_t offset_i = i-1;
			// this for loop is to avoid having huge duplicated code for the if statement's expression. It loops twice.
			for (uint8_t tempI=0;tempI<2;tempI++){
				if (!((i==0 & tempI==0) | (i==(numberOfSegments-1) & tempI==1))){ // this is to prevent out of bounds access
					if (
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"unsigned") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"signed") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"char") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"short") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"int") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"long") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"double") ||
					specificStringEqualCheck(stringInternal,segments[offset_i].startInString,segments[offset_i].endInString,"float")
					){
						*errorValue=1;
						return NULL;
					}
				}
				offset_i = i+1; // this is set for the second iteration only
			}
		}
	}
	// now we know that everything is in the correct order and is valid.
	// however, this is still some more to do.
	{
		// first, let's apply what has been done to a different string
		char* tempString = cosmic_malloc(length+1);
		int32_t walkingIndex = 0;
		for (int32_t i=0;i<length;i++) tempString[i]=' '; // set all to spaces so that placement is easy
		
		for (uint16_t segmentIndex=0;segmentIndex<numberOfSegments;segmentIndex++){
			int32_t startAt = segments[segmentIndex].startInString;
			int32_t endAt = segments[segmentIndex].endInString;
			for (int32_t stringIndex=startAt;stringIndex<endAt;stringIndex++) tempString[walkingIndex++]=stringInternal[stringIndex];
			
			tempString[walkingIndex++]=' ';
		}
		// tempString doesn't need null termination
		
		// the following copies tempString to stringInternal
		// the reason why is to help prevent heap fragmentation a little
		for (int32_t i=0;i<length;i++) stringInternal[i]=tempString[i];
		
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
						if (stringInternal[i0]==' ') newNumberOfSegments++;
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
					stringInternal[stopPoint+0]=' ';
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
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"int") && (
				specificStringEqualCheck(stringInternal,segments[i-1].startInString,segments[i-1].endInString,"short") ||
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
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"signed") && (
				specificStringEqualCheck(stringInternal,segments[i+1].startInString,segments[i+1].endInString,"long") ||
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
			if (((i==numberOfSegments-1 & i!=0) | (i+1<numberOfSegments && 
				((tempC=stringInternal[segments[i+1].startInString]), (tempC==';' | tempC==',' | tempC==')')))) && (
				specificStringEqualCheck(stringInternal,segments[i].startInString,segments[i].endInString,"volatile") ||
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
							printf("Internal Error: typeString corrupted (no open curly bracket or empty brackets while moving volatile/const)\n");
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
				if (i==iTo){
					*errorValue=4;
					return NULL;
				}
				// I'm doing this the lazy-ish way, I don't want to write a better one right now
				char* temp0 = copyStringSegmentToHeap(stringInternal,0,segments[iTo].startInString);
				char* temp1 = copyStringSegmentToHeap(stringInternal,segments[iTo].startInString,segments[i].startInString-1);
				char* temp2 = copyStringSegmentToHeap(stringInternal,segments[i].endInString,strlen(stringInternal));
				char* temp3 = strMerge5(temp0,isHandlingConst?"const":"volatile"," ",temp1,temp2);
				cosmic_free(stringInternal);cosmic_free(temp0);cosmic_free(temp1);cosmic_free(temp2);
				stringInternal = temp3;
				isSegmentRebuildNecessary = true;
				continue;
			}
			i++;
		}
	}
	cosmic_free(segments);
	// and nearly done with normalizing everything, just need to:
	// 1. add unique names to anonymous struct, enum, union
	// 2. add values to any enum's enumerator-lists there may be
	stringInternal = giveNamesToAllAnonymous(stringInternal);
	stringInternal = giveNumbersForAllEnumEnumerators(stringInternal);
	// finally done with normalizing everything
	char* stringOut = copyStringToHeapString(stringInternal); // lets ensure a proper sized allocation area for this string
	cosmic_free(stringInternal);
	return stringOut;
}




struct TypeToken{
	int32_t stringIndexStart;
	int32_t stringIndexEnd;
	int16_t enclosementMatch;
	bool isKeywordOrTypedefed;
	bool isNonSymbol;
	char firstCharacter;
};

struct TypeTokenArray{
	struct TypeToken* typeTokens;
	int16_t* indexesForRearrangement;
	int16_t tokenLength;
	int16_t indexNext;
	int16_t indexLength; // indexLength is the allocation length of indexesForRearrangement, but indexesForRearrangement may be terminated earlier by a -1
};

struct TypeTokenArray typeTokenArray; // only one is needed at any time, so it is global


void writeIndexToNextSlotInTypeTokenArray(int16_t index){
	assert(index!=-1);
	if (typeTokenArray.indexNext<typeTokenArray.indexLength){
		typeTokenArray.indexesForRearrangement[typeTokenArray.indexNext++]=index;
		return;
	}
	// by the way, the code below rarely executes
	int16_t i;
	const int16_t l=(typeTokenArray.indexLength+1)*2;
	int16_t*const t=cosmic_malloc((typeTokenArray.indexLength+1)*(2*sizeof(int16_t)));
	for (i=0;i<typeTokenArray.indexLength;i++){
		t[i]=typeTokenArray.indexesForRearrangement[i];
	}
	t[typeTokenArray.indexNext++]=index;
	for (i=typeTokenArray.indexLength+1;i<l;i++){
		t[i]=-1;
	}
	cosmic_free(typeTokenArray.indexesForRearrangement);
	typeTokenArray.indexLength=l;
	typeTokenArray.indexesForRearrangement=t;
}

int16_t findStartForTypeTokens(int16_t boundStart, int16_t boundEnd){
	Start:;
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
				boundStart++;
				goto Start;
			}
		} else if (lengthOfBoundStartToken==6){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"struct")){
				doContainerStart = true;
				doStartSkip = true;
			}
		} else if (lengthOfBoundStartToken==8){
			if (isSectionOfStringEquivalent(sourceContainer.string,startIndexOfStartBound,"volatile")){
				boundStart++;
				goto Start;
			}
		}
	}
	// doContainerStart is used to more easily parse containers (struct, enum, union)
	if (doContainerStart){
		// this is to ensure that the containerStart is done only when actually needed (and to detangle the logic)
		if (boundStart+1>=boundEnd){
			err_1101_("Invalid enclosing boundaries when analyzing struct/union/enum type declaration",typeTokenArray.typeTokens[boundEnd].stringIndexStart);
		}
		bool isNotOnFirstToken;
		if (typeTokenArray.typeTokens[boundStart+1].firstCharacter=='{'){
			doContainerStart = true;
			isNotOnFirstToken = false;
		} else if (boundStart+2>=boundEnd){
			doContainerStart = false;
		} else if (typeTokenArray.typeTokens[boundStart+2].firstCharacter=='{'){
			doContainerStart = true;
			isNotOnFirstToken = true;
		} else {
			doContainerStart = false;
		}
		if (doContainerStart){
			// this is to ensure that the containerStart is done only when actually needed (and to detangle the logic)
			int16_t checkThisIndex = typeTokenArray.typeTokens[boundStart+1+isNotOnFirstToken].enclosementMatch+1;
			if (checkThisIndex>=boundEnd){
				return boundStart;
			} else {
				boundStart=checkThisIndex;
				goto Start;
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
	therefore, search for identifier, colon, or function pointer start point
	*/
	for (int16_t i0=boundStart+doStartSkip*2;i0<boundEnd;i0++){
		struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+i0;
		char firstCharacter=thisTypeTokenPtr->firstCharacter;
		if (firstCharacter=='[' | firstCharacter=='{'){
			i0 = thisTypeTokenPtr->enclosementMatch;
		} else if (firstCharacter==':' | (!thisTypeTokenPtr->isKeywordOrTypedefed & thisTypeTokenPtr->isNonSymbol)){
			return i0; // identifier or colon found
		} else if (firstCharacter=='('){
			uint16_t stage=0;
			int16_t i2=i0+1;
			for (int16_t i1=i2;i1<boundEnd;i1++){
				firstCharacter = typeTokenArray.typeTokens[i1].firstCharacter;
				switch (stage){
					case 0:
					stage=4-(firstCharacter=='*')*3;
					i2=i1;
					break;
					case 1:
					if (firstCharacter=='*'){
						i2=i1;
					} else if (firstCharacter=='['){
						i2=i1;
						i1=typeTokenArray.typeTokens[i1].enclosementMatch;
					} else {
						stage=4-(firstCharacter==')')*2;
					}
					break;
					case 2:
					stage=4-(firstCharacter=='(')*1;
					if (stage==3) return i2; // function pointer start point found
					break;
				}
				if (stage==4) break;
			}
		}
	}
	/*
	then this has no identifier or colon
	therefore, find the most inclosed token that isn't an enclosement
	*/
	int16_t mostInclosedToken = boundStart;
	int16_t amountOfEnclosementOnMostEnclosedToken = 0;
	int16_t amountOfEnclosementOnThisToken = 0;
	for (int16_t i=boundStart;i<boundEnd;i++){
		struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+i;
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
				err_1101_("Could not find valid starting place for type parsing around here",thisTypeTokenPtr->stringIndexStart);
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
			err_1101_("Invalid enclosing boundaries when looking for valid starting place for type parsing",thisTypeTokenPtr->stringIndexStart);
		}
	}
	// if nothing is enclosed, then this defaults to the last valid token (because amountOfEnclosementOnMostEnclosedToken==0)
	return mostInclosedToken;
}

void mainWalkForTypeTokens(int16_t, int16_t, int16_t, bool, int16_t, int16_t);
void splitterStarterForTypeTokens(int16_t, char);

void enumHandlerForTypeTokens(int16_t bracketIndex){
	int16_t endBracketIndex = typeTokenArray.typeTokens[bracketIndex].enclosementMatch;
	for (int16_t i=bracketIndex;i<=endBracketIndex;i++){
		writeIndexToNextSlotInTypeTokenArray(i);
	}
}

int16_t squareBracketHandlerForTypeTokens(int16_t squareBracketIndex0){
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
		writeIndexToNextSlotInTypeTokenArray(i);
	}
	return squareBracketIndex1;
}

void bracketHandlerForMainWalkForTypeTokens(int16_t boundStart, int16_t bracketIndex){
	if (bracketIndex-1<boundStart){
		goto Fail;
	}
	int16_t descriptorIndex;
	if (typeTokenArray.typeTokens[bracketIndex-1].isKeywordOrTypedefed){
		descriptorIndex = bracketIndex-1;
	} else {
		descriptorIndex = bracketIndex-2;
		if (bracketIndex-2<boundStart){
			goto Fail;
		}
		if (!typeTokenArray.typeTokens[bracketIndex-2].isKeywordOrTypedefed){
			goto Fail;
		}
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
		goto Fail;
	}
	if (outID!=1){
		splitterStarterForTypeTokens(bracketIndex,';');
	} else {
		enumHandlerForTypeTokens(bracketIndex);
	}
	return;
	Fail:;
	err_1101_("Invalid \'{\' in type, could not find appropriate matching keyword",typeTokenArray.typeTokens[bracketIndex].stringIndexStart);
}

void mainWalkForTypeTokens(int16_t boundStart, int16_t boundEnd, int16_t externalStart, bool doExternalStart, int16_t startIdentifierSkipCount, int16_t backwardSkipToIndex){
	int16_t startIdentifierSkipCountLeft=startIdentifierSkipCount;
	int16_t backwardSkipToIndexFuture=backwardSkipToIndex;
	int16_t startIndex;
	if (boundStart+1==boundEnd){
		writeIndexToNextSlotInTypeTokenArray(boundStart);
		return;
	}
	if (boundStart+1>boundEnd){
		err_1101_("(Internal) failure to parse type (ID:1)",typeTokenArray.typeTokens[boundStart].stringIndexStart);
	}
	bool ableToStartForward = false;
	bool hasParenEnd = false;
	bool needsFurtherSkip = false;
	startIndex=doExternalStart?typeTokenArray.typeTokens[externalStart].enclosementMatch+1:findStartForTypeTokens(boundStart,boundEnd);
	if (doExternalStart & startIdentifierSkipCount!=0){
		for (int16_t i=findStartForTypeTokens(boundStart,boundEnd);i<startIndex;i++){
			struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+i;
			char firstCharacter = thisTypeTokenPtr->firstCharacter;
			if (firstCharacter==','){
				if (--startIdentifierSkipCountLeft<0){
					err_1101_("Failure to parse type, it seems like this comma is not placed correctly",thisTypeTokenPtr->stringIndexStart);
				}
			} else if (firstCharacter=='(' | firstCharacter=='[' | firstCharacter=='{'){
				i = thisTypeTokenPtr->enclosementMatch;
			} else if (firstCharacter==':'){
				err_1101_("I do not support commas seperating bitfields at this time",thisTypeTokenPtr->stringIndexStart);
			}
		}
	}
	for (int16_t i=startIndex;i<boundEnd;i++){
		struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+i;
		char firstCharacter = thisTypeTokenPtr->firstCharacter;
		if (startIdentifierSkipCountLeft!=0){
			if (firstCharacter==','){
				if (--startIdentifierSkipCountLeft==0){
					if (i+1>=boundEnd){
						err_1101_("Failure to parse type, it seems like this comma is not placed correctly",thisTypeTokenPtr->stringIndexStart);
					}
					startIndex=findStartForTypeTokens(i+1,boundEnd);
					i=startIndex-1;
				}
			} else if (firstCharacter=='(' | firstCharacter=='[' | firstCharacter=='{'){
				i = thisTypeTokenPtr->enclosementMatch;
			} else if (firstCharacter==':'){
				err_1101_("I do not support commas seperating bitfields at this time",thisTypeTokenPtr->stringIndexStart);
			}
		} else {
			if (firstCharacter==','){
				needsFurtherSkip=true;
				startIdentifierSkipCount++;
				if (backwardSkipToIndexFuture==-1){
					backwardSkipToIndexFuture=i-1;
				}
				break;
			}
			if (firstCharacter==')'){
				hasParenEnd=true;
				break;
			}
			ableToStartForward = true;
			if (firstCharacter=='*' & i!=startIndex){
				err_1111_("Unexpected \'*\' while parsing type",thisTypeTokenPtr->stringIndexStart,thisTypeTokenPtr->stringIndexEnd);
			}
			if (firstCharacter=='['){
				i=squareBracketHandlerForTypeTokens(i);
			} else if (firstCharacter=='('){
				splitterStarterForTypeTokens(i,',');
				i = thisTypeTokenPtr->enclosementMatch;
			} else if (firstCharacter=='{'){
				bracketHandlerForMainWalkForTypeTokens(boundStart,i);
				i = thisTypeTokenPtr->enclosementMatch;
			} else {
				writeIndexToNextSlotInTypeTokenArray(i);
				if (firstCharacter==':'){
					struct TypeToken* typeTokenPtrToNumber = typeTokenArray.typeTokens+(i+1);
					if ((i+2!=boundEnd | doExternalStart) || !(typeTokenPtrToNumber->firstCharacter>='0' & typeTokenPtrToNumber->firstCharacter<='9')){
						// typeTokenPtrToNumber is not known to be valid at the point of giving this error, so we can't use it, so thisTypeTokenPtr->stringIndexEnd is used instead
						err_1101_("Bitfield size must be a single positive integer \nI do not support commas seperating bitfields at this time",thisTypeTokenPtr->stringIndexEnd);
					}
					struct NumberParseResult numberParseResult;
					parseNumber(&numberParseResult,sourceContainer.string,typeTokenPtrToNumber->stringIndexStart,typeTokenPtrToNumber->stringIndexEnd);
					if (numberParseResult.errorCode!=0 | numberParseResult.typeOfNonDecimal==0){
						err_1111_("Bitfield size must be a single positive integer",typeTokenPtrToNumber->stringIndexStart,typeTokenPtrToNumber->stringIndexEnd);
					}
					if (numberParseResult.valueUnion.value>16){
						err_1111_("Bitfield size cannot be larger than 16 (\'int\' is 16 bits)",typeTokenPtrToNumber->stringIndexStart,typeTokenPtrToNumber->stringIndexEnd);
					}
					if (i!=startIndex & numberParseResult.valueUnion.value==0){
						err_1111_("Bitfields of 0 size must have no identifier",typeTokenPtrToNumber->stringIndexStart,typeTokenPtrToNumber->stringIndexEnd);
					}
					writeIndexToNextSlotInTypeTokenArray(i+1);
					break;
				}
			}
		}
	}
	if (!ableToStartForward & !doExternalStart){
		err_1101_("Failure to parse type, it seems like either a comma or identifier is misplaced near here",typeTokenArray.typeTokens[startIndex].stringIndexStart);
	}
	bool hasAppliedCommaSkip = false;
	bool hasParenStart = false;
	int16_t parenStartIndex;
	startIndex=doExternalStart?externalStart-1:startIndex-1;
	for (int16_t i=startIndex;i>=boundStart;i--){
		struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+i;
		char firstCharacter = thisTypeTokenPtr->firstCharacter;
		if (firstCharacter==','){
			if (hasAppliedCommaSkip){
				err_1101_("Failure to parse type (ID:4)",thisTypeTokenPtr->stringIndexStart);
			} else if (backwardSkipToIndex==-1){
				err_1101_("Failure to parse type (ID:5)",thisTypeTokenPtr->stringIndexStart);
			} else {
				hasAppliedCommaSkip = true;
				i = backwardSkipToIndex;
				continue;
			}
		}
		if (firstCharacter=='('){
			hasParenStart=true;
			parenStartIndex=i;
			break;
		}
		if (firstCharacter=='}'){
			mainWalkForTypeTokens(boundStart,i+1,0,false,0,-1);
			break;
		}
		if (firstCharacter==']'){
			i=squareBracketHandlerForTypeTokens(i);
		} else if (firstCharacter==':'){
			err_1111_("Unexpected \':\' while parsing type",thisTypeTokenPtr->stringIndexStart,thisTypeTokenPtr->stringIndexEnd);
		} else {
			writeIndexToNextSlotInTypeTokenArray(i);
		}
	}
	if (hasParenStart | hasParenEnd){
		if (hasParenStart & hasParenEnd){
			mainWalkForTypeTokens(boundStart,boundEnd,parenStartIndex,true,startIdentifierSkipCount,backwardSkipToIndexFuture);
		} else {
			// this error probably should never happen, regardless of input
			err_1101_("(Internal) failure to parse type (ID:2)",typeTokenArray.typeTokens[boundStart].stringIndexStart);
		}
	}
	if (needsFurtherSkip){
		writeIndexToNextSlotInTypeTokenArray(-2);
		mainWalkForTypeTokens(boundStart,boundEnd,0,false,startIdentifierSkipCount,backwardSkipToIndexFuture);
	}
}

void splitterStarterForTypeTokens(int16_t boundStart, char splittingChar){
	int16_t boundEnd = typeTokenArray.typeTokens[boundStart].enclosementMatch;
	int16_t previousStart = boundStart+1;
	writeIndexToNextSlotInTypeTokenArray(boundStart);
	if (previousStart!=boundEnd){
		for (int16_t i=boundStart+1;i<boundEnd;i++){
			char c = typeTokenArray.typeTokens[i].firstCharacter;
			if (c == splittingChar){
				mainWalkForTypeTokens(previousStart,i,0,false,0,-1);
				previousStart = i+1;
				writeIndexToNextSlotInTypeTokenArray(i);
			} else if (c=='(' | c=='{'){
				i = typeTokenArray.typeTokens[i].enclosementMatch;
			}
		}
		if (splittingChar==','){
			mainWalkForTypeTokens(previousStart,boundEnd,0,false,0,-1);
		}
	}
	writeIndexToNextSlotInTypeTokenArray(boundEnd);
}


int16_t specificEnclosementMatchTypeTokens(int16_t startIndex){
	const char startChar = typeTokenArray.typeTokens[startIndex].firstCharacter;
	const char endingEnclosements[]=")]}";
	uint8_t typeOfEnclosement;
	if (startChar=='('){
		typeOfEnclosement=0;
	} else if (startChar=='['){
		typeOfEnclosement=1;
	} else if (startChar=='{'){
		typeOfEnclosement=2;
	} else {
		assert(false);
	}
	int16_t i=startIndex+1;
	char c;
	while ((c=typeTokenArray.typeTokens[i].firstCharacter)!=endingEnclosements[typeOfEnclosement]){
		if (c==')' | c==']' | c=='}' | i+1>=typeTokenArray.tokenLength){
			if (i+1>=typeTokenArray.tokenLength){
				err_1101_("Expected this opening bracket to match a closing bracket, but none existed",typeTokenArray.typeTokens[startIndex].stringIndexStart);
			} else {
				err_11000("Expected this closing bracket to match",typeTokenArray.typeTokens[i].stringIndexStart);
				err_1101_("Expected this opening bracket to match",typeTokenArray.typeTokens[startIndex].stringIndexStart);
			}
		}
		if (c=='(' | c=='[' | c=='{'){
			i = specificEnclosementMatchTypeTokens(i);
		}
		i++;
	}
	typeTokenArray.typeTokens[startIndex].enclosementMatch=i;
	typeTokenArray.typeTokens[i].enclosementMatch=startIndex;	
	return i;
}


void typeTokenizeForAnalysis(int32_t startIndex, int32_t endIndex){
	bool terminatedAtCorrectIndex = false;
	int16_t lengthCount = 0;
	for (int32_t i=startIndex;i<endIndex;){
		char c=sourceContainer.string[i];
		if (!(c==' ' | c=='\n')){
			if (++lengthCount>10000){
				err_1111_("This type declaration is too large (>10000 tokens)",startIndex,endIndex);
			}
			if (c=='L'){
				c=sourceContainer.string[i+1];
			}
			if (c=='\'' | c=='\"'){
				err_1111_("String and charcter literals may not be in type declarations",i,getEndOfToken(i));
			}
		}
		i = getEndOfToken(i);
		terminatedAtCorrectIndex = i==endIndex;
	}
	if (!terminatedAtCorrectIndex){
		// the endIndex is inside of a token. That's the calling function's problem, and it shouldn't happen
		err_1101_("(Internal), cannot agree on end of token",endIndex);
	}
	if (lengthCount==0){
		err_1111_("Expected type declaration here",startIndex,endIndex);
	}
	typeTokenArray.indexNext=0;
	typeTokenArray.tokenLength = lengthCount;
	typeTokenArray.indexLength = lengthCount;
	typeTokenArray.typeTokens = cosmic_malloc(sizeof(struct TypeToken)*lengthCount);
	typeTokenArray.indexesForRearrangement = cosmic_malloc(sizeof(int16_t)*lengthCount);
	for (int16_t i=0;i<lengthCount;i++) typeTokenArray.indexesForRearrangement[i]=-1;
	
	int32_t walkingIndex = 0;
	for (int32_t i=startIndex;i<endIndex;){
		int32_t tokenStart = i;
		i = getEndOfToken(i);
		char c=sourceContainer.string[tokenStart];
		if (!(c==' ' | c=='\n')){
			struct TypeToken* thisToken = typeTokenArray.typeTokens+(walkingIndex++);
			thisToken->stringIndexStart = tokenStart;
			thisToken->stringIndexEnd = i;
			thisToken->enclosementMatch = -1;
			thisToken->firstCharacter = c;
			thisToken->isNonSymbol = (c>='0' & c<='9') | (c>='A' & c<='Z') | (c>='a' & c<='z') | c=='_' | (c=='.' & i-tokenStart==3); // the . is for the ... for variadic arguments
			{
			const char* p;
			char buf[8];
			switch (i-tokenStart){
				case 8:
				p = sourceContainer.string+tokenStart;
				buf[0]=*(p+0);
				buf[1]=*(p+1);
				buf[2]=*(p+2);
				buf[3]=*(p+3);
				buf[4]=*(p+4);
				buf[5]=*(p+5);
				buf[6]=*(p+6);
				buf[7]=*(p+7);
				if (
				(buf[0]=='u' & buf[1]=='n' & buf[2]=='s' & buf[3]=='i' & buf[4]=='g' & buf[5]=='n' & buf[6]=='e' & buf[7]=='d') | //unsigned
				(buf[0]=='v' & buf[1]=='o' & buf[2]=='l' & buf[3]=='a' & buf[4]=='t' & buf[5]=='i' & buf[6]=='l' & buf[7]=='e')   //volatile
				){
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='r' & buf[1]=='e' & buf[2]=='g' & buf[3]=='i' & buf[4]=='s' & buf[5]=='t' & buf[6]=='e' & buf[7]=='r')   //register
				){
					err_1111_("This keyword is not allowed here",tokenStart,i);
				}
				break;
				case 6:
				p = sourceContainer.string+tokenStart;
				buf[0]=*(p+0);
				buf[1]=*(p+1);
				buf[2]=*(p+2);
				buf[3]=*(p+3);
				buf[4]=*(p+4);
				buf[5]=*(p+5);
				if (
				(buf[0]=='s' & buf[1]=='i' & buf[2]=='g' & buf[3]=='n' & buf[4]=='e' & buf[5]=='d') | //signed
				(buf[0]=='d' & buf[1]=='o' & buf[2]=='u' & buf[3]=='b' & buf[4]=='l' & buf[5]=='e')   //double
				){
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='s' & buf[1]=='t' & buf[2]=='r' & buf[3]=='u' & buf[4]=='c' & buf[5]=='t')   //struct
				){
					if (walkingIndex>=lengthCount){
						err_1111_("keyword \'struct\' cannot be at end of type declaration",tokenStart,i);
					}
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='s' & buf[1]=='t' & buf[2]=='a' & buf[3]=='t' & buf[4]=='i' & buf[5]=='c') | //static
				(buf[0]=='e' & buf[1]=='x' & buf[2]=='t' & buf[3]=='e' & buf[4]=='r' & buf[5]=='n') | //extern
				(buf[0]=='i' & buf[1]=='n' & buf[2]=='l' & buf[3]=='i' & buf[4]=='n' & buf[5]=='e')   //inline
				){
					err_1111_("This keyword is not allowed here",tokenStart,i);
				}
				break;
				case 5:
				p = sourceContainer.string+tokenStart;
				buf[0]=*(p+0);
				buf[1]=*(p+1);
				buf[2]=*(p+2);
				buf[3]=*(p+3);
				buf[4]=*(p+4);
				if (
				(buf[0]=='s' & buf[1]=='h' & buf[2]=='o' & buf[3]=='r' & buf[4]=='t') | // short
				(buf[0]=='_' & buf[1]=='B' & buf[2]=='o' & buf[3]=='o' & buf[4]=='l') | //_Bool
				(buf[0]=='c' & buf[1]=='o' & buf[2]=='n' & buf[3]=='s' & buf[4]=='t') | //const
				(buf[0]=='f' & buf[1]=='l' & buf[2]=='o' & buf[3]=='a' & buf[4]=='t')   //float
				){
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='u' & buf[1]=='n' & buf[2]=='i' & buf[3]=='o' & buf[4]=='n')   //union
				){
					if (walkingIndex>=lengthCount){
						err_1111_("keyword \'union\' cannot be at end of type declaration",tokenStart,i);
					}
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				break;
				case 4:
				p = sourceContainer.string+tokenStart;
				buf[0]=*(p+0);
				buf[1]=*(p+1);
				buf[2]=*(p+2);
				buf[3]=*(p+3);
				if (
				(buf[0]=='l' & buf[1]=='o' & buf[2]=='n' & buf[3]=='g') | //long
				(buf[0]=='c' & buf[1]=='h' & buf[2]=='a' & buf[3]=='r') | //char
				(buf[0]=='v' & buf[1]=='o' & buf[2]=='i' & buf[3]=='d')   //void
				){
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='e' & buf[1]=='n' & buf[2]=='u' & buf[3]=='m')   //enum
				){
					if (walkingIndex>=lengthCount){
						err_1111_("keyword \'enum\' cannot be at end of type declaration",tokenStart,i);
					}
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
				if (
				(buf[0]=='a' & buf[1]=='u' & buf[2]=='t' & buf[3]=='o')   //auto
				){
					err_1111_("This keyword is not allowed here",tokenStart,i);
				}
				break;
				case 3:
				p = sourceContainer.string+tokenStart;
				if (
				(p[0]=='i' & p[1]=='n' & p[2]=='t')   //int
				){
					thisToken->isKeywordOrTypedefed = true;
					continue;
				}
			}
			}
			thisToken->isKeywordOrTypedefed = isSectionOfStringTypedefed(sourceContainer.string,tokenStart,i);
		}
	}
	for (int16_t i=0;i<lengthCount;i++){
		char c = typeTokenArray.typeTokens[i].firstCharacter;
		if (c==')' | c==']' | c=='}'){
			err_1101_("Expected this closing bracket to match an opening bracket, but none existed",typeTokenArray.typeTokens[i].stringIndexStart);
		}
		if (c=='(' | c=='[' | c=='{'){
			i=specificEnclosementMatchTypeTokens(i);
		}
	}
}


char** convertType(int32_t startIndex, int32_t endIndex){
	typeTokenizeForAnalysis(startIndex,endIndex);
	mainWalkForTypeTokens(0,typeTokenArray.tokenLength,0,false,0,-1);
	// now we generate the string
	int16_t splitCount=0;
	char* resultString;
	{
		int16_t walkingIndex = 0;
		int32_t walkingCount = 0;
		int16_t valueIndex;
		while ((valueIndex=typeTokenArray.indexesForRearrangement[walkingIndex])!=-1){
			if (valueIndex==-2){
				walkingCount += 2;
			} else {
				struct TypeToken* thisTypeTokenPtr = typeTokenArray.typeTokens+valueIndex;
				walkingCount += (thisTypeTokenPtr->stringIndexEnd-thisTypeTokenPtr->stringIndexStart)+1;
			}
			if (++walkingIndex==typeTokenArray.indexLength) break;
		}
		resultString = cosmic_malloc(walkingCount);
		walkingIndex = 0;
		walkingCount = 0;
		int16_t encloseCount=0;
		while ((valueIndex=typeTokenArray.indexesForRearrangement[walkingIndex])!=-1){
			if (valueIndex==-2){
				splitCount+=(encloseCount==0);
				resultString[walkingCount++]=(encloseCount==0)?'@':';';
			} else {
				struct TypeToken thisTypeToken = typeTokenArray.typeTokens[valueIndex];
				encloseCount+=(thisTypeToken.firstCharacter=='{');
				encloseCount-=(thisTypeToken.firstCharacter=='}');
				for (int32_t i=thisTypeToken.stringIndexStart;i<thisTypeToken.stringIndexEnd;i++){
					resultString[walkingCount++]=sourceContainer.string[i];
				}
			}
			resultString[walkingCount++]=' ';
			if (++walkingIndex==typeTokenArray.indexLength) break;
		}
		resultString[walkingCount-1]=0;
		cosmic_free(typeTokenArray.typeTokens);
		cosmic_free(typeTokenArray.indexesForRearrangement);
	}
	if (!areEnclosementsValidInString(resultString)){
		err_1111_("Type parsing failed due to something unusual with parentheses, brackets, or braces",startIndex,endIndex);
	}
	char** strArr0=cosmic_calloc((splitCount+2),sizeof(char*));
	if (splitCount!=0){
		int32_t j=0;
		int32_t i=-1;
		int32_t s;
		int16_t n=0;
		char c;
		while ((c=resultString[++i])){
			if (c=='@'){
				s=i-j;
				assert(s>0);
				strArr0[n]=cosmic_malloc(s);
				s--;
				memcpy(strArr0[n],resultString+j,s);
				strArr0[n][s]=0;
				n++;
				j=i+2;
				i++;
				assert(resultString[i]!=0);
				assert(n<=splitCount);
			}
		}
		s=(i-j)+1;
		assert(s>0);
		strArr0[n]=cosmic_malloc(s);
		memcpy(strArr0[n],resultString+j,s);
		cosmic_free(resultString);
	} else {
		uint16_t errorValueForTypeNormalizer;
		if ((strArr0[0]=checkAndApplyTypeReorderAndNormalizationAnalysisToTypeStringToNew(resultString,&errorValueForTypeNormalizer,startIndex,endIndex))==NULL){
			if (errorValueForTypeNormalizer==1){
				err_1111_("Type keywords cannot be out of proper order",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==2){
				// this case is now impossible to reach
				err_1111_("Types cannot contain the character `\'` or `\"`",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==3){
				err_1111_("Types cannot contain nested array brackets",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==4){
				err_1111_("Failed to find the type to qualify as either const or volatile",startIndex,endIndex);
			} else {
				assert(false);
			}
			exit(1);
		}
		cosmic_free(resultString);
		return strArr0;
	}
	assert(strArr0[splitCount+1]==NULL);
	// do not use resultString beyond this point
	
	char** strArr1=cosmic_calloc((splitCount+2),sizeof(char*));
	bool hasDupedDefinition=false;
	char* dupedName=NULL;
	for (int16_t i=0;strArr0[i]!=NULL;i++){
		if (!areEnclosementsValidInString(strArr0[i])){
			err_1111_("Type parsing failed due to something unusual with parentheses, brackets, or braces",startIndex,endIndex);
		}
		uint16_t errorValueForTypeNormalizer;
		if ((strArr1[i]=checkAndApplyTypeReorderAndNormalizationAnalysisToTypeStringToNew(strArr0[i],&errorValueForTypeNormalizer,startIndex,endIndex))==NULL){
			if (errorValueForTypeNormalizer==1){
				err_1111_("Type keywords cannot be out of proper order",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==2){
				// this case is now impossible to reach
				err_1111_("Types cannot contain the character `\'` or `\"`",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==3){
				err_1111_("Types cannot contain nested array brackets",startIndex,endIndex);
			} else if (errorValueForTypeNormalizer==4){
				err_1111_("Failed to find the type to qualify as either const or volatile",startIndex,endIndex);
			} else {
				assert(false);
			}
			exit(1);
		}
		assert(errorValueForTypeNormalizer==0);
		assert(strArr1[i][0]!=0);
		cosmic_free(strArr0[i]);
		if (!areEnclosementsValidInString(strArr1[i])){
			err_1111_("Type parsing failed due to something unusual with parentheses, brackets, or braces",startIndex,endIndex);
		}
		if (i==0){
			const int32_t temporaryLenMinusOne=strlen(strArr1[0])-1;
			hasDupedDefinition=(strArr1[0][temporaryLenMinusOne]=='}');
			if (hasDupedDefinition){
				int32_t indexOfDupedDefinitionNameStart;
				int32_t indexOfDupedDefinitionNameEnd;
				indexOfDupedDefinitionNameEnd=getIndexOfMatchingEnclosement(strArr1[0],temporaryLenMinusOne)-1;
				indexOfDupedDefinitionNameStart=indexOfDupedDefinitionNameEnd;
				assert(indexOfDupedDefinitionNameStart>0);
				while (strArr1[0][--indexOfDupedDefinitionNameStart]!=' '){
					assert(indexOfDupedDefinitionNameStart>0);
				}
				++indexOfDupedDefinitionNameStart;
				dupedName=copyStringSegmentToHeap(strArr1[0],indexOfDupedDefinitionNameStart,indexOfDupedDefinitionNameEnd);
			}
		} else if (hasDupedDefinition){
			const int32_t temporaryLenMinusOne=strlen(strArr1[i])-1;
			if (strArr1[i][temporaryLenMinusOne]!='}'){
				err_1111_("(Internal) failure to parse type (ID:6)",startIndex,endIndex);
			}
			int32_t indexOfDupedDefinitionNameStartTemp=getIndexOfMatchingEnclosement(strArr1[i],temporaryLenMinusOne)-1;
			assert(indexOfDupedDefinitionNameStartTemp>0);
			while (strArr1[i][--indexOfDupedDefinitionNameStartTemp]!=' '){
				assert(indexOfDupedDefinitionNameStartTemp>0);
			}
			++indexOfDupedDefinitionNameStartTemp;
			char* s0=copyStringSegmentToHeap(strArr1[i],0,indexOfDupedDefinitionNameStartTemp);
			cosmic_free(strArr1[i]);
			strArr1[i]=strMerge2(s0,dupedName);
			cosmic_free(s0);
		}
	}
	if (hasDupedDefinition){
		cosmic_free(dupedName);
	}
	cosmic_free(strArr0);
	return strArr1;
}


