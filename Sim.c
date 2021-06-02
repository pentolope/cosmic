
#include <stddef.h>
#include <time.h>
#include <stdnoreturn.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <windows.h>

#define INCLUDE_BACKEND
#include "StatementWalk.c"
#include "FinalizersAndFileIO.c"




struct GlobalSDL_Information{
	SDL_Window* window;
	SDL_Surface* surfaceOnWindow;
	SDL_Event event;
	const struct {
		uint16_t width;
		uint16_t height;
		uint16_t sizeOfUnit;
	} windowSize;
	uint8_t* pixelState;
	uint16_t eventPullCountdown;
	bool isSLD_initialized;
	bool hasPixelBeenWritten;
	bool hasPixelBeenWrittenSinceVisualDelay;
	bool isVisualDelaying;
} globalSDL_Information = {.windowSize = {320,200,3},.eventPullCountdown=1};



struct AddressInstructionPair{
	uint32_t address;
	bool skipThis;
	InstructionSingle IS;
}* addressToInstructionTranslation;
uint32_t addressToInstructionTranslationLen;

uint64_t instructionExecutionCount;
bool printEachInstruction;
bool doFPGAbootGeneration;
bool doGenerateFastSim;
bool doCacheSim;
bool isTerminationTriggered;
uint16_t terminationValue;
uint32_t endOfExecutable;

uint32_t tracebackLevel;
bool hadTypicalEnd=true;

struct MemSeg{
	uint8_t seg[1LU<<11];
	uint64_t ieCountAtSet[1LU<<11];
};

struct MachineState{
	uint32_t pc;
	uint16_t sp;
	uint16_t reg[16];
	
	uint64_t ieCountAtSetPc;
	uint64_t ieCountAtSetSp;
	uint64_t ieCountAtSetReg[16];
	
	uint8_t walk_cache;
	uint16_t back_cache[16];
	struct MemSeg cache[1LU<<5];
	uint8_t in_cache[1LU<<15];
	struct MemSeg mainMem[1LU<<15];
};

struct MachineState* machineState;
struct MachineState* initialMachineState;

void performTraceback(uint64_t);
void printProfileResults();


noreturn void bye(void){
	printf("\nTotal instructions executed:%llu\n",(unsigned long long)instructionExecutionCount);
	printProfileResults();
	fflush(stdout);
	// these were allocated using normal calloc, not cosmic_calloc
	free(globalSDL_Information.pixelState);
	free(machineState);
	free(initialMachineState);
	
	if (globalSDL_Information.isSLD_initialized){
		if (globalSDL_Information.hasPixelBeenWritten){
			if (globalSDL_Information.event.type != SDL_QUIT){
				printf("Waiting for SDL_QUIT event...\n");
				fflush(stdout);
				do {
					Sleep(15);
					SDL_PollEvent(&globalSDL_Information.event);
				} while (globalSDL_Information.event.type != SDL_QUIT);
			}
		} else {
			printf("All pixels never left the initialized state. Therefore not waiting for SDL_QUIT event.\n");
		}
		SDL_Quit();
	}
	printf("Exiting...\n");
	fflush(stdout);
	exit(0);
}

void writePixelToScreenWithSDL(uint16_t a, uint8_t c){
	if (a==0x0000FFFF){
		if (!(globalSDL_Information.isVisualDelaying=c!=0) & globalSDL_Information.hasPixelBeenWrittenSinceVisualDelay){
			const SDL_Rect rect={.x=1,.y=1,.h=globalSDL_Information.windowSize.height*globalSDL_Information.windowSize.sizeOfUnit,.w=globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.sizeOfUnit};
			SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&rect,1);
			globalSDL_Information.hasPixelBeenWrittenSinceVisualDelay=false;
		}
	} else if (a<(uint32_t)globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.height && globalSDL_Information.pixelState[a]!=c){
		globalSDL_Information.pixelState[a]=c;
		globalSDL_Information.hasPixelBeenWritten=true;
		const uint8_t r=(255u/7u)*(7u&(unsigned)c/32u)+1u;
		const uint8_t g=(255u/7u)*(7u&(unsigned)c/4u)+1u;
		const uint8_t b=(255u/3u)*(3u&(unsigned)c);
		const SDL_Rect rect={.x=(a%320u)*globalSDL_Information.windowSize.sizeOfUnit+1,.y=((a/320u)%200u)*globalSDL_Information.windowSize.sizeOfUnit+1,.h=globalSDL_Information.windowSize.sizeOfUnit,.w=globalSDL_Information.windowSize.sizeOfUnit};
		SDL_FillRect(globalSDL_Information.surfaceOnWindow,&rect,SDL_MapRGB(globalSDL_Information.surfaceOnWindow->format, r, g, b));
		
		if (!globalSDL_Information.isVisualDelaying) SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&rect,1); // updating is the slow part
		else globalSDL_Information.hasPixelBeenWrittenSinceVisualDelay=true;
	}
}

void initializeSDL(){
	if (SDL_Init(SDL_INIT_VIDEO)!=0){
		printf("Error initializing SDL: %s\n", SDL_GetError());
		bye();
	}
	globalSDL_Information.window = SDL_CreateWindow("Sim",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.sizeOfUnit+2,
		globalSDL_Information.windowSize.height*globalSDL_Information.windowSize.sizeOfUnit+2,
		0
	);
	globalSDL_Information.surfaceOnWindow = SDL_GetWindowSurface(globalSDL_Information.window);
	
	const SDL_Rect fullRect={.x=0,.y=0,.h=globalSDL_Information.windowSize.height*globalSDL_Information.windowSize.sizeOfUnit+2,.w=globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.sizeOfUnit+2};
	SDL_FillRect(globalSDL_Information.surfaceOnWindow,&fullRect,SDL_MapRGB(globalSDL_Information.surfaceOnWindow->format, 0, 0, 0));
	SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&fullRect,1);
	globalSDL_Information.isSLD_initialized=true;
}

void stateDump(){
	makeColor(COLOR_TO_TEXT,COLOR_GRAY_OR_BLACK);
	printf("{ pc:%08X ",machineState->pc);
	printf(" sp:%04X ",machineState->sp);
	for (int i=0;i<16;i++){
		printf(" %%%01X:%04X ",i,machineState->reg[i]);
	}
	printf("}\n");
	resetColor();
	fflush(stdout);
}


void initMemWrite(uint32_t a,uint8_t b){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if (aUD!=0){
		printf("Binary won't fit in memory\n");
		bye();
	}
	machineState->mainMem[aU_].seg[aL]=b;
}

uint8_t initMemRead(uint32_t a){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	assert(aUD==0);
	return machineState->mainMem[aU_].seg[aL];
}

void initPush(unsigned int w){
	w&=0xFFFFu;
	machineState->sp-=2;
	uint32_t a=machineState->sp;
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	machineState->mainMem[aU_].seg[aL+0]=(uint8_t)((unsigned)w>>0);
	machineState->mainMem[aU_].seg[aL+1]=(uint8_t)((unsigned)w>>8);
}

void createInitialMachineState(){
	*initialMachineState=*machineState;
}

void restoreMachineState(){
	*machineState=*initialMachineState;
}



/*
all special accesses are performed using byte operations [volatile may be needed] (unless otherwise specified). ranges are given as inclusive ranges

GPU memory may be written to at 0x04000000->0x0400FFFE
For speed and visual purposes, the simulation's screen pixel changes can be visually delayed by writing a 1 to 0x0400FFFF, then an update may be triggered by writting a 0 to 0x0400FFFF

Halt/Exit is achieved by WORD writing to 0x04010000 , the value is the return value.
If main terminates, it will set the pc to 0x04010000, at which point the value at 0x0000FFFE will be interpreted as the return value.

Opening a file for reading or writing may be done by writing mode to 0x04020001 , then writing path to 0x04020002 (null termination, simply write each byte to same address)
  if successful, reading at the location 0x04020003 will hold the value 1, otherwise it will hold the value 0
  closing a file may be done by writing a 0 to 0x04020003
  only one file may be opened at a time
  Once a file is opened for reading, it's contents are in the range 0x05000000->0x05FFFFFF and the length [which may be word read/written] is at 0x04020006->0x04020009
  Once a file is opened for writing, it's contents can be written to by writting to 0x05000000 repeatedly (there is no sense of position)

[Debug only please] a character may be written to the simulator console by writting to 0x04030000 with the character


*/


// currently, this will only work for reading
struct FileAccessInfo{
	struct StringBuilder name;
	struct StringBuilder mode;
	uint8_t* contents;
	uint32_t length;
	bool isRead;
	FILE* outFile;
} fileAccessInfo={.isRead=true};

