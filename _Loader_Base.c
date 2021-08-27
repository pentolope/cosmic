
#ifndef __STD_REAL
#define __STD_REAL
#endif

#define INCLUDE_BACKEND
#define IGNORE_NONBACKEND


#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <stdlib.c>
#include <string.c>


#define err_00__0(message)     _err_(message)
#define err_00__1(message)     _err_(message)
#define err_10_00(message)     _err_(message)
#define err_10_01(message)     _err_(message)
#define err_10_1_(message)     _err_(message)
#define err_010_0(message,s)   _err_(message)
#define err_010_1(message,s)   _err_(message)
#define err_011_0(message,s,e) _err_(message)
#define err_011_1(message,s,e) _err_(message)
#define err_11000(message,s)   _err_(message)
#define err_11001(message,s)   _err_(message)
#define err_1101_(message,s)   _err_(message)
#define err_11100(message,s,e) _err_(message)
#define err_11101(message,s,e) _err_(message)
#define err_1111_(message,s,e) _err_(message)

static void _err_(const char* message){
	fprintf(stderr,"%s\n",message);
	exit(1);
}


#include "TargetInstructions/InstructionBase.c"
#include "IntrinsicBuiltFiles/_intrinsic_c_functions.c"

uint32_t numberOfLabels;
struct LabelNumberAddressPair {
	uint32_t number;
	uint32_t address;
} *labelAddressPair;

uint32_t mainLabel;
uint32_t mainAddress;
void* binaryLocation;

char* strMerge3(const char*const s0,const char*const s1,const char*const s2){
	const uint32_t l0 = strlen(s0);
	const uint32_t l1 = strlen(s1);
	const uint32_t l2 = strlen(s2);
	char*const sf = malloc(l0+l1+l2+1lu);
	if (sf==NULL){
		fprintf(stderr,"malloc failed to allocate %lu bytes\n",l0+l1+l2+1lu);
		exit(1);
	}
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

void sortLabels(){
	// this uses shell sort
	static uint32_t shellDeltaArray[20]={11797391,5243285,2330349,1035711,460316,204585,90927,40412,17961,7983,3548,1577,701,301,132,57,23,10,4,1};
	for (uint16_t shellDeltaI=0;shellDeltaI<20;shellDeltaI++){
		uint32_t shellDelta=shellDeltaArray[shellDeltaI];
		for (uint32_t i0=shellDelta;i0<numberOfLabels;i0++){
			uint32_t i1=i0-shellDelta;
			if (labelAddressPair[i1].number>labelAddressPair[i0].number){
				uint32_t i2=i0;
				struct LabelNumberAddressPair swap_temp;
				while (1){
					swap_temp=labelAddressPair[i2];
					labelAddressPair[i2]=labelAddressPair[i1];
					labelAddressPair[i1]=swap_temp;
					if (i1<shellDelta) break;
					i2=i1;
					i1-=shellDelta;
					if (labelAddressPair[i2].number>labelAddressPair[i1].number) break;
				}
				
			}
		}
	}
}

uint32_t getLabelAddress(uint32_t labelToSearch){
	if (labelToSearch==0){
		fprintf(stderr,"Label address of 0 is invalid\n");
		exit(1);
	}
	int32_t low,high,mid;
	uint32_t cmp;
	low=0u;
	high=numberOfLabels-1u;
	while (low<=high){
		cmp=labelAddressPair[mid=((uint32_t)low+(uint32_t)high)>>1].number;
		if (cmp==labelToSearch) return labelAddressPair[mid].address;
		if (cmp<labelToSearch){
			low=mid+1;
		} else {
			high=mid-1;
		}
	}
	fprintf(stderr,"Unresolved Label %04X%04X\n",(unsigned)(labelToSearch>>16),(unsigned)labelToSearch);
	exit(1);
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
		default:;
	}
	return 0;
}

#include "StdObjInitializer.c"

struct {
	uint16_t vs[100]; // [value stack]
	int16_t vsl; // [value stack location]
	int16_t pvsl; // [previous value stack location]
} symbolicStack;


