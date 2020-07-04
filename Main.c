
#include "StatementWalk.c"


int main(int argc, char** argv){
	printf("Argument list:\n");
	for (int argi=0;argi<argc;argi++){
		printf("  '%s'\n",argv[argi]);
	}
	
	insertBeginningFileAndRunPreprocesser(startFileName);
	
	printf("Starting Compiler\n");
	
	fileScopeStatementsWalk();
	
	printf("Compiler Finished\n");
	return 0;
}