void triggerFileLoad(){
	fileAccessInfo.length=0;
	fileAccessInfo.contents=NULL;
	fileAccessInfo.outFile=NULL;
	fileAccessInfo.isRead=true;
	char* filePath=stringBuilderToString(&fileAccessInfo.name);
	char* fileMode=stringBuilderToString(&fileAccessInfo.mode);
	stringBuilderClear(&fileAccessInfo.name);
	stringBuilderClear(&fileAccessInfo.mode);
	//printf("\nfopen(\"%s\",\"%s\")->",filePath,fileMode);
	FILE* inputFile = fopen(filePath,fileMode);
	if (inputFile==NULL){
		//printf("NULL\n");
		return;
	}
	//printf("VALID\n");
	if ((fileAccessInfo.isRead=!doStringsMatch(fileMode,"wb"))){
		fpos_t startingPositionOfFile;
		fgetpos(inputFile,&startingPositionOfFile);
		while (fgetc(inputFile)!=EOF){
			fileAccessInfo.length++;
			if (fileAccessInfo.length>268435456L){ // 2**28
				fclose(inputFile);
				printf("%s",strMerge3("Error: Input file \"",filePath,"\" is too large"));
				bye();
			}
		}
		fsetpos(inputFile,&startingPositionOfFile);
		fileAccessInfo.contents = cosmic_malloc(fileAccessInfo.length*sizeof(char));
		for (int32_t i=0;i<fileAccessInfo.length;i++){
			int c=fgetc(inputFile);
			assert(c!=EOF);
			fileAccessInfo.contents[i]=c;
		}
		fclose(inputFile);
	} else {
		fileAccessInfo.outFile=inputFile;
	}
	cosmic_free(filePath);
	cosmic_free(fileMode);
}

void triggerFileClose(){
	if (fileAccessInfo.isRead){
		cosmic_free(fileAccessInfo.contents);
	} else {
		fclose(fileAccessInfo.outFile);
	}
	fileAccessInfo.outFile=NULL;
	fileAccessInfo.contents=NULL;
	fileAccessInfo.length=0;
	//printf("\nfclose()\n");
}





void handleCacheMiss(){
	
}


uint16_t readWord(uint32_t a){
	//if (a!=machineState->pc) printf("MEM R word:[%08X]\n",a);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:word read misalligned:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (aUD!=0){
		if (a==0x04020006){
			return (uint16_t)(fileAccessInfo.length>> 0);
		}
		if (a==0x04020008){
			return (uint16_t)(fileAccessInfo.length>> 16);
		}
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:word read out of bounds:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		uint16_t w=machineState->mainMem[aU_].seg[aL+0];
		w|=(unsigned)machineState->mainMem[aU_].seg[aL+1]<<8;
		return w;
	}
}

uint8_t readByte(uint32_t a){
	//printf("MEM R byte:[%08X]\n",a);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if (aUD!=0){
		if (a==0x04020003){
			return fileAccessInfo.isRead?fileAccessInfo.contents!=NULL:fileAccessInfo.outFile!=NULL;
		}
		if (a>=0x05000000 & a<=0x05FFFFFF){
			if (fileAccessInfo.contents==NULL | !fileAccessInfo.isRead){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Mem fault:byte read at file when no file is open for read:%08X\n",a);
				resetColor();
				stateDump();
				bye();
			}
			if (a-0x05000000<fileAccessInfo.length){
				//printf("{%08X:%02X}",a-0x05000000,fileAccessInfo.contents[a-0x05000000]);
				if (!fileAccessInfo.isRead | fileAccessInfo.contents==NULL){
					makeColor(COLOR_TO_TEXT,COLOR_RED);
					printf("Mem fault:byte read at file when no file is open for read:%08X\n",a);
					resetColor();
					stateDump();
					bye();
				}
				return fileAccessInfo.contents[a-0x05000000];
			}
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("Mem warn:byte read at file out of bounds:%08X\n",a);
			resetColor();
			return -1;// EOF
		}
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:byte read out of bounds:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		return machineState->mainMem[aU_].seg[aL];
	}
}

void writeWord(uint32_t a,uint16_t w){
	//printf("MEM W word:[%08X,%04X]\n",a,w);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:word write misalligned:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (aUD!=0){
		if (a==0x04010000){
			isTerminationTriggered=true;
			terminationValue=w;
			return;
		}
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:word write out of bounds:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		machineState->mainMem[aU_].seg[aL+0]=(uint8_t)((unsigned)w>>0);
		machineState->mainMem[aU_].seg[aL+1]=(uint8_t)((unsigned)w>>8);
		machineState->mainMem[aU_].ieCountAtSet[aL+0]=instructionExecutionCount;
		machineState->mainMem[aU_].ieCountAtSet[aL+1]=instructionExecutionCount;
		return;
	}
}

void writeByte(uint32_t a,uint8_t b){
	//printf("MEM W byte:[%08X,%02X]\n",a,b);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if (aUD!=0){
		if (a==0x04030000){
			printf("%c",b);
			fflush(stdout);
			return;
		}
		if (a==0x04020001){
			stringBuilderAppendChar(&fileAccessInfo.mode,b);
			return;
		}
		if (a==0x04020002){
			stringBuilderAppendChar(&fileAccessInfo.name,b);
			if (b==0){
				triggerFileLoad();
			}
			return;
		}
		if (a==0x04020003){
			if (b!=0){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Mem fault:byte write to file open detector is not 0:%08X\n",a);
				resetColor();
				stateDump();
				bye();
			}
			triggerFileClose();
			return;
		}
		if (a>=0x04000000 && a<=0x0400FFFF){
			writePixelToScreenWithSDL(a-0x04000000,b);
			return;
		}
		if (a==0x05000000){
			if (fileAccessInfo.outFile==NULL | fileAccessInfo.isRead){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Mem fault:byte write at file when no file is open for write\n");
				resetColor();
				stateDump();
				bye();
			}
			if (fputc(b,fileAccessInfo.outFile)!=b){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Sim fault:could not write byte:%02X\n",b);
				resetColor();
				stateDump();
				bye();
			}
			fflush(fileAccessInfo.outFile);
			return;
		}
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Mem fault:byte write out of bounds:%08X\n",a);
		resetColor();
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		machineState->mainMem[aU_].seg[aL]=b;
		machineState->mainMem[aU_].ieCountAtSet[aL]=instructionExecutionCount;
		return;
	}
}

void push(uint16_t w){
	if ((machineState->sp&1)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Error:Stack Pointer not in allignment (at push) %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
		resetColor();
		bye();
	}
	machineState->sp-=2;
	writeWord(machineState->sp,w);
}
uint16_t pop(){
	if ((machineState->sp&1)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Error:Stack Pointer not in allignment (at pop) %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
		resetColor();
		bye();
	}
	uint16_t r=readWord(machineState->sp);
	machineState->sp+=2;
	return r;
}

uint16_t countdownToFlush;
char printf_buffer[120000];
uint16_t stackLengths[256]={[0]=6};
const char* stackNames[2048]={[0]="main"};
uint32_t stackAddresses[2048];
int32_t stackLabelIndexes[2048]={[0]=-1};
uint16_t stackPosition=1;
uint16_t stackPrintLimit=40;

uint32_t numberOfLabels;
uint64_t* labelExecutionCountSkipped;
uint64_t* labelExecutionCountTopSkipped;
uint64_t totalExecutionCountSkipped;
uint32_t* labelNumbers;
uint32_t* labelAddresses;
char** labelNames;


uint32_t getLabelIndex(uint32_t labelToSearch){
	for (uint32_t i=0;i<numberOfLabels;i++){
		if (labelNumbers[i]==labelToSearch) return i;
	}
	printf("Unresolved Label %08X\n",labelToSearch);
	bye();
}

uint32_t getLabelAddress(uint32_t labelToSearch){
	return labelAddresses[getLabelIndex(labelToSearch)];
}

char* getFunctionNameForAddress(uint32_t address, int32_t* index){
	for (uint32_t i=0;i<numberOfLabels;i++){
		if (labelAddresses[i]==address){
			*index=i;
			return labelNames[i];
		}
	}
	*index=-1;
	return NULL;
}

void printProfileResults(){
	if (totalExecutionCountSkipped!=0){
		printf("\nProfile results:\n");
		uint32_t functionCount=0;
		for (uint32_t i=0;i<numberOfLabels;i++){
			if (labelExecutionCountSkipped[i]!=0){
				functionCount++;
			}
		}
		uint32_t* arrangeIndexes=cosmic_calloc(functionCount*sizeof(uint32_t),1);
		functionCount=0;
		for (uint32_t i=0;i<numberOfLabels;i++){
			if (labelExecutionCountSkipped[i]!=0){
				arrangeIndexes[functionCount++]=i;
			}
		}
		for (uint32_t i0=1;i0<functionCount;i0++){
			uint32_t i1=i0-1;
			uint32_t i2=i0;
			uint32_t i1v=arrangeIndexes[i1];
			uint32_t i2v=arrangeIndexes[i2];
			while (labelExecutionCountSkipped[i1v]>labelExecutionCountSkipped[i2v]){
				uint32_t t=arrangeIndexes[i2];
				arrangeIndexes[i2]=arrangeIndexes[i1];
				arrangeIndexes[i1]=t;
				if (i1==0){
					break;
				} else {
					i2v=arrangeIndexes[--i2];
					i1v=arrangeIndexes[--i1];
				}
			}
		}
		for (uint32_t i=0;i<functionCount;i++){
			uint32_t iv=arrangeIndexes[i];
			double percent0=((double)labelExecutionCountSkipped[iv]/(double)totalExecutionCountSkipped)*100;
			unsigned int wholePercent0=(unsigned int)percent0;
			unsigned int decimalPercent0=(unsigned int)((percent0-wholePercent0)*1000);
			double percent1=((double)labelExecutionCountTopSkipped[iv]/(double)totalExecutionCountSkipped)*100;
			unsigned int wholePercent1=(unsigned int)percent1;
			unsigned int decimalPercent1=(unsigned int)((percent1-wholePercent1)*1000);
			char* name=labelNames[iv];
			printf("%3u.%04u / %3u.%04u : ",wholePercent0,decimalPercent0,wholePercent1,decimalPercent1);
			if (name==NULL){
				printf("@ %08X\n",labelAddresses[iv]);
			} else {
				printf("%s()\n",name);
			}
		}
		printf("\n");
		cosmic_free(arrangeIndexes);
		fflush(stdout);
	} else {
		printf("Profile results do not exist\n");
	}
}