void symbolicResolutionSingle(const uint8_t* workingByteCode,InstructionSingle* IS_temp){
	decompressInstruction(workingByteCode,IS_temp);
	if (symbolicStack.vsl<10){
		fprintf(stderr,"ByteCode corrupted, symbolic stack overflow\n");
		exit(1);
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
			fprintf(stderr,"ByteCode corrupted, symbolic stack underflow\n");
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
			case 4:assert(false);// qword not ready yet
			
			default:assert(false);
		}
	}
}


int binSearchStdObjNames(char* name){
	int low,high,mid,cmp;
	low=0;
	high=NUM_STD_SYMBOLS-1;
	while (low<=high){
		cmp=strcmp(stdObjects[mid=(low+high)>>1].symbol,name);
		if (cmp==0) return mid;
		if (cmp<0){
			low=mid+1;
		} else {
			high=mid-1;
		}
	}
	return -1;
}

uint32_t binaryFile_noEOF_fget_32(){
	uint32_t v;
	v =(uint32_t)binaryFile_noEOF_fgetc();
	v|=(uint32_t)binaryFile_noEOF_fgetc()<<8;
	v|=(uint32_t)binaryFile_noEOF_fgetc()<<16;
	v|=(uint32_t)binaryFile_noEOF_fgetc()<<24;
	return v;
}

