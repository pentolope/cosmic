
#define INCLUDE_BACKEND
#include "StatementWalk.c"
#include "FinalizersAndFileIO.c"
#include "PrintInstructionBuffer.c"
#define IS_BUILDING_RUN
#include "TargetInstructions/GeneratedInstructionInitialization.c"
#undef IS_BUILDING_RUN

#include <stddef.h>
#include <time.h>
#include <stdnoreturn.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <windows.h>


struct GlobalSDL_Information{
	SDL_Window* window;
	SDL_Surface* surfaceOnWindow;
	SDL_Event event;
	const struct WindowSize{
		uint16_t width;
		uint16_t height;
		uint16_t sizeOfUnit;
	} windowSize;
	uint8_t pixelState[320u*200u];
	uint16_t eventPullCountdown;
	bool isSLD_initialized;
} globalSDL_Information = {.windowSize = {320u,200u,3u},.eventPullCountdown=1};

uint64_t instructionExecutionCount=0;
bool printEachInstruction=false;
bool doCacheSim=false;
bool isTerminationTriggered=false;
uint16_t terminationValue=0;
uint32_t endOfExecutable;

struct MemSeg{
	uint8_t seg[1LU<<11];
};

struct MachineState{
	uint32_t pc;
	uint16_t reg[16];
	uint16_t sp;
	
	uint8_t walk_cache;
	uint16_t back_cache[16];
	struct MemSeg cache[1LU<<5];
	uint8_t in_cache[1LU<<15];
	struct MemSeg mainMem[1LU<<15];
} machineState={.pc=1LU<<16,.sp=0xFFFE};


noreturn void bye(void){
	printf("\nTotal instructions executed:%llu\n",(unsigned long long)instructionExecutionCount);
	if (globalSDL_Information.isSLD_initialized){
		if (globalSDL_Information.event.type != SDL_QUIT){
			printf("Waiting for quit event from SDL...\n");
			do {
				Sleep(10);
				SDL_PollEvent(&globalSDL_Information.event);
			} while (globalSDL_Information.event.type != SDL_QUIT);
		}
		SDL_Quit();
	}
	printf("Exiting...\n");
	exit(0);
}

void writePixelToScreenWithSDL(uint16_t a, uint8_t c){
	if (a<320u*200u && globalSDL_Information.pixelState[a]!=c){
		globalSDL_Information.pixelState[a]=c;
		const uint8_t r=(255u/7u)*(7u&(unsigned)c/32u)+1u;
		const uint8_t g=(255u/7u)*(7u&(unsigned)c/4u)+1u;
		const uint8_t b=(255u/3u)*(3u&(unsigned)c);
		const SDL_Rect rect={.x=(a%320u)*globalSDL_Information.windowSize.sizeOfUnit+1,.y=((a/320u)%200u)*globalSDL_Information.windowSize.sizeOfUnit+1,.h=globalSDL_Information.windowSize.sizeOfUnit,.w=globalSDL_Information.windowSize.sizeOfUnit};
		SDL_FillRect(globalSDL_Information.surfaceOnWindow,&rect,SDL_MapRGB(globalSDL_Information.surfaceOnWindow->format, r, g, b));
		
		SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&rect,1); // updating is slow... not much I can do about it though, it has to update after every pixel change
	}
}

void initializeSDL(){
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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
	printf("State Dump Start:\n");
	printf("  pc:%08X\n",machineState.pc);
	printf("  sp:%04X\n",machineState.sp);
	for (int i=0;i<16;i++){
		printf("  %%%01X:%04X\n",i,machineState.reg[i]);
	}
	printf("State Dump End\n");
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
	machineState.mainMem[aU_].seg[aL]=b;
}

void initPush(unsigned int w){
	w&=0xFFFFu;
	machineState.sp-=2;
	uint32_t a=machineState.sp;
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	machineState.mainMem[aU_].seg[aL+0]=(uint8_t)((unsigned)w>>0);
	machineState.mainMem[aU_].seg[aL+1]=(uint8_t)((unsigned)w>>8);
}

