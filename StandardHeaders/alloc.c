
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
static int printf(const char* format, ...);

#define _MemorySize 67108864LU // 2**26
#define _AlignExtend(size) (((size)+7U)&0x03FFFFFC)
#define _MemoryRead32(addr) (*((uint32_t*)(addr)))
#define _MemoryWrite32(addr,val) (*((uint32_t*)(addr)))=((uint32_t)(val))

#if 0
//#define _HeapFreeEntryCount 65536
#define _HeapFreeEntryCount 2048
//#define _HeapFreeEntryCount 256

static struct _HeapFreeEntries{
	struct _HeapFreeEntry{
		uint32_t size; // size (in bytes) of this free section
		uint32_t start; // start address (lowest address) of this free section
	} entries[_HeapFreeEntryCount];
	uint32_t begin;
} _hfe;



static void* malloc(uint32_t size){
	if (_hfe.begin==0){
		// bootstrap the heap then
		_hfe.begin=_MemoryRead32(0);
		_hfe.entries[0].start=_hfe.begin;
		_hfe.entries[0].size=_MemorySize-_hfe.begin;
	}
	uint32_t rsize = _AlignExtend(size);
	struct _HeapFreeEntry* end=_hfe.entries+_HeapFreeEntryCount;
	struct _HeapFreeEntry* target=NULL;
	{
		struct _HeapFreeEntry* i;
		for (i=_hfe.entries;i!=end;i++){
			if (i->size>=rsize){
				target=i;
				break;
			}
		}
		if (target==NULL) return NULL; // then there is no space for that allocation
		for (;i!=end;i++){
			if (i->size>=rsize && target->size>=i->size){
				target=i;
				if (target->size==rsize) break; // then a perfectly sized free entry is found
			}
		}
	}
	void* uptr=(void*)(target->start+4U);
	_MemoryWrite32(target->start,rsize);
	target->size-=rsize;
	target->start+=rsize;
	//printf("+%lu,%lu\n",uptr,rsize);
	
	return uptr;
}

static void* calloc(uint32_t nitems, uint32_t size){
	uint32_t usize = nitems*size;
	void* ptr = malloc(usize);
	if (ptr==NULL) return NULL;
	for (uint32_t i=0;i<usize;i+=4) {
		_MemoryWrite32((uint32_t)ptr+i,0U);
	}
	return ptr;
}

static void free(void* ptr){
	//printf("-%lu\n",ptr);
	
	if (ptr==NULL) return;
	uint32_t rstart=(uint32_t)(((uint32_t*)ptr)-1U);
	uint32_t rsize=_MemoryRead32(rstart);
	uint32_t rend=rstart+rsize;
	struct _HeapFreeEntry* end=_hfe.entries+_HeapFreeEntryCount;
	struct _HeapFreeEntry* targetAtStart=NULL;
	struct _HeapFreeEntry* targetAtEnd=NULL;
	struct _HeapFreeEntry* targetZero=NULL;
	struct _HeapFreeEntry* i;
	for (i=_hfe.entries;i!=end;i++){
		if (i->size!=0){
			uint32_t eStart=i->start;
			uint32_t eEnd=eStart+i->size;
			if (eStart==rend) targetAtEnd=i;
			else if (eEnd==rstart) targetAtStart=i; 
		} else targetZero=i;
	}
	if (targetAtStart==NULL & targetAtEnd==NULL){
		if (targetZero==NULL){
			*((volatile char*)0x04030000)='!';
			*((volatile char*)0x04030000)='!';
			*((volatile char*)0x04030000)='!';
			*((volatile char*)0x04030000)='!';
			
			/*
			then an entry will need to be created.
			This is a relatively complex operation, and will hopefully be avoided in the typical case
			targetZero will be used to find the smallest sized free entry,
			which will then be made into a zero entry.
			
			targetAtStart will be used for the entry that is at the start side of the targetZero free entry
			  no free entry will exist next to another free entry, there will be allocations in between
			  it is possible that there is no free entry on the left or right side
			*/
			targetZero=_hfe.entries;
			for (i=_hfe.entries;i!=end;i++){
				if (i->size<targetZero->size) targetZero=i;
			}
			// targetZero found
			uint32_t targetZeroStart = targetZero->start;
			for (i=_hfe.entries;i!=end;i++){
				if (i->start<targetZeroStart){
					targetAtStart=i;
					break;
				}
			}
			for (;i!=end;i++){
				if (i->start<targetZeroStart && targetAtStart->start<i->start) targetAtStart=i;
			}
			// targetAtStart is either found or doesn't exist
			uint32_t walk;
			if (targetAtStart!=NULL) walk=targetAtStart->start+targetAtStart->size;
			else walk=_hfe.begin;
			while (walk<targetZeroStart){
				uint32_t tempWalk = walk+_MemoryRead32(walk);
				if ((tempWalk&3)!=0 | walk==tempWalk) break;
				if (targetZeroStart==tempWalk){
					// then it is the one before targetZero, so expand that allocation
					_MemoryWrite32(walk,_MemoryRead32(walk)+targetZero->size);
					// then set that entry and done
					goto MakeEntry;
				}
				walk=tempWalk;
			}
			exit(0x0374);
		}
		MakeEntry:
		
		targetZero->start=rstart;
		targetZero->size=rsize;
	} else if (targetAtStart!=NULL){
		targetAtStart->size+=rsize;
		if (targetAtEnd!=NULL){
			targetAtStart->size+=targetAtEnd->size;
			targetAtEnd->size=0;
		}
	} else {
		targetAtEnd->size+=rsize;
		targetAtEnd->start-=rsize;
	}
}




