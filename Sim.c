
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
	bool isDoingInit;
} globalSDL_Information = {.windowSize = {320u,200u,3u},.isDoingInit=true};

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
	printf("\nExiting...\n");
	if (!globalSDL_Information.isDoingInit) SDL_Quit();
	exit(0);
}

void writePixelToScreenWithSDL(uint16_t a, uint8_t c){
	if (a>=320u*200u || (globalSDL_Information.isDoingInit | globalSDL_Information.pixelState[a]!=c)){
		globalSDL_Information.pixelState[a]=c;
		const uint8_t r=(255u/7u)*(7u&(unsigned)c/32u)+1u;
		const uint8_t g=(255u/7u)*(7u&(unsigned)c/4u)+1u;
		const uint8_t b=(255u/3u)*(3u&(unsigned)c);
		const SDL_Rect rect={.x=a%320u,.y=(a/320u)%200u,.h=globalSDL_Information.windowSize.sizeOfUnit,.w=globalSDL_Information.windowSize.sizeOfUnit};
		SDL_FillRect(globalSDL_Information.surfaceOnWindow,&rect,SDL_MapRGB(globalSDL_Information.surfaceOnWindow->format, r, g, b));
		
		if (!globalSDL_Information.isDoingInit) SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&rect,1); // updating is slow...
		
		SDL_PollEvent(&globalSDL_Information.event); // to keep Windows from complaining
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
		globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.sizeOfUnit,
		globalSDL_Information.windowSize.height*globalSDL_Information.windowSize.sizeOfUnit,
		0
	);
	globalSDL_Information.surfaceOnWindow = SDL_GetWindowSurface(globalSDL_Information.window);
	for (uint16_t i=0;i<320u*200u;i++) writePixelToScreenWithSDL(i,0);
	
	const SDL_Rect fullRect={.x=0,.y=0,.h=globalSDL_Information.windowSize.height*globalSDL_Information.windowSize.sizeOfUnit,.w=globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.sizeOfUnit};
	SDL_UpdateWindowSurfaceRects(globalSDL_Information.window,&fullRect,1);
	globalSDL_Information.isDoingInit=false;
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

/*
all special accesses are performed using volatile byte operations (unless otherwise specified). ranges are given as inclusive ranges

GPU memory may be written to at 0x04000000->0x0400FFFF

Halt/Exit is achieved by writing to 0x04010000 , the value is the return value.
If main terminates, it will set the pc to 0x04010000, at which point the value at 0x0000FFFE will be interpreted as the return value.

Opening a file may be done by writing mode to 0x04010001 , then writing path to 0x04020002 (null termination, simply write each byte to same address)
  if successful, the reading at the location 0x04020003 will hold the value 1, otherwise it will hold the value 0
  closing a file may be done by writing a 0 to 0x04020003
  only one file may be opened at a time
  Once a file is opened, it's contents are in the range 0x05000000->0x05FFFFFF and the length is stored in 0x04010006->0x04010009


*/





























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
	printf("Loading Binary...\n");
	struct BinContainer binContainer=loadFileContentsAsBinContainer(argv[1]);
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
	quadMergeIB(&allData,&ib_internal_div32_s_s,&ib_internal_div32_u_u,&ib_internal_mod32_s_s,&ib_internal_mod32_u_u);
	dualMergeIB(&allData,&binContainer.functions,&binContainer.staticData);
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
		storageAddress+=backendInstructionSize(IS);
		if (IS.id==I_LABL){
			labelNames[labelCount]=allData.buffer[i].arg.D.a_0;
			labelAddresses[labelCount]=storageAddress;
			labelCount++;
		} else if (IS.id==I_JJMP){
			labelNames[labelCount]=allData.buffer[i].arg.BBD.a_2;
			labelAddresses[labelCount]=storageAddress;
			labelCount++;
		} else if (IS.id==I_FCST){
			labelNames[labelCount]=allData.buffer[i].arg.BWD.a_2;
			labelAddresses[labelCount]=storageAddress;
			labelCount++;
		}
	}
	uint32_t storageSize=storageAddress;
	{
	uint8_t* temporaryStorageBuffer=cosmic_calloc(storageAddress,sizeof(uint8_t));
	uint8_t* temporaryStorageBufferWalk=temporaryStorageBuffer+(1LU<<16);
	uint32_t D32U_label_address=0;
	uint32_t R32U_label_address=0;
	uint32_t D32S_label_address=0;
	uint32_t R32S_label_address=0;
	for (uint32_t i=0;i<labelTotal;i++){
		if (labelNames[i]<=4){
			uint32_t v=labelAddresses[i];
			switch (labelNames[i]){
				case 0:printf("NULL label is invalid (0)\n");bye();
				case 1:D32U_label_address=v;break;
				case 2:R32U_label_address=v;break;
				case 3:D32S_label_address=v;break;
				case 4:R32S_label_address=v;break;
			}
		}
	}
	if (D32U_label_address==0 | R32U_label_address==0 | D32S_label_address==0 | R32S_label_address==0) {printf("NULL label is invalid (1)\n");bye();}
	uint16_t func_stack_size;
	uint8_t func_stack_initial;
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
		backendInstructionWrite(&temporaryStorageBufferWalk,symVal,func_stack_size,func_stack_initial,IS);
	}
	uint32_t mainAddress=getLabelAddress(mainLabelNumber,labelTotal,labelNames,labelAddresses);
	cosmic_free(labelNames);
	cosmic_free(labelAddresses);
	for (uint32_t i=1LU<<16;i<storageSize;i++){
		initMemWrite(i,temporaryStorageBuffer[i]);
	}
	cosmic_free(temporaryStorageBuffer);
	// now insert the arguments
	int argcSim=argc-1;
	int w=0;
	for (int i=1;i<argc;i++){
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
	for (int i=1;i<argc;i++){
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
	initMemWrite(0,0xFF&(storageSize>> 0));
	initMemWrite(1,0xFF&(storageSize>> 8));
	initMemWrite(2,0xFF&(storageSize>>16));
	initMemWrite(3,0xFF&(storageSize>>24));
	
	}
	printf("Starting SDL...\n");
	initializeSDL();
	printf("Running...\n");
	
	
	
	printf("Finished Normally.\n");
	bye();
}