/*
all special accesses are performed using volatile byte operations (unless otherwise specified). ranges are given as inclusive ranges

GPU memory may be written to at 0x04000000->0x0400FFFF

Halt/Exit is achieved by WORD writing to 0x04010000 , the value is the return value.
If main terminates, it will set the pc to 0x04010000, at which point the value at 0x0000FFFE will be interpreted as the return value.

Opening a file may be done by writing mode to 0x04010001 , then writing path to 0x04020002 (null termination, simply write each byte to same address)
  if successful, the reading at the location 0x04020003 will hold the value 1, otherwise it will hold the value 0
  closing a file may be done by writing a 0 to 0x04020003
  only one file may be opened at a time
  Once a file is opened, it's contents are in the range 0x05000000->0x05FFFFFF and the length is stored in 0x04010006->0x04010009

*/


void handleCacheMiss(){
	
}


uint16_t readWord(uint32_t a){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		printf("Mem fault:word read misalligned:%08X\n",a);
		stateDump();
		bye();
	}
	if (aUD!=0){
		printf("Mem fault:word read out of bounds:%08X\n",a);
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		uint16_t w=machineState.mainMem[aU_].seg[aL+0];
		w|=(unsigned)machineState.mainMem[aU_].seg[aL+1]<<8;
		return w;
	}
}
uint8_t readByte(uint32_t a){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if (aUD!=0){
		printf("Mem fault:byte read out of bounds:%08X\n",a);
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		return machineState.mainMem[aU_].seg[aL];
	}
}
void writeWord(uint32_t a,uint16_t w){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		printf("Mem fault:word write misalligned:%08X\n",a);
		stateDump();
		bye();
	}
	if (aUD!=0){
		if (a==0x04010000){
			isTerminationTriggered=true;
			terminationValue=w;
			return;
		}
		printf("Mem fault:word write out of bounds:%08X\n",a);
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		machineState.mainMem[aU_].seg[aL+0]=(uint8_t)((unsigned)w>>0);
		machineState.mainMem[aU_].seg[aL+1]=(uint8_t)((unsigned)w>>8);
		return;
	}
}
void writeByte(uint32_t a,uint8_t b){
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if (aUD!=0){
		if (a>=0x04000000 && a<=0x0400FFFF){
			writePixelToScreenWithSDL(a-0x04000000,b);
			return;
		}
		printf("Mem fault:byte write out of bounds:%08X\n",a);
		stateDump();
		bye();
	}
	if (doCacheSim){
		printf("cache sim not ready\n");
		bye();
	} else {
		machineState.mainMem[aU_].seg[aL]=b;
		return;
	}
}
void push(uint16_t w){
	machineState.sp-=2;
	writeWord(machineState.sp,w);
}
uint16_t pop(){
	uint16_t r=readWord(machineState.sp);
	machineState.sp+=2;
	return r;
}