static void* realloc(void* ptr, uint32_t size){
	if (ptr==NULL) return malloc(size);
	uint32_t rstartOld = (uint32_t)(((uint32_t*)ptr)-1U);
	uint32_t rsizeOld=_MemoryRead32(rstartOld);
	uint32_t rsizeNew = _AlignExtend(size);
	uint32_t rendOld=rstartOld+rsizeOld;
	if (rsizeNew<=rsizeOld & rsizeOld-rsizeNew<=8) return ptr; // then it is not needed and not worth it
	if (rsizeNew<rsizeOld){
		// then this allocation is shrinked, then a new allocation is created then freed
		_MemoryWrite32(rendOld,rsizeOld-rsizeNew);
		_MemoryWrite32(rstartOld,rsizeNew);
		free((void*)(rendOld+4U));
		return ptr;
	} else {
		uint32_t rsizeDelta = rsizeNew-rsizeOld;
		struct _HeapFreeEntry* end=_hfe.entries+_HeapFreeEntryCount;
		struct _HeapFreeEntry* targetAtStartOld=NULL;
		struct _HeapFreeEntry* targetAtEndOld=NULL;
		struct _HeapFreeEntry* targetZero=NULL;
		uint32_t rstartNew;
		bool doFree=false;
		for (struct _HeapFreeEntry* i=_hfe.entries;i!=end;i++){
			if (i->size!=0){
				uint32_t eStart=i->start;
				uint32_t eEnd=eStart+i->size;
				if (eStart==rendOld) targetAtEndOld=i;
				else if (eEnd==rstartOld) targetAtStartOld=i;
			} else targetZero=i;
		}
		if (targetAtEndOld!=NULL && targetAtEndOld->size>=rsizeDelta){
			_MemoryWrite32(rstartOld,rsizeNew);
			targetAtEndOld->size-=rsizeDelta;
			targetAtEndOld->start+=rsizeDelta;
			return ptr;
		}
		if (targetAtStartOld!=NULL && targetAtStartOld->size>=rsizeDelta){
			rstartNew = rstartOld-rsizeDelta;
			_MemoryWrite32(rstartNew,rsizeNew);
			targetAtStartOld->size-=rsizeDelta;
			goto ShiftMem;
		}
		if ((targetAtStartOld!=NULL & targetAtEndOld!=NULL) && targetAtStartOld->size+targetAtEndOld->size>=rsizeDelta){
			rstartNew = rstartOld-targetAtStartOld->size;
			_MemoryWrite32(rstartNew,rsizeNew);
			targetAtStartOld->size=0;
			uint32_t extraAmount = rsizeDelta-targetAtStartOld->size;
			targetAtEndOld->size-=extraAmount;
			targetAtEndOld->start+=extraAmount;
			goto ShiftMem;
		}
		doFree=true;
		rstartNew = (uint32_t)malloc(rsizeNew-4U);
		
		ShiftMem:
		for (uint32_t rsizeWalk=4U;rsizeWalk<rsizeOld;rsizeWalk+=4U){
			_MemoryWrite32(rsizeWalk+rstartNew,_MemoryRead32(rsizeWalk+rstartOld));
		}
		if (doFree) free(ptr);
		return (void*)(rstartNew+4U);
	}
}
#else

static void memcpy(void* dest, const void* src, unsigned long n);

