
/*
this file is to have stuff for parsing numbers (and knowing type endings)
this file is included by Common.c
*/
struct NumberParseResult{
	uint8_t typeOfDecimal;
	/*
	if typeOfDecimal==0 then the number is non-decimal (not a float,double,long double)
	if typeOfDecimal==1 then the number is a float
	if typeOfDecimal==2 then the number is a double
	if typeOfDecimal==3 then the number is a long double (currently not implemented)
	*/
	uint8_t typeOfNonDecimal;
	/*
	if typeOfNonDecimal==0 then the number is decimal (either a float,double,long double)
	if typeOfNonDecimal==1 then the number is a signed int
	if typeOfNonDecimal==2 then the number is a unsigned int
	if typeOfNonDecimal==3 then the number is a signed long
	if typeOfNonDecimal==4 then the number is a unsigned long
	if typeOfNonDecimal==5 then the number is a signed long long (currently not implemented)
	if typeOfNonDecimal==6 then the number is a unsigned long long (currently not implemented)
	*/
	union {
		uint32_t value;
		// float floatValue;
		// double doubleValue;
		// long double longDoubleValue; // currently not implemented
	} valueUnion;
	const char* typeString; // this string is not heap allocated
	uint8_t errorCode;
	/*
	if errorCode==0 then no error occured, and the number value and type is valid
	if errorCode==1 then the given range for the string was zero
	if errorCode==2 then the number did not start with a proper character
	if errorCode==3 then the number had a character that is not a valid character for the number
	if errorCode==4 then the number had an invalid suffix
	if errorCode==5 then the number had a suffix that could not contain the value of the number
	if errorCode>=6 then there was a different parsing error
	*/
};

// this function is not to be called outside of this file
void floatParse(
		struct NumberParseResult* numberParseResult, 
		const char* string, 
		int32_t startIndex, 
		int32_t endIndex, 
		int32_t indexOfDecimal,
		int32_t indexOfE,
		bool isFloatSuffixSpecified, 
		bool isLongDoubleSuffixSpecified,
		bool hasDecimalPoint,
		bool hasE){
	
	printf("floatParse() cannot run, float, double, and long double are not implemented.\n");
	exit(1);
	// todo: write decimal parsing
	
	/*
	
	struct I32{uint32_t v1;};
	struct I64{uint32_t v1; uint32_t v2;};
	struct I128{uint32_t v1; uint32_t v2; uint32_t v3; uint32_t v4;};
	union {
		struct I32 value1;
		struct I32 value2;
		struct I32 value3;
		float floatValue;
	} floatConvertionUnion;
	union {
		struct I64 value1;
		struct I64 value2;
		struct I64 value3;
		double doubleValue;
	} doubleConvertionUnion;
	union {
		struct I128 value1;
		struct I128 value2;
		struct I128 value3;
		long double longDoubleValue;
	} longDoubleConvertionUnion;
	*/
}