void singleExecute(){
	instructionExecutionCount++;
	unsigned instr=readWord(machineState.pc);
	uint8_t type=((instr&0xF000u)!=0xF000u)?((instr>>12)&0xFu):(((instr>>8)&0xFu)|0x10u);
	uint8_t r0=(instr>>0)&0xFu;
	uint8_t r1=(instr>>4)&0xFu;
	uint8_t r2=(instr>>8)&0xFu;
	uint8_t imm=(instr>>4)&0xFFu;
	uint32_t temp;
	if (printEachInstruction) printf("Exec:%08X->%04X[%02X]\n",machineState.pc,instr,type);
	machineState.pc+=2;
	switch (type){
		case 0x00:machineState.reg[r0]=imm;break;
		case 0x01:machineState.reg[r0]|=((unsigned)imm<<8)&0xFF00u;break;
		case 0x02:machineState.reg[r0]=readWord(machineState.reg[1]+2*imm);break;
		case 0x03:writeWord(machineState.reg[1]+2*imm,machineState.reg[r0]);break;
		case 0x04:machineState.reg[r0]=machineState.reg[r1]&machineState.reg[r2];break;
		case 0x05:machineState.reg[r0]=machineState.reg[r1]|machineState.reg[r2];break;
		case 0x06:machineState.reg[r0]=machineState.reg[r1]^machineState.reg[r2];break;
		case 0x07:
		temp=(uint32_t)machineState.reg[r0]+(uint32_t)machineState.reg[r1]+(uint32_t)(~machineState.reg[r2]&0xFFFFu);
		machineState.reg[r1]=(uint16_t)temp;
		machineState.reg[r0]=(temp&0x00030000)!=0;
		break;
		case 0x08:machineState.reg[r0]=readWord(((uint32_t)machineState.reg[r2]<<16)|machineState.reg[r1]);break;
		case 0x09:writeWord(((uint32_t)machineState.reg[r2]<<16)|machineState.reg[r1],machineState.reg[r0]);break;
		case 0x0A:machineState.reg[r0]=machineState.reg[r1]+machineState.reg[r2];break;
		case 0x0B:
		temp=(uint32_t)machineState.reg[r1]+(uint32_t)machineState.reg[r2];
		machineState.reg[r0]=(uint16_t)temp;
		machineState.reg[15]=(temp&0x00010000)!=0;
		break;
		case 0x0C:
		temp=(uint32_t)machineState.reg[r1]+~((uint32_t)machineState.reg[r2])+(uint32_t)1;
		machineState.reg[r0]=(uint16_t)temp;
		break;
		case 0x0D:
		temp=(uint32_t)machineState.reg[r1]+~((uint32_t)machineState.reg[r2])+(uint32_t)1;
		machineState.reg[r0]=(temp&0x00010000)!=0;
		break;
		case 0x0E:
		if (machineState.reg[r2]==0){
			machineState.pc=((uint32_t)machineState.reg[r0])|((uint32_t)machineState.reg[r1]<<16);
		}
		break;
		case 0x10:push(machineState.reg[r0]);break;
		case 0x11:push(machineState.reg[r0]);push(machineState.reg[r1]);break;
		case 0x12:machineState.reg[r0]=pop();break;
		case 0x13:machineState.reg[r0]=pop();machineState.reg[r1]=pop();break;
		case 0x14:machineState.reg[r0]=machineState.reg[r1];break;
		case 0x15:machineState.reg[r0]=(((unsigned)machineState.reg[r1]<<8)&0xFF00u)|(((unsigned)machineState.reg[r1]>>8)&0x00FFu);break;
		case 0x16:machineState.reg[r0]=((unsigned)machineState.reg[r1]>>1)&0x7FFFu;break;
		case 0x17:machineState.reg[r0]*=machineState.reg[r1];break;
		case 0x18:
		temp=((uint32_t)machineState.reg[15]<<16)|(uint32_t)machineState.reg[14];
		temp*=((uint32_t)machineState.reg[r1]<<16)|(uint32_t)machineState.reg[r0];
		machineState.reg[14]=(uint16_t)temp;
		machineState.reg[15]=(uint16_t)(temp>>16);
		break;
		case 0x19:
		temp=machineState.reg[r0]/machineState.reg[r1];
		machineState.reg[r1]=machineState.reg[r0]%machineState.reg[r1];
		machineState.reg[r0]=temp;
		break;
		case 0x1A:
		push(machineState.reg[0]);
		push(machineState.reg[1]);
		push((machineState.pc>>16)&0xFFFFu);
		push(machineState.pc&0xFFFFu);
		machineState.reg[0]=machineState.sp;
		machineState.pc=(uint32_t)machineState.reg[r0]|((uint32_t)machineState.reg[r1]<<16);
		break;
		case 0x1B:
		machineState.sp=machineState.reg[0];
		temp=pop();
		machineState.pc=((uint32_t)pop()<<16)|temp;
		machineState.reg[1]=pop();
		machineState.reg[0]=pop();
		temp=pop();
		machineState.sp+=temp;
		break;
		case 0x1C:machineState.reg[r0]=readByte(((uint32_t)machineState.reg[r1]<<16)|machineState.reg[14]);break;
		case 0x1D:writeByte(((uint32_t)machineState.reg[r1]<<16)|machineState.reg[14],machineState.reg[r0]&0x00FFu);break;
		case 0x1E:machineState.pc=((uint32_t)machineState.reg[r0])|((uint32_t)machineState.reg[r1]<<16);break;
		case 0x1F:machineState.sp-=machineState.reg[r0];machineState.reg[r0]=machineState.sp;break;
		
		
		case 0x0F:
		default:printf("Instruction corrupted (%d)\n",type);bye();
	}
}


