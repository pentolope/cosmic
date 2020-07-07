


#include "Common.c"


// doesFileExist isn't used now, but I might want it later
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


//TODO: this function doesn't really need to be in this file
void writeFileContentsToDisk(char* filePath, char* string){
    FILE *inputFile = fopen(filePath,"w");
    if (inputFile==NULL){
        printf("Null pointer given for file in writeFileContentsToDisk()\n");
		exit(1);
    }
    for (int32_t i=0;string[i];i++){
        char returnedValue = fputc(string[i],inputFile);
        if (returnedValue!=string[i]){
            fclose(inputFile);
            printf("error in writing file in function writeFileContentsToDisk()\n");
			exit(1);
        }
    }
    int returnedValue = fclose(inputFile);
    if (returnedValue!=0){
        printf("error in closing file that was being written to\n");
		exit(1);
    }
}



// also prepends and appends a newline
char* loadFileContentsToStringOnHeap(char* filePath){
    FILE *inputFile = fopen(filePath,"r");
    if (inputFile==NULL){
        printf("Error: could not load file: \"%s\"\n",filePath);
		exit(1);
    }
    fpos_t startingPositionOfFile;
    fgetpos(inputFile,&startingPositionOfFile);
    int32_t lengthOfFile = 0;
    while (fgetc(inputFile)!=EOF){
        lengthOfFile++;
        if (lengthOfFile>268435456L){ // 2**28
            fclose(inputFile);
            printf("Error: Input file \"%s\" is too long\n",filePath);
			exit(1);
        }
    }
    fsetpos(inputFile,&startingPositionOfFile);
    char *characterArrayOfFile = cosmic_malloc((lengthOfFile+3)*sizeof(char));
    characterArrayOfFile[0] = '\n';
    characterArrayOfFile[lengthOfFile+1] = '\n';
    characterArrayOfFile[lengthOfFile+2] = 0;
    for (int32_t i=1;i<lengthOfFile+1;i++){
        char currentCharacter = fgetc(inputFile);
        characterArrayOfFile[i]=currentCharacter;
        if (!(currentCharacter!=0 | currentCharacter!=16 | currentCharacter!=26)){
            fclose(inputFile);
            printf("Error: asserting ((char!=0) | (char!=16) | (char!=26)) failed for input file. I can't process \"%s\" as source code.\n",filePath);
			exit(1);
        }
    }
    fclose(inputFile);
    return characterArrayOfFile;
}


// used after the preprocesser is done so that the compiler can just manage a string
// will also trim the allocation of sourceContainer.sourceChar to only what it is using
void createSourceContainerString(){
	int32_t length=0;
	while (sourceContainer.sourceChar[length].c!=0){
		++length;
	}
	sourceContainer.sourceChar = cosmic_realloc(sourceContainer.sourceChar,(length+1)*sizeof(SourceChar));
	sourceContainer.allocationLength=length+1;
	sourceContainer.string = cosmic_malloc(length+1);
	for (int32_t i=0;i<length;i++){
		sourceContainer.string[i]=sourceContainer.sourceChar[i].c;
	}
	sourceContainer.string[length]=0;
}

// sourceStringLength is the current length of sourceContainer.sourceChar, terminated by a 0
void ensureSourceContainerLength(int32_t sourceStringLength, int32_t lengthIncrease){
	if (sourceContainer.allocationLength<=sourceStringLength+lengthIncrease+1){
		int32_t targetSize = sourceStringLength+lengthIncrease+1000;
		sourceContainer.sourceChar = cosmic_realloc(sourceContainer.sourceChar,targetSize*sizeof(SourceChar));
		for (int32_t i=sourceContainer.allocationLength;i<targetSize;i++){
			sourceContainer.sourceChar[sourceStringLength].c=0; // this is the only field that needs to be zero initialized
		}
		sourceContainer.allocationLength = targetSize;
	}
}

// assumes that the sourceContainerLength is at or larger than i
int32_t findSourceContainerLength(int32_t i){
	if (sourceContainer.allocationLength!=0){
		SourceChar* ptr = sourceContainer.sourceChar+i;
		while ((ptr++)->c) ++i;
	}
	return i;
}

/*
will shift everything that is currently there over.
this is used when inserting the first source file and for any include directives
*/
void insertArrayToSourceContainer(SourceChar* arrayOfSourceChar, int32_t index){
	int32_t inputStringLength = 0;
	while (arrayOfSourceChar[inputStringLength].c) inputStringLength++;
	
	int32_t sourceStringLength = findSourceContainerLength(index); // it couldn't be smaller then this, but otherwise it must be recalculated
	ensureSourceContainerLength(sourceStringLength,inputStringLength);
	sourceContainer.sourceChar[sourceStringLength+inputStringLength].c=0;
	SourceChar* p0=sourceContainer.sourceChar+(sourceStringLength-1);
	SourceChar* p1=sourceContainer.sourceChar+((sourceStringLength-1)+inputStringLength);
	int32_t c=sourceStringLength-index;
	while (c--) *(p1--)=*(p0--);
	
	p0=arrayOfSourceChar;
	p1=sourceContainer.sourceChar+index;
	c=inputStringLength;
	while (c--) *(p1++)=*(p0++);
	
}



// this is a different version of the above, the version below is more explanatory
#if 0
void insertArrayToSourceContainer(SourceChar* arrayOfSourceChar, int32_t index){
	int32_t inputStringLength = 0;
	while (arrayOfSourceChar[inputStringLength].c){
		inputStringLength++;
	}
	int32_t sourceStringLength = findSourceContainerLength(index); // it couldn't be smaller then this, but otherwise it must be recalculated
	ensureSourceContainerLength(sourceStringLength,inputStringLength);
	sourceContainer.sourceChar[sourceStringLength+inputStringLength].c=0;
	for (int32_t i=sourceStringLength-1;i>=index;i--){
		sourceContainer.sourceChar[i+inputStringLength] = sourceContainer.sourceChar[i];
	}
	for (int32_t i=0;i<inputStringLength;i++){
		sourceContainer.sourceChar[i+index] = arrayOfSourceChar[i];
	}
}
#endif





/*
the range [indexToStart,indexToEnd) will be removed and replaced by string
the source location populated from indexToStart
*/

void replaceAreaOfSourceContainer(char* string, int32_t indexToStart, int32_t indexToEnd){
	int32_t lengthOfString = strlen(string);
	SourceChar sourceCharForLocation = sourceContainer.sourceChar[indexToStart];
	{
		int32_t indexDiff=indexToEnd-indexToStart;
		if (indexDiff>lengthOfString){
			int32_t diff=indexDiff-lengthOfString;
			SourceChar* p0=sourceContainer.sourceChar+indexToEnd;
			SourceChar* p1=sourceContainer.sourceChar+(indexToEnd-diff);
			while (p0->c)  *(p1++)=*(p0++);
			while (p0!=p1) (p1++)->c=0;
		} else if (indexDiff<lengthOfString){
			int32_t sourceStringLength = findSourceContainerLength(indexToEnd);
			int32_t diff = lengthOfString-indexDiff;
			ensureSourceContainerLength(sourceStringLength,diff);
			SourceChar* p0=sourceContainer.sourceChar+sourceStringLength;
			SourceChar* p1=sourceContainer.sourceChar+(sourceStringLength+diff);
			int32_t c=(sourceStringLength-indexToEnd)+1;
			while (c--) *(p1--)=*(p0--);
		}
	}
	SourceChar* currentSourceChar = sourceContainer.sourceChar+indexToStart;
	for (int32_t si=0;si<lengthOfString;si++,currentSourceChar++){
		currentSourceChar->c=string[si];
		currentSourceChar->indexInOriginalSource=sourceCharForLocation.indexInOriginalSource;
		currentSourceChar->indexOfOriginalFile=sourceCharForLocation.indexOfOriginalFile;
	}
}


// the one below is a little slower

#if 0
void replaceAreaOfSourceContainer(char* string, int32_t indexToStart, int32_t indexToEnd){
	int32_t lengthOfString = strlen(string);
	SourceChar sourceCharForLocation = sourceContainer.sourceChar[indexToStart];
	{
		int32_t indexDiff=indexToEnd-indexToStart;
		if (indexDiff>lengthOfString){
			int32_t diff=indexDiff-lengthOfString;
			int32_t i;
			for (i=indexToEnd;sourceContainer.sourceChar[i].c;i++){
				sourceContainer.sourceChar[i-diff]=sourceContainer.sourceChar[i];
			}
			int32_t sourceStringLength = i;
			for (i=i-diff;i<sourceStringLength;i++){
				sourceContainer.sourceChar[i].c=0;
			}
		} else if (indexDiff<lengthOfString){
			int32_t sourceStringLength = findSourceContainerLength(indexToEnd);
			int32_t diff = lengthOfString-indexDiff;
			ensureSourceContainerLength(sourceStringLength,diff);
			for (int32_t i=sourceStringLength;i>=indexToEnd;i--){
				sourceContainer.sourceChar[i+diff]=sourceContainer.sourceChar[i];
			}
		}
	}
	SourceChar* currentSourceChar = sourceContainer.sourceChar+indexToStart;
	for (int32_t si=0;si<lengthOfString;si++,currentSourceChar++){
		currentSourceChar->c=string[si];
		currentSourceChar->indexInOriginalSource=sourceCharForLocation.indexInOriginalSource;
		currentSourceChar->indexOfOriginalFile=sourceCharForLocation.indexOfOriginalFile;
	}
}
#endif



/*
fills gaps of character 26 in by pulling all other characters down and setting the resulting empty space at end with 0
*/
void pullDownSourceChar(SourceChar* sourceChar){
	int32_t destinationIndex=0;
    int32_t sourceIndex=0;
    for (sourceIndex=0;sourceChar[sourceIndex].c;sourceIndex++){
        if (sourceChar[sourceIndex].c==26){
            sourceChar[destinationIndex].c = 0;
        } else {
            sourceChar[destinationIndex++]=sourceChar[sourceIndex];
        }
    }
    for (;destinationIndex<sourceIndex;destinationIndex++){
        sourceChar[destinationIndex].c = 0;
    }
}

// given index of `'` or `"` it will give the index of the ending `'` or `"` respectively
int32_t literalSkipSourceChar(SourceChar *sourceChar, int32_t start){
    if (sourceChar[start].c=='\''){
        start++;
        while (sourceChar[start].c!='\''){
            if (sourceChar[start].c==0){
				printf("literalSkip() detected unbounded char literal\n");
				exit(1);
            }
            if (sourceChar[start].c=='\\'){
                start++;
            }
            start++;
        }
        return start; // this parameter has been modified
    } else if (sourceChar[start].c=='\"'){
        start++;
        while (sourceChar[start].c!='\"'){
            if (sourceChar[start].c==0){
				printf("literalSkip() detected unbounded string literal\n");
				exit(1);
            }
            if (sourceChar[start].c=='\\'){
                start++;
            }
            start++;
        }
        return start; // this parameter has been modified
    } else {
		printf("literalSkip() got invalid character for beginning\n");
		exit(1);
    }
}



/*
given index of the asterisk in the start of a block comment, get index of the backslash that is at the end of the comment. set all characters of comment with ' '
assumes start index is valid
*/
int32_t commentBlockSetAndSkip(SourceChar *sourceChar, int32_t index){
    sourceChar[index-1].c=' ';
    sourceChar[index].c = ' ';
    index+=2;
    while (!((sourceChar[index-1].c=='*') & (sourceChar[index].c=='/'))){
        if (sourceChar[index].c==0){
			printf("commentBlockSetAndSkip() didn't find end\n");
			exit(1);
        }
        sourceChar[index-1].c=' ';
        index++;
    }
    sourceChar[index-1].c=' ';
    sourceChar[index].c = ' ';
    return index;
}


/*
given index of second slash in line comment start, get index of next newline. set all characters of comment with ' '
assumes start index is valid
*/
int32_t commentLineSetAndSkip(SourceChar *sourceChar, int32_t index){
    sourceChar[index-1].c=' ';
    sourceChar[index].c = ' ';
    while (sourceChar[index].c!='\n'){
        if (sourceChar[index].c==0){
			printf("commentLineSetAndSkip() didn't find end\n");
			exit(1);
        }
        sourceChar[index].c=' ';
        index++;
    }
    return index;
}

/*
replaces comments with spaces
*/
void commentStripSourceChar(SourceChar *sourceChar){
    if (sourceChar[0].c!=0){
        for (int32_t i=1;sourceChar[i].c;i++){
            if (sourceChar[i].c=='\'' | sourceChar[i].c=='\"'){
				i = literalSkipSourceChar(sourceChar,i);
            } else if (sourceChar[i-1].c=='/' & sourceChar[i].c=='*'){
                i = commentBlockSetAndSkip(sourceChar,i);
            } else if (sourceChar[i-1].c=='/' & sourceChar[i].c=='/'){
                i = commentLineSetAndSkip(sourceChar,i);
            }
        }
    }
}

void initialStripSourceChar(SourceChar *sourceChar){
	bool didSetOne=false;
	int32_t i;
	for (i=0;sourceChar[i].c;i++){
		if (sourceChar[i].c==13){
			// carriage return will be removed
			sourceChar[i].c=26;
			didSetOne=true;
		} else if (sourceChar[i].c==9){
			// tab characters are turned into spaces
			sourceChar[i].c=' ';
			didSetOne=true;
		}
	}
	if (didSetOne){
		pullDownSourceChar(sourceChar);
	}
	if (sourceChar[0].c!=0){
		didSetOne=false;
		for (i=1;sourceChar[i].c;i++){
			if ((sourceChar[i-1].c=='\\') & (sourceChar[i].c=='\n')){
				sourceChar[i-1].c=26;
				sourceChar[i  ].c=26;
				didSetOne=true;
			}
		}
		if (didSetOne){
			pullDownSourceChar(sourceChar);
		}
	}
}

