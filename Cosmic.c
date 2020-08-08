
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
	printf("\nTask completed successfully\n");
	//printf("%lu,%lu\n",(unsigned long)heap_track.max,(unsigned long)heap_track.size);
	return 0;
}


