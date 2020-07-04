

// this should only be used by applyReorder() and insertInstructionAt()
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
			walkIndex=next;
		}
	} else {
		while (destination!=walkIndex){
			next = walkIndex-1;
			buffer[walkIndex]=buffer[next];
			walkIndex=next;
		}
	}
	buffer[destination]=targetInstruction;
}

void applyReorder(InstructionBuffer* ib,const uint32_t target,const uint32_t destination){
	if (target==destination) return;
	enum InstructionTypeID id=ib->buffer[target].id;
	if (id==I_SYRB | id==I_SYRW | id==I_SYRD){
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




void doLabelRename(InstructionBuffer* ib, uint32_t from, uint32_t to){
	InstructionSingle* ISP;
	InstructionSingle* buffer = ib->buffer;
	uint32_t i = ib->numberOfSlotsTaken;
	while (i--!=0){
		ISP = &(buffer[i]);
		if (ISP->id==I_LABL | ISP->id==I_SYCL | ISP->id==I_JTEN | ISP->id==I_FCST){
			if (ISP->arg.D.a_0==from) ISP->arg.D.a_0=to;
		} else if (ISP->id==I_JJMP){
			if (ISP->arg.BBD.a_2==from) ISP->arg.BBD.a_2=to;
		}
	}
}


// renames input and output registers. Assumes input is valid. range is inclusive.
void doRegRename(InstructionBuffer* ib, uint32_t start, uint32_t end, uint8_t from, uint8_t to){
	InstructionSingle* ISP;
	InstructionSingle* buffer = ib->buffer;
	for (uint32_t i=start;i<=end;i++){
		ISP = &(buffer[i]);
		if (ISP->id==I_STPI){
			printf("Internal Error: I_STPI got through\n");
			exit(1);
		}
		switch (ISP->id){
			case I_INSR:
			case I_ERR_:
			case I_DEPL:
			default:
			printInstructionBufferWithMessageAndNumber(ib,"here",i);
			printf("Internal Error: invalid opcode during reg rename\n");
			exit(1);
			case I_NOP_:
			case I_RET_:
			case I_LABL:
			case I_D32U:
			case I_R32U:
			case I_D32S:
			case I_R32S:
			case I_FCST:
			case I_FCEN:
			case I_JTEN:
			case I_JEND:
			case I_SYRE:
			case I_SYDB:
			case I_SYDW:
			case I_SYDD:
			case I_SYDE:
			case I_SYC0:
			case I_SYC1:
			case I_SYC2:
			case I_SYC3:
			case I_SYC4:
			case I_SYC5:
			case I_SYC6:
			case I_SYC7:
			case I_SYC8:
			case I_SYC9:
			case I_SYCB:
			case I_SYCW:
			case I_SYCD:
			case I_SYCL:
			case I_SYCZ:
			case I_SYCS:
			case I_SYCT:
			break;
			case I_PU1_:
			case I_PUA1:
			case I_POP1:
			case I_ALOC:
			case I_SYRW:
			case I_SYRB:
			if (ISP->arg.B.a_0==from){
				ISP->arg.B.a_0=to;
			}
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
			if (ISP->arg.BB.a_0==from){
				ISP->arg.BB.a_0=to;
			}
			if (ISP->arg.BB.a_1==from){
				ISP->arg.BB.a_1=to;
			}
			break;
			case I_CJMP:
			case I_AND_:
			case I_OR__:
			case I_XOR_:
			case I_ADDN:
			case I_ADDC:
			case I_SSUB:
			case I_MWWN:
			case I_MRWN:
			case I_MWWV:
			case I_MRWV:
			case I_SUBN:
			case I_SUBC:
			if (ISP->arg.BBB.a_0==from){
				ISP->arg.BBB.a_0=to;
			}
			if (ISP->arg.BBB.a_1==from){
				ISP->arg.BBB.a_1=to;
			}
			if (ISP->arg.BBB.a_2==from){
				ISP->arg.BBB.a_2=to;
			}
			break;
			case I_BL1_:
			case I_STWN:
			case I_STRN:
			case I_STWV:
			case I_STRV:
			if (ISP->arg.BB.a_0==from){
				ISP->arg.BB.a_0=to;
			}
			break;
			case I_RL1_:
			case I_ALCR:
			case I_STPA:
			case I_STPS:
			if (ISP->arg.BW.a_0==from){
				ISP->arg.BW.a_0=to;
			}
			break;
			case I_STOF:
			if (ISP->arg.BBW.a_0==from){
				ISP->arg.BBW.a_0=to;
			}
			if (ISP->arg.BBW.a_1==from){
				ISP->arg.BBW.a_1=to;
			}
			break;
			case I_RL2_:
			case I_JJMP:
			if (ISP->arg.BBD.a_0==from){
				ISP->arg.BBD.a_0=to;
			}
			if (ISP->arg.BBD.a_1==from){
				ISP->arg.BBD.a_1=to;
			}
			break;
		}
	}
}

/*
regFrom should be used at the target
the target does not need to be the origin
if regTo==16, then this function will try to find an unused register and use it
*/
bool attemptRegRename(InstructionBuffer* ib, uint32_t target, uint8_t regFrom, uint8_t regTo){
	if ( regFrom==0 | regFrom==1 | regTo==0 | regTo==1 ){
		return false; // then it is invalid to try to rename
	}
	struct RegRenameInfo regRenameInfo;
	regRenameInfo.target=target;
	regRenameInfo.regFrom=regFrom;
	regRenameInfo.regTo=regTo;
	getRegRenameInfo(ib,&regRenameInfo);
	if (regRenameInfo.didSucceed){
		if (regTo==16) regTo=regRenameInfo.suggestedReg;
		doRegRename(ib,regRenameInfo.lowerBound,regRenameInfo.upperBound,regFrom,regTo);
	}
	return regRenameInfo.didSucceed;
}


void removeNop(InstructionBuffer* ib){
	InstructionSingle* buffer = ib->buffer;
	InstructionSingle* IS_i;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i;
	for (i=0;i<numberOfSlotsTaken;i++){
		if (buffer[i].id==I_NOP_){
			uint32_t delta=1;
			for (i++;i<numberOfSlotsTaken;i++){
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
		if (id0==I_PU1_ | id0==I_POP1){
			uint32_t i1;
			for (i1=i0+1;i1<numberOfSlotsTaken;i1++){
				enum InstructionTypeID id1 = (IS_i1=&(buffer[i1]))->id;
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
							IS_i0->arg.BB.a_0=IS_i0->arg.B.a_0;
							IS_i0->arg.BB.a_1=IS_i1->arg.B.a_0;
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
		uint32_t nextSlotsTaken;
		for (i=0;i<numberOfSlotsTaken;i++){
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
			IS_push.arg.B.a_0=IS_i->arg.BB.a_1;
			buffer[iNext--]=IS_push;
			IS_push.arg.B.a_0=IS_i->arg.BB.a_0;
			buffer[iNext  ]=IS_push;
		} else if (id==I_POP2){
			IS_pop.arg.B.a_0=IS_i->arg.BB.a_1;
			buffer[iNext--]=IS_pop;
			IS_pop.arg.B.a_0=IS_i->arg.BB.a_0;
			buffer[iNext  ]=IS_pop;
		} else {
			buffer[iNext  ]=*IS_i;
		}
	}
}














