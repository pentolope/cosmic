#include <stdlib.h>
#include <stdio.h>

void _exit_springboard(){
	__FUNCTION_RET_INSTRUCTION_ADDRESS=_exit_info.ret_address;
	__FUNCTION_CALLER_RET_STACK_ADDRESS=_exit_info.ret_stack_address;
	__FUNCTION_CALLER_FRAME_STACK_ADDRESS=_exit_info.frame_stack_address;
	// I had a situation when running a program on the fpga where the program managed to return from exit. it's... weird, to say the least.
	// ordering the assignments like this (with the return address assignment first) seemed to fix the problem, but that doesn't make much sense.
}

void exit(int status){
	*((int*)_exit_info.ret_val_ptr)=status;
	_exit_springboard();
}

void abort(void){
	*((int*)_exit_info.ret_val_ptr)=134;
	_exit_springboard();
}
