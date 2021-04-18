
#ifndef HAS_COMMON_BEEN_INCLUDED
#define HAS_COMMON_BEEN_INCLUDED


// Yes Microsoft, I want to use fopen() without warnings. It's in the C standard, so deal with it.
#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif


//#define NDEBUG

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




// this is for valgrind testing. if defined it will null out the entire exp buffer when clearing previous expressions
//#define FORCE_EXP_BUFFER_NULL


#define ERR_MSG_LEN 18 // controls the length of error messages (60 is generally good)


//#define STATEMENT_DEBUG
//#define EXP_TO_ASSEMBLY_DEBUG
//#define COMPILE_EXP_DEBUG_PRINTOUT

//#define COMPILE_ONLY_EXP_DEBUG // this includes a main


//#define OPT_DEBUG_SANITY
//#define OPT_DEBUG_GENERAL_ACTIVE
//#define OPT_DEBUG_PUSH_POP_MOV
//#define OPT_DEBUG_CONST
//#define OPT_PEEPHOLE_PRINT_TREE

//#define PRINT_EACH_FUNCTION // this prints the optimized instruction buffer for each function

/*
STATEMENT_DEBUG tells the statement walker to print every statement it reaches prior to processing it
EXP_TO_ASSEMBLY_DEBUG tells ExpressionToInstructions to print some information about what it is currently processing
COMPILE_EXP_DEBUG_PRINTOUT tells ExpressionParser to explain how the expression tree got built
*/




#define USE_ALT_ALLOC

void* cosmic_malloc(size_t size);
void* cosmic_calloc(size_t nmemb,size_t size);
void  cosmic_free(void* ptr);
void* cosmic_realloc(void* ptr,size_t size);

void printInformativeMessageAtSourceContainerIndex(bool,const char*,int32_t,int32_t);
int16_t buildExpressionTreeToGlobalBufferAndReturnRootIndex(int32_t,int32_t,bool);
uint32_t expressionToConstantValue(const char*,int16_t);

/*
Explanation of these error functions:
Because of the large amount of places that call 
  printInformativeMessageAtSourceContainerIndex()
  and may pass constant arguments, then often call exit(),
  these functions are made to reduce both code size and binary size
  (unfortunately, it is also likely that when using optimizations,
  modern compilers will likely inline these functions)

err_ABCDE()

A: is error (otherwise warning)
B: has start bound
C: has end bound (is _ when B==0)
D: do exit after giving error (is _ when A==0)
E: free the input string (is _ when D==1)

therefore:

err_00__0 [warning,no start,------,no exit,no free]
err_00__1 [warning,no start,------,no exit,   free]
err_10_00 [  error,no start,------,no exit,no free]
err_10_01 [  error,no start,------,no exit,   free]
err_10_1_ [  error,no start,------,   exit,-------]
err_010_0 [warning,   start,no end,no exit,no free]
err_010_1 [warning,   start,no end,no exit,   free]
err_011_0 [warning,   start,   end,no exit,no free]
err_011_1 [warning,   start,   end,no exit,   free]
err_11000 [  error,   start,no end,no exit,no free]
err_11001 [  error,   start,no end,no exit,   free]
err_1101_ [  error,   start,no end,   exit,-------]
err_11100 [  error,   start,   end,no exit,no free]
err_11101 [  error,   start,   end,no exit,   free]
err_1111_ [  error,   start,   end,   exit,-------]

*/