void loadFileContentsAsByteCode(const char* filePath){
	for (uint16_t i1=0;i1<NUM_STD_SYMBOLS;i1++){stdObjects[i1].label=0;} // this for loop isn't really needed for sim loader but it will be needed for kernel's loader.
	labelAddressPair=NULL;binaryFileLoadState.corruptionErrorMessage=NULL;binaryFileLoadState.binaryFile=NULL; // setting these to NULL is not really needed
	
	const uint8_t icc_to_delta[] = {[0]=0,[1]=1,[2]=2,[3]=2,[4]=2,[5]=3,[6]=3,[7]=3,[8]=4,[9]=5,[10]=6,[11]=8,[12]=3,[13]=4,[14]=4,[15]=5};
	uint16_t id_temp;
	uint8_t workingByteCode[10];
	uint32_t initialLabelCount=NUM_STD_SYMBOLS;
	uint32_t initialRawSize=0;
	uint32_t dataWalkIndex=0;
	while ((id_temp=_intrinsic_c_functions[dataWalkIndex++])!=0){
		workingByteCode[0]=id_temp;
		initialLabelCount+=id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP;
		const uint8_t icc = instructionContentCatagory(id_temp);
		const uint8_t deltaByteCode=icc_to_delta[icc];
		for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
			workingByteCode[iDelta]=_intrinsic_c_functions[dataWalkIndex++];
		}
		initialRawSize+=backendInstructionSizeFromByteCode(workingByteCode);
	}
	initialRawSize+=initialRawSize & 1;
	// the loop above could be reduced to a calculation at compile time... but I'm not going to do that right now.
	
	binaryFileLoadState.binaryFile=fopen(filePath,"rb");
	if (binaryFileLoadState.binaryFile==NULL){err_10_1_(strMerge3("Error: Could not load file \"",filePath,"\""));}
	uint32_t totalFunctionAndStaticDataBackendSize,totalFunctionAndStaticDataBackendSizeAligned,totalFunctionAndStaticDataLength,len_symbols;
	uint8_t* fileContentAfterSymbols;
	binaryFileLoadState.corruptionErrorMessage=strMerge3("Error: Input file \"",filePath,"\" is corrupted");
	totalFunctionAndStaticDataBackendSize=binaryFile_noEOF_fget_32();
	totalFunctionAndStaticDataLength=binaryFile_noEOF_fget_32();
	numberOfLabels=binaryFile_noEOF_fget_32();
	len_symbols=binaryFile_noEOF_fget_32();
	totalFunctionAndStaticDataBackendSize+=initialRawSize;
	numberOfLabels+=initialLabelCount;
	totalFunctionAndStaticDataBackendSizeAligned=totalFunctionAndStaticDataBackendSize + (totalFunctionAndStaticDataBackendSize & 1);
	free((char*)binaryFileLoadState.corruptionErrorMessage);// free is temporary, it will be remade. it is freed because then it's more likely that the binary can be placed better by malloc
	binaryFileLoadState.corruptionErrorMessage=NULL;
	binaryLocation=malloc(totalFunctionAndStaticDataBackendSizeAligned);
	fileContentAfterSymbols=malloc(totalFunctionAndStaticDataLength);
	labelAddressPair=calloc(numberOfLabels,sizeof(struct LabelNumberAddressPair));
	if (binaryLocation==NULL | fileContentAfterSymbols==NULL | labelAddressPair==NULL){
		fprintf(stderr,"Could not allocate sufficient space for binary\n");
		exit(1);
	}
	binaryFileLoadState.corruptionErrorMessage=strMerge3("Error: Input file \"",filePath,"\" is corrupted");
	for (uint32_t i=0;i<len_symbols;i++){
		uint32_t label;
		char name[64]; // identifiers over 60 characters long won't match any standard identifier anyway
		name[0]=0;
		uint8_t name_i=0;
		char c;
		label=binaryFile_noEOF_fget_32();
		while (6<=(c=binaryFile_noEOF_fgetc())){
			if (name_i!=62){
				name[name_i++]=c;
				name[name_i]=0;
			}
		}
		uint8_t type=c;
		if (type==4){
			if (strcmp("main",name)==0){
				mainLabel=label;
			}
		} else if (type==5 | type==1){
			int match=binSearchStdObjNames(name);
			if (match!=-1 && stdObjects[match].expectedType==type){
				stdObjects[match].label=label;
			}
		}
	}
	if (mainLabel==0){
		fprintf(stderr,"Could not find \'main\' symbol\n");
		exit(1);
	}
	if (fread(fileContentAfterSymbols,1,totalFunctionAndStaticDataLength,binaryFileLoadState.binaryFile)!=totalFunctionAndStaticDataLength){
		GiveCorruptedMessage:;
		fprintf(stderr,"%s\n",binaryFileLoadState.corruptionErrorMessage);
		exit(1);
	}
	binaryFile_isEOF_fgetc();
	if (feof(binaryFileLoadState.binaryFile)==0){
		goto GiveCorruptedMessage;
	}
	if (fclose(binaryFileLoadState.binaryFile)!=0){
		fprintf(stderr,"Error when closing \"%s\"\n",filePath);
		exit(1);
	}
	bool doubleLoopSwitch;
	uint32_t rawWalkPosition=(uint32_t)binaryLocation;
	for (uint16_t i=0;i<NUM_STD_SYMBOLS;i++){
		labelAddressPair[i].number=stdObjects[i].label;
		labelAddressPair[i].address=(uint32_t)stdObjects[i].address;
	}
	uint32_t labelWalkCount=NUM_STD_SYMBOLS;
	dataWalkIndex=0;
	while ((id_temp=_intrinsic_c_functions[dataWalkIndex++])!=0){
		workingByteCode[0]=id_temp;
		const uint8_t icc = instructionContentCatagory(id_temp);
		const uint8_t deltaByteCode=icc_to_delta[icc];
		for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
			workingByteCode[iDelta]=_intrinsic_c_functions[dataWalkIndex++];
		}
		rawWalkPosition+=backendInstructionSizeFromByteCode(workingByteCode);
		if (id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP){
			if (labelWalkCount>=numberOfLabels){
				goto GiveCorruptedMessage;
			}
			InstructionSingle IS;
			decompressInstruction(workingByteCode,&IS);
			labelAddressPair[labelWalkCount].address=rawWalkPosition;
			if (id_temp==I_LABL){
				labelAddressPair[labelWalkCount++].number=IS.arg.D.a_0;
			} else if (id_temp==I_FCST){
				labelAddressPair[labelWalkCount++].number=IS.arg.BWD.a_2;
			} else {
				labelAddressPair[labelWalkCount++].number=IS.arg.BBD.a_2;
			}
		}
	}
	rawWalkPosition+=rawWalkPosition & 1;
	doubleLoopSwitch=false;
	dataWalkIndex=0;
	do {
		while ((id_temp=fileContentAfterSymbols[dataWalkIndex++])!=0){
			workingByteCode[0]=id_temp;
			const uint8_t icc = instructionContentCatagory(id_temp);
			const uint8_t deltaByteCode=icc_to_delta[icc];
			for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
				workingByteCode[iDelta]=fileContentAfterSymbols[dataWalkIndex++];
			}
			// dataWalkIndex may walk a little too far (and thus outside of it's allocation) but it probably won't be too bad since it is just reading data...
			if (dataWalkIndex>totalFunctionAndStaticDataLength){
				goto GiveCorruptedMessage;
			}
			rawWalkPosition+=backendInstructionSizeFromByteCode(workingByteCode);
			if (id_temp==I_LABL | id_temp==I_FCST | id_temp==I_JJMP){
				if (labelWalkCount>=numberOfLabels){
					goto GiveCorruptedMessage;
				}
				InstructionSingle IS;
				decompressInstruction(workingByteCode,&IS);
				labelAddressPair[labelWalkCount].address=rawWalkPosition;
				if (id_temp==I_LABL){
					labelAddressPair[labelWalkCount++].number=IS.arg.D.a_0;
				} else if (id_temp==I_FCST){
					labelAddressPair[labelWalkCount++].number=IS.arg.BWD.a_2;
				} else {
					labelAddressPair[labelWalkCount++].number=IS.arg.BBD.a_2;
				}
			}
		}
		if (doubleLoopSwitch) break;
		doubleLoopSwitch=true;
	} while (true);
	rawWalkPosition+=rawWalkPosition & 1;
	// dataWalkIndex may walk a little too far (and thus outside of it's allocation) but it probably won't be too bad since it is just reading data...
	if (dataWalkIndex!=totalFunctionAndStaticDataLength | labelWalkCount!=numberOfLabels | rawWalkPosition!=(uint32_t)binaryLocation+totalFunctionAndStaticDataBackendSizeAligned){
		goto GiveCorruptedMessage;
	}
	// label addresses now determined
	sortLabels();
	mainAddress=getLabelAddress(mainLabel);
	
	uint8_t* binaryLocationWalk=binaryLocation;
	uint16_t func_stack_size;
	uint8_t func_stack_initial;
	dataWalkIndex=0;
	while ((id_temp=_intrinsic_c_functions[dataWalkIndex++])!=0){
		workingByteCode[0]=id_temp;
		uint8_t icc = instructionContentCatagory(id_temp);
		uint8_t deltaByteCode=icc_to_delta[icc];
		for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
			workingByteCode[iDelta]=_intrinsic_c_functions[dataWalkIndex++];
		}
		InstructionSingle IS;
		decompressInstruction(workingByteCode,&IS);
		
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
					workingByteCode[0]=_intrinsic_c_functions[dataWalkIndex++];
					if (workingByteCode[0]==0){
						// end inside symbolic
						goto GiveCorruptedMessage;
					}
					icc = instructionContentCatagory(workingByteCode[0]);
					deltaByteCode=icc_to_delta[icc];
					for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
						workingByteCode[iDelta]=_intrinsic_c_functions[dataWalkIndex++];
					}
					symbolicResolutionSingle(workingByteCode,&IS_temp);
				} while (IS_temp.id!=I_SYRE & IS_temp.id!=I_SYDE);
				if (100-symbolicStack.vsl!=symbolSizeWords){
					// symbolic result size mismatch
					goto GiveCorruptedMessage;
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
		backendInstructionWrite(&binaryLocationWalk,symVal,func_stack_size,func_stack_initial,IS);
	}
	dataWalkIndex=0;
	doubleLoopSwitch=false;
	do {
		while ((id_temp=fileContentAfterSymbols[dataWalkIndex++])!=0){
		workingByteCode[0]=id_temp;
		uint8_t icc = instructionContentCatagory(id_temp);
		uint8_t deltaByteCode=icc_to_delta[icc];
		for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
			workingByteCode[iDelta]=fileContentAfterSymbols[dataWalkIndex++];
		}
		InstructionSingle IS;
		decompressInstruction(workingByteCode,&IS);
		
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
					workingByteCode[0]=fileContentAfterSymbols[dataWalkIndex++];
					if (workingByteCode[0]==0){
						// end inside symbolic
						goto GiveCorruptedMessage;
					}
					icc = instructionContentCatagory(workingByteCode[0]);
					deltaByteCode=icc_to_delta[icc];
					for (uint8_t iDelta=1;iDelta<deltaByteCode;iDelta++){
						workingByteCode[iDelta]=fileContentAfterSymbols[dataWalkIndex++];
					}
					symbolicResolutionSingle(workingByteCode,&IS_temp);
				} while (IS_temp.id!=I_SYRE & IS_temp.id!=I_SYDE);
				if (100-symbolicStack.vsl!=symbolSizeWords){
					// symbolic result size mismatch
					goto GiveCorruptedMessage;
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
		backendInstructionWrite(&binaryLocationWalk,symVal,func_stack_size,func_stack_initial,IS);
		}
		if (doubleLoopSwitch) break;
		doubleLoopSwitch=true;
	} while (true);
	if (dataWalkIndex!=totalFunctionAndStaticDataLength){
		// should not be possible, but anyway...
		goto GiveCorruptedMessage;
	}
	// all data is now placed
	free((char*)binaryFileLoadState.corruptionErrorMessage);
	free(fileContentAfterSymbols);
	free(labelAddressPair);
}

