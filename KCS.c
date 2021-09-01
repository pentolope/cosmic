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

int edit_program(char* filePath){
	_isKernelExecuting=0;// this enables the ability to use _freeNonKernel to remove allocations that don't get free'd
	resetColor();
	printf("Opening \"%s\"...\n",filePath);
	FILE* f=fopen(filePath,"rb");
	if (f==NULL){
		f=fopen(filePath,"wb");
		if (f==NULL){
			printf("Error: Could not open or create file\n");
			return 1;
		}
		if (fclose(f)){
			printf("Error: fclose failed when creating file\n");
			return 2;
		}
		f=fopen(filePath,"rb");
		if (f==NULL){
			printf("Error: Could not open after creating file\n");
			return 3;
		}
	}
	uint32_t fSize=0;
	uint32_t lCount=0;
	int tc;
	char* o;
	char** l;
	bool useCarrigeReturnNewline=0;
	bool didEndHaveNewline=1;
	{
		uint32_t i;
		o=malloc(512);
		if (o==NULL) return 100;
		while ((i=fread(o,1u,512u,f))==512u){
			fSize+=i;
		}
		fSize+=i;
		free(o);
		if (feof(f)==0){
			printf("Error: file read failed\n");
			fclose(f);
			return 1;
		}
		if (rewind(f)){
			printf("Error: file rewind failed\n");
			fclose(f);
			return 5;
		}
		o=calloc(fSize+1u,1u); // +1u is to prevent asking for a size of 0 allocation
		if (o==NULL){
			printf("Error: file too large\n");
			fclose(f);
			return 6;
		}
		if (fread(o,1u,fSize,f)!=fSize){
			printf("Error: file reread failed\n");
			fclose(f);
			return 1;
		}
		fclose(f);
		for (i=0;i<fSize;i++){
			tc=o[i];
			lCount+=(tc=='\n');
			useCarrigeReturnNewline |= (tc=='\r');
			if (!((tc>=' ' & tc<='~') | tc=='\t' | tc=='\r' | tc=='\n')){
				printf("File contains nonprintable characters, aborting edit\n");
				return 4;
			}
		}
		uint32_t* ls;
		l=malloc((lCount+1u)*sizeof(char*)); // +1u is important
		ls=calloc(lCount+1u,sizeof(uint32_t)); // +1u is important
		if (l==NULL | ls==NULL){
			printf("Error: file too large\n");
			return 6;
		}
		uint32_t li=0;
		for (i=0;i<fSize;i++){
			tc=o[i];
			uint32_t k=li;
			li+=(tc=='\n');
			ls[k]+=(tc!='\r');
		}
		for (li=0;li<=lCount;li++){
			if ((l[li]=calloc(ls[li]+2u,1u))==NULL){ // +2u is important
				printf("Error: file too large\n");
				return 6;
			}
		}
		li=0;
		uint32_t sli=0;
		for (i=0;i<fSize;i++){
			tc=o[i];
			if (tc!='\r'){
				l[li][sli++]=tc;
				if (tc=='\n'){
					sli=0;
					li++;
				}
			}
		}
		free(ls);
		i=strlen(l[lCount]);
		if (i==0){
			didEndHaveNewline=0;
			l[lCount][0]='\n';
			assert(l[lCount][i+1u]==0); // should be garenteed by the +2u on size of calloc
		} else {
			if (l[lCount][i-1u]!='\n'){
				didEndHaveNewline=0;
				assert(l[lCount][i]==0); // should be garenteed by strlen
				l[lCount][i]='\n';
				assert(l[lCount][i+1u]==0); // should be garenteed by the +2u on size of calloc
			}
		}
	}
	stdin_do_line_block=0;
	uint16_t cursorLimit=getTerminalCursorLimit();
	for (uint16_t i=0;i<cursorLimit;i++){
		const uint32_t a=0x80800000lu+i*3lu;
		*(volatile uint8_t*)(a+0)=' ';
		*(volatile uint8_t*)(a+1)=182;
		*(volatile uint8_t*)(a+2)=0;
	}
	_terminalCharacterState.cursor=0;
	uint16_t lineSingleLimit=((*(volatile uint8_t*)(0x80804ffflu) &(1<<4))!=0)?80u:71u;
	uint16_t lineAllLimit=cursorLimit/lineSingleLimit;
	makeColor(COLOR_TO_BACKGROUND,COLOR_YELLOW);
	makeColor(COLOR_TO_TEXT,COLOR_MAGENTA);
	printf("Editing \"%s\"\n",filePath);
	resetColor();
	uint16_t startCursor0=_terminalCharacterState.cursor;
	uint16_t startCursor1=startCursor0+lineSingleLimit;
	uint16_t startLine=startCursor1/lineSingleLimit;
	uint32_t currentSingleLine=0;
	uint32_t currentAllLine=0;
	uint32_t currentSingleLineRender=0;
	uint32_t currentAllLineRender=0;
	uint32_t tempLen0,tempLen1;
	char* tempBuffer0;
	goto Render;
	while ((tc=fgetc(stdin))!=27 && tc!=EOF){
		if (!((tc>=' ' & tc<='~') | tc=='\n' | tc=='\t' | tc==8 | tc==17 | tc==18 | tc==19 | tc==20)) continue;
		switch (tc){
			case '\n':
				tempLen0=strlen(l[currentAllLine]+currentSingleLine)+1u;
				tempBuffer0=calloc(tempLen0,1u);
				l=realloc(l,(lCount+2u)*sizeof(char*));
				if (l==NULL | tempBuffer0==NULL) return 100;
				memcpy(tempBuffer0,l[currentAllLine]+currentSingleLine,tempLen0);
				l[currentAllLine][currentSingleLine+0u]='\n';
				l[currentAllLine][currentSingleLine+1u]=0;
				memmove(l+(currentAllLine+2u),l+(currentAllLine+1u),(lCount-currentAllLine)*sizeof(char*));
				l[currentAllLine+1u]=tempBuffer0;
				lCount++;
				currentAllLine++;
				currentSingleLine=0;
			break;
			case 8: //backspace
				if (currentSingleLine==0){
					if (currentAllLine!=0){
						tempLen0=strlen(l[currentAllLine-1u]);
						tempLen1=strlen(l[currentAllLine-0u]);
						l[currentAllLine-1u][tempLen0-1u]=0;
						l[currentAllLine-0u][tempLen1-1u]=0;
						tempBuffer0=calloc(tempLen0+tempLen1+3u,1u);
						strcat(tempBuffer0,l[currentAllLine-1u]);
						strcat(tempBuffer0,l[currentAllLine-0u]);
						strcat(tempBuffer0,"\n");
						free(l[currentAllLine-0u]);
						free(l[currentAllLine-1u]);
						l[currentAllLine-0u]=tempBuffer0;
						memmove(l+(currentAllLine-1u),l+(currentAllLine),((lCount-currentAllLine)+1u)*sizeof(char*));
						lCount--;
						currentAllLine--;
						currentSingleLine=tempLen0-1u;
					}
				} else {
					tempLen0=strlen(l[currentAllLine]);
					memmove(l[currentAllLine]+(currentSingleLine-1u),l[currentAllLine]+(currentSingleLine),(tempLen0-currentSingleLine)+1u);
					currentSingleLine--;
				}
			break;
			case 17: // up arrow
				if (currentAllLine!=0){
					currentAllLine--;
				}
			break;
			case 18: // left arrow
				if (currentSingleLine==0){
					if (currentAllLine!=0){
						currentAllLine--;
						currentSingleLine=strlen(l[currentAllLine])-1u;
					}
				} else {
					currentSingleLine--;
				}
			break;
			case 19: // down arrow
				if (currentAllLine<lCount){
					currentAllLine++;
				}
			break;
			case 20: // right arrow
				tempLen0=strlen(l[currentAllLine]);
				if (++currentSingleLine>=tempLen0){
					if (currentAllLine<lCount){
						currentAllLine++;
						currentSingleLine=0;
					}
				}
			break;
			default:
				tempLen0=strlen(l[currentAllLine]);
				l[currentAllLine]=realloc(l[currentAllLine],tempLen0+2u);
				if (l[currentAllLine]==NULL) return 100;
				memmove(l[currentAllLine]+(currentSingleLine+1u),l[currentAllLine]+(currentSingleLine),(tempLen0-currentSingleLine)+1u);
				l[currentAllLine][currentSingleLine++]=tc;
			break;
		}
		if (currentAllLine>lCount){
			currentAllLine=lCount;
		}
		tempLen0=strlen(l[currentAllLine]);
		if (currentSingleLine>=tempLen0){
			currentSingleLine=tempLen0-1u;
		}
		if (currentSingleLine==0){
			currentSingleLineRender=0;
		} else if (currentSingleLine -1u<currentSingleLineRender){
			currentSingleLineRender=currentSingleLine-1u;
		}
		while (currentSingleLine -currentSingleLineRender > lineSingleLimit -2u){
			currentSingleLineRender++; // I don't want to think of a better way to do this right now
		}
		if (currentAllLine==0){
			currentAllLineRender=0;
		} else if (currentAllLine-1u<currentAllLineRender){
			currentAllLineRender=currentAllLine-1u;
		}
		while (currentAllLine -currentAllLineRender > (lineAllLimit -startLine) -2u){
			currentAllLineRender++; // I don't want to think of a better way to do this right now
		}
		Render:;
		// render text
		uint16_t cursor0=startCursor1;
		uint16_t cursor1=0;
		uint16_t endLine0=currentAllLineRender+(lineAllLimit-startLine);
		uint16_t endLine1=endLine0>lCount?lCount+1u:endLine0;
		uint32_t i0;
		for (i0=currentAllLineRender;i0<endLine1;i0++){
			assert(strlen(l[i0])>0);
			tempLen0=strlen(l[i0])-1u;
			uint16_t i2=0;
			uint32_t i1;
			for (i1=currentSingleLineRender;i1<tempLen0 & i2<lineSingleLimit;i1++,i2++){
				tc=l[i0][i1];
				if (tc=='\t') tc=' ';
				*(volatile uint8_t*)(0x80800000lu+(cursor0+i2)*3lu)=tc;
				if (currentAllLine==i0 & currentSingleLine==i1){
					cursor1=cursor0+i2;
				}
			}
			if (currentAllLine==i0 & currentSingleLine==i1){
				cursor1=cursor0+i2;
			}
			while (i2<lineSingleLimit){
				*(volatile uint8_t*)(0x80800000lu+(cursor0+i2++)*3lu)=' ';
			}
			cursor0+=lineSingleLimit;
		}
		for (;i0<endLine0;i0++){
			uint16_t i2=0;
			while (i2<lineSingleLimit){
				*(volatile uint8_t*)(0x80800000lu+(cursor0+i2++)*3lu)=' ';
			}
			cursor0+=lineSingleLimit;
		}
		// print position
		for (uint16_t i=startCursor1-1u;i>=startCursor0;i--){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
		_terminalCharacterState.cursor=startCursor0;
		makeColor(COLOR_TO_BACKGROUND,COLOR_BLUE);
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("@ %lu:%lu\n",currentAllLine,currentSingleLine);
		resetColor();
		// set cursor position at proper position
		_terminalCharacterState.cursor=cursor1;
	}
	for (uint16_t i=0;i<cursorLimit;i++){
		const uint32_t a=0x80800000lu+i*3lu;
		*(volatile uint8_t*)(a+0)=' ';
		*(volatile uint8_t*)(a+1)=255;
		*(volatile uint8_t*)(a+2)=0;
	}
	_terminalCharacterState.cursor=0;
	printf("Exited edit mode\n");
	uint32_t endlen=0;
	for (uint32_t i=0;i<=lCount;i++){
		endlen+=strlen(l[i]);
		endlen+=useCarrigeReturnNewline;
	}
	char* e=calloc(endlen+1u,1u); // +1u is important
	tempLen0=0;
	for (uint32_t i=0;i<=lCount;i++){
		tempLen1=strlen(l[i]);
		assert(tempLen1!=0);
		tempLen1--;
		memcpy(e+tempLen0,l[i],tempLen1);
		tempLen0+=tempLen1;
		if (i!=lCount | didEndHaveNewline){
			if (useCarrigeReturnNewline) e[tempLen0++]='\r';
			e[tempLen0++]='\n';
		}
	}
	tempLen0=strlen(e);
	if (strcmp(e,o)==0){
		printf("No changes were made\n");
	} else {
		printf("Changes were made, save those changes? (y/n): ");
		do {
			tc=fgetc(stdin);
		} while (tc!='Y' & tc!='y' & tc!='N' & tc!='n');
		printf("%c\n",tc);
		if (tc=='Y' | tc=='y'){
			f=fopen(filePath,"wb");
			if (f==NULL){
				printf("Failed to save changes! (%s failed)\n","fopen");
				return 1;
			}
			if (fwrite(e,1u,tempLen0,f)!=tempLen0){
				printf("Failed to save changes! (%s failed)\n","fwrite");
				fclose(f);
				return 1;
			}
			if (fclose(f)){
				printf("Failed to save changes! (%s failed)\n","fclose");
				return 1;
			}
			printf("Changes were saved\n");
		} else {
			printf("Changes were not saved\n");
		}
	}
	return 0;
}

bool parse_input_failed_from_alloc;

// may modify input
char** parse_to_argv(char* s){
	while (s[0]==' '){
		memmove(s,s+1,strlen(s));
	}
	uint32_t slen;
	while ((slen=strlen(s))>0u && s[slen-1u]==' '){
		s[slen-1u]=0;
	}
	if (slen==0u) return NULL;
	char** o;
	char* os;
	char* sm=malloc(slen);
	if (sm==NULL){
		parse_input_failed_from_alloc=1;
		return NULL;
	}
	/* sm values:
	0-included
	1-not included, seperator
	2-not included, skipped
	*/
	uint16_t state=0;
	/* state values:
	0-typical
	1-' quote
	2-" quote
	*/
	uint32_t oslen=0;
	uint16_t olen=0;
	uint16_t smv;
	uint32_t i;
	char c;
	for (i=0;s[i];i++){
		c=s[i];
		smv=0;
		switch (state){
			case 0:
			switch (c){
				case '\'':smv=2;state=1;break;
				case '\"':smv=2;state=2;break;
				case ' ': smv=1;break;
				default:break;
			}
			break;
			case 1:
			switch (c){
				case '\'':smv=2;state=0;break;
				default:break;
			}
			break;
			case 2:
			switch (c){
				case '\"':smv=2;state=0;break;
				default:break;
			}
			break;
		}
		sm[i]=smv;
		oslen+=(smv==0u | smv==1u);
		olen+=(smv==1u);
		if (i!=0){
			if (sm[i-1u]==1u & smv==1u){
				sm[i-1u]=2;
			}
		}
	}
	if (state!=0){
		free(sm);
		return NULL;
	}
	os=calloc(oslen+1u,sizeof(char));
	o=calloc(olen+2u,sizeof(char*));
	if (os==NULL | o==NULL){
		free(sm);
		free(os);
		free(o);
		parse_input_failed_from_alloc=1;
		return NULL;
	}
	oslen=0;
	olen=0;
	o[0]=os;
	for (i=0;s[i];i++){
		switch (sm[i]){
			case 0:os[oslen++]=s[i];break;
			case 1:os[oslen++]=0;o[++olen]=os+oslen;break;
			//case 2:break;
		}
	}
	free(sm);
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
	int rv;
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
			printf("Error finding working directory\n");
			resetColor();
		} else {
			bool show_hidden_and_sys=0;
			if (arg[1]!=NULL && strcmp("-a",arg[1])==0) show_hidden_and_sys=1;
			fat_enter_target_folder(ffo);
			dcia->ffo=ffo;
			dcia->hide_mask=0;
			dcia->current_cluster=ffo->parent_folder_entrance_cluster;
			uint8_t* ffo_target_name=ffo->target.name;
			while (fat_directory_content_iterator(dcia)){
				bool isSysOrHidden=(ffo->target.attributes & FAT_ATTRIB_HIDDEN)!=0 | (ffo->target.attributes & FAT_ATTRIB_SYSTEM)!=0;
				bool isDir=(ffo->target.attributes & FAT_ATTRIB_DIR)!=0;
				if (show_hidden_and_sys){
					if (isSysOrHidden){
						makeColor(COLOR_TO_BACKGROUND,COLOR_YELLOW);
					}
				} else {
					if (isSysOrHidden){
						continue;
					}
				}
				if (isDir){
					makeColor(COLOR_TO_TEXT,COLOR_BLUE);
				} else {
					makeColor(COLOR_TO_TEXT,COLOR_GREEN);
				}
				printf("%s\n",ffo_target_name);
				resetColor();
			}
		}
		free(ffo);
		free(dcia);
	} else if (strcmp("rm",arg[0])==0){
		struct Folder_File_Object* ffo=malloc(sizeof(struct Folder_File_Object));
		struct Directory_Content_Iterator_Arguments* dcia=malloc(sizeof(struct Directory_Content_Iterator_Arguments));
		if (ffo==NULL | dcia==NULL) goto alloc_fail;
		struct RecursiveFilePathFiLo {
			char* path;
			struct RecursiveFilePathFiLo* next;
			bool secondHit;
		}* recursiveFilePathFiLo=calloc(1,sizeof(struct RecursiveFilePathFiLo));
		if (recursiveFilePathFiLo==NULL) goto alloc_fail;
		recursiveFilePathFiLo->path=strMerge3(all_open_files->working_directory,"/",arg[1]);
		while (recursiveFilePathFiLo!=NULL){
			memset(ffo,0,sizeof(struct Folder_File_Object));
			struct RecursiveFilePathFiLo this=*recursiveFilePathFiLo;
			free(recursiveFilePathFiLo);
			recursiveFilePathFiLo=this.next;
			if (ffo==NULL) goto alloc_fail;
			uint8_t res0=fat_find_path_target(this.path,ffo,0);
			if (res0!=0){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Could not find `%s`\n",this.path);
				free(this.path);
				while (recursiveFilePathFiLo!=NULL){
					this=*recursiveFilePathFiLo;
					printf("Deletion canceled for `%s`\n",this.path);
					free(this.path);
					free(recursiveFilePathFiLo);
					recursiveFilePathFiLo=this.next;
				}
				resetColor();
			} else if (!this.secondHit & (ffo->target.attributes & FAT_ATTRIB_DIR)!=0){
				struct RecursiveFilePathFiLo* temp=calloc(1,sizeof(struct RecursiveFilePathFiLo));
				if (temp==NULL) goto alloc_fail;
				temp->path=this.path;
				temp->next=recursiveFilePathFiLo;
				temp->secondHit=1;
				recursiveFilePathFiLo=temp;
				fat_enter_target_folder(ffo);
				memset(dcia,0,sizeof(struct Directory_Content_Iterator_Arguments));
				dcia->ffo=ffo;
				dcia->current_cluster=ffo->parent_folder_entrance_cluster;
				uint8_t* ffo_target_name=ffo->target.name;
				while (fat_directory_content_iterator(dcia)){
					if (strcmp(".",ffo_target_name)!=0 && strcmp("..",ffo_target_name)!=0){
						temp=calloc(1,sizeof(struct RecursiveFilePathFiLo));
						if (temp==NULL) goto alloc_fail;
						temp->path=strMerge3(this.path,"/",ffo_target_name);
						temp->next=recursiveFilePathFiLo;
						recursiveFilePathFiLo=temp;
					}
				}
			} else {
				printf("`%s`\n",this.path);
				uint8_t res1=fat_delete_object(ffo);
				if (res1==1){
					makeColor(COLOR_TO_TEXT,COLOR_YELLOW);
					printf("Warning:the removed cluster chain ended with a free_cluster, reserved_cluster, or bad_cluster\n");
					resetColor();
				} else if (res1!=0){
					makeColor(COLOR_TO_TEXT,COLOR_RED);
					printf("Deletion likely failed\n");
					resetColor();
				}
				free(this.path);
			}
		}
		free(ffo);
		free(dcia);
	} else if (strcmp("edit",arg[0])==0){
		if (arg[1]==NULL){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("No filepath specified\n");
			resetColor();
		} else {
			run_command_was_binary=1; // it kinda isn't, but let's say it is
			if (catch_exit()){rv=caught_exit_value;goto AfterBinaryRun;}
			rv=edit_program(arg[1]);
			goto AfterBinaryRun;
		}
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
	} else if (strcmp("mkdir",arg[0])==0){
		if (arg[1]==NULL){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("No filepath specified\n");
			resetColor();
		} else {
			struct Folder_File_Object* ffo=calloc(1,sizeof(struct Folder_File_Object));
			if (ffo==NULL) goto alloc_fail;
			char* t0,t1,t2;
			t0=malloc(strlen(arg[1])+1u);
			if (t0==NULL) goto alloc_fail;
			memcpy(t0,arg[1],strlen(arg[1])+1u);
			bool hit;
			do {
				hit=0;
				if (t0[0]!=0 && (t1=strrchr(t0,'/'))!=NULL && (t1-t0)+1u==strlen(t0)){
					*t1=0;
					hit=1;
				}
				if (t0[0]!=0 && (t1=strrchr(t0,'\\'))!=NULL && (t1-t0)+1u==strlen(t0)){
					*t1=0;
					hit=1;
				}
			} while (hit);
			if (t0[0]==0){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("No filepath specified\n");
				resetColor();
			} else {
				if ((t1=strrchr(t0,'/'))!=NULL || (t1=strrchr(t0,'\\'))!=NULL){
					*t1=0;
					t1++;
					t2=strMerge3(all_open_files->working_directory,"/",t1);
				} else {
					t1=t0;
					t2=strMerge3(all_open_files->working_directory,"","");
				}
				uint8_t res0=fat_find_path_target(t2,ffo,0);
				free(t2);
				if (res0!=0){
					makeColor(COLOR_TO_TEXT,COLOR_RED);
					printf("Error when finding base filepath\n");
					resetColor();
				} else {
					uint8_t res1=fat_create_object(ffo,t1,FAT_ATTRIB_DIR);
					if (res1!=0){
						makeColor(COLOR_TO_TEXT,COLOR_RED);
						printf("Error when creating directory\n");
						resetColor();
					}
				}
			}
			free(t0);
			free(ffo);
		}
	} else if (strcmp("help",arg[0])==0){
		static struct {
			const char* cname;
			const char* info0;
			const char* info1;
		} command_list[7]={
			[0].cname="help",
			[0].info0="Lists helpful information. Use `help (command)` to get extra info.",
			[0].info1="To run an executable file, simply type it\'s name instead of a command.\nDouble and single quotes can be used to escape spaces like a typical shell.\nGlob patterns are not supported.",
			[1].cname="ls",
			[1].info0="Lists files and directories.",
			[1].info1="Lists files and directories in the current working directory.\nThey appear in the order they appear on disk.\nDirectories are blue. Files are green.\nIf `-a` is passed, files and directories marked Hidden or System are shown too.\nEntries marked Hidden or System are shown with a yellow background.",
			[2].cname="cd",
			[2].info0="Changes the working directory.",
			[2].info1="Changes the current working directory by appending the argument to the current\n working directory.\nUse `..` to go up a level.",
			[3].cname="clear",
			[3].info0="Clears the screen.",
			[3].info1=NULL,
			[4].cname="rm",
			[4].info0="Remove a file or directory.",
			[4].info1=NULL,
			[5].cname="echo",
			[5].info0="Print back a message from arguments.",
			[5].info1="Prints each argument using printf.\nArguments are seperated by a space when printed.",
			[6].cname="edit",
			[6].info0="View/Edit a text file.",
			[6].info1="First argument specifies the target file.\nWhen veiwing, press `esc` key to exit.\nWhen exiting, if changes were made then you will be prompted if you want to save those changes."
		};
		uint16_t i;
		if (arg[1]!=NULL){
			bool found;found=0;
			for (i=0;i<7;i++){
				if (strcmp(arg[1],command_list[i].cname)==0){
					found=1;
					const char* info;
					if ((info=command_list[i].info1)==NULL){
						info=command_list[i].info0;
					}
					printf("Extra information for ");
					makeColor(COLOR_TO_TEXT,COLOR_GREEN);
					printf("%s",command_list[i].cname);
					resetColor();
					printf(":\n%s\n",info);
					break;
				}
			}
			if (!found){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("%s",arg[1]);
				resetColor();
				printf(" is not a built in command.\n");
			}
		} else {
			uint16_t l;
			for (i=0;i<7;i++){
				makeColor(COLOR_TO_TEXT,COLOR_GREEN);
				printf("%s",command_list[i].cname);
				l=5u-strlen(command_list[i].cname);
				while (l--!=0u){printf(" ");}
				resetColor();
				printf(":%s\n",command_list[i].info0);
			}
		}
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
			printf("Could not find execution target\n");
			resetColor();
		} else if (isDir){
			makeColor(COLOR_TO_TEXT,COLOR_RED);
			printf("Execution target cannot be a directory\n");
			resetColor();
		} else {
			run_command_was_binary=1;
			rv=run_binary(len,arg);
			AfterBinaryRun:;
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
				printf("Warning: fflush failed\n");
				resetColor();
			}
			if (_freeNonKernel()){
				makeColor(COLOR_TO_TEXT,COLOR_RED);
				printf("Warning: heap corruption detected\n");
				resetColor();
			}
			printf("\n(%d)\n",rv);
			return rv;
		}
	}
	if (fflush(NULL)!=0){
		makeColor(COLOR_TO_TEXT,COLOR_RED);
		printf("Warning: fflush failed\n");
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
	all_open_files->working_directory=malloc(1);
	if (all_open_files->working_directory==NULL) goto alloc_fail;
	all_open_files->working_directory[0]=0;
	
	char* a;
	resetColor();
	makeColor(COLOR_TO_TEXT,COLOR_GREEN);
	printf("KernelConsoleShell is ready.\n");
	makeColor(COLOR_TO_TEXT,COLOR_BLUE);
	for (uint16_t i=0;i<80u;i++){printf("-");}
	resetColor();
	a=malloc(5);
	if (a==NULL) goto alloc_fail;
	memcpy(a,"help",5);
	goto parse_and_execute;
	while (1){
		makeColor(COLOR_TO_TEXT,COLOR_MAGENTA);
		printf("%s/$ ",all_open_files->working_directory);
		resetColor();
		a=calloc(260,1);
		if (a==NULL) goto alloc_fail;
		uint16_t i=0;
		int c;
		while ((c=fgetc(stdin))!='\n' && c!=EOF){
			a[i++]=c;
		}
		parse_and_execute:;
		char** arg=parse_to_argv(a);
		bool emptyCommand=(a[0]==0);
		free(a);
		if (parse_input_failed_from_alloc) goto alloc_fail;
		if (emptyCommand) continue;
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
