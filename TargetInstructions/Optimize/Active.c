
// `true` indicates it made progress in splitting, not that the instruction is completely split of all modifies
bool attemptSplitModify(InstructionBuffer* ib, uint32_t index){
	InstructionSingle IS_insert={.id=I_MOV_};
	InstructionSingle* IS=ib->buffer+index;
	InstructionSingle* IS_prev=IS-1;
	enum InstructionTypeID id=IS->id;
	bool b;
	if (id==I_ADDN){
		if ((b=IS->arg.B3.a_0==IS->arg.B3.a_1) | IS->arg.B3.a_0==IS->arg.B3.a_2){
			uint8_t unused=findUnusedRegisterAfter(ib,index,IS->arg.B3.a_0,b?IS->arg.B3.a_2:IS->arg.B3.a_1);
			
			if (unused!=16){
				IS->arg.B3.a_0=unused;
				IS_insert.arg.B2.a_1=unused;
				IS_insert.arg.B2.a_0=b?IS->arg.B3.a_1:IS->arg.B3.a_2;
				insertInstructionAt(ib,index+1,IS_insert);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				return true;
			}
		}
	} else if (id==I_MOV_){
		if (IS->arg.B2.a_0==IS->arg.B2.a_1 & IS_prev->id==I_POP1 & IS_prev->arg.B1.a_0==IS->arg.B2.a_1){
			// this is used for aggressive push->pop reduction when the pop couldn't be renamed easily
			uint8_t unused=findUnusedRegisterAfter(ib,index,IS->arg.B2.a_0,16);
			if (unused!=16){
				IS_prev->arg.B1.a_0=unused;
				IS->arg.B2.a_1=unused;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				return true;
			}
		}
	}
	return false;
}

bool attemptJmpOpt(InstructionBuffer* ib){
	// todo: add some things to check for PHIS that don't have PHIE on the other side of the label (in which case the PHIS could be removed)
	InstructionSingle* buffer = ib->buffer;
	InstructionSingle IS;
	bool didSucceedAtLeastOnce_0=false;
	bool didSucceedAtLeastOnce_1;
	uint32_t subI;
	do {
		didSucceedAtLeastOnce_1=false;
		uint32_t i=ib->numberOfSlotsTaken;
		while (i--!=0){
			enum InstructionTypeID id=buffer[i].id;
			if (id==I_AJMP | id==I_CJMP){
				IS=buffer[i];
				if (id==I_CJMP){
					uint16_t jumpValue;
					uint8_t r0=IS.arg.B3.a_0;
					uint8_t r1=IS.arg.B3.a_1;
					uint8_t r2=IS.arg.B3.a_2;
					if (getValueInRegisterIfTraceableToRawConstants(ib,i-1,r2,&jumpValue)){
						// IS is now used to fill in values, not read them
						if (jumpValue==0){
							IS.id=I_AJMP; 
// this cannot run sanity check because it changes CJMP to AJMP, which will cause regs to not have origins. Which is fine, because that area is unreachable anyway, however jmp opt has to finish everything before other passes run
							IS.arg.B2.a_0=r0;
							IS.arg.B2.a_1=r1;
							buffer[i]=IS;
						} else {
							buffer[i].id=I_NOP_;
							subI=i;
							while (subI--!=0){
								enum InstructionTypeID* idPtr=&(ib->buffer[subI].id);
								if (*idPtr==I_PHIS) *idPtr=I_NOP_;
								else break;
							}
						}
						didSucceedAtLeastOnce_1=true;
						continue;
					}
				}
				uint32_t labelNumber;
				uint32_t labelLoadLocation;
				if (traceJumpLabel(ib,i,&labelNumber,&labelLoadLocation)){
					uint32_t locationOfLabel = findLabelLocation(ib,labelNumber);
					assert(buffer[locationOfLabel].id==I_LABL);
					if ((i+1)==locationOfLabel){
						buffer[i].id=I_NOP_;
						didSucceedAtLeastOnce_1=true;
						continue;
					}
					if (buffer[labelLoadLocation+1].id==I_SYCL & (locationOfLabel+4)<ib->numberOfSlotsTaken){
						
						InstructionSingle* temp_IS_0 = buffer+locationOfLabel;
						InstructionSingle* temp_IS_1 = temp_IS_0+1;
						InstructionSingle* temp_IS_2 = temp_IS_1+1;
						InstructionSingle* temp_IS_3 = temp_IS_2+1;
						InstructionSingle* temp_IS_4 = temp_IS_3+1;
						if ((temp_IS_1->id==I_SYRD & temp_IS_2->id==I_SYCL & temp_IS_3->id==I_SYRE & temp_IS_4->id==I_AJMP) &&
							(temp_IS_1->arg.B2.a_0==temp_IS_4->arg.B2.a_0 & temp_IS_1->arg.B2.a_1==temp_IS_4->arg.B2.a_1)){
							
							buffer[labelLoadLocation+1].arg.D.a_0=temp_IS_2->arg.D.a_0;
							didSucceedAtLeastOnce_1=true;
							continue;
						}
					}
				}
			} else if (id==I_LABL){
				IS=buffer[i];
				if (i!=0 && buffer[i-1].id==I_LABL){
					uint32_t to=buffer[i-1].arg.D.a_0;
					uint32_t from=buffer[i].arg.D.a_0;
					buffer[i].id=I_NOP_;
					doLabelRename(ib,from,to);
					didSucceedAtLeastOnce_1=true;
					continue;
				}
				if (!doesLabelHaveUsage(ib,buffer[i].arg.D.a_0)){
					buffer[i].id=I_NOP_;
					didSucceedAtLeastOnce_1=true;
					for (subI=i+1;subI<ib->numberOfSlotsTaken;subI++){
						enum InstructionTypeID* idPtr=&(ib->buffer[subI].id);
						if (*idPtr==I_PHIE) *idPtr=I_NOP_;
						else break;
					}
					subI=i;
					while (subI--!=0){
						enum InstructionTypeID* idPtr=&(ib->buffer[subI].id);
						if (*idPtr==I_PHIS) *idPtr=I_NOP_;
						else break;
					}
					continue;
				}
			}
			if (id==I_AJMP | id==I_RET_ | id==I_JEND){
				bool setNewUnreachable=false;
				for (subI=i+1;subI<ib->numberOfSlotsTaken;subI++){
					enum InstructionTypeID* idPtr=&(ib->buffer[subI].id);
					if (*idPtr!=I_NOP_){
						if (*idPtr==I_LABL | *idPtr==I_FCEN) break;
						setNewUnreachable=true;
						*idPtr=I_NOP_;
					}
				}
				if (setNewUnreachable){
					didSucceedAtLeastOnce_1=true;
					continue;
				}
			}
		}
		if (didSucceedAtLeastOnce_1) removeNop(ib);
		i=ib->numberOfSlotsTaken;
		while (i-- >8){
			InstructionSingle* base_IS_ptr=buffer+i;

			if (
			((base_IS_ptr-0)->id==I_LABL) &
			((base_IS_ptr-4)->id==I_SYRD) &
			((base_IS_ptr-5)->id==I_CJMP)
			){
				if (
				((base_IS_ptr-1)->id==I_AJMP) &
				((base_IS_ptr-2)->id==I_SYRE) &
				((base_IS_ptr-3)->id==I_SYCL) &
				((base_IS_ptr-6)->id==I_SYRE) &
				((base_IS_ptr-7)->id==I_SYCL) &
				((base_IS_ptr-8)->id==I_SYRD)
				){
					if (
					(base_IS_ptr-8)->arg.B2.a_0==(base_IS_ptr-5)->arg.B2.a_0 &
					(base_IS_ptr-8)->arg.B2.a_1==(base_IS_ptr-5)->arg.B2.a_1 &
					(base_IS_ptr-4)->arg.B2.a_0==(base_IS_ptr-1)->arg.B2.a_0 &
					(base_IS_ptr-4)->arg.B2.a_1==(base_IS_ptr-1)->arg.B2.a_1 &
					(base_IS_ptr-7)->arg.D .a_0==(base_IS_ptr-0)->arg.D .a_0
					){
						uint8_t rCJMP=(base_IS_ptr-5)->arg.B3.a_2;
						uint8_t rTemp;
						(rCJMP==5)?(rTemp=4):(rTemp=5);
						didSucceedAtLeastOnce_1=true;
						*(base_IS_ptr-1)=*(base_IS_ptr-5);
						*(base_IS_ptr-4)=*(base_IS_ptr-8);
						InstructionSingle temp_IS;
						
// rTemp just needs to be different then rCJMP, because there was an AJMP the reg values would be indeterminate anyway
						temp_IS.id=I_XOR_;
						temp_IS.arg.B3.a_0=rCJMP;
						temp_IS.arg.B3.a_1=rCJMP;
						temp_IS.arg.B3.a_2=rTemp;
						*(base_IS_ptr-5)=temp_IS;
						temp_IS.id=I_BL1_;
						temp_IS.arg.B2.a_0=rCJMP;
						temp_IS.arg.B2.a_1=1;
						*(base_IS_ptr-6)=temp_IS;
						temp_IS.id=I_SSUB;
						temp_IS.arg.B3.a_0=rTemp;
						temp_IS.arg.B3.a_1=rCJMP;
						temp_IS.arg.B3.a_2=rTemp;
						*(base_IS_ptr-7)=temp_IS;
						temp_IS.id=I_BL1_;
						temp_IS.arg.B2.a_0=rTemp;
						temp_IS.arg.B2.a_1=0;
						*(base_IS_ptr-8)=temp_IS;
					}
				}
			}
		}
		didSucceedAtLeastOnce_0|=didSucceedAtLeastOnce_1;
	} while (didSucceedAtLeastOnce_1);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	return didSucceedAtLeastOnce_0;
}