/*
removes repeated spaces and newlines in raw code.
also changes the sequence "\n# " -> "\n#"  (for proper preprocesser directive support)
expects that character and string literals have not been converted to their true form (such as turning \n into a newline).
*/
void superStripSourceChar(SourceChar *sourceChar, int32_t startLocation){
    int32_t i;
	bool didSetOne;
	if (startLocation<=0){
		startLocation=1;
	}
	if (sourceChar[0].c=='\'' | sourceChar[0].c=='\"' | sourceChar[1].c=='\'' | sourceChar[1].c=='\"'){
		// this is to ensure that the following loops don't start inside a string literal
		printf("A string literal is not allowed here\n");
		exit(1);
	}
    if (sourceChar[0].c!=0){
		didSetOne=false;
        for (i=startLocation;sourceChar[i].c;i++){
            char previousChar = sourceChar[i-1].c;
            char currentChar  = sourceChar[i].c;
            if (currentChar=='\'' | currentChar=='\"'){
                i = literalSkipSourceChar(sourceChar,i);
            } else if (previousChar==' ' & (currentChar==' ' | currentChar=='\n')){
                sourceChar[i-1].c=26;
				didSetOne=true;
            }
        }
        if (didSetOne){
			pullDownSourceChar(sourceChar);
		}
    }
    if (sourceChar[0].c!=0){
		didSetOne=false;
        for (i=startLocation;sourceChar[i].c;i++){
            // newline characters are not allowed in strings anyway, so we don't need a literal skip
            if (sourceChar[i-1].c=='\n' & sourceChar[i].c==' '){
                sourceChar[i].c = 26;
				didSetOne=true;
            }
        }
        if (didSetOne){
			pullDownSourceChar(sourceChar);
		}
    }
    if (sourceChar[0].c!=0){
		didSetOne=false;
        for (i=startLocation;sourceChar[i].c;i++){
            // newline characters are not allowed in strings anyway, so we don't need a literal skip
            if (sourceChar[i-1].c=='\n' & sourceChar[i].c=='\n'){
                sourceChar[i-1].c = 26;
				didSetOne=true;
            }
        }
        if (didSetOne){
			pullDownSourceChar(sourceChar);
		}
    }
	if (sourceChar[0].c!=0 & sourceChar[1].c!=0 & sourceChar[startLocation].c!=0){
		didSetOne=false;
        for (i=startLocation+1;sourceChar[i].c;i++){
            // newline characters are not allowed in strings anyway, so we don't need a literal skip
            if (sourceChar[i-2].c=='\n' & sourceChar[i-1].c=='#' & sourceChar[i].c==' '){
                sourceChar[i].c = 26;
				didSetOne=true;
            }
        }
        if (didSetOne){
			pullDownSourceChar(sourceChar);
		}
    }
}

// expects that the sourceChar has repeated spaces eliminated
void adjacentStringConcatInSourceChar(SourceChar *sourceChar){
	int32_t i=0;
	if (sourceChar[0].c=='\'' | sourceChar[0].c=='\"'){ // this is to ensure that the following loop does not access outside the array bounds
		printf("A string literal shouldn't be at the first character\n");
		exit(1);
	}
	while (sourceChar[i].c!=0){
		char c = sourceChar[i].c;
		if (c=='\''){
			i = literalSkipSourceChar(sourceChar,i);
		} else if (c=='\"'){
			bool isThisWideLiteral = sourceChar[i-1].c=='L'; // does not access out of bounds due to check at beginning of function
			int32_t beginning=i;
			i = literalSkipSourceChar(sourceChar,i);
			if (sourceChar[i+2].c=='\"' & sourceChar[i+1].c==' ' & !isThisWideLiteral){
				int32_t endOfNextStringLiteral = literalSkipSourceChar(sourceChar,i+2);
				sourceChar[i+2].c = 26;
				sourceChar[i+1].c = 26;
				sourceChar[i  ].c = 26;
				i=beginning;
				continue;
			} else if (sourceChar[i+3].c=='\"' & sourceChar[i+2].c=='L' & sourceChar[i+1].c==' ' & isThisWideLiteral){
				int32_t endOfNextStringLiteral = literalSkipSourceChar(sourceChar,i+3);
				sourceChar[i+3].c = 26;
				sourceChar[i+2].c = 26;
				sourceChar[i+1].c = 26;
				sourceChar[i  ].c = 26;
				i=beginning;
				continue;
			}
		}
		i++;
	}
	pullDownSourceChar(sourceChar);
}






bool isNextNonSpaceOpenParen(int32_t startIndexInSourceString){
	int32_t i=startIndexInSourceString;
	if (sourceContainer.sourceChar[i].c=='('){
		return true;
	}
	while (sourceContainer.sourceChar[i].c==' '){
		if (sourceContainer.sourceChar[++i].c=='('){
			return true;
		}
	}
	return false;
}


char* copySegmentOfSourceContainerToHeapString(int32_t startIndex, int32_t endIndex){
	int32_t length=endIndex-startIndex;
	char* heapString = cosmic_malloc(length+1);
	for (int32_t i=0;i<length;i++){
		heapString[i] = sourceContainer.sourceChar[i+startIndex].c;
	}
	heapString[length]=0;
	return heapString;
}

/*
preprocesser tokens can use a uint8_t length to specify length of the token, and uint8_t to specify type.

tokens have 12 catagories:
 0 - null - termination token
 1 - stringStart - length of 1, declares that this is the start of a string
 2 - inString - declares that this is in a string. Things in string need not be split
 3 - stringEnd - length of 1, declares that this is the end of the string
 4 - text - declares that this is a segment of characters, numbers, and underscores
 5 - parenOpen - length of 1, declares that this is an opening parenthese
 6 - parenClose - length of 1, declares that this is a closing parenthese
 7 - symbols - declares that this is unimportant symbols
 8 - boundEnter - length of 0, not part of source, declares that this is the start of a (possible nested) bounded area
 9 - boundExit - length of 0, not part of source, declares that this is the end of a (possible nested) bounded area
10 - textNoExpand - same as text, except that it is not allowed to be macro expanded (due to previous expansion being avoided due to the macro being suppressed)
11 - comma - length of 1, this is a comma


the preprocesser attempts to go over the entire code input, 
	stopping at preprocessing directives, which may 
	1. modify macro substitution for later segments between directives
	2. be if-like directives that may remove sections of input
*/
typedef struct PreprocessToken{
	uint8_t catagory;
	uint8_t length;
} PreprocessToken;

typedef struct PreprocessTokens{
	PreprocessToken *preprocessTokens;
	int32_t allocationLength;
	int32_t startIndexInSourceString;
} PreprocessTokens;
PreprocessTokens preprocessTokens;


typedef struct PreprocessTokenWithContents{
	char *contents;// when catagory is 0, this is null
	PreprocessToken token;
} PreprocessTokenWithContents;


void nullOutPreprocessTokensAtAndAboveIndex(int32_t indexToStartAt){
	for (;indexToStartAt<preprocessTokens.allocationLength;indexToStartAt++){
		preprocessTokens.preprocessTokens[indexToStartAt].catagory = 0;
	}
}

// ensures that the preprocessTokens has the allocation length to handle the index given
void ensureLengthInPreprocessingTokens(int32_t i){
	if (i>=preprocessTokens.allocationLength){
		int32_t previousAllocationLength = preprocessTokens.allocationLength;
		preprocessTokens.allocationLength=i+100;
		preprocessTokens.preprocessTokens = cosmic_realloc(preprocessTokens.preprocessTokens,preprocessTokens.allocationLength*sizeof(PreprocessTokens));
		nullOutPreprocessTokensAtAndAboveIndex(previousAllocationLength);
	}
}

// returns the index of the closing parenthese that matches (includes nesting)
int32_t getIndexOfMatchingParenthesePreprocessingToken(int32_t indexOfOpenParenthese){
	int32_t walkingIndex = indexOfOpenParenthese;
	uint8_t currentCatagory = preprocessTokens.preprocessTokens[walkingIndex].catagory;
	if (currentCatagory!=5){
		printf("getIndexOfMatchingParenthesePreprocessingToken() called wrong\n");
		exit(1);
	}
	currentCatagory = preprocessTokens.preprocessTokens[++walkingIndex].catagory;
	while (currentCatagory!=6){
		if (currentCatagory==0){
			printf("getIndexOfMatchingParenthesePreprocessingToken() hit termination token\n");
			exit(1);
		} else if (currentCatagory==5){
			walkingIndex = getIndexOfMatchingParenthesePreprocessingToken(walkingIndex);
		}
		currentCatagory = preprocessTokens.preprocessTokens[++walkingIndex].catagory;
	}
	return walkingIndex;
}


bool isCharacterPartOfPreprocessTokenText(char c){
	return (c>=48 & c<=57) | (c>=65 & c<=90) | (c>=97 & c<=122) | c==95 | c==35; // the 35 is needed for identifying stringed arguments
}


// this will return 1 if the character is `'` or `"` , the rest of the string like token logic must be done elsewhere
uint8_t catagorizeCharacterForPreprocessToken(char c){
	if (isCharacterPartOfPreprocessTokenText(c)){
		return 4;
	} else if (c=='\'' | c=='\"'){
		return 1;
	} else if (c=='('){
		return 5;
	} else if (c==')'){
		return 6;
	} else if (c==','){
		return 11;
	} else {
		return 7;
	}
}


/*
returns the indexInString that it stopped at (which at this point is useless). will be the location of null if the string terminated, or the index of the '#' for a directive
if stopAtDirective==true, it will stop at the next directive (index out is the '#')
if stopAtDirective==false, it will stop at the next newline (index out is the '\n')
also erases all current preprocessing tokens
*/
int32_t generatePreprocessTokensUntilStopPoint(int32_t startIndexInString, bool stopAtDirective){
	uint8_t previousCatagory = 0;
	uint8_t currentCatagory;
	int32_t indexInTokens = -1; // this gets a +1 at the points a new token is added, and it is signed, so the -1 works
	int32_t indexInString = startIndexInString;
	
	nullOutPreprocessTokensAtAndAboveIndex(0);
	preprocessTokens.startIndexInSourceString = startIndexInString;
	
	for (;sourceContainer.sourceChar[indexInString].c;indexInString++){
		if (sourceContainer.sourceChar[indexInString].c=='\n'){
			if (stopAtDirective){
				if (sourceContainer.sourceChar[indexInString+1].c=='#'){ // null termination prevents out of bounds
					indexInString++;
					break;
				}
			} else {
				break;
			}
		}
		currentCatagory = catagorizeCharacterForPreprocessToken(sourceContainer.sourceChar[indexInString].c);
		if (currentCatagory==1){
			ensureLengthInPreprocessingTokens(++indexInTokens);
			preprocessTokens.preprocessTokens[indexInTokens].catagory = 1;
			preprocessTokens.preprocessTokens[indexInTokens].length = 1;
			
			int32_t stringEnd = literalSkipSourceChar(sourceContainer.sourceChar,indexInString);
			indexInString++;
			while (indexInString<stringEnd){
				int32_t distance = stringEnd-indexInString;
				uint8_t lengthOfMiddleToken;
				if (distance>=255){
					lengthOfMiddleToken=255;
				} else if (distance<=0){
					printf("preprocess string length error (1)\n");
					exit(1);
				} else {
					lengthOfMiddleToken=distance;
				}
				indexInString+=lengthOfMiddleToken;
				
				ensureLengthInPreprocessingTokens(++indexInTokens);
				preprocessTokens.preprocessTokens[indexInTokens].catagory = 2;
				preprocessTokens.preprocessTokens[indexInTokens].length = lengthOfMiddleToken;
			}
			if (indexInString!=stringEnd){
				printf("preprocess string length error (2)\n");
				exit(1);
			}
			ensureLengthInPreprocessingTokens(++indexInTokens);
			preprocessTokens.preprocessTokens[indexInTokens].catagory = 3;
			preprocessTokens.preprocessTokens[indexInTokens].length = 1;
			
			previousCatagory=3;
			
		} else if (currentCatagory==previousCatagory){
			if (preprocessTokens.preprocessTokens[indexInTokens].length++==255){
				printf("preprocess token length error (catagory %d)\n",currentCatagory);
				exit(1);
			}
		} else {
			ensureLengthInPreprocessingTokens(++indexInTokens);
			preprocessTokens.preprocessTokens[indexInTokens].catagory = currentCatagory;
			preprocessTokens.preprocessTokens[indexInTokens].length = 1;
			
			if (currentCatagory==5 | currentCatagory==6 | currentCatagory==11){
				previousCatagory=0;
			} else {
				previousCatagory = currentCatagory;
			}
		}
	}
	ensureLengthInPreprocessingTokens(++indexInTokens);
	preprocessTokens.preprocessTokens[indexInTokens].catagory = 0;
	
	return indexInString;
}


int32_t findLengthOfPreprocessingTokens(){
	int32_t i;
	for (i=0;preprocessTokens.preprocessTokens[i].catagory!=0;i++){
	}
	return i;
}


