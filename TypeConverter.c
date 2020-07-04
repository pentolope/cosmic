

#include "Common.c"
#include "TypeParser.c"


uint32_t getSizeofForTypeString(const char*, bool);

/*
only strips what is at the base.
expects that typeString has no identifier.
return value should NOT be cosmic_freed.
hadVolatile and hadConst may be NULL.
*/
const char* stripQualifiersC(const char* typeString,bool* hadVolatile,bool* hadConst){
	if (hadVolatile!=NULL) *hadVolatile=false;
	if (hadConst!=NULL) *hadConst=false;
	const char* sub=typeString;
	if (isSectionOfStringEquivalent(typeString,0,"volatile ")){
		sub=stripQualifiersC(typeString+9,hadVolatile,hadConst);
		if (hadVolatile!=NULL) *hadVolatile=true;
	} else if (isSectionOfStringEquivalent(typeString,0,"const ")){
		sub=stripQualifiersC(typeString+6,hadVolatile,hadConst);
		if (hadConst!=NULL) *hadConst=true;
	}
	return sub;
}

static inline char* stripQualifiers(char* typeString,bool* hadVolatile,bool* hadConst){
	return (char*)stripQualifiersC(typeString,hadVolatile,hadConst);
}

// returns -1 if that space number doesn't exist
// the first space is n==0
int32_t getIndexOfNthSpace(const char* string, uint16_t n){
	uint16_t walkingN = 0;
	for (int32_t i=0;string[i];i++){
		if (string[i]==' '){
			if (walkingN==n){
				return i;
			} else {
				walkingN++;
			}
		}
	}
	return -1;
}

void copyDownForInPlaceEdit(char* string){
	int32_t length = strlen(string);
	int32_t walkingIndex=0;
	for (int32_t i=0;i<=length;i++){
		// yes, i<=length is desired, the null terminator needs to be copied too
		string[walkingIndex]=string[i];
		if (string[i]!=26){
			walkingIndex++;
		}
	}
}

// does a basic check to see if array brackets have one end for each beginning and are not nested
// returns true if the array brackets are invalid
bool isArrayBracketsInvalidOnTypeString(const char* string){
	int32_t length = strlen(string);
	for (int32_t i=0;i<length;i++){
		if (string[i]=='['){
			bool hasSeenAnEnd = false;
			bool hasSeenMultipleEnds = false;
			for (++i;i<length;i++){
				if (string[i]==']'){
					if (hasSeenAnEnd){
						hasSeenMultipleEnds = true;
					} else {
						hasSeenAnEnd = true;
					}
				} else if (string[i]=='['){
					i--; // step back to allow this open bracket to be caught in the parent loop
					break;
				}
			}
			if (hasSeenMultipleEnds | !hasSeenAnEnd){
				return true; // invalid
			}
		} else if (string[i]==']'){
			return true; // invalid
		}
	}
	return false; // valid
}



// performs operation in place
// expects any potential array brackets to match properly with no nesting
// expects curly brackets to match properly
void applyToTypeStringArrayDecayToSelf(char* string){
	for (int32_t i=0;string[i];i++){
		if (string[i]=='['){
			string[i++]='*';
			while (string[i]!=']'){
				if (string[i]==0){
					printf("Internal Error: typeString corrupted\n");
					exit(1);
				}
				string[i++]=26;
			}
			string[i]=26;
		} else if (string[i]=='{'){
			i = getIndexOfMatchingEnclosement(string,i);
		}
	}
	copyDownForInPlaceEdit(string);
}


// performs operation in place
// assumes no identifier
// only decays array at start of typeString
// returns true if it decayed, false otherwise
bool applyToTypeStringBaseArrayDecayToSelf(char* string){
	int32_t start=0;
	if (isSectionOfStringEquivalent(string,start,"const ")){
		start+=6;
	} else if (isSectionOfStringEquivalent(string,start,"volatile ")){
		start+=9;
	}
	if (string[start]=='['){
		int32_t indexOfOtherBracket = getIndexOfMatchingEnclosement(string,start);
		for (int32_t i=start;i<indexOfOtherBracket;i++){
			string[i]=26;
		}
		string[indexOfOtherBracket]='*';
		copyDownForInPlaceEdit(string);
		return true;
	}
	return false;
}



// allocates new string
// expects any potential array brackets to match properly with no nesting
char* applyToTypeStringBaseArrayDecayToNew(const char* string){
	char* outString = copyStringToHeapString(string);
	applyToTypeStringBaseArrayDecayToSelf(outString);
	return outString;
}

