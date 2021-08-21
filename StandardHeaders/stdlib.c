#include <stdlib.h>

// the arguments and return value are important, don't remove them
int _exit_springboard(int argc_unused, char** argv_unused){
	__FUNCTION_RET_INSTRUCTION_ADDRESS=_exit_ret_address;
}

void exit(int status){
	*((int*)_exit_ret_val_ptr)=status;
	_exit_springboard(0,NULL);
}

void abort(void){
	*((int*)_exit_ret_val_ptr)=134;
	_exit_springboard(0,NULL);
}