static void _exec_springboard1(){
	// this is a very neat trick.
	// after _exec_springboard1 returns, the loaded binary's main is run instead of going back to the loader's _exec_springboard0
	__FUNCTION_RET_INSTRUCTION_ADDRESS=mainAddress;
}

static int _exec_springboard0(int argc, char** argv){
	*((int*)__FUNCTION_RET_VALUE_PTR)=0;
	_exec_springboard1();
}



static void catch_exit_sub1(){
	_exit_info.ret_stack_address=__FUNCTION_CALLER_RET_STACK_ADDRESS;
	_exit_info.frame_stack_address=__FUNCTION_CALLER_FRAME_STACK_ADDRESS;
	_exit_info.ret_address=__FUNCTION_RET_INSTRUCTION_ADDRESS;
}

static int catch_exit_sub0(){
	static struct {
		unsigned long r0;
		unsigned int r1;
		unsigned int r2;
		unsigned int r3;
		unsigned int r4;
	} restore; // must be static !
	
	restore.r0=__FUNCTION_RET_INSTRUCTION_ADDRESS;
	restore.r1=__FUNCTION_CALLER_RET_STACK_ADDRESS;
	restore.r2=__FUNCTION_CALLER_FRAME_STACK_ADDRESS;
	restore.r3=__FUNCTION_ARG_SIZE;
	restore.r4=__FUNCTION_RET_VALUE_PTR;
	_exit_info.ret_val_ptr=__FUNCTION_RET_VALUE_PTR;
	catch_exit_sub1();
	__FUNCTION_RET_INSTRUCTION_ADDRESS=restore.r0;
	__FUNCTION_CALLER_RET_STACK_ADDRESS=restore.r1;
	__FUNCTION_CALLER_FRAME_STACK_ADDRESS=restore.r2;
	__FUNCTION_ARG_SIZE=restore.r3;
	__FUNCTION_RET_VALUE_PTR=restore.r4;
}

