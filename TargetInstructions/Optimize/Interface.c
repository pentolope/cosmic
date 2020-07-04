
void printInstructionBufferWithMessageAndNumber(const InstructionBuffer*,const char*,const uint32_t);
void printSingleInstructionOptCode(const InstructionSingle);


#include "Passive.c"
#include "PeepHole.c"
#include "Modify.c"
#include "Active.c"



/*
all functions here modify their input
may read settings from the CompileSettings struct in "Common.c" back in the compiler

*/


void runOptimizerOnFunctionPriorToGlobalIntegration(InstructionBuffer *ib){
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("Before Opt:%d:\n",ib->numberOfSlotsTaken);
#endif
	expandPushPop(ib);
	attemptAllActiveOptPhase1(ib);
	attemptAllActiveOptPhase2(ib);
	contractPushPop(ib);
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("After Opt:%d:\n",ib->numberOfSlotsTaken);
#endif
}