void handleFunctionStackPrint(uint32_t address, bool isCall){
	if (isCall){
		if (stackPosition>2000){
			printf("Error: Too many call instructions for profiler\n");
			bye();
		}
		int32_t index;
		const char*const name=getFunctionNameForAddress(address,&index);
		stackAddresses[stackPosition]=address;
		stackNames[stackPosition]=name;
		stackLabelIndexes[stackPosition]=index;
		if (stackPosition>stackPrintLimit){
			stackPosition++;
			return;
		}
		if (!printEachInstruction) printf("%5d:",stackPosition);
		if (name==NULL){
			if (!printEachInstruction) printf("@ %08X\n",address);
			stackLengths[stackPosition++]=16;
		} else {
			if (!printEachInstruction) printf("%s()\n",name);
			stackLengths[stackPosition++]=strlen(name)+8;
		}
		//fflush(stdout);
	} else {
		if (stackPosition<1){
			printf("Error: Too many ret instructions\n");
			bye();
		}
		if (stackPosition>stackPrintLimit+1){
			stackPosition--;
			return;
		}
		if (!printEachInstruction) printf("%c%c%c%c",
			 27,
			'[',
			'1',
			'F');
		const uint16_t l=stackLengths[--stackPosition];
		for (uint16_t j=0;j<l;j++){
			if (!printEachInstruction) printf(" ");
		}
		if (!printEachInstruction) printf("\r");
		//fflush(stdout);
	}
}

void profilerTick(){
	totalExecutionCountSkipped+=1;
	if (stackPosition>0 && stackLabelIndexes[stackPosition-1]!=-1){
		labelExecutionCountTopSkipped[stackLabelIndexes[stackPosition-1]]+=1;
	}
	for (uint16_t i0=0;i0<stackPosition;i0++){
		const int32_t index=stackLabelIndexes[i0];
		if (index!=-1){
			for (uint16_t i1=0;i1<i0;i1++){
				if (index==stackLabelIndexes[i1]){
					goto LoopEnd;
				}
			}
			labelExecutionCountSkipped[index]+=1;
		}
		LoopEnd:;
	}
}

void singleExecute(){
	instructionExecutionCount++;
	if (printEachInstruction){
		if (countdownToFlush==0){
			fflush(stdout);
			profilerTick();
		}
		countdownToFlush--;
		countdownToFlush &= 0xFFF;
	} else {
		fflush(stdout);
		profilerTick();
	}
	unsigned instr=readWord(machineState->pc);
	uint8_t type=((instr&0xF000u)!=0xF000u)?((instr>>12)&0xFu):(((instr>>8)&0xFu)|0x10u);
	uint8_t r0=(instr>>0)&0xFu;
	uint8_t r1=(instr>>4)&0xFu;
	uint8_t r2=(instr>>8)&0xFu;
	uint8_t imm=(instr>>4)&0xFFu;
	uint32_t temp;
	if (printEachInstruction){
		makeColor(COLOR_TO_TEXT,COLOR_BLUE);
		printf("Exec:%08X->%04X[%02X]  ",machineState->pc,instr,type);
		resetColor();
		bool didFind=false;
		for (uint32_t i=0;i<addressToInstructionTranslationLen;i++){
			if (!addressToInstructionTranslation[i].skipThis & machineState->pc==addressToInstructionTranslation[i].address){
				didFind=true;
				makeColor(COLOR_TO_TEXT,COLOR_GREEN);
				printSingleInstructionOptCode(addressToInstructionTranslation[i].IS);
				resetColor();
				break;
			}
		}
		if (!didFind){
			makeColor(COLOR_TO_TEXT,COLOR_YELLOW);
			printf("^^^^");
			resetColor();
		}
		printf("\n");
		
	}
	machineState->pc+=2;
	switch (type){
		case 0x00:machineState->reg[r0]=imm;machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x01:machineState->reg[r0]|=((unsigned)imm<<8)&0xFF00u;machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x02:machineState->reg[r0]=readWord(machineState->reg[1]+2*imm);machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x03:writeWord(machineState->reg[1]+2*imm,machineState->reg[r0]);break;
		case 0x04:machineState->reg[r0]=machineState->reg[r1]&machineState->reg[r2];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x05:machineState->reg[r0]=machineState->reg[r1]|machineState->reg[r2];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x06:machineState->reg[r0]=machineState->reg[r1]^machineState->reg[r2];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x07:
		temp=(uint32_t)machineState->reg[r0]+(uint32_t)machineState->reg[r1]+(uint32_t)(~machineState->reg[r2]&0xFFFFu);
		machineState->reg[r1]=(uint16_t)temp;
		machineState->reg[r0]=(temp&0x00030000)!=0;
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		machineState->ieCountAtSetReg[r1]=instructionExecutionCount;
		break;
		case 0x08:machineState->reg[r0]=readWord(((uint32_t)machineState->reg[r2]<<16)|machineState->reg[r1]);machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x09:writeWord(((uint32_t)machineState->reg[r2]<<16)|machineState->reg[r1],machineState->reg[r0]);break;
		case 0x0A:machineState->reg[r0]=machineState->reg[r1]+machineState->reg[r2];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x0B:
		temp=(uint32_t)machineState->reg[r1]+(uint32_t)machineState->reg[r2]+(uint32_t)machineState->reg[15];
		machineState->reg[r0]=(uint16_t)temp;
		machineState->reg[15]=(temp&0x00030000)!=0;
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		machineState->ieCountAtSetReg[15]=instructionExecutionCount;
		break;
		case 0x0C:
		temp=(uint32_t)machineState->reg[r1]+(~(uint32_t)machineState->reg[r2]&0xFFFFu)+(uint32_t)1;
		machineState->reg[r0]=(uint16_t)temp;
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		break;
		case 0x0D:
		temp=(uint32_t)machineState->reg[r1]+(~(uint32_t)machineState->reg[r2]&0xFFFFu)+(uint32_t)1;
		machineState->reg[r0]=(temp&0x00010000)!=0;
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		break;
		case 0x0E:
		if (machineState->reg[r2]==0){
			machineState->pc=((uint32_t)machineState->reg[r0])|((uint32_t)machineState->reg[r1]<<16);
			machineState->ieCountAtSetPc=instructionExecutionCount;
		}
		break;
		case 0x10:push(machineState->reg[r0]);break;
		case 0x11:push(machineState->reg[r0]);push(machineState->reg[r1]);break;
		case 0x12:machineState->reg[r0]=pop();machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x13:machineState->reg[r0]=pop();machineState->reg[r1]=pop();machineState->ieCountAtSetReg[r0]=instructionExecutionCount;machineState->ieCountAtSetReg[r1]=instructionExecutionCount;break;
		case 0x14:machineState->reg[r0]=machineState->reg[r1];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x15:machineState->reg[r0]=(((unsigned)machineState->reg[r1]<<8)&0xFF00u)|(((unsigned)machineState->reg[r1]>>8)&0x00FFu);machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x16:machineState->reg[r0]=((unsigned)machineState->reg[r1]>>1)&0x7FFFu;machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x17:machineState->reg[r0]*=machineState->reg[r1];machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x18:
		temp=((uint32_t)machineState->reg[14]<<16)|(uint32_t)machineState->reg[13];
		temp*=((uint32_t)machineState->reg[r1]<<16)|(uint32_t)machineState->reg[r0];
		machineState->reg[13]=(uint16_t)temp;
		machineState->reg[14]=(uint16_t)(temp>>16);
		machineState->ieCountAtSetReg[13]=instructionExecutionCount;
		machineState->ieCountAtSetReg[14]=instructionExecutionCount;
		break;
		case 0x19:
		temp=machineState->reg[r0]/machineState->reg[r1];
		machineState->reg[r1]=machineState->reg[r0]%machineState->reg[r1];
		machineState->reg[r0]=temp;
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		machineState->ieCountAtSetReg[r1]=instructionExecutionCount;
		break;
		case 0x1A:
		push(machineState->reg[0]);
		push(machineState->reg[1]);
		push((machineState->pc>>16)&0xFFFFu);
		push(machineState->pc&0xFFFFu);
		machineState->reg[0]=machineState->sp;
		machineState->pc=(uint32_t)machineState->reg[r0]|((uint32_t)machineState->reg[r1]<<16);
		machineState->ieCountAtSetReg[r0]=instructionExecutionCount;
		machineState->ieCountAtSetPc=instructionExecutionCount;
		handleFunctionStackPrint(machineState->pc,true);
		break;
		case 0x1B:
		machineState->sp=machineState->reg[0];
		temp=pop();
		machineState->pc=((uint32_t)pop()<<16)|temp;
		machineState->reg[1]=pop();
		machineState->reg[0]=pop();
		temp=pop();
		machineState->sp+=temp;
		machineState->ieCountAtSetReg[1]=instructionExecutionCount;
		machineState->ieCountAtSetReg[0]=instructionExecutionCount;
		machineState->ieCountAtSetPc=instructionExecutionCount;
		machineState->ieCountAtSetSp=instructionExecutionCount;
		handleFunctionStackPrint(machineState->pc,false);
		break;
		case 0x1C:machineState->reg[r0]=readByte(((uint32_t)machineState->reg[r1]<<16)|machineState->reg[13]);machineState->ieCountAtSetReg[r0]=instructionExecutionCount;break;
		case 0x1D:writeByte(((uint32_t)machineState->reg[r1]<<16)|machineState->reg[13],machineState->reg[r0]&0x00FFu);break;
		case 0x1E:machineState->pc=((uint32_t)machineState->reg[r0])|((uint32_t)machineState->reg[r1]<<16);machineState->ieCountAtSetPc=instructionExecutionCount;break;
		case 0x1F:machineState->sp-=machineState->reg[r0];machineState->reg[r0]=machineState->sp;machineState->ieCountAtSetSp=instructionExecutionCount;break;
		
		
		case 0x0F:
		default:printf("Instruction corrupted (%d)\n",type);bye();
	}
	if (printEachInstruction){
		stateDump();
		printf("\n\n");
	}
}