void err_00__0(const char* message){printInformativeMessageAtSourceContainerIndex(false,message,-1,0);}
void err_00__1(      char* message){err_00__0(message);cosmic_free(message);}
void err_10_00(const char* message){printInformativeMessageAtSourceContainerIndex(true,message,-1,0);}
void err_10_01(      char* message){err_10_00(message);cosmic_free(message);}
void err_10_1_(const char* message){err_10_00(message);exit(1);}
void err_010_0(const char* message,int32_t s){printInformativeMessageAtSourceContainerIndex(false,message,s,0);}
void err_010_1(      char* message,int32_t s){err_010_0(message,s);cosmic_free(message);}
void err_011_0(const char* message,int32_t s,int32_t e){printInformativeMessageAtSourceContainerIndex(false,message,s,e);}
void err_011_1(      char* message,int32_t s,int32_t e){printInformativeMessageAtSourceContainerIndex(false,message,s,e);cosmic_free(message);}
void err_11000(const char* message,int32_t s){printInformativeMessageAtSourceContainerIndex(true,message,s,0);}
void err_11001(      char* message,int32_t s){err_11000(message,s);cosmic_free(message);}
void err_1101_(const char* message,int32_t s){err_11000(message,s);exit(1);}
void err_11100(const char* message,int32_t s,int32_t e){printInformativeMessageAtSourceContainerIndex(true,message,s,e);}
void err_11101(      char* message,int32_t s,int32_t e){printInformativeMessageAtSourceContainerIndex(true,message,s,e);cosmic_free(message);}
void err_1111_(const char* message,int32_t s,int32_t e){printInformativeMessageAtSourceContainerIndex(true,message,s,e);exit(1);}



struct {
	uint8_t optLevel; // range is 0->4 inclusive. The value is one higher then the argument supplied
	bool noColor; // used to disable color printing if desired
	
	bool hasGivenConstDivByZeroWarning; 
	// if the optimizer detects a constant division by zero, it sets this and prints a warning if the flag wasn't set
	
	bool hasGivenOutOfBoundsStackAccessWarning; 
	// if the optimizer detects an out of bounds stack access, it sets this and prints a warning if the flag wasn't set
	
	bool hasGivenMisalignedWordOnStackWarning; 
	// if the optimizer detects a misaligned memory access to a word, it sets this and prints a warning if the flag wasn't set
	
} compileSettings = {
	.optLevel=1
};




#define COLOR_GRAY_OR_BLACK '0' // gray when on text, black when on background
#define COLOR_RED           '1'
#define COLOR_GREEN         '2'
#define COLOR_YELLOW        '3'
#define COLOR_BLUE          '4'
#define COLOR_MAGENTA       '5'
#define COLOR_CYAN          '6'
#define COLOR_WHITE         '7'

#define COLOR_TO_TEXT        '9'
#define COLOR_TO_BACKGROUND  '4'


void makeColor(char type, char color){
	if (compileSettings.noColor) return;
    printf("%c%c%c%c%c",        // printf() doesn't always work as intended when these are printed with %s format
        27,'[',type,color,'m');
}

void resetColor(){
	if (compileSettings.noColor) return;
	printf("\033[0m");
}


#include "TargetInstructions/IncludeInstructions.c"
#include "Alloc.c"



static void checkArchitecture(){
	uint32_t v0 = 0xFFEE5522;
	uint8_t* a0 = (uint8_t*)(&v0);
	uint8_t r0 = *(a0+0);
	uint8_t r1 = *(a0+1);
	uint8_t r2 = *(a0+2);
	uint8_t r3 = *(a0+3);
	uint16_t* a1 = (uint16_t*)(&v0);
	uint16_t r4 = *(a1+0);
	uint16_t r5 = *(a1+1);
	if (r0!=0x22 | r1!=0x55 | r2!=0xEE | r3!=0xFF | r4!=0x5522u | r5!=0xFFEE){
		printf("Fatal Error: Computer Architecture not compatible. This compiler requires running on a computer with a byte-accessed little-endian architecture.\n");
		exit(2);
	}
}


typedef struct SourceChar{
	char c;
	uint8_t indexOfOriginalFile;
	int32_t indexInOriginalSource;
} SourceChar;


typedef struct SourceContainer{
	const char* string; // this is used for the compiler, and is allocated after the preprocesser is finshed
	SourceChar *sourceChar;
	int32_t allocationLength;
	int32_t stringOffset; // if isStringForPreprocesser is true, this holds the offset for .string
	bool isStringForPreprocesser; // if this is true, then currently the preprocesser is using .string to evaluate an `if like` directive
} SourceContainer;
SourceContainer sourceContainer;

const char* directoryOfExecutable;


bool doStringsMatch(const char*const string1,const char*const string2){
	int32_t i=0;
	char c1;
	char c2;
	while ((c1=string1[i])!=0 & (c2=string2[i])!=0){
		i++;
		if (c1!=c2) return false;
	}
	return c1==c2;
}