// this was the original insertSinglePreprocessingTokenAtIndex(), the one further below is a faster one 
#if 0
void insertSinglePreprocessingTokenAtIndex(PreprocessToken preprocessToken, int32_t index){
	int32_t lengthOfPreprocessingTokens = findLengthOfPreprocessingTokens();
	ensureLengthInPreprocessingTokens(lengthOfPreprocessingTokens+1);
	for (int32_t i=lengthOfPreprocessingTokens-1;i>=index;i--){
		preprocessTokens.preprocessTokens[i+1]=preprocessTokens.preprocessTokens[i];
	}
	preprocessTokens.preprocessTokens[index]=preprocessToken;
}
#endif

void insertSinglePreprocessingTokenAtIndex(PreprocessToken preprocessToken, int32_t index){
	int32_t end=index;
	if (preprocessTokens.preprocessTokens[index].catagory!=0){
		int32_t i=index+1;
		PreprocessToken pt0=preprocessTokens.preprocessTokens[index];
		PreprocessToken pt1;
		PreprocessToken* ptp=preprocessTokens.preprocessTokens+i;
		while (ptp->catagory!=0){
			pt1=*ptp;
			*ptp=pt0;
			pt0=pt1;
			i++;
			ptp++;
		}
		pt1=*ptp;
		*ptp=pt0;
		pt0=pt1;
		end=i;
	}
	ensureLengthInPreprocessingTokens(end+2);
	const PreprocessToken n={0};
	preprocessTokens.preprocessTokens[end+1]=n;
	preprocessTokens.preprocessTokens[index]=preprocessToken;
}

void removeSinglePreprocessingTokenAtIndex(int32_t index){
	int32_t i=index;
	int32_t pi;
	while (preprocessTokens.preprocessTokens[pi=i++].catagory!=0){
		preprocessTokens.preprocessTokens[pi]=preprocessTokens.preprocessTokens[i];
	}
}

// expects that the preprocessing tokens up to that point are valid, of course
int32_t getIndexInStringAtStartOfPreprocessingToken(int32_t index){
	int32_t stringIndex = preprocessTokens.startIndexInSourceString;
	for (int32_t i=0;i<index;i++){
		stringIndex+=preprocessTokens.preprocessTokens[i].length;
		if (preprocessTokens.preprocessTokens[i].catagory==0){
			printf("getIndexInStringAtStartOfPreprocessingToken() failed with token termination\n");
			exit(1);
		}
	}
	return stringIndex;
}

int32_t getIndexOfEndingBoundFromStartBound(int32_t startBoundIndex){
	int32_t currentIndex = startBoundIndex+1;
	uint8_t currentCatagory = preprocessTokens.preprocessTokens[startBoundIndex].catagory;
	if (currentCatagory!=8){
		printf("get end bound for preprocessing tokens called incorrectly\n");
		exit(1);
	}
	currentCatagory = preprocessTokens.preprocessTokens[currentIndex].catagory;
	while (currentCatagory!=0 & currentCatagory!=9){
		if (currentCatagory==8){
			currentIndex = getIndexOfEndingBoundFromStartBound(currentIndex);
		}
		currentCatagory = preprocessTokens.preprocessTokens[++currentIndex].catagory;
	}
	if (currentCatagory==0){
		printf("get end bound for preprocessing tokens hit termination\n");
		exit(1);
	}
	return currentIndex;
}


// the start and end indexes are as if the indexes don't change. They do, but... that's part of what this abstraction is for.
// maybe someday I will make this run faster. but I don't have time now.
void placeBoundsIntoPreprocessingTokens(int32_t startBoundIndex, int32_t endBoundIndex){
	PreprocessToken tempPreprocessToken;
	tempPreprocessToken.catagory=9;
	tempPreprocessToken.length=0;
	insertSinglePreprocessingTokenAtIndex(tempPreprocessToken,endBoundIndex);
	tempPreprocessToken.catagory=8;
	insertSinglePreprocessingTokenAtIndex(tempPreprocessToken,startBoundIndex);
}


void removeBoundsInPreprocessingTokens(int32_t startBoundIndex){
	int32_t endBoundIndex = getIndexOfEndingBoundFromStartBound(startBoundIndex);
	removeSinglePreprocessingTokenAtIndex(endBoundIndex);
	removeSinglePreprocessingTokenAtIndex(startBoundIndex);
}


// this length is referring to the number of tokens
int32_t findLengthOfPreprocessTokensWithContents(PreprocessTokenWithContents* tokens){
	int32_t length=0;
	while (tokens[length++].token.catagory!=0){
	}
	return length-1;
}


char* convertPreprocessTokensWithContentsToString(PreprocessTokenWithContents* tokens){
	int32_t lengthOfString=0;
	int32_t tokenIndex=0;
	while (tokens[tokenIndex].token.catagory!=0){
		lengthOfString+=tokens[tokenIndex].token.length;
		if (tokens[tokenIndex].token.length!=strlen(tokens[tokenIndex].contents)){ // this test is mostly just for debug
			printf("internal token to string conversion error\n");
			exit(1);
		}
		tokenIndex++;
	}
	int32_t tokenLength = tokenIndex;
	int32_t walkingIndex=0;
	char *newString = cosmic_malloc(lengthOfString+1);
	for (tokenIndex=0;tokenIndex<tokenLength;tokenIndex++){
		for (int32_t i=0;tokens[tokenIndex].contents[i];i++){
			newString[walkingIndex++]=tokens[tokenIndex].contents[i];
		}
	}
	newString[lengthOfString]=0;
	return newString;
}

// expects the area of source string and preprocessing tokens before the matching end bound to be valid and up to date with each other
// maintains the bounds tokens. If the bounds tokens are not needed after this however, they should be removed after the function completes.
void preprocesserBoundedReplace(PreprocessTokenWithContents* tokens, int32_t startBoundIndex){
	int32_t endBoundIndex = getIndexOfEndingBoundFromStartBound(startBoundIndex);
	for (int32_t i=startBoundIndex+1;i<endBoundIndex;i++){
		uint8_t tempCatagory = preprocessTokens.preprocessTokens[i].catagory;
		if (tempCatagory==8 | tempCatagory==9){
			printf("preprocessing token replacement error: bounds in range of bounds to be replaced [%d,%d,%d]\n",i,startBoundIndex,endBoundIndex);
			exit(1);
		}
	}
	char* stringToInsert = convertPreprocessTokensWithContentsToString(tokens);
	int32_t stringIndexStartBeforeReplace = getIndexInStringAtStartOfPreprocessingToken(startBoundIndex);
	int32_t stringIndexEndBeforeReplace = getIndexInStringAtStartOfPreprocessingToken(endBoundIndex);
	int32_t stringLengthOfRemoval = stringIndexEndBeforeReplace-stringIndexStartBeforeReplace;
	int32_t stringLengthOfInsert = strlen(stringToInsert);
	int32_t stringIndexEndAfterReplace = stringIndexStartBeforeReplace+stringLengthOfInsert;
	replaceAreaOfSourceContainer(stringToInsert,stringIndexStartBeforeReplace,stringIndexEndBeforeReplace);
	// these next 2 loops have horrible time complexity... and there is a better way to do them...
	for (int32_t i=endBoundIndex-1;i>startBoundIndex;i--){
		removeSinglePreprocessingTokenAtIndex(i);
	}
	for (int32_t i=0;tokens[i].token.catagory!=0;i++){
		insertSinglePreprocessingTokenAtIndex(tokens[i].token,i+startBoundIndex+1);
	}
	cosmic_free(stringToInsert);
}




PreprocessTokenWithContents* replaceTokenWithTokensForPreprocessTokenWithContents(
		PreprocessTokenWithContents* tokensToOperateOn,
		PreprocessTokenWithContents* tokensToInsert,
		int32_t tokenToReplace){
	
	int32_t length1 = 0;
	while (tokensToOperateOn[length1].token.catagory!=0){
		length1++;
	}
	int32_t length2 = 0;
	while (tokensToInsert[length2].token.catagory!=0){
		length2++;
	}
	cosmic_free(tokensToOperateOn[tokenToReplace].contents);
	for (int32_t i=tokenToReplace;i<length1;i++){
		tokensToOperateOn[i]=tokensToOperateOn[i+1];
	}
	tokensToOperateOn = cosmic_realloc(
		tokensToOperateOn,
		(length1+length2)*sizeof(PreprocessTokenWithContents));
	for (int32_t i=length1-1;i>=tokenToReplace;i--){
		tokensToOperateOn[i+length2]=tokensToOperateOn[i];
	}
	int32_t tempIndex;
	for (int32_t i=0;i<length2;i++){
		tempIndex = i+tokenToReplace;
		tokensToOperateOn[tempIndex]=tokensToInsert[i];
		tokensToOperateOn[tempIndex].contents = copyStringToHeapString(tokensToOperateOn[tempIndex].contents);
		// copying the string here helps to control what is responcible for free-ing the string
	}
	return tokensToOperateOn;
}


PreprocessTokenWithContents* convertStringToHeapPreprocessTokenWithContents(char* string){
	int32_t length = strlen(string);
	PreprocessTokenWithContents* tokens = cosmic_calloc(length+1,sizeof(PreprocessTokenWithContents));// likely oversized, but it will always work
	uint8_t currentCatagory;
	int32_t indexInTokens = 0;
	uint8_t *catagoryAtIndex = cosmic_calloc(length,1);
	uint8_t *lengthAtIndex = cosmic_calloc(length,1);
	uint8_t temp;
	for (int32_t i=0;i<length;i++){
		temp = catagorizeCharacterForPreprocessToken(string[i]);
		catagoryAtIndex[i] = temp;
		if (temp==1){
			i++;
			while (catagorizeCharacterForPreprocessToken(string[i])!=1){
				if (string[i]=='\\'){
					catagoryAtIndex[i++]=2;
					if (i>=length){
						printf("convertStringToHeapPreprocessTokenWithContents() got invalid string termination\n");
						exit(1);
					}
				}
				catagoryAtIndex[i++]=2;
				if (i>=length){
					printf("convertStringToHeapPreprocessTokenWithContents() got invalid string termination\n");
					exit(1);
				}
			}
			catagoryAtIndex[i]=3;
		}
	}
	int32_t lastSwitch=0;
	uint8_t previousCatagory=0;
	for (int32_t i=0;i<length;i++){
		if (catagoryAtIndex[i]!=previousCatagory){
			lastSwitch=i;
		}
		previousCatagory=catagoryAtIndex[i];
		if (previousCatagory==5 | previousCatagory==6 | previousCatagory==11){
			previousCatagory=0;
		}
		if (lengthAtIndex[lastSwitch]==255){ // mainly for long strings
			lastSwitch=i;
		}
		lengthAtIndex[lastSwitch]++;
	}
	int32_t walkingIndex=0;
	PreprocessToken tempToken;
	for (int32_t i=0;i<length;i++){
		if (lengthAtIndex[i]!=0){
			tempToken.length=lengthAtIndex[i];
			tempToken.catagory=catagoryAtIndex[i];
			tokens[walkingIndex].contents = copyStringSegmentToHeap(string,i,i+tempToken.length);
			tokens[walkingIndex++].token=tempToken;
		}
	}
	cosmic_free(lengthAtIndex);
	cosmic_free(catagoryAtIndex);
	return tokens;
}



PreprocessTokenWithContents* convertBoundedAreaToHeapPreprocessTokenWithContents(int32_t startBoundIndex){
	int32_t endBoundIndex = getIndexOfEndingBoundFromStartBound(startBoundIndex);
	int32_t length = endBoundIndex-startBoundIndex-1;
	PreprocessTokenWithContents* tokens = cosmic_calloc(length+1,sizeof(PreprocessTokenWithContents));
	PreprocessTokenWithContents* tokenPtr;
	int32_t tempStringStart;
	int32_t tempStringEnd;
	int32_t tokenIndexInMain;
	for (int32_t tokenIndex=0;tokenIndex<length;tokenIndex++){
		tokenPtr = &(tokens[tokenIndex]);
		tokenIndexInMain = tokenIndex+startBoundIndex+1;
		tempStringStart = getIndexInStringAtStartOfPreprocessingToken(tokenIndexInMain);
		tempStringEnd = getIndexInStringAtStartOfPreprocessingToken(tokenIndexInMain+1);
		tokenPtr->contents = copySegmentOfSourceContainerToHeapString(tempStringStart,tempStringEnd);
		tokenPtr->token = preprocessTokens.preprocessTokens[tokenIndexInMain];
		if (tokenPtr->token.catagory==8 | tokenPtr->token.catagory==9){
			printf("Internal Error: copy of bounds in preprocessTokenWithContents not allowed\n");
			exit(1);
		}
	}
	return tokens;
}


PreprocessTokenWithContents* rangeCopyWithoutContentsCopyForPreprocessTokenWithContents(PreprocessTokenWithContents* tokensIn, int32_t startIndex, int32_t endIndex){
	int32_t length = endIndex-startIndex;
	PreprocessTokenWithContents* tokensOut = cosmic_calloc(length+1,sizeof(PreprocessTokenWithContents));
	for (int32_t i=0;i<length;i++){
		tokensOut[i]=tokensIn[i+startIndex];
	}
	return tokensOut;
}


void freePreprocessTokenWithContents(PreprocessTokenWithContents* tokens){
	int32_t i=0;
	while (tokens[i].token.catagory!=0){
		cosmic_free(tokens[i].contents);// these contents need to be free-ed
		i++;
	}
	cosmic_free(tokens);
}


