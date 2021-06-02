

// this should only be used by applyReorder() , insertInstructionAt() , and insertNOP_InFrontOf()
void applyReorderNoArea(InstructionBuffer* ib,const uint32_t target,const uint32_t destination){
	if (target==destination) return;
	InstructionSingle* buffer = ib->buffer;
	InstructionSingle targetInstruction = buffer[target];
	uint32_t walkIndex = target;
	uint32_t next;
	if (destination>target){
		while (destination!=walkIndex){
			next = walkIndex+1;
			buffer[walkIndex]=buffer[next];
			consolePrintBufferPoint(ib,walkIndex,next,2,"Reordering Forwards");
			walkIndex=next;
		}
	} else {
		while (destination!=walkIndex){
			next = walkIndex-1;
			buffer[walkIndex]=buffer[next];
			consolePrintBufferPoint(ib,walkIndex,next,2,"Reordering Backwards");
			walkIndex=next;
		}
	}
	buffer[destination]=targetInstruction;
}

void applyReorder(InstructionBuffer* ib,const uint32_t target,const uint32_t destination){
	if (target==destination) return;
	enum InstructionTypeID id=ib->buffer[target].id;
	if (id==I_SYRB | id==I_SYRW | id==I_SYRD | id==I_SYRQ){
		uint16_t length = findLengthOfSymbolicCalc(ib,target);
		uint16_t offset;
		bool isDestSmaller=destination<target;
		for (uint16_t i=0;i<length;i++){
			offset=isDestSmaller?i:0U;
			applyReorderNoArea(ib,target+offset,destination+offset);
		}
	} else {
		applyReorderNoArea(ib,target,destination);
	}
}



void insertInstructionAtNoNopSearch(InstructionBuffer* ib,const uint32_t destination,const InstructionSingle IS){
	uint32_t target=ib->numberOfSlotsTaken;
	addInstruction(ib,IS);
	applyReorderNoArea(ib,target,destination);
}

/*
this moves the instruction that is there to the next slot (newIndex==destination+1)
this should be used sparingly

if it is able to find a NOP in a desirable location, it will destroy it.
*/
void insertInstructionAt(InstructionBuffer* ib,const uint32_t destination,const InstructionSingle IS){
	int32_t nopLocation=-1;
	int32_t numberOfSlotsTaken = ib->numberOfSlotsTaken;
	InstructionSingle* buffer = ib->buffer;
	for (int32_t i=destination;i<numberOfSlotsTaken;i++){
		consolePrintBufferPoint(ib,i,-1,0,"Finding NOP_ for insertInstructionAt()");
		if (buffer[i].id==I_NOP_){
			nopLocation=i;
			break;
		}
	}
	int32_t futureTarget;
	if (nopLocation==-1){
		futureTarget = numberOfSlotsTaken;
		addInstruction(ib,IS);
	} else {
		futureTarget = nopLocation;
		buffer[nopLocation] = IS;
	}
	applyReorderNoArea(ib,futureTarget,destination);
}


void createNopPocket(InstructionBuffer* ib, const uint32_t destination, const uint16_t size){
	const InstructionSingle IS={.id=I_NOP_};
	for (uint16_t i=0;i<size;i++){
		addInstruction(ib,IS);
	}
	InstructionSingle*const buffer=ib->buffer;
	const uint32_t end=destination+size;
	uint32_t i=ib->numberOfSlotsTaken;
	while (i--!=end){
		buffer[i]=buffer[i-size];
	}
	i=destination+size;
	while (i--!=destination){
		buffer[i]=IS;
	}
}



void doLabelRename(InstructionBuffer* ib, uint32_t from, uint32_t to){
	InstructionSingle* ISP;
	InstructionSingle* buffer = ib->buffer;
	uint32_t i = ib->numberOfSlotsTaken;
	while (i--!=0){
		consolePrintBufferPoint(ib,i,-1,2,"Renaming Labels");
		ISP = buffer+i;
		enum InstructionTypeID id=ISP->id;
		if (id==I_LABL | id==I_SYCL | id==I_JTEN | id==I_FCST){
			if (ISP->arg.D.a_0==from) ISP->arg.D.a_0=to;
		} else if (id==I_JJMP){
			if (ISP->arg.BBD.a_2==from) ISP->arg.BBD.a_2=to;
		}
	}
}