char* copyStringToHeapString(const char*const string){
	int32_t length = strlen(string);
	char* newString = cosmic_malloc(length+1);
	for (int32_t i=0;i<length;i++){
		newString[i]=string[i];
	}
	newString[length]=0;
	return newString;
}

char* copyStringSegmentToHeap(const char*const string,const int32_t start,const int32_t end){
	assert(end>=start);
	int32_t length = end-start;
	char* newString = cosmic_malloc(length+1);
	for (int32_t i=0;i<length;i++){
		newString[i]=string[i+start];
	}
	newString[length]=0;
	return newString;
}

bool isStringLengthLargerThan(const char*const string,const int32_t startIndex,const int32_t amount){
	int32_t amountLeft = amount;
	for (int32_t i=startIndex;string[i];i++){
		if (amountLeft==0){
			return true;
		}
		amountLeft--;
	}
	return amountLeft==0;
}

bool specificStringEqualCheck(const char*const stringLarge,const int32_t startInStringLarge,const int32_t endInStringLarge,const char*const subStringToCheck){
	assert(endInStringLarge>=startInStringLarge);
	int32_t lengthOfSubString = strlen(subStringToCheck);
	if (endInStringLarge-startInStringLarge!=lengthOfSubString || 
			!isStringLengthLargerThan(stringLarge,startInStringLarge,lengthOfSubString-1)){
		
		return false;
	}
	for (int32_t i=0;i<lengthOfSubString;i++){
		if (stringLarge[i+startInStringLarge]!=subStringToCheck[i]){
			return false;
		}
	}
	return true;
}


// lower letters are c>96 & c<123
// upper letters are c>64 & c<91
#define isLetter(c) ((c>96 & c<123) | (c>64 & c<91) | c=='_')
#define isDigit(c) (c>47 & c<58)


bool isSectionOfStringEquivalent(const char*const string1,const int32_t startIndexForString1,const char*const string2){
	for (int32_t i=0;string2[i];i++){
		char c = string1[i+startIndexForString1];
		if (c==0 | c!=string2[i]){
			return false;
		}
	}
	return true;
}

bool isSegmentAnywhereInString(const char*const stringSearchIn,const char*const stringSearchFor){
	int32_t lengthIn = strlen(stringSearchIn);
	int32_t lengthFor = strlen(stringSearchFor);
	if (lengthFor>lengthIn){
		return false;
	}
	int32_t lengthToSearchTo = lengthIn-lengthFor;
	for (int32_t i=0;i<lengthToSearchTo;i++){
		if (isSectionOfStringEquivalent(stringSearchIn,i,stringSearchFor)){
			return true;
		}
	}
	return false;
}

