


#ifdef USE_ALT_ALLOC

#define ALT_ALLOC_BLK_SIZE 64 // 88 is just about the limit, but a little less generally uses less memory
/*
Because of the way that malloc,calloc,realloc,free are often used 
(especially in the compiler), it can result in heap fragmentation.
This implementation is used to help prevent that 
(at the cost of a small amount of computational overhead)
*/
#define ALT_ALLOC_INSTRUCTION_BUFFER_COUNT 64

struct {
	struct {
		InstructionSingle buffer[INIT_INSTRUCTION_BUFFER_SIZE];
	} buffers[ALT_ALLOC_INSTRUCTION_BUFFER_COUNT];
	bool isTaken[ALT_ALLOC_INSTRUCTION_BUFFER_COUNT];
} cosmic_instruction_single_buffers = {0};







struct cosmic_alloc_block{
	uint8_t entries[ALT_ALLOC_BLK_SIZE*8][ALT_ALLOC_BLK_SIZE];
	uint8_t isNotTaken[ALT_ALLOC_BLK_SIZE];
	struct cosmic_alloc_block* next;
	struct cosmic_alloc_block* prev;
};
struct {
	struct cosmic_alloc_block* current;
	uint16_t total;
} cosmic_alloc_global;


void* cosmic_malloc(size_t size){
	if (size==INIT_INSTRUCTION_BUFFER_SIZE*sizeof(InstructionSingle)){
		for (uint8_t i=0;i<ALT_ALLOC_INSTRUCTION_BUFFER_COUNT;i++){
			if (!cosmic_instruction_single_buffers.isTaken[i]){
				cosmic_instruction_single_buffers.isTaken[i]=true;
				return cosmic_instruction_single_buffers.buffers[i].buffer;
			}
		}
		// if this position is reached, then there are more then ALT_ALLOC_INSTRUCTION_BUFFER_COUNT many instruction buffers that are of the initial size
		// it's fine if this position is reached, by the way
	}
	if (size>ALT_ALLOC_BLK_SIZE) {
		void* r=malloc(size);
		if (r==NULL){
			printf("malloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
			exit(3);
		}
		return r;
	}
	struct cosmic_alloc_block* localCurrent=cosmic_alloc_global.current;
	if (localCurrent!=NULL) goto find;
	progress:
	if (localCurrent==cosmic_alloc_global.current){
		localCurrent=calloc(1,sizeof(struct cosmic_alloc_block));
		if (localCurrent==NULL){
			printf("calloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
			exit(3);
		}
		if (cosmic_alloc_global.current!=NULL){
			localCurrent->next=cosmic_alloc_global.current->next;
			cosmic_alloc_global.current->next->prev=localCurrent;
			cosmic_alloc_global.current->next=localCurrent;
			localCurrent->prev=cosmic_alloc_global.current;
		} else {
			cosmic_alloc_global.current=localCurrent;
			localCurrent->next=localCurrent;
			localCurrent->prev=localCurrent;
		}
		for (uint16_t i=0;i<ALT_ALLOC_BLK_SIZE;i++){
			localCurrent->isNotTaken[i]=0xFF;
		}
	}
	find:;
	uint16_t i=ALT_ALLOC_BLK_SIZE;
	while (i--){
		if (localCurrent->isNotTaken[i]){
			goto alloc;
		}
	}
	localCurrent=localCurrent->next;
	goto progress;
	alloc:
	cosmic_alloc_global.current=localCurrent;
	uint8_t isNotTaken=localCurrent->isNotTaken[i];
	uint16_t e=7;
	uint16_t v=1;
	while (true){
		if (isNotTaken&v) goto finish;
		v*=2;e--;
	}
	finish:
	localCurrent->isNotTaken[i]^=v;
	void* ret=(void*)((uint8_t*)&localCurrent->entries+sizeof(uint8_t[ALT_ALLOC_BLK_SIZE])*(uint32_t)((7-e)+i*8));
	return ret;
}

void* cosmic_calloc(size_t nmemb,size_t size){
	size_t total=nmemb*size;
	uint8_t* ret=cosmic_malloc(total);
	for (size_t i=0;i<total;i++) ret[i]=0;
	return ret;
}

void cosmic_ptr_hit(void* ptr,struct cosmic_alloc_block* hit){
	uint32_t diff=(uint32_t)((uint8_t*)ptr-(uint8_t*)&hit->entries);
	assert((uint32_t)diff==(uint16_t)diff); // if this is false then the type size for diff on the cast needs to be larger
	uint16_t entry=(uint16_t)diff/(uint16_t)sizeof(uint8_t[ALT_ALLOC_BLK_SIZE]);
	hit->isNotTaken[entry/8U]^=1U<<(entry&7U);
}

void cosmic_free(void* ptr){
	if (ptr==NULL) return;
	if ((uint8_t*)ptr>=(uint8_t*)(cosmic_instruction_single_buffers.buffers[0].buffer) & 
		(uint8_t*)ptr<=(uint8_t*)(cosmic_instruction_single_buffers.buffers[ALT_ALLOC_INSTRUCTION_BUFFER_COUNT-1].buffer)){
		
		for (uint8_t i=0;i<ALT_ALLOC_INSTRUCTION_BUFFER_COUNT;i++){
			if (cosmic_instruction_single_buffers.buffers[i].buffer==ptr){
				assert(cosmic_instruction_single_buffers.isTaken[i]);
				cosmic_instruction_single_buffers.isTaken[i]=false;
				return;
			}
		}
		assert(false); // this statement should not be reached
	}
	struct cosmic_alloc_block* localCurrent=cosmic_alloc_global.current;
	do {
		if ((uint8_t*)ptr>=(uint8_t*)&localCurrent->entries & (uint8_t*)ptr<(uint8_t*)&localCurrent->isNotTaken){
			cosmic_ptr_hit(ptr,localCurrent);
			return;
		}
		localCurrent=localCurrent->next;
	} while (localCurrent!=cosmic_alloc_global.current);
	free(ptr);
	return;
}

void* cosmic_realloc(void* ptr,size_t size){
	if ((uint8_t*)ptr>=(uint8_t*)(cosmic_instruction_single_buffers.buffers[0].buffer) & 
		(uint8_t*)ptr<=(uint8_t*)(cosmic_instruction_single_buffers.buffers[ALT_ALLOC_INSTRUCTION_BUFFER_COUNT-1].buffer)){
		
		if (size==INIT_INSTRUCTION_BUFFER_SIZE*sizeof(InstructionSingle)){
			return ptr;
		}
		for (uint8_t i=0;i<ALT_ALLOC_INSTRUCTION_BUFFER_COUNT;i++){
			if (cosmic_instruction_single_buffers.buffers[i].buffer==ptr){
				assert(cosmic_instruction_single_buffers.isTaken[i]);
				cosmic_instruction_single_buffers.isTaken[i]=false;
				uint8_t* new=malloc(size);
				if (new==NULL){
					printf("malloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
					exit(3);
				}
				for (size_t c=0;c<size;c++) new[c]=((uint8_t*)ptr)[c];
				return new;
			}
		}
		assert(false); // this statement should not be reached
	}
	struct cosmic_alloc_block* localCurrent=cosmic_alloc_global.current;
	do {
		if ((uint8_t*)ptr>=(uint8_t*)&localCurrent->entries & (uint8_t*)ptr<(uint8_t*)&localCurrent->isNotTaken){
			if (size>ALT_ALLOC_BLK_SIZE){
				uint8_t* new=cosmic_malloc(size);
				for (uint16_t i=0;i<ALT_ALLOC_BLK_SIZE;i++) new[i]=((uint8_t*)ptr)[i];
				cosmic_ptr_hit(ptr,localCurrent);
				return new;
			} else {
				return ptr;
			}
		}
		localCurrent=localCurrent->next;
	} while (localCurrent!=cosmic_alloc_global.current);
	void* r=realloc(ptr,size);
	if (r==NULL){
		printf("realloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
		exit(3);
	}
	return r;
}

#else

void* cosmic_malloc(size_t size){
	void* r=malloc(size);
	if (r==NULL){
		printf("malloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
		exit(3);
	}
	return r;
}
void* cosmic_calloc(size_t nmemb,size_t size){
	void* r=calloc(nmemb,size);
	if (r==NULL){
		printf("calloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
		exit(3);
	}
	return r;
}
void cosmic_free(void* ptr){
	if (ptr!=NULL) free(ptr);
}
void* cosmic_realloc(void* ptr,size_t size){
	void* r=realloc(ptr,size);
	if (r==NULL){
		printf("realloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
		exit(3);
	}
	return r;
}

#endif