typedef struct SourceFileOriginal{
	char *fileName;
	char *originalContentOfFile;
	bool hadBrackets;
} SourceFileOriginal;

// this holds an array of the file names
typedef struct SourceFileListing{
	uint8_t allocationCount;
	SourceFileOriginal sourceFileOriginals[255];
} SourceFileListing;
SourceFileListing sourceFileListing;


uint8_t getPotentiallyNewIDforFile(char* fileName, bool hadBrackets){
	for (uint8_t i=0;i<sourceFileListing.allocationCount;i++){
		if ((hadBrackets==sourceFileListing.sourceFileOriginals[i].hadBrackets) & 
			doStringsMatch(fileName,sourceFileListing.sourceFileOriginals[i].fileName)){
			
			return i; // this is a duplicate file, so it gets the same ID
		}
	}
	if (sourceFileListing.allocationCount==255){
		printf("File count exceeded 255, preprocesser halting.\n");
		exit(1);
	} else {
		// then it doesn't match anything we currently have, so lets mark it down
		SourceFileOriginal sfo;
		sfo.fileName=copyStringToHeapString(fileName);
		sfo.originalContentOfFile=loadFileContentsToStringOnHeap(fileName);
		sfo.hadBrackets=hadBrackets;
		sourceFileListing.sourceFileOriginals[sourceFileListing.allocationCount]=sfo;
		return sourceFileListing.allocationCount++;
	}
}


// fileName should have double quotes around it if it had quotes when it was include-ed or was the file designated from the command line
// fileName should have angle brackets <> if it had angle brackets when it was include-ed
void insertFileIntoSourceContainer(const char* fileName, int32_t characterIndexToInsertAt){
	int32_t lengthOfFileName = strlen(fileName);
	if (lengthOfFileName<3){
		printf("That file name is too small to be valid (%s)\n",fileName);
		exit(1);
	}
	char* fileNameWithoutEdges = copyStringSegmentToHeap(fileName,1,lengthOfFileName-1);
	for (uint32_t i=0;fileNameWithoutEdges[i];i++){
		char c=fileNameWithoutEdges[i];
		if (c=='\"' | c=='<' | c=='>'){
			printf("filename has odd bounds [%s]",fileName);
			exit(1);
		}
	}
	char firstChar = fileName[0];
	char lastChar = fileName[lengthOfFileName-1];
	bool hadBrackets=false;
	if (firstChar=='\"' & lastChar=='\"'){
		// if characterIndexToInsertAt==0 then this is the initial file, so SourceContainer is not ready yet
		if (characterIndexToInsertAt!=0){
			// if characterIndexToInsertAt!=0 then the directory of the file that the current character came from should be where to include the file from
			// it is better to check several characters before the characterIndexToInsertAt for multiple reasons
			uint8_t fileID = sourceContainer.sourceChar[characterIndexToInsertAt-3].indexOfOriginalFile;
			if (fileID==255){
				printf("Number of files exceeded 254, and therefore this file\'s information is insufficent to perform an #include directive\n");
				exit(1);
			}
			char* directoryStr = copyStringToHeapString(sourceFileListing.sourceFileOriginals[fileID].fileName);
			{
				// find last slash in directoryStr 
				// and remove contents of string after the last slash, removing everything if there is no slash
				int32_t i=strlen(directoryStr);
				while (i!=0){
					if (directoryStr[--i]=='/'){
						++i;
						break;
					}
				}
				while (directoryStr[i]){
					directoryStr[i++]=0;
				}
			}
			char* newStr = strMerge2(directoryStr,fileNameWithoutEdges);
			cosmic_free(directoryStr);
			cosmic_free(fileNameWithoutEdges);
			fileNameWithoutEdges=newStr;
		}
	} else if (firstChar=='<' & lastChar=='>'){
		hadBrackets=true;
		char* newStr = strMerge2("StandardHeaders/",fileNameWithoutEdges);
		cosmic_free(fileNameWithoutEdges);
		fileNameWithoutEdges=newStr;
	} else {
		printf("insertFileIntoSourceContainer() doesn\'t know where to look for the file name (it didn\'t have angled brackets or double quotes)\n");
		exit(1);
	}
	uint8_t fileID = getPotentiallyNewIDforFile(fileNameWithoutEdges,hadBrackets);
	char* fileContents = sourceFileListing.sourceFileOriginals[fileID].originalContentOfFile;
	int32_t fileContentsLength = strlen(fileContents);
	SourceChar *sourceChar = cosmic_malloc((fileContentsLength+1)*sizeof(SourceChar));
	SourceChar tempSourceChar = {0};
	tempSourceChar.indexOfOriginalFile = fileID;
	sourceChar[fileContentsLength]=tempSourceChar;// null ending
	for (int32_t i=0;i<fileContentsLength;i++){
		tempSourceChar.c=fileContents[i];
		tempSourceChar.indexInOriginalSource=i;
		sourceChar[i]=tempSourceChar;
	}
	initialStripSourceChar(sourceChar);
	commentStripSourceChar(sourceChar);
	superStripSourceChar(sourceChar,0);
	insertArrayToSourceContainer(sourceChar,characterIndexToInsertAt);
	cosmic_free(sourceChar);
	cosmic_free(fileNameWithoutEdges);
}


// this struct is for error messages
typedef struct LineNumAndCharNum{
	int32_t charNumInSourceContainer; // this is the character index in the entire sourceContainer
	int32_t lineNumInOriginal;
	int32_t charNumInLineInOriginal; // this is the character in the given line
	int32_t charNumInOriginal; // this is the character index in the entire original file
	char* originalContents;
	char* fileName;
	uint8_t fileID;
} LineNumAndCharNum;


LineNumAndCharNum getLineNumAndCharNumFromSourceContainerIndex(int32_t index){
	LineNumAndCharNum lineNumAndCharNum = {0};
	lineNumAndCharNum.charNumInSourceContainer = index;
	lineNumAndCharNum.charNumInOriginal = sourceContainer.sourceChar[index].indexInOriginalSource;
	lineNumAndCharNum.fileID = sourceContainer.sourceChar[index].indexOfOriginalFile;
	if (lineNumAndCharNum.fileID == 255){
		printf("Internal Error: fileID==255\n");
		exit(1);
	}
	lineNumAndCharNum.fileName = sourceFileListing.sourceFileOriginals[lineNumAndCharNum.fileID].fileName;
	lineNumAndCharNum.originalContents = sourceFileListing.sourceFileOriginals[lineNumAndCharNum.fileID].originalContentOfFile;
	for (int32_t i=0;i<lineNumAndCharNum.charNumInOriginal;i++){
		++lineNumAndCharNum.charNumInLineInOriginal;
		if (lineNumAndCharNum.originalContents[i]=='\n'){
			lineNumAndCharNum.charNumInLineInOriginal=0;
			++lineNumAndCharNum.lineNumInOriginal;
		}
	}
	return lineNumAndCharNum;
}

void normalizeInformativePrintStringEnd(char* str){
	for (uint8_t i=0;i<ERR_MSG_LEN*2;i++){
		if (str[i]==0){
			--i;
			while (str[i]==' '){
				if (--i==0) break;
			}
			++i;
			for (uint8_t c=i;c<ERR_MSG_LEN*2;c++){
				str[c]=0;
			}
			return;
		}
	}
}


/*
exit() is not called in this function (unless the function itself has an error)
if (indexStart==-1) then no location will be emmitted
else if (indexEnd<=indexStart) then only the arrow will be printed, not the tildas after it
*/
void printInformativeMessageAtSourceContainerIndex(bool isError,const char* message, int32_t indexStart, int32_t indexEnd){
	char tempString1[ERR_MSG_LEN*2+1];
	char tempString2[ERR_MSG_LEN*2+1];
	char arrowString1[ERR_MSG_LEN*2+1];
	char arrowString2[ERR_MSG_LEN*2+1];
	for (uint8_t i=0;i<ERR_MSG_LEN*2+1;i++){
		arrowString1[i]=0;
		arrowString2[i]=0;
		tempString1[i]=0;
		tempString2[i]=0;
	}
	LineNumAndCharNum lineNumAndCharNum;
	if (sourceContainer.isStringForPreprocesser){
		indexStart+=sourceContainer.stringOffset;
		indexEnd+=sourceContainer.stringOffset;
		makeColor(COLOR_TO_TEXT,COLOR_CYAN);
		printf(
			"Note: This warning/error is occuring during the preprocesser when evaluating an expression.\n"\
			"    The message being given may not be exactly correct, but is somewhat relevant");
		resetColor();
		printf("\n");
	}
	bool noLocation=indexStart==-1;
	if (noLocation) goto PrintMessage;
	if (indexEnd<=indexStart) indexEnd=indexStart+1;
	lineNumAndCharNum = getLineNumAndCharNumFromSourceContainerIndex(indexStart);
	int8_t numSkipFor1 = 0;
	int8_t numSkipFor2 = 0;
	bool haltPrint1 = false;
	bool haltPrint2 = false;
	int32_t testingIndex;
	char c;
	for (int32_t i=0;i<ERR_MSG_LEN*2;i++){
		arrowString1[i]=' ';
		arrowString2[i]=' ';
		tempString1[i]=' ';
		tempString2[i]=' ';
		if (!haltPrint1){
			testingIndex = lineNumAndCharNum.charNumInSourceContainer+i-ERR_MSG_LEN;
			if (testingIndex<0){
				numSkipFor1++;
			} else {
				c=sourceContainer.sourceChar[testingIndex].c;
				if (c=='\n'){
					if (testingIndex<lineNumAndCharNum.charNumInSourceContainer){
						for (uint8_t tempI=0;tempI<ERR_MSG_LEN*2;tempI++){
							tempString1[tempI]=' '; // reset to spaces
						}
						numSkipFor1=i;
					} else {
						haltPrint1=true;
					}
				} else {
					tempString1[i-numSkipFor1]=c;
				}
			}
		}
		if (!haltPrint2){
			testingIndex = lineNumAndCharNum.charNumInOriginal+i-ERR_MSG_LEN;
			if (testingIndex<0){
				numSkipFor2++;
			} else {
				c=lineNumAndCharNum.originalContents[testingIndex];
				if (c=='\n'){
					if (testingIndex<lineNumAndCharNum.charNumInOriginal){
						for (uint8_t tempI=0;tempI<ERR_MSG_LEN*2;tempI++){
							tempString2[tempI]=' '; // reset to spaces
						}
						numSkipFor2=i;
					} else {
						haltPrint2=true;
					}
				} else {
					tempString2[i-numSkipFor2]=c;
				}
			}
		}
		
	}
	if (numSkipFor1>ERR_MSG_LEN | numSkipFor2>ERR_MSG_LEN){
		printf("Internal Error: printing source location failed\n");
		exit(1);
	}
	arrowString1[ERR_MSG_LEN-numSkipFor1]='^';
	arrowString2[ERR_MSG_LEN-numSkipFor2]='^';
	for (uint8_t i=0;i<ERR_MSG_LEN*2;i++){
		if (tempString1[i]=='\t'){
			tempString1[i]=' ';
		}
		if (tempString2[i]=='\t'){
			tempString2[i]=' ';
		}
	}
	if (indexEnd!=0){
		uint16_t distance;
		{
			int32_t distanceTemp=indexEnd-indexStart;
			if (distanceTemp>=0 & distanceTemp<ERR_MSG_LEN*2){
				distance=distanceTemp-1;
			} else {
				distance=ERR_MSG_LEN; // might be ERR_MSG_LEN*2
			}
		}
		uint16_t offset=1+(ERR_MSG_LEN-numSkipFor1);
		for (uint16_t i=0;i<distance;i++){
			uint16_t iFinal=i+offset;
			if (iFinal>=ERR_MSG_LEN*2){
				break;
			}
			arrowString1[iFinal]='~';
		}
	}
	tempString1[ERR_MSG_LEN*2]=0;
	tempString2[ERR_MSG_LEN*2]=0;
	arrowString1[ERR_MSG_LEN*2]=0;
	arrowString2[ERR_MSG_LEN*2]=0;
	for (uint8_t i=0;i<ERR_MSG_LEN*2;i++){
		if (tempString1[i]=='\t'){
			tempString1[i]=' ';
		}
		if (tempString2[i]=='\t'){
			tempString2[i]=' ';
		}
	}
	for (uint8_t i=0;i<ERR_MSG_LEN*2;i++){
		if (tempString1[i]!=' ' | tempString2[i]!=' '){
			if (i!=0){
				for (uint8_t c=i;c<ERR_MSG_LEN*2+1;c++){
					tempString1[c-i]=tempString1[c];
					tempString2[c-i]=tempString2[c];
					arrowString1[c-i]=arrowString1[c];
					arrowString2[c-i]=arrowString2[c];
				}
			}
			break;
		}
	}
	normalizeInformativePrintStringEnd(tempString1);
	normalizeInformativePrintStringEnd(tempString2);
	normalizeInformativePrintStringEnd(arrowString1);
	normalizeInformativePrintStringEnd(arrowString2);
	PrintMessage:;
	if (isError){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("\nError: ");
	} else {
		makeColor(COLOR_TO_TEXT,COLOR_YELLOW);
		printf("\nWarning: ");
	}
	makeColor(COLOR_TO_TEXT,COLOR_WHITE);
	printf("%s",message);
	if (noLocation) goto End;
	resetColor();
	printf("\n  Location in preprocessed input  :\n  %s\n  ",
		tempString1);
	makeColor(COLOR_TO_TEXT,COLOR_CYAN);
	printf("%s",arrowString1);
	resetColor();
	printf("\n  Original file name is: %s\n  Location in original file :%d:%d:\n  %s\n  ",
		lineNumAndCharNum.fileName,
		lineNumAndCharNum.lineNumInOriginal,
		lineNumAndCharNum.charNumInLineInOriginal,
		tempString2);
	makeColor(COLOR_TO_TEXT,COLOR_CYAN);
	printf("%s",arrowString2);
	End:
	resetColor();
	printf("\n");
}









