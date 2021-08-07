
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define _MemorySize 67108864lu // 2**26
#define _AlignExtend(size) (((size)+7u)&0x03FFFFFClu)
#define _MemoryRead32(addr) (*((uint32_t*)(addr)))
#define _MemoryWrite32(addr,val) (*((uint32_t*)(addr)))=((uint32_t)(val))


static void memcpy(void* dest, const void* src, unsigned long n);

static uint32_t _heapStart;

static void* malloc(uint32_t size){
	if (_heapStart==0u){
		uint32_t s=_MemorySize-(_heapStart=_MemoryRead32(0u));
		uint8_t allign=s&2u;
		s-=allign;
		_heapStart+=allign;
		_MemoryWrite32(_heapStart,s);
	}
	uint32_t prevBlockAddr,prevBlockSize,bsize,currentBlockAddr,currentBlockSize,bestBlockAddr,bestBlockSize,nextBlockAddr,nextBlockSize;
	bool isCurrentAllocated,isNextAllocated;
	prevBlockAddr=0u;
	prevBlockSize=0u;
	
	bsize=_AlignExtend(size);
	currentBlockAddr=_heapStart;
	currentBlockSize=_MemoryRead32(currentBlockAddr);
	isCurrentAllocated=currentBlockSize&1u;
	currentBlockSize^=isCurrentAllocated;
	
	bestBlockAddr=0u;
	bestBlockSize=0xFFFFFFF0lu;
	while (1){
		if (currentBlockSize==0u | (currentBlockSize&3u)!=0u | (currentBlockAddr&3u)!=0u){
			// heap corruption
			exit(0x0373);
		}
		if (!isCurrentAllocated & currentBlockSize>=bsize & currentBlockSize<bestBlockSize){
			// then this is a better block then the one currently stored, so replace it
			bestBlockAddr=currentBlockAddr;
			bestBlockSize=currentBlockSize;
			if (bestBlockSize-8u<=bsize){
				// if size of free entry at least nearly matches, that take it right away
				break;
			}
		}
		
		nextBlockAddr=currentBlockSize+currentBlockAddr;
		if (nextBlockAddr>_MemorySize){
			// heap corruption
			exit(0x0374);
		}
		
		prevBlockAddr=currentBlockAddr;
		prevBlockSize=currentBlockSize;
		
		if (nextBlockAddr==_MemorySize) break;
		nextBlockSize=_MemoryRead32(nextBlockAddr);
		isNextAllocated=nextBlockSize&1u;
		nextBlockSize^=isNextAllocated;
		if (!isCurrentAllocated & !isNextAllocated){
			// merge the two free blocks that are next to each other, restart loop on updated current block
			currentBlockSize+=nextBlockSize;
			_MemoryWrite32(currentBlockAddr,currentBlockSize);
			continue;
		}
		currentBlockAddr=nextBlockAddr;
		currentBlockSize=nextBlockSize;
		isCurrentAllocated=isNextAllocated;
	}
	if (bestBlockAddr==0u){
		// no space
		return NULL;
	}
	uint32_t leftoverSize=bestBlockSize-bsize;
	if (leftoverSize<=16u){
		// if the best block size is close [or exact], just pretend the asked-for size is exactly the block size
		_MemoryWrite32(bestBlockAddr,bestBlockSize^1u);
		return (void*)(bestBlockAddr+4u);
	} else {
		_MemoryWrite32(bestBlockAddr+bsize,leftoverSize);
		_MemoryWrite32(bestBlockAddr,(bestBlockSize-leftoverSize)^1u);
		return (void*)(bestBlockAddr+4u);
	}
}

static void* calloc(uint32_t numitems, uint32_t itemSize){
	uint32_t size=numitems*itemSize;
	uint32_t* ptr=malloc(size);
	if (ptr==NULL) return NULL;
	// inside of calloc, we can do the memory setting faster then memset() because allignments are already known to not be an issue as well as the value is always 0
	uint32_t* ptre=(uint32_t*)((uint8_t*)ptr+(_AlignExtend(size)-4u));
	for (uint32_t* ptri=ptr;ptri!=ptre;ptri++){
		*ptri=0u;
	}
	return ptr;
}

static void free(void* ptr){
	if (ptr==NULL) return;
	uint32_t* tp=(uint32_t*)ptr-1u;
	uint32_t t;
	t=*tp;*tp=t^1u;
	if ((t&1u)^1u){
		// ptr already free-d or heap corruption
		exit(0x0376);
	}
}

static void* realloc(void* ptr, uint32_t size_new){
	if (ptr==NULL) return malloc(size_new);
	uint32_t bsize_new=_AlignExtend(size_new);
	uint32_t bsize_old=*((uint32_t*)ptr-1u)^1u;
	if ((bsize_old&3u)!=0u | ((uint32_t)ptr&3u)!=0u){
		// ptr already free-d or heap corruption
		exit(0x0377);
	}
	if (bsize_new==bsize_old | (bsize_new<bsize_old & bsize_old-bsize_new<=16u)){
		// close enough to ignore the request
		return ptr;
	}
	// I'm not doing anything clever right now
	void* ptr_new=malloc(size_new);
	if (ptr_new==NULL) return NULL;
	uint32_t bsize_copy=(bsize_old>bsize_new?bsize_new:bsize_old)-4u;
	assert((bsize_copy&3u)==0u);
	uint32_t bsize_copy_left=bsize_copy;
	uint32_t* ptr_new_i=ptr_new;
	uint32_t* ptr_old_i=ptr;
	// inside of realloc, we can do the memory copying faster then memcpy() because allignments are already known to not be an issue
	while (bsize_copy_left!=0u){
		*(ptr_new_i++)=*(ptr_old_i++);
		bsize_copy_left-=4u;
	}
	free(ptr);
	return ptr_new;
}


#undef _MemorySize
#undef _AlignExtend(size)
#undef _MemoryRead32(addr)
#undef _MemoryWrite32(addr,val)
