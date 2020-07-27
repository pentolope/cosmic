

#define PH_VTEP_COUNT 44 // number of peephole templates
#define PH_VTEP_MAX_SYM_CONST 3 // maxiumum number of symbolic constants
#define PH_VTEP_MAX_IDEN 1 // maxiumum iden value in templates
#define PH_VTEP_MAX_DEPTH 5 // maxiumum (potentially UNSAFE) depth of VTE
#define PH_VTEP_SAFE_DEPTH 3 // maxiumum SAFE depth of VTE
#define PH_VTEP_LENGTH 84 // maxiumum number of VTE nodes
/*
PH_VTEP_LENGTH relation to PH_VTEP_SAFE_DEPTH

the max branching factor is 4 so PH_VTEP_LENGTH is:
     4**3+4**2+4**1 ( 84) with safe depth at 3
if PH_VTEP_SAFE_DEPTH wants to be changed, remember to change PH_VTEP_LENGTH
PH_VTEP_LENGTH should always be below 255, as there exists indexes that are stored as uint8_t
*/


/*
NOTE: the following instructions are not allowed in ValueTraceEntry trees:
-- all memory access
-- all symbolic constants
-- all raw data inserts
-- all push and pop
-- all jumps
-- all instructions that take more then 4 inputs
-- all instructions that have more then 2 outputs
-- all instructions that destroy registers

ALOC
ALCR
STOF
CALL
RET_

LABL

RL2_

FCST
FCEN



Additionally, all BL1_ will be represented as RL1_ in the ValueTraceEntry tree, 
and BL1_ should not exist in the templates

*/


typedef struct ValueTraceEntry{
	enum InstructionTypeID id;
	/*
	some special situations exist for id:
	(non-template) DEPL (used when the value cannot be traced throught this instruction)
	(    template) DEPL (used when the instruction does not matter, and template terminates)
	(    template) PEPH (
			it indicates a push or pop from an output, 
			it is NOT EXACTLY a traced value,
			it's a special placeholder,
			the array is transversed as if it has 1 input)
	
	
	*/
	uint8_t entryType;
	/*
	(entryType&0xF0)==0x00: (non-template) entry of instruction with inputs
	(entryType&0xF0)==0x10: (non-template) entry of instruction with constant load (generic symbolic-ness)
	
	(entryType&0xF0)==0x20: (    template) entry of instruction with inputs 
	(entryType&0xF0)==0x30: (    template) entry of instruction with constant load (non-symbolic)
	(entryType&0xF0)==0x40: (    template) entry of instruction with constant load (    symbolic)
	(entryType&0xF0)==0x50: (    template) POP1 the output of next node
	(entryType&0xF0)==0x60: (    template) PU1_ the output of next node
	
	(entryType&0xF0)==0xB0: DEPL
	
	
	(entryType&0x01)      : II.regOUT[entryType&0x01] is the value/register this trace is from 
                          (sometimes instructions have multiple outputs)
	
	(entryType&0x02)==0x00: (non-template) inputs are listed (if there are no inputs then this may be uninitialized)
	(entryType&0x02)==0x02: (non-template) inputs are not listed due to depth limit being reached
	
	(entryType&0x04)==0x00: (non-template) the other output of this instruction (if it exists) are NOT    used
	                        (    template) the other output of this instruction (if it exists) may     be used
	(entryType&0x04)==0x04: (non-template) the other output of this instruction (if it exists) are        used
	                        (    template) the other output of this instruction (if it exists) may NOT be used
	(entryType&0x08)==0x00: (non-template) this output of this instruction is  NOT    used in other instruction's inputs (does not include the usage used to generate the tree)
	                        (    template) this output of this instruction may     be used in other instruction's inputs (does not include the usage used to generate the tree)
	(entryType&0x08)==0x08: (non-template) this output of this instruction is         used in other instruction's inputs (does not include the usage used to generate the tree)
	                        (    template) this output of this instruction may NOT be used in other instruction's inputs (does not include the usage used to generate the tree)
	*/
	bool isSwapped; // (non-template)
	/*
	isSwapped is initialized to 0 by generateValueTraceEntries()
	flipped by swapValueTraceEntriesInputs() when it swaps the inputs
	(inputs may be swapped when trying to check for congruence with the template)
	*/
	uint8_t regOUT; // (non-template) the name of the register this instruction outputs to
	uint8_t nRegIN; // (non-template) the number of input registers to this instruction
	uint8_t iden;   // (    template) if non-zero, it indicates that all other instructions with the same iden value must have the same value for instr for this template to match
	uint16_t cv;    // (    generic ) cv is constant value, if applicable. (RL2_ is not allowed in ValueTraceEntry trees)
	uint32_t instr; // (non-template)
	/*
	instr is only used for non-template,
	it is index of the instruction in the `ib->buffer`
	*/
} ValueTraceEntry;

typedef struct InsertionInstructionPacket{
	const enum InstructionTypeID id;
	const uint8_t r0;
	const uint8_t r1;
	const uint8_t r2;
	const uint16_t cv;
} InsertionInstructionPacket;




const ValueTraceEntry VTE_arr_00[] = {
{I_PEPH,0x50},{I_XOR_,0x20},
 {I_XOR_,0x20},
  {I_RL1_,0x40,.cv=0},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x40,.cv=0}
};

const ValueTraceEntry VTE_arr_01[] = {
{I_PEPH,0x50},{I_AND_,0x20},
 {I_PEPH,0x60},{I_AND_,0x20},
  {I_RL1_,0x40,.cv=0},
  {I_DEPL,0xB0},
 {I_RL1_,0x40,.cv=0}
};

const ValueTraceEntry VTE_arr_02[] = {
{I_PEPH,0x50},{I_OR__,0x20},
 {I_PEPH,0x60},{I_OR__,0x20},
  {I_RL1_,0x40,.cv=0},
  {I_DEPL,0xB0},
 {I_RL1_,0x40,.cv=0}
};

const ValueTraceEntry VTE_arr_03[] = {
{I_PEPH,0x50},{I_SSUB,0x24},
 {I_RL1_,0x30,.cv=0},
 {I_PEPH,0x60},{I_SSUB,0x20},
  {I_RL1_,0x30,.cv=0},
  {I_DEPL,0xB0},
  {I_RL1_,0x30,.cv=0},
 {I_RL1_,0x30,.cv=0}
};

