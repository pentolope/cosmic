
#include "StatementWalk.c"
#include "FinalizersAndFileIO.c"

// if addAngleBrackets==false, then double quotes will be added
void runPreprocessCompile(const char* startFilePath, const char* outFilePath,bool addAngleBrackets){
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
	finalOutput(outFilePath);
	destroyAllCompilerInfo();
}

struct {
	bool doTypicalCompile;
	bool doBinView;
	bool doLink;
	bool doLoadAndRun;
	bool expectOutNext;
	bool optz;
	bool opt0;
	bool opt1;
	bool opt2;
	bool opt3;
	const char* outPath;
	const char* in1Path;
	const char* in2Path;
} mainArg = {.doTypicalCompile=true};

int main(int argc, char** argv){
	for (int argi=1;argi<argc;argi++){
		const char* arg=argv[argi];
		if (mainArg.expectOutNext){
			if (mainArg.outPath!=NULL){
				err_10_1_("Command argument: Only one output may be specified");
			}
			mainArg.outPath=arg;
			mainArg.expectOutNext=false;
		} else if (doStringsMatch(arg,"-Oz")){
			mainArg.optz=true;
			if (mainArg.opt0 | mainArg.opt1 | mainArg.opt2 | mainArg.opt3){
				err_10_1_("Command argument: Only one optimization level may be specified");
			}
		} else if (doStringsMatch(arg,"-O0")){
			mainArg.opt0=true;
			if (mainArg.optz | mainArg.opt1 | mainArg.opt2 | mainArg.opt3){
				err_10_1_("Command argument: Only one optimization level may be specified");
			}
		} else if (doStringsMatch(arg,"-O1")){
			mainArg.opt1=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt2 | mainArg.opt3){
				err_10_1_("Command argument: Only one optimization level may be specified");
			}
		} else if (doStringsMatch(arg,"-O2")){
			mainArg.opt2=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt1 | mainArg.opt3){
				err_10_1_("Command argument: Only one optimization level may be specified");
			}
		} else if (doStringsMatch(arg,"-O3")){
			mainArg.opt3=true;
			if (mainArg.optz | mainArg.opt0 | mainArg.opt1 | mainArg.opt2){
				err_10_1_("Command argument: Only one optimization level may be specified");
			}
		} else if (doStringsMatch(arg,"-o")){
			mainArg.expectOutNext=true;
		} else if (doStringsMatch(arg,"-v")){
			if (!mainArg.doTypicalCompile){
				err_10_1_("Command argument: Only one task may be specified");
			}
			mainArg.doBinView=true;
			mainArg.doTypicalCompile=false;
		} else if (doStringsMatch(arg,"-l")){
			if (!mainArg.doTypicalCompile){
				err_10_1_("Command argument: Only one task may be specified");
			}
			mainArg.doLink=true;
			mainArg.doTypicalCompile=false;
		} else if (doStringsMatch(arg,"-r")){
			if (!mainArg.doTypicalCompile){
				err_10_1_("Command argument: Only one task may be specified");
			}
			mainArg.doLoadAndRun=true;
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
	if (mainArg.expectOutNext){
		err_10_1_("Command argument: Expected output file path after \'-o\'");
	}
	if (mainArg.doTypicalCompile & mainArg.in2Path!=NULL){
		err_10_1_("Command argument: Maxiumum of one input files may be specified for compile task");
	}
	if (!mainArg.doLink & mainArg.in2Path!=NULL){
		err_10_1_("Command argument: Maxiumum of one input files may be specified for that task");
	}
	if (mainArg.in1Path==NULL){
		err_10_1_("Command argument: No input files specified");
	}
	if (mainArg.optz){
		compileSettings.optLevel=0;
	}
	if (mainArg.opt0){
		compileSettings.optLevel=1;
	}
	if (mainArg.opt1){
		compileSettings.optLevel=2;
	}
	if (mainArg.opt2){
		compileSettings.optLevel=3;
	}
	if (mainArg.opt3){
		compileSettings.optLevel=4;
	}
	
	if (mainArg.doTypicalCompile){
		if (mainArg.outPath==NULL) mainArg.outPath="a.out";
		runPreprocessCompile(mainArg.in1Path,mainArg.outPath,false);
	}
	if (mainArg.doBinView){
		err_10_1_("Command argument: bin view not ready yet");
	}
	if (mainArg.doLink){
		err_10_1_("Command argument: link not ready yet");
	}
	if (mainArg.doLoadAndRun){
		err_10_1_("Command argument: load and run not ready yet");
	}
	printf("\nTask completed successfully\n");
	return 0;
}