static uint32_t _heapStart;


static void* malloc(uint32_t size){
	if (_heapStart==0){
		uint32_t s=_MemorySize-(_heapStart=_MemoryRead32(0));
		uint8_t allign=s&2;
		s-=allign;
		_heapStart+=allign;
		_MemoryWrite32(_heapStart,s);
	}
	uint32_t prevBlockAddr=0;
	uint32_t prevBlockSize=0;
	
	uint32_t bsize=_AlignExtend(size);
	uint32_t currentBlockAddr=_heapStart;
	uint32_t currentBlockSize=_MemoryRead32(currentBlockAddr);
	bool isCurrentAllocated=currentBlockSize&1;
	currentBlockSize^=isCurrentAllocated;
	
	uint32_t bestBlockAddr=0;
	uint32_t bestBlockSize=0xFFFFFFFF;
	while (1){
		if ((currentBlockSize&3)!=0){
			// heap corruption
			exit(0x0372);
		}
		if (currentBlockSize==0){
			// heap corruption
			exit(0x0373);
		}
		if (!isCurrentAllocated & currentBlockSize>=bsize & currentBlockSize<=bestBlockSize){
			// then this is a better block [or identical, which doesn't matter] then the one currently stored, so replace it
			bestBlockAddr=currentBlockAddr;
			bestBlockSize=currentBlockSize;
			if (bestBlockSize==bsize){
				// if size of free entry matches perfectly, that take it right away
				break;
			}
		}
		uint32_t nextBlockAddr;
		uint32_t nextBlockSize;
		bool isNextAllocated;
		
		
		nextBlockAddr=currentBlockSize+currentBlockAddr;
		if (nextBlockAddr>_MemorySize){
			// heap corruption
			exit(0x0374);
		}
		
		prevBlockAddr=currentBlockAddr;
		prevBlockSize=currentBlockSize;
		
		if (nextBlockAddr==_MemorySize) break;
		nextBlockSize=_MemoryRead32(nextBlockAddr);
		isNextAllocated=nextBlockSize&1;
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
	if (bestBlockAddr==0){
		// no space
		return NULL;
	}
	uint32_t leftoverSize=bestBlockSize-bsize;
	if (leftoverSize<=8){
		// if the best block size is close [or exact], just pretend the asked-for size is exactly the block size
		_MemoryWrite32(bestBlockAddr,bestBlockSize^1);
		return (void*)(bestBlockAddr+4);
	} else {
		_MemoryWrite32(bestBlockAddr+bsize,leftoverSize);
		_MemoryWrite32(bestBlockAddr,(bestBlockSize-leftoverSize)^1);
		return (void*)(bestBlockAddr+4);
	}
}

static void* calloc(uint32_t numitems, uint32_t itemSize){
	uint32_t size=numitems*itemSize;
	uint32_t* ptr=malloc(size);
	if (ptr==NULL){
		return NULL;
	}
	// inside of calloc, we can do the memory setting faster then memset() because allignments are already known to not be an issue as well as the value is always 0
	uint32_t* ptre=(uint32_t*)((uint8_t*)ptr+(_AlignExtend(size)-4));
	for (uint32_t* ptri=ptr;ptri!=ptre;ptri++){
		*ptri=0;
	}
	return ptr;
}

static void free(void* ptr){
	if (ptr==NULL) return;
	uint32_t* tp=(uint32_t*)ptr-1;
	uint32_t t=*tp;
	assert(t==*((uint32_t*)ptr-1));
	*tp=t^1;
	if ((t&1)^1){
		// ptr already free-d or heap corruption
		exit(0x0376);
	}
}

static void* realloc(void* ptr, uint32_t size_new){
	if (ptr==NULL) return malloc(size_new);
	uint32_t bsize_new=_AlignExtend(size_new);
	uint32_t bsize_old=*((uint32_t*)ptr-1)^1;
	if (bsize_old&1){
		// ptr already free-d or heap corruption
		exit(0x0377);
	}
	if (bsize_new==bsize_old){
		return ptr;
	}
	uint32_t bsize_copy=bsize_old>bsize_new?bsize_new:bsize_old;
	// I'm not doing anything clever right now
	void* ptr_new=malloc(size_new);
	memcpy(ptr_new,ptr,bsize_copy-4);
	free(ptr);
	return ptr_new;
}





#endif



#undef _MemorySize
#undef _AlignExtend(size)
#undef _MemoryRead32(addr)
#undef _MemoryWrite32(addr,val)
