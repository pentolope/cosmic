
#define __BUILDING_SIM_LOADER
#define __STD_REAL

static struct {
	unsigned long ret_address;
	unsigned int ret_val_ptr;
	unsigned int ret_stack_address;
	unsigned int frame_stack_address;
} _exit_info;

#include <_implementation_sim.c>
#include "_Loader_Base.c"


int main(int argc, char** argv){
	if (argc==0) return 0xFFFF;
	return run_binary(argc-1,argv+1);
}
