
#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <windows.h>

#include <stdnoreturn.h>

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


noreturn void bye(void){
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

char* strMerge2(const char*const s0,const char*const s1){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	char*const sf = malloc(l0+l1+1);
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
	char*const sf = malloc(l0+l1+l2+1);
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
	char*const sf = malloc(l0+l1+l2+l3+1);
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
	char*const sf = malloc(l0+l1+l2+l3+l4+1);
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



struct StringBuilder{
	struct StringBuilder* nextStringBuilder;
	char buffer[256];
	uint16_t nextCharIndex;
};

static void stringBuilderAppendChar(struct StringBuilder* stringBuilderIn,char c){
	struct StringBuilder* stringBuilderWork=stringBuilderIn;
	while (stringBuilderWork->nextStringBuilder!=NULL){
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	if (stringBuilderWork->nextCharIndex==256){
		stringBuilderWork->nextStringBuilder=calloc(1,sizeof(struct StringBuilder));
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	stringBuilderWork->buffer[stringBuilderWork->nextCharIndex++]=c;
}

static char* stringBuilderToString(struct StringBuilder* stringBuilderIn){
	uint32_t length=1;
	struct StringBuilder* stringBuilderWork=stringBuilderIn;
	while (stringBuilderWork->nextStringBuilder!=NULL){
		length+=stringBuilderWork->nextCharIndex;
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	}
	do {
		length+=stringBuilderWork->nextCharIndex;
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	} while (stringBuilderWork!=NULL);
	char* string = calloc(length,1);
	stringBuilderWork=stringBuilderIn;
	uint32_t i0=0;
	do {
		for (uint16_t i1=0;i1<stringBuilderWork->nextCharIndex;i1++){
			string[i0++]=stringBuilderWork->buffer[i1];
		}
		stringBuilderWork=stringBuilderWork->nextStringBuilder;
	} while (stringBuilderWork!=NULL);
	return string;
}

static struct StringBuilder* stringBuilderCreate(){
	return calloc(1,sizeof(struct StringBuilder));
}

static void stringBuilderDestroy(struct StringBuilder* stringBuilderIn){
	if (stringBuilderIn==NULL) return;
	stringBuilderDestroy(stringBuilderIn->nextStringBuilder);
	free(stringBuilderIn);
}

static void stringBuilderClear(struct StringBuilder* stringBuilderIn){
	stringBuilderDestroy(stringBuilderIn->nextStringBuilder);
	memset(stringBuilderIn,0,sizeof(struct StringBuilder));
}


struct FileAccessInfo{
	struct StringBuilder name;
	struct StringBuilder mode;
	uint8_t* contents;
	uint32_t length;
	bool isRead;
	FILE* outFile;
} fileAccessInfo={.isRead=true};

static void triggerFileLoad(){
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
		fileAccessInfo.contents = malloc(fileAccessInfo.length*sizeof(char));
		for (int32_t i=0;i<fileAccessInfo.length;i++){
			int c=fgetc(inputFile);
			assert(c!=EOF);
			fileAccessInfo.contents[i]=c;
		}
		fclose(inputFile);
	} else {
		fileAccessInfo.outFile=inputFile;
	}
	free(filePath);
	free(fileMode);
}

static void triggerFileClose(){
	if (fileAccessInfo.isRead){
		free(fileAccessInfo.contents);
	} else {
		fclose(fileAccessInfo.outFile);
	}
	fileAccessInfo.outFile=NULL;
	fileAccessInfo.contents=NULL;
	fileAccessInfo.length=0;
	//printf("\nfclose()\n");
}



uint8_t readByte(uint32_t a){
	//printf("MEM R byte:[%08X]\n",a);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	assert(aUD!=0);
	if (a==0x04020003){
		return fileAccessInfo.isRead?fileAccessInfo.contents!=NULL:fileAccessInfo.outFile!=NULL;
	}
	if (a>=0x05000000 & a<=0x05FFFFFF){
		if (fileAccessInfo.contents==NULL | !fileAccessInfo.isRead){
			printf("Mem fault:byte read at file when no file is open for read:%08X\n",a);
			bye();
		}
		if (a-0x05000000<fileAccessInfo.length){
			//printf("{%08X:%02X}",a-0x05000000,fileAccessInfo.contents[a-0x05000000]);
			if (!fileAccessInfo.isRead | fileAccessInfo.contents==NULL){
				printf("Mem fault:byte read at file when no file is open for read:%08X\n",a);
				bye();
			}
			return fileAccessInfo.contents[a-0x05000000];
		}
		printf("Mem warn:byte read at file out of bounds:%08X\n",a);
		return -1;// EOF
	}
	printf("Mem fault:byte read out of bounds:%08X\n",a);
	bye();
}

void writeByte(uint32_t a,uint8_t b){
	//printf("MEM W byte:[%08X,%02X]\n",a,b);
	b&=0xFF;
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	assert(aUD!=0);
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
			printf("Mem fault:byte write to file open detector is not 0:%08X\n",a);
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
			printf("Mem fault:byte write at file when no file is open for write\n");
			bye();
		}
		if (fputc(b,fileAccessInfo.outFile)!=b){
			printf("Sim fault:could not write byte:%02X\n",b);
			bye();
		}
		fflush(fileAccessInfo.outFile);
		return;
	}
	printf("Mem fault:byte write out of bounds:%08X\n",a);
	bye();
}

void writeWord(uint32_t a,uint16_t w){
	//printf("MEM W word:[%08X,%04X]\n",a,w);
	w&=0xFFFF;
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		printf("Mem fault:word write misalligned:%08X\n",a);
		bye();
		return;
	}
	assert(aUD!=0);
	if (a==0x04010000){
		printf("Termination Value:`%u`\n",(unsigned)w);
		bye();
		return;
	}
	printf("Mem fault:word write out of bounds:%08X\n",a);
	bye();
}