void fullExecute(){
	while (!isTerminationTriggered){
		if (globalSDL_Information.eventPullCountdown--==0){
			SDL_PollEvent(&globalSDL_Information.event);
			if (globalSDL_Information.event.type == SDL_QUIT){
				printf("Premature Exit Initiated...\n");
				bye();
			}
			globalSDL_Information.eventPullCountdown=20;
		}
		if (machineState->pc==0x04010000){
			isTerminationTriggered=true;
			terminationValue=readWord(0x0000FFFE);
			return;
		}
		if (machineState->sp>machineState->reg[1]){
			//printf("Over %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
		}
		if (machineState->pc<0x00010000 | machineState->pc>=endOfExecutable){
// this will need to be disabled when using the loader
#if 0
printf("Error:Program Counter out of bounds %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
bye();
#endif
		}
		if ((machineState->sp&1)!=0){
			printf("Error:Stack Pointer not in allignment (at general) %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
			bye();
		}
		if ((machineState->pc&1)!=0){
			printf("Error:Program Counter not in allignment (at general) %08X:%08X\n",(unsigned int)machineState->pc,(unsigned int)instructionExecutionCount);
			bye();
		}
		singleExecute();
	}
}


void performTraceback(uint64_t iicTarget){
	printf("Traceback not ready yet\n");
	bye();
	restoreMachineState();
	instructionExecutionCount=0;
	globalSDL_Information.eventPullCountdown=1;
	tracebackLevel++;
	while (iicTarget>instructionExecutionCount){
		assert(!isTerminationTriggered);
		if (globalSDL_Information.eventPullCountdown--==0){
			SDL_PollEvent(&globalSDL_Information.event);
			if (globalSDL_Information.event.type == SDL_QUIT){
				printf("Premature Exit Initiated...\n");
				bye();
			}
			globalSDL_Information.eventPullCountdown=20;
		}
		singleExecute();
	}
	if (iicTarget!=instructionExecutionCount){
		printf("Traceback Not Possible\n");
	} else {
		
	}
	tracebackLevel--;
}


uint16_t getIntrinsicID(enum InstructionTypeID id){
	switch (id){
		case I_D32U:return 0x01;
		case I_R32U:return 0x02;
		case I_D32S:return 0x03;
		case I_R32S:return 0x04;
		case I_D64U:return 0x05;
		case I_R64U:return 0x06;
		case I_D64S:return 0x07;
		case I_R64S:return 0x08;
		case I_LAD4:return 0x09;
		case I_LAD5:return 0x0A;
		case I_LSU4:return 0x0B;
		case I_LSU5:return 0x0C;
		case I_LMU4:return 0x0D;
		case I_LMU5:return 0x0E;
		case I_LDI4:return 0x0F;
		case I_LDI5:return 0x10;
		case I_LLS6:return 0x11;
		case I_LLS7:return 0x12;
		case I_LRS6:return 0x13;
		case I_LRS7:return 0x14;
	}
	return 0;
}


static struct {
	uint16_t vs[100]; // [value stack]
	int16_t vsl; // [value stack location]
	int16_t pvsl; // [previous value stack location]
} symbolicStack;


static void symbolicResolutionSingle(InstructionSingle* IS_temp){
	if (symbolicStack.vsl<10){
		printf("ByteCode corrupted, symbolic stack overflow\n");
		bye();
	}
	symbolicStack.pvsl=symbolicStack.vsl;
	bool wasLoad=false;
	uint16_t inSize;
	uint16_t outSize;
	switch (IS_temp->id){
		case I_SYRE:
		case I_SYDE:
		return;
		case I_SYCB:
		symbolicStack.vsl-=1;
		symbolicStack.vs[symbolicStack.vsl  ]=IS_temp->arg.B1.a_0;
		wasLoad=true;
		break;
		case I_SYCW:
		symbolicStack.vsl-=1;
		symbolicStack.vs[symbolicStack.vsl  ]=IS_temp->arg.W.a_0;
		wasLoad=true;
		break;
		case I_SYCD:
		symbolicStack.vsl-=2;
		symbolicStack.vs[symbolicStack.vsl+1]=IS_temp->arg.D.a_0>>16;
		symbolicStack.vs[symbolicStack.vsl+0]=IS_temp->arg.D.a_0>> 0;
		wasLoad=true;
		break;
		case I_SYCL:
		symbolicStack.vsl-=2;
		{
		uint32_t labelAddressesTemp = getLabelAddress(IS_temp->arg.D.a_0);
		symbolicStack.vs[symbolicStack.vsl+1]=labelAddressesTemp>>16;
		symbolicStack.vs[symbolicStack.vsl+0]=labelAddressesTemp>> 0;
		}
		wasLoad=true;
		break;
		case I_SYW0:
		case I_SYW1:
		case I_SYW2:
		case I_SYW3:
		case I_SYW4:
		case I_SYW5:
		case I_SYW6:
		case I_SYW7:
		case I_SYW8:
		case I_SYW9:
		case I_SBLW:
		case I_SBRW:
		case I_SCDW:
		case I_SCDB:
		inSize=2;
		outSize=1;
		break;
		case I_SYD0:
		case I_SYD1:
		case I_SYD2:
		case I_SYD3:
		case I_SYD4:
		case I_SYD5:
		case I_SYD6:
		case I_SYD7:
		case I_SYD8:
		case I_SYD9:
		case I_SCQD:
		inSize=4;
		outSize=2;
		break;
		case I_SYQ0:
		case I_SYQ1:
		case I_SYQ2:
		case I_SYQ3:
		case I_SYQ4:
		case I_SYQ5:
		case I_SYQ6:
		case I_SYQ7:
		case I_SYQ8:
		case I_SYQ9:
		inSize=8;
		outSize=4;
		break;
		case I_SBLD:
		case I_SBRD:
		inSize=3;
		outSize=2;
		break;
		case I_SBLQ:
		case I_SBRQ:
		inSize=5;
		outSize=4;
		break;
		case I_SCWD:
		case I_SCZD:
		inSize=1;
		outSize=2;
		break;
		case I_SCDQ:
		case I_SCZQ:
		inSize=2;
		outSize=4;
		break;
		case I_SCBW:
		case I_SCWB:
		inSize=1;
		outSize=1;
		break;
		case I_SCQB:
		inSize=4;
		outSize=1;
		break;
		default:;assert(false);
	}
	if (!wasLoad){
		symbolicStack.vsl+=inSize;
		symbolicStack.vsl-=outSize;
		if (symbolicStack.pvsl+inSize>100){
			printf("ByteCode corrupted, symbolic stack underflow\n");
			exit(1);
		}
		uint16_t vs_in[8];
		uint32_t val_out; // todo: when updating to allow qword, change to uint64_t
		for (uint16_t i=0;i<inSize;i++){
			vs_in[i]=symbolicStack.vs[symbolicStack.pvsl+i];
		}
		switch (IS_temp->id){
			case I_SYW0:val_out=vs_in[1]+vs_in[0];break;
			case I_SYW1:val_out=vs_in[1]-vs_in[0];break;
			case I_SYW2:val_out=vs_in[1]*vs_in[0];break;
			case I_SYW3:val_out=(  signed)vs_in[1]/(  signed)vs_in[0];break;
			case I_SYW4:val_out=(unsigned)vs_in[1]/(unsigned)vs_in[0];break;
			case I_SYW5:val_out=(  signed)vs_in[1]%(  signed)vs_in[0];break;
			case I_SYW6:val_out=(unsigned)vs_in[1]%(unsigned)vs_in[0];break;
			case I_SYW7:val_out=vs_in[1]^vs_in[0];break;
			case I_SYW8:val_out=vs_in[1]&vs_in[0];break;
			case I_SYW9:val_out=vs_in[1]|vs_in[0];break;
			case I_SBLW:val_out=(unsigned)vs_in[1]<<vs_in[0];break;
			case I_SBRW:val_out=(unsigned)vs_in[1]>>vs_in[0];break;
			case I_SCWB:val_out=vs_in[0]!=0u;break;
			case I_SCDB:val_out=(vs_in[1]|vs_in[0])!=0u;break;
			case I_SCQB:val_out=(vs_in[2]|vs_in[3]|vs_in[0]|vs_in[1])!=0u;break;
			case I_SCZD:case I_SCDW:val_out=vs_in[0];break;
			case I_SCBW:val_out=(((unsigned)vs_in[0]&0xA0u)*0x01FEu)|(unsigned)vs_in[0];break;
			case I_SCWD:val_out=(((uint32_t)vs_in[0]&0xA000u)*(uint32_t)0x0001FFFELU)|(uint32_t)vs_in[0];break;
			case I_SYD0:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))+(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD1:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))-(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD2:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))*(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD3:val_out=((( int32_t)vs_in[2]<<0)|(( int32_t)vs_in[3]<<16))/((( int32_t)vs_in[0]<<0)|(( int32_t)vs_in[1]<<16));break;
			case I_SYD4:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))/(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD5:val_out=((( int32_t)vs_in[2]<<0)|(( int32_t)vs_in[3]<<16))%((( int32_t)vs_in[0]<<0)|(( int32_t)vs_in[1]<<16));break;
			case I_SYD6:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))%(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD7:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))^(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD8:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))&(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SYD9:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16))|(((uint32_t)vs_in[0]<<0)|((uint32_t)vs_in[1]<<16));break;
			case I_SCQD:val_out=(((uint32_t)vs_in[2]<<0)|((uint32_t)vs_in[3]<<16));break;
			case I_SBLD:val_out=(((uint32_t)vs_in[1]<<0)|((uint32_t)vs_in[2]<<16))<<vs_in[0];break;
			case I_SBRD:val_out=(((uint32_t)vs_in[1]<<0)|((uint32_t)vs_in[2]<<16))>>vs_in[0];break;
			
			case I_SYQ0:
			case I_SYQ1:
			case I_SYQ2:
			case I_SYQ3:
			case I_SYQ4:
			case I_SYQ5:
			case I_SYQ6:
			case I_SYQ7:
			case I_SYQ8:
			case I_SYQ9:
			case I_SBLQ:
			case I_SBRQ:
			
			case I_SCDQ:
			case I_SCZQ:
			assert(false);// qword not ready yet
			
			default:;assert(false);
		}
		switch (outSize){
			case 1:
			symbolicStack.vs[symbolicStack.vsl  ]=(uint16_t)val_out;
			break;
			case 2:
			symbolicStack.vs[symbolicStack.vsl+1]=val_out>>16;
			symbolicStack.vs[symbolicStack.vsl+0]=val_out>> 0;
			break;
			case 4:assert(false);// qword not ready yet [will have to use insertInstructionAt()]
			
			default:assert(false);
		}
	}
}



