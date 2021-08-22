
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define _MemorySize 67108864lu // 2**26
#define _AlignExtend(size) (((size)+7u)&0x03FFFFFClu)
#define _MemoryRead32(addr) (*((uint32_t*)(addr)))
#define _MemoryWrite32(addr,val) (*((uint32_t*)(addr)))=((uint32_t)(val))

static uint32_t _heapStart;
static bool _isKernelExecuting;
static bool _isNextAllocFromKernel;


// _freeNonKernel should only be called by the kernel, it will effectively free any allocations made by applications
// returns 0 if no heap corruption was detected, 1 if heap corruption was detected.
bool _freeNonKernel(){
	if (_heapStart==0u) return 0;
	if (!_isKernelExecuting) return 0;
	uint32_t currentBlockAddr,currentBlockSize,nextBlockAddr,nextBlockSize;
	bool isCurrentAllocated,isNextAllocated;
	
	currentBlockAddr=_heapStart;
	currentBlockSize=_MemoryRead32(currentBlockAddr);
	if ((currentBlockSize&0x80000001lu)==1lu){
		_MemoryWrite32(currentBlockAddr,_MemoryRead32(currentBlockAddr)&0x7FFFFFFElu);
	}
	currentBlockSize&=0x7FFFFFFElu;
	
	while (1){
		if (currentBlockSize==0u | (currentBlockSize&3u)!=0u | (currentBlockAddr&3u)!=0u) return 1;
		
		nextBlockAddr=currentBlockSize+currentBlockAddr;
		if (nextBlockAddr>=_MemorySize) return 1;
		
		if (nextBlockAddr==_MemorySize) break;
		nextBlockSize=_MemoryRead32(nextBlockAddr);
		if ((nextBlockSize&0x80000001lu)==1lu){
			_MemoryWrite32(nextBlockAddr,_MemoryRead32(nextBlockAddr)&0x7FFFFFFElu);
		}
		nextBlockSize&=0x7FFFFFFElu;
		currentBlockAddr=nextBlockAddr;
		currentBlockSize=nextBlockSize;
	}
	// this next part isn't necessary, but I am doing it anyway. It merges all adjacent free blocks.
	currentBlockAddr=_heapStart;
	currentBlockSize=_MemoryRead32(currentBlockAddr);
	isCurrentAllocated=currentBlockSize&1u;
	currentBlockSize^=isCurrentAllocated;
	currentBlockSize&=0x7FFFFFFFlu;
	while (1){
		if (currentBlockSize==0u | (currentBlockSize&3u)!=0u | (currentBlockAddr&3u)!=0u) return 1;
		
		nextBlockAddr=currentBlockSize+currentBlockAddr;
		if (nextBlockAddr>=_MemorySize) return nextBlockAddr!=_MemorySize;
		
		nextBlockSize=_MemoryRead32(nextBlockAddr);
		isNextAllocated=nextBlockSize&1u;
		nextBlockSize^=isNextAllocated;
		nextBlockSize&=0x7FFFFFFFlu;
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
}


void* malloc(uint32_t size){
	if (_heapStart==0u){
		uint32_t s=_MemorySize-(_heapStart=_MemoryRead32(0u));
		uint8_t allign=s&2u;
		s-=allign;
		_heapStart+=allign;
		_MemoryWrite32(_heapStart,s);
	}
	uint32_t bsize,currentBlockAddr,currentBlockSize,bestBlockAddr,bestBlockSize,nextBlockAddr,nextBlockSize;
	bool isCurrentAllocated,isNextAllocated;
	
	bsize=_AlignExtend(size);
	currentBlockAddr=_heapStart;
	currentBlockSize=_MemoryRead32(currentBlockAddr);
	isCurrentAllocated=currentBlockSize&1u;
	currentBlockSize^=isCurrentAllocated;
	currentBlockSize&=0x7FFFFFFFlu;
	
	bestBlockAddr=0u;
	bestBlockSize=0xFFFFFFF0lu;
	while (1){
		if (currentBlockSize==0u | (currentBlockSize&3u)!=0u | (currentBlockAddr&3u)!=0u){
			// heap corruption
			_isNextAllocFromKernel=0;
			exit(0x0373);
		}
		if (!isCurrentAllocated & currentBlockSize>=bsize & currentBlockSize<bestBlockSize){
			// then this is a better block then the one currently stored, so replace it
			bestBlockAddr=currentBlockAddr;
			bestBlockSize=currentBlockSize;
			if (bestBlockSize-8u<=bsize){
				// if size of free entry nearly (or exactly) matches, then take it right away
				break;
			}
		}
		
		nextBlockAddr=currentBlockSize+currentBlockAddr;
		if (nextBlockAddr>_MemorySize){
			// heap corruption
			_isNextAllocFromKernel=0;
			exit(0x0374);
		}
		
		if (nextBlockAddr==_MemorySize) break;
		nextBlockSize=_MemoryRead32(nextBlockAddr);
		isNextAllocated=nextBlockSize&1u;
		nextBlockSize^=isNextAllocated;
		nextBlockSize&=0x7FFFFFFFlu;
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
		_isNextAllocFromKernel=0;
		return NULL;
	}
	uint32_t leftoverSize=bestBlockSize-bsize;
	if (leftoverSize<=16u){
		// if the best block size is close (or exact), just pretend the asked-for size is exactly the block size
		_MemoryWrite32(bestBlockAddr,((_isKernelExecuting|_isNextAllocFromKernel)?0x80000000lu:0x0lu)|(bestBlockSize^1u));
		_isNextAllocFromKernel=0;
		return (void*)(bestBlockAddr+4u);
	} else {
		_MemoryWrite32(bestBlockAddr+bsize,leftoverSize);
		_MemoryWrite32(bestBlockAddr,((_isKernelExecuting|_isNextAllocFromKernel)?0x80000000lu:0x0lu)|((bestBlockSize-leftoverSize)^1u));
		_isNextAllocFromKernel=0;
		return (void*)(bestBlockAddr+4u);
	}
}

void* calloc(uint32_t numitems, uint32_t itemSize){
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

void free(void* ptr){
	if (ptr==NULL) return;
	uint32_t* tp=(uint32_t*)ptr-1u;
	uint32_t t;
	t=*tp;*tp=(t^1u)&0x7FFFFFFFlu;
	if ((t&1u)^1u){
		// ptr already free-d or heap corruption
		exit(0x0376);
	}
}

// having applications call realloc on a pointer allocated by the kernel should be avoided
void* realloc(void* ptr, uint32_t size_new){
	if (ptr==NULL) return malloc(size_new);
	uint32_t bsize_new=_AlignExtend(size_new);
	uint32_t bsize_old=(*((uint32_t*)ptr-1u)^1u)&0x7FFFFFFFlu;
	if ((bsize_old&3u)!=0u | ((uint32_t)ptr&3u)!=0u){
		// ptr already free-d or heap corruption
		_isNextAllocFromKernel=0;
		exit(0x0377);
	}
	if (bsize_new==bsize_old | (bsize_new<bsize_old & bsize_old-bsize_new<=16u)){
		// close enough to ignore the request
		_isNextAllocFromKernel=0;
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
