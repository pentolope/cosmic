
#include "StatementWalk.c"


void runPreprocessCompile(const char* startFilePath){
	insertBeginningFileAndRunPreprocesser(startFilePath);
	printf("Starting Compiler\n");
	fileScopeStatementsWalk();
	printf("Finalizing...\n");
	applyFinalizeToGlobalStaticData();
	printf("Compiler Finished\n");
}

int main(int argc, char** argv){
	printf("Argument list:\n");
	for (int argi=0;argi<argc;argi++){
		printf("  '%s'\n",argv[argi]);
	}
	runPreprocessCompile(startFileName);	
	return 0;
}