// this function is designed to be able to parse numbers into their internal format, including suffixes
void parseNumber(struct NumberParseResult* numberParseResult,const char* string, int32_t startIndex, int32_t endIndex){
	if (endIndex-startIndex<=0){
		numberParseResult->errorCode=1;
		return;
	}
	uint8_t decimalCount=0;
	uint8_t eCount=0;
	uint8_t xCount=0;
	bool isDecimalNumber = false;
	int32_t indexOfDecimal;
	int32_t indexOfE;
	int32_t indexOfX;
	for (int32_t i=startIndex;i<endIndex;i++){
		char c = string[i];
		if (c=='x' | c=='X'){
			xCount++;
			indexOfX=i;
		} else if (c=='.'){
			decimalCount++;
			isDecimalNumber = true;
			indexOfDecimal = i;
		} else if (c=='e' | c=='E'){
			eCount++;
			isDecimalNumber = true;
			indexOfE = i;
		} else if (c==' '){
			numberParseResult->errorCode=3;
			return;
		}
	}
	if (xCount==0){
		if (decimalCount>1 | eCount>1){
			numberParseResult->errorCode=6;
			return;
		}
	} else {
		isDecimalNumber = false;
		if (decimalCount!=0 | xCount>1 | indexOfX-startIndex!=1 | string[startIndex]!='0'){
			numberParseResult->errorCode=6;
			return;
		}
	}
	if (isDecimalNumber){
		char lastChar = string[endIndex-1];
		bool isFloatSuffixSpecified = lastChar=='f' | lastChar=='F';
		bool isLongDoubleSuffixSpecified = lastChar=='l' | lastChar=='L';
		bool hasDecimalPoint = decimalCount!=0;
		if (eCount!=0 & (indexOfE+1>=endIndex | (decimalCount==1 & indexOfE<indexOfDecimal))){
			numberParseResult->errorCode=7;
			return;
		}
		floatParse(
			numberParseResult,
			string,
			startIndex,
			endIndex,
			indexOfDecimal,
			indexOfE,
			isFloatSuffixSpecified,
			isLongDoubleSuffixSpecified,
			hasDecimalPoint,
			eCount!=0);
	} else if (xCount==0 & eCount!=0){
		numberParseResult->errorCode=3;
	} else {
		// this number is a non-decimal number (as in doesn't use a decimal point)
		int32_t endWithoutSuffix = endIndex;
		for (int32_t i=endIndex-1;i>startIndex;i--){
			char c = string[i];
			if (c=='L' | c=='l' | c=='U' | c=='u'){
				endWithoutSuffix=i;
			} else {
				break;
			}
		}
		uint8_t baseOfLiteral;
		if (endIndex-startIndex>1){
			if (string[startIndex]=='0'){
				if (string[startIndex+1]=='x' | string[startIndex+1]=='X'){
					baseOfLiteral = 16;
					startIndex+=2;
				} else {
					baseOfLiteral = 8;
				}
			} else {
				baseOfLiteral = 10;
			}
		} else {
			baseOfLiteral = 10;
		}
		uint32_t value = 0;
		for (int32_t i=startIndex;i<endWithoutSuffix;i++){
			uint8_t digit = string[i];
			if (digit>'9'){
				if ((digit>='a') & (digit<='f')){
					digit=(digit-'a')+10;
				} else if ((digit>='A') & (digit<='F')){
					digit=(digit-'A')+10;
				} else {
					digit=17; // this will cause a failure
				}
			} else {
				digit-='0';
			}
			if (digit>=baseOfLiteral){
				numberParseResult->errorCode=3;
				return;
			}
			value *= baseOfLiteral;
			value += digit;
		}
		uint8_t suffixType=0;
		/*
		if suffixType==0 then there was no suffix
		if suffixType==2 then the suffix was U
		if suffixType==3 then the suffix was L
		if suffixType==4 then the suffix was LL
		if suffixType==5 then the suffix was LU
		if suffixType==6 then the suffix was LLU
		if suffixType==9 then there was a suffix error
		*/
		uint8_t requiredType; // this has similiar codes as suffixType (requiredType==1 is int)
		uint8_t finalType; // this has similiar codes as suffixType    (finalType==1 is int)
		if (endIndex!=endWithoutSuffix){
			int32_t lengthOfSuffix = endIndex-endWithoutSuffix;
			char firstCharacter = string[endWithoutSuffix];
			bool isCapitals = firstCharacter>='A' & firstCharacter<='Z';
			char suffix[3];
			suffix[0]=firstCharacter;
			if (lengthOfSuffix==1){
				if (isCapitals){
					if (suffix[0]=='U'){
						suffixType=2;
					} else if (suffix[0]=='L'){
						suffixType=3;
					} else {
						suffixType=9;
					}
				} else {
					if (suffix[0]=='u'){
						suffixType=2;
					} else if (suffix[0]=='l'){
						suffixType=3;
					} else {
						suffixType=9;
					}
				}
			} else if (lengthOfSuffix==2){
				suffix[1]=string[endWithoutSuffix+1];
				if (isCapitals){
					if (suffix[0]=='L' & suffix[1]=='U'){
						suffixType=5;
					} else if (suffix[0]=='L' & suffix[1]=='L'){
						suffixType=4;
					} else {
						suffixType=9;
					}
				} else {
					if (suffix[0]=='l' & suffix[1]=='u'){
						suffixType=5;
					} else if (suffix[0]=='l' & suffix[1]=='l'){
						suffixType=4;
					} else {
						suffixType=9;
					}
				}
			} else if (lengthOfSuffix==3){
				suffix[1]=string[endWithoutSuffix+1];
				suffix[2]=string[endWithoutSuffix+2];
				if (isCapitals){
					if (suffix[0]=='L' & suffix[1]=='L' & suffix[2]=='U'){
						suffixType=6;
					} else {
						suffixType=9;
					}
				} else {
					if (suffix[0]=='l' & suffix[1]=='l' & suffix[2]=='u'){
						suffixType=6;
					} else {
						suffixType=9;
					}
				}
			}
		}
		if (suffixType==9){
			numberParseResult->errorCode=4;
			return;
		}
		if (value>32767LU){
			if (value>65535LU){
				if (value>2147483647LU){
					// in the future, this will keep checking larger numbers
					requiredType=5;
				} else {
					requiredType=3;
				}
			} else {
				requiredType=2;
			}
		} else {
			requiredType=1;
		}
		if (suffixType!=0 & requiredType>suffixType){
			numberParseResult->errorCode=5;
			return;
		}
		if (suffixType!=0){
			finalType = suffixType;
			// should this check if that suffix could hold the value?
		} else {
			finalType = requiredType;
		}
		numberParseResult->typeOfDecimal = 0;
		numberParseResult->typeOfNonDecimal = finalType;
		numberParseResult->valueUnion.value = value;
		numberParseResult->errorCode=0;
		if (finalType==1){
			numberParseResult->typeString = "const int";
		} else if (finalType==2){
			numberParseResult->typeString = "const unsigned int";
		} else if (finalType==3){
			numberParseResult->typeString = "const long";
		} else if (finalType==4){
			numberParseResult->typeString = "const long long";
			printf("long long literal not ready yet\n");
			exit(1);
		} else if (finalType==5){
			numberParseResult->typeString = "const unsigned long";
		} else if (finalType==6){
			numberParseResult->typeString = "const unsigned long long";
			printf("unsigned long long literal not ready yet\n");
			exit(1);
		} else {
			numberParseResult->errorCode=8;
		}
	}
}