bool doesStringMatchUntilParen(char* string1, char* string2){
	int32_t indexOfParen1=-1;
	int32_t indexOfParen2=-1;
	int32_t i;
	char c;
	for (i=0;(c=string1[i]);i++){
		if (c=='('){
			indexOfParen1=i;
			break;
		}
	}
	for (i=0;(c=string2[i]);i++){
		if (c=='('){
			indexOfParen2=i;
			break;
		}
	}
	if (indexOfParen1!=-1 & indexOfParen2!=-1){
		if (indexOfParen1==indexOfParen2){
			for (i=0;i<indexOfParen1;i++){
				if (string1[i]!=string2[i]) return false;
			}
			return true;
		} else {
			return false;
		}
	} else {
		printf("doesStringMatchUntilParen() called wrong\n");
		exit(1);
	}
}


struct Macro{
	char *definition; // if null, then this Macro is not a usable macro, instead it is an empty slot for a macro to be placed at
	char *result; // if null (and the above is not null), this macro is defined but has no result (will be expanded to nothing)
	bool isOverriddenToNotExpand; // used for macro suppression, which enables this program to achieve the desired output from the preprocesser
};
typedef struct Macro Macro;

struct MacroListing{
	int16_t numberOfAllocatedSlotsInArray;
	Macro *macros;
};
typedef struct MacroListing MacroListing;
MacroListing macroListing;

struct PreDefinedMacros{
	const char *dateDefinition;
	const char *dateResult;
	const char *timeDefinition;
	const char *timeResult;
	const char *fileDefinition;
	const char *lineDefinition;
	int16_t dateMacroIndex;
	int16_t timeMacroIndex;
	int16_t fileMacroIndex;
	int16_t lineMacroIndex;
};
typedef struct PreDefinedMacros PreDefinedMacros;
PreDefinedMacros preDefinedMacros;


bool doesMacroHaveReplacement(int16_t macroIndex){
	return macroListing.macros[macroIndex].result==NULL;
}

bool isMacroComplex(int16_t macroIndex){
	Macro thisMacro=macroListing.macros[macroIndex];
	if (thisMacro.definition==NULL) return false;
	for (int16_t i=0;thisMacro.definition[i];i++){
		if (thisMacro.definition[i]=='(') return true;
	}
	return false;
}

void removeMacro(int16_t macroIndex){
	Macro* this=macroListing.macros+macroIndex;
	char* definition=this->definition;
	if (doStringsMatch(definition,preDefinedMacros.dateDefinition) |
		doStringsMatch(definition,preDefinedMacros.timeDefinition) |
		doStringsMatch(definition,preDefinedMacros.fileDefinition) |
		doStringsMatch(definition,preDefinedMacros.lineDefinition)
		){
		
		printInformativeMessageAtSourceContainerIndex(false,"Removing or redefining a standard macro is not allowed",-1,0);
	} else {
		cosmic_free(definition);
		this->definition=NULL;
		if (doesMacroHaveReplacement(macroIndex)){
			cosmic_free(this->result);
			this->result=NULL;
		}
	}
}

int16_t insertMacroIntoEmptySlot(Macro macroToPlace){
	macroToPlace.isOverriddenToNotExpand=false;
	if (macroToPlace.definition==NULL){
		printf("Internal Error:null macro cannot be inserted in definition list\n");
		exit(1);
	}
	bool isInputComplex=false;
	for (int16_t i=0;macroToPlace.definition[i];i++){
		if (macroToPlace.definition[i]=='(') isInputComplex=true;
	}
	for (int16_t i=0;i<macroListing.numberOfAllocatedSlotsInArray;i++){
		char* i_def=macroListing.macros[i].definition;
		if (i_def!=NULL){
			bool hasConflict;
			if (isInputComplex) hasConflict = isMacroComplex(i) && doesStringMatchUntilParen(i_def,macroToPlace.definition);
			else hasConflict = doStringsMatch(i_def,macroToPlace.definition);
			if (hasConflict){
				char* temp=strMerge3("Warning: macro with name \"",i_def,"\" is being redefined");
				printInformativeMessageAtSourceContainerIndex(false,temp,-1,0);
				cosmic_free(temp);
				removeMacro(i);
			}
		}
	}
	if (macroToPlace.result==NULL){
		macroToPlace.result = cosmic_calloc(1,1);
	}
	for (int16_t i=0;i<macroListing.numberOfAllocatedSlotsInArray;i++){
		if (macroListing.macros[i].definition==NULL){
			macroListing.macros[i]=macroToPlace;
			return i;
		}
	}
	if (macroListing.numberOfAllocatedSlotsInArray<=30000){
		const Macro nullMacro = {0};
		macroListing.numberOfAllocatedSlotsInArray+=20;
		macroListing.macros = cosmic_realloc(macroListing.macros,macroListing.numberOfAllocatedSlotsInArray*sizeof(Macro));
		for (int16_t i=macroListing.numberOfAllocatedSlotsInArray-20;i<macroListing.numberOfAllocatedSlotsInArray;i++){
			macroListing.macros[i]=nullMacro;
		}
		return insertMacroIntoEmptySlot(macroToPlace);
	} else {
		printf("Internal Error: too many macros\n");
		exit(1);
	}
}

char* copystrWithQuotes(const char* str){
	char* new=cosmic_malloc(strlen(str)+3);
	int32_t i=0;
	int32_t w=1;
	while ((new[w++]=str[i++])){
	}
	new[  0]='\"';
	new[w-1]='\"';
	new[w  ]=0;
	return new;
}


void initMacros(){
	/*
	todo, mostly. would involve a few things like this for initializng these properly:
	#include <time.h>
	time_t t = time(0);
	struct tm *tm = localtime(&t);
	char[64] str;
	asctime(s,64,"%c",tm);
	*/
	/*
	then some string formating on str
	also, asctime() contains some important return information
	copyStringToHeapString() can copy strings
	*/
	preDefinedMacros.dateResult = copyStringToHeapString("\"Jan 10 2020\"");
	preDefinedMacros.timeResult = copyStringToHeapString("\"00:00:00\"");
	
	preDefinedMacros.dateDefinition = "__DATE__";
	preDefinedMacros.timeDefinition = "__TIME__";
	preDefinedMacros.fileDefinition = "__FILE__";
	preDefinedMacros.lineDefinition = "__LINE__";
	// __STDC__ and __STDC_VERSION__ are not done for right now
	Macro tempMacro={0};
	
	tempMacro.definition = copyStringToHeapString(preDefinedMacros.dateDefinition);
	tempMacro.result = copyStringToHeapString(preDefinedMacros.dateResult);
	preDefinedMacros.dateMacroIndex = insertMacroIntoEmptySlot(tempMacro);
	
	tempMacro.definition = copyStringToHeapString(preDefinedMacros.timeDefinition);
	tempMacro.result = copyStringToHeapString(preDefinedMacros.timeResult);
	preDefinedMacros.timeMacroIndex = insertMacroIntoEmptySlot(tempMacro);
	
	tempMacro.definition = copyStringToHeapString(preDefinedMacros.fileDefinition);
	tempMacro.result = copystrWithQuotes("FileMacroError");
	preDefinedMacros.fileMacroIndex = insertMacroIntoEmptySlot(tempMacro);
	
	tempMacro.definition = copyStringToHeapString(preDefinedMacros.lineDefinition);
	tempMacro.result = copyStringToHeapString("0");
	preDefinedMacros.lineMacroIndex = insertMacroIntoEmptySlot(tempMacro);
}



// should only be called when it is known it is valid to ask such a thing
// this function is just to get a quick guess to see if it needs to make a copy of the arguments in string form
bool doesMacroUseStringedArguments(int16_t macroIndex){
	Macro thisMacro=macroListing.macros[macroIndex];
	for (int16_t i=0;thisMacro.result[i];i++){
		if (thisMacro.result[i]=='#') return true; // not exactly, but it's a decent guess, and it helps for speed and memory.
	}
	return false;
}



// may look a little beyond in the string as well to see if the macro matches in it's complexity (if it is a called macro and if it is called)
// don't check much. Use searchForMatchingMacro() for more safety
bool doesMacroMatchStringRange(int32_t stringStartIndex, int32_t stringEndIndex, int16_t macroIndex){
	if (isMacroComplex(macroIndex)){
		for (int32_t i=0;macroListing.macros[macroIndex].definition[i];i++){
			char macroChar = macroListing.macros[macroIndex].definition[i];
			if (macroChar=='(') return stringEndIndex-stringStartIndex==i && isNextNonSpaceOpenParen(stringEndIndex);
			else if (macroChar!=sourceContainer.sourceChar[i+stringStartIndex].c) return false;
		}
		return false;
	} else {
		int32_t i;
		for (i=0;macroListing.macros[macroIndex].definition[i];i++){
			if (macroListing.macros[macroIndex].definition[i]!=sourceContainer.sourceChar[i+stringStartIndex].c) return false;
		}
		return i==stringEndIndex-stringStartIndex;
	}
}

// returns macro index if it found one, otherwise returns -1
// if stringStartIndex is unknown, a -1 may be used, in which case the stringStartIndex will be calculated
int16_t searchForMatchingMacro(int32_t preprocessTokenIndex,int32_t stringStartIndex){
	if (stringStartIndex==-1) stringStartIndex = getIndexInStringAtStartOfPreprocessingToken(preprocessTokenIndex);
	int32_t stringEndIndex = stringStartIndex+preprocessTokens.preprocessTokens[preprocessTokenIndex].length;
	// test predefined macros
	if (stringEndIndex-stringStartIndex==8){
		bool isTime=true;
		bool isDate=true;
		bool isFile=true;
		bool isLine=true;
		for (uint8_t i=0;i<8;i++){
			char c = sourceContainer.sourceChar[stringStartIndex+i].c;
			if (c!=preDefinedMacros.timeDefinition[i]) isTime=false;
			if (c!=preDefinedMacros.dateDefinition[i]) isDate=false;
			if (c!=preDefinedMacros.fileDefinition[i]) isFile=false;
			if (c!=preDefinedMacros.lineDefinition[i]) isLine=false;
		}
		if (isTime | isDate){
			printInformativeMessageAtSourceContainerIndex(false,"This standard macro is not supported... yet",stringStartIndex,stringEndIndex);
			// todo: support those macros. I think it shouldn't be too hard.
		} else if (isFile | isLine){
			LineNumAndCharNum lineNumAndCharNum = getLineNumAndCharNumFromSourceContainerIndex(stringStartIndex);
			if (isLine){
				char numberBuffer[14] = {0};
				snprintf(numberBuffer,13,"%lu",(unsigned long)lineNumAndCharNum.lineNumInOriginal);
				char** result=&(macroListing.macros[preDefinedMacros.lineMacroIndex].result);
				cosmic_free(*result);
				*result=copyStringToHeapString(numberBuffer);
				return preDefinedMacros.lineMacroIndex;
			} else {
				char** result=&(macroListing.macros[preDefinedMacros.fileMacroIndex].result);
				if (!doStringsMatch(*result,lineNumAndCharNum.fileName)){
					cosmic_free(*result);
					*result=copystrWithQuotes(lineNumAndCharNum.fileName);
				}
				return preDefinedMacros.fileMacroIndex;
			}
		}
	}
	// test all macros
	char firstCharacter = sourceContainer.sourceChar[stringStartIndex].c;
	for (int16_t i=0;i<macroListing.numberOfAllocatedSlotsInArray;i++){
		Macro* macroPtr = &(macroListing.macros[i]);
		if ((macroPtr->definition!=NULL) & (macroPtr->result!=NULL)){
			if (macroPtr->definition[0]!=firstCharacter) continue; // no need to check farther
			if (doesMacroMatchStringRange(stringStartIndex,stringEndIndex,i)){
				if (macroPtr->isOverriddenToNotExpand){
					preprocessTokens.preprocessTokens[preprocessTokenIndex].catagory=10;// if a token does not expand because the macro has been suppressed, then suppress the token
					return -1;
				} else {
					return i;
				}
			}
		}
	}
	return -1;
}

// input is for indexes in sourceContainer
bool isMacroDefined(int32_t characterIndexStart, int32_t characterIndexEnd){
	for (int16_t mi=0;mi<macroListing.numberOfAllocatedSlotsInArray;mi++){
		if (macroListing.macros[mi].definition!=NULL){
			char *definitionForMacro = macroListing.macros[mi].definition;
			int32_t lengthOfMacroDefinition=0;
			if (isMacroComplex(mi)){
				for (;definitionForMacro[lengthOfMacroDefinition]!='(';lengthOfMacroDefinition++){
				}
			} else {
				for (;definitionForMacro[lengthOfMacroDefinition];lengthOfMacroDefinition++){
				}
			}
			if (characterIndexEnd-characterIndexStart==lengthOfMacroDefinition){
				bool hadFailedCharacter = false;
				for (int32_t i=0;(definitionForMacro[i]!='(')&(definitionForMacro[i]!=0);i++){
					if (sourceContainer.sourceChar[i+characterIndexStart].c!=definitionForMacro[i]){
						hadFailedCharacter=true;
						break;
					}
				}
				if (!hadFailedCharacter) return true;
			}
		}
	}
	return false;
}

