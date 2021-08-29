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

void makeColor(char type, char color){
	printf("%c%c%c%c%c",        // printf() doesn't always work as intended when these are printed with %s format
		27,'[',type,color,'m');
}

void resetColor(){
	printf("\033[0m");
}


char** parse_to_argv(char* s){
	uint32_t len=1;
	uint16_t seg_count=1;
	uint16_t state=0;
	uint32_t i;
	char c;
	for (i=0;s[i];i++){
		c=s[i];
		switch (state){
			case 0:
			if (c=='\''){
				state=1;
			} else if (c=='\"'){
				state=2;
			} else {
				len++;
				if (c==' '){
					seg_count++;
					state=3;
				}
			}
			break;
			case 1:
			if (c=='\''){
				state=0;
			} else {
				len++;
			}
			break;
			case 2:
			if (c=='\"'){
				state=0;
			} else {
				len++;
			}
			break;
			case 3:
			if (c=='\''){
				state=1;
			} else if (c=='\"'){
				state=2;
			} else {
				if (c!=' '){
					len++;
					state=0;
				}
			}
			break;
		}
	}
	if (state!=3) return NULL;
	char** o=calloc(seg_count,sizeof(char*));
	if (o==NULL) return NULL;
	char* os=calloc(len,sizeof(char));
	if (os==NULL) {free(o);return NULL;}
	o[0]=os;
	state=0;
	len=0;
	seg_count=0;
	for (i=0;s[i];i++){
		c=s[i];
		switch (state){
			case 0:
			if (c=='\''){
				state=1;
			} else if (c=='\"'){
				state=2;
			} else {
				if (c==' '){
					seg_count++;
					os[len++]=0;
					state=3;
				} else {
					os[len++]=c;
				}
			}
			break;
			case 1:
			if (c=='\''){
				state=0;
			} else {
				os[len++]=c;
			}
			break;
			case 2:
			if (c=='\"'){
				state=0;
			} else {
				os[len++]=c;
			}
			break;
			case 3:
			if (c=='\''){
				state=1;
			} else if (c=='\"'){
				state=2;
			} else {
				if (c!=' '){
					o[seg_count]=os+len;
					os[len++]=c;
					state=0;
				}
			}
			break;
		}
	}
	if (o[seg_count]!=NULL){
		free(os);
		free(o);
		return NULL; // that should not happen...
	}
	return o;
}

void reset_standard_streams(){
	stdin =all_open_files->file_descriptor_handles+0;
	stdout=all_open_files->file_descriptor_handles+1;
	stderr=all_open_files->file_descriptor_handles+2;
	stdin_do_line_block=1;
}

bool run_command_caught_exit;
bool run_command_alloc_failed;
bool run_command_was_binary;