bool attemptConstOpt(InstructionBuffer* ib){
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"starting const opt",0);
#endif
	bool didSucceedAtLeastOnce_0=false;
	bool didSucceedAtLeastOnce_1=false;
	bool isQuickEndTriggered=false;
	uint32_t i;
	while (true){
		didSucceedAtLeastOnce_0|=didSucceedAtLeastOnce_1;
		didSucceedAtLeastOnce_1=false;
		InstructionSingle* buffer=ib->buffer;
		InstructionInformation II;
		{
			bool isRegValueUnused[17];
			isRegValueUnused[ 2]=1;
			isRegValueUnused[ 3]=1;
			isRegValueUnused[ 4]=1;
			isRegValueUnused[ 5]=1;
			isRegValueUnused[ 6]=1;
			isRegValueUnused[ 7]=1;
			isRegValueUnused[ 8]=1;
			isRegValueUnused[ 9]=1;
			isRegValueUnused[10]=1;
			isRegValueUnused[11]=1;
			isRegValueUnused[12]=1;
			isRegValueUnused[13]=1;
			isRegValueUnused[14]=1;
			// isRegValueUnused[15] doesn't matter
			isRegValueUnused[16]=1;
			i=ib->numberOfSlotsTaken;
			while (i--!=0){
				InstructionSingle writeIS;
				UnusedResultLoopStart:
				fillInstructionInformation(&II,ib,i);
				if (II.isSymbolicInternal | II.id==I_NOP_) continue;
				isRegValueUnused[16]=1;
				if (II.regIN[0]==II.regIN[1] & II.regIN[0]!=16){
					if (II.id==I_XOR_){
						writeIS.id=I_BL1_;
						writeIS.arg.B2.a_0=II.regOUT[0];
						writeIS.arg.B2.a_1=0;
					} else if (II.id==I_OR__){
						writeIS.id=I_MOV_;
						writeIS.arg.B2.a_0=II.regOUT[0];
						writeIS.arg.B2.a_1=II.regIN[0];
					} else if (II.id==I_AND_){
						writeIS.id=I_MOV_;
						writeIS.arg.B2.a_0=II.regOUT[0];
						writeIS.arg.B2.a_1=II.regIN[0];
					} else {
						goto ExitForNoSpecial_0;
					}
					didSucceedAtLeastOnce_1=true;
					buffer[i]=writeIS;
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"const opt by special 0",i);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
					goto UnusedResultLoopStart;
				}
				ExitForNoSpecial_0:;
				
				if (!isRegValueUnused[II.regOUT[0]] & isRegValueUnused[II.regOUT[1]] & II.regOUT[1]!=16 & (II.id==I_LAD0 | II.id==I_LAD1 | II.id==I_LAD2 | II.id==I_MULL)){
					if (II.id==I_MULL){
						writeIS.id=I_MULS;
						writeIS.arg.B2.a_0=13;
						writeIS.arg.B2.a_1=II.regIN[0];
					} else {
						writeIS.arg.B3.a_2=II.id==I_LAD2?II.regIN[1]:II.regIN[2];
						writeIS.id=I_ADDN;
						writeIS.arg.B3.a_0=II.regOUT[0];
						writeIS.arg.B3.a_1=II.regIN[0];
					}
					didSucceedAtLeastOnce_1=true;
					buffer[i]=writeIS;
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"const opt by special 1",i);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
					goto UnusedResultLoopStart;
				}
				bool canRemove=
					(II.regOUT[0]!=16 & !(II.doesMoveStack | II.isMemoryAccess)) && (
					isRegValueUnused[II.regOUT[0]] &
					isRegValueUnused[II.regOUT[1]] &
					isRegValueUnused[II.regOUT[2]] &
					isRegValueUnused[II.regOUT[3]]);
				if (canRemove){
					didSucceedAtLeastOnce_1=true;
					buffer[i].id=I_NOP_; // by the way, this is fine for PHIE too
					if (II.id==I_SYRB | II.id==I_SYRW | II.id==I_SYRD | II.id==I_SYRQ){
						uint32_t iSub=i;
						bool isNotEnd;
						do {
							isNotEnd=buffer[++iSub].id!=I_SYRE;
							buffer[iSub].id=I_NOP_;
						} while (isNotEnd);
					}
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"const opt by result unused",i);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				}
				if (II.doesDestroyReg | II.stopLinearRegTrace){
					isRegValueUnused[ 2]=1;
					isRegValueUnused[ 3]=1;
					isRegValueUnused[ 4]=1;
					isRegValueUnused[ 5]=1;
					isRegValueUnused[ 6]=1;
					isRegValueUnused[ 7]=1;
					isRegValueUnused[ 8]=1;
					isRegValueUnused[ 9]=1;
					isRegValueUnused[10]=1;
					isRegValueUnused[11]=1;
					isRegValueUnused[12]=1;
					isRegValueUnused[13]=1;
					isRegValueUnused[14]=1;
				} else {
					isRegValueUnused[II.regOUT[0]]=1;
					isRegValueUnused[II.regOUT[1]]=1;
					isRegValueUnused[II.regOUT[2]]=1;
					isRegValueUnused[II.regOUT[3]]=1;
				}
				if (!canRemove){
					isRegValueUnused[II.regIN[0]]=0;
					isRegValueUnused[II.regIN[1]]=0;
					isRegValueUnused[II.regIN[2]]=0;
					isRegValueUnused[II.regIN[3]]=0;
					isRegValueUnused[II.regIN[4]]=0;
					isRegValueUnused[II.regIN[5]]=0;
					isRegValueUnused[II.regIN[6]]=0;
					isRegValueUnused[II.regIN[7]]=0;
				}
			}
		}
		if (isQuickEndTriggered) break; // in this case, running the passes below is not nessessary
		{
			bool isRegValueConst[17];
			isRegValueConst[ 2]=0;
			isRegValueConst[ 3]=0;
			isRegValueConst[ 4]=0;
			isRegValueConst[ 5]=0;
			isRegValueConst[ 6]=0;
			isRegValueConst[ 7]=0;
			isRegValueConst[ 8]=0;
			isRegValueConst[ 9]=0;
			isRegValueConst[10]=0;
			isRegValueConst[11]=0;
			isRegValueConst[12]=0;
			isRegValueConst[13]=0;
			isRegValueConst[14]=0;
			bool isRegINconst[8];
			uint16_t constVal[17]={0};
			uint16_t partialConstApplyCount;
			uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
			InstructionSingle writeIS_0;
			InstructionSingle writeIS_1;
			for (i=0;i<numberOfSlotsTaken;i++){
				ConstifyLoopStart:
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				fillInstructionInformation(&II,ib,i);
				if (II.isSymbolicInternal | II.id==I_NOP_) continue;
				// `unsigned` is used here to avoid potential promotions to signed int
				isRegValueConst[16]=1;
				isRegINconst[0]=isRegValueConst[II.regIN[0]];
				unsigned cv0=constVal[II.regIN[0]];
				isRegINconst[1]=isRegValueConst[II.regIN[1]];
				unsigned cv1=constVal[II.regIN[1]];
				isRegINconst[2]=isRegValueConst[II.regIN[2]];
				unsigned cv2=constVal[II.regIN[2]];
				isRegINconst[3]=isRegValueConst[II.regIN[3]];
				unsigned cv3=constVal[II.regIN[3]];
				isRegINconst[4]=isRegValueConst[II.regIN[4]];
				unsigned cv4=constVal[II.regIN[4]];
				isRegINconst[5]=isRegValueConst[II.regIN[5]];
				unsigned cv5=constVal[II.regIN[5]];
				isRegINconst[6]=isRegValueConst[II.regIN[6]];
				unsigned cv6=constVal[II.regIN[6]];
				isRegINconst[7]=isRegValueConst[II.regIN[7]];
				unsigned cv7=constVal[II.regIN[7]];
				if ((
					isRegINconst[0] & isRegINconst[1] &
					isRegINconst[2] & isRegINconst[3] &
					isRegINconst[4] & isRegINconst[5] &
					isRegINconst[6] & isRegINconst[7]) &&
					(!(II.doesMoveStack | II.isMemoryAccess) & 
					II.id!=I_MOV_ & II.regIN[0]!=16)
				){
					writeIS_0.id=I_RL1_; writeIS_0.arg.BW.a_0=II.regOUT[0]; // this is typically what is needed there, otherwise it is modified
					switch (II.id){
					case I_ADDN:
					writeIS_0.arg.BW.a_1=cv0+cv1;
					break;
					case I_SUBN:
					writeIS_0.arg.BW.a_1=cv0-cv1;
					break;
					case I_SUBC:
					writeIS_0.arg.BW.a_1=(((uint32_t)cv0+(~(uint32_t)cv1&0xFFFFu)+1u)&(uint32_t)0x00010000lu)!=0u;
					break;
					case I_SHFT:
					writeIS_0.arg.BW.a_1=cv0>>1;
					break;
					case I_BSWP:
					writeIS_0.arg.BW.a_1=((cv0>>8)&0x00FFu)|((cv0<<8)&0xFF00u);
					break;
					case I_AND_:
					writeIS_0.arg.BW.a_1=cv0&cv1;
					break;
					case I_OR__:
					writeIS_0.arg.BW.a_1=cv0|cv1;
					break;
					case I_XOR_:
					writeIS_0.arg.BW.a_1=cv0^cv1;
					break;
					case I_MULS:
					writeIS_0.arg.BW.a_1=cv0*cv1;
					break;
					case I_SSUB:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[1];
					writeIS_0.arg.BBD.a_1=II.regOUT[0];
					writeIS_0.arg.BBD.a_2=(uint32_t)cv0+(uint32_t)cv1+(~(uint32_t)cv2&0xFFFFu);
					writeIS_0.arg.BBD.a_2=((uint32_t)((writeIS_0.arg.BBD.a_2&(uint32_t)0x00030000lu)!=0u)<<16)|(writeIS_0.arg.BBD.a_2&0xFFFFu); // this step ensures that the value is calculated exactly.
					break;
					case I_LAD0:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)+(((uint32_t)cv3<<16)|(uint32_t)cv2);
					break;
					case I_LAD1:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)+(uint32_t)cv2;
					break;
					case I_LAD2:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(uint32_t)cv0+(uint32_t)cv1;
					break;
					case I_LSU0:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)-(((uint32_t)cv3<<16)|(uint32_t)cv2);
					break;
					case I_LLS6:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)<<cv2;
					break;
					case I_LRS6:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=II.regOUT[0];
					writeIS_0.arg.BBD.a_1=II.regOUT[1];
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)>>cv2;
					break;
					case I_MULL:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=13;
					writeIS_0.arg.BBD.a_1=14;
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)*(((uint32_t)cv3<<16)|(uint32_t)cv2);
					break;
					case I_D32U:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=8;
					writeIS_0.arg.BBD.a_1=9;
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)/(((uint32_t)cv3<<16)|(uint32_t)cv2);
					break;
					case I_R32U:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=6;
					writeIS_0.arg.BBD.a_1=7;
					writeIS_0.arg.BBD.a_2=(((uint32_t)cv1<<16)|(uint32_t)cv0)%(((uint32_t)cv3<<16)|(uint32_t)cv2);
					break;
					case I_D32S:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=8;
					writeIS_0.arg.BBD.a_1=9;
					writeIS_0.arg.BBD.a_2=((( int32_t)cv1<<16)|( int32_t)cv0)/((( int32_t)cv3<<16)|( int32_t)cv2);
					break;
					case I_R32S:
					writeIS_0.id=I_RL2_;
					writeIS_0.arg.BBD.a_0=6;
					writeIS_0.arg.BBD.a_1=7;
					writeIS_0.arg.BBD.a_2=((( int32_t)cv1<<16)|( int32_t)cv0)%((( int32_t)cv3<<16)|( int32_t)cv2);
					break;
					default:goto ConstifySwitchExit;
					}
					didSucceedAtLeastOnce_1=true;
					buffer[i]=writeIS_0;
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"const opt by result known",i);
#endif
					goto ConstifyLoopStart;
				}
				ConstifySwitchExit:;
				bool t; // 't' is a temporary value used in the switch statement below
				switch (II.id){
					case I_RL2_:
					isRegValueConst[II.regOUT[0]]=1;
					isRegValueConst[II.regOUT[1]]=1;
					constVal[II.regOUT[0]]=(uint16_t)(II.cv>> 0);
					constVal[II.regOUT[1]]=(uint16_t)(II.cv>>16);
					continue;
					case I_RL1_:
					case I_BL1_:
					isRegValueConst[II.regOUT[0]]=1;
					constVal[II.regOUT[0]]=II.cv;
					continue;
					case I_MOV_:
					isRegValueConst[II.regOUT[0]]=isRegValueConst[II.regIN[0]];
					constVal[II.regOUT[0]]=constVal[II.regIN[0]];
					continue;
					case I_ADDN:
					if ((t=(isRegINconst[0] & cv0==0))|(isRegINconst[1] & cv1==0)){
						writeIS_0.arg.B2.a_1=t?II.regIN[1]:II.regIN[0];
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_AND_:
					if ((isRegINconst[0] & cv0==0)|(isRegINconst[1] & cv1==0)){
						writeIS_0.id=I_BL1_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						writeIS_0.arg.B2.a_1=0;
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if ((t=(isRegINconst[0] & cv0==0xFFFFu))|(isRegINconst[1] & cv1==0xFFFFu)){
						writeIS_0.arg.B2.a_1=t?II.regIN[1]:II.regIN[0];
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_OR__:
					if ((t=(isRegINconst[0] & cv0==0))|(isRegINconst[1] & cv1==0)){
						writeIS_0.arg.B2.a_1=t?II.regIN[1]:II.regIN[0];
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if ((t=(isRegINconst[0] & cv0==0xFFFFu))|(isRegINconst[1] & cv1==0xFFFFu)){
						writeIS_0.arg.B2.a_1=t?II.regIN[0]:II.regIN[1];
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_XOR_:
					if ((t=(isRegINconst[0] & cv0==0))|(isRegINconst[1] & cv1==0)){
						writeIS_0.arg.B2.a_1=t?II.regIN[1]:II.regIN[0];
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_MULS:
					if ((isRegINconst[0] & cv0==0)|(isRegINconst[1] & cv1==0)){
						writeIS_0.id=I_BL1_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						writeIS_0.arg.B2.a_1=0;
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if (isRegINconst[0] & cv0==1){
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=II.regOUT[0];
						writeIS_0.arg.B2.a_1=II.regIN[1];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if (isRegINconst[1] & cv1==1){
						buffer[i].id=I_NOP_;
						goto ConstifyLoopStart; // trivial, don't bother using ApplyPartialConst
					}
					break;
					case I_MULL:
					if (((isRegINconst[0] & cv0==0)&(isRegINconst[1] & cv1==0))|((isRegINconst[2] & cv2==0)&(isRegINconst[3] & cv3==0))){
						writeIS_0.id=I_RL2_;
						writeIS_0.arg.BBD.a_0=13;
						writeIS_0.arg.BBD.a_1=14;
						writeIS_0.arg.BBD.a_2=0;
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if ((isRegINconst[0] & cv0==1)&(isRegINconst[1] & cv1==0)){
						buffer[i].id=I_NOP_;
						goto ConstifyLoopStart; // trivial, don't bother using ApplyPartialConst
					}
					if ((isRegINconst[2] & cv2==1)&(isRegINconst[3] & cv3==0)){
						writeIS_0.id=I_MOV_;
						writeIS_0.arg.B2.a_0=13;
						writeIS_0.arg.B2.a_1=II.regIN[0];
						writeIS_1.id=I_MOV_;
						writeIS_1.arg.B2.a_0=14;
						writeIS_1.arg.B2.a_1=II.regIN[1];
						partialConstApplyCount=2;
						goto ApplyPartialConst;
					}
					break;
					case I_LAD0:
					if (isRegINconst[3] & cv3==0){
						writeIS_0.id=I_LAD1;
						writeIS_0.arg.B5.a_0=II.regOUT[0];
						writeIS_0.arg.B5.a_1=II.regOUT[1];
						writeIS_0.arg.B5.a_2=II.regIN[0];
						writeIS_0.arg.B5.a_3=II.regIN[1];
						writeIS_0.arg.B5.a_4=II.regIN[2];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					if (isRegINconst[1] & cv1==0){
						writeIS_0.id=I_LAD1;
						writeIS_0.arg.B5.a_0=II.regOUT[0];
						writeIS_0.arg.B5.a_1=II.regOUT[1];
						writeIS_0.arg.B5.a_2=II.regIN[2];
						writeIS_0.arg.B5.a_3=II.regIN[3];
						writeIS_0.arg.B5.a_4=II.regIN[0];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_LAD1:
					if (isRegINconst[1] & cv1==0){
						writeIS_0.id=I_LAD2;
						writeIS_0.arg.B4.a_0=II.regOUT[0];
						writeIS_0.arg.B4.a_1=II.regOUT[1];
						writeIS_0.arg.B4.a_2=II.regIN[0];
						writeIS_0.arg.B4.a_3=II.regIN[2];
						partialConstApplyCount=1;
						goto ApplyPartialConst;
					}
					break;
					case I_LSU0:
					if ((isRegINconst[2] & cv2==0) & (isRegINconst[3] & cv3==0)){
						buffer[i].id=I_NOP_;
						goto ConstifyLoopStart; // trivial, don't bother using ApplyPartialConst
					}
					break;
					case I_LLS6: // what is neat about this one: it natually destroys registers, so the optimization application can use any registers with no repercussions
					if (isRegINconst[2]){
						if (cv2>31){
							writeIS_0.id=I_BL1_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=0;
							writeIS_1.id=I_BL1_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=0;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
						if (cv2==0){
							writeIS_0.id=I_MOV_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=6;
							writeIS_1.id=I_MOV_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=7;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
						if (cv2==16){
							writeIS_0.id=I_BL1_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=0;
							writeIS_1.id=I_MOV_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=6;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
					}
					break;
					case I_LRS6: // what is neat about this one: it natually destroys registers, so the optimization application can use any registers with no repercussions
					if (isRegINconst[2]){
						if (cv2>31){
							writeIS_0.id=I_BL1_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=0;
							writeIS_1.id=I_BL1_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=0;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
						if (cv2==0){
							writeIS_0.id=I_MOV_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=6;
							writeIS_1.id=I_MOV_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=7;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
						if (cv2==16){
							writeIS_0.id=I_MOV_;
							writeIS_0.arg.B2.a_0=2;
							writeIS_0.arg.B2.a_1=7;
							writeIS_1.id=I_BL1_;
							writeIS_1.arg.B2.a_0=3;
							writeIS_1.arg.B2.a_1=0;
							partialConstApplyCount=2;
							goto ApplyPartialConst;
						}
					}
					break;
					// todo: add more checks here (mainly for MULL and 32 bit division)
					
					
					
					ApplyPartialConst:;
					switch (partialConstApplyCount){
						case 1:
						buffer[i]=writeIS_0;
						break;
						case 2:
						buffer[i]=writeIS_0;
						insertInstructionAt(ib,i+1,writeIS_1);
						buffer=ib->buffer;
						break;
						default:;assert(false);
					}
					goto ConstifyLoopStart;
					default:; // used to avoid warnings about missing enumeration values
				}
				isRegValueConst[II.regOUT[0]]=0;
				isRegValueConst[II.regOUT[1]]=0;
				if (II.doesDestroyReg | II.stopLinearRegTrace){
					isRegValueConst[ 2]=0;
					isRegValueConst[ 3]=0;
					isRegValueConst[ 4]=0;
					isRegValueConst[ 5]=0;
					isRegValueConst[ 6]=0;
					isRegValueConst[ 7]=0;
					isRegValueConst[ 8]=0;
					isRegValueConst[ 9]=0;
					isRegValueConst[10]=0;
					isRegValueConst[11]=0;
					isRegValueConst[12]=0;
					isRegValueConst[13]=0;
					isRegValueConst[14]=0;
				}
			}
		}
		if (didSucceedAtLeastOnce_1) continue; // seperating RL2_ and RL1_->BL1_ don't need to happen yet if either of the two passes above succeed
		{
			InstructionSingle IS_0;
			InstructionSingle IS_1;
			IS_0.id=I_RL1_;
			IS_1.id=I_RL1_;
			isQuickEndTriggered=true;
			i=ib->numberOfSlotsTaken;
			while (i--!=0){
				if (buffer[i].id==I_RL2_){
					didSucceedAtLeastOnce_1=true;
					InstructionSingle IS_this=buffer[i];
					IS_0.arg.BW.a_0=IS_this.arg.BBD.a_0;
					IS_1.arg.BW.a_0=IS_this.arg.BBD.a_1;
					IS_0.arg.BW.a_1=IS_this.arg.BBD.a_2>> 0;
					IS_1.arg.BW.a_1=IS_this.arg.BBD.a_2>>16;
					buffer[i]=IS_0;
					insertInstructionAt(ib,i,IS_1);
					buffer=ib->buffer; // buffer pointer may have changed due to potential expansion by insertInstructionAt()
#ifdef OPT_DEBUG_CONST
printInstructionBufferWithMessageAndNumber(ib,"const opt by seperating RL2_",i);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				}
			}
			IS_0.id=I_BL1_;
			i=ib->numberOfSlotsTaken;
			while (i--!=0){
				if (buffer[i].id==I_RL1_){
					IS_1=buffer[i];
					if (IS_1.arg.BW.a_1<256){
						IS_0.arg.B2.a_0=IS_1.arg.BW.a_0;
						IS_0.arg.B2.a_1=IS_1.arg.BW.a_1;
						buffer[i]=IS_0;
					}
				}
			}
		}
		if (!didSucceedAtLeastOnce_1) break;
	}
	if (didSucceedAtLeastOnce_0) removeNop(ib);
	return didSucceedAtLeastOnce_0;
}



// expandPushPop() should be run on the input before this is run
bool attemptMovPushPopOpt(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i0=numberOfSlotsTaken;
	while (i0--!=0){
		LoopStart:;
		InstructionSingle* IS_i0 = ib->buffer+i0;
		enum InstructionTypeID id0 = IS_i0->id;
		if (id0==I_MOV_){
			/*
			This message is relatively new:
			
			Rarely, this situation gets past mov opt, which should eliminate the second instruction:
			
			MOV_ %2 %D
			MOV_ %D %2
			
			This case was spotted without any instructions inbetween, 
			but the analysis would get more complicated if there were any instructions in between.
			*/
			bool didThisReduceSucceed=false;
			uint8_t rFrom = IS_i0->arg.B2.a_1;
			uint8_t rTo = IS_i0->arg.B2.a_0;
			if (rFrom==rTo){
				// moving a register to itself obviously always does nothing
				IS_i0->id=I_NOP_;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				didSucceedAtLeastOnce=true;
				didThisReduceSucceed=true;
			} else {
				InstructionInformation II_1;
				bool isToWritten = false;
				bool isFromWritten = false;
				bool wasToWrittenWhenFromWritten = false;
				bool wasFromWrittenWhenToWritten = false;
				bool wasToModifiedWhenFromWritten = false;
				bool wasFromModifiedWhenToWritten = false;
				bool wasToUsedAfterFromChanged = false;
				bool wasFromUsedAfterToChanged = false;
				bool isToModified = false;
				bool isFromModified = false;
				bool wasToWrittenWhenFromModified = false;
				bool wasFromWrittenWhenToModified = false;
				bool wasToModifiedWhenFromModified = false;
				bool wasFromModifiedWhenToModified = false;
				bool isToUsed = false;
				bool isFromUsed = false;
				bool containFromOut;
				bool containFromIn;
				bool containToOut;
				bool containToIn;
				bool cache_isToWritten;
				bool cache_isToModified;
				bool cache_isFromWritten;
				bool cache_isFromModified;
				uint32_t indexOfFromModify;
				uint32_t indexOfToModify;
				for (uint32_t i1=i0+1;i1<numberOfSlotsTaken;i1++){
					fillInstructionInformation(&II_1,ib,i1);
					if (II_1.isSymbolicInternal | II_1.id==I_NOP_) continue;
					containFromOut=doesRegListContain(II_1.regOUT,rFrom);
					containFromIn=doesRegListContain(II_1.regIN,rFrom);
					containToOut=doesRegListContain(II_1.regOUT,rTo);
					containToIn=doesRegListContain(II_1.regIN,rTo);
					
					cache_isToWritten = isToWritten;
					cache_isToModified = isToModified;
					cache_isFromWritten = isFromWritten;
					cache_isFromModified = isFromModified;
					
					if (II_1.doesDestroyReg | II_1.stopLinearRegTrace){
						if (!isToWritten & containToIn){
							isToUsed=true;
							wasToUsedAfterFromChanged=isFromModified | isFromWritten;
						}
						if (!isFromWritten & containFromIn){
							isFromUsed=true;
							wasFromUsedAfterToChanged=isToModified | isToWritten;
						}
						break;
					} else {
						if (!isFromWritten & containFromOut){
							if (containFromIn){
								isFromModified=true;
								indexOfFromModify=i1;
								wasToWrittenWhenFromModified=cache_isToWritten;
								wasToModifiedWhenFromModified=cache_isToModified;
							} else {
								isFromWritten=true;
								wasToWrittenWhenFromWritten=cache_isToWritten;
								wasToModifiedWhenFromWritten=cache_isToModified;
							}
						}
						if (!isToWritten & containToOut){
							if (containToIn){
								isToModified=true;
								indexOfToModify=i1;
								wasFromWrittenWhenToModified=cache_isFromWritten;
								wasFromModifiedWhenToModified=cache_isFromModified;
							} else {
								isToWritten=true;
								wasFromWrittenWhenToWritten=cache_isFromWritten;
								wasFromModifiedWhenToWritten=cache_isFromModified;
							}
						}
						if (isToWritten & isFromWritten){
							break;
						}
						if (!isToWritten & containToIn){
							isToUsed=true;
							wasToUsedAfterFromChanged=isFromModified | isFromWritten;
						}
						if (!isFromWritten & containFromIn){
							isFromUsed=true;
							wasFromUsedAfterToChanged=isToModified | isToWritten;
						}
					}
				}
				if (!isToUsed){
					// if what it is going to is not used then there is no need to move anything
					IS_i0->id=I_NOP_;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
					didSucceedAtLeastOnce=true;
					didThisReduceSucceed=true;
				} else if (!wasToUsedAfterFromChanged & !wasFromUsedAfterToChanged){
					uint32_t upperRenameBound;
					if (findRegRenameBoundaryFromOrigin(ib,i0,rTo,&upperRenameBound)){
						//printf("{mov calling doRegRename at place 1 start}\n");
						if (!wouldRegRenameViolateMultiOutputLaws(ib,i0,upperRenameBound,rTo,rFrom)) {
							doRegRename(ib,i0,upperRenameBound,rTo,rFrom);
							//printf("{mov calling doRegRename at place 1 end}\n");
							IS_i0->id=I_NOP_; // do not need this mov instruction anymore, it is moving to the same register it is coming from
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							didSucceedAtLeastOnce=true;
							didThisReduceSucceed=true;
						}
					}
				}
				if (!isFromUsed & !didThisReduceSucceed){
					uint32_t backwardsRenameStart = i0;
					bool didFindFailingUsage = false;
					while (backwardsRenameStart!=0 && isValueInRegUsedAfterTarget(ib,backwardsRenameStart-1,rFrom,NULL)){
						--backwardsRenameStart;
						fillInstructionInformation(&II_1,ib,backwardsRenameStart);
						if ((II_1.usesReg_D & rFrom==13) | (II_1.usesReg_E & rFrom==14)){
							didFindFailingUsage = true; // can't rename
							break;
						}
						if (doesRegListContain(II_1.regIN,rTo) || doesRegListContain(II_1.regOUT,rTo)){
							//printf("{mov may call attemptRegRename at place 2 start}\n");
							if (!II_1.noRename && attemptRegRename(ib,backwardsRenameStart,rTo,16)){
#ifdef OPT_DEBUG_PUSH_POP_MOV
printInstructionBufferWithMessageAndNumber(ib,"mov rename to attempt to reduce",i0);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
								goto LoopStart; // if this rename succeeds, then try this instruction again from the start
							}
							//printf("{mov may call attemptRegRename at place 2 end}\n");
							didFindFailingUsage = true;
							break;
						}
					}
					if (!didFindFailingUsage){
						// this protects against a few invalid edge cases that get past the typical method above
						fillInstructionInformation(&II_1,ib,backwardsRenameStart);
						didFindFailingUsage=
							((II_1.usesReg_D & rFrom==13) | (II_1.usesReg_E & rFrom==14)) |
							(II_1.noRename && 
							(doesRegListContain(II_1.regIN,rTo) |
							doesRegListContain(II_1.regOUT,rTo) |
							doesRegListContain(II_1.regIN,rFrom) |
							doesRegListContain(II_1.regOUT,rFrom)
							));
					}
					if (!didFindFailingUsage){
						//printf("{mov calling doRegRename at place 3 start}\n");
						if (!(didFindFailingUsage=wouldRegRenameViolateMultiOutputLaws(ib,backwardsRenameStart,i0,rFrom,rTo))){
							doRegRename(ib,backwardsRenameStart,i0,rFrom,rTo);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							IS_i0->id=I_NOP_; // do not need this mov instruction anymore, it is moving to the same register it is coming from
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							//printf("{mov calling doRegRename at place 3 end}\n");
							didSucceedAtLeastOnce=true;
							didThisReduceSucceed=true;
						}
					}
				}
				if (!didThisReduceSucceed){
					if (isToModified){
						//printf("{mov calling attemptSplitModify at place 4}\n");
						if (attemptSplitModify(ib,indexOfToModify)){
							numberOfSlotsTaken=ib->numberOfSlotsTaken;
							goto LoopStart;
						}
					}
					if (isFromModified){
						//printf("{mov calling attemptSplitModify at place 5}\n");
						if (attemptSplitModify(ib,indexOfFromModify)){
							numberOfSlotsTaken=ib->numberOfSlotsTaken;
							goto LoopStart;
						}
					}
					uint32_t upperReorderBoundry=findUpperReorderBoundry(ib,i0);
					if (upperReorderBoundry!=i0){
						bool doReorder=true;
						while (ib->buffer[--upperReorderBoundry].id==I_MOV_){
							if (upperReorderBoundry<=i0){
								doReorder=false;
								break;
							}
						}
						++upperReorderBoundry;
						if (doReorder){
							//printf("{mov calling applyReorder at place 6}\n");
							applyReorder(ib,i0,upperReorderBoundry);
							// This (probably) will not cause infinite loops, because of the checking done above
#ifdef OPT_DEBUG_GENERAL_ACTIVE
							printf("*");
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							i0=upperReorderBoundry;
							goto LoopStart;
						}
					}
				}
			}
			if (didThisReduceSucceed){
#ifdef OPT_DEBUG_PUSH_POP_MOV
				printInstructionBufferWithMessageAndNumber(ib,"after mov opt",i0);
#endif
				i0+=3;
				if (i0>numberOfSlotsTaken){
					i0=numberOfSlotsTaken;
				}
			}
		} else if (id0==I_PU1_){
			uint32_t i1;
			bool didFind=false;
			bool hitImpossible=false;
			uint8_t rFrom = IS_i0->arg.B1.a_0;
			for (i1=i0+1;i1<numberOfSlotsTaken;i1++){
				InstructionInformation II_1;
				fillInstructionInformation(&II_1,ib,i1);
				if (II_1.id==I_POP1){
					didFind=true;
					break;
				} else if (II_1.doesBlockReorder | II_1.doesDestroyReg | II_1.doesRelyOnStack){
					hitImpossible=true;
					break;
				} 
			}
			if (didFind){
				uint32_t lowerBoundry=findLowerReorderBoundry(ib,i1);
				uint8_t rTo;
				bool isProperlyReorderable = i0+1==lowerBoundry;
				if (!isProperlyReorderable){
					rTo = ib->buffer[i1].arg.B1.a_0;
					uint32_t temp=i0;
					bool couldRename=true;
					while (isRegMentionedAtOrAfterTarget(ib,temp+1,rTo,&temp)){
						if (temp>=i1){
							break;
						}
						if (!attemptRegRename(ib,temp,rTo,16)){
							if (rTo==13 | rTo==14){
								if (attemptRegRename(ib,i1,rTo,16)){
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
									rTo = ib->buffer[i1].arg.B1.a_0;
									temp=i0;
									continue;
								}
								InstructionSingle IS_temp={.id=I_MOV_,.arg.B2.a_0=rTo,.arg.B2.a_1=rTo};
								insertInstructionAt(ib,i1+1,IS_temp);
#ifdef OPT_DEBUG_GENERAL_ACTIVE
									printf(".");
#endif
								if (attemptSplitModify(ib,i1+1)){
#ifdef OPT_DEBUG_GENERAL_ACTIVE
									printf("-");
#endif
									goto LoopStart;
								} else {
									ib->buffer[i1+1].id=I_NOP_;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
								}
							}
							couldRename=false;
							break;
						} else {
#ifdef OPT_DEBUG_PUSH_POP_MOV
							printInstructionBufferWithMessageAndNumber(ib,"after push/pop rename for attempt to reduce",i0);
#endif
						}
					}
					if (couldRename){
						if (temp==i0){
							printInstructionBufferWithMessageAndNumber(ib,"Look into this if it happens: odd, that would make more sense if it had found a usage\n",i0);
							printf("print out end\n");
							exit(1);
						}
						lowerBoundry=findLowerReorderBoundry(ib,i1);
						isProperlyReorderable = i0+1==lowerBoundry;
					}
				}
				if (isProperlyReorderable){
					rFrom = IS_i0->arg.B1.a_0;
					applyReorder(ib,i1,lowerBoundry);
					InstructionSingle* IS_i1 = IS_i0+1;
					rTo = IS_i1->arg.B1.a_0;
					IS_i0->id = I_MOV_;
					IS_i0->arg.B2.a_0 = rTo;
					IS_i0->arg.B2.a_1 = rFrom;
					IS_i1->id = I_NOP_;
					didSucceedAtLeastOnce=true;
#ifdef OPT_DEBUG_PUSH_POP_MOV
					printInstructionBufferWithMessageAndNumber(ib,"after push/pop opt",i0);
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
					i0++;
				}
			} else if (!hitImpossible){
				printInstructionBufferWithMessageAndNumber(ib,"Look into this if it happens: A push did not find the pop. that might be a problem\n",i0);
				printf("print out end\n");
				exit(1);
			}
		}
	}
	if (didSucceedAtLeastOnce){
		removeNop(ib);
	}
	return didSucceedAtLeastOnce;
}


bool attemptStackMemoryOpt(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	bool isStackSizeKnown=false;
	uint8_t target_1;
	uint16_t stackSize;
	uint16_t offsetValue;
	uint32_t i0;
	uint32_t i1;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* buffer=ib->buffer;
	InstructionInformation II_0;
	InstructionInformation II_1;
	InstructionSingle IS_BL1;
	IS_BL1.id=I_BL1_;
	IS_BL1.arg.B2.a_1=0;
	InstructionSingle IS_temp;
	for (i0=0;i0<numberOfSlotsTaken;i0++){
		enum InstructionTypeID id=buffer[i0].id;
		if (id==I_FCST | id==I_FCEN){
			isStackSizeKnown=false;
			if (id==I_FCST){
				IS_temp=buffer[i0];
				isStackSizeKnown=(IS_temp.arg.BWD.a_0!=0);
				stackSize=IS_temp.arg.BWD.a_1;
			}
		} else if (
				id==I_MWWV |
				id==I_MWWN |
				id==I_MRWV |
				id==I_MRWN |
				id==I_MWBV |
				id==I_MWBN |
				id==I_MRBV |
				id==I_MRBN){
			
			i1=i0;
			fillInstructionInformation(&II_0,ib,i0);
			IS_BL1.arg.B2.a_0=(target_1=II_0.regIN[1]);
			while (true){
				assert(i1!=0);
				i1--;
				fillInstructionInformation(&II_1,ib,i1);
				if (doesRegListContain(II_1.regOUT,target_1)){
					if (II_1.id==I_MOV_){
						target_1=II_1.regIN[0];
					} else {
						if (II_1.id==I_LAD2 & target_1==II_1.regOUT[1]){
							insertInstructionAt(ib,i1+1,IS_BL1);
							i0--;
							didSucceedAtLeastOnce=true;
							buffer=ib->buffer;
							numberOfSlotsTaken=ib->numberOfSlotsTaken;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							break;
						} else if (II_1.id==I_BL1_ & !II_0.usesReg_D & isStackSizeKnown){
							// `!II_0.usesReg_D` is to check if the memory operation is not a byte instruction
							if (buffer[i1].arg.B2.a_1==0){
								// i1, II_1, target_1  are now going to be used for a different loop
								i1=i0;
								target_1=II_0.regIN[0];
								while (true){
									assert(i1!=0);
									i1--;
									fillInstructionInformation(&II_1,ib,i1);
									if (doesRegListContain(II_1.regOUT,target_1)){
										if (II_1.id==I_MOV_){
											target_1=II_1.regIN[0];
										} else {
											if (II_1.id==I_STPA | II_1.id==I_STPS){
												offsetValue=stackSize-buffer[i1].arg.BW.a_1;
												if (!((offsetValue&1) | (offsetValue>510U))){
													if      (II_0.id==I_MWWN) IS_temp.id=I_STWN, IS_temp.arg.B2.a_0=II_0.regIN[2];
													else if (II_0.id==I_MRWN) IS_temp.id=I_STRN, IS_temp.arg.B2.a_0=II_0.regOUT[0];
													else if (II_0.id==I_MWWV) IS_temp.id=I_STWV, IS_temp.arg.B2.a_0=II_0.regIN[2];
													else if (II_0.id==I_MRWV) IS_temp.id=I_STRV, IS_temp.arg.B2.a_0=II_0.regOUT[0];
													else assert(false);
													IS_temp.arg.B2.a_1=offsetValue/2U;
													buffer[i0]=IS_temp;
													didSucceedAtLeastOnce=true;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
												}
											}
											break;
										}
									}
								}
							}
						}
						break;
					}
				}
			}
		}
	}
	return didSucceedAtLeastOnce;
}


// this pass is mainly called by attemptDupeConstOpt() to avoid conflicts
void applyMovFromSmallConstOpt(InstructionBuffer* ib){
	uint32_t i=ib->numberOfSlotsTaken;
	uint16_t value;
	InstructionSingle IS;
	IS.id=I_BL1_;
	InstructionSingle* buffer=ib->buffer;
	InstructionSingle* IS_ptr;
	while (i--!=0){
		if (buffer[i].id==I_MOV_){
			IS_ptr=buffer+i;
			if (getValueInRegisterIfTraceableToRawConstants(ib,i-1,IS_ptr->arg.B2.a_1,&value)){
				if ((value&0xFF00)==0){
					IS.arg.B2.a_0=IS_ptr->arg.B2.a_0;
					IS.arg.B2.a_1=(uint8_t)value;
					*IS_ptr=IS;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
				}
			}
		}
	}
}



bool attemptDupeConstOpt(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* buffer=ib->buffer;
	enum InstructionTypeID id_0;
	for (uint32_t i0=0;i0<numberOfSlotsTaken;i0++){
		id_0=buffer[i0].id;
		if (id_0==I_BL1_ | id_0==I_RL1_ | id_0==I_STPA | id_0==I_STPS){
			uint8_t thisReg;
			if (id_0==I_BL1_) thisReg=buffer[i0].arg.B2.a_0;
			else              thisReg=buffer[i0].arg.BW.a_0;
			if (thisReg==13 | thisReg==14) attemptRegRename(ib,i0,thisReg,16); // don't care if it succeeds, we just want to try
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		}
	}
	InstructionInformation II_0;
	InstructionSingle* IS_ptr0;
	InstructionSingle* IS_ptr1;
	InstructionSingle IS;
	struct {
		uint16_t constValue;
		uint8_t movCount;
		bool isValueConst;
		bool isRawConst;
	} regTrack[17]={0};
	for (uint32_t i0=0;i0<numberOfSlotsTaken;i0++){
		fillInstructionInformation(&II_0,ib,i0);
		if (II_0.isSymbolicInternal) continue;
		if (II_0.doesDestroyReg | II_0.stopLinearRegTrace){
			regTrack[ 2].isValueConst=0;
			regTrack[ 3].isValueConst=0;
			regTrack[ 4].isValueConst=0;
			regTrack[ 5].isValueConst=0;
			regTrack[ 6].isValueConst=0;
			regTrack[ 7].isValueConst=0;
			regTrack[ 8].isValueConst=0;
			regTrack[ 9].isValueConst=0;
			regTrack[10].isValueConst=0;
			regTrack[11].isValueConst=0;
			regTrack[12].isValueConst=0;
			regTrack[13].isValueConst=0;
			regTrack[14].isValueConst=0;
		} else if (II_0.id==I_BL1_ | II_0.id==I_RL1_ | II_0.id==I_STPA | II_0.id==I_STPS){
			uint16_t thisConstValue;
			if (II_0.id==I_BL1_) thisConstValue=buffer[i0].arg.B2.a_1;
			else                 thisConstValue=buffer[i0].arg.BW.a_1;
			bool isThisRawConst=II_0.id==I_BL1_ | II_0.id==I_RL1_;
			uint8_t thisReg=II_0.regOUT[0];
			bool didFind=false;
			uint8_t minMovCount=8;
			uint8_t regWithMinMovCount=16;
			for (uint8_t r=2;r<15;r++){
				if (regTrack[r].isRawConst==isThisRawConst & 
					regTrack[r].constValue==thisConstValue &
					regTrack[r].isValueConst){
					
					
					didFind=true;
					if (regTrack[r].movCount<minMovCount){
						minMovCount=regTrack[r].movCount;
						regWithMinMovCount=r;
					}
				}
			}
			regTrack[thisReg].isRawConst=isThisRawConst;
			regTrack[thisReg].isValueConst=1;
			regTrack[thisReg].constValue=thisConstValue;
			regTrack[thisReg].movCount=0;
			if (didFind & regWithMinMovCount!=16){
				
				regTrack[thisReg].movCount=minMovCount+1;
				didSucceedAtLeastOnce=true;
				buffer[i0].id=I_MOV_;
				buffer[i0].arg.B2.a_0=thisReg;
				buffer[i0].arg.B2.a_1=regWithMinMovCount;
				i0--;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
			}
		} else if (II_0.id==I_MOV_){
			regTrack[II_0.regOUT[0]]=regTrack[II_0.regIN[0]];
			if (regTrack[II_0.regOUT[0]].movCount<4){
				regTrack[II_0.regOUT[0]].movCount++;
			}
		} else {
			regTrack[II_0.regOUT[0]].isValueConst=0;
			regTrack[II_0.regOUT[1]].isValueConst=0;
			regTrack[II_0.regOUT[2]].isValueConst=0;
			regTrack[II_0.regOUT[3]].isValueConst=0;
		}
	}
	if (didSucceedAtLeastOnce){
		bool didMovSucceed=attemptMovPushPopOpt(ib);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		applyMovFromSmallConstOpt(ib);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		return didMovSucceed;
	}
	return false;
}


// no need for change detection, it always does everything it can and does not effect other passes
void applySTPAtoSTPSopt(InstructionBuffer* ib){
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	InstructionSingle* IS_ptr0=ib->buffer;
	InstructionSingle* IS_ptr1;
	InstructionSingle IS;
	bool isStackSizeKnown=false;
	uint16_t stackSize=0;
	for (uint32_t i=0;i<numberOfSlotsTaken;i++){
		switch ((IS_ptr1=IS_ptr0++)->id){
			case I_STPA:;
			// this out of bounds check is only valid if the function is not a varadic function
			/*
			if (stackSize<IS_ptr1->arg.BW.a_1 && !compileSettings.hasGivenOutOfBoundsStackAccessWarning){
				compileSettings.hasGivenOutOfBoundsStackAccessWarning=true;
err_00__0("Optimizer detected out of bounds memory access on stack\n  no source location avalible\n  future warnings of this type will be suppressed");
			}
			*/
			if (isStackSizeKnown & ((stackSize-IS_ptr1->arg.BW.a_1)&0xFF00)==0) IS_ptr1->id=I_STPS;
			break;
			case I_FCST:
			isStackSizeKnown=(IS_ptr1->arg.BWD.a_0!=0);
			stackSize=IS_ptr1->arg.BWD.a_1;
			break;
			case I_FCEN:
			isStackSizeKnown=false;
			default:;
		}
	}
}


bool attemptSTPoffsetOpt(InstructionBuffer* ib){
	InstructionInformation II_0;
	InstructionSingle IS_stp;
	InstructionSingle* buffer=ib->buffer;
	IS_stp.id=I_STPA;
	bool didSucceedAtLeastOnce=false;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i0=numberOfSlotsTaken;
	while (i0--!=0){
		if (buffer[i0].id==I_ADDN){
			fillInstructionInformation(&II_0,ib,i0);
			uint16_t valTraceRaw0;
			bool traceRaw0;
			uint16_t valTraceRaw1;
			bool traceRaw1;
			
			traceRaw0=getValueInRegisterIfTraceableToRawConstants(ib,i0-1,II_0.regIN[0],&valTraceRaw0);
			traceRaw1=getValueInRegisterIfTraceableToRawConstants(ib,i0-1,II_0.regIN[1],&valTraceRaw1);
			if (!traceRaw0 & !traceRaw1) continue;
			if (traceRaw0 ^ traceRaw1){
				uint16_t valTraceStp;
				uint16_t valTraceRaw=traceRaw1?valTraceRaw1:valTraceRaw0;
				bool traceStp = getValueInSTPifTracableToSTP(ib,i0-1,(traceRaw1?II_0.regIN[0]:II_0.regIN[1]),&valTraceStp);
				if (traceStp){
					if (valTraceStp>=valTraceRaw){
						IS_stp.arg.BW.a_1=valTraceStp-valTraceRaw;
						IS_stp.arg.BW.a_0=II_0.regOUT[0];
						didSucceedAtLeastOnce=true;
						buffer[i0]=IS_stp;
						
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
					} else if (!compileSettings.hasGivenOutOfBoundsStackAccessWarning){
						compileSettings.hasGivenOutOfBoundsStackAccessWarning=true;
err_00__0("Optimizer detected out of bounds memory access on stack\n  no source location avalible\n  future warnings of this type will be suppressed");
					}
				}
			} else if (traceRaw0 & traceRaw1){
				// could do some of const opt's job here, it's easy...
			}
		}
	}
	return didSucceedAtLeastOnce;
}


bool attemptRepeatedConstantOpt(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	InstructionInformation II_0;
	InstructionInformation II_1;
	InstructionSingle* buffer=ib->buffer;
	struct RepeatedConstInfo repeatedConstInfo;
	repeatedConstInfo.ib=ib;
	repeatedConstInfo.i0=ib->numberOfSlotsTaken;
	while (repeatedConstInfo.i0--!=0){
		InstructionSingle writeIS;
		repeatedConstInfo.id=buffer[repeatedConstInfo.i0].id;
		switch (repeatedConstInfo.id){
			case I_AND_:
			case I_XOR_:
			case I_OR__:
			case I_MULS:
			case I_ADDN:
			if (attemptRepeatedConstantOpt_sub1(&repeatedConstInfo)){
				//printInstructionBufferWithMessageAndNumber(ib,"Before",repeatedConstInfo.i0);
				uint16_t cv;
				switch (repeatedConstInfo.id){
					case I_AND_:
					cv=repeatedConstInfo.cvBuff[0] & repeatedConstInfo.cvBuff[1];
					break;
					case I_XOR_:
					cv=repeatedConstInfo.cvBuff[0] ^ repeatedConstInfo.cvBuff[1];
					break;
					case I_OR__:
					cv=repeatedConstInfo.cvBuff[0] | repeatedConstInfo.cvBuff[1];
					break;
					case I_MULS:
					cv=repeatedConstInfo.cvBuff[0] * repeatedConstInfo.cvBuff[1];
					break;
					case I_ADDN:
					cv=repeatedConstInfo.cvBuff[0] + repeatedConstInfo.cvBuff[1];
					break;
					default:;
				}
				
				uint8_t r1=repeatedConstInfo.II_0.regIN[repeatedConstInfo.types[0]];
				uint8_t r0=repeatedConstInfo.II_0.regIN[repeatedConstInfo.types[0]^1];
				uint8_t r2=repeatedConstInfo.II_0.regOUT[0];
				uint8_t r3=r0!=r2?r0:r1;
				assert(r0!=r2 | r1!=r2); // this transform would be invalid if this fails, but I don't think that should fail
				if (repeatedConstInfo.id==I_MULS){
					writeIS.arg.B2.a_0=r2;
					writeIS.arg.B2.a_1=r3;
				} else {
					writeIS.arg.B3.a_0=r2;
					writeIS.arg.B3.a_1=r2;
					writeIS.arg.B3.a_2=r3;
				}
				writeIS.id=repeatedConstInfo.id;
				buffer[repeatedConstInfo.i0]=writeIS;
				
				writeIS.id=I_RL1_;
				writeIS.arg.BW.a_0=r2;
				writeIS.arg.BW.a_1=cv;
				insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				
				writeIS.id=I_POP1;
				writeIS.arg.B1.a_0=r3;
				insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				
				writeIS.id=I_PU1_;
				writeIS.arg.B1.a_0=repeatedConstInfo.II_1.regIN[repeatedConstInfo.types[1]];
				insertInstructionAt(ib,repeatedConstInfo.indexBuff[0],writeIS);
				
				//printInstructionBufferWithMessageAndNumber(ib,"After",repeatedConstInfo.i0);
				//exit(0);
				buffer=ib->buffer;
				didSucceedAtLeastOnce=true;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
			}
			break;
			case I_LAD0:
			case I_LAD1:
			// I_LAD2 is not here on purpose, it will need to use a different detection method. Also, I_LAD2 would only trace back to I_ADDN, not any of these: I_LAD0,I_LAD1,I_LAD2
			if (attemptRepeatedConstantOpt_sub3(&repeatedConstInfo)){
				//printInstructionBufferWithMessageAndNumber(ib,"Before",repeatedConstInfo.i0);
				uint32_t cvLarge=(repeatedConstInfo.cvBuff[0] | ((uint32_t)repeatedConstInfo.cvBuff[1]<<16)) + (repeatedConstInfo.cvBuff[2] | ((uint32_t)repeatedConstInfo.cvBuff[3]<<16));
				bool does0have2ConstInputRegisters=repeatedConstInfo.normalizedReg0[3]!=16;
				bool does1have2NonconstInputRegisters=repeatedConstInfo.normalizedReg1[3]!=16;
				
				if (!does0have2ConstInputRegisters & (cvLarge&0xFFFF0000LU)!=0){
					uint8_t extraReg=16;
					for (uint8_t r=2;r<15;r++){
						if (!doesRegListContain(repeatedConstInfo.II_0.regIN,r)){
							// it might not matter for regOUT
							if (!doesRegListContain(repeatedConstInfo.II_0.regOUT,r)){
								if (!isValueInRegUsedAfterTarget(ib,repeatedConstInfo.i0,r,NULL)){
									extraReg=r;
									break;
								}
							}
						}
					}
					if (extraReg==16){
						break; // break the switch, causing the for loop to continue. This optimization cannot be performed in that case, as there are no temporary registers to use and one is needed
					}
					writeIS.id=I_LAD0;
					writeIS.arg.B6.a_0=repeatedConstInfo.II_0.regOUT[0];
					writeIS.arg.B6.a_1=repeatedConstInfo.II_0.regOUT[1];
					writeIS.arg.B6.a_2=repeatedConstInfo.II_0.regIN[0];
					writeIS.arg.B6.a_3=repeatedConstInfo.II_0.regIN[1];
					writeIS.arg.B6.a_4=repeatedConstInfo.II_0.regIN[2];
					writeIS.arg.B6.a_5=extraReg;
					buffer[repeatedConstInfo.i0]=writeIS;
					
					writeIS.id=I_RL2_;
					writeIS.arg.BBD.a_0=repeatedConstInfo.normalizedReg0[2];
					writeIS.arg.BBD.a_1=extraReg;
					writeIS.arg.BBD.a_2=cvLarge;
					insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				} else if (does0have2ConstInputRegisters){
					writeIS.id=I_RL2_;
					writeIS.arg.BBD.a_0=repeatedConstInfo.normalizedReg0[2];
					writeIS.arg.BBD.a_1=repeatedConstInfo.normalizedReg0[3];
					writeIS.arg.BBD.a_2=cvLarge;
					insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				} else {
					assert((cvLarge&0xFFFF0000LU)==0);
					writeIS.id=I_RL1_;
					writeIS.arg.BW.a_0=repeatedConstInfo.normalizedReg0[2];
					writeIS.arg.BW.a_1=(uint16_t)cvLarge;
					insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				}
				writeIS.id=I_POP1;
				writeIS.arg.B1.a_0=repeatedConstInfo.normalizedReg0[0];
				insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
				if (does1have2NonconstInputRegisters){
					writeIS.id=I_POP1;
					writeIS.arg.B1.a_0=repeatedConstInfo.normalizedReg0[1];
					insertInstructionAt(ib,repeatedConstInfo.i0,writeIS);
					writeIS.id=I_PU1_;
					writeIS.arg.B1.a_0=repeatedConstInfo.normalizedReg1[3];
					insertInstructionAt(ib,repeatedConstInfo.indexBuff[0],writeIS);
				}
				writeIS.id=I_PU1_;
				writeIS.arg.B1.a_0=repeatedConstInfo.normalizedReg1[2];
				insertInstructionAt(ib,repeatedConstInfo.indexBuff[0],writeIS);
				
				//printInstructionBufferWithMessageAndNumber(ib,"After",repeatedConstInfo.i0);
				//exit(0);
				didSucceedAtLeastOnce=true;
				buffer=ib->buffer;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
			}
			default:;
		}
	}
	return didSucceedAtLeastOnce;
}



// this cannot be checked for success, it will often cause infinite loops if you tried
void applyTightOpt(InstructionBuffer* ib){
	uint32_t i=ib->numberOfSlotsTaken;
	uint32_t upperReorderBoundry;
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	while (i--!=0){
		InstructionInformation II;
		fillInstructionInformation(&II,ib,i);
		if (!II.isSymbolicInternal & II.id!=I_SYRB & II.id!=I_SYRW & II.id!=I_SYRD & II.id!=I_SYRQ){
			upperReorderBoundry=findUpperReorderBoundry(ib,i);
			if (upperReorderBoundry!=i) applyReorder(ib,i,upperReorderBoundry);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		}
	}
#ifdef OPT_DEBUG_GENERAL_ACTIVE
	printf(".");
#endif
	// SYRB,SYRW,SYRD,I_SYRQ have priority, so they get a seperate pass after the other reorder pass
	i=ib->numberOfSlotsTaken;
	while (i--!=0){
		enum InstructionTypeID id=ib->buffer[i].id;
		if (id==I_SYRB | id==I_SYRW | id==I_SYRD | id==I_SYRQ){
			upperReorderBoundry=findUpperReorderBoundry(ib,i);
			if (upperReorderBoundry!=i) applyReorder(ib,i,upperReorderBoundry);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		}
	}
}




bool attemptStackMemoryAccessReduction(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	bool isStackSizeKnown=false;
	uint16_t stackSize;
	struct StackDataForMemoryAccess sdfma_0;
	struct StackDataForMemoryAccess sdfma_1;
	uint16_t trueOffset_0;
	uint16_t trueOffset_1;
	uint32_t numberOfSlotsTaken=ib->numberOfSlotsTaken;
	uint32_t i0;
	uint32_t i1;
	InstructionInformation II_0;
	InstructionInformation II_1;
	InstructionSingle IS;
	uint8_t regList[2];
	for (i0=0;i0<numberOfSlotsTaken;i0++){
		fillInstructionInformation(&II_0,ib,i0);
		if (II_0.id==I_FCST | II_0.id==I_FCEN){
			isStackSizeKnown=false;
			if (II_0.id==I_FCST){
				IS=ib->buffer[i0];
				isStackSizeKnown=(IS.arg.BWD.a_0!=0);
				stackSize=IS.arg.BWD.a_1;
			}
		} else if (II_0.isMemoryAccess & !II_0.isMemoryAccessVolatile){
			if (!isStackSizeKnown) continue;
			if (attemptToGetStackInfoForMemoryAccess(ib,i0,&sdfma_0)){
				if (!sdfma_0.isDiscrete) trueOffset_0=stackSize-sdfma_0.offset*2U;
				else trueOffset_0=sdfma_0.offset;
				trueOffset_0=stackSize-trueOffset_0;
				i1=i0;
				bool isRegMethodValid=true;
				bool isStackMethodValid=true;
				while (i1--!=0){
					fillInstructionInformation(&II_1,ib,i1);
					if (II_1.isMemoryAccess){
						if (attemptToGetStackInfoForMemoryAccess(ib,i1,&sdfma_1)){
							if (!sdfma_1.isDiscrete) trueOffset_1=stackSize-sdfma_1.offset*2U;
							else trueOffset_1=sdfma_1.offset;
							trueOffset_1=stackSize-trueOffset_1;
							//printf("%04X,%04X,(----)[%04X,%04X]\n",i1,i0,trueOffset_0,trueOffset_1);
							bool isBaseEqual=(trueOffset_0&0xFFFE)==(trueOffset_1&0xFFFE);
							bool isExactEqual=trueOffset_0==trueOffset_1;
							if (isExactEqual | isBaseEqual){
if (II_1.isMemoryAccessVolatile) break; // Let's just be totally safe with volatile, at least for now
uint16_t state= 
	sdfma_0.isWord*1U |
	sdfma_1.isWord*2U |
	sdfma_0.isRead*4U |
	sdfma_1.isRead*8U |
	isExactEqual*16U ;

switch (state){
// !isExactEqual
	
	case 0x00:
	case 0x04:
	case 0x08:
	case 0x0C:
	goto SwitchContinue; // no relation in memory, these are seperate bytes, so continue searching
	
// !sdfma_0.isRead & !sdfma_1.isRead
	case 0x01:
	//printf("Method empty at %04X,%04X,(0x01)\n",i1,i0);
	break;
	case 0x02:
	//printf("Method empty at %04X,%04X,(0x02)\n",i1,i0);
	break;
	case 0x03:
	//printf("Method empty at %04X,%04X,(0x03)\n",i1,i0);
	break;
	
//  sdfma_0.isRead & !sdfma_1.isRead
	case 0x05:
	//printf("Method empty at %04X,%04X,(0x05)\n",i1,i0);
	break;
	case 0x06:
	//printf("%04X,%04X,(0x06)\n",i1,i0);
	if (isRegMethodValid){
		if (findUnusedRegsInRange(ib,i1,i0,regList,1)){
			didSucceedAtLeastOnce=true;
			//printInstructionBufferWithMessageAndNumber(ib,"Before:",i1);
			ib->buffer[i0].id=I_MOV_;
			ib->buffer[i0].arg.B2.a_0=II_0.regOUT[0];
			ib->buffer[i0].arg.B2.a_1=regList[0];
			IS.id=I_BSWP;
			IS.arg.B2.a_0=II_0.regOUT[0];
			IS.arg.B2.a_1=II_0.regOUT[0];
			insertInstructionAt(ib,i0+1,IS);
			IS.id=I_BL1_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=0xFF;
			insertInstructionAt(ib,i0+2,IS);
			IS.id=I_AND_;
			IS.arg.B3.a_0=II_0.regOUT[0];
			IS.arg.B3.a_1=II_0.regOUT[0];
			IS.arg.B3.a_2=regList[0];
			insertInstructionAt(ib,i0+3,IS);
			IS.id=I_MOV_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=sdfma_1.isDiscrete?II_1.regIN[2]:II_1.regIN[0];
			insertInstructionAt(ib,i1+1,IS);
			//printInstructionBufferWithMessageAndNumber(ib,"After:",i0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		} else {
			//printf("Couldn't find unused reg! (0x06)\n");
		}
	} else {
		//printf("Reg method invalid! (0x06)\n");
	}
	break;
	case 0x07:
	//printf("Method empty at %04X,%04X,(0x07)\n",i1,i0);
	break;
	
//  sdfma_0.isRead &  sdfma_1.isRead
	case 0x0D:
	//printf("Method empty at %04X,%04X,(0x0D)\n",i1,i0);
	break;
	case 0x0E:
	//printf("Method empty at %04X,%04X,(0x0E)\n",i1,i0);
	break;
	case 0x0F:
	//printf("Method empty at %04X,%04X,(0x0F)\n",i1,i0);
	break;

	
	

//  isExactEqual
// !sdfma_0.isRead & !sdfma_1.isRead
	case 0x10:
	//printf("%04X,%04X,(0x10)\n",i1,i0);
	//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
	ib->buffer[i1].id=I_NOP_;
	//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	break;
	case 0x11:
	//printf("%04X,%04X,(0x11)\n",i1,i0);
	//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
	ib->buffer[i1].id=I_NOP_;
	//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	break;
	case 0x12:
	//printf("Method empty at %04X,%04X,(0x12)\n",i1,i0);
	// this is the case of writting a word then a byte. 
	// it isn't solved by simply removing of a write,
	// because part of the word written is still in memory
	break;
	case 0x13:
	didSucceedAtLeastOnce=true;
	//printf("%04X,%04X,(0x13)\n",i1,i0);
	//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
	ib->buffer[i1].id=I_NOP_;
	//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	break;
	
//  sdfma_0.isRead & !sdfma_1.isRead
	case 0x14:
	//printf("Method empty at %04X,%04X,(0x14)\n",i1,i0);
	break;
	case 0x15:
	//printf("Method empty at %04X,%04X,(0x15)\n",i1,i0);
	break;
	case 0x16:
	//printf("%04X,%04X,(0x16)\n",i1,i0);
	if (isRegMethodValid){
		if (findUnusedRegsInRange(ib,i1,i0,regList,1)){
			didSucceedAtLeastOnce=true;
			//printInstructionBufferWithMessageAndNumber(ib,"Before:",i1);
			ib->buffer[i0].id=I_MOV_;
			ib->buffer[i0].arg.B2.a_0=II_0.regOUT[0];
			ib->buffer[i0].arg.B2.a_1=regList[0];
			IS.id=I_BL1_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=0xFF;
			insertInstructionAt(ib,i0+1,IS);
			IS.id=I_AND_;
			IS.arg.B3.a_0=II_0.regOUT[0];
			IS.arg.B3.a_1=II_0.regOUT[0];
			IS.arg.B3.a_2=regList[0];
			insertInstructionAt(ib,i0+2,IS);
			IS.id=I_MOV_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=sdfma_1.isDiscrete?II_1.regIN[2]:II_1.regIN[0];
			insertInstructionAt(ib,i1+1,IS);
			//printInstructionBufferWithMessageAndNumber(ib,"After:",i0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		} else {
			//printf("Couldn't find unused reg! (0x16)\n");
		}
	} else {
		//printf("Reg method invalid! (0x16)\n");
	}
	break;
	case 0x17:
	//printf("%04X,%04X,(0x17)\n",i1,i0);
	if (isRegMethodValid){
		if (findUnusedRegsInRange(ib,i1,i0,regList,1)){
			didSucceedAtLeastOnce=true;
			//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
			ib->buffer[i0].id=I_MOV_;
			ib->buffer[i0].arg.B2.a_0=II_0.regOUT[0];
			ib->buffer[i0].arg.B2.a_1=regList[0];
			IS.id=I_MOV_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=sdfma_1.isDiscrete?II_1.regIN[2]:II_1.regIN[0];
			insertInstructionAt(ib,i1+1,IS);
			//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		} else {
			//printf("Couldn't find unused reg! (0x17)\n");
		}
	} else {
		//printf("Reg method invalid! (0x17)\n");
	}
	break;
//  sdfma_0.isRead &  sdfma_1.isRead
	case 0x1C:
	//printf("%04X,%04X,(0x1C)\n",i1,i0);
	if (isRegMethodValid){
		if (findUnusedRegsInRange(ib,i1,i0,regList,1)){
			didSucceedAtLeastOnce=true;
			//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
			ib->buffer[i0].id=I_MOV_;
			ib->buffer[i0].arg.B2.a_0=II_0.regOUT[0];
			ib->buffer[i0].arg.B2.a_1=regList[0];
			IS.id=I_MOV_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=II_1.regOUT[0];
			insertInstructionAt(ib,i1+1,IS);
			//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		} else {
			//printf("Couldn't find unused reg! (0x1C)\n");
		}
	} else {
		//printf("Reg method invalid! (0x1C)\n");
	}
	break;
	case 0x1D:
	//printf("Method empty at %04X,%04X,(0x1D)\n",i1,i0);
	break;
	case 0x1E:
	//printf("Method empty at %04X,%04X,(0x1E)\n",i1,i0);
	break;
	case 0x1F:
	//printf("%04X,%04X,(0x1F)\n",i1,i0);
	if (isRegMethodValid){
		if (findUnusedRegsInRange(ib,i1,i0,regList,1)){
			didSucceedAtLeastOnce=true;
			//printInstructionBufferWithMessageAndNumber(ib,"Before:",0);
			ib->buffer[i0].id=I_MOV_;
			ib->buffer[i0].arg.B2.a_0=II_0.regOUT[0];
			ib->buffer[i0].arg.B2.a_1=regList[0];
			IS.id=I_MOV_;
			IS.arg.B2.a_0=regList[0];
			IS.arg.B2.a_1=II_1.regOUT[0];
			insertInstructionAt(ib,i1+1,IS);
			//printInstructionBufferWithMessageAndNumber(ib,"After:",0);
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
		} else {
			//printf("Couldn't find unused reg! (0x1F)\n");
		}
	} else {
		//printf("Reg method invalid! (0x1F)\n");
	}
	break;

	
// !sdfma_0.isRead & sdfma_1.isRead   (it's always useless)
	case 0x09:case 0x0A:case 0x0B:case 0x18:case 0x19:case 0x1A:case 0x1B:break;
	default:assert(false);
}
break;
// assumptions about current state may be invalid, so stop the current search 
// (as well as there are cases where the current assumptions of the search are known to be invalid)
							}
						} else if (
!sdfma_1.isRead & (!sdfma_1.isOnStackKnown | sdfma_1.isOnStack)){
	//printInstructionBufferWithMessageAndNumber(ib,"This memory access DOES stop search",i1);
	break; // then the location of a memory write couldn't be verified to not be on the stack, which must stop this search
} else {
	//printInstructionBufferWithMessageAndNumber(ib,"This memory access DOES NOT stop search",i1);
}
						
					} else if (II_1.id==I_FCST | II_1.id==I_CJMP | II_1.id==I_CALL | II_1.stopLinearRegTrace) break; 
					// CJMP stops this search. memory is not invalidated when CJMP jumps
					// CALL stops this search. memory is unknown when crossing a CALL
					isRegMethodValid&=!II_1.doesDestroyReg;
					isStackMethodValid&=!(II_1.doesMoveStack | II_1.doesRelyOnStack);
					SwitchContinue:;
				}
			}
		}
	}
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
	return didSucceedAtLeastOnce;
}













bool attemptSinglePeepHoleTransform(ValueTraceEntriesApplicableRepeatedParams* vtearp){
	uint8_t unusedReg[6]={16,16,16,16,16,16};
	if (vtearp->template->nUnusedReg!=0){
		if (!findUnusedRegsAfterTarget(vtearp->ib,vtearp->startIndex,unusedReg,vtearp->template->nUnusedReg)){
			return false;
		}
	}
	//printInstructionBufferWithMessageAndNumber(vtearp->ib,"Peephole (1/4)",vtearp->startIndex);
#ifdef OPT_DEBUG_SANITY
sanityCheck(vtearp->ib);
#endif
	DualBoundPeephole dbph;
	vtearp->walkSource=0;vtearp->walkTemplate=0;vtearp->pushPopWalk=0;
	applyInitialPeepHoleTransform(vtearp);
	//printInstructionBufferWithMessageAndNumber(vtearp->ib,"Peephole (2/4)",vtearp->startIndex);
	uint8_t sourceLength=vtearp->walkSource;
	vtearp->walkSource=0;vtearp->walkTemplate=0;
	dbph=getDualBoundPeephole(vtearp);
	vtearp->walkSource=0;vtearp->walkTemplate=0;
	//printf("=%04X,%04X=",dbph.lowerBoundForUnusedOut,dbph.upperBoundForUnusedOut);
	switch (vtearp->template->postApplyIndex){
		case 0:break;
		case 1:dbph.upperBoundForUnusedOut+=applyPostPeepHole_1(vtearp,unusedReg);break;
		case 2:dbph.upperBoundForUnusedOut+=applyPostPeepHole_2(vtearp,unusedReg);break;
		case 3:dbph.upperBoundForUnusedOut+=applyPostPeepHole_3(vtearp,unusedReg);break;
		case 4:dbph.upperBoundForUnusedOut+=applyPostPeepHole_4(vtearp,unusedReg);break;
		case 5:dbph.upperBoundForUnusedOut+=applyPostPeepHole_5(vtearp,unusedReg);break;
		case 6:dbph.upperBoundForUnusedOut+=applyPostPeepHole_6(vtearp,unusedReg);break;
		default:assert(false);
	}
	//printInstructionBufferWithMessageAndNumber(vtearp->ib,"Peephole (3/4)",vtearp->startIndex);
#ifdef OPT_DEBUG_SANITY
sanityCheck(vtearp->ib);
#endif
	uint32_t i=dbph.upperBoundForUnusedOut;
	InstructionInformation II;
	do {
		fillInstructionInformation(&II,vtearp->ib,i);
		if (!II.doesMoveStack & !II.isMemoryAccess & !II.isSymbolicInternal & II.id!=I_SYRB & II.id!=I_SYRW & II.id!=I_SYRD & II.id!=I_SYRQ){
			bool isNoneUsed=true;
			uint8_t r;
			uint8_t ri=0;
			while ((r=II.regOUT[ri++])!=16){
				isNoneUsed=isNoneUsed && !isValueInRegUsedAfterTarget(vtearp->ib,i,r,NULL);
			}
			if (ri!=1 & isNoneUsed){
				vtearp->ib->buffer[i].id=I_NOP_;
			}
		}
	} while (i--!=dbph.lowerBoundForUnusedOut);
	//printInstructionBufferWithMessageAndNumber(vtearp->ib,"Peephole (4/4)",vtearp->startIndex);
#ifdef OPT_DEBUG_SANITY
sanityCheck(vtearp->ib);
#endif
	return true;
}




bool attemptPeepHoleOpt(InstructionBuffer* ib){
	bool didSucceedAtLeastOnce=false;
	ValueTraceEntriesApplicableRepeatedParams vtearp;
	vtearp.ib=ib;
	InstructionInformation II;
	uint32_t i=ib->numberOfSlotsTaken;
	while (i--!=0){
		fillInstructionInformation(&II,ib,i);
		if (II.isAllowedInVTE & II.regIN[0]!=16){
			// `II.regIN[0]!=16` is for if the instruction has input. If it doesn't, it is useless to be the root (no templates will match).
			uint8_t r;
			uint8_t ri=0;
			while ((r=II.regOUT[ri++])!=16){
				uint32_t usageIndex;
				if (!isValueInRegUsedAfterTarget(ib,i,r,&usageIndex)){
					// basically, if you don't use an output then don't try to make an improvement to the calculation of that output
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("--peephole: skipping unused output\n");
#endif
					continue;
				}
				bool isThisOutTraceableToBool=isTraceableToBool(ib,i,r);
				vtearp.startIndex=i;
				vtearp.startReg=r;
				vtearp.walkSource=0;
				vtearp.walkTemplate=0;
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("--------------------------+ [%d]\n",isThisOutTraceableToBool);
prettyPrintPeepholeTreeStartDepth=PH_VTEP_MAX_DEPTH;
#endif
				if (generateValueTraceEntries(&vtearp,PH_VTEP_MAX_DEPTH,r,false,i,usageIndex)){
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("peephole generation is being restarted with safe depth instead\n");
prettyPrintPeepholeTreeStartDepth=PH_VTEP_SAFE_DEPTH;
#endif
					vtearp.walkSource=0;
					vtearp.walkTemplate=0;
					if (generateValueTraceEntries(&vtearp,PH_VTEP_SAFE_DEPTH,r,false,i,usageIndex)){
						printf("Internal Error: peephole source overflow when using safe depth\n");
						exit(1);
					}
				}
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("--------------------------=\n");
#endif
				for (uint16_t ph_template_i=0;ph_template_i<PH_VTEP_COUNT;ph_template_i++){
					if ((VTEP_arr+ph_template_i)->vte==NULL) continue;// temporary while peephole templates and system are being developed
					
					vtearp.template=VTEP_arr+ph_template_i;
					vtearp.walkSource=0;
					vtearp.walkTemplate=0;
					vtearp.pushPopWalk=0;
					for (uint8_t symCVi=0;symCVi<PH_VTEP_MAX_SYM_CONST;symCVi++){
						vtearp.cvTable.isSet[symCVi]=0;
					}
					for (uint8_t idenI=0;idenI<PH_VTEP_MAX_IDEN;idenI++){
						vtearp.idenTable.isSet[idenI]=0;
					}
					if (
					!(vtearp.template->requireBoolVerify & !isThisOutTraceableToBool) && 
					isValueTraceEntriesApplicable(&vtearp)){
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("peephole opt now attempting to apply `%02X`\n",ph_template_i);
#endif
						vtearp.walkSource=0;
						vtearp.walkTemplate=0;
						if (attemptSinglePeepHoleTransform(&vtearp)){
							didSucceedAtLeastOnce=true;
							goto MainLoop;
						} else {
							printf("Peephole opt couldn't find unused reg(s) when it needed one!\n");
						}
					}
				}
			}
		} else if (II.id==I_CJMP){
			uint32_t regSource;
			struct LocationAndIfRequireDepl ladpl;
			ladpl.target=i-1;
			ladpl.reg=II.regIN[2];
			
			CJMP_MOV_look_through:
			regSource=findRegValueSourceSignalCross(ib,&ladpl);
			fillInstructionInformation(&II,ib,regSource);
			if (II.id==I_MOV_){
				ladpl.target=regSource-1;
				ladpl.reg=II.regIN[0];
				goto CJMP_MOV_look_through;
			}
			if (II.id==I_SSUB & ladpl.reg==II.regOUT[0]){
				uint16_t val0;
				uint16_t val1;
				if (getValueInRegisterIfTraceableToRawConstants(ib,regSource-1,II.regIN[0],&val0)){
					if (val0==0){
						if (getValueInRegisterIfTraceableToRawConstants(ib,regSource-1,II.regIN[2],&val1)){
							if (val1==0){
								didSucceedAtLeastOnce=true;
								InstructionSingle IS;
								IS.id=I_POP1;
								IS.arg.B1.a_0=ladpl.reg;
								insertInstructionAt(ib,regSource+1,IS);
								IS.id=I_PU1_;
								IS.arg.B1.a_0=II.regIN[1];
								insertInstructionAt(ib,regSource,IS);
								++i;
#ifdef OPT_PEEPHOLE_PRINT_TREE
printf("peephole opt applied `removal of SSUB from CJMP`\n");
#endif
#ifdef OPT_DEBUG_SANITY
sanityCheck(ib);
#endif
							}
						}
					}
				}
			}
		}
		MainLoop:;
	}
	return didSucceedAtLeastOnce;
}



#ifdef OPT_DEBUG_GENERAL_ACTIVE
#define DEBUG_STATUS(s0,s1,b) printf("%s",b?s0:s1),fflush(stdout);
#else
#define DEBUG_STATUS(s0,s1,b) ((void)0)
#endif


// expandPushPop() should be run on the input before any of these functions is run
void attemptAllActiveOptPhase1(InstructionBuffer* ib){
	uint16_t i=0;
	bool v0=1;
	bool v1=1;
	while (1){
		DEBUG_STATUS("mpp->","",1);
		v0 = attemptMovPushPopOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v0);
		if (!(v0|v1)) break;
		DEBUG_STATUS("con->","",1);
		v1 = attemptConstOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v0);
		if (!(v0|v1)) break;
		DEBUG_STATUS("tgh->","",1);
		applyTightOpt(ib);
		DEBUG_STATUS("[DONE]\n","",1);
		if (i++>1000){
			printInstructionBufferWithMessageAndNumber(ib,"attemptAllActiveOptPhase1() took too long",0);
			exit(0);
		}
	}
	applyMovFromSmallConstOpt(ib);
}

void attemptAllActiveOptPhase2(InstructionBuffer* ib){
	uint16_t i=0;
	bool v0=1;
	bool v1=1;
	bool v2=1;
	bool v3=1;
	bool v4=1;
	bool v5=1;
	bool v6=1;
	bool v7;
	DEBUG_STATUS("jmp->","",1);
	v7=attemptJmpOpt(ib);
	DEBUG_STATUS("[YES]\n","[NO]\n",v7);
	while (1){
		Start:;
		DEBUG_STATUS("spo->","",1);
		v0 = attemptSTPoffsetOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v0);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		DEBUG_STATUS("stm->","",1);
		v1 = attemptStackMemoryOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v1);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		DEBUG_STATUS("mar->","",1);
		v2 = attemptStackMemoryAccessReduction(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v2);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		DEBUG_STATUS("rco->","",1);
		v3 = attemptRepeatedConstantOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v3);
		Middle:;
		DEBUG_STATUS("con->","",1);
		v4 = attemptConstOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v4);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		DEBUG_STATUS("mpp->","",1);
		v5 = attemptMovPushPopOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v5);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		DEBUG_STATUS("dup->","",1);
		v6 = attemptDupeConstOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v6);
		DEBUG_STATUS("tgh->","",1);
		applyTightOpt(ib);
		DEBUG_STATUS("[DONE]\n","",1);
		if (!(v0|v1|v2|v3|v4|v5|v6)) break;
		if (i++>1000){
			printInstructionBufferWithMessageAndNumber(ib,"attemptAllActiveOptPhase2() took too long",0);
			exit(0);
		}
	}
	if (compileSettings.optLevel>2){
		DEBUG_STATUS("pep->","",1);
		v7 = attemptPeepHoleOpt(ib);
		DEBUG_STATUS("[YES]\n","[NO]\n",v7);
		if (v7) {v0=1;v1=1;v2=1;v3=1;v4=1;v5=1;v6=1;goto Middle;}
	}
	DEBUG_STATUS("jmp->","",1);
	v7=attemptJmpOpt(ib);
	DEBUG_STATUS("[YES]\n","[NO]\n",v7);
	if (v7) {v0=1;v1=1;v2=1;v3=1;v4=1;v5=1;v6=1;goto Start;}
	applySTPAtoSTPSopt(ib);
	applyMovFromSmallConstOpt(ib);
	removeNop(ib); // not all passes do this, because some passes do not add many. this just ensures that they are all gone at the end
}


#undef DEBUG_STATUS













