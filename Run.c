
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void* cosmic_malloc(size_t size);
void* cosmic_calloc(size_t nmemb,size_t size);
void  cosmic_free(void* ptr);
void* cosmic_realloc(void* ptr,size_t size);
#define IS_BUILDING_RUN
#include "TargetInstructions/InstructionBase.c"
#include "Alloc.c"
#include "PrintInstructionBuffer.c"


int main(int argc, char** argv){
	
}