char* stringifyStringForMacroWithFreeInput(char* inputString){
	Start:;
	char* temp;
	if (inputString[0]==' '){
		temp=copyStringToHeapString(inputString+1);
		cosmic_free(inputString);
		inputString=temp;
		goto Start;
	}
	int32_t length=strlen(inputString);
	char c=inputString[length-1];
	if (c==' '){
		inputString[length-1]=0;
		goto Start;
	}
	if (c=='\\'){
		inputString=cosmic_realloc(inputString,++length);
		inputString[length-1]=0;
	}
	bool b=true;
	int32_t i0=0;
	int32_t extraCount=0;
	while ((c=inputString[i0++])){
		if (b & c=='\\'){
			extraCount++;
			char t=inputString[i0++];
			if (t=='\'' | t=='\"') extraCount++;
		} else if (c=='\'' | c=='\"') extraCount++,b^=1;
	}
	char* outputString=cosmic_malloc(length+extraCount+4);
	i0=0;
	int32_t i1=1;
	while ((c=inputString[i0++])){
		if (b & c=='\\') {
			outputString[i1++]='\\';
			outputString[i1++]='\\';
			char t=inputString[i0++];
			if (t=='\'' | t=='\"'){
				outputString[i1++]='\\';
			}
			outputString[i1++]=t;
		} else if (c=='\'' | c=='\"') {
			b^=1;
			outputString[i1++]='\\';
			outputString[i1++]=c;
		} else {
			outputString[i1++]=c;
		}
	}
	cosmic_free(inputString);
	outputString[0]='\"';
	outputString[i1++]='\"';
	outputString[i1]=0;
	return outputString;
}

int32_t macroShieldSkip(char* string, int32_t start){
	int32_t i=start+1;
	char startC = string[start];
	char c0;
	char c1;
	bool isNestable=true;
	if (startC=='\''){
		c1='\'';
		isNestable=false;
	} else if (startC=='\"'){
		c1='\"';
		isNestable=false;
	} else if (startC=='('){
		c0='(';
		c1=')';
	} else if (startC=='['){
		c0='[';
		c1=']';
	} else if (startC=='{'){
		c0='{';
		c1='}';
	} else {
		assert(false); // called wrong
	}
	if (isNestable){
		while (string[i]!=c1){
			assert(string[i]!=0);
			if (string[i]==c0){
				i = macroShieldSkip(string,i);
			}
			i++;
		}
	} else {
		while (string[i]!=c1){
			assert(string[i]!=0);
			if (string[i++]=='\\') i++;
		}
	}
	return i;
}


int32_t findIndexInPreprocessTokenWithContentsFromStringIndex(PreprocessTokenWithContents* ptwc, int32_t strIndex){
	int32_t currentLen=0;
	int32_t preprocessTokenIndex=0;
	while (currentLen!=strIndex){
		currentLen+=(ptwc++)->token.length;
		preprocessTokenIndex++;
		assert(currentLen<=strIndex);
	}
	return preprocessTokenIndex;
}


// this does the string to string argument replacement inside a complex macro
// stringedArgumentsBeforePrescan may be null if the macro doesn't require stringedArgumentsBeforePrescan (this is checked and an error is thrown if this requirement is not satisfied)
PreprocessTokenWithContents* macroComplexReplaceSubFunction(int16_t macroIndex, char* stringedArgumentsBeforePrescan, PreprocessTokenWithContents* argumentsAfterPrescan){
	static struct MacroArgument{
		char* name; // if this is null, then this is not used
		PreprocessTokenWithContents* replacementAfterPrescan;
		char* nameWithHash; // only valid when the macro has stringified argument usages
		PreprocessTokenWithContents* replacementBeforePrescan; // only valid when the macro has stringified argument usages
	} macroArgument[128]; // this function is never called twice at the same time, therefore static is fine
	int32_t i;
	for (i=0;i<128;i++) macroArgument[i].name=NULL;
	
	bool needsArgumentsBeforePrescan = stringedArgumentsBeforePrescan!=NULL;
	assert(!(!needsArgumentsBeforePrescan && doesMacroUseStringedArguments(macroIndex)));
	// assert ensure that if that macro needed stringedArgumentsBeforePrescan that it has it
	
	Macro thisMacro = macroListing.macros[macroIndex];
	PreprocessTokenWithContents* resultOfExpansion = convertStringToHeapPreprocessTokenWithContents(thisMacro.result);
	int16_t expectedArgumentCount=1;
	for (i=0;thisMacro.definition[i];i++){
		if (thisMacro.definition[i]==',') expectedArgumentCount++;
	}
	if (expectedArgumentCount>128){
		printf("A maximum of 128 macro arguments is allowed\n");
		exit(1);
	}
	int16_t count=0;
	int16_t lastIndex=0;
	for (int16_t startUpIndex=0;thisMacro.definition[startUpIndex];startUpIndex++){
		if (thisMacro.definition[startUpIndex]=='('){
			lastIndex=startUpIndex+1;
			for (i=startUpIndex+1;thisMacro.definition[i];i++){
				char c=thisMacro.definition[i];
				if (c==',' | c==')'){
					struct MacroArgument* this=macroArgument+count++;
					this->name = copyStringSegmentToHeap(thisMacro.definition,lastIndex,i);
					lastIndex=i+1;
					if (needsArgumentsBeforePrescan){
						this->nameWithHash = strMerge2("#",this->name);
					}
				}
			}
			break;
		}
	}
	assert(count==expectedArgumentCount);
	if (needsArgumentsBeforePrescan){
		count=0;
		lastIndex=0;
		char *temp;
		for (i=0;stringedArgumentsBeforePrescan[i];i++){
			char c=stringedArgumentsBeforePrescan[i];
			if (c==','){
				temp = stringifyStringForMacroWithFreeInput(copyStringSegmentToHeap(stringedArgumentsBeforePrescan,lastIndex,i));
				macroArgument[count++].replacementBeforePrescan = convertStringToHeapPreprocessTokenWithContents(temp);
				cosmic_free(temp);
				lastIndex=i+1;
			} else if (c=='{' | c=='[' | c=='(' | c=='\'' | c=='\"'){
				i=macroShieldSkip(stringedArgumentsBeforePrescan,i);
			}
		}
		temp = stringifyStringForMacroWithFreeInput(copyStringSegmentToHeap(stringedArgumentsBeforePrescan,lastIndex,i));
		macroArgument[count++].replacementBeforePrescan = convertStringToHeapPreprocessTokenWithContents(temp);
		cosmic_free(temp);
		assert(count==expectedArgumentCount);
	}
	count=0;
	lastIndex=0;
	char* stringedArgumentsAfterPrescan=convertPreprocessTokensWithContentsToString(argumentsAfterPrescan);
	for (i=0;stringedArgumentsAfterPrescan[i];i++){
		char c=stringedArgumentsAfterPrescan[i];
		if (c==','){
			int32_t i0=findIndexInPreprocessTokenWithContentsFromStringIndex(argumentsAfterPrescan,i);
			macroArgument[count++].replacementAfterPrescan = rangeCopyWithoutContentsCopyForPreprocessTokenWithContents(argumentsAfterPrescan,lastIndex,i0);
			lastIndex=i0+1;
		} else if (c=='{' | c=='[' | c=='(' | c=='\'' | c=='\"'){
			i=macroShieldSkip(stringedArgumentsAfterPrescan,i);
		}
	}
	macroArgument[count++].replacementAfterPrescan = rangeCopyWithoutContentsCopyForPreprocessTokenWithContents(argumentsAfterPrescan,lastIndex,findIndexInPreprocessTokenWithContentsFromStringIndex(argumentsAfterPrescan,i));
	assert(count==expectedArgumentCount);
	cosmic_free(stringedArgumentsAfterPrescan);
	// initialization of arguments finished. Time to replace everything.
	for (i=0;resultOfExpansion[i].token.catagory!=0;i++){
		if (resultOfExpansion[i].token.catagory==4){
			for (int16_t argumentNumber=0;argumentNumber<expectedArgumentCount;argumentNumber++){
				struct MacroArgument thisMacroArgument = macroArgument[argumentNumber];
				if (doStringsMatch(resultOfExpansion[i].contents,thisMacroArgument.name)){
					resultOfExpansion = replaceTokenWithTokensForPreprocessTokenWithContents(resultOfExpansion,thisMacroArgument.replacementAfterPrescan,i);
					i+=findLengthOfPreprocessTokensWithContents(thisMacroArgument.replacementAfterPrescan)-1;
					break;
				}
				if (needsArgumentsBeforePrescan){
					if (doStringsMatch(resultOfExpansion[i].contents,thisMacroArgument.nameWithHash)){
						resultOfExpansion = replaceTokenWithTokensForPreprocessTokenWithContents(resultOfExpansion,thisMacroArgument.replacementBeforePrescan,i);
						i+=findLengthOfPreprocessTokensWithContents(thisMacroArgument.replacementBeforePrescan)-1;
						break;
					}
				}
			}
		}
	}
	// replacing done.
	for (int16_t argumentNumber=0;argumentNumber<expectedArgumentCount;argumentNumber++){
		cosmic_free(macroArgument[argumentNumber].name);
		cosmic_free(macroArgument[argumentNumber].replacementAfterPrescan);// the contents of these should not be free-ed because of how they are copied
	}
	if (needsArgumentsBeforePrescan){
		for (int16_t argumentNumber=0;argumentNumber<expectedArgumentCount;argumentNumber++){
			cosmic_free(macroArgument[argumentNumber].nameWithHash);
			freePreprocessTokenWithContents(macroArgument[argumentNumber].replacementBeforePrescan);// these need to be free-ed
		}
	}
	return resultOfExpansion;
}



void macroScanBounded(int32_t startPreprocessToken);


// returns token index it ended on
int32_t macroComplexReplace(int32_t startPreprocessToken, int16_t macroIndex){
	int32_t openParentheseTokenIndex = startPreprocessToken+1;
	if (preprocessTokens.preprocessTokens[openParentheseTokenIndex].catagory!=5){
		assert(isNextNonSpaceOpenParen(getIndexInStringAtStartOfPreprocessingToken(openParentheseTokenIndex))); // if called when potential macro doesn't have () in definition
		openParentheseTokenIndex++;
		assert(preprocessTokens.preprocessTokens[openParentheseTokenIndex].catagory==5);// ensuring found sync point between string and preprocessing tokens
	}
	char *stringedArgumentsBeforePrescan = NULL;
	int32_t tempIndexOfMatchingParenthese = getIndexOfMatchingParenthesePreprocessingToken(openParentheseTokenIndex);
	int32_t tokenAfterOpeningParenthese = openParentheseTokenIndex+1;
	if (tokenAfterOpeningParenthese==tempIndexOfMatchingParenthese){
		printf("for a macro with arguments, you cannot have the argument parentheses be like this: \"()\", it must be like this if the argument is to be a space: \"( )\"\n");
		exit(1);
	}
	if (doesMacroUseStringedArguments(macroIndex)){
		stringedArgumentsBeforePrescan = copySegmentOfSourceContainerToHeapString(
			getIndexInStringAtStartOfPreprocessingToken(tokenAfterOpeningParenthese),
			getIndexInStringAtStartOfPreprocessingToken(tempIndexOfMatchingParenthese));
	}
	placeBoundsIntoPreprocessingTokens(startPreprocessToken,tempIndexOfMatchingParenthese+1);// this bounds is the entire macro, and is used for the rescan after the argument prescan
	++tokenAfterOpeningParenthese;++tempIndexOfMatchingParenthese;// these are needed due to the new bound enter token
	placeBoundsIntoPreprocessingTokens(tokenAfterOpeningParenthese,tempIndexOfMatchingParenthese);// this bounds is for the arguments of the macro, for the argument prescan
	macroScanBounded(tokenAfterOpeningParenthese); // argument prescan is allowed to replace this macro
	PreprocessTokenWithContents *argumentsAfterPrescan = convertBoundedAreaToHeapPreprocessTokenWithContents(tokenAfterOpeningParenthese);
	removeBoundsInPreprocessingTokens(tokenAfterOpeningParenthese);
	// at this point, we figure out what the arguments are, and prepare to use those in the definition substitution.
	PreprocessTokenWithContents *resultOfMacroOnHeap = macroComplexReplaceSubFunction(macroIndex,stringedArgumentsBeforePrescan,argumentsAfterPrescan);
	freePreprocessTokenWithContents(argumentsAfterPrescan);
	if (stringedArgumentsBeforePrescan!=NULL) cosmic_free(stringedArgumentsBeforePrescan);
	preprocesserBoundedReplace(resultOfMacroOnHeap,startPreprocessToken);
	freePreprocessTokenWithContents(resultOfMacroOnHeap);
	macroListing.macros[macroIndex].isOverriddenToNotExpand=true;
	macroScanBounded(startPreprocessToken); // do rescan without replacing this macro
	macroListing.macros[macroIndex].isOverriddenToNotExpand=false;
	int32_t endingTokenIndexWithoutBounds = getIndexOfEndingBoundFromStartBound(startPreprocessToken)-2;
	removeBoundsInPreprocessingTokens(startPreprocessToken);
	return endingTokenIndexWithoutBounds;
}