// renames input and output registers. Assumes input is valid. range is inclusive.
void doRegRename(InstructionBuffer* ib, uint32_t start, uint32_t end, uint8_t from, uint8_t to){
	InstructionSingle* ISP;
	InstructionSingle* buffer = ib->buffer;
	for (uint32_t i=start;i<=end;i++){
		ISP = buffer+i;
		switch (ISP->id){
			case I_NOP_:
			case I_SYCB:
			case I_SYCW:
			case I_SYCD:
			case I_SYCL:
			case I_SYRE:
			case I_SYW0:
			case I_SYW1:
			case I_SYW2:
			case I_SYW3:
			case I_SYW4:
			case I_SYW5:
			case I_SYW6:
			case I_SYW7:
			case I_SYW8:
			case I_SYW9:
			case I_SBLW:
			case I_SBRW:
			case I_SYD0:
			case I_SYD1:
			case I_SYD2:
			case I_SYD3:
			case I_SYD4:
			case I_SYD5:
			case I_SYD6:
			case I_SYD7:
			case I_SYD8:
			case I_SYD9:
			case I_SBLD:
			case I_SBRD:
			case I_SYQ0:
			case I_SYQ1:
			case I_SYQ2:
			case I_SYQ3:
			case I_SYQ4:
			case I_SYQ5:
			case I_SYQ6:
			case I_SYQ7:
			case I_SYQ8:
			case I_SYQ9:
			case I_SBLQ:
			case I_SBRQ:
			case I_SCBW:
			case I_SCWD:
			case I_SCZD:
			case I_SCDQ:
			case I_SCZQ:
			case I_SCQD:
			case I_SCDW:
			case I_SCWB:
			case I_SCDB:
			case I_SCQB:
			break;
			case I_PU1_:
			case I_PUA1:
			case I_POP1:
			case I_SYRW:
			case I_SYRB:
			if (ISP->arg.B1.a_0==from) ISP->arg.B1.a_0=to;
			break;
			case I_PU2_:
			case I_PUA2:
			case I_POP2:
			case I_CALL:
			case I_AJMP:
			case I_MOV_:
			case I_MULS:
			case I_MULL:
			case I_DIVM:
			case I_SHFT:
			case I_BSWP:
			case I_MWBN:
			case I_MRBN:
			case I_MWBV:
			case I_MRBV:
			case I_SYRD:
			if (ISP->arg.B2.a_1==from) ISP->arg.B2.a_1=to;
			case I_BL1_:
			case I_STWN:
			case I_STRN:
			case I_STWV:
			case I_STRV:
			if (ISP->arg.B2.a_0==from) ISP->arg.B2.a_0=to;
			break;
			case I_CJMP:
			case I_AND_:
			case I_OR__:
			case I_XOR_:
			case I_ADDN:
			case I_SSUB:
			case I_MWWN:
			case I_MRWN:
			case I_MWWV:
			case I_MRWV:
			case I_SUBN:
			case I_SUBC:
			if (ISP->arg.B3.a_0==from) ISP->arg.B3.a_0=to;
			if (ISP->arg.B3.a_1==from) ISP->arg.B3.a_1=to;
			if (ISP->arg.B3.a_2==from) ISP->arg.B3.a_2=to;
			break;
			case I_RL1_:
			case I_ALCR:
			case I_STPA:
			case I_STPS:
			case I_STOF:
			if (ISP->arg.BW.a_0==from) ISP->arg.BW.a_0=to;
			break;
			case I_RL2_:
			case I_JJMP:
			if (ISP->arg.BBD.a_0==from) ISP->arg.BBD.a_0=to;
			if (ISP->arg.BBD.a_1==from) ISP->arg.BBD.a_1=to;
			break;
			case I_LAD2:
			if (ISP->arg.B4.a_0==from) ISP->arg.B4.a_0=to;
			if (ISP->arg.B4.a_1==from) ISP->arg.B4.a_1=to;
			if (ISP->arg.B4.a_2==from) ISP->arg.B4.a_2=to;
			if (ISP->arg.B4.a_3==from) ISP->arg.B4.a_3=to;
			break;
			case I_LAD1:
			if (ISP->arg.B5.a_0==from) ISP->arg.B5.a_0=to;
			if (ISP->arg.B5.a_1==from) ISP->arg.B5.a_1=to;
			if (ISP->arg.B5.a_2==from) ISP->arg.B5.a_2=to;
			if (ISP->arg.B5.a_3==from) ISP->arg.B5.a_3=to;
			if (ISP->arg.B5.a_4==from) ISP->arg.B5.a_4=to;
			break;
			case I_LSU0:
			case I_LAD0:
			if (ISP->arg.B6.a_0==from) ISP->arg.B6.a_0=to;
			if (ISP->arg.B6.a_1==from) ISP->arg.B6.a_1=to;
			if (ISP->arg.B6.a_2==from) ISP->arg.B6.a_2=to;
			if (ISP->arg.B6.a_3==from) ISP->arg.B6.a_3=to;
			if (ISP->arg.B6.a_4==from) ISP->arg.B6.a_4=to;
			if (ISP->arg.B6.a_5==from) ISP->arg.B6.a_5=to;
			break;
			case I_LAD3:
			case I_LSU3:
			case I_LMU3:
			if (ISP->arg.B8.a_0==from) ISP->arg.B8.a_0=to;
			if (ISP->arg.B8.a_1==from) ISP->arg.B8.a_1=to;
			if (ISP->arg.B8.a_2==from) ISP->arg.B8.a_2=to;
			if (ISP->arg.B8.a_3==from) ISP->arg.B8.a_3=to;
			if (ISP->arg.B8.a_4==from) ISP->arg.B8.a_4=to;
			if (ISP->arg.B8.a_5==from) ISP->arg.B8.a_5=to;
			if (ISP->arg.B8.a_6==from) ISP->arg.B8.a_6=to;
			if (ISP->arg.B8.a_7==from) ISP->arg.B8.a_7=to;
			break;
			case I_RET_:
			case I_LABL:
			case I_D32U:
			case I_R32U:
			case I_D32S:
			case I_R32S:
			case I_D64U:
			case I_R64U:
			case I_D64S:
			case I_R64S:
			case I_FCST:
			case I_FCEN:
			case I_JTEN:
			case I_JEND:
			printInstructionBufferWithMessageAndNumber(ib,"here",i);
			printf("Internal Error: I don\'t think reg rename should be going across that\n");
			exit(1);
			//case I_INSR:
			//case I_ERR_:
			//case I_DEPL:
			case I_PHIS:
			case I_PHIE:
			if (ISP->arg.B1.a_0!=from) break;
			default:
			printInstructionBufferWithMessageAndNumber(ib,"here",i);
			printf("Internal Error: invalid opcode during reg rename[%08X:%08X]{%01X->%01X}\n",start,end,from,to);
			exit(1);
			
		}
		consolePrintBufferPoint(ib,-1,i,2,"Renaming Registers");
	}
}

