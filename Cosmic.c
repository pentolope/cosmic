
#ifdef __COSMIC
#define __STD_REAL
#define __BUILDING_SIM_LOADER

static unsigned long _exit_ret_address=0xFFFFFFF0;
static unsigned int _exit_ret_val_ptr;
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <stdlib.c>
#include <stdio.c>
#include <printf.c>
#include <string.c>
#endif



#include "StatementWalk.c"
#include "FinalizersAndFileIO.c"


// if addAngleBrackets==false, then double quotes will be added
void runPreprocessCompile(const char* startFilePath, const char* outFilePath, bool addAngleBrackets){
	char* startFilePathWithEnds;
	if (addAngleBrackets){
		startFilePathWithEnds=strMerge3("<",startFilePath,">");
	} else {
		startFilePathWithEnds=strMerge3("\"",startFilePath,"\"");
	}
	insertBeginningFileAndRunPreprocesser(startFilePathWithEnds);
	cosmic_free(startFilePathWithEnds);
	fileScopeStatementsWalk();
	doFunctionDefinitionCheck();
	applyFinalizeToGlobalStaticData();
	finalOutputFromCompile(outFilePath);
	destroyAllCompilerInfo();
}

void intrinsicBuild(){
	compileSettings.optLevel=4;
	globalLabelID = 0x2F;
	runPreprocessCompile("TargetInstructions/ComplexIntrinsics/source.c","IntrinsicBuiltFiles/_intrinsics.bin",false);
	printf("Compilation of \'source.c\' finished, inserting smaller intrinsics...\n");
	struct BinContainer bc=loadFileContentsAsBinContainer("IntrinsicBuiltFiles/_intrinsics.bin");
	quadMergeIB(&bc.functions,&ib_intrinsic_back_div32_u_u,&ib_intrinsic_back_mod32_u_u,&ib_intrinsic_back_div32_s_s,&ib_intrinsic_back_mod32_s_s);
	dualMergeIB(&bc.functions,&ib_intrinsic_back_Lshift32,&ib_intrinsic_back_Rshift32);
	CompressedInstructionBuffer cib0 = compressInstructionBuffer(&bc.functions);
	CompressedInstructionBuffer cib1 = compressInstructionBuffer(&bc.staticData);
	safe_fputc_file_path="IntrinsicBuiltFiles/_intrinsics_symbols.bin";
	safe_fputc_file = fopen("IntrinsicBuiltFiles/_intrinsics_symbols.bin","wb");
	if (safe_fputc_file==NULL) err_10_1_("Could not open required file for output");
	safe_fputc((uint8_t)(bc.len_symbols));
	safe_fputc((uint8_t)(bc.len_symbols>>8));
	safe_fputc((uint8_t)(bc.len_symbols>>16));
	safe_fputc((uint8_t)(bc.len_symbols>>24));
	for (uint32_t i0=0;i0<bc.len_symbols;i0++){
		struct SymbolEntry se=bc.symbols[i0];
		safe_fputc((uint8_t)(se.label));
		safe_fputc((uint8_t)(se.label>>8));
		safe_fputc((uint8_t)(se.label>>16));
		safe_fputc((uint8_t)(se.label>>24));
		for (uint32_t i1=0;se.name[i1];i1++){
			safe_fputc(se.name[i1]);
		}
		safe_fputc(se.type);
		cosmic_free(se.name);
	}
	if (fclose(safe_fputc_file)!=0) err_10_1_("Could not close required file after writting");
	
	safe_fputc_file_path="IntrinsicBuiltFiles/_intrinsics_functions.bin";
	safe_fputc_file = fopen("IntrinsicBuiltFiles/_intrinsics_functions.bin","wb");
	if (safe_fputc_file==NULL) err_10_1_("Could not open required file for output");
	for (uint32_t i=0;i<cib0.allocLen;i++){
		safe_fputc(cib0.byteCode[i]);
	}
	if (fclose(safe_fputc_file)!=0) err_10_1_("Could not close required file after writting");
	
	safe_fputc_file_path="IntrinsicBuiltFiles/_intrinsics_static.bin";
	safe_fputc_file = fopen("IntrinsicBuiltFiles/_intrinsics_static.bin","wb");
	if (safe_fputc_file==NULL) err_10_1_("Could not open required file for output");
	for (uint32_t i=0;i<cib1.allocLen;i++){
		safe_fputc(cib1.byteCode[i]);
	}
	if (fclose(safe_fputc_file)!=0) err_10_1_("Could not close required file after writting");
	
	safe_fputc_file=NULL;
	safe_fputc_file_path=NULL;
	destroyInstructionBuffer(&bc.functions);
	destroyInstructionBuffer(&bc.staticData);
	cosmic_free(cib0.byteCode);
	cosmic_free(cib1.byteCode);
	cosmic_free(bc.symbols);
}

void runLink(const char* outPath,const char* in1Path,const char* in2Path){
	if (doStringsMatch(in1Path,in2Path)) err_10_1_("Linking identical files is silly");
	struct BinContainer bc0=loadFileContentsAsBinContainer(in1Path);
	struct BinContainer bc1=loadFileContentsAsBinContainer(in2Path);
	genAllBinContainerMatch(bc0,bc1);
	printf("\nLink task done, but not finished. I have yet to write the rest.\n");
	exit(0);
}