// returns token index it ended on
int32_t macroSimpleReplace(int32_t startPreprocessToken, int16_t macroIndex){
	placeBoundsIntoPreprocessingTokens(startPreprocessToken,startPreprocessToken+1);
	PreprocessTokenWithContents* tokens = convertStringToHeapPreprocessTokenWithContents(macroListing.macros[macroIndex].result);
	preprocesserBoundedReplace(tokens,startPreprocessToken);
	freePreprocessTokenWithContents(tokens);
	macroListing.macros[macroIndex].isOverriddenToNotExpand=true;
	macroScanBounded(startPreprocessToken); // do rescan without replacing this macro
	macroListing.macros[macroIndex].isOverriddenToNotExpand=false;
	int32_t endingTokenIndexWithoutBounds = getIndexOfEndingBoundFromStartBound(startPreprocessToken)-2;
	removeBoundsInPreprocessingTokens(startPreprocessToken);
	return endingTokenIndexWithoutBounds;
}


void macroScanBounded(int32_t startPreprocessToken){
	int32_t preprocessTokenIndex = startPreprocessToken+1;
	uint8_t currentCatagory = preprocessTokens.preprocessTokens[preprocessTokenIndex].catagory;
	int32_t endBound = getIndexOfEndingBoundFromStartBound(startPreprocessToken);
	while (preprocessTokenIndex<endBound){
		// if this token is the null token or bound exit token, then end loop.
		if (currentCatagory==1){
			while (currentCatagory!=3){
				currentCatagory = preprocessTokens.preprocessTokens[++preprocessTokenIndex].catagory;
			}
		} else if (currentCatagory==4){
			int16_t macroSearchResult = searchForMatchingMacro(preprocessTokenIndex,-1);
			if (macroSearchResult!=-1){
				if (isMacroComplex(macroSearchResult)){
					preprocessTokenIndex = macroComplexReplace(preprocessTokenIndex,macroSearchResult);
				} else {
					preprocessTokenIndex = macroSimpleReplace(preprocessTokenIndex,macroSearchResult);
				}
				endBound = getIndexOfEndingBoundFromStartBound(startPreprocessToken);
			}
		} else if (currentCatagory==0){
			assert(false); // unbounded exit during bounded scan
		}
		currentCatagory = preprocessTokens.preprocessTokens[++preprocessTokenIndex].catagory;
	}
	assert(preprocessTokenIndex==endBound); // understep or overstep ending bound
}

// macro replacement over an area is started be calling "macroScanUnbounded()" (and will be finished when function returns)
void macroScanUnbounded(){
	int32_t preprocessTokenIndex = 0;
	uint8_t currentCatagory = preprocessTokens.preprocessTokens[preprocessTokenIndex].catagory;
	int32_t walkingStartIndexInSourceString = preprocessTokens.startIndexInSourceString;
	while (currentCatagory!=0){
		// if this token is the null token, then end loop. 
		if (currentCatagory==1){
			while (currentCatagory!=3){
				walkingStartIndexInSourceString+=preprocessTokens.preprocessTokens[preprocessTokenIndex].length;
				currentCatagory = preprocessTokens.preprocessTokens[++preprocessTokenIndex].catagory;
			}
		} else if (currentCatagory==4){
			int16_t macroSearchResult = searchForMatchingMacro(preprocessTokenIndex,walkingStartIndexInSourceString);
			if (macroSearchResult!=-1){
				if (isMacroComplex(macroSearchResult)){
					preprocessTokenIndex = macroComplexReplace(preprocessTokenIndex,macroSearchResult);
				} else {
					preprocessTokenIndex = macroSimpleReplace(preprocessTokenIndex,macroSearchResult);
				}
				walkingStartIndexInSourceString=getIndexInStringAtStartOfPreprocessingToken(preprocessTokenIndex);
			}
		}
		walkingStartIndexInSourceString+=preprocessTokens.preprocessTokens[preprocessTokenIndex].length;
		currentCatagory = preprocessTokens.preprocessTokens[++preprocessTokenIndex].catagory;
	}
}

// these 3 functions return the character index directly after they ended ('#' if directive, '\n' if newline, and 0 if sourceChar terminated)
// Note: the above comment is slightly a lie. if sourceChar terminated, then the index of 0 is the index prior, not the index returned
int32_t replaceMacrosInGeneratedPreprocessingTokens(){
	macroScanUnbounded();
	return getIndexInStringAtStartOfPreprocessingToken(findLengthOfPreprocessingTokens())+1; 
}

int32_t replaceMacrosUntilNextDirective(int32_t startCharacterInSourceString){
	generatePreprocessTokensUntilStopPoint(startCharacterInSourceString,true);
	return replaceMacrosInGeneratedPreprocessingTokens();
}

int32_t replaceMacrosUntilNextNewline(int32_t startCharacterInSourceString){
	generatePreprocessTokensUntilStopPoint(startCharacterInSourceString,false);
	return replaceMacrosInGeneratedPreprocessingTokens();
}



/*
macro replacement functions are above. preprocesser managment functions are below.
*/



// silently returns null character's index if it hits it
int32_t findNextDirectiveInSourceContainer(int32_t startIndex){
	int32_t index=startIndex;
	char c1=sourceContainer.sourceChar[index  ].c;
	char c2=sourceContainer.sourceChar[index+1].c;
	while ((!((c1=='\n') & (c2=='#'))) & (c2!=0)){
		c1=sourceContainer.sourceChar[++index].c;
		c2=sourceContainer.sourceChar[1+index].c;
	}
	return index+1;
}

// silently returns null character's index if it hits it
int32_t findNextNewlineInSourceContainer(int32_t startIndex){
	int32_t index=startIndex;
	char c=sourceContainer.sourceChar[index].c;
	while ((c!='\n') & (c!=0)){
		c=sourceContainer.sourceChar[++index].c;
	}
	return index;
}


typedef struct DirectiveTypeAndArgumentIndex{
	uint8_t directiveType;
	int32_t indexOfSpace;
} DirectiveTypeAndArgumentIndex;


DirectiveTypeAndArgumentIndex determineDirectiveType(int32_t characterIndexOfHash){
	DirectiveTypeAndArgumentIndex directiveTypeAndArgumentIndex;
	assert(sourceContainer.sourceChar[characterIndexOfHash].c=='#'); // called wrong
	int32_t i=0;
	int32_t end=0;
	char buffer[20] = {0};
	bool stopCopy=false;
	while (!stopCopy){
		int32_t indexInSourceContainer = i+characterIndexOfHash+1;
		char c=sourceContainer.sourceChar[indexInSourceContainer].c;
		assert(c!=0);
		if (i>=20){
			goto BadDirective;
		}
		if (c==' ' | c=='\n') stopCopy=true;
		else buffer[i]=c;
		i++;
	}
	end=i;
	if (doStringsMatch(buffer,"if")){
		directiveTypeAndArgumentIndex.directiveType=1;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+3;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"endif")){
		directiveTypeAndArgumentIndex.directiveType=2;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+6;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"elif")){
		directiveTypeAndArgumentIndex.directiveType=3;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+5;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"ifdef")){
		directiveTypeAndArgumentIndex.directiveType=4;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+6;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"ifndef")){
		directiveTypeAndArgumentIndex.directiveType=5;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+7;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"include")){
		directiveTypeAndArgumentIndex.directiveType=6;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+8;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"define")){
		directiveTypeAndArgumentIndex.directiveType=7;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+7;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"pragma")){
		directiveTypeAndArgumentIndex.directiveType=8;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+7;
		return directiveTypeAndArgumentIndex; // by the way, this directive is ignored for now
	}
	if (doStringsMatch(buffer,"undef")){
		directiveTypeAndArgumentIndex.directiveType=9;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+6;
		return directiveTypeAndArgumentIndex;
	}
	if (doStringsMatch(buffer,"else")){
		directiveTypeAndArgumentIndex.directiveType=10;
		directiveTypeAndArgumentIndex.indexOfSpace=characterIndexOfHash+5;
		return directiveTypeAndArgumentIndex;
	}
	BadDirective:
	printInformativeMessageAtSourceContainerIndex(true,"Invalid preprocessing directive name",characterIndexOfHash,end);
	exit(1);
}


int32_t macroArgSkip(int32_t i){
	assert(sourceContainer.sourceChar[i].c=='('); // called wrong
	char c;
	while ((c=sourceContainer.sourceChar[++i].c)!=')'){
		assert(c!=0 & c!='\n');
		if (c=='('){
			i = macroArgSkip(i);
		}
	}
	return i;
}

Macro formatDefineInSourceContainer(int32_t indexOfSpaceAfterDirective, int32_t indexOfNewlineThatEndsDirective){
	bool isComplex = false;
	int32_t indexOfSeperationSpace=-1;
	for (int32_t i=indexOfSpaceAfterDirective+1;i<indexOfNewlineThatEndsDirective;i++){
		char c=sourceContainer.sourceChar[i].c;
		if (c==' '){
			indexOfSeperationSpace=i;
			break;
		}
		if (c=='('){
			isComplex=true;
			indexOfSeperationSpace=macroArgSkip(i)+1;
			if (indexOfSeperationSpace>indexOfNewlineThatEndsDirective){
				printInformativeMessageAtSourceContainerIndex(
					true,"in this \"#define\" directive, this parenthese does not end inside the macro",i,0);
				exit(1);
			}
			break;
		}
	}
	bool hasResult = indexOfSeperationSpace!=-1;
	if (!hasResult) indexOfSeperationSpace=indexOfNewlineThatEndsDirective;
	int32_t startOfDefinition = indexOfSpaceAfterDirective+1;
	int32_t indexAfterDefinition = indexOfSeperationSpace;
	int32_t lengthOfDefinition = indexAfterDefinition-startOfDefinition; // this may technically be decreased due to spaces that will be removed after the copy
	int32_t startOfResult = indexOfSeperationSpace+1;
	int32_t indexAfterResult = indexOfNewlineThatEndsDirective;
	int32_t lengthOfResult = indexAfterResult-startOfResult;
	Macro macro={0};
	macro.definition = cosmic_calloc(lengthOfDefinition+1,1);
	if (hasResult){
		macro.result = cosmic_calloc(lengthOfResult+1,1);
	}
	for (int32_t i=0;i<lengthOfDefinition;i++){
		macro.definition[i]=sourceContainer.sourceChar[i+startOfDefinition].c;
	}
	if (hasResult){
		for (int32_t i=0;i<lengthOfResult;i++){
			macro.result[i]=sourceContainer.sourceChar[i+startOfResult].c;
		}
	}
	if (isComplex){
		// now remove spaces from macro.definition if it is complex (if it isn't then this would be useless)
		int32_t walkingIndex=0;
		for (int32_t i=0;i<lengthOfDefinition+1;i++){ // length +1 is to get the null terminator as well
			if (macro.definition[i]!=' '){
				macro.definition[walkingIndex++]=macro.definition[i];
			}
		}
	}
	// todo: maybe some checks to ensure that the definition is valid? there isn't much checking here
	return macro;
}


bool isDefinedKeyword(int32_t index){
	for (int32_t i=index;i<index+8;i++){
		if (sourceContainer.sourceChar[i].c!=" defined"[i-index]) return false;
	}
	char c=sourceContainer.sourceChar[index+8].c;
	return c==' '|c=='(';
}


// performs the processing for things like "defined(MACRO)"
void applyIfStatementDefinedUntilNewline(int32_t indexOfSpaceAfterDirective){
	bool hadDefined=false;
	for (int32_t i0=indexOfSpaceAfterDirective;sourceContainer.sourceChar[i0].c!='\n';i0++){
		if (isDefinedKeyword(i0)){
			hadDefined=true;
			int32_t i1=i0+8;
			i0++;
			char c;
			while (c=sourceContainer.sourceChar[++i1].c,c==' '|c=='('){
			}
			int32_t i2=i1;
			while (isCharacterPartOfPreprocessTokenText(sourceContainer.sourceChar[++i2].c)){
			}
			if (isMacroDefined(i1,i2)) c='1';
			else c='0';
			for (int32_t i3=i0;i3<i2;i3++){
				char* cp=&sourceContainer.sourceChar[i3].c;
				if (*cp!='(') *cp=26;
			}
			sourceContainer.sourceChar[i1].c=c;
		}
	}
	if (hadDefined) pullDownSourceChar(sourceContainer.sourceChar+indexOfSpaceAfterDirective);
}




// returns if the `if like` directive evaluated to true
bool processIfLikeDirectiveToBool(int32_t indexOfSpaceAfterDirective){
	applyIfStatementDefinedUntilNewline(indexOfSpaceAfterDirective);
	replaceMacrosUntilNextNewline(indexOfSpaceAfterDirective);
	superStripSourceChar(sourceContainer.sourceChar,indexOfSpaceAfterDirective-1);
	int32_t endIndex=indexOfSpaceAfterDirective;
	while (sourceContainer.sourceChar[endIndex++].c!='\n'){
	}
	sourceContainer.string=copySegmentOfSourceContainerToHeapString(indexOfSpaceAfterDirective,endIndex);
	sourceContainer.stringOffset=indexOfSpaceAfterDirective;
	sourceContainer.isStringForPreprocesser=true;
	int16_t expRoot=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,1,endIndex-indexOfSpaceAfterDirective,true);
	bool result=false;
	if (expRoot==-1){
		sourceContainer.isStringForPreprocesser=false;
		printInformativeMessageAtSourceContainerIndex(false,"No constant expression was found here for this directive\n    Preprocessing will continue as if it was false",indexOfSpaceAfterDirective,0);
	} else {
		result=expressionToConstantValue("unsigned long",expRoot)!=0;
	}
	sourceContainer.isStringForPreprocesser=false;
	cosmic_free(sourceContainer.string);
	sourceContainer.string=NULL;
	sourceContainer.stringOffset=0;
	return result;
}