/*
regFrom should be used at the target
the target does not need to be the origin
if regTo==16, then this function will try to find an unused register and use it
*/
bool attemptRegRename(InstructionBuffer* ib, uint32_t target, uint8_t regFrom, uint8_t regTo){
	struct RegRenameInfo regRenameInfo;
	regRenameInfo.target=target;
	regRenameInfo.regFrom=regFrom;
	regRenameInfo.regTo=regTo;
	getRegRenameInfo(ib,&regRenameInfo);
	if (regRenameInfo.didSucceed){
		if (regTo==16) regTo=regRenameInfo.suggestedReg;
		//printf("{attemptRegRename() performing reg rename start}\n");
		//printf("[%08X:%08X | %01X:%01X]\n",regRenameInfo.lowerBound,regRenameInfo.upperBound,regFrom,regTo);
		//printInstructionBufferWithMessageAndNumber(ib,"before rename",target);
		doRegRename(ib,regRenameInfo.lowerBound,regRenameInfo.upperBound,regFrom,regTo);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		//printf("{attemptRegRename() performing reg rename end}\n");
	}
	return regRenameInfo.didSucceed;
}


void removeNop(InstructionBuffer* ib){
	InstructionSingle* buffer = ib->buffer;
	InstructionSingle* IS_i;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i;
	for (i=0;i<numberOfSlotsTaken;i++){
		consolePrintBufferPoint(ib,i,-1,0,"Finding NOP_ for Removal");
		if (buffer[i].id==I_NOP_){
			uint32_t delta=1;
			for (i++;i<numberOfSlotsTaken;i++){
				consolePrintBufferPoint(ib,i,i-delta,2,"Removing NOP_");
				if ((IS_i=&(buffer[i]))->id==I_NOP_){
					delta++;
				} else {
					buffer[i-delta]=*IS_i;
				}
			}
			ib->numberOfSlotsTaken-=delta;
			return;
		}
	}
}


