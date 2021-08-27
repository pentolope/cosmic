// This is the Kernel Console Shell. It isn't part of cosmic, so maybe it shouldn't be here.

#define __STD_REAL

static struct {
	unsigned long ret_address;
	unsigned int ret_val_ptr;
	unsigned int ret_stack_address;
	unsigned int frame_stack_address;
} _exit_info;


#include <_implementation_real.c>
#include "_Loader_Base.c"

bool hasAlreadyStarted;

int main(){
	if (hasAlreadyStarted){
		_putstr_screen("\nFatal Error: KCS detected that an unknown error occured");
		while (1){}
	}
	hasAlreadyStarted=1;
	_isKernelExecuting=1;
	*((int*)__FUNCTION_RET_VALUE_PTR)=0;
	if (catch_exit()) goto exit_caught;
	{
		uint16_t limit=getTerminalCursorLimit();
		for (uint16_t i=0;i<limit;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
	}
	all_open_files=calloc(1,sizeof(struct All_Open_Files));
	if (all_open_files==NULL){
		_putstr_screen("\nFatal Error: KCS can\'t dynamically allocate memory");
		while (1){}
	}
	stdin =all_open_files->file_descriptor_handles+0;
	stdout=all_open_files->file_descriptor_handles+1;
	stderr=all_open_files->file_descriptor_handles+2;
	
	stdin_do_line_block=1;
	if (perform_file_system_init()){
		_putstr_screen("\nFatal Error: KCS can\'t initialize the file system");
		while (1){}
	}
	const char* arg[]={"a.out","-nc","Cosmic.c","-O3","-o","m.out",NULL};
	int rv=run_binary(6,arg);
	if (catch_exit()) goto exit_caught;
	printf("\nReturned to Kernel (%d)\n",rv);
	
	while (1){
		_putchar_screen(ex_stdin_block_appropriate());
	}
	
	// this point should be unreachable
	
	exit_caught:;{
		_putstr_screen("\nFatal Error: KCS detected that a core function called exit(0x");
		unsigned int byte0,digit0,digit1;
		byte0=((char*)&caught_exit_value)[1];
		digit0=(byte0>>4)&0xFu;
		digit1=(byte0>>0)&0xFu;
		digit0=digit0+(digit0>=10u)*7u+48u;
		digit1=digit1+(digit1>=10u)*7u+48u;
		_putchar_screen(digit0);
		_putchar_screen(digit1);
		byte0=((char*)&caught_exit_value)[0];
		digit0=(byte0>>4)&0xFu;
		digit1=(byte0>>0)&0xFu;
		digit0=digit0+(digit0>=10u)*7u+48u;
		digit1=digit1+(digit1>=10u)*7u+48u;
		_putchar_screen(digit0);
		_putchar_screen(digit1);
		_putchar_screen(')');
	}
	while (1){}
}
