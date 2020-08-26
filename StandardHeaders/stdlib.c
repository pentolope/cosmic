


#include <stdlib.h>

// the arguments and return value are important, don't remove them
static int _exit_springboard(int argc_unused, char** argv_unused){
	__FUNCTION_RET_INSTRUCTION_ADDRESS=_exit_ret_address;
}

static void exit(int status){
	// placeholder definition
	printf("\nexit:[%04X]",status);
	
	*((int*)_exit_ret_val_ptr)=status;
	_exit_springboard(0,NULL);
}

static void abort(void){
	// placeholder definition
	
	printf("\nabort:[]");
	
	*((int*)_exit_ret_val_ptr)=137; // what should abort() cause to be returned ?
	_exit_springboard(0,NULL);
}