struct {
	bool doTypicalCompile;
	bool doLink;
	bool expectOutNext;
	bool optz;
	bool opt0;
	bool opt1;
	bool opt2;
	bool opt3;
	bool outVoid;
	const char* outPath;
	const char* in1Path;
	const char* in2Path;
} mainArg = {.doTypicalCompile=true};

int main(int argc, char** argv){
	checkArchitecture();
	if (argc==0){
		err_10_1_("It\'s the Operating System\'s fault");
	}
	{
		char* directoryStr = copyStringToHeapString(argv[0]);
		int32_t i=0;
		char c;
		// change any \ to / in directoryStr
		while ((c=directoryStr[i++])){
			if (c=='\\'){
				directoryStr[i-1]='/';
			}
		}
		i--;
		// find last slash in directoryStr 
		while (i!=0){
			if (directoryStr[--i]=='/'){
				++i;
				break;
			}
		}
		// and remove contents of string after the last slash, removing everything if there is no slash
		directoryStr[i]=0;
		directoryOfExecutable=directoryStr;
	}
	for (int argi=1;argi<argc;argi++){
		const char* arg=argv[argi];
		if (mainArg.expectOutNext){
			if (mainArg.outPath!=NULL) err_10_1_("Command argument: Only one output may be specified");
			mainArg.outPath=arg;
			mainArg.expectOutNext=false;
		} else if (doStringsMatch(arg,"-Oz")){
			mainArg.optz=true;
			if (mainArg.opt0 | mainArg.opt1 | mainArg.opt2 | mainArg.opt3) err_10_1_("Command argument: Only one optimization level may be specified");
		} else if (doStringsMatch(arg,"-O0")){
			mainArg.opt0=true;
			if (mainArg.optz | mainArg.opt1 | mainArg.opt2 | mainArg.opt3) err_10_1_("Command argument: Only one optimization level may be specified");
		} else if (doStringsMatch(arg,"-O1")){
			mainArg.opt1=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt2 | mainArg.opt3) err_10_1_("Command argument: Only one optimization level may be specified");
		} else if (doStringsMatch(arg,"-O2")){
			mainArg.opt2=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt1 | mainArg.opt3) err_10_1_("Command argument: Only one optimization level may be specified");
		} else if (doStringsMatch(arg,"-O3")){
			mainArg.opt3=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt1 | mainArg.opt2) err_10_1_("Command argument: Only one optimization level may be specified");
		} else if (doStringsMatch(arg,"-void")){
			mainArg.outVoid=true;
			if (mainArg.outPath!=NULL) err_10_1_("Command argument: \"-void\" may not be specified with \"-o\"");
		} else if (doStringsMatch(arg,"-o")){
			mainArg.expectOutNext=true;
			if (mainArg.outVoid) err_10_1_("Command argument: \"-void\" may not be specified with \"-o\"");
		} else if (doStringsMatch(arg,"-l")){
			mainArg.doLink=true;
			mainArg.doTypicalCompile=false;
		} else if (doStringsMatch(arg,"-nc")){
			compileSettings.noColor=true;
		} else if (doStringsMatch(arg,"-_intrinsicBuild")){
			// -_intrinsicBuild will override anything else
			printf("\nPerforming Intrinsic Build...\n");
			intrinsicBuild();
			printf("Intrinsic Build Completed Successfully\n");
			return 0;
		} else if (mainArg.in1Path==NULL){
			mainArg.in1Path=arg;
		} else if (mainArg.in2Path==NULL){
			mainArg.in2Path=arg;
		} else {
			err_10_1_("Command argument: Maxiumum of two input files may be specified");
		}
	}
	
	if (mainArg.expectOutNext) err_10_1_("Command argument: Expected output file path after \'-o\'");
	if (mainArg.doTypicalCompile & mainArg.in2Path!=NULL) err_10_1_("Command argument: Maxiumum of one input files may be specified for compile task");
	if (mainArg.doLink & mainArg.in2Path==NULL) err_10_1_("Command argument: Two input files required for link task");
	if (mainArg.in1Path==NULL) err_10_1_("Command argument: No input files specified");
	if (mainArg.optz) compileSettings.optLevel=0;
	if (mainArg.opt0) compileSettings.optLevel=1;
	if (mainArg.opt1) compileSettings.optLevel=2;
	if (mainArg.opt2) compileSettings.optLevel=3;
	if (mainArg.opt3) compileSettings.optLevel=4;
	if (mainArg.outPath==NULL & !mainArg.outVoid) mainArg.outPath="a.out";
	
	if (mainArg.doTypicalCompile) runPreprocessCompile(mainArg.in1Path,mainArg.outPath,false);
	if (mainArg.doLink) runLink(mainArg.outPath,mainArg.in1Path,mainArg.in2Path);
	printf("\nTask Completed Successfully\n");
	
	//printf("%lu,%lu\n",(unsigned long)heap_track.max,(unsigned long)heap_track.size);
	return 0;
}