static int caught_exit_value;

static bool catch_exit(){
	static struct {
		unsigned long r0;
		unsigned int r1;
		unsigned int r2;
		unsigned int r3;
		unsigned int r4;
		bool will_catch_exit;
		bool did_catch_exit;
	} restore; // must be static !
	
	restore.r0=__FUNCTION_RET_INSTRUCTION_ADDRESS;
	restore.r1=__FUNCTION_CALLER_RET_STACK_ADDRESS;
	restore.r2=__FUNCTION_CALLER_FRAME_STACK_ADDRESS;
	restore.r3=__FUNCTION_ARG_SIZE;
	restore.r4=__FUNCTION_RET_VALUE_PTR;

	restore.will_catch_exit=0;
	caught_exit_value=catch_exit_sub0();
	__FUNCTION_RET_INSTRUCTION_ADDRESS=restore.r0;
	__FUNCTION_CALLER_RET_STACK_ADDRESS=restore.r1;
	__FUNCTION_CALLER_FRAME_STACK_ADDRESS=restore.r2;
	__FUNCTION_ARG_SIZE=restore.r3;
	__FUNCTION_RET_VALUE_PTR=restore.r4;
	restore.did_catch_exit=restore.will_catch_exit;
	restore.will_catch_exit=1;
	return restore.did_catch_exit;
}

int run_binary(int argc, char** argv){
	_isKernelExecuting=0;
	if (catch_exit()) return caught_exit_value;
	if (argc==0) return 0xFFFF;
	loadFileContentsAsByteCode(argv[0]);
	int ret = _exec_springboard0(argc,argv);
	if (catch_exit()) return caught_exit_value;
	free(binaryLocation);
	return ret;
}