int main(int argc, char** argv){
	if (argc<2){
		printf("No arguments given, you give me no reason to start\n");
		exit(0);
	}
	if (doStringsMatch(argv[1],"-p")){
		printEachInstruction=true;
	} else if (doStringsMatch(argv[1],"-fpga")){
		doFPGAbootGeneration=true;
	} else if (doStringsMatch(argv[1],"-fast")){
		doGenerateFastSim=true;
	}
	if ((printEachInstruction || doFPGAbootGeneration || doGenerateFastSim) && argc<3){
		printf("No arguments given, you give me no reason to start\n");
		exit(0);
	}
	printf("Initializing Memory...\n");
	// These allocations use normal calloc, not cosmic_calloc
	globalSDL_Information.pixelState=calloc((uint32_t)globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.height,sizeof(uint8_t));
	machineState=calloc(1,sizeof(struct MachineState));
	initialMachineState=calloc(1,sizeof(struct MachineState));
	assert(globalSDL_Information.pixelState!=NULL && machineState!=NULL && initialMachineState!=NULL);
	machineState->pc=1LU<<16;
	machineState->sp=0xFFFE;
	printf("Loading Binary...\n");
	struct BinContainer binContainer=loadFileContentsAsBinContainer(argv[1+(printEachInstruction+doFPGAbootGeneration+doGenerateFastSim)]);
	printf("Integrating Binary...\n");
	uint32_t mainLabelNumber=0;
	for (uint32_t i=0;i<binContainer.len_symbols;i++){
		if (binContainer.symbols[i].type==4 && doStringsMatch(binContainer.symbols[i].name,"main")){
			mainLabelNumber=binContainer.symbols[i].label;
			break;
		}
	}
	if (mainLabelNumber==0){
		printf("Could not find \'main\' in that binary.\n");
		bye();
	}
	if (!(doFPGAbootGeneration || doGenerateFastSim)){
		setvbuf(stdout,printf_buffer,_IOFBF,120000);
	}
	InstructionBuffer allData;
	initInstructionBuffer(&allData);
	singleMergeIB(&allData,&binContainer.functions);
	//printInstructionBufferWithMessageAndNumber(&binContainer.functions,"functions:",allData.numberOfSlotsTaken);
	//printInstructionBufferWithMessageAndNumber(&binContainer.staticData,"staticData:",allData.numberOfSlotsTaken);
	destroyInstructionBuffer(&binContainer.functions);
	{
	bool doesHaveIntrinsic[0x1F]={0};
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		switch (allData.buffer[i].id){
			case I_D32U:if (!doesHaveIntrinsic[0x01]) singleMergeIB(&allData,&ib_intrinsic_back_div32_u_u);doesHaveIntrinsic[0x01]=1;break;
			case I_R32U:if (!doesHaveIntrinsic[0x02]) singleMergeIB(&allData,&ib_intrinsic_back_mod32_u_u);doesHaveIntrinsic[0x02]=1;break;
			case I_D32S:if (!doesHaveIntrinsic[0x03]) singleMergeIB(&allData,&ib_intrinsic_back_div32_s_s);doesHaveIntrinsic[0x03]=1;break;
			case I_R32S:if (!doesHaveIntrinsic[0x04]) singleMergeIB(&allData,&ib_intrinsic_back_mod32_s_s);doesHaveIntrinsic[0x04]=1;break;
			case I_LLS6:if (!doesHaveIntrinsic[0x11]) singleMergeIB(&allData,&ib_intrinsic_back_Lshift32);doesHaveIntrinsic[0x11]=1;break;
			case I_LRS6:if (!doesHaveIntrinsic[0x13]) singleMergeIB(&allData,&ib_intrinsic_back_Rshift32);doesHaveIntrinsic[0x13]=1;break;
			case I_D64U: //if (!doesHaveIntrinsic[0x05]) singleMergeIB(&allData,&ib_intrinsic_back_div64_u_u);doesHaveIntrinsic[0x05]=1;break;
			case I_R64U: //if (!doesHaveIntrinsic[0x06]) singleMergeIB(&allData,&ib_intrinsic_back_mod64_u_u);doesHaveIntrinsic[0x06]=1;break;
			case I_D64S: //if (!doesHaveIntrinsic[0x07]) singleMergeIB(&allData,&ib_intrinsic_back_div64_s_s);doesHaveIntrinsic[0x07]=1;break;
			case I_R64S: //if (!doesHaveIntrinsic[0x08]) singleMergeIB(&allData,&ib_intrinsic_back_mod64_s_s);doesHaveIntrinsic[0x08]=1;break;
			case I_LAD4: //if (!doesHaveIntrinsic[0x09]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x09]=1;break;
			case I_LAD5: //if (!doesHaveIntrinsic[0x0A]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0A]=1;break;
			case I_LSU4: //if (!doesHaveIntrinsic[0x0B]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0B]=1;break;
			case I_LSU5: //if (!doesHaveIntrinsic[0x0C]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0C]=1;break;
			case I_LMU4: //if (!doesHaveIntrinsic[0x0D]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0D]=1;break;
			case I_LMU5: //if (!doesHaveIntrinsic[0x0E]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0E]=1;break;
			case I_LDI4: //if (!doesHaveIntrinsic[0x0F]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x0F]=1;break;
			case I_LDI5: //if (!doesHaveIntrinsic[0x10]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x10]=1;break;
			case I_LRS7: //if (!doesHaveIntrinsic[0x14]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x14]=1;break;
			case I_LLS7: //if (!doesHaveIntrinsic[0x12]) singleMergeIB(&allData,&ib_intrinsic_back_);doesHaveIntrinsic[0x12]=1;break;
			printf("Backend not ready for that instruction\n");
			bye();
		}
	}
	singleMergeIB(&allData,&binContainer.staticData);
	destroyInstructionBuffer(&binContainer.staticData);
	}
	uint32_t labelCount=0;
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		enum InstructionTypeID id=allData.buffer[i].id;
		if (id==I_LABL | id==I_FCST | id==I_JJMP) labelCount++;
	}
	numberOfLabels=labelCount;
	labelNumbers=cosmic_malloc(numberOfLabels*sizeof(uint32_t));
	labelAddresses=cosmic_malloc(numberOfLabels*sizeof(uint32_t));
	labelNames=cosmic_calloc(numberOfLabels*sizeof(char*),1);
	labelExecutionCountSkipped=cosmic_calloc(numberOfLabels*sizeof(uint64_t),1);
	labelExecutionCountTopSkipped=cosmic_calloc(numberOfLabels*sizeof(uint64_t),1);
	if (printEachInstruction){
		addressToInstructionTranslationLen=allData.numberOfSlotsTaken;
		addressToInstructionTranslation=cosmic_malloc(addressToInstructionTranslationLen*sizeof(struct AddressInstructionPair));
	}
	labelCount=0;
	uint32_t storageAddress=1LU<<16;
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		InstructionSingle IS=allData.buffer[i];
		if (printEachInstruction){
			printf("addr:%08X:",storageAddress);
			printSingleInstructionOptCode(IS);
			printf("\n");
			addressToInstructionTranslation[i].address=storageAddress;
			addressToInstructionTranslation[i].IS=IS;
			addressToInstructionTranslation[i].skipThis=0==backendInstructionSize(IS);
		}
		storageAddress+=backendInstructionSize(IS);
		if (IS.id==I_LABL){
			labelNumbers[labelCount]=allData.buffer[i].arg.D.a_0;
			labelAddresses[labelCount]=storageAddress;
			//printf("%08X:%08X\n",labelNumbers[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_JJMP){
			labelNumbers[labelCount]=allData.buffer[i].arg.BBD.a_2;
			labelAddresses[labelCount]=storageAddress;
			//printf("%08X:%08X\n",labelNumbers[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_FCST){
			labelNumbers[labelCount]=allData.buffer[i].arg.BWD.a_2;
			labelAddresses[labelCount]=storageAddress;
			for (uint32_t iTemp=0;iTemp<binContainer.len_symbols;iTemp++){
				if (binContainer.symbols[iTemp].label==labelNumbers[labelCount]){
					labelNames[labelCount]=copyStringToHeapString(binContainer.symbols[iTemp].name);
					break;
				}
			}
			//printf("%08X:%08X\n",labelNumbers[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_FCEN){
			endOfExecutable=storageAddress;
		}
	}
	
	for (uint32_t i=0;i<binContainer.len_symbols;i++){
		if (binContainer.symbols[i].label==0){
			printf("Object `%s` has NULL label\n",binContainer.symbols[i].name);
			bye();
		}
		cosmic_free(binContainer.symbols[i].name);
	}
	cosmic_free(binContainer.symbols);
	
	uint32_t storageSize=storageAddress;
	uint16_t func_stack_size;
	uint8_t func_stack_initial;
	uint8_t* temporaryStorageBuffer=cosmic_calloc(storageAddress,sizeof(uint8_t));
	uint8_t* temporaryStorageBufferWalk=temporaryStorageBuffer+(1LU<<16);
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		InstructionSingle IS=allData.buffer[i];
		uint16_t intrinsicID=getIntrinsicID(IS.id);
		uint32_t symVal;
		if (intrinsicID!=0){
			symVal=getLabelAddress(intrinsicID);
		} else {
			uint8_t symbolSizeBytes=0;
			uint8_t symbolSizeWords=0;
			switch (IS.id){
				case I_FCST:
				func_stack_initial=IS.arg.BWD.a_0;
				func_stack_size=IS.arg.BWD.a_1;
				break;
				case I_JTEN:
				symVal=getLabelAddress(IS.arg.D.a_0);
				break;
				case I_SYRD:
				case I_SYDD:
				symbolSizeBytes=4;
				symbolSizeWords=2;
				break;
				case I_SYDB:
				case I_SYRB:
				symbolSizeBytes=1;
				symbolSizeWords=1;
				break;
				case I_SYRW:
				case I_SYDW:
				symbolSizeBytes=2;
				symbolSizeWords=1;
				break;
				case I_SYRQ:
				case I_SYDQ:
				symbolSizeBytes=8;
				symbolSizeWords=4;
				break;
				default:;
			}
			if (symbolSizeWords!=0){
				symbolicStack.vsl=100;
				InstructionSingle IS_temp;
				do {
					++i;
					if (i>=allData.numberOfSlotsTaken){
						printf("ByteCode corrupted, end inside symbolic\n");
						bye();
					}
					IS_temp=allData.buffer[i];
					symbolicResolutionSingle(&IS_temp);
				} while (IS_temp.id!=I_SYRE & IS_temp.id!=I_SYDE);
				if (100-symbolicStack.vsl!=symbolSizeWords){
					printf("ByteCode corrupted, symbolic result size mismatch\n");
					bye();
				}
				switch (symbolSizeBytes){
					case 1:symVal=(uint32_t)(symbolicStack.vs[99]&255);break;
					case 2:symVal=(uint32_t)symbolicStack.vs[99];break;
					case 4:symVal=((uint32_t)symbolicStack.vs[99]<<16)|symbolicStack.vs[98];break;
					//case 8:break;
					default:assert(false);
				}
			}
		}
		backendInstructionWrite(&temporaryStorageBufferWalk,symVal,func_stack_size,func_stack_initial,IS);
	}
	assert(temporaryStorageBufferWalk==(temporaryStorageBuffer+storageSize));
	uint32_t mainAddress=getLabelAddress(mainLabelNumber);
	stackLabelIndexes[0]=getLabelIndex(mainLabelNumber);
	if (doFPGAbootGeneration){
		FILE* fpga_boot_asm_file=NULL;
		fpga_boot_asm_file=fopen("boot.asm","w");
		if (fpga_boot_asm_file==NULL){
			goto failure_to_write_fpga_boot_asm_file;
		}
		fprintf(fpga_boot_asm_file,"%s","\n.org !00010000\n");
		for (uint32_t i=0;i<storageSize;i++){
			fprintf(fpga_boot_asm_file,".datab $%02X\n",(unsigned int)(temporaryStorageBuffer[i]));
		}
		if ((storageSize & 1)!=0){
			fprintf(fpga_boot_asm_file,".datab $00\n");
		}
		// storageSize should not be used beyond this point
		fprintf(fpga_boot_asm_file,"%s","\n:00000000\n");
		fprintf(fpga_boot_asm_file,"%s","LDLO %0 $00\n");
		fprintf(fpga_boot_asm_file,"%s","SPSS %0\n");
		fprintf(fpga_boot_asm_file,"%s","SPSS %0\n");
		fprintf(fpga_boot_asm_file,"%s","LDLO %0 $02\n");
		fprintf(fpga_boot_asm_file,"%s","SPSS %0\n"); // stack pointer is now reset to FFFE
		fprintf(fpga_boot_asm_file,"%s","LDLO %0 $00\n");
		fprintf(fpga_boot_asm_file,"%s","PUSH %0\n"); // set main return value to 0
		fprintf(fpga_boot_asm_file,"%s","LDFU %0 #FFFE\n");
		fprintf(fpga_boot_asm_file,"%s","PUSH %0\n"); // push stack address of main return value
		fprintf(fpga_boot_asm_file,"%s","LDLO %0 $02\n");
		fprintf(fpga_boot_asm_file,"%s","PUSH %0\n"); // push arg size
		fprintf(fpga_boot_asm_file,"%s","LDLA %A %B @00000001\n"); // get the true storage size
		fprintf(fpga_boot_asm_file,"%s","LDLO %D $02\n");
		fprintf(fpga_boot_asm_file,"%s","LDLO %C $00\n");
		fprintf(fpga_boot_asm_file,"%s","MWRW %A %C %C\n");
		fprintf(fpga_boot_asm_file,"%s","MWRW %B %D %C\n"); // write storage size to place that malloc and related functions will find it
		fprintf(fpga_boot_asm_file,"%s%04X\n","LDFU %A #",(unsigned int)(0xFFFF & mainAddress));
		fprintf(fpga_boot_asm_file,"%s%04X\n","LDFU %B #",(unsigned int)(0xFFFF & (mainAddress >> 16))); // set up main address
		fprintf(fpga_boot_asm_file,"%s","CALL %A %B\n"); // call main
		fprintf(fpga_boot_asm_file,"%s","LDLA %A %B @00000000\n");
		fprintf(fpga_boot_asm_file,"%s","AJMP %A %B\n"); // jump back to where boot starts (this is a fail safe, in general this main function shouldn't return anyway)
		fprintf(fpga_boot_asm_file,"%s",":00000001\n"); // label to mark end
		fprintf(fpga_boot_asm_file,"%s",".datab $00\n"); // an extra byte. probably not needed, but it's here anyway
		if (fclose(fpga_boot_asm_file)!=0){
			failure_to_write_fpga_boot_asm_file:;
			printf("Failed to write \"boot.asm\" for the fpga\n");
			bye();
		}
		printf("Wrote \"boot.asm\" for the fpga\n");
	} else {
		for (uint32_t i=1LU<<16;i<storageSize;i++){
			initMemWrite(i,temporaryStorageBuffer[i]);
		}
		cosmic_free(temporaryStorageBuffer);
		destroyInstructionBuffer(&allData);
		// now insert the arguments
		int argcSim=(argc-(printEachInstruction+doFPGAbootGeneration+doGenerateFastSim))-1;
		int w=0;
		for (int i=1+(printEachInstruction+doFPGAbootGeneration+doGenerateFastSim);i<argc;i++){
			for (int ii=0;argv[i][ii];ii++){
				initMemWrite(storageSize+ w++,argv[i][ii]);
			}
			initMemWrite(storageSize+ w++,0);
			if ((w&1)!=0) initMemWrite(storageSize+ w++,0);
		}
		uint32_t prevStorageSize0=storageSize;
		storageSize+=w;
		storageSize+=storageSize&1;
		uint32_t argvSim=storageSize;
		int w2=0;
		for (int i=1+(printEachInstruction+doFPGAbootGeneration+doGenerateFastSim);i<argc;i++){
			initMemWrite(storageSize+0,0xFF&((prevStorageSize0+w2)>> 0));
			initMemWrite(storageSize+1,0xFF&((prevStorageSize0+w2)>> 8));
			initMemWrite(storageSize+2,0xFF&((prevStorageSize0+w2)>>16));
			initMemWrite(storageSize+3,0xFF&((prevStorageSize0+w2)>>24));
			storageSize+=4;
			for (int ii=0;argv[i][ii];ii++){w2++;}
			w2++;
			if ((w2&1)!=0) w2++;
		}
		// now set up low level requirments for running
		initPush(0xFFFFu&(argvSim>>16));
		initPush(0xFFFFu&(argvSim));
		initPush(0xFFFFu&(argcSim));
		initPush(0xFFFEu);// ret write address
		initPush(0x0008);// arg size
		initPush(0x0000);//%0
		initPush(0x0000);//%1
		initPush(0x0401);initPush(0x0000);//ret address dword
		machineState->reg[0]=machineState->sp; //sp->%0
		
		// write end storageSize so malloc and friends can pick it up
		initMemWrite(0,0xFF&(storageSize>> 0));
		initMemWrite(1,0xFF&(storageSize>> 8));
		initMemWrite(2,0xFF&(storageSize>>16));
		initMemWrite(3,0xFF&(storageSize>>24));
		
		machineState->pc=mainAddress;// set pc to main address
		createInitialMachineState();
		
		if (doGenerateFastSim){
			printf("Generating Fast Sim...\n");
			FILE* fastSimFile;
			fastSimFile=fopen("FastSimInitGen.c","w");
			if (fastSimFile==NULL){
				printf("Failed to open \"FastSimInitGen.c\"\n");
				bye();
			}
			fprintf(fastSimFile,"%s","#include <stdint.h>\n");
			fprintf(fastSimFile,"%s","void memInit(uint8_t* mem){");
			for (uint32_t i0=0;i0<(1LLU<<26);i0++){
				uint8_t b=initMemRead(i0);
				if (b!=0){
					fprintf(fastSimFile,"mem[%lu]=%u;\n",(unsigned long)i0,(unsigned)b);
				}
			}
			fprintf(fastSimFile,"%s","}\n");
			if (fclose(fastSimFile)!=0){
				printf("Failed to finish writing to \"FastSimInitGen.c\"\n");
				bye();
			}
			fastSimFile=fopen("FastSimGen.c","w");
			if (fastSimFile==NULL){
				printf("Failed to open \"FastSimGen.c\"\n");
				bye();
			}
			fprintf(fastSimFile,"%s","#include <stdint.h>\n");
			fprintf(fastSimFile,"%s","#include <stddef.h>\n");
			fprintf(fastSimFile,"%s","#include <stdlib.h>\n");
			fprintf(fastSimFile,"%s","#include <stdio.h>\n");
			fprintf(fastSimFile,"%s","struct MachineState{uint8_t* mem;uint32_t pc;uint16_t sp;uint16_t reg[16];};\n");
			fprintf(fastSimFile,"%s","extern void bye();\n");
			fprintf(fastSimFile,"%s","extern uint8_t readByte(uint32_t a);\n");
			fprintf(fastSimFile,"%s","extern void writeByte(uint32_t a,uint8_t b);\n");
			fprintf(fastSimFile,"%s","extern void writeWord(uint32_t a,uint16_t w);\n");
			fprintf(fastSimFile,"%s","extern uint16_t readWord(uint32_t a);\n");
			fprintf(fastSimFile,"%s","extern void sdl_pull_event();\n");
			fprintf(fastSimFile,"%s","void run(struct MachineState ms){\n");
			fprintf(fastSimFile,"%s","uint32_t tp0,tp1,tp2,tp3;\n");
			fprintf(fastSimFile,"%s","uint32_t upperExBound;\n");
			for (unsigned i0=0;i0<16;i0++){
				fprintf(fastSimFile,"ms.reg[%u]=%u;\n",(unsigned)i0,(unsigned)machineState->reg[i0]);
			}
			fprintf(fastSimFile,"ms.sp=%u;\n",(unsigned)machineState->sp);
			fprintf(fastSimFile,"upperExBound=%lu;\n",(unsigned long)endOfExecutable);
			fprintf(fastSimFile,"ms.pc=%lu;\n",(unsigned long)machineState->pc);
			
			fprintf(fastSimFile,"LJUMP:;\n");
			//fprintf(fastSimFile,"printf(\"$%%08X\\n\",(unsigned)ms.pc);");
			//fprintf(fastSimFile,"stateDump(ms);\n");
			//fprintf(fastSimFile,"sdl_pull_event();\n");
			fprintf(fastSimFile,"switch (ms.pc){\n");
			for (uint32_t i1=0;i1<numberOfLabels;i1++){
				if ((labelAddresses[i1]&1)==0 && labelAddresses[i1]<=endOfExecutable) fprintf(fastSimFile,"case %lu:goto LN%08X;\n",(unsigned long)labelAddresses[i1],(unsigned)labelNumbers[i1]);
			}
			for (uint32_t i0=1LU<<16;i0<(1LLU<<26) && i0<=endOfExecutable;i0+=2){
				uint8_t b0=initMemRead(i0+0);
				uint8_t b1=initMemRead(i0+1);
				uint16_t w=(b1<<8)|b0;
				uint8_t type=((w&0xF000u)!=0xF000u)?((w>>12)&0xFu):(((w>>8)&0xFu)|0x10u);
				bool dup=false;
				if (type==0x1A){
					for (uint32_t i1=0;i1<numberOfLabels;i1++){
						if ((labelAddresses[i1]&1)==0 && labelAddresses[i1]<=endOfExecutable){
							if ((i0+2u)==labelAddresses[i1]){
								dup=true;
								break;
							}
						}
					}
					if (!dup){
						fprintf(fastSimFile,"case %u:goto AR%08X;\n",(unsigned)(i0+2u),(unsigned)(i0+2u));
					}
				}
			}
			fprintf(fastSimFile,"default:printf(\"Error: bad jump address [it was not a label or after a CALL] {%%08X}\\n\",ms.pc);bye();return;\n");
			fprintf(fastSimFile,"}\n");
			fprintf(fastSimFile,"ms.pc=1LU<<16;\n");
			
			for (uint32_t i0=1LU<<16;i0<(1LLU<<26) && i0<=endOfExecutable;i0+=2){
				for (uint32_t i1=0;i1<numberOfLabels;i1++){
					if (labelAddresses[i1]==i0){
						fprintf(fastSimFile,"LN%08X:;\n",(unsigned)labelNumbers[i1]);
					}
				}
				fprintf(fastSimFile,"ms.pc+=2;");
				uint8_t b0=initMemRead(i0+0);
				uint8_t b1=initMemRead(i0+1);
				uint16_t w=(b1<<8)|b0;
				uint8_t type=((w&0xF000u)!=0xF000u)?((w>>12)&0xFu):(((w>>8)&0xFu)|0x10u);
				uint8_t r0=(w>>0)&0xFu;
				uint8_t r1=(w>>4)&0xFu;
				uint8_t r2=(w>>8)&0xFu;
				uint8_t imm=(w>>4)&0xFFu;
				//fprintf(fastSimFile,"printf(\"#%02X @%08X\\n\");",(unsigned)type,(unsigned)i0);
				switch (type){
					case 0x00:
					fprintf(fastSimFile,"ms.reg[%u]=%u;\n",(unsigned)r0,(unsigned)imm);
					break;
					case 0x01:
					fprintf(fastSimFile,"ms.reg[%u]&=0xFF;ms.reg[%u]|=((unsigned)%u<<8)& 0xFF00u;\n",(unsigned)r0,(unsigned)r0,(unsigned)imm);
					break;
					case 0x02:
					fprintf(fastSimFile,"tp0=ms.reg[1];");
					fprintf(fastSimFile,"ms.reg[%u] =(unsigned)ms.mem[(unsigned)(tp0+%u*2+0)& 0xFFFFu]<<0;",(unsigned)r0,(unsigned)imm);
					fprintf(fastSimFile,"ms.reg[%u]|=(unsigned)ms.mem[(unsigned)(tp0+%u*2+1)& 0xFFFFu]<<8;",(unsigned)r0,(unsigned)imm);
					fprintf(fastSimFile,"tp0=0;\n");
					break;
					case 0x03:
					fprintf(fastSimFile,"ms.mem[(unsigned)(ms.reg[1]+%u*2+0)& 0xFFFFu]=(unsigned)ms.reg[%u]>>0;"  ,(unsigned)imm,(unsigned)r0);
					fprintf(fastSimFile,"ms.mem[(unsigned)(ms.reg[1]+%u*2+1)& 0xFFFFu]=(unsigned)ms.reg[%u]>>8;\n",(unsigned)imm,(unsigned)r0);
					break;
					case 0x04:
					fprintf(fastSimFile,"ms.reg[%u]=ms.reg[%u] & ms.reg[%u];\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x05:
					fprintf(fastSimFile,"ms.reg[%u]=ms.reg[%u] | ms.reg[%u];\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x06:
					fprintf(fastSimFile,"ms.reg[%u]=ms.reg[%u] ^ ms.reg[%u];\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x07:
					fprintf(fastSimFile,"tp0=(uint32_t)ms.reg[%u] + (uint32_t)ms.reg[%u] + (uint32_t)(~ms.reg[%u]&0xFFFF);",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					fprintf(fastSimFile,"ms.reg[%u]=(uint16_t)tp0;ms.reg[%u]=(tp0&0x00030000)!=0;tp0=0;\n",(unsigned)r1,(unsigned)r0);
					break;
					case 0x08:
					fprintf(fastSimFile,"tp1=((uint32_t)ms.reg[%u]<<16)|ms.reg[%u];ms.reg[%u]=((tp1^(tp1&0x3FFFFFE))!=0)?readWord(tp1):(ms.mem[(tp1&0x3FFFFFE)+1]<<8)|(ms.mem[(tp1&0x3FFFFFE)+0]<<0);tp1=0;\n",(unsigned)r2,(unsigned)r1,(unsigned)r0);
					break;
					case 0x09:
					fprintf(fastSimFile,"tp1=((uint32_t)ms.reg[%u]<<16)|ms.reg[%u];((tp1^(tp1&0x3FFFFFE))!=0)?writeWord(tp1,ms.reg[%u]):((ms.mem[(tp1&0x3FFFFFE)+1]=(unsigned)ms.reg[%u]>>8),(ms.mem[(tp1&0x3FFFFFE)+0]=(unsigned)ms.reg[%u]>>0));tp1=0;\n",(unsigned)r2,(unsigned)r1,(unsigned)r0,(unsigned)r0,(unsigned)r0);
					break;
					case 0x0A:
					fprintf(fastSimFile,"ms.reg[%u]=ms.reg[%u] + ms.reg[%u];\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x0B:
					fprintf(fastSimFile,"tp0=(uint32_t)ms.reg[%u] + (uint32_t)ms.reg[%u] + (uint32_t)ms.reg[15];",(unsigned)r1,(unsigned)r2);
					fprintf(fastSimFile,"ms.reg[%u]=(uint16_t)tp0;ms.reg[15]=(tp0&0x00030000)!=0;tp0=0;\n",(unsigned)r0);
					break;
					case 0x0C:
					fprintf(fastSimFile,"ms.reg[%u]=(uint32_t)ms.reg[%u]+(~(uint32_t)ms.reg[%u]&0xFFFFu)+(uint32_t)1;\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x0D:
					fprintf(fastSimFile,"ms.reg[%u]=(((uint32_t)ms.reg[%u]+(~(uint32_t)ms.reg[%u]&0xFFFFu)+(uint32_t)1)&0x00010000)!=0;\n",(unsigned)r0,(unsigned)r1,(unsigned)r2);
					break;
					case 0x0E:
					fprintf(fastSimFile,"if (ms.reg[%u]==0u){ms.pc=((uint32_t)ms.reg[%u]<<16)|((uint32_t)ms.reg[%u]<<0);goto LJUMP;}\n",(unsigned)r2,(unsigned)r1,(unsigned)r0);
					break;
					case 0x10:
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=(unsigned)ms.reg[%u]>>0;ms.mem[(ms.sp+1)&0xFFFF]=(unsigned)ms.reg[%u]>>8;\n",(unsigned)r0,(unsigned)r0);
					break;
					case 0x11:
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=(unsigned)ms.reg[%u]>>0;ms.mem[(ms.sp+1)&0xFFFF]=(unsigned)ms.reg[%u]>>8;"  ,(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=(unsigned)ms.reg[%u]>>0;ms.mem[(ms.sp+1)&0xFFFF]=(unsigned)ms.reg[%u]>>8;\n",(unsigned)r1,(unsigned)r1);
					break;
					case 0x12:
					fprintf(fastSimFile,"ms.reg[%u]=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];ms.reg[%u]|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;\n",(unsigned)r0,(unsigned)r0);
					break;
					case 0x13:
					fprintf(fastSimFile,"ms.reg[%u]=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];ms.reg[%u]|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;"  ,(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.reg[%u]=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];ms.reg[%u]|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;\n",(unsigned)r1,(unsigned)r1);
					break;
					case 0x14:
					fprintf(fastSimFile,"ms.reg[%u]=ms.reg[%u];\n",(unsigned)r0,(unsigned)r1);
					break;
					case 0x15:
					fprintf(fastSimFile,"ms.reg[%u]=((ms.reg[%u]<<8)&0xFF00u)|((ms.reg[%u]>>8)&0x00FFu);\n",(unsigned)r0,(unsigned)r1,(unsigned)r1);
					break;
					case 0x16:
					fprintf(fastSimFile,"ms.reg[%u]=(ms.reg[%u]>>1)&0x7FFFu;\n",(unsigned)r0,(unsigned)r1);
					break;
					case 0x17:
					fprintf(fastSimFile,"ms.reg[%u]*=ms.reg[%u];\n",(unsigned)r0,(unsigned)r1);
					break;
					case 0x18:
					fprintf(fastSimFile,"tp0=(((uint32_t)ms.reg[%u]<<16) | ((uint32_t)ms.reg[%u]<<0)) * (((uint32_t)ms.reg[14]<<16)|((uint32_t)ms.reg[13]<<0));ms.reg[13]=tp0;ms.reg[14]=tp0>>16;tp0=0;\n",(unsigned)r1,(unsigned)r0);
					break;
					case 0x19:
					fprintf(fastSimFile,"tp0=ms.reg[%u] / ms.reg[%u];ms.reg[%u]=ms.reg[%u] %s ms.reg[%u];ms.reg[%u]=tp0;tp0=0;\n",(unsigned)r0,(unsigned)r1,(unsigned)r1,(unsigned)r0,"%",(unsigned)r1,(unsigned)r0);
					break;
					case 0x1A:
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=(unsigned)ms.reg[0]>>0;ms.mem[(ms.sp+1)&0xFFFF]=(unsigned)ms.reg[0]>>8;",(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=(unsigned)ms.reg[1]>>0;ms.mem[(ms.sp+1)&0xFFFF]=(unsigned)ms.reg[1]>>8;",(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=ms.pc>>16;ms.mem[(ms.sp+1)&0xFFFF]=ms.pc>>24;",(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.sp-=2;ms.mem[(ms.sp+0)&0xFFFF]=ms.pc>> 0;ms.mem[(ms.sp+1)&0xFFFF]=ms.pc>> 8;",(unsigned)r0,(unsigned)r0);
					fprintf(fastSimFile,"ms.reg[0]=ms.sp;\n",(unsigned)r0,(unsigned)r1);
					fprintf(fastSimFile,"ms.pc=((uint32_t)ms.reg[%u]<<16)|((uint32_t)ms.reg[%u]<<0);goto LJUMP;",(unsigned)r1,(unsigned)r0);
					fprintf(fastSimFile,"AR%08X:;\n",(unsigned)(i0+2u));
					break;
					case 0x1B:
					fprintf(fastSimFile,"ms.sp=ms.reg[0]&0xFFFE;");
					fprintf(fastSimFile,"tp0=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];tp0|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;");
					fprintf(fastSimFile,"tp1=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];tp1|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;");
					fprintf(fastSimFile,"ms.reg[1]=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];ms.reg[1]|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;");
					fprintf(fastSimFile,"ms.reg[0]=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];ms.reg[0]|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;");
					fprintf(fastSimFile,"tp2=(unsigned)ms.mem[(ms.sp+0)&0xFFFF];tp2|=(unsigned)ms.mem[(ms.sp+1)&0xFFFF]<<8;ms.sp+=2;");
					fprintf(fastSimFile,"tp0=tp0&0xFFFF;tp1=tp1&0xFFFF;tp2=tp2&0xFFFF;");
					fprintf(fastSimFile,"ms.sp=(ms.sp+tp2)&0xFFFE;ms.pc=(tp1<<16)|tp0;tp0=0;tp1=0;tp2=0;\n");
					fprintf(fastSimFile,"sdl_pull_event();\n");
					fprintf(fastSimFile,"goto LJUMP;\n");
					break;
					case 0x1C:
					fprintf(fastSimFile,"tp1=((uint32_t)ms.reg[%u]<<16)|ms.reg[13];ms.reg[%u]=(((tp1^(tp1&0x3FFFFFF))!=0)?readByte(tp1):(ms.mem[tp1&0x3FFFFFF]<<0))&0xFF;tp1=0;\n",(unsigned)r1,(unsigned)r0);
					break;
					case 0x1D:
					fprintf(fastSimFile,"tp1=((uint32_t)ms.reg[%u]<<16)|ms.reg[13];((tp1^(tp1&0x3FFFFFF))!=0)?writeByte(tp1,ms.reg[%u]&0xFF):(ms.mem[tp1&0x3FFFFFF]=ms.reg[%u]&0xFF);tp1=0;\n",(unsigned)r1,(unsigned)r0,(unsigned)r0);
					break;
					case 0x1E:
					fprintf(fastSimFile,"ms.pc=((uint32_t)ms.reg[%u]<<16)|ms.reg[%u];goto LJUMP;\n",(unsigned)r1,(unsigned)r0);
					break;
					case 0x1F:
					fprintf(fastSimFile,"ms.sp-=ms.reg[%u];ms.sp=ms.sp&0xFFFE;ms.reg[%u]=ms.sp;\n",(unsigned)r0,(unsigned)r0);
					break;
					case 0x0F:
					default:assert(false);
				}
				//fprintf(fastSimFile,"stateDump(ms);\n");
			}
			fprintf(fastSimFile,"printf(\"Error: execution ran off the end of executable\");bye();return;}\n");
			if (fclose(fastSimFile)!=0){
				printf("Failed to finish writing to \"FastSimGen.c\"\n");
				bye();
			}
			printf("Finished Generating Fast Sim\n");
		} else {
			printf("Starting SDL...\n");
			initializeSDL();
			printf("Running...\n");
			if (!printEachInstruction){
				printf("Call Stack:\n|\n%5d:main()\n",0);
			}
			fullExecute();
			
			
			printf("Finished Normally. Return Value is `%d`\n",terminationValue);
		}
	}
	bye();
}