// where possible, transforms 2*PU1_ -> PU2_ and 2*POP1 -> POP2
// ib should not already contain PU2_ or POP2
void contractPushPop(InstructionBuffer* ib){
	InstructionSingle* buffer = ib->buffer;
	InstructionSingle* IS_i0;
	InstructionSingle* IS_i1;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t count=0;
	for (uint32_t i0=0;i0<numberOfSlotsTaken;i0++){
		enum InstructionTypeID id0 = (IS_i0=&(buffer[i0]))->id;
		consolePrintBufferPoint(ib,i0,-1,0,"Finding Push/Pop for Contraction (1)");
		if (id0==I_PU1_ | id0==I_POP1){
			uint32_t i1;
			for (i1=i0+1;i1<numberOfSlotsTaken;i1++){
				enum InstructionTypeID id1 = (IS_i1=&(buffer[i1]))->id;
				consolePrintBufferPoint(ib,i1,-1,0,"Finding Push/Pop for Contraction (2)");
				if (id1==I_PU1_ | id1==I_POP1){
					if (id0==id1){
						if (i0+1==findLowerReorderBoundry(ib,i1)){
							enum InstructionTypeID id2;
							if (id1==I_PU1_){
								id2=I_PU2_;
							} else {
								id2=I_POP2;
							}
							IS_i0->id=id2;
							IS_i0->arg.B2.a_0=IS_i0->arg.B1.a_0;
							IS_i0->arg.B2.a_1=IS_i1->arg.B1.a_0;
							IS_i1->id=I_NOP_;
							count++;
						}
					}
					break;
				}
			}
			i0=i1;
		}
	}
	if (count!=0){
		removeNop(ib);
	}
}

// transforms PU2_ -> 2*PU1_  and POP2 -> 2*POP1
void expandPushPop(InstructionBuffer* ib){
	InstructionSingle* buffer = ib->buffer;
	uint32_t iNext;
	uint32_t i;
	enum InstructionTypeID id;
	{
		uint32_t countOfExpansion=0;
		uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
		for (i=0;i<numberOfSlotsTaken;i++){
			consolePrintBufferPoint(ib,i,-1,0,"Counting Push/Pop for Expansion");
			id = buffer[i].id;
			if (id==I_PU2_ | id==I_POP2) countOfExpansion++;
		}
		if (countOfExpansion==0) return;
		i = numberOfSlotsTaken;
		numberOfSlotsTaken+=countOfExpansion;
		ib->numberOfSlotsTaken=(iNext=numberOfSlotsTaken);
		if (numberOfSlotsTaken>ib->numberOfSlotsAllocated){
			ib->buffer = (buffer = cosmic_realloc(buffer,(ib->numberOfSlotsAllocated*=2)*sizeof(InstructionSingle)));
		}
	}
	InstructionSingle* IS_i;
	InstructionSingle IS_push;
	InstructionSingle IS_pop;
	IS_push.id=I_PU1_;
	IS_pop.id=I_POP1;
	while (i--!=iNext--){
		id=(IS_i=buffer+i)->id;
		if (id==I_PU2_){
			IS_push.arg.B1.a_0=IS_i->arg.B2.a_1;
			buffer[iNext--]=IS_push;
			IS_push.arg.B1.a_0=IS_i->arg.B2.a_0;
			buffer[iNext  ]=IS_push;
		} else if (id==I_POP2){
			IS_pop.arg.B1.a_0=IS_i->arg.B2.a_1;
			buffer[iNext--]=IS_pop;
			IS_pop.arg.B1.a_0=IS_i->arg.B2.a_0;
			buffer[iNext  ]=IS_pop;
		} else {
			buffer[iNext  ]=*IS_i;
		}
		consolePrintBufferPoint(ib,i,iNext,2,"Performing Push/Pop Expansion");
	}
}














