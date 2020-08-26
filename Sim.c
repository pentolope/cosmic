
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

noreturn void bye(void){
	printf("\nTotal instructions executed:%llu\n",(unsigned long long)instructionExecutionCount);
	
	// these were allocated using normal calloc, not cosmic_calloc
	free(globalSDL_Information.pixelState);
	free(machineState);
	free(initialMachineState);
	
	if (globalSDL_Information.isSLD_initialized){
		if (globalSDL_Information.hasPixelBeenWritten){
			if (globalSDL_Information.event.type != SDL_QUIT){
				printf("Waiting for SDL_QUIT event...\n");
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
	printf("\nfopen(\"%s\",\"%s\")->",filePath,fileMode);
	FILE* inputFile = fopen(filePath,fileMode);
	if (inputFile==NULL){
		printf("NULL\n");
		return;
	}
	printf("VALID\n");
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
	printf("\nfclose()\n");
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



void singleExecute(){
	instructionExecutionCount++;
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

uint32_t numberOfLabels;
uint32_t* labelNumbers;
uint32_t* labelAddresses;



uint32_t getLabelAddress(uint32_t labelToSearch){
	for (uint32_t i=0;i<numberOfLabels;i++){
		if (labelNumbers[i]==labelToSearch) return labelAddresses[i];
	}
	printf("Unresolved Label %08X\n",labelToSearch);
	bye();
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
	}
	if (printEachInstruction && argc<3){
		printf("No arguments given, you give me no reason to start\n");
		exit(0);
	}
	printf("Initializing Memory...\n");
	// These allocations use normal calloc, not cosmic_calloc
	globalSDL_Information.pixelState=calloc((uint32_t)globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.height,sizeof(uint8_t));
	machineState=calloc(1,sizeof(struct MachineState));
	initialMachineState=calloc(1,sizeof(struct MachineState));
	machineState->pc=1LU<<16;
	machineState->sp=0xFFFE;
	printf("Loading Binary...\n");
	struct BinContainer binContainer=loadFileContentsAsBinContainer(argv[1+printEachInstruction]);
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
	
	for (uint32_t i=0;i<binContainer.len_symbols;i++){
		if (binContainer.symbols[i].label==0){
			printf("Object `%s` has NULL label\n",binContainer.symbols[i].name);
			bye();
		}
		cosmic_free(binContainer.symbols[i].name);
	}
	cosmic_free(binContainer.symbols);
	
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
			//printf("%08X:%08X\n",labelNumbers[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_FCEN){
			endOfExecutable=storageAddress;
		}
	}
	uint32_t storageSize=storageAddress;
	{
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
			/*
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
				{
				InstructionSingle IS_temp0=allData.buffer[i+1];
				InstructionSingle IS_temp1=allData.buffer[i+2];
				if (IS_temp0.id!=I_SYCL | (IS_temp1.id!=I_SYRE & IS_temp1.id!=I_SYDE)){
					printf("I don't wanna do that yet...");
					bye();
				}
				symVal=getLabelAddress(IS_temp0.arg.D.a_0);
				}
				break;
				case I_SYDB:
				case I_SYRB:
				case I_SYRW:
				case I_SYDW:
				case I_SYRQ:
				case I_SYDQ:
				printf("I don't wanna do that yet...");
				bye();
				
				
				default:;
			}
			*/
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
	assert(temporaryStorageBufferWalk==(temporaryStorageBuffer+storageAddress));
	uint32_t mainAddress=getLabelAddress(mainLabelNumber);
	cosmic_free(labelNumbers);
	cosmic_free(labelAddresses);
	for (uint32_t i=1LU<<16;i<storageSize;i++){
		initMemWrite(i,temporaryStorageBuffer[i]);
	}
	cosmic_free(temporaryStorageBuffer);
	destroyInstructionBuffer(&allData);
	// now insert the arguments
	int argcSim=(argc-printEachInstruction)-1;
	int w=0;
	for (int i=1+printEachInstruction;i<argc;i++){
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
	for (int i=1+printEachInstruction;i<argc;i++){
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
	}
	printf("Starting SDL...\n");
	initializeSDL();
	printf("Running...\n");
	fullExecute();
	
	
	
	printf("Finished Normally. Return Value is `%d`\n",terminationValue);
	bye();
}