const ValueTraceEntry VTE_arr_09[] = { // this transformation is not always valid! if the result of the multiplication is larger then a word, then there will be different behavior.
{I_DIVM,0x24}, // this transformation is currently disabled because of the reason above
 {I_DIVM,0x2A},
  {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_09_0[] = {
{I_POP1,2,16,16},
{I_POP1,1,16,16},
{I_POP1,0,16,16},
{I_MULS,1, 2,16},
{I_DIVM,0, 1,16}
};

const InsertionInstructionPacket IIP_arr_09_1[] = {
{I_POP1,2,16,16},
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_MULS,1, 2,16},
{I_DIVM,0, 1,16}
};

const InsertionInstructionPacket IIP_arr_09_2[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_POP1,2,16,16},
{I_MULS,1, 2,16},
{I_DIVM,0, 1,16}
};


const ValueTraceEntry VTE_arr_0A[] = {
{I_PEPH,0x50},{I_DIVM,0x20},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=1}
};

const ValueTraceEntry VTE_arr_0B[] = {
{I_DIVM,0x25},
 {I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=1}
};

const InsertionInstructionPacket IIP_arr_0B[] = {
{I_RL1_,0,16,16,0}
};

const ValueTraceEntry VTE_arr_0C[] = {
{I_DIVM,0x24},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=2}
};

const InsertionInstructionPacket IIP_arr_0C[] = {
{I_POP1,0,16,16},
{I_SHFT,0, 0,16}
};

const ValueTraceEntry VTE_arr_0D[] = {
{I_DIVM,0x24},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=4}
};

const InsertionInstructionPacket IIP_arr_0D[] = {
{I_POP1,0,16,16},
{I_SHFT,0, 0,16},
{I_SHFT,0, 0,16}
};

const ValueTraceEntry VTE_arr_0F[] = {
{I_DIVM,0x24},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=8}
};

const InsertionInstructionPacket IIP_arr_0F[] = {
{I_POP1,0,16,16},
{I_SHFT,0, 0,16},
{I_SHFT,0, 0,16},
{I_SHFT,0, 0,16}
};

const ValueTraceEntry VTE_arr_10[] = {
{I_DIVM,0x24},
 {I_SHFT,0x28},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_10_0[] = {
{I_POP1,0,16,16},
{I_MOV_,0, 0,16},// essentially a nop, however it must not be a nop because of mechanics of insertInstructionAt()
{I_MOV_,0, 0,16},
{I_RL1_,1,16,16,2},
{I_MULS,1, 0,16},
{I_POP1,0,16,16},
{I_DIVM,0, 1,16}
};

const InsertionInstructionPacket IIP_arr_10_1[] = {
{I_POP1,1,16,16},
{I_POP1,0,16,16},
{I_PU1_,1,16,16},
{I_RL1_,1,16,16,2},
{I_MULS,1, 0,16},
{I_POP1,0,16,16},
{I_DIVM,0, 1,16}
};

const InsertionInstructionPacket IIP_arr_11_0[] = {
{I_POP1,0,16,16},
{I_MOV_,0, 0,16},// essentially a nop, however it must not be a nop because of mechanics of insertInstructionAt()
{I_MOV_,0, 0,16},
{I_RL1_,1,16,16,1},
{I_SUBN,0, 0, 1},
{I_POP1,1,16,16},
{I_AND_,0, 0, 1}
};

const InsertionInstructionPacket IIP_arr_11_1[] = {
{I_POP1,1,16,16},
{I_POP1,0,16,16},
{I_PU1_,1,16,16},
{I_RL1_,1,16,16,1},
{I_SUBN,0, 0, 1},
{I_POP1,1,16,16},
{I_AND_,0, 0, 1}
};

const ValueTraceEntry VTE_arr_11[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=2}
};

const ValueTraceEntry VTE_arr_12[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=4}
};

const ValueTraceEntry VTE_arr_13[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=8}
};

const ValueTraceEntry VTE_arr_14[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=16}
};

const ValueTraceEntry VTE_arr_15[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=32}
};

const ValueTraceEntry VTE_arr_16[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=64}
};

const ValueTraceEntry VTE_arr_17[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=128}
};

const ValueTraceEntry VTE_arr_18[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=256}
};

const ValueTraceEntry VTE_arr_19[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=512}
};

const ValueTraceEntry VTE_arr_1A[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=1024}
};

const ValueTraceEntry VTE_arr_1B[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=2048}
};

const ValueTraceEntry VTE_arr_1C[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=4096}
};

const ValueTraceEntry VTE_arr_1D[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=8192}
};

const ValueTraceEntry VTE_arr_1E[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=16384}
};

const ValueTraceEntry VTE_arr_1F[] = {
{I_DIVM,0x25},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=32768}
};