int32_t getIndexOfNextDirectiveThatEndsIfStatement(int32_t indexOfNewlineToStartAt){
	int32_t index = indexOfNewlineToStartAt-1;
	DirectiveTypeAndArgumentIndex directiveTypeAndArgumentIndex;
	int16_t nestLevel=1;
	do {
		index=findNextDirectiveInSourceContainer(index+1);
		if (sourceContainer.sourceChar[index].c==0){
			int32_t tempIndex=indexOfNewlineToStartAt-1;
			while (sourceContainer.sourceChar[tempIndex].c!='\n'){
				--tempIndex;
				assert(tempIndex>=0); // when error index was attempting to be found, failed to find previous newline
			}
			printInformativeMessageAtSourceContainerIndex(
				true,"could not find matching if-like directive for this directive when trying to skip it",++tempIndex,0);
			exit(1);
		}
		directiveTypeAndArgumentIndex = determineDirectiveType(index);
		if (directiveTypeAndArgumentIndex.directiveType==1 |
			directiveTypeAndArgumentIndex.directiveType==4 |
			directiveTypeAndArgumentIndex.directiveType==5){
			
			++nestLevel;
		} else if (directiveTypeAndArgumentIndex.directiveType==2){
			--nestLevel;
		} else if (directiveTypeAndArgumentIndex.directiveType==3 |
				   directiveTypeAndArgumentIndex.directiveType==10){
			
			if (nestLevel==1) nestLevel=0;
		}
	} while (nestLevel!=0);
	return index;
}


// things with if directives are todo. also, the things with chaining if directives are very untested.
// actually, basically everything with directives is untested. so yeah. expect some of that to fail right now.

// beginningFileName should have quotes around it
void insertBeginningFileAndRunPreprocesser(const char *beginningFileName){
	checkArchitecture();
	initMacros();
	insertFileIntoSourceContainer(beginningFileName,0);
	int32_t walkingCharacterIndex=0;
	int32_t indexOfNewlineThatEndsDirective;
	int32_t tempIndex;
	int16_t ifLevelsDeep=0;
	DirectiveTypeAndArgumentIndex directiveTypeAndArgumentIndex;
	DirectiveTypeAndArgumentIndex temporaryDirectiveType;
	while (1){
		int32_t charIndexReturn = replaceMacrosUntilNextDirective(walkingCharacterIndex);
		if (sourceContainer.sourceChar[charIndexReturn-1].c==0) break; // this indicates the end of input was reached
		char charAtIndexReturn = sourceContainer.sourceChar[charIndexReturn].c;
		if (charAtIndexReturn=='#'){
			// handle directive
			directiveTypeAndArgumentIndex = determineDirectiveType(charIndexReturn);
			if (directiveTypeAndArgumentIndex.directiveType==1){ // if
				++ifLevelsDeep;
				if (processIfLikeDirectiveToBool(directiveTypeAndArgumentIndex.indexOfSpace)){
					indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				} else {
					indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn)-1;
					bool continueSearch=true;
					while (continueSearch){
						indexOfNewlineThatEndsDirective = getIndexOfNextDirectiveThatEndsIfStatement(
									findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1))-1;
						
						temporaryDirectiveType = determineDirectiveType(indexOfNewlineThatEndsDirective+1);
						if (temporaryDirectiveType.directiveType==2){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
							--ifLevelsDeep;
						} else if (temporaryDirectiveType.directiveType==3){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							if (processIfLikeDirectiveToBool(temporaryDirectiveType.indexOfSpace)){
								continueSearch=false;
							}
						} else if (temporaryDirectiveType.directiveType==10){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
						} else {
							assert(false);
						}
					}
				}
			} else if (directiveTypeAndArgumentIndex.directiveType==2){ // endif
				if (ifLevelsDeep==0){
					printInformativeMessageAtSourceContainerIndex(
						true,"encountered directive \"#endif\" when there is currently no open if directives",charIndexReturn,0);
					exit(1);
				}
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				--ifLevelsDeep;
			} else if (directiveTypeAndArgumentIndex.directiveType==3){ // elif
				if (ifLevelsDeep==0){
					printInformativeMessageAtSourceContainerIndex(
						true,"encountered directive \"#elif\" when there is currently no open if directives",charIndexReturn,0);
					exit(1);
				}
				// if this is encountered in this way, we need to scan for next matching #endif directive and null to there
				// #elif will be treated similiar to #else in this context
				tempIndex = charIndexReturn;
				bool continueSearch=true;
				while (continueSearch){
					tempIndex = getIndexOfNextDirectiveThatEndsIfStatement(findNextNewlineInSourceContainer(tempIndex));
					temporaryDirectiveType = determineDirectiveType(tempIndex);
					if (temporaryDirectiveType.directiveType==2){
						indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(tempIndex);
						continueSearch=false;
						--ifLevelsDeep;
					}
				}
			} else if (directiveTypeAndArgumentIndex.directiveType==4){ // ifdef
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				++ifLevelsDeep;
				if (!isMacroDefined(directiveTypeAndArgumentIndex.indexOfSpace+1,indexOfNewlineThatEndsDirective)){
					indexOfNewlineThatEndsDirective = indexOfNewlineThatEndsDirective-1;
					bool continueSearch=true;
					while (continueSearch){
						indexOfNewlineThatEndsDirective = getIndexOfNextDirectiveThatEndsIfStatement(
									findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1))-1;
						
						temporaryDirectiveType = determineDirectiveType(indexOfNewlineThatEndsDirective+1);
						if (temporaryDirectiveType.directiveType==2){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
							--ifLevelsDeep;
						} else if (temporaryDirectiveType.directiveType==3){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							if (processIfLikeDirectiveToBool(temporaryDirectiveType.indexOfSpace)){
								continueSearch=false;
							}
						} else if (temporaryDirectiveType.directiveType==10){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
						} else {
							assert(false);
						}
					}
				}
			} else if (directiveTypeAndArgumentIndex.directiveType==5){ // ifndef
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				++ifLevelsDeep;
				if (isMacroDefined(directiveTypeAndArgumentIndex.indexOfSpace+1,indexOfNewlineThatEndsDirective)){
					indexOfNewlineThatEndsDirective = indexOfNewlineThatEndsDirective-1;
					bool continueSearch=true;
					while (continueSearch){
						indexOfNewlineThatEndsDirective = getIndexOfNextDirectiveThatEndsIfStatement(
									findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1))-1;
						
						temporaryDirectiveType = determineDirectiveType(indexOfNewlineThatEndsDirective+1);
						if (temporaryDirectiveType.directiveType==2){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
							--ifLevelsDeep;
						} else if (temporaryDirectiveType.directiveType==3){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							if (processIfLikeDirectiveToBool(temporaryDirectiveType.indexOfSpace)){
								continueSearch=false;
							}
						} else if (temporaryDirectiveType.directiveType==10){
							indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(indexOfNewlineThatEndsDirective+1);
							continueSearch=false;
						} else {
							assert(false);
						}
					}
				}
			} else if (directiveTypeAndArgumentIndex.directiveType==6){ // include
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				char* tempFileName = copySegmentOfSourceContainerToHeapString(directiveTypeAndArgumentIndex.indexOfSpace+1,indexOfNewlineThatEndsDirective);
				insertFileIntoSourceContainer(tempFileName,indexOfNewlineThatEndsDirective+1);
				cosmic_free(tempFileName);
			} else if (directiveTypeAndArgumentIndex.directiveType==7){ // define
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				
				Macro tempMacro = formatDefineInSourceContainer(directiveTypeAndArgumentIndex.indexOfSpace,indexOfNewlineThatEndsDirective);
				
				insertMacroIntoEmptySlot(tempMacro);
			} else if (directiveTypeAndArgumentIndex.directiveType==8){ // pragma
				printInformativeMessageAtSourceContainerIndex(
					false,"\"#pragma\" directive ignored",charIndexReturn,0);
				exit(1);
				// this just should clear this area taken up by the directive, because it is ignored for now
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
			} else if (directiveTypeAndArgumentIndex.directiveType==9){ // undef
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				// should not exit if the macro is already not defined. probably not even a warning
				generatePreprocessTokensUntilStopPoint(directiveTypeAndArgumentIndex.indexOfSpace+1,false);
				if (preprocessTokens.preprocessTokens[0].catagory!=4){
					printInformativeMessageAtSourceContainerIndex(
						true,"That doesn\'t seem like a macro",directiveTypeAndArgumentIndex.indexOfSpace+1,0);
					exit(1);
				}
				int16_t macroIndex = searchForMatchingMacro(0,-1);
				if (macroIndex!=-1) removeMacro(macroIndex);
			} else if (directiveTypeAndArgumentIndex.directiveType==10){ // else
				indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(charIndexReturn);
				// will clear the area to the matching #endif
				// the reason why this just clears this area is because 
				//   this part of the code is only run when the area after the #else to the matching #endif should be skipped over
				tempIndex = charIndexReturn;
				bool continueSearch=true;
				while (continueSearch){
					tempIndex = getIndexOfNextDirectiveThatEndsIfStatement(findNextNewlineInSourceContainer(tempIndex));
					temporaryDirectiveType = determineDirectiveType(tempIndex);
					if (temporaryDirectiveType.directiveType==2){
						indexOfNewlineThatEndsDirective = findNextNewlineInSourceContainer(tempIndex);
						continueSearch=false;
						--ifLevelsDeep;
					} else if (temporaryDirectiveType.directiveType==3){
						printInformativeMessageAtSourceContainerIndex(
							true,"encountered directive \"#elif\" when scan scanning on same nesting level for the end of an \"#else\" directive. That is not allowed.",charIndexReturn,0);
						exit(1);
					} else if (temporaryDirectiveType.directiveType==10){
						printInformativeMessageAtSourceContainerIndex(
							true,"encountered directive \"#else\" when scan scanning on same nesting level for the end of an \"#else\" directive. That is not allowed.",charIndexReturn,0);
						exit(1);
					}
				}
			} else {
				assert(false);
			}
			// this essentially clears the area taken up by the directive (and any area that should be cleared with it)
			for (int32_t i=charIndexReturn;i<indexOfNewlineThatEndsDirective;i++){
				sourceContainer.sourceChar[i].c=' ';
			}
			walkingCharacterIndex = indexOfNewlineThatEndsDirective; // the functions that use this should start on the previous newline to ensure everything is caught correctly
		} else {
			assert(false); // preprocesser macro replacement ended in wrong spot
		}
	}
	// these are at end of everything, and are needed for the compiler
	superStripSourceChar(sourceContainer.sourceChar,0);
	adjacentStringConcatInSourceChar(sourceContainer.sourceChar);
	if (preprocessTokens.allocationLength!=0){ // probably always true, I'm not sure at the moment
		preprocessTokens.allocationLength=0;
		cosmic_free(preprocessTokens.preprocessTokens); // it is not needed anymore
		preprocessTokens.preprocessTokens=NULL;
	}
	// destroy all macros now, they are not needed any more
	for (int16_t i=0;i<macroListing.numberOfAllocatedSlotsInArray;i++){
		char* t0=macroListing.macros[i].definition;
		char* t1=macroListing.macros[i].result;
		if (t0!=NULL) cosmic_free(t0);
		if (t1!=NULL) cosmic_free(t1);
	}
	cosmic_free(macroListing.macros);
	
	createSourceContainerString(); // so the compiler can just manage a string
}








#ifdef COMPILE_PREPROCESSER_TEST



int main(){
	printf("Starting...\n\n");
	initMacros();
	


	insertFileIntoSourceContainer("\"in.txt\"",0);
	//char* inputString = "b(h)";
	//tempStringWithProcessingToAppendingToSourceContainer(inputString);
	
	Macro testMacro = {0};
	
	/*
	testMacro.definition = "a";
	testMacro.result = "[g]";
	insertMacroIntoEmptySlot(testMacro);
	testMacro.definition = "g";
	testMacro.result = "{a}";
	insertMacroIntoEmptySlot(testMacro);
	*/
	
	testMacro.definition = "c(s)";
	testMacro.result = "|a(s)|";
	insertMacroIntoEmptySlot(testMacro);
	testMacro.definition = "b(s)";
	testMacro.result = "{c(s)}";
	insertMacroIntoEmptySlot(testMacro);
	testMacro.definition = "a(s)";
	testMacro.result = "[b(s)]";
	insertMacroIntoEmptySlot(testMacro);
	
	/*
	testMacro.definition="b(s)";
	testMacro.result="{a(s)}";
	insertMacroIntoEmptySlot(testMacro);
	testMacro.definition="a(s)";
	testMacro.result="[b(#s)]";
	insertMacroIntoEmptySlot(testMacro);
	*/
	
	replaceMacrosUntilNextDirective(0);
	
	

	//printf("\nInput:\n%s\n\nOutput:\n",inputString);
	int32_t i=0;
	while (sourceContainer.sourceChar[i].c){
		printf("%c",sourceContainer.sourceChar[i++].c);
	}
	printf("\n");
	i=1;
	while (sourceContainer.sourceChar[i].c){
		printInformativeMessageAtSourceContainerIndex(false, "test message", i,0);
		i++;
	}
	return 0;
}

#endif