// returns -1 if that space number doesn't exist
// the first space is n==0
int32_t getIndexOfNthSpace(const char*const string, uint16_t n){
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

int32_t getIndexOfFirstSpaceInString(const char*const string){
	return getIndexOfNthSpace(string,0);
}

char* strMerge2(const char*const s0,const char*const s1){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	char*const sf = cosmic_malloc(l0+l1+1);
	char* wa = sf;
	memcpy(wa,s0,l0);
	wa+=l0;
	memcpy(wa,s1,l1);
	wa+=l1;
	*wa=0;
	return sf;
}

char* strMerge3(const char*const s0,const char*const s1,const char*const s2){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	const uint32_t l2 = strlen(s2);
	char*const sf = cosmic_malloc(l0+l1+l2+1);
	char* wa = sf;
	memcpy(wa,s0,l0);
	wa+=l0;
	memcpy(wa,s1,l1);
	wa+=l1;
	memcpy(wa,s2,l2);
	wa+=l2;
	*wa=0;
	return sf;
}

char* strMerge4(const char*const s0,const char*const s1,const char*const s2,const char*const s3){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	const uint32_t l2 = strlen(s2);
	const uint32_t l3 = strlen(s3);
	char*const sf = cosmic_malloc(l0+l1+l2+l3+1);
	char* wa = sf;
	memcpy(wa,s0,l0);
	wa+=l0;
	memcpy(wa,s1,l1);
	wa+=l1;
	memcpy(wa,s2,l2);
	wa+=l2;
	memcpy(wa,s3,l3);
	wa+=l3;
	*wa=0;
	return sf;
}

char* strMerge5(const char*const s0,const char*const s1,const char*const s2,const char*const s3,const char*const s4){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	const uint32_t l2 = strlen(s2);
	const uint32_t l3 = strlen(s3);
	const uint32_t l4 = strlen(s4);
	char*const sf = cosmic_malloc(l0+l1+l2+l3+l4+1);
	char* wa = sf;
	memcpy(wa,s0,l0);
	wa+=l0;
	memcpy(wa,s1,l1);
	wa+=l1;
	memcpy(wa,s2,l2);
	wa+=l2;
	memcpy(wa,s3,l3);
	wa+=l3;
	memcpy(wa,s4,l4);
	wa+=l4;
	*wa=0;
	return sf;
}


// this next part is typedef entries listing and managment functions, which must be avalible to many parts of the compiler

struct TypedefEntry{
	char *typeSpecifier;
	int32_t lengthOfIdentifier;
	bool isThisSlotTaken; // if true, this is a valid entry. if false, this is an empty slot
};

struct TypedefEntries{
	struct TypedefEntry *entries;
	uint32_t numberOfAllocatedSlots;
} globalTypedefEntries;

void ensureNoIdentifierConflictForTypedef(const char*,int32_t);


void expandGlobalTypedefEntries(){
	uint32_t previousNumberOfSlots = globalTypedefEntries.numberOfAllocatedSlots;
	globalTypedefEntries.numberOfAllocatedSlots += 30;
	globalTypedefEntries.entries = cosmic_realloc(
		globalTypedefEntries.entries,
		sizeof(struct TypedefEntry)*globalTypedefEntries.numberOfAllocatedSlots);
	for (uint32_t i=previousNumberOfSlots;i<globalTypedefEntries.numberOfAllocatedSlots;i++){
		globalTypedefEntries.entries[i].isThisSlotTaken = false;
	}
}

// typeSpecifierToInsert should have an identifier at the start (and have typedefs inside itself already substituted in)
// typeSpecifierToInsert is copied inside this function, so it manages it's own strings
int32_t addTypedefEntry(const char* typeSpecifierToInsert, int32_t indexForError){
	struct TypedefEntry typedefEntry;
	typedefEntry.isThisSlotTaken = true;
	typedefEntry.lengthOfIdentifier = getIndexOfFirstSpaceInString(typeSpecifierToInsert);
	if (typedefEntry.lengthOfIdentifier==-1){
		printf("Typedef entry\'s typeSpecifierToInsert must have a space\n");
		exit(1);
	}
	ensureNoIdentifierConflictForTypedef(typeSpecifierToInsert,indexForError);
	typedefEntry.typeSpecifier = copyStringToHeapString(typeSpecifierToInsert);
	do {
		for (int32_t i=0;i<globalTypedefEntries.numberOfAllocatedSlots;i++){
			if (!globalTypedefEntries.entries[i].isThisSlotTaken){
				globalTypedefEntries.entries[i] = typedefEntry;
				return i;
			}
		}
		expandGlobalTypedefEntries();
	} while (true);
}

void removeTypedefEntry(const int32_t indexOfTypedefEntry){
	struct TypedefEntry* typedefEntryPtr = globalTypedefEntries.entries+indexOfTypedefEntry;
	assert(typedefEntryPtr->isThisSlotTaken);
	typedefEntryPtr->isThisSlotTaken = false;
	cosmic_free(typedefEntryPtr->typeSpecifier);
}

// returns -1 if it is not there
int32_t indexOfTypedefEntryForSectionOfString(const char* string,const int32_t startIndex,const int32_t endIndex){
	uint32_t lengthOfThisSection = endIndex-startIndex;
	for (uint32_t i=0;i<globalTypedefEntries.numberOfAllocatedSlots;i++){
		if (globalTypedefEntries.entries[i].isThisSlotTaken && 
			globalTypedefEntries.entries[i].lengthOfIdentifier==lengthOfThisSection){
			char *thisTypeSpecifier = globalTypedefEntries.entries[i].typeSpecifier;
			uint32_t i2 = 0;
			bool isEqual = true;
			while (thisTypeSpecifier[i2]!=' '){
				if (thisTypeSpecifier[i2]!=string[startIndex+i2]){
					isEqual=false;
					break;
				}
				i2++;
			}
			if (isEqual) return i;
		}
	}
	return -1;
}


bool isSectionOfStringTypedefed(const char* string,const int32_t startIndex,const int32_t endIndex){
	return -1 != indexOfTypedefEntryForSectionOfString(string,startIndex,endIndex);
}


// does not check for a space
// expects string to be valid in the range given
bool isSegmentOfStringTypeLike(const char*const string, const int32_t startIndex, const int32_t endIndex){
	{
	const char* p;
	char buf[8];
	switch (endIndex-startIndex){
		case 8:
		p = string+startIndex;
		buf[0]=*(p++);
		buf[1]=*(p++);
		buf[2]=*(p++);
		buf[3]=*(p++);
		buf[4]=*(p++);
		buf[5]=*(p++);
		buf[6]=*(p++);
		buf[7]=*(p++);
		if (
		(buf[0]=='u' & buf[1]=='n' & buf[2]=='s' & buf[3]=='i' & buf[4]=='g' & buf[5]=='n' & buf[6]=='e' & buf[7]=='d') | //unsigned
		(buf[0]=='v' & buf[1]=='o' & buf[2]=='l' & buf[3]=='a' & buf[4]=='t' & buf[5]=='i' & buf[6]=='l' & buf[7]=='e') | //volatile
		(buf[0]=='r' & buf[1]=='e' & buf[2]=='g' & buf[3]=='i' & buf[4]=='s' & buf[5]=='t' & buf[6]=='e' & buf[7]=='r')   //register
		){
			return true;
		}
		break;
		case 6:
		p = string+startIndex;
		buf[0]=*(p++);
		buf[1]=*(p++);
		buf[2]=*(p++);
		buf[3]=*(p++);
		buf[4]=*(p++);
		buf[5]=*(p++);
		if (
		(buf[0]=='s' & buf[1]=='i' & buf[2]=='g' & buf[3]=='n' & buf[4]=='e' & buf[5]=='d') | //signed
		(buf[0]=='s' & buf[1]=='t' & buf[2]=='r' & buf[3]=='u' & buf[4]=='c' & buf[5]=='t') | //struct
		(buf[0]=='d' & buf[1]=='o' & buf[2]=='u' & buf[3]=='b' & buf[4]=='l' & buf[5]=='e') | //double
		(buf[0]=='s' & buf[1]=='t' & buf[2]=='a' & buf[3]=='t' & buf[4]=='i' & buf[5]=='c') | //static
		(buf[0]=='e' & buf[1]=='x' & buf[2]=='t' & buf[3]=='e' & buf[4]=='r' & buf[5]=='n') | //extern
		(buf[0]=='i' & buf[1]=='n' & buf[2]=='l' & buf[3]=='i' & buf[4]=='n' & buf[5]=='e')   //inline
		){
			return true;
		}
		break;
		case 5:
		p = string+startIndex;
		buf[0]=*(p++);
		buf[1]=*(p++);
		buf[2]=*(p++);
		buf[3]=*(p++);
		buf[4]=*(p++);
		if (
		(buf[0]=='s' & buf[1]=='h' & buf[2]=='o' & buf[3]=='r' & buf[4]=='t') | // short
		(buf[0]=='_' & buf[1]=='B' & buf[2]=='o' & buf[3]=='o' & buf[4]=='l') | //_Bool
		(buf[0]=='u' & buf[1]=='n' & buf[2]=='i' & buf[3]=='o' & buf[4]=='n') | //union
		(buf[0]=='c' & buf[1]=='o' & buf[2]=='n' & buf[3]=='s' & buf[4]=='t') | //const
		(buf[0]=='f' & buf[1]=='l' & buf[2]=='o' & buf[3]=='a' & buf[4]=='t')   //float
		){
			return true;
		}
		break;
		case 4:
		p = string+startIndex;
		buf[0]=*(p++);
		buf[1]=*(p++);
		buf[2]=*(p++);
		buf[3]=*(p++);
		if (
		(buf[0]=='l' & buf[1]=='o' & buf[2]=='n' & buf[3]=='g') | //long
		(buf[0]=='c' & buf[1]=='h' & buf[2]=='a' & buf[3]=='r') | //char
		(buf[0]=='e' & buf[1]=='n' & buf[2]=='u' & buf[3]=='m') | //enum
		(buf[0]=='v' & buf[1]=='o' & buf[2]=='i' & buf[3]=='d') | //void
		(buf[0]=='a' & buf[1]=='u' & buf[2]=='t' & buf[3]=='o')   //auto
		){
			return true;
		}
		break;
		case 3:
		p = string+startIndex;
		if (
		(p[0]=='i' & p[1]=='n' & p[2]=='t')   //int
		){
			return true;
		}
	}
	}
	return isSectionOfStringTypedefed(string,startIndex,endIndex);
}


/*
writes 8 characters
the letters of hexadecimal are uppercase
does not shift the characters in the string over, it just writes to them
*/
void writeHexInString(char* string, uint32_t numberToTranslate){
	{
	uint8_t byteArray[4] = {
		((char*)&numberToTranslate)[0],
		((char*)&numberToTranslate)[1],
		((char*)&numberToTranslate)[2],
		((char*)&numberToTranslate)[3]
	};
	assert(*((uint32_t*)byteArray)==numberToTranslate);
	// if this assert fails, then the current computer's arcitecture is not compatible with this compiler
	}
	
	for (unsigned int i=0;i<4u;i++){
		unsigned int byte0=((char*)&numberToTranslate)[3u-i];
		unsigned int digit0=(byte0>>4)&0xFu;
		unsigned int digit1=(byte0>>0)&0xFu;
		digit0=digit0+(digit0>=10u)*7u+48u;
		digit1=digit1+(digit1>=10u)*7u+48u;
		string[i*2u+0u]=digit0;
		string[i*2u+1u]=digit1;
	}
}

// reads 8 characters. assumes this would be a valid operation. letters should be uppercase
uint32_t readHexInString(const char* string){
	uint32_t value=0;
	for (unsigned int i=0;i<8u;i++){
		value*=16u;
		uint8_t c=string[i];
		value+=c-('0'+(c>='A')*(('A'-'9')-1));
	}
	return value;
}




// does not check for string literals, assumes input is valid
int32_t getIndexOfMatchingEnclosement(const char* string,const int32_t index){
	const char list[]={'(',')','<','>','[',']','{','}'};
	int32_t i=index;
	const char next=string[index];
	const char type=(next==list[1])*1|(next==list[2])*2|(next==list[3])*3|(next==list[4])*4|(next==list[5])*5|(next==list[6])*6|(next==list[7])*7;
	const char end=list[type^1];
	char c;
	if (type&1){
		while ((c=string[--i])){
			if (c==end) return i;
			if (c==next) i=getIndexOfMatchingEnclosement(string,i);
		}
	} else {
		while ((c=string[++i])){
			if (c==end) return i;
			if (c==next) i=getIndexOfMatchingEnclosement(string,i);
		}
	}
	printf("Internal Error: internal enclosement match failed\n");
	exit(1);
}

int32_t emptyIndexRecede(int32_t strStart){
	int32_t i=strStart+1;
	while (i!=0){
		char c=sourceContainer.string[--i];
		if (!(c==' ' | c=='\n')) return i;
	}
	err_1101_("Expected something before this point",strStart);
	return 0; // unreachable
}

int32_t emptyIndexAdvance(int32_t index){
	char c;
	int32_t i=index;
	while ((c=sourceContainer.string[i])){
		if (c==' ' | c=='\n') i++;
		else return i;
	}
	err_1101_("Unexpected EOF after this point",index-1);
	return 0; // unreachable
}



#include "NumberParser.c"



#endif //#ifndef HAS_COMMON_BEEN_INCLUDED

