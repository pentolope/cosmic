
void printInstructionBufferWithMessageAndNumber(const InstructionBuffer*,const char*,const uint32_t);
void printSingleInstructionOptCode(const InstructionSingle);


#include "Passive.c"
#include "PeepHole.c"
#include "Modify.c"
#include "Active.c"



/*
all functions here modify their input
may read settings from the CompileSettings struct in "Common.c"

*/


void runOptimizerOnFunctionPriorToGlobalIntegration(InstructionBuffer *ib){
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("Before Opt:%d:\n",ib->numberOfSlotsTaken);
#endif
	expandPushPop(ib);
	if (compileSettings.optLevel>0) attemptAllActiveOptPhase1(ib);
	if (compileSettings.optLevel>1) attemptAllActiveOptPhase2(ib);
	contractPushPop(ib);
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("After Opt:%d:\n",ib->numberOfSlotsTaken);
#endif
}



