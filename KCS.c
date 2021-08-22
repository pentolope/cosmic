// This is the Kernel Console Shell. It isn't part of cosmic, so maybe it shouldn't be here.

#define __STD_REAL

static unsigned long _exit_ret_address;
static unsigned int _exit_ret_val_ptr;

#include <_implementation_real.c>
#include "_Loader_Base.c"


int main(){
	stdin_do_line_block=1;
	_isKernelExecuting=1;
	{
		uint16_t limit=getTerminalCursorLimit();
		for (uint16_t i=0;i<limit;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
	}
	while (1){
		ex_stdin_block_appropriate();
		_putchar_screen('|');
		_putchar_screen('\n');
	}
}

