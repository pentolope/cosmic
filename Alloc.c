


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


#if 0
struct {
	struct Heap_Track_Item{size_t p;size_t s;}* items;
	size_t len;
	size_t alen;
	size_t max;
	size_t size;
} heap_track;

void heap_track_update(){
	heap_track.size=0;
	for (size_t i=0;i<heap_track.len;i++){
		heap_track.size+=heap_track.items[i].s;
	}
	if (heap_track.size>heap_track.max) heap_track.max=heap_track.size;
	//printf("%lu\n",(unsigned long)heap_track.size);
}

void heap_track_add(size_t p,size_t s){
	if (p==0) return;
	struct Heap_Track_Item t={.p=p,.s=s};
	if (heap_track.len==heap_track.alen){
		heap_track.alen+=100;
		heap_track.items = realloc(heap_track.items,heap_track.alen*sizeof(struct Heap_Track_Item));
	}
	heap_track.items[heap_track.len++]=t;
	heap_track_update();
}

void heap_track_modify(size_t p0,size_t p1,size_t s){
	if (p0==0){
		heap_track_add(p1,s);
		return;
	}
	for (size_t i=0;i<heap_track.len;i++){
		if (heap_track.items[i].p==p0){
			heap_track.items[i].p=p1;
			heap_track.items[i].s=s;
			heap_track_update();
			return;
		}
	}
	assert(false);
}

void heap_track_remove(size_t p){
	for (size_t i=0;i<heap_track.len;i++){
		if (heap_track.items[i].p==p){
			for (i++;i<heap_track.len;i++){
				heap_track.items[i-1]=heap_track.items[i];
			}
			heap_track.len--;
			heap_track_update();
			return;
		}
	}
	assert(false);
}
#endif

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
	if (size>ALT_ALLOC_BLK_SIZE){
		void* r=malloc(size);
		//heap_track_add((size_t)r,size);
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
		//heap_track_add((size_t)localCurrent,sizeof(struct cosmic_alloc_block));
		if (localCurrent==NULL){
			printf("calloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)sizeof(struct cosmic_alloc_block));
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
		memset(&(localCurrent->isNotTaken),0xFF,ALT_ALLOC_BLK_SIZE);
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
	return (void*)((uint8_t*)&localCurrent->entries+sizeof(uint8_t[ALT_ALLOC_BLK_SIZE])*(uint32_t)((7-e)+i*8));
}

void* cosmic_calloc(size_t nmemb,size_t size){
	size_t total=nmemb*size;
	uint8_t* ret=cosmic_malloc(total);
	memset(ret,0,total);
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
	//heap_track_remove((size_t)ptr);
	free(ptr);
	return;
}

void* cosmic_realloc(void* ptr,size_t size){
	if (ptr==NULL) return cosmic_malloc(size);
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
				//heap_track_add((size_t)new,size);
				if (new==NULL){
					printf("malloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)size);
					exit(3);
				}
				memcpy(new,ptr,INIT_INSTRUCTION_BUFFER_SIZE*sizeof(InstructionSingle));
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
				memcpy(new,ptr,ALT_ALLOC_BLK_SIZE);
				cosmic_ptr_hit(ptr,localCurrent);
				return new;
			} else {
				return ptr;
			}
		}
		localCurrent=localCurrent->next;
	} while (localCurrent!=cosmic_alloc_global.current);
	void* r=realloc(ptr,size);
	//heap_track_modify((size_t)ptr,(size_t)r,size);
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
		printf("calloc() failed to allocate %lu bytes. Exiting now.\n",(unsigned long)(nmemb*size));
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


