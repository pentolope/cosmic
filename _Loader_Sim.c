
#define __BUILDING_SIM_LOADER
#include "_Loader_Base.c"


int main(int argc, char** argv){
	*((int*)__FUNCTION_RET_VALUE_PTR)=0;
	_exit_ret_address=__FUNCTION_RET_INSTRUCTION_ADDRESS;
	_exit_ret_val_ptr=__FUNCTION_RET_VALUE_PTR;
	if (argc==0) return 0xFFFF;
	return run_binary(argc-1,argv+1);
}