void fullExecute(){
	while (!isTerminationTriggered){
		if (globalSDL_Information.eventPullCountdown--==0){
			SDL_PollEvent(&globalSDL_Information.event); // to keep Windows from complaining
			if (globalSDL_Information.event.type == SDL_QUIT){
				printf("Premature Exit Initiated...\n");
				bye();
			}
			globalSDL_Information.eventPullCountdown=20;
		}
		if (machineState.pc==0x04010000){
			isTerminationTriggered=true;
			terminationValue=readWord(0x0000FFFE);
			return;
		}
		if (machineState.sp>machineState.reg[1]){
			//printf("Over %08X:%08X\n",(unsigned int)machineState.pc,(unsigned int)instructionExecutionCount);
		}
		if (machineState.pc<0x00010000 | machineState.pc>=endOfExecutable){
			printf("Error:Program Counter out of bounds %08X:%08X\n",(unsigned int)machineState.pc,(unsigned int)instructionExecutionCount);
			bye();
		}
		singleExecute();
		if (printEachInstruction) stateDump();
	}
}







uint32_t getLabelAddress(uint32_t labelToSearch,uint32_t labelTotal,uint32_t* labelNames,uint32_t* labelAddresses){
	for (uint32_t i=0;i<labelTotal;i++){
		if (labelNames[i]==labelToSearch) return labelAddresses[i];
	}
	printf("Unresolved Label %08X\n",labelToSearch);
	bye();
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
	if (!printEachInstruction) quadMergeIB(&allData,&ib_internal_div32_s_s,&ib_internal_div32_u_u,&ib_internal_mod32_s_s,&ib_internal_mod32_u_u);
	dualMergeIB(&allData,&binContainer.functions,&binContainer.staticData);
	//printInstructionBufferWithMessageAndNumber(&binContainer.functions,"functions:",allData.numberOfSlotsTaken);
	//printInstructionBufferWithMessageAndNumber(&binContainer.staticData,"staticData:",allData.numberOfSlotsTaken);
	destroyInstructionBuffer(&binContainer.functions);
	destroyInstructionBuffer(&binContainer.staticData);
	uint32_t labelCount=0;
	uint32_t labelTotal;
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		enum InstructionTypeID id=allData.buffer[i].id;
		if (id==I_LABL | id==I_FCST | id==I_JJMP) labelCount++;
	}
	labelTotal=labelCount;
	uint32_t* labelNames=cosmic_malloc(labelTotal*sizeof(uint32_t));
	uint32_t* labelAddresses=cosmic_malloc(labelTotal*sizeof(uint32_t));
	labelCount=0;
	uint32_t storageAddress=1LU<<16;
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		InstructionSingle IS=allData.buffer[i];
		if (printEachInstruction) {printf("addr:%08X:",storageAddress);printSingleInstructionOptCode(IS);printf("\n");}
		storageAddress+=backendInstructionSize(IS);
		if (IS.id==I_LABL){
			labelNames[labelCount]=allData.buffer[i].arg.D.a_0;
			labelAddresses[labelCount]=storageAddress;
			//printf("%08X:%08X\n",labelNames[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_JJMP){
			labelNames[labelCount]=allData.buffer[i].arg.BBD.a_2;
			labelAddresses[labelCount]=storageAddress;
			//printf("%08X:%08X\n",labelNames[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_FCST){
			labelNames[labelCount]=allData.buffer[i].arg.BWD.a_2;
			labelAddresses[labelCount]=storageAddress;
			//printf("%08X:%08X\n",labelNames[labelCount],labelAddresses[labelCount]);
			labelCount++;
		} else if (IS.id==I_FCEN){
			endOfExecutable=storageAddress;
		}
	}
	uint32_t storageSize=storageAddress;
	{
	uint32_t D32U_label_address=0x04000000;
	uint32_t R32U_label_address=0x04000000;
	uint32_t D32S_label_address=0x04000000;
	uint32_t R32S_label_address=0x04000000;
	for (uint32_t i=0;i<labelTotal;i++){
		if (labelNames[i]<=4){
			uint32_t v=labelAddresses[i];
			switch (labelNames[i]){
				case 0:printf("NULL label is invalid\n");bye();
				case 1:D32U_label_address=v;break;
				case 2:R32U_label_address=v;break;
				case 3:D32S_label_address=v;break;
				case 4:R32S_label_address=v;break;
			}
		}
	}
	uint16_t func_stack_size;
	uint8_t func_stack_initial;
	uint8_t* temporaryStorageBuffer=cosmic_calloc(storageAddress,sizeof(uint8_t));
	uint8_t* temporaryStorageBufferWalk=temporaryStorageBuffer+(1LU<<16);
	for (uint32_t i=0;i<allData.numberOfSlotsTaken;i++){
		InstructionSingle IS=allData.buffer[i];
		uint32_t symVal;
		switch (IS.id){
			case I_FCST:
			func_stack_initial=IS.arg.BWD.a_0;
			func_stack_size=IS.arg.BWD.a_1;
			break;
			case I_D32U:symVal=D32U_label_address;break;
			case I_R32U:symVal=R32U_label_address;break;
			case I_D32S:symVal=D32S_label_address;break;
			case I_R32S:symVal=R32S_label_address;break;
			case I_JTEN:
			symVal=getLabelAddress(IS.arg.D.a_0,labelTotal,labelNames,labelAddresses);
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
			symVal=getLabelAddress(IS_temp0.arg.D.a_0,labelTotal,labelNames,labelAddresses);
			}
			break;
			case I_SYDB:
			case I_SYRB:
			case I_SYRW:
			case I_SYDW:
			printf("I don't wanna do that yet...");
			bye();
			default:;
		}
		switch (IS.id){
			case I_D32U:
			case I_R32U:
			case I_D32S:
			case I_R32S:
			if (symVal==0x04000000){
				printf("Error: div32 components not linked put were needed\n");
				bye();
			}
		}
		backendInstructionWrite(&temporaryStorageBufferWalk,symVal,func_stack_size,func_stack_initial,IS);
	}
	assert(temporaryStorageBufferWalk==(temporaryStorageBuffer+storageAddress));
	uint32_t mainAddress=getLabelAddress(mainLabelNumber,labelTotal,labelNames,labelAddresses);
	cosmic_free(labelNames);
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
	machineState.reg[0]=machineState.sp; //sp->%0
	
	// write end storageSize so malloc and friends can pick it up
	initMemWrite(0,0xFF&(storageSize>> 0));
	initMemWrite(1,0xFF&(storageSize>> 8));
	initMemWrite(2,0xFF&(storageSize>>16));
	initMemWrite(3,0xFF&(storageSize>>24));
	
	machineState.pc=mainAddress;// set pc to main address
	}
	printf("Starting SDL...\n");
	initializeSDL();
	printf("Running...\n");
	fullExecute();
	
	
	
	printf("Finished Normally. Return Value is `%d`\n",terminationValue);
	bye();
}