int run_command(char** arg){
	run_command_was_binary=0;
	uint16_t len;
	for (len=0;arg[len]!=NULL;len++){}
	
	if (len==1 & arg[0][0]==0){
		// do nothing because there isn't really a command
	} else if (strcmp("cd",arg[0])==0){
		char* t0=strMerge3(all_open_files->working_directory,"/",arg[1]);
		char* t1=normalize_path(t0);
		struct Folder_File_Object* ffo=calloc(1,sizeof(struct Folder_File_Object));
		if (ffo==NULL) goto alloc_fail;
		uint8_t res=fat_find_normalized_path_target(t1,ffo,0);
		if (res!=0 || (ffo->target.attributes & FAT_ATTRIB_DIR)==0){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			if (res!=0) printf("`cd` target was rejected because it is not found.\n");
			else printf("`cd` target was rejected because it is not a directory.\n");
			resetColor();
			free(t1);
		} else {
			free(all_open_files->working_directory);
			all_open_files->working_directory=t1;
		}
		free(t0);
		free(ffo);
	} else if (strcmp("ls",arg[0])==0){
		struct Folder_File_Object* ffo=calloc(1,sizeof(struct Folder_File_Object));
		if (ffo==NULL) goto alloc_fail;
		struct Directory_Content_Iterator_Arguments* dcia=calloc(1,sizeof(struct Directory_Content_Iterator_Arguments));
		if (dcia==NULL) goto alloc_fail;
		uint8_t res=fat_find_path_target(all_open_files->working_directory,ffo,0);
		if (res!=0 || (ffo->target.attributes & FAT_ATTRIB_DIR)==0){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("~Error finding working directory~\n");
			resetColor();
		} else {
			fat_enter_target_folder(ffo);
			dcia->ffo=ffo;
			dcia->hide_mask=0;
			dcia->current_cluster=ffo->parent_folder_entrance_cluster;
			uint8_t* ffo_target_name=ffo->target.name;
			while (fat_directory_content_iterator(dcia)){
				if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0){
					makeColor(COLOR_TO_TEXT,COLOR_BLUE);
				} else {
					makeColor(COLOR_TO_TEXT,COLOR_GREEN);
				}
				if ((ffo->target.attributes & FAT_ATTRIB_HIDDEN)!=0 | (ffo->target.attributes & FAT_ATTRIB_SYSTEM)!=0){
					makeColor(COLOR_TO_BACKGROUND,COLOR_YELLOW);
				}
				printf("%s\n",ffo_target_name);
				resetColor();
			}
		}
		free(ffo);
		free(dcia);
	} else if (strcmp("rm",arg[0])==0){
		struct Folder_File_Object* ffo=calloc(1,sizeof(struct Folder_File_Object));
		if (ffo==NULL) goto alloc_fail;
		char* t0=strMerge3(all_open_files->working_directory,"/",arg[1]);
		char* t1=normalize_path(t0);
		uint8_t res0=fat_find_normalized_path_target(t1,ffo,0);
		free(t0);
		free(t1);
		if (res0!=0){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("~could not find deletion target~\n");
			resetColor();
		} else if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("~directory deletion is not ready yet~\n");
			resetColor();
		} else {
			uint8_t res1=fat_delete_object(ffo);
			if (res1==1){
				makeColor(COLOR_TO_TEXT,COLOR_YELLOW);
				printf("~warning:the removed cluster chain ended with a free_cluster, reserved_cluster, or bad_cluster~\n");
				resetColor();
			} else if (res1!=0){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("~deletion likely failed~\n");
				resetColor();
			}
		}
		free(ffo);
	} else if (strcmp("clear",arg[0])==0){
		uint16_t limit=getTerminalCursorLimit();
		for (uint16_t i=0;i<limit;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
		_terminalCharacterState.cursor=0;
	} else if (strcmp("echo",arg[0])==0){
		uint16_t k;
		for (k=1;arg[k]!=NULL;k++){
			printf("%s",arg[k]);
			if (arg[k+1u]!=NULL) printf(" ");
		}
		printf("\n");
	} else {
		struct Folder_File_Object* ffo=calloc(1,sizeof(struct Folder_File_Object));
		if (ffo==NULL) goto alloc_fail;
		char* t0=strMerge3(all_open_files->working_directory,"/",arg[0]);
		char* t1=normalize_path(t0);
		uint8_t res0=fat_find_normalized_path_target(t1,ffo,0);
		bool isDir=(ffo->target.attributes & FAT_ATTRIB_DIR)!=0;
		free(t0);
		free(t1);
		free(ffo);
		if (res0!=0){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("~could not find execution target~\n");
			resetColor();
		} else if (isDir){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("~execution target cannot be a directory~\n");
			resetColor();
		} else {
			run_command_was_binary=1;
			int rv=run_binary(len,arg);
			if (catch_exit()) {run_command_caught_exit=1;return 0;}
			_isKernelExecuting=1;
			bool flush_failure=0;
			for (uint16_t k=0;k<MAX_FILE_DESCRIPTORS;k++){
				if (all_open_files->file_descriptor_handles[k].file_descriptor>=3){
					if (fclose(all_open_files->file_descriptor_handles+k)!=0){
						flush_failure=1;
					}
				}
			}
			if (fflush(NULL)!=0){
				flush_failure=1;
			}
			reset_standard_streams();
			resetColor();
			if (flush_failure){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("~warning: fflush failed~\n");
				resetColor();
			}
			if (_freeNonKernel()){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("~warning: heap corruption detected~\n");
				resetColor();
			}
			printf("\n(%d)\n",rv);
			return rv;
		}
	}
	if (fflush(NULL)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("~warning: fflush failed~\n");
		resetColor();
	}
	return 0;
	alloc_fail:run_command_alloc_failed=1;return 0;
}

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
		_terminalCharacterState.cursor=0;
	}
	all_open_files=calloc(1,sizeof(struct All_Open_Files));
	if (all_open_files==NULL){
		alloc_fail:;
		_putstr_screen("\nFatal Error: KCS can\'t dynamically allocate memory");
		while (1){}
	}
	reset_standard_streams();
	if (perform_file_system_init()){
		_putstr_screen("\nFatal Error: KCS can\'t initialize the file system");
		while (1){}
	}
	all_open_files->working_directory=malloc(2);
	if (all_open_files->working_directory==NULL) goto alloc_fail;
	all_open_files->working_directory[0]='/';
	all_open_files->working_directory[1]=0;
	
	resetColor();
	makeColor(COLOR_TO_TEXT,COLOR_GREEN);
	printf("KernelConsoleShell is ready.\n");
	resetColor();
	
	while (1){
		makeColor(COLOR_TO_TEXT,COLOR_MAGENTA);
		printf("%s$ ",all_open_files->working_directory);
		resetColor();
		char* a=calloc(260,1);
		uint16_t i=0;
		char c;
		while ((c=ex_stdin_block_appropriate())!='\n'){
			a[i++]=c;
		}
		a[i++]=' ';
		char** arg=parse_to_argv(a);
		free(a);
		if (arg==NULL){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("Failed to parse previous command\n");
			resetColor();
		} else {
			int rv=run_command(arg);
			if (run_command_alloc_failed) goto alloc_fail;
			if (run_command_caught_exit) goto exit_caught;
			if (catch_exit()) goto exit_caught;
			free(arg[0]);
			free(arg);
		}
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
