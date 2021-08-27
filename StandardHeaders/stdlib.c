#include <stdlib.h>

void _exit_springboard(){
	__FUNCTION_CALLER_RET_STACK_ADDRESS=_exit_info.ret_stack_address;
	__FUNCTION_CALLER_FRAME_STACK_ADDRESS=_exit_info.frame_stack_address;
	__FUNCTION_RET_INSTRUCTION_ADDRESS=_exit_info.ret_address;
}

void exit(int status){
	*((int*)_exit_info.ret_val_ptr)=status;
	_exit_springboard();
}

void abort(void){
	*((int*)_exit_info.ret_val_ptr)=134;
	_exit_springboard();
}