// performs operation in place
// assumes input has an identifier (and type information after it)
void applyToTypeStringRemoveIdentifierToSelf(char* string){
	int32_t indexOfFirstSpace = getIndexOfFirstSpaceInString(string);
	for (int32_t i=0;i<=indexOfFirstSpace;i++){
		string[i]=26;
	}
	copyDownForInPlaceEdit(string);
}


// allocates new string
// assumes input has an identifier (and type information after it)
char* applyToTypeStringRemoveIdentifierToNew(const char* string){
	return copyStringToHeapString(string+(getIndexOfFirstSpaceInString(string)+1));
}


// allocates new string
// assumes input has an identifier (and type information after it)
char* applyToTypeStringGetIdentifierToNew(const char* string){
	int32_t indexOfFirstSpace = getIndexOfFirstSpaceInString(string);
	char* outString = cosmic_malloc(indexOfFirstSpace+1);
	for (int32_t i=0;i<indexOfFirstSpace;i++){
		outString[i]=string[i];
	}
	outString[indexOfFirstSpace]=0;
	return outString;
}


// returns if the start of a type string is a struct or union. should not have an identifier or qualifiers.
bool isTypeStringOfStructOrUnion(const char* typeString){
	return isSectionOfStringEquivalent(typeString,0,"struct ") || isSectionOfStringEquivalent(typeString,0,"union ");
}


// returns a const string (NOT on heap), does not modify input. should not have identifier
const char* singleTypicalIntegralTypePromote(const char* typeString, bool* wasPromotable){
	const char* typeStringNQ = stripQualifiersC(typeString,NULL,NULL);
	const char* new;
	if (
		doStringsMatch(typeStringNQ,"int") ||
		doStringsMatch(typeStringNQ,"char") ||
		doStringsMatch(typeStringNQ,"unsigned char") ||
		doStringsMatch(typeStringNQ,"short") ||
		doStringsMatch(typeStringNQ,"_Bool") ||
		isSectionOfStringEquivalent(typeStringNQ,0,"enum ")
		){
		new="int";
	} else if (
		doStringsMatch(typeStringNQ,"unsigned int") ||
		doStringsMatch(typeStringNQ,"unsigned short")
		){
		new="unsigned int";
	} else if (
		doStringsMatch(typeStringNQ,"long")
		){
		new="long";
	} else if (
		doStringsMatch(typeStringNQ,"unsigned long")
		){
		new="unsigned long";
	} else {
		if (wasPromotable!=NULL) *wasPromotable=false;
		return typeStringNQ;
	}
	if (wasPromotable!=NULL) *wasPromotable=true;
	return new;
}



// expects all array brackets to match properly with no nesting
// arrayNumberFromStart starts at 0
char* insertSizeToEmptySizedArrayForTypeStringToNew(char* string, uint16_t arrayNumberFromStart, uint32_t sizeToInsert){
	char numberBuffer[14] = {0};
	snprintf(numberBuffer,13,"%lu",(unsigned long)sizeToInsert);
	uint8_t lengthOfNumber = strlen(numberBuffer);
	int32_t lengthOfOriginalString = strlen(string);
	int32_t newLengthForString = lengthOfOriginalString+lengthOfNumber+1;
	char* newString = cosmic_malloc(newLengthForString+1);
	uint16_t arrayCount = 0;
	int32_t walkingIndex = 0;
	for (int32_t i=0;i<lengthOfOriginalString;i++){
		if (walkingIndex>=newLengthForString){
			printf("Internal Error: Unexpected length error precatch (0)\n");
			exit(1);
		}
		newString[walkingIndex++]=string[i];
		if (string[i]=='['){
			if (arrayNumberFromStart==arrayCount){
				if (string[i+1]!=' ' | string[i+2]!=']'){
					printf("Internal Error: insertSizeToEmptySizedArrayForTypeStringToNew() called wrong\n");
					exit(1);
				}
				if (walkingIndex>=newLengthForString){
					printf("Internal Error: Unexpected length error precatch (1)\n");
					exit(1);
				}
				newString[walkingIndex++] = ' ';
				for (uint8_t numbersCharacterIndex=0;numbersCharacterIndex<lengthOfNumber;numbersCharacterIndex++){
					if (walkingIndex>=newLengthForString){
						printf("Internal Error: Unexpected length error precatch (2)\n");
						exit(1);
					}
					newString[walkingIndex++] = numberBuffer[numbersCharacterIndex];
				}
			}
			arrayCount++;
		}
	}
	if (walkingIndex!=newLengthForString){
		printf("Internal Error: Unexpected length error\n");
		exit(1);
	}
	newString[newLengthForString]=0;
	return newString;
}