const ValueTraceEntry VTE_arr_20[] = {
{I_SHFT,0x20},
 {I_SHFT,0x28},
  {I_SHFT,0x28},
   {I_SHFT,0x28},
    {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_20[] = {
{I_POP1,0,16,16},
{I_RL1_,1,16,16,16},
{I_DIVM,0, 1,16}
};

const ValueTraceEntry VTE_arr_21[] = {
{I_XOR_,0x20},
 {I_PEPH,0x60},{I_RL1_,0x40,.cv=1},
 {I_XOR_,0x28},
  {I_XOR_,0x28},
   {I_PEPH,0x60},{I_RL1_,0x40,.cv=0},
   {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const ValueTraceEntry VTE_arr_22[] = {
{I_AND_,0x20},
 {I_PEPH,0x60},{I_RL1_,0x40,.cv=1},
 {I_AND_,0x28},
  {I_AND_,0x28},
   {I_PEPH,0x60},{I_RL1_,0x40,.cv=0},
   {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const ValueTraceEntry VTE_arr_23[] = {
{I_OR__,0x20},
 {I_PEPH,0x60},{I_RL1_,0x40,.cv=1},
 {I_OR__,0x28},
  {I_OR__,0x28},
   {I_PEPH,0x60},{I_RL1_,0x40,.cv=0},
   {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const ValueTraceEntry VTE_arr_24[] = {
{I_MULS,0x20},
 {I_PEPH,0x60},{I_RL1_,0x40,.cv=1},
 {I_MULS,0x28},
  {I_MULS,0x28},
   {I_PEPH,0x60},{I_RL1_,0x40,.cv=0},
   {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const ValueTraceEntry VTE_arr_25[] = {
{I_ADDN,0x20},
 {I_PEPH,0x60},{I_RL1_,0x40,.cv=1},
 {I_ADDN,0x28},
  {I_ADDN,0x28},
   {I_PEPH,0x60},{I_RL1_,0x40,.cv=0},
   {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};






const ValueTraceEntry VTE_arr_79[] = {
{I_OR__,0x20},
 {I_SSUB,0x24},
  {I_RL1_,0x30,.cv=0},
  {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_RL1_,0x30,.cv=0},
 {I_SSUB,0x24},
  {I_RL1_,0x30,.cv=0},
  {I_PEPH,0x60},{I_DEPL,0xB0},
  {I_RL1_,0x30,.cv=0}
};

const InsertionInstructionPacket IIP_arr_79[] = {
{I_POP1,1,16,16},
{I_POP1,0,16,16},
{I_OR__,1, 1, 0},
{I_BL1_,0,16,16,0},
{I_SSUB,0, 1, 0}
};

const ValueTraceEntry VTE_arr_7A[] = {
{I_MULS,0x20},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=2}
};

const InsertionInstructionPacket IIP_arr_7A[] = {
{I_POP1,0,16,16},
{I_ADDN,0, 0, 0}
};

const ValueTraceEntry VTE_arr_7B[] = { // requires bool
{I_OR__,0x20},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=1},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=1},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_7B[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_AND_,0, 0, 1},
{I_BL1_,1,16,16,1},
{I_XOR_,0, 0, 1}
};

const ValueTraceEntry VTE_arr_7C[] = { // requires bool
{I_AND_,0x20},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=1},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=1},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_7C[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_OR__,0, 0, 1},
{I_BL1_,1,16,16,1},
{I_XOR_,0, 0, 1}
};



const ValueTraceEntry VTE_arr_7D[] = { // requires bool
{I_PEPH,0x50},{I_OR__,0x20},
 {I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=1}
};

const ValueTraceEntry VTE_arr_7E[] = { // requires bool
{I_PEPH,0x50},{I_AND_,0x20},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=1}
};

const ValueTraceEntry VTE_arr_7F[] = {
{I_OR__,0x20},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=0xFFFF},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=0xFFFF},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_7F[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_AND_,0, 0, 1},
{I_BL1_,1,16,16,1},
{I_XOR_,0, 0, 1}
};

const ValueTraceEntry VTE_arr_80[] = {
{I_AND_,0x20},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=0xFFFF},
  {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_XOR_,0x20},
  {I_RL1_,0x30,.cv=0xFFFF},
  {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_80[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_OR__,0, 0, 1},
{I_BL1_,1,16,16,1},
{I_XOR_,0, 0, 1}
};

const ValueTraceEntry VTE_arr_81[] = {
{I_PEPH,0x50},{I_OR__,0x20},
 {I_DEPL,0xB0},
 {I_PEPH,0x60},{I_RL1_,0x30,.cv=0xFFFF}
};

const ValueTraceEntry VTE_arr_82[] = {
{I_PEPH,0x50},{I_AND_,0x20},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_RL1_,0x30,.cv=0xFFFF}
};

const ValueTraceEntry VTE_arr_83[] = { // requires bool
{I_MULS,0x20},
 {I_PEPH,0x60},{I_DEPL,0xB0},
 {I_PEPH,0x60},{I_DEPL,0xB0}
};

const InsertionInstructionPacket IIP_arr_83[] = {
{I_POP1,0,16,16},
{I_POP1,1,16,16},
{I_AND_,0, 0, 1}
};



typedef struct ValueTraceEntriesPacket{
	const bool requireBoolVerify;
	const uint8_t postApplyIndex;
	const uint8_t nUnusedReg;
	const uint8_t iipLen;
	const ValueTraceEntry * const vte;
	const InsertionInstructionPacket * iip;
} ValueTraceEntriesPacket;



ValueTraceEntriesPacket VTEP_arr[PH_VTEP_COUNT] = {
{0,0,0,0,VTE_arr_00},
{0,0,0,0,VTE_arr_01},
{0,0,0,0,VTE_arr_02},
{0,0,0,0,VTE_arr_03},

{0,3,2,5,NULL}, // for VTE_arr_09 when I make a way to check for validity
{0,0,0,0,VTE_arr_0A},
{0,2,0,1,VTE_arr_0B,IIP_arr_0B},
{0,2,0,2,VTE_arr_0C,IIP_arr_0C},
{0,2,0,3,VTE_arr_0D,IIP_arr_0D},
{0,0,0,0,NULL},
{0,2,0,4,VTE_arr_0F,IIP_arr_0F},
{0,4,1,7,VTE_arr_10},
{0,5,1,7,VTE_arr_11},
{0,5,1,7,VTE_arr_12},
{0,5,1,7,VTE_arr_13},
{0,5,1,7,VTE_arr_14},
{0,5,1,7,VTE_arr_15},
{0,5,1,7,VTE_arr_16},
{0,5,1,7,VTE_arr_17},
{0,5,1,7,VTE_arr_18},
{0,5,1,7,VTE_arr_19},
{0,5,1,7,VTE_arr_1A},
{0,5,1,7,VTE_arr_1B},
{0,5,1,7,VTE_arr_1C},
{0,5,1,7,VTE_arr_1D},
{0,5,1,7,VTE_arr_1E},
{0,5,1,7,VTE_arr_1F},
{0,2,1,3,VTE_arr_20,IIP_arr_20},

{0,6,3,8,VTE_arr_21},
{0,6,3,8,VTE_arr_22},
{0,6,3,8,VTE_arr_23},
{0,6,3,8,VTE_arr_24},
{0,6,3,8,VTE_arr_25},

{0,2,1,5,VTE_arr_79,IIP_arr_79},
{0,2,0,2,VTE_arr_7A,IIP_arr_7A},
{1,2,1,5,VTE_arr_7B,IIP_arr_7B},
{1,2,1,5,VTE_arr_7C,IIP_arr_7C},
{1,0,0,0,VTE_arr_7D},
{1,0,0,0,VTE_arr_7E},
{0,2,1,5,VTE_arr_7F,IIP_arr_7F},
{0,2,1,5,VTE_arr_80,IIP_arr_80},
{0,0,0,0,VTE_arr_81},
{0,0,0,0,VTE_arr_82},
{1,2,1,3,VTE_arr_83,IIP_arr_83}
};


typedef struct ValueTraceEntriesApplicableRepeatedParams{
	uint8_t walkSource;
	uint8_t walkTemplate;
	
	uint8_t startReg;
	uint8_t pushPopWalk;
	bool isRootBool;
	uint32_t startIndex; // startIndex is for inside `ib->buffer`
	uint32_t pushPopRecord[8];
	InstructionBuffer* ib;
	
	ValueTraceEntriesPacket* template;
	struct {
		uint16_t real[PH_VTEP_MAX_SYM_CONST];
		uint16_t symbol[PH_VTEP_MAX_SYM_CONST];
		bool isSet[PH_VTEP_MAX_SYM_CONST];
		uint8_t setBy[PH_VTEP_MAX_SYM_CONST];
	} cvTable;
	struct {
		uint32_t instr[PH_VTEP_MAX_IDEN]; // these indexes are only used during template matching and are not always valid after push/pop insertion
		bool isSet[PH_VTEP_MAX_IDEN];
		uint8_t setBy[PH_VTEP_MAX_IDEN];
	} idenTable;
	ValueTraceEntry source[PH_VTEP_LENGTH];
} ValueTraceEntriesApplicableRepeatedParams;



typedef struct DualBoundPeephole{
	uint32_t lowerBoundForUnusedOut;
	uint32_t upperBoundForUnusedOut;
} DualBoundPeephole;




/*
The functions below are passive in that they DO NOT modify the InstructionBuffer
*/


#ifdef OPT_PEEPHOLE_PRINT_TREE
uint8_t prettyPrintPeepholeTreeStartDepth;
#endif

// if this returns true, then vtearp->source overflowed, and generation needs to be restarted with the safe depth
bool generateValueTraceEntries(
		ValueTraceEntriesApplicableRepeatedParams* vtearp,
		uint8_t depthLeft,
		uint8_t currentReg,
		bool require_depl,
		uint32_t currentIndex,
		uint32_t usageIndex){

#ifdef OPT_PEEPHOLE_PRINT_TREE
for (uint8_t i=0;i<prettyPrintPeepholeTreeStartDepth-depthLeft;i++){
printf("  ");
}
printSingleInstructionOptCode(vtearp->ib->buffer[currentIndex]);
printf("  [%%%01X]",currentReg);
if (require_depl){
printf("  [CROSSED STACK]");
}
if (depthLeft==1){
printf("  [HIT DEPL]");
}
#endif
	InstructionInformation II;
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource++;
	assert(vtearp->walkSource<=PH_VTEP_LENGTH);
	if (vtearp->walkSource==PH_VTEP_LENGTH){
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("\npeephole source overflow!\n");
#endif
		return true;
	}
	thisSource->instr=currentIndex;
	thisSource->nRegIN=0;
	thisSource->regOUT=currentReg;
	fillInstructionInformation(&II,vtearp->ib,currentIndex);
	assert(doesRegListContain(II.regOUT,currentReg));
	bool isThisValueUsedInOtherPlaces=isValueInRegUsedAfterTargetExceptAt(vtearp->ib,currentIndex,usageIndex,currentReg);
#ifdef OPT_PEEPHOLE_PRINT_TREE
if (isThisValueUsedInOtherPlaces) printf(" [USED ELSEWHERE]");
#endif
	if (II.isAllowedInVTE & !require_depl){
		thisSource->id=II.id;
	} else {
		thisSource->id=I_DEPL;
		thisSource->entryType=0xB0|(isThisValueUsedInOtherPlaces*0x08);
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("  [INSERT DEPL]\n");
#endif
		return false;
	}
	if (II.id==I_BL1_ | II.id==I_RL1_){
		InstructionSingle* thisIS=vtearp->ib->buffer+currentIndex;
		thisSource->id=I_RL1_; // id is overwritten on purpose
		if (II.id==I_BL1_) thisSource->cv=thisIS->arg.B2.a_1;
		else thisSource->cv=thisIS->arg.BW.a_1;
	}
	bool isSecondRegOut=II.regOUT[0]!=currentReg;
	bool isOtherValueUsed=0;
	if (II.regOUT[1]!=16) isOtherValueUsed=isValueInRegUsedAfterTarget(vtearp->ib,currentIndex,II.regOUT[!isSecondRegOut],NULL);
#ifdef OPT_PEEPHOLE_PRINT_TREE
if (isOtherValueUsed) printf(" [OTHER USED]");
printf("\n");
#endif
	uint8_t depthNext=depthLeft-1;
	bool noDepthNext=depthNext==0;
	thisSource->entryType=
		((II.id==I_BL1_ | II.id==I_RL1_)*0x10)|(isThisValueUsedInOtherPlaces*0x08)|
		(isSecondRegOut*0x01)|(noDepthNext*0x02)|(isOtherValueUsed*0x04);
	if (noDepthNext) return false;
	struct LocationAndIfRequireDepl ladpl;
	ladpl.target=currentIndex-1;
	uint8_t r;
	uint8_t ri=0;
	while ((r=II.regIN[ri++])!=16){
		ladpl.reg=r;
		uint32_t regValSource=findRegValueSourceSignalCross(vtearp->ib,&ladpl);
		if (generateValueTraceEntries(vtearp,depthNext,r,ladpl.require_depl,regValSource,currentIndex)){
			return true;
		}
	}
	thisSource->nRegIN=ri-1;
	return false;
}

void pseudoWalkSource(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource++;
	if ((thisSource->entryType&0x02)|(thisSource->id==I_DEPL)) {return;}
	InstructionInformation II;
	fillInstructionInformation(&II,vtearp->ib,thisSource->instr);
	uint8_t ri=0;
	while (II.regIN[ri++]!=16){
		pseudoWalkSource(vtearp);
	}
}

// swapValueTraceEntriesInputs() should only be called from isValueTraceEntriesApplicable()
void swapValueTraceEntriesInputs(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	ValueTraceEntry tempVTE_arr[PH_VTEP_LENGTH];
	ValueTraceEntry* sourcePtr=vtearp->source;
	struct {
		uint8_t walk0;
		uint8_t walk1;
		uint8_t walk2;
		uint8_t walki;
		uint8_t walk2_1;
		uint8_t walk1_0;
	} v;
	v.walk0=vtearp->walkSource;
	pseudoWalkSource(vtearp);
	v.walk1=vtearp->walkSource;
	pseudoWalkSource(vtearp);
	v.walk2=vtearp->walkSource;
	vtearp->walkSource=v.walk0;
	sourcePtr[v.walk0-1].isSwapped^=1;
	v.walk1_0=v.walk1-v.walk0;
	v.walk2_1=v.walk2-v.walk1;
	for (v.walki=v.walk0;v.walki<v.walk2;v.walki++){
		tempVTE_arr[v.walki]=sourcePtr[v.walki];
	}
	for (v.walki=v.walk1;v.walki<v.walk2;v.walki++){
		sourcePtr[v.walki-v.walk1_0]=tempVTE_arr[v.walki];
	}
	for (v.walki=v.walk0;v.walki<v.walk1;v.walki++){
		sourcePtr[v.walki+v.walk2_1]=tempVTE_arr[v.walki];
	}
}

bool isValueTraceEntriesApplicable(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource;
	const ValueTraceEntry* vte=vtearp->template->vte+vtearp->walkTemplate;
	while (vte->id==I_PEPH){
		vte=vtearp->template->vte+ ++vtearp->walkTemplate;
	}
	uint8_t backupWalkTemplate=++vtearp->walkTemplate;
	uint8_t thisIden=vte->iden;
	if (thisIden!=0){
		uint8_t idenSub1=thisIden-1;
		if (vtearp->idenTable.isSet[idenSub1]){
			if (vtearp->idenTable.instr[idenSub1]!=thisSource->instr){
				++vtearp->walkSource; // for consistency
				return false; // failure by invalid iden
			}
		} else {
			vtearp->idenTable.isSet[idenSub1]=1;
			vtearp->idenTable.setBy[idenSub1]=vtearp->walkSource;
			vtearp->idenTable.instr[idenSub1]=thisSource->instr;
		}
	}
	if (vte->id==I_DEPL){
		pseudoWalkSource(vtearp);
		return true;
	}
	uint8_t backupWalkSource=++vtearp->walkSource;
	if (
	thisSource->id!=vte->id | 
	((thisSource->entryType&0x01)^(vte->entryType&0x01))| (
	(thisSource->entryType&0x04)&
	(vte->entryType&0x04))| (
	(thisSource->entryType&0x08)&
	(vte->entryType&0x08))){
		//printf("Not applicable (%d,%d,%d,%d,%d,%d,%d)\n",thisSource->id!=vte->id,thisSource->entryType&0x01,vte->entryType&0x01,(thisSource->entryType&0x04)/0x04,(vte->entryType&0x04)/0x04,(thisSource->entryType&0x08)/0x08,(vte->entryType&0x08)/0x08);
		// includes `thisSource->id==I_DEPL` , if the wrong output is used, if thisSource's outputs are used when they can't be (for both types of used output checks)
		if (thisIden!=0) vtearp->idenTable.isSet[thisIden-1]=0;
		return false;
	}
	if (vte->id==I_RL1_){
		// constants have no inputs, so it will return inside this if statement
		if ((vte->entryType&0xF0)==0x30){
			return vte->cv==thisSource->cv;
		} else {
			assert((vte->entryType&0xF0)==0x40);
			for (uint8_t symCVi=0;symCVi<PH_VTEP_MAX_SYM_CONST;symCVi++){
				if (vtearp->cvTable.isSet[symCVi]){
					if (vtearp->cvTable.symbol[symCVi]==vte->cv){
						return vtearp->cvTable.real[symCVi]==thisSource->cv;
					}
				}
			}
			// then the symbol from the template doesn't exist yet, so it will be inserted
			for (uint8_t symCVi=0;symCVi<PH_VTEP_MAX_SYM_CONST;symCVi++){
				if (!vtearp->cvTable.isSet[symCVi]){
					vtearp->cvTable.symbol[symCVi]=vte->cv;
					vtearp->cvTable.real[symCVi]=thisSource->cv;
					vtearp->cvTable.isSet[symCVi]=1;
					vtearp->cvTable.setBy[symCVi]=backupWalkSource-1;
					return true;
				}
			}
		}
	} else {
		// at this point, it is known that this tree node matches 
		bool areInputsListed=(thisSource->entryType&0x02)==0x00;
		InstructionInformation II;
		fillInstructionInformation(&II,vtearp->ib,thisSource->instr);
		uint8_t ri=0;
		while (II.regIN[ri++]!=16){
			if (areInputsListed){
				if (!isValueTraceEntriesApplicable(vtearp)){
					if (II.inputTrivialSwapable){
						vtearp->walkSource=backupWalkSource;
						vtearp->walkTemplate=backupWalkTemplate;
						ri=0;
						swapValueTraceEntriesInputs(vtearp);
						// now remove symbolic constant entries that shouldn't exist yet
						for (uint8_t symCVi=0;symCVi<PH_VTEP_MAX_SYM_CONST;symCVi++){
							if (vtearp->cvTable.isSet[symCVi]){
								if (vtearp->cvTable.setBy[symCVi]>=backupWalkSource){
									vtearp->cvTable.isSet[symCVi]=0;
								}
							}
						}
						// now remove iden trackers that shouldn't exist yet
						for (uint8_t idenI=0;idenI<PH_VTEP_MAX_IDEN;idenI++){
							if (vtearp->idenTable.isSet[idenI]){
								if (vtearp->idenTable.setBy[idenI]>=backupWalkSource){
									vtearp->idenTable.isSet[idenI]=0;
								}
							}
						}
						while (II.regIN[ri++]!=16){
							if (!isValueTraceEntriesApplicable(vtearp)){return false;}
						}
						return true;
					}
					return false;
				}
			} else {
				printf("Internal Warning: peephole opt couldn\'t verify template because source didn't have enough depth\n");
				// this does not indicate an error, but it does make the template match check say that it doesn't match, when it could have if the tree was generated larger
				return false;
			}
		}
		return true;
	}
	printf("Internal Error in peephole optimization\n");
	// then there were too many symbolic constants in a template
	exit(1);
}



DualBoundPeephole getDualBoundPeephole(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource;
	const ValueTraceEntry* vte=vtearp->template->vte+vtearp->walkTemplate;
	DualBoundPeephole dbph;
	DualBoundPeephole dbph_temp;
	dbph.lowerBoundForUnusedOut=thisSource->instr;
	dbph.upperBoundForUnusedOut=dbph.lowerBoundForUnusedOut+1;
	uint32_t lowerBound=thisSource->instr;
	while (vte->id==I_PEPH){
		vte=vtearp->template->vte+ ++vtearp->walkTemplate;
	}
	++vtearp->walkTemplate;
	if (vte->id==I_DEPL) {pseudoWalkSource(vtearp);goto End;}
	++vtearp->walkSource;
	InstructionInformation II;
	fillInstructionInformation(&II,vtearp->ib,thisSource->instr);
	assert(II.isAllowedInVTE);
	uint8_t ri=0;
	while (II.regIN[ri++]!=16){
		dbph_temp=getDualBoundPeephole(vtearp);
		if (dbph_temp.lowerBoundForUnusedOut<dbph.lowerBoundForUnusedOut)
			dbph.lowerBoundForUnusedOut=dbph_temp.lowerBoundForUnusedOut;
		if (dbph_temp.upperBoundForUnusedOut>dbph.upperBoundForUnusedOut)
			dbph.upperBoundForUnusedOut=dbph_temp.upperBoundForUnusedOut;
	}
	End:;
	return dbph;
}


void shiftInstrIndexesForFutureInsert(ValueTraceEntriesApplicableRepeatedParams* vtearp,bool* hasShifted, uint32_t above){
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource;
	uint32_t thisInstr=thisSource->instr;
	if (!hasShifted[vtearp->walkSource] & thisInstr>above) ++thisSource->instr;
	hasShifted[vtearp->walkSource]=1;
	++vtearp->walkSource;
	if (thisSource->id==I_DEPL | (thisSource->entryType&0x02)) return;
	InstructionInformation II;
	fillInstructionInformation(&II,vtearp->ib,thisInstr);
	uint8_t ri=0;
	while (II.regIN[ri++]!=16){
		shiftInstrIndexesForFutureInsert(vtearp,hasShifted,above);
	}
}

/*
The functions below are NOT passive in that they DO modify the InstructionBuffer
*/
void applyReorderNoArea(InstructionBuffer*,const uint32_t,const uint32_t);

// this does NOT insert into the same place as insertInstructionAt()
void insertNOP_InFrontOf(InstructionBuffer* ib, uint32_t destination){
	int32_t numberOfSlotsTaken = ib->numberOfSlotsTaken;
	InstructionSingle IS;
	IS.id=I_NOP_;
	addInstruction(ib,IS);
	applyReorderNoArea(ib,numberOfSlotsTaken,destination+1);
}

void insertNOP_InFrontOf_abstraction(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint32_t target){
	bool hasShifted[PH_VTEP_LENGTH]={0};
	for (uint8_t t=0;t<vtearp->pushPopWalk;t++){
		if (vtearp->pushPopRecord[t]>target){
			++vtearp->pushPopRecord[t];
		}
	}
	uint8_t backupWalkSource=vtearp->walkSource;
	vtearp->walkSource=0;
	shiftInstrIndexesForFutureInsert(vtearp,hasShifted,target);
	vtearp->walkSource=backupWalkSource;
	insertNOP_InFrontOf(vtearp->ib,target);
}

void applyInitialPeepHoleTransform(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	ValueTraceEntry* thisSource=vtearp->source+vtearp->walkSource;
	const ValueTraceEntry* vte=vtearp->template->vte+vtearp->walkTemplate;
	InstructionInformation II;
	fillInstructionInformation(&II,vtearp->ib,thisSource->instr);
	while (vte->id==I_PEPH){
		uint32_t target=thisSource->instr;
		insertNOP_InFrontOf_abstraction(vtearp,target);
		InstructionSingle IS;
		const enum InstructionTypeID ids[2]={I_POP1,I_PU1_};
		IS.id=ids[(vte->entryType&0xF0)==0x60];
		IS.arg.B1.a_0=thisSource->regOUT;
		vtearp->ib->buffer[target+1]=IS;
		vtearp->pushPopRecord[vtearp->pushPopWalk++]=target+1;
		vte=vtearp->template->vte+ ++vtearp->walkTemplate;
	}
	++vtearp->walkTemplate;
	if (vte->id==I_DEPL) {pseudoWalkSource(vtearp);return;}
	++vtearp->walkSource;
	if (!II.isAllowedInVTE){
		printInstructionBufferWithMessageAndNumber(vtearp->ib,"",thisSource->instr);
		exit(0);
	}
	assert(II.isAllowedInVTE);
	if (vtearp->template->postApplyIndex!=0){
		assert(thisSource->id==vte->id);
	}
	uint8_t ri=0;
	uint8_t r;
	while ((r=II.regIN[ri++])!=16){
		assert((thisSource->entryType&0x02)==0x00);
		applyInitialPeepHoleTransform(vtearp);
	}
}



uint8_t peepholeSortTriple(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	assert(vtearp->pushPopWalk==3);
	struct TripleSorted{
		uint32_t ppr[3];
		uint8_t rel[3];
		uint8_t perm;
	} ts={{vtearp->pushPopRecord[0],vtearp->pushPopRecord[1],vtearp->pushPopRecord[2]},{0,1,2},0};
	uint8_t t;
	if (ts.ppr[ts.rel[0]]>ts.ppr[ts.rel[1]]){
		t=ts.rel[0];
		ts.rel[0]=ts.rel[1];
		ts.rel[1]=t;
		ts.perm|=1;
	}
	if (ts.ppr[ts.rel[1]]>ts.ppr[ts.rel[2]]){
		t=ts.rel[1];
		ts.rel[1]=ts.rel[2];
		ts.rel[2]=t;
		ts.perm|=2;
	}
	if (ts.ppr[ts.rel[0]]>ts.ppr[ts.rel[1]]){
		t=ts.rel[0];
		ts.rel[0]=ts.rel[1];
		ts.rel[1]=t;
		ts.perm|=4;
	}
	assert(!(ts.perm==4 | ts.perm==5));
	//printf("%04X,%04X,%04X->%04X,%04X,%04X (%d)\n",ts.ppr[0],ts.ppr[1],ts.ppr[2],ts.ppr[ts.rel[0]],ts.ppr[ts.rel[1]],ts.ppr[ts.rel[2]],ts.perm);
	return ts.perm;
}

uint8_t peepholeSortQuad(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	assert(vtearp->pushPopWalk==4);
	uint32_t a=vtearp->pushPopRecord[0];
	uint32_t b=vtearp->pushPopRecord[1];
	uint32_t c=vtearp->pushPopRecord[2];
	uint32_t d=vtearp->pushPopRecord[3];
	if (a > b){
		if (b > c){
			if (d > b)
				return (d < a)+0;
			else
				return (d < c)+2;
		} else {
			if (a > c){
				if (d > c)
					return (d < a)+4;
				else
					return (d < b)+6;
			} else {
				if (d > a)
					return (d < c)+8;
				else
					return (d < b)+10;
			}
		}
	} else {
		if (a > c){
			if (d > a)
				return (d < b)+12;
			else
				return (d < c)+14;
		} else {
			if (b > c){
				if (d > c)
					return (d < b)+16;
				else
					return (d < a)+18;
			} else {
				if (d > b)
					return (d < c)+20;
				else
					return (d < a)+22;
			}
		}
	}
}

void peepholeSortQuadWithPattern(ValueTraceEntriesApplicableRepeatedParams* vtearp,uint8_t* p,uint8_t* r){
	switch (peepholeSortQuad(vtearp)){
		case  0:p[0]=r[3];p[1]=r[0];p[2]=r[1];p[3]=r[2];return;
		case  1:p[0]=r[0];p[1]=r[3];p[2]=r[1];p[3]=r[2];return;
		case  2:p[0]=r[0];p[1]=r[1];p[2]=r[3];p[3]=r[2];return;
		case  3:p[0]=r[0];p[1]=r[1];p[2]=r[2];p[3]=r[3];return;
		case  4:p[0]=r[3];p[1]=r[0];p[2]=r[2];p[3]=r[1];return;
		case  5:p[0]=r[0];p[1]=r[3];p[2]=r[2];p[3]=r[1];return;
		case  6:p[0]=r[0];p[1]=r[2];p[2]=r[3];p[3]=r[1];return;
		case  7:p[0]=r[0];p[1]=r[2];p[2]=r[1];p[3]=r[3];return;
		case  8:p[0]=r[3];p[1]=r[2];p[2]=r[0];p[3]=r[1];return;
		case  9:p[0]=r[2];p[1]=r[3];p[2]=r[0];p[3]=r[1];return;
		case 10:p[0]=r[2];p[1]=r[0];p[2]=r[3];p[3]=r[1];return;
		case 11:p[0]=r[2];p[1]=r[0];p[2]=r[1];p[3]=r[3];return;
		case 12:p[0]=r[3];p[1]=r[1];p[2]=r[0];p[3]=r[2];return;
		case 13:p[0]=r[1];p[1]=r[3];p[2]=r[0];p[3]=r[2];return;
		case 14:p[0]=r[1];p[1]=r[0];p[2]=r[3];p[3]=r[2];return;
		case 15:p[0]=r[1];p[1]=r[0];p[2]=r[2];p[3]=r[3];return;
		case 16:p[0]=r[3];p[1]=r[1];p[2]=r[2];p[3]=r[0];return;
		case 17:p[0]=r[1];p[1]=r[3];p[2]=r[2];p[3]=r[0];return;
		case 18:p[0]=r[1];p[1]=r[2];p[2]=r[3];p[3]=r[0];return;
		case 19:p[0]=r[1];p[1]=r[2];p[2]=r[0];p[3]=r[3];return;
		case 20:p[0]=r[3];p[1]=r[2];p[2]=r[1];p[3]=r[0];return;
		case 21:p[0]=r[2];p[1]=r[3];p[2]=r[1];p[3]=r[0];return;
		case 22:p[0]=r[2];p[1]=r[1];p[2]=r[3];p[3]=r[0];return;
		case 23:p[0]=r[2];p[1]=r[1];p[2]=r[0];p[3]=r[3];return;
		default:assert(false);
	}
	/*
	case = items in decending order
	
	 0=[d, a, b, c]
	 1=[a, d, b, c]
	 2=[a, b, d, c]
	 3=[a, b, c, d]
	 4=[d, a, c, b]
	 5=[a, d, c, b]
	 6=[a, c, d, b]
	 7=[a, c, b, d]
	 8=[d, c, a, b]
	 9=[c, d, a, b]
	10=[c, a, d, b]
	11=[c, a, b, d]
	12=[d, b, a, c]
	13=[b, d, a, c]
	14=[b, a, d, c]
	15=[b, a, c, d]
	16=[d, b, c, a]
	17=[b, d, c, a]
	18=[b, c, d, a]
	19=[b, c, a, d]
	20=[d, c, b, a]
	21=[c, d, b, a]
	22=[c, b, d, a]
	23=[c, b, a, d]
	*/
}

/*
The functions below are specialized post application functions
*/

void insertInstructionAt(InstructionBuffer*,const uint32_t,const InstructionSingle);


uint16_t applyPostPeepHole_2(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint8_t regs[5]={vtearp->source[0].regOUT,unusedRegs[0],unusedRegs[1],unusedRegs[2],unusedRegs[3]};
	uint8_t length=vtearp->template->iipLen;
	uint32_t insertionIndex=vtearp->source[0].instr;
	const InsertionInstructionPacket* iip=vtearp->template->iip;
	InstructionBuffer* ib=vtearp->ib;
	InstructionSingle IS;
	for (uint8_t i=0;i<length;i++){
		IS.id=iip[i].id;
		if (IS.id==I_BL1_){
			IS.arg.B2.a_0=regs[iip[i].r0];
			IS.arg.B2.a_1=iip[i].cv;
		} else if (IS.id==I_RL1_){
			IS.arg.BW.a_0=regs[iip[i].r0];
			IS.arg.BW.a_1=iip[i].cv;
		} else {
			uint8_t rCount=0;
			if (iip[i].r0!=16) ++rCount;
			if (iip[i].r1!=16) ++rCount;
			if (iip[i].r2!=16) ++rCount;
			if (rCount==1){
				IS.arg.B1.a_0=regs[iip[i].r0];
			} else if (rCount==2){
				IS.arg.B2.a_0=regs[iip[i].r0];
				IS.arg.B2.a_1=regs[iip[i].r1];
			} else {
				IS.arg.B3.a_0=regs[iip[i].r0];
				IS.arg.B3.a_1=regs[iip[i].r1];
				IS.arg.B3.a_2=regs[iip[i].r2];
			}
		}
		insertInstructionAt(ib,++insertionIndex,IS);
	}
	//printInstructionBufferWithMessageAndNumber(ib,"Check:",insertionIndex);
	return length;
}


uint16_t applyPostPeepHole_1(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint8_t perm=peepholeSortTriple(vtearp);
	uint8_t regOUT=vtearp->source[0].regOUT;
	uint32_t insertionIndex=vtearp->source[0].instr;
	const enum InstructionTypeID id=vtearp->source[0].id;
	InstructionBuffer* ib=vtearp->ib;
	InstructionSingle IS;
	switch (perm){
		case 2:
		case 6:// these cases need two unused registers
		IS.id=I_POP1;
		IS.arg.B1.a_0=regOUT;
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[0];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[1];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=unusedRegs[0];
			IS.arg.B2.a_1=unusedRegs[1];
		} else {
			IS.arg.B3.a_0=unusedRegs[0];
			IS.arg.B3.a_1=unusedRegs[0];
			IS.arg.B3.a_2=unusedRegs[1];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=regOUT;
			IS.arg.B2.a_1=unusedRegs[0];
		} else {
			IS.arg.B3.a_0=regOUT;
			IS.arg.B3.a_1=regOUT;
			IS.arg.B3.a_2=unusedRegs[0];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		break;
		
		case 0:
		case 7:
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[0];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=regOUT;
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[1];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=unusedRegs[0];
			IS.arg.B2.a_1=unusedRegs[1];
		} else {
			IS.arg.B3.a_0=unusedRegs[0];
			IS.arg.B3.a_1=unusedRegs[0];
			IS.arg.B3.a_2=unusedRegs[1];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=regOUT;
			IS.arg.B2.a_1=unusedRegs[0];
		} else {
			IS.arg.B3.a_0=regOUT;
			IS.arg.B3.a_1=regOUT;
			IS.arg.B3.a_2=unusedRegs[0];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		break;
		
		case 1:
		case 3:// these cases only need one unused register
		IS.id=I_POP1;
		IS.arg.B1.a_0=regOUT;
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[0];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=regOUT;
			IS.arg.B2.a_1=unusedRegs[0];
		} else {
			IS.arg.B3.a_0=regOUT;
			IS.arg.B3.a_1=regOUT;
			IS.arg.B3.a_2=unusedRegs[0];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=I_POP1;
		IS.arg.B1.a_0=unusedRegs[0];
		insertInstructionAt(ib,++insertionIndex,IS);
		IS.id=id;
		if (id==I_MULS){
			IS.arg.B2.a_0=regOUT;
			IS.arg.B2.a_1=unusedRegs[0];
		} else {
			IS.arg.B3.a_0=regOUT;
			IS.arg.B3.a_1=regOUT;
			IS.arg.B3.a_2=unusedRegs[0];
		}
		insertInstructionAt(ib,++insertionIndex,IS);
		break;
	}
	return 5;
}


uint16_t applyPostPeepHole_3(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint8_t perm=peepholeSortTriple(vtearp);
	uint32_t insertionIndex=vtearp->source[0].instr;
	InstructionBuffer* ib=vtearp->ib;
	switch (perm){
		case 1:// todo: these cases should be re-checked at some point
		case 6:
		vtearp->template->iip=IIP_arr_09_1;
		break;
		case 0:
		case 2:
		vtearp->template->iip=IIP_arr_09_0;
		break;
		case 3:
		case 7:
		vtearp->template->iip=IIP_arr_09_2;
		break;
	}
	return applyPostPeepHole_2(vtearp,unusedRegs);
}

uint16_t applyPostPeepHole_4(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint32_t insertionIndex=vtearp->source[0].instr;
	InstructionBuffer* ib=vtearp->ib;
	assert(vtearp->pushPopWalk==2);
	if (vtearp->pushPopRecord[0]>vtearp->pushPopRecord[1]){
		vtearp->template->iip=IIP_arr_10_1;
	} else {
		vtearp->template->iip=IIP_arr_10_0;
	}
	return applyPostPeepHole_2(vtearp,unusedRegs);
}

uint16_t applyPostPeepHole_5(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint32_t insertionIndex=vtearp->source[0].instr;
	InstructionBuffer* ib=vtearp->ib;
	assert(vtearp->pushPopWalk==2);
	if (vtearp->pushPopRecord[0]>vtearp->pushPopRecord[1]){
		vtearp->template->iip=IIP_arr_11_1;
	} else {
		vtearp->template->iip=IIP_arr_11_0;
	}
	return applyPostPeepHole_2(vtearp,unusedRegs);
}


uint16_t applyPostPeepHole_6(ValueTraceEntriesApplicableRepeatedParams* vtearp, uint8_t* unusedRegs){
	uint8_t r[4]={vtearp->source[0].regOUT,unusedRegs[0],unusedRegs[1],unusedRegs[2]};
	uint8_t p[4]; // pattern for registers
	peepholeSortQuadWithPattern(vtearp,p,r);
	enum InstructionTypeID id=vtearp->source[0].id;
	uint32_t insertionIndex=vtearp->source[0].instr;
	InstructionBuffer* ib=vtearp->ib;
	InstructionSingle IS;
	IS.id=I_POP1;
	IS.arg.B1.a_0=r[0];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.arg.B1.a_0=r[1];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.arg.B1.a_0=r[2];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.arg.B1.a_0=r[3];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.id=id;
	IS.arg.B3.a_0=p[0];
	IS.arg.B3.a_1=p[0];
	IS.arg.B3.a_2=p[1];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.arg.B3.a_0=p[2];
	IS.arg.B3.a_1=p[2];
	IS.arg.B3.a_2=p[3];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.arg.B3.a_0=p[0];
	IS.arg.B3.a_1=p[0];
	IS.arg.B3.a_2=p[2];
	insertInstructionAt(ib,++insertionIndex,IS);
	IS.id=I_MOV_;
	IS.arg.B2.a_0=r[0];
	IS.arg.B2.a_1=p[0];
	insertInstructionAt(ib,++insertionIndex,IS);
	return 8;
}