uint16_t readWord(uint32_t a){
	//printf("MEM R word:[%08X]\n",a);
	uint32_t aL=a&((1LU<<11)-1);
	uint32_t aU=(a>>11);
	uint32_t aU_=aU&((1LU<<15)-1);
	uint32_t aUD=(aU-aU_)>>15;
	if ((a&1)!=0){
		printf("Mem fault:word read misalligned:%08X\n",a);
		bye();
		return 0;
	}
	assert(aUD!=0);
	if (a==0x04020006){
		return (uint16_t)(fileAccessInfo.length>> 0);
	}
	if (a==0x04020008){
		return (uint16_t)(fileAccessInfo.length>> 16);
	}
	printf("Mem fault:word read out of bounds:%08X\n",a);
	bye();
}



struct MachineState{
	uint8_t* mem;
	uint32_t pc;
	uint16_t sp;
	uint16_t reg[16];
};

void stateDump(struct MachineState ms){
	printf("{ pc:%08X ",ms.pc);
	printf(" sp:%04X ",ms.sp);
	for (int i=0;i<16;i++){
		printf(" %%%01X:%04X ",i,ms.reg[i]);
	}
	printf("}\n");
}

void sdl_pull_event(){
	SDL_PollEvent(&globalSDL_Information.event);
	if (globalSDL_Information.event.type == SDL_QUIT){printf("Premature Exit Initiated...\n");bye();}
}

extern void run(struct MachineState);
extern void memInit(void*);

int main(){
	globalSDL_Information.pixelState=calloc((uint32_t)globalSDL_Information.windowSize.width*globalSDL_Information.windowSize.height,sizeof(uint8_t));
	initializeSDL();
	struct MachineState ms={0};
	ms.mem=calloc(1LLU<<26,sizeof(uint8_t));
	memInit(ms.mem);
	printf("Running...\n");
	run(ms);
}
