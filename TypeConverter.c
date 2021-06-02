

#include "Common.c"
#include "TypeParser.c"


uint32_t getSizeofForTypeString(const char*, bool);

/*
only strips what is at the base.
expects that typeString has no identifier.
return value should NOT be cosmic_free-d.
hadVolatile and hadConst may be NULL.
*/
const char* stripQualifiersC(const char* typeString,bool* hadVolatile,bool* hadConst){
	if (hadVolatile!=NULL) *hadVolatile=false;
	if (hadConst!=NULL) *hadConst=false;
	Repeat:;
	if (isPrefixOfStringEquivalent(typeString,"volatile ")){
		typeString+=9;
		if (hadVolatile!=NULL) *hadVolatile=true;
		goto Repeat;
	}
	if (isPrefixOfStringEquivalent(typeString,"const ")){
		typeString+=6;
		if (hadConst!=NULL) *hadConst=true;
		goto Repeat;
	}
	if (hadVolatile!=NULL & hadConst!=NULL){
		if (*hadConst & *hadVolatile){
			*hadVolatile=false;
			err_00__0("Type with \'volatile\' and \'const\' qualifier is being interpreted as not having the \'volatile\' qualifier");
		}
	}
	return typeString;
}

#define stripQualifiers(typeString,hadVolatile,hadConst) ((char*)stripQualifiersC((const char*)(typeString),hadVolatile,hadConst))


void copyDownForInPlaceEdit(char* string){
	int32_t walkingIndex=0;
	int32_t i=0;
	char c;
	do {
		c=(string[walkingIndex]=string[i++]);
		walkingIndex+=c!=26;
	} while (c!=0);
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
	int32_t startIndex=0;
	Start:;
	if (isPrefixOfStringEquivalent(string+startIndex,"const ")){
		startIndex+=6;
		goto Start;
	} else if (isPrefixOfStringEquivalent(string+startIndex,"volatile ")){
		startIndex+=9;
		goto Start;
	}
	if (string[startIndex]=='['){
		int32_t indexOfOtherBracket = getIndexOfMatchingEnclosement(string,startIndex);
		for (int32_t i=startIndex;i<indexOfOtherBracket;i++){
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
	return isPrefixOfStringEquivalent(typeString,"struct ") || isPrefixOfStringEquivalent(typeString,"union ");
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
		isPrefixOfStringEquivalent(typeStringNQ,"enum ")
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
char* insertSizeToEmptySizedArrayForTypeStringToNew(const char* string, uint16_t arrayNumberFromStart, uint32_t sizeToInsert){
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












