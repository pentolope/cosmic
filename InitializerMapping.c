

/*

running initializerMapRoot() will destroy any previous initializer mappings
	doesn't even need to have the type defined, it doesn't look at blockFrameArray

initializerImplementRoot() does need to have the type defined, 
	and it will walk the initializer map that is currently created 

There is constant and static expression walking and evaluation in this file too

*/


struct MemoryOrganizerForInitializer{
	uint32_t size;
	bool* isSet; // isSet[location] is if the byte is set at location
	InstructionBuffer ib;
};

void initMemoryOrganizerForInitializer(
		struct MemoryOrganizerForInitializer* mofi,
		uint32_t size){
	
	mofi->size=size;
	mofi->isSet=cosmic_calloc(size,sizeof(bool));
	initInstructionBuffer(&mofi->ib);
}

// finalizes the MemoryOrganizerForInitializer into a single InstructionBuffer without LOFF
// the MemoryOrganizerForInitializer is destroyed
InstructionBuffer finalizeMemoryOrganizerForInitializer(
		struct MemoryOrganizerForInitializer* mofi_ptr,
		uint32_t labelNumber,bool isConst){
	
	assert(labelNumber!=0);
	struct MemoryOrganizerForInitializer mofi=*mofi_ptr;
	cosmic_free(mofi.isSet);
	InstructionBuffer ib_destination;
	InstructionSingle IS_temp;
	initInstructionBuffer(&ib_destination);
	IS_temp.id=isConst?I_NSCB:I_NSNB;
	IS_temp.arg.D.a_0=mofi.size;
	addInstruction(&ib_destination,IS_temp);
	IS_temp.id=I_LABL;
	IS_temp.arg.D.a_0=labelNumber;
	addInstruction(&ib_destination,IS_temp);
	bool didFind;
	uint32_t lowestOffset;
	uint32_t locationOfLowestOffset;
	uint32_t currentOffset=0;
	InstructionSingle* IS_ptr_end=mofi.ib.buffer+mofi.ib.numberOfSlotsTaken;
	goto TransitionLoopStart;
	do {
		if (currentOffset!=lowestOffset){
			IS_temp.id=I_ZNXB;
			IS_temp.arg.D.a_0=lowestOffset-currentOffset;
			currentOffset=lowestOffset;
			addInstruction(&ib_destination,IS_temp);
		}
		mofi.ib.buffer[locationOfLowestOffset++].id=I_NOP_;
		assert(locationOfLowestOffset<mofi.ib.numberOfSlotsTaken);
		{
			enum InstructionTypeID data_type=mofi.ib.buffer[locationOfLowestOffset].id;
			if (data_type==I_BYTE | data_type==I_SYDB){
				currentOffset+=1;
			} else if (data_type==I_WORD | data_type==I_SYDW){
				currentOffset+=2;
			} else {
				assert(data_type==I_DWRD | data_type==I_SYDD);
				currentOffset+=4;
			}
		}
		do {
			InstructionSingle* IS_ptr=mofi.ib.buffer+locationOfLowestOffset;
			if (IS_ptr->id==I_NOP_){
				do {
					if (++locationOfLowestOffset<mofi.ib.numberOfSlotsTaken) goto TransitionLoopStart;
					IS_ptr=mofi.ib.buffer+locationOfLowestOffset;
				} while (IS_ptr->id!=I_NOP_);
			}
			if (IS_ptr->id==I_LOFF) goto TransitionLoopStart;
			addInstruction(&ib_destination,*IS_ptr);
			IS_ptr->id=I_NOP_;
		} while (++locationOfLowestOffset<mofi.ib.numberOfSlotsTaken);
		TransitionLoopStart:
		didFind=false;
		lowestOffset=0;
		locationOfLowestOffset=0;
		for (uint32_t i=0;i<mofi.ib.numberOfSlotsTaken;i++){
			InstructionSingle* IS_ptr=mofi.ib.buffer+i;
			if (IS_ptr->id==I_LOFF){
				if (!didFind | IS_ptr->arg.D.a_0<lowestOffset){
					didFind=true;
					lowestOffset=IS_ptr->arg.D.a_0;
					locationOfLowestOffset=i;
				}
			}
		}
	} while (didFind);
	if (mofi.size!=currentOffset){
		IS_temp.id=I_ZNXB;
		IS_temp.arg.D.a_0=mofi.size-currentOffset;
		currentOffset=mofi.size;
		addInstruction(&ib_destination,IS_temp);
	}
	destroyInstructionBuffer(&mofi.ib);
	dataBytecodeReduction(&ib_destination);
	return ib_destination;
}


/*
the instruction buffer that is given should have LOFF as first instruction
the instruction buffer that is given will be destroyed (deallocated)
returns 0 for no error
returns 1 for memory overwrite failure
returns 2 for alignment failure
returns 3 for value stack overflow
if return value is not zero, the InstructionBuffer is not destroyed
*/
uint8_t setUnitInitializer(
		struct MemoryOrganizerForInitializer* mofi,
		InstructionBuffer* ib){
	
	assert(ib->buffer[0].id==I_LOFF);
	uint8_t expectedResultWordSize;
	enum InstructionTypeID data_type=ib->buffer[1].id;
	{
		uint32_t location=ib->buffer[0].arg.D.a_0;
		bool* isSet=mofi->isSet+location;
		if (data_type==I_BYTE | data_type==I_SYDB){
			if (*(isSet+0)) return 1;
			*(isSet+0)=1;
			expectedResultWordSize=1;
		} else if (data_type==I_WORD | data_type==I_SYDW){
			if ((location&1)!=0) return 2;
			if (*(isSet+0) | *(isSet+1)) return 1;
			*(isSet+0)=1;*(isSet+1)=1;
			expectedResultWordSize=1;
		} else {
			assert(data_type==I_DWRD | data_type==I_SYDD);
			if ((location&1)!=0) return 2;
			if (*(isSet+0) | *(isSet+1) | *(isSet+2) | *(isSet+3)) return 1;
			*(isSet+0)=1;*(isSet+1)=1;*(isSet+2)=1;*(isSet+3)=1;
			expectedResultWordSize=2;
		}
	}
	if (data_type!=I_BYTE & data_type!=I_WORD & data_type!=I_DWRD){
		// todo: test value computations. especially dword, I'm not completely sure that the 2 words don't get switched at some point
		ConstOptStart:;
		uint16_t valueStack[100];
		bool isValueStackConst[100];
		int16_t valueStackLocation=0;
		InstructionSingle* IS_ptr=ib->buffer+2;
		assert(IS_ptr->id!=I_SYDE);
		do {
			if (valueStackLocation>97) return 3;
			bool isResultConstant; // is false for constant loads
			switch (IS_ptr->id){
				case I_SYCB:
				valueStack[valueStackLocation]=IS_ptr->arg.B.a_0;
				isValueStackConst[valueStackLocation]=true;
				valueStackLocation+=1;
				isResultConstant=false;
				break;
				case I_SYCW:
				valueStack[valueStackLocation]=IS_ptr->arg.W.a_0;
				isValueStackConst[valueStackLocation]=true;
				valueStackLocation+=1;
				isResultConstant=false;
				break;
				case I_SYCD:
				valueStack[valueStackLocation+0]=IS_ptr->arg.D.a_0>>16;
				valueStack[valueStackLocation+1]=IS_ptr->arg.D.a_0>> 0;
				isValueStackConst[valueStackLocation+0]=true;
				isValueStackConst[valueStackLocation+1]=true;
				valueStackLocation+=2;
				isResultConstant=false;
				break;
				case I_SYCL:
				isValueStackConst[valueStackLocation+0]=false;
				isValueStackConst[valueStackLocation+1]=false;
				valueStackLocation+=2;
				isResultConstant=false;
				break;
				case I_SYC1:
				case I_SYC3:
				case I_SYC5:
				case I_SYC7:
				case I_SYC9:
				case I_SYCY:
				case I_SYCU:
				case I_SYCQ:
				isResultConstant=isValueStackConst[valueStackLocation-1]&isValueStackConst[valueStackLocation-2]&isValueStackConst[valueStackLocation-3]&isValueStackConst[valueStackLocation-4];
				valueStackLocation-=4;
				isValueStackConst[valueStackLocation+0]=false;
				isValueStackConst[valueStackLocation+1]=false;
				valueStackLocation+=2;
				break;
				case I_SYC0:
				case I_SYC2:
				case I_SYC4:
				case I_SYC6:
				case I_SYC8:
				case I_SYCX:
				case I_SYCA:
				case I_SYCO:
				case I_SYCT:
				case I_SYCM:
				isResultConstant=isValueStackConst[valueStackLocation-1]&isValueStackConst[valueStackLocation-2];
				valueStackLocation-=2;
				isValueStackConst[valueStackLocation]=false;
				valueStackLocation+=1;
				break;
				case I_SYCZ:
				case I_SYCS:
				isResultConstant=isValueStackConst[valueStackLocation-1];
				valueStackLocation-=1;
				isValueStackConst[valueStackLocation+0]=false;
				isValueStackConst[valueStackLocation+1]=false;
				valueStackLocation+=2;
				break;
				case I_SYCC:
				case I_SYCN:
				isResultConstant=isValueStackConst[valueStackLocation-1];
				isValueStackConst[valueStackLocation-1]=false;
				break;
				default:;assert(false);
			}
			assert(valueStackLocation>=0);
			if (isResultConstant){
				//printInstructionBufferWithMessageAndNumber(ib,"Identified constant operation",0);
				int16_t deltaToDestroy;
				{
					const enum InstructionTypeID id_that_is_const=IS_ptr->id;
					uint16_t constValues[4];
					uint32_t constValues32[2];
					bool resultIsDword=false;
					if (id_that_is_const==I_SYCZ | 
						id_that_is_const==I_SYCS){
						
						resultIsDword=true;
						deltaToDestroy=1;
						valueStackLocation-=1;
						constValues[0]=valueStack[valueStackLocation-1];
					} else if (
						id_that_is_const==I_SYCC | 
						id_that_is_const==I_SYCN){
						
						deltaToDestroy=1;
						constValues[0]=valueStack[valueStackLocation-1];
					} else if (
						id_that_is_const==I_SYC1 | 
						id_that_is_const==I_SYC3 | 
						id_that_is_const==I_SYC5 | 
						id_that_is_const==I_SYC7 | 
						id_that_is_const==I_SYC9 | 
						id_that_is_const==I_SYCY | 
						id_that_is_const==I_SYCU | 
						id_that_is_const==I_SYCQ){
						
						resultIsDword=true;
						deltaToDestroy=4;
						valueStackLocation+=2;
						constValues[0]=valueStack[valueStackLocation-4];
						constValues[1]=valueStack[valueStackLocation-3];
						constValues[2]=valueStack[valueStackLocation-2];
						constValues[3]=valueStack[valueStackLocation-1];
						constValues32[0]=(constValues[1]<<16)|(constValues[0]<< 0);
						constValues32[1]=(constValues[3]<<16)|(constValues[2]<< 0);
					} else {
						deltaToDestroy=2;
						valueStackLocation+=1;
						constValues[0]=valueStack[valueStackLocation-2];
						constValues[1]=valueStack[valueStackLocation-1];
					}
					switch (id_that_is_const){
						case I_SYC1:
						constValues32[0]+=constValues32[1];
						break;
						case I_SYC3:
						constValues32[0]-=constValues32[1];
						break;
						case I_SYC5:
						constValues32[0]*=constValues32[1];
						break;
						case I_SYC7:
						constValues32[0]/=constValues32[1];
						break;
						case I_SYC9:
						constValues32[0]%=constValues32[1];
						break;
						case I_SYCY:
						constValues32[0]^=constValues32[1];
						break;
						case I_SYCU:
						constValues32[0]&=constValues32[1];
						break;
						case I_SYCQ:
						constValues32[0]|=constValues32[1];
						break;
						case I_SYC0:
						constValues[0]+=constValues[1];
						break;
						case I_SYC2:
						constValues[0]-=constValues[1];
						break;
						case I_SYC4:
						constValues[0]*=constValues[1];
						break;
						case I_SYC6:
						constValues[0]/=constValues[1];
						break;
						case I_SYC8:
						constValues[0]%=constValues[1];
						break;
						case I_SYCX:
						constValues[0]^=constValues[1];
						break;
						case I_SYCA:
						constValues[0]&=constValues[1];
						break;
						case I_SYCO:
						constValues[0]|=constValues[1];
						break;
						case I_SYCZ:
						constValues32[0]=constValues[0];
						break;
						case I_SYCS:
						constValues32[0] =((uint32_t)0xFFFF0000lu*((constValues[0]&0xA000u)!=0))|constValues[0];
						break;
						case I_SYCC:
						constValues[0]&=0xFFu;
						constValues[0]|=0xFF00u*((constValues[0]&0x00A0u)!=0);
						break;
						case I_SYCN:
						constValues[0]=constValues[0]!=0;					
						break;
						case I_SYCM:
						constValues[0]=(constValues[0]|constValues[1])!=0;
						case I_SYCT:
						break;
						default:;assert(false);
					}
					if (resultIsDword){
						IS_ptr->id=I_SYCD;
						IS_ptr->arg.D.a_0=constValues32[0];
					} else {
						IS_ptr->id=I_SYCW;
						IS_ptr->arg.W.a_0=constValues[0];
					}
				}
				//printInstructionBufferWithMessageAndNumber(ib,"After Assignment before removal",0);
				while (deltaToDestroy!=0){
					assert(deltaToDestroy>0);
					--IS_ptr;
					assert(IS_ptr>ib->buffer);
					if (IS_ptr->id==I_SYCD){
						if (deltaToDestroy==1){
							printf("I don\'t think this should happen\n");
						}
						deltaToDestroy-=2;
					} else {
						assert(IS_ptr->id==I_SYCW | IS_ptr->id==I_SYCB);
						deltaToDestroy-=1;
					}
					IS_ptr->id=I_NOP_;
				}
				//printInstructionBufferWithMessageAndNumber(ib,"After removal",0);
				removeNop(ib);
				goto ConstOptStart;
			}
		} while ((++IS_ptr)->id!=I_SYDE);
		assert(expectedResultWordSize==valueStackLocation);
		IS_ptr=ib->buffer+2;
		do {
			if (IS_ptr->id==I_SYCW){
				if ((IS_ptr->arg.W.a_0&0xFF00u)==0){
					uint8_t b=IS_ptr->arg.W.a_0;
					IS_ptr->id=I_SYCB;
					IS_ptr->arg.B.a_0=b;
				}
			}
		} while ((IS_ptr++)->id!=I_SYDE);
		if (ib->numberOfSlotsTaken==4){
			IS_ptr=ib->buffer+1;
			if ((IS_ptr+1)->id!=I_SYCL & (IS_ptr+2)->id==I_SYDE){
				const enum InstructionTypeID id0=IS_ptr->id;
				if      (id0==I_SYDB) IS_ptr->id=I_BYTE;
				else if (id0==I_SYDW) IS_ptr->id=I_WORD;
				else if (id0==I_SYDD) IS_ptr->id=I_DWRD;
				else goto SimpleOptExit;
				uint32_t val;
				ib->numberOfSlotsTaken=2;
				const InstructionSingle IS_temp=*(IS_ptr+1);
				if      (IS_temp.id==I_SYCB)    val=IS_temp.arg.B.a_0;
				else if (IS_temp.id==I_SYCW)    val=IS_temp.arg.W.a_0;
				else{assert(IS_temp.id==I_SYCD);val=IS_temp.arg.D.a_0;}
				if      (id0==I_SYDB) IS_ptr->arg.B.a_0=val;
				else if (id0==I_SYDW) IS_ptr->arg.W.a_0=val;
				else                  IS_ptr->arg.D.a_0=val;
			}
		}
		SimpleOptExit:;
	}
	singleMergeIB(&mofi->ib,ib);
	destroyInstructionBuffer(ib);
	return 0;
}


// not all operators are done yet
void applyConstantOperator(ExpressionTreeNode* thisNode){
	ExpressionTreeNode* ln;
	ExpressionTreeNode* rn;
	uint16_t operatorTypeID = thisNode->post.operatorTypeID;
	uint32_t extraVal = thisNode->post.extraVal;
	uint32_t leftConstVal;
	uint32_t rightConstVal;
	uint32_t* thisConstVal=&thisNode->post.constVal;
	if (thisNode->pre.hasLeftNode){
		ln=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
		leftConstVal=ln->post.constVal;
	}
	if (thisNode->pre.hasRightNode){
		rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
		rightConstVal=rn->post.constVal;
	}
	switch (thisNode->operatorID){
		case 10:{
			*thisConstVal=rightConstVal;
		}
		break;
		case 11:{
			*thisConstVal=-((int32_t)rightConstVal);
			if (operatorTypeID==2) *thisConstVal&=0xFFFFu;
		}
		break;
		case 18:{
			*thisConstVal=leftConstVal*rightConstVal;
			if (operatorTypeID==2) *thisConstVal&=0xFFFFu;
		}
		break;
		case 21:{
			if (operatorTypeID==1){
				*thisConstVal=leftConstVal+rightConstVal;
			} else if (operatorTypeID==2){
				*thisConstVal=(leftConstVal+rightConstVal)&0xFFFFu;
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 22:{
			if (operatorTypeID==1){
				*thisConstVal=leftConstVal-rightConstVal;
			} else if (operatorTypeID==2){
				*thisConstVal=(leftConstVal-rightConstVal)&0xFFFFu;
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 14:
		case 62:{
			*thisConstVal=extraVal;
		}
		break;
		case 59:{
			if (operatorTypeID==1){
				printInformativeMessageForExpression(true,"Global variables cannot be in constant expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==2){
				printInformativeMessageForExpression(true,"Local variables cannot be in constant expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==3){
				*thisConstVal=extraVal;
			} else { //operatorTypeID==4
				printInformativeMessageForExpression(true,"Function pointers cannot be in constant expressions",thisNode);
				exit(1);
			}
		}
		break;
		case 61:{
			if (operatorTypeID==1){
				*thisConstVal=extraVal;
			} else {
				printInformativeMessageForExpression(false,"String literals cannot be in constant expressions",thisNode);
				exit(1);
			}
		}
		break;
		default:
		GiveHasNotAddedMessage:
		printInformativeMessageForExpression(true,"Unimplemented Error: I have not (or cannot) add support for this operand in constant expressions",thisNode);
		exit(1);
	}
}

void applySymbolicOperator(ExpressionTreeNode* thisNode){
	ExpressionTreeNode* ln;
	ExpressionTreeNode* rn;
	uint16_t operatorTypeID = thisNode->post.operatorTypeID;
	uint32_t extraVal = thisNode->post.extraVal;
	InstructionBuffer* ib=&thisNode->ib;
	InstructionBuffer* ibLeft;
	InstructionBuffer* ibRight;
	InstructionSingle writeIS; 
	if (thisNode->pre.hasLeftNode){
		ln=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.leftNode;
		ibLeft=&(ln->ib);
	}
	if (thisNode->pre.hasRightNode){
		rn=expressionTreeGlobalBuffer.expressionTreeNodes+thisNode->pre.rightNode;
		if (!(thisNode->operatorID==5 | thisNode->operatorID==6)) ibRight=&(rn->ib);
	}
	switch (thisNode->operatorID){
		case 10:
		case 14:
		singleMergeIB(ib,ibRight);
		break;
		case 11:{
			if (operatorTypeID==2){
				writeIS.id=I_SYCD;
				writeIS.arg.D.a_0=0;
				addInstruction(ib,writeIS);
				singleMergeIB(ib,ibRight);
				writeIS.id=I_SYC3;
				addInstruction(ib,writeIS);
			} else {
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=0;
				addInstruction(ib,writeIS);
				singleMergeIB(ib,ibRight);
				writeIS.id=I_SYC2;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 18:{
			if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC5;
				addInstruction(ib,writeIS);
			} else {
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC4;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 21:{
			if (operatorTypeID==1){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC1;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC0;
				addInstruction(ib,writeIS);
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 22:{
			if (operatorTypeID==1){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC3;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				dualMergeIB(ib,ibLeft,ibRight);
				writeIS.id=I_SYC2;
				addInstruction(ib,writeIS);
			} else {
				goto GiveHasNotAddedMessage;
			}
		}
		break;
		case 59:{
			if (operatorTypeID==1){
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else if (operatorTypeID==2){
				// should be unreachable
				printInformativeMessageForExpression(true,"Local variables cannot be in static expressions",thisNode);
				exit(1);
			} else if (operatorTypeID==3){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else { //operatorTypeID==4
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
				printInformativeMessageForExpression(true,"Function pointers in static expressions are... possibly functional right now",thisNode);
			}
		}
		break;
		case 61:{
			if (operatorTypeID==1){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else {
				writeIS.id=I_SYCL;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			}
		}
		break;
		case 62:
		{
			if (operatorTypeID==1){
				writeIS.id=I_SYCW;
				writeIS.arg.W.a_0=extraVal;
				addInstruction(ib,writeIS);
			} else {
		case 17:
				writeIS.id=I_SYCD;
				writeIS.arg.D.a_0=extraVal;
				addInstruction(ib,writeIS);
			}
		}
		break;
		default:
		GiveHasNotAddedMessage:
		printInformativeMessageForExpression(true,"Unimplemented Error: I have not (or cannot) add support for this operand in static expressions",thisNode);
		exit(1);
	}
}

void expressionToConstant(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if ((oID>=38 & oID<=48) | oID==1 | oID==2 | oID==8 | oID==9){
		printInformativeMessageForExpression(true,"Cannot have value modifying operator inside constant expression",thisNode);
		exit(1);
	}
	if (oID==65 | oID==66){
		printInformativeMessageForExpression(true,"Cannot have function call inside constant expression",thisNode);
		exit(1);
	}
	if (oID==36 | oID==37){
		printInformativeMessageForExpression(true,"Cannot have ternary inside constant expression",thisNode);
		exit(1);
	}
	// should I add short-circuiting to this evaluation?
	if (thisNode->pre.hasLeftNode)
		expressionToConstant(thisNode->pre.leftNode);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionToConstant(thisNode->pre.rightNode);
	ensureExpNodeInit(thisNode);
	applyAutoTypeConversion(thisNode);
	applyConstantOperator(thisNode);
}


void expressionToSymbolic(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if ((oID>=38 & oID<=48) | oID==1 | oID==2 | oID==8 | oID==9){
		printInformativeMessageForExpression(true,"Cannot have value modifying operator inside static expression",thisNode);
		exit(1);
	}
	if (oID==65 | oID==66){
		printInformativeMessageForExpression(true,"Cannot have function call inside static expression",thisNode);
		exit(1);
	}
	if (oID==36 | oID==37){
		printInformativeMessageForExpression(true,"Cannot have ternary inside static expression",thisNode);
		exit(1);
	}
	if (thisNode->pre.hasLeftNode)
		expressionToSymbolic(thisNode->pre.leftNode);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionToSymbolic(thisNode->pre.rightNode);
	ensureExpNodeInit(thisNode);
	initInstructionBuffer(&thisNode->ib);
	applyAutoTypeConversion(thisNode);
	applySymbolicOperator(thisNode);
	if (thisNode->pre.hasLeftNode)
		destroyInstructionBuffer(&expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.leftNode ].ib);
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6 | oID==17))
		destroyInstructionBuffer(&expressionTreeGlobalBuffer.expressionTreeNodes[thisNode->pre.rightNode].ib);
}




struct ConstValueTypePair{
	uint32_t value;
	uint32_t size; // of typeString
};


// this function has specific expectations for being called
bool shouldAvoidWarningsForInitializerExpressionElement(ExpressionTreeNode* rootNode){
	return rootNode->operatorID==62 && rootNode->post.extraVal==0;
}


void expressionToConstantBase(struct ConstValueTypePair* cvtp,const char* typeStringCast, int16_t nodeIndex){ // potentialNoWarn is intended to be removed
	expressionToConstant(nodeIndex);
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	applyTypeCast(thisNode,typeStringCast,15);
	cvtp->value=thisNode->post.constVal;
	cvtp->size=getSizeofForTypeString(thisNode->post.typeString,true);
	if (cvtp->size==0){
		printInformativeMessageForExpression(true,"Failed to get size for the type of this constant expression",thisNode);
		exit(1);
	}
	cosmic_free(thisNode->post.typeString);
}

uint32_t expressionToConstantValue(const char* typeStringCast,int16_t nodeIndex){
	struct ConstValueTypePair cvtp;
	expressionToConstantBase(&cvtp,typeStringCast,nodeIndex);
	return cvtp.value;
}

void expressionToSymbolicRoot(struct MemoryOrganizerForInitializer* mofi,const char* typeStringCast,uint32_t offset,int16_t nodeIndex, bool potentialNoWarn){
	assert(!doSymbolicConstantGenForExp);
	doSymbolicConstantGenForExp=true;
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint32_t sizeOfCastType=getSizeofForTypeString(typeStringCast,true);
	expressionToSymbolic(nodeIndex);
	bool shouldNoWarn=potentialNoWarn && shouldAvoidWarningsForInitializerExpressionElement(thisNode);
	bool isToArrayType=*typeStringCast=='[';
	if (sizeOfCastType==0 | sizeOfCastType==3 | sizeOfCastType>4 | isToArrayType){
		if (sizeOfCastType!=0 & isToArrayType){
			if (shouldNoWarn){
				// then the destination type is an array when doing no warn, in which case it is valid, any there is nothing more to do (except free a few things)
				cosmic_free(thisNode->post.typeString);
				destroyInstructionBuffer(&thisNode->ib);
				doSymbolicConstantGenForExp=false;
				return;
			}
			printInformativeMessageForExpression(true,"cannot initialize an array type with expression",thisNode);
		} else {
			printInformativeMessageForExpression(true,"type size for the destination type of this static expression is invalid",thisNode);
		}
		exit(1);
	}
	applyTypeCast(thisNode,typeStringCast,15*!shouldNoWarn);
	cosmic_free(thisNode->post.typeString);
	InstructionBuffer* ib=&thisNode->ib;
	InstructionBuffer ibTemp;
	initInstructionBuffer(&ibTemp);
	InstructionSingle writeIS;
	writeIS.id=I_LOFF;
	writeIS.arg.D.a_0=offset;
	addInstruction(&ibTemp,writeIS);
	if (sizeOfCastType==1){
		writeIS.id=I_SYDB;
	} else if (sizeOfCastType==2){
		writeIS.id=I_SYDW;
	} else if (sizeOfCastType==4){
		writeIS.id=I_SYDD;
	} else {
		assert(false);
	}
	addInstruction(&ibTemp,writeIS);
	singleMergeIB(&ibTemp,ib);
	destroyInstructionBuffer(ib);
	writeIS.id=I_SYDE;
	addInstruction(&ibTemp,writeIS);
	uint8_t errorCode=setUnitInitializer(mofi,&ibTemp);
	if (errorCode!=0){
		if (errorCode==1){
			printInformativeMessageForExpression(true,"Overwrites data previously set inside this initializer",thisNode);
		} else if (errorCode==2){
			printInformativeMessageForExpression(true,"Data alignment error",thisNode);
		} else {
			printInformativeMessageForExpression(true,"This expression is too large",thisNode);
		}
		exit(1);
	}
	doSymbolicConstantGenForExp=false;
}


struct InitializerMap{
	struct InitializerMapEntry{
		// for the next 3, the value is -1 if it doesn't exist
		int32_t chainEntry;
		int32_t subEntry;
		int32_t descriptionBranchEntry;
		int32_t strStart;
		int32_t strMid;
		int32_t strEnd;
		uint8_t typeOfEntry;
		int16_t expNode;
	} *entries;
	int32_t numberOfEntriesAllocated;
	int32_t numberOfEntriesTaken;
	int32_t rootEntry;
	uint8_t typeOfInit;
	
	
} initializerMap;
/*

typeOfInit==0 : no initializer
typeOfInit==1 : bracket pair
typeOfInit==2 : string
typeOfInit==3 : character
typeOfInit==4 : expression


typeOfEntry==0 : {invalid}
typeOfEntry==1 : bracket pair
	subEntry points to first content of bracket pair, which are chained on chainEntry
typeOfEntry==2 : string
	if typeOfInit==2, this is the root and is a modifiable string literal. 
	Otherwise, it is not the root, and is an unmodifiable string literal.
typeOfEntry==3 : character
	if typeOfInit==3, this is the root. Otherwise, it is not the root.
typeOfEntry==4 : expression
	if typeOfInit==4, this is the root. Otherwise, it is not the root.

typeOfEntry==5 : location-array
typeOfEntry==6 : location-member
typeOfEntry==7 : {internal only} used to skip the . for location

*/


int32_t partitionEntryInInitializerMap(){
	if (initializerMap.numberOfEntriesTaken>=initializerMap.numberOfEntriesAllocated){
		if (initializerMap.numberOfEntriesAllocated<64){
			initializerMap.numberOfEntriesAllocated=64;
		} else {
			initializerMap.numberOfEntriesAllocated*=2;
		}
		initializerMap.entries=cosmic_realloc(initializerMap.entries,initializerMap.numberOfEntriesAllocated*sizeof(struct InitializerMapEntry));
	}
	return initializerMap.numberOfEntriesTaken++;
}

int32_t emptyIndexRecedeForInitializerMap(int32_t strStart){
	int32_t strWalk=strStart+1;
	while (true){
		char c=sourceContainer.string[--strWalk];
		assert(strWalk!=0); // hit string start
		if (!(c==' ' | c=='\n')){
			return strWalk;
		}
	}
}

int32_t emptyIndexAdvanceForInitializerMap(int32_t strStart){
	int32_t strWalk=strStart-1;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (!(c==' ' | c=='\n')){
			return strWalk;
		}
	}
}

int32_t findLocatorEndForInitializerMap(int32_t strStart){
	char strStartChar=sourceContainer.string[strStart];
	int32_t strWalk=strStart;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=';' & c!=0);
		if (strStartChar=='.'){
			if (!(c==' ' | c=='\n')){
				return strWalk;
			}
		} else if (strStartChar=='['){
			assert(c!='['); // hit nested brackets
			if (c==']'){
				return strWalk+1;
			}
		} else if (strStartChar=='='){
			return strStart;
		} else if (c==' ' | c=='\n' | c=='.' | c=='[' | c=='='){
			return strWalk;
		} else {
			assert(c!='\'' & c!='\"' & c!='{' & c!='}'); // should not hit string or character literals or brackets
		}
	}
}

int32_t parenSkipForInitializerMap(int32_t strStart){
	bool inLiteral=false;
	int32_t strWalk=strStart;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (inLiteral){
			if (c=='\\') ++strWalk;
			else if (c=='\"' | c=='\'') inLiteral=false;
		} else {
			if (c=='(') strWalk=parenSkipForInitializerMap(strWalk);
			else if (c==')') return strWalk;
			else if (c=='\"' | c=='\'') inLiteral=true;
		}
	}
}

int32_t findTypicalEndForInitializerMap(int32_t strStart){
	bool inLiteral=false;
	int32_t strWalk=strStart-1;
	while (true){
		char c=sourceContainer.string[++strWalk];
		assert(c!=0);
		if (c=='\'' | c=='\"'){
			inLiteral^=true;
			continue;
		}
		if (inLiteral){
			if (c=='\\') ++strWalk;
			continue;
		}
		if (c==';'){
			return strWalk;
		} else if (c=='('){
			strWalk=parenSkipForInitializerMap(strWalk);
		} else if (c=='{' & strWalk!=strStart){
			strWalk=findTypicalEndForInitializerMap(strWalk);
		} else if (c==',' | c=='}'){
			if (!(c==',' & sourceContainer.string[strStart]=='{')){
				return strWalk;
			}
		} else if (c=='=' && sourceContainer.string[strStart]!='{'){
			int32_t t = findTypicalEndForInitializerMap(emptyIndexAdvanceForInitializerMap(strWalk+1));
			return t;
		}
	}
}

uint8_t catagorizeInitializerMapLocator(int32_t strStart){
	char strStartChar0=sourceContainer.string[strStart];
	assert(strStartChar0!=' ' & strStartChar0!='\n'); // bad starting place
	if (strStartChar0=='.'){
		return 7;
	} else if (strStartChar0=='['){
		return 5;
	} else {
		return 6;
	}
}

int32_t initializerMapLocator(int32_t strStart){
	struct InitializerMapEntry ime={-1,-1,-1,strStart,strStart,
		findLocatorEndForInitializerMap(strStart),catagorizeInitializerMapLocator(strStart),-1};
	if (ime.typeOfEntry==7){
		return initializerMapLocator(ime.strEnd);
	} else if (strStart==ime.strEnd){
		return -1;
	}
	int32_t thisEntry=partitionEntryInInitializerMap();
	ime.chainEntry=initializerMapLocator(emptyIndexAdvanceForInitializerMap(ime.strEnd));
	initializerMap.entries[thisEntry]=ime;
	return thisEntry;
}

uint8_t catagorizeInitializerMapTypical(char* string,int32_t index){
	const char c0=string[index  ];
	const char c1=string[index+1];
	if (c0=='{'){
		return 1;
	} else if (c0=='\"' | (c0=='L' & c1=='\"')){
		if (initializerMap.typeOfInit==0){
			if (string[emptyIndexAdvanceForInitializerMap(getEndOfToken(string,index))]==';') return 2;
			return 4;
		} else if (initializerMap.typeOfInit==4) return 4;
		return 2;
	} else if (c0=='\'' | (c0=='L' & c1=='\'')){
		return 3;
	}
	return 4;
}

int32_t initializerMapTypical(int32_t strStart){
	struct InitializerMapEntry ime={-1,-1,-1,strStart,strStart,findTypicalEndForInitializerMap(strStart),0,-1};
	{
	char strStartChar0=sourceContainer.string[strStart];
	assert(!(strStartChar0==' ' | strStartChar0=='\n')); // bad starting place
	if (strStartChar0=='.' | strStartChar0=='['){
		while (true){
			char c=sourceContainer.string[++ime.strMid];
			if (c=='='){
				ime.strMid=emptyIndexAdvanceForInitializerMap(ime.strMid+1);
				assert(ime.strMid<ime.strEnd); // overran strEnd while looking for locator end
				ime.descriptionBranchEntry=initializerMapLocator(strStart);
				break;
			} else {
				assert(c!=';' & c!='{' & c!='}' & c!=0); // should not hit these while looking for locator
			}
		}
	}
	ime.typeOfEntry=catagorizeInitializerMapTypical(sourceContainer.string,ime.strMid);
	}
	int32_t thisEntry=partitionEntryInInitializerMap();
	int32_t strEndNextIndex=emptyIndexAdvanceForInitializerMap(ime.strEnd+1);
	char strEndChar=sourceContainer.string[ime.strEnd];
	if (ime.typeOfEntry==1){
		assert(strEndChar=='}'); // bad end
		ime.subEntry=initializerMapTypical(emptyIndexAdvanceForInitializerMap(ime.strMid+1));
		if (sourceContainer.string[strEndNextIndex]==','){
			ime.chainEntry=initializerMapTypical(emptyIndexAdvanceForInitializerMap(strEndNextIndex+1));
		}
	} else {
		assert(strEndChar=='}' | strEndChar==';' | strEndChar==','); // bad end
		if (strEndChar==','){
			ime.chainEntry=initializerMapTypical(strEndNextIndex);
		}
	}
	initializerMap.entries[thisEntry]=ime;
	return thisEntry;
}

// returns true if the end was found exactly, throws error if end was overstepped
bool endCheckInitializerMap(int32_t entryIndex,int32_t strStart,int32_t strEnd){
	if (entryIndex==-1) return false;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	int32_t thisStrEnd=ime->strEnd;
	bool foundEnd=thisStrEnd==strEnd;
	assert(thisStrEnd<=strEnd); // check if initializerMapRoot() overstepped strEnd
	foundEnd|=endCheckInitializerMap(ime->subEntry              ,strStart,strEnd);
	foundEnd|=endCheckInitializerMap(ime->chainEntry            ,strStart,strEnd);
	foundEnd|=endCheckInitializerMap(ime->descriptionBranchEntry,strStart,strEnd);
	return foundEnd;
}

void expressionPrecheckForInitializer(int16_t nodeIndex){
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = thisNode->operatorID;
	if (oID==59){
		struct IdentifierSearchResult isr;
		char* identifier = copyStringSegmentToHeap(sourceContainer.string,thisNode->startIndexInString,thisNode->endIndexInString);
		searchForIdentifier(&isr,identifier,true,true,true,true,true);
		if (!isr.didExist){
			printInformativeMessageForExpression(true,"This identifier does not exist",thisNode);
			exit(1);
		}
		cosmic_free(identifier);
	}
	/*
	the times where it is valid to call a function here, 
	the function call identifier will be checked elsewhere 
	and it will be okay to not precheck it as other identifiers must be.
	That is because there are times where it is valid to use an identifier 
	but this precheck must be done on that identifier.
	*/
	if (thisNode->pre.hasRightNode&!(oID==5 | oID==6))
		expressionPrecheckForInitializer(thisNode->pre.rightNode);
	if (thisNode->pre.hasLeftNode)
		expressionPrecheckForInitializer(thisNode->pre.leftNode);
	if (thisNode->pre.hasChainNode)
		expressionPrecheckForInitializer(thisNode->pre.chainNode);
	
}

void genExpressionsForInitializerMap(int32_t entryIndex){
	if (entryIndex==-1) return;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	uint8_t typeOfEntry=ime->typeOfEntry;
	if (typeOfEntry>=2 & typeOfEntry<=5){
		int32_t start=ime->strMid;
		int32_t end=ime->strEnd-1;
		if (typeOfEntry==5){
			++start;
			--end;
		}
		start=emptyIndexAdvanceForInitializerMap(start);
		end=emptyIndexRecedeForInitializerMap(end)+1;
		ime->expNode=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,start,end,false);
		if (ime->expNode!=-1){
			expressionPrecheckForInitializer(ime->expNode);
		} else {
			printInformativeMessageAtSourceContainerIndex(true,"Expression expected here",start,end);
			exit(1);
		}
	}
	genExpressionsForInitializerMap(ime->subEntry);
	genExpressionsForInitializerMap(ime->chainEntry);
	genExpressionsForInitializerMap(ime->descriptionBranchEntry);
}


// returns root. clears any previous initializer mappings, and clear previous expressions. if strEnd==-1 then it doesn't check for overstepping strEnd
int32_t initializerMapRoot(int32_t strStart,int32_t strEnd){
	clearPreviousExpressions();
	initializerMap.numberOfEntriesTaken=0;
	initializerMap.typeOfInit=0;//this set is needed for catagorizeInitializerMapTypical() in case a string is first and it needs to know if it is running to find the typeOfInit or typeOfEntry
	initializerMap.typeOfInit=catagorizeInitializerMapTypical(sourceContainer.string,strStart);
	int32_t root = initializerMapTypical(strStart);
	if (initializerMap.typeOfInit==1){
		strEnd=emptyIndexRecedeForInitializerMap(strEnd-1);
		assert(strStart<strEnd); // bracket location calc failure
	}
	assert(initializerMap.entries[root].typeOfEntry==initializerMap.typeOfInit); // failed to agree on catagory
	assert(!(strEnd!=-1 && !endCheckInitializerMap(root,strStart,strEnd))); // understepped the end of the initializer
	if (initializerMap.typeOfInit!=2){
		genExpressionsForInitializerMap(root);
	}
	return root;
}

#include "TypeStringCracking.c"


char* getTypeStringOfFirstNonStructUnionMember(const char* typeString){
	struct TypeSearchResult tsr;
	typeString=stripQualifiersC(typeString,NULL,NULL);
	assert(isTypeStringOfStructOrUnion(typeString));
	bool isUnion=isSectionOfStringEquivalent(typeString,0,"union");
	searchForType(&tsr,typeString+(7-isUnion),isUnion);
	assert(tsr.didExist);
	char* firstMemberTypeString;
	if (tsr.isGlobal) firstMemberTypeString=(char*)blockFrameArray.globalBlockFrame.globalTypeEntries[tsr.typeEntryIndex].arrayOfMemberEntries[0].typeString;
	else firstMemberTypeString=(char*)blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries[tsr.typeEntryIndex].arrayOfMemberEntries[0].typeString;
	firstMemberTypeString=applyToTypeStringRemoveIdentifierToNew(firstMemberTypeString);// this is why the cast to (char*) is okay [a new string gets allocated]
	if (isTypeStringOfStructOrUnion(firstMemberTypeString)){
		char* sub=getTypeStringOfFirstNonStructUnionMember(firstMemberTypeString);
		cosmic_free(firstMemberTypeString);
		return sub;
	}
	return firstMemberTypeString;
}


void initializerImplementStaticExpression(
		int32_t entryIndex,
		struct MemoryOrganizerForInitializer* mofi,
		const char* typeStringCast,
		uint32_t memoryOffset,
		bool potentialNoWarn){
	
	if (isTypeStringOfStructOrUnion(stripQualifiersC(typeStringCast,NULL,NULL))){
		// this causes static initizations of structs/unions from expression -> initializing that struct/union 's first member from expression
		char* firstMemberTypeString=getTypeStringOfFirstNonStructUnionMember(typeStringCast);
		initializerImplementStaticExpression(entryIndex,mofi,firstMemberTypeString,memoryOffset,true);
		cosmic_free(firstMemberTypeString);
	} else {
		expressionToSymbolicRoot(mofi,typeStringCast,memoryOffset,initializerMap.entries[entryIndex].expNode,potentialNoWarn); // todo: remove null
	}
}

void initializerImplementNonstaticExpression(
		int32_t entryIndex,
		InstructionBuffer* ib,
		const char* typeStringCast,
		uint16_t stackOffset,
		bool isBehindList,
		bool potentialNoWarn){
	
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	ExpressionTreeNode* thisNode=expressionTreeGlobalBuffer.expressionTreeNodes+ime->expNode;
	expressionToAssembly(ime->expNode,NULL,0);
	bool isSU=isTypeStringOfStructOrUnion(thisNode->post.typeStringNQ);
	uint32_t typeSize=getSizeofForTypeString(typeStringCast,true);
	const char* typeStringCastNQ=stripQualifiersC(typeStringCast,NULL,NULL);
	bool isToSU=isTypeStringOfStructOrUnion(typeStringCastNQ);
	uint8_t warnValue=15*!(potentialNoWarn && shouldAvoidWarningsForInitializerExpressionElement(thisNode));
	if (isSU!=isToSU){
		if (isBehindList&!isSU){
			// this causes the first member of a struct/union to be initialized when attempting to initialize it from a non-struct/union expression when in a list initializer
			isToSU=false;isSU=false; // not needed, but to be clear what this ends up doing
			
			char* firstMemberTypeString=getTypeStringOfFirstNonStructUnionMember(typeStringCast);
			applyTypeCast(thisNode,firstMemberTypeString,warnValue);
			typeSize=getSizeofForTypeString(firstMemberTypeString,true);
			cosmic_free(firstMemberTypeString);
			goto InsertJmp;
		} else {
			const char* message;
			if (isSU) message="Cannot initialize non-struct and non-union with struct or union expression";
			else message="Cannot initialize struct or union with non-struct and non-union expression";
			printInformativeMessageAtSourceContainerIndex(true,message,ime->strStart,0);
			exit(1);
		}
	}
	if (!isSU) applyTypeCast(thisNode,typeStringCast,warnValue);
	InsertJmp:;
	singleMergeIB(ib,&thisNode->ib);
	destroyInstructionBuffer(&thisNode->ib);
	if (isSU){
		/*
		TODO: the following ignores volatile and const. when reading memory, it should obey this.
		For writing memory, 
			I would kinda expect that volatile can be ignored (it is garenteed to be writing to stack memory), 
			and const should be ignored (because this is the initializer, 
			if the initializer can't write to it then initializing const values wouldn't be possible)
		*/
		if (!doStringsMatch(thisNode->post.typeStringNQ,typeStringCastNQ)){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize differing struct or union types",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (typeSize==0){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize incomplete struct or union",ime->strStart,0);
			exit(1);
		}
		insert_IB_STPI(ib,stackOffset);
		singleMergeIB(ib,&ib_stack_swp_22);
		while (typeSize!=0){
			singleMergeIB(ib,&ib_mem_word_copy_n_n);
			typeSize-=2;
		}
		addQuadVoidPop(ib);
	} else {
		assert(!(typeSize==0 | typeSize==3 | typeSize>4));
		insert_IB_STPI(ib,stackOffset);
		if (typeSize==1){
			singleMergeIB(ib,&ib_mem_byte_write_n);
		} else if (typeSize==4){
			singleMergeIB(ib,&ib_mem_dword_write_n);
			addVoidPop(ib);
		} else {
			singleMergeIB(ib,&ib_mem_word_write_n);
		}
		addVoidPop(ib);
	}
	cosmic_free(thisNode->post.typeString);
}

char* advanceToMembersInCrackedType(char* crackedType){
	char c;
	while ((c=*(crackedType++))!='{'){
		assert(c!=0);
		if (c=='|') return NULL;
	}
	return crackedType;
}

char* advanceToNextMemberInCrackedType(char* crackedType){
	char c;
	while ((c=*crackedType)!=';'){
		assert(c!=0);
		if (c=='<') crackedType=getIndexOfMatchingEnclosement(crackedType,0)+crackedType;
		crackedType++;
	}
	return crackedType+1;
}

char* gotoMemberAtIndex(char* crackedType,uint16_t memberIndex){
	crackedType=advanceToMembersInCrackedType(crackedType);
	if (crackedType==NULL) return NULL;
	for (uint16_t i=0;i<memberIndex;i++){
		crackedType=advanceToNextMemberInCrackedType(crackedType);
		if (*crackedType=='}') return NULL;
	}
	return crackedType;
}

char* advanceToNameInCrackedType(char* crackedType){
	char c;
	while (true){
		c=*crackedType;
		if (c=='<'){
			crackedType+=getIndexOfMatchingEnclosement(crackedType,0)+1;
			break;
		} else if (c=='('){
			crackedType+=getIndexOfMatchingEnclosement(crackedType,0)+1;
			crackedType=advanceToNameInCrackedType(crackedType);
			break;
		} else if (c=='?'|c=='!'|c=='#'){
			crackedType+=8;
		} else if (c=='j' | c=='h'){
			while (*crackedType!='|'){
				assert(*crackedType!=0);
				crackedType++;
			}
			crackedType++;
			break;
		} else if (c!='*'){
			crackedType++;
			break;
		}
		crackedType++;
	}
	return crackedType;
}

uint16_t findMemberIndexForName(char* crackedType,int32_t strStart,int32_t strEnd){
	char* subCrackedType;
	uint16_t i=0;
	while ((subCrackedType=gotoMemberAtIndex(crackedType,i))!=NULL){
		subCrackedType=advanceToNameInCrackedType(subCrackedType);
		int32_t strWalk=strStart;
		while (true){
			if (*subCrackedType==';'){
				if (strWalk==strEnd) return i;
				else break;
			}
			if (strWalk>strEnd) break;
			if (*(subCrackedType++)!=sourceContainer.string[strWalk++]) break;
		}
		i++;
	}
	return 0xFFFFu;
}


uint32_t designatorSourceStart;
uint32_t designatorSourceEnd;
char* designatedCrackedType;
uint32_t baseDesignation;
uint32_t designationOffsetTotal;
bool doDesignationOffsetCalc;


// fallbackIndex is used only if there are no designators
void calcPotentialDesignatorAndSize(int32_t entryIndex,char* crackedType, uint32_t fallbackIndex){
	bool hasSubEntry;
	bool hasQualifier;
	bool isArray;
	bool isFirst=true;
	char cts;
	char* crackedTypeNQ;
	char* subCrackedType;
	uint32_t thisBaseDesignation=fallbackIndex;
	designationOffsetTotal=0;
	if (entryIndex==-1){
		cts= crackedType[0];
		if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
		isArray=cts!='<';
		crackedTypeNQ=crackedType+hasQualifier;
		if (isArray){
			uint32_t suggestedLength=readHexInString(crackedTypeNQ+1);
			if (thisBaseDesignation>=suggestedLength){
				if (cts=='#'){
					printInformativeMessageAtSourceContainerIndex(true,"Array is too small for this list",designatorSourceStart,designatorSourceEnd);
					exit(1);
				}
				*crackedTypeNQ='!';
				writeHexInString(crackedTypeNQ+1,thisBaseDesignation+1);
			}
			subCrackedType=crackedTypeNQ+9;
		} else {
			subCrackedType = gotoMemberAtIndex(crackedTypeNQ,thisBaseDesignation);
			if (subCrackedType==NULL){
				printInformativeMessageAtSourceContainerIndex(true,"No more members to initialize",designatorSourceStart,designatorSourceEnd);
				exit(1);
			}
		}
		baseDesignation=thisBaseDesignation;
		goto LabelForNoDesignators;
	}
	do {
		struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
		assert(ime->typeOfEntry==5 | ime->typeOfEntry==6);
		cts= crackedType[0];
		if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
		if ((ime->typeOfEntry==5 & cts!='?' & cts!='!' & cts!='#') |
			(ime->typeOfEntry==6 & cts!='<')){
			
			printInformativeMessageAtSourceContainerIndex(true,"This designator and the type do not match",ime->strStart,ime->strEnd);
			exit(1);
		}
		crackedTypeNQ=crackedType+hasQualifier;
		isArray=cts!='<';
		if (isArray){
			thisBaseDesignation=expressionToConstantValue("unsigned long",ime->expNode);
			uint32_t suggestedLength=readHexInString(crackedTypeNQ+1);
			if (thisBaseDesignation>=suggestedLength){
				if (cts=='#'){
					printInformativeMessageAtSourceContainerIndex(true,"This designator\'s index is out of bounds",ime->strStart,ime->strEnd);
					exit(1);
				}
				*crackedTypeNQ='!';
				writeHexInString(crackedTypeNQ+1,thisBaseDesignation+1);
			}
			subCrackedType=crackedTypeNQ+9;
		} else {
			thisBaseDesignation = findMemberIndexForName(crackedTypeNQ,ime->strStart,ime->strEnd);
			if ((uint16_t)thisBaseDesignation==0xFFFFu){
				printInformativeMessageAtSourceContainerIndex(true,"No member of this name in this type",ime->strStart,ime->strEnd);
				exit(1);
			}
			subCrackedType=gotoMemberAtIndex(crackedTypeNQ,thisBaseDesignation);
		}
		if (isFirst) baseDesignation=thisBaseDesignation;
		entryIndex=ime->chainEntry; // these expressions are preparing for the potential next loop body
		isFirst=false;
		LabelForNoDesignators:
		crackedType=subCrackedType;
		if (doDesignationOffsetCalc){
			if (isArray){
				uint32_t unitSize;
				{
					char* unitType=advancedCrackedTypeToTypeString(crackedTypeNQ+9);
					unitSize=getSizeofForTypeString(unitType,true);
					cosmic_free(unitType);
				}
				designationOffsetTotal+=unitSize*thisBaseDesignation;
			} else {
				struct TypeMemberEntry* tmeArray;
				{
					char* unitType=advancedCrackedTypeToTypeString(crackedTypeNQ);
					struct TypeSearchResult tsr;
					char diffSU=*(crackedTypeNQ+1);
					assert(diffSU=='j' | diffSU=='h');
					assert(getIndexOfNthSpace(unitType,0)!=-1);
					assert(getIndexOfNthSpace(unitType,1)==-1);
					searchForType(&tsr,(getIndexOfNthSpace(unitType,0)+1)+unitType,diffSU=='h');
					assert(tsr.didExist);
					if (tsr.isGlobal) tmeArray=blockFrameArray.globalBlockFrame.globalTypeEntries[tsr.typeEntryIndex].arrayOfMemberEntries;
					else tmeArray=blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries[tsr.typeEntryIndex].arrayOfMemberEntries;
					cosmic_free(unitType);
				}
				designationOffsetTotal+=tmeArray[thisBaseDesignation].offset;
			}
		}
	} while (entryIndex!=-1);
	designatedCrackedType=crackedType;
}



void initializerPreImplementList(
		int32_t entryIndex,
		char* crackedType){
	
	doDesignationOffsetCalc=false;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	char cts= crackedType[0];
	bool hasQualifier;
	if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,designatedCrackedType);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isArray){
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			if (imeChain->typeOfEntry==1){
				initializerPreImplementList(chainIndex,designatedCrackedType);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isBase){
		if (imeChain->chainEntry!=-1){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot use multiple expressions to initialize a type that isn't an array, struct, or union",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (imeChain->descriptionBranchEntry!=-1){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot use designator to initialize a type that isn't an array, struct, or union",ime->strStart,ime->strEnd);
			exit(1);
		}
		if (imeChain->typeOfEntry==1){
			initializerPreImplementList(ime->subEntry,crackedType);
		}
	} else {
		assert(false);// corrupted crackedType
	}
}

void initializerImplementList(
		struct MemoryOrganizerForInitializer* mofi,
		InstructionBuffer* ib,
		uint32_t memoryOffset,
		int32_t entryIndex,
		char* crackedType,
		bool isStatic){
	
	doDesignationOffsetCalc=true;
	struct InitializerMapEntry* ime=initializerMap.entries+entryIndex;
	struct InitializerMapEntry* imeChain=initializerMap.entries+ime->subEntry; // name is a little misleading at the beginning
	int32_t chainIndex=ime->subEntry;
	char cts= crackedType[0];
	bool hasQualifier;
	if ((hasQualifier=(cts=='g' | cts=='f'))) cts= crackedType[1];
	bool isSU= cts=='<';
	bool isArray= cts=='?' | cts=='!' | cts=='#';
	bool isBase= cts=='z'|cts=='x'|cts=='c'|cts=='v'|cts=='b'|cts=='n'|cts=='m'|cts=='l'|cts=='k'|cts=='*'|cts=='(';
	uint32_t walkingIndex=0;
	if (isSU){
		bool isFirst=true;
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			uint32_t subOffset=memoryOffset+designationOffsetTotal;
			bool isFirstAndLast=isFirst&(imeChain->chainEntry==-1);
			if (imeChain->typeOfEntry==1){
				initializerImplementList(mofi,ib,subOffset,chainIndex,designatedCrackedType,isStatic);
			} else {
				char* subTypeString=advancedCrackedTypeToTypeString(designatedCrackedType);
				if (doesThisTypeStringHaveAnIdentifierAtBeginning(subTypeString)){
					applyToTypeStringRemoveIdentifierToSelf(subTypeString);
				}
				if (isStatic) {initializerImplementStaticExpression(chainIndex,mofi,subTypeString,subOffset,isFirstAndLast);}
				else {initializerImplementNonstaticExpression(chainIndex,ib,subTypeString,subOffset,true,isFirstAndLast);}
				cosmic_free(subTypeString);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				isFirst=false;
				continue;
			}
			break;
		}
	} else if (isArray){
		while (true){
			designatedCrackedType=NULL;
			designatorSourceStart=imeChain->strStart;
			designatorSourceEnd=imeChain->strEnd;
			calcPotentialDesignatorAndSize(imeChain->descriptionBranchEntry,crackedType,walkingIndex);
			walkingIndex=baseDesignation;
			assert(designatedCrackedType!=NULL);
			assert(walkingIndex<readHexInString(crackedType+1+hasQualifier));
			uint32_t subOffset=memoryOffset+designationOffsetTotal;
			if (imeChain->typeOfEntry==1){
				initializerImplementList(mofi,ib,subOffset,chainIndex,crackedType+9+hasQualifier,isStatic);
			} else {
				char* subTypeString=advancedCrackedTypeToTypeString(designatedCrackedType);
				if (doesThisTypeStringHaveAnIdentifierAtBeginning(subTypeString)){
					applyToTypeStringRemoveIdentifierToSelf(subTypeString);
				}
				if (isStatic) {initializerImplementStaticExpression(chainIndex,mofi,subTypeString,subOffset,false);}
				else {initializerImplementNonstaticExpression(chainIndex,ib,subTypeString,subOffset,true,false);}
				cosmic_free(subTypeString);
			}
			if (imeChain->chainEntry!=-1){
				chainIndex=imeChain->chainEntry;
				imeChain=initializerMap.entries+imeChain->chainEntry;
				walkingIndex++;
				continue;
			}
			break;
		}
	} else if (isBase){
		assert(imeChain->chainEntry==-1);
		assert(imeChain->descriptionBranchEntry==-1);
		if (imeChain->typeOfEntry==1){
			initializerImplementList(mofi,ib,memoryOffset,ime->subEntry,crackedType,isStatic);
		} else {
			char* unitType=advancedCrackedTypeToTypeString(crackedType);
			if (isStatic) {initializerImplementStaticExpression(chainIndex,mofi,unitType,memoryOffset,false);}
			else {initializerImplementNonstaticExpression(chainIndex,ib,unitType,memoryOffset,true,false);}
			cosmic_free(unitType);
		}
	} else {
		assert(false);
	}
}


// returns true if it inserted initialization data into InstructionBuffer, otherwise it inserted assembly
bool initializerImplementRoot(
		InstructionBuffer* ib,
		int32_t root,
		uint32_t labelNumber,
		char** typeStringPtr,
		bool isStatic){
	
	struct MemoryOrganizerForInitializer mofi;
	char* localTypeString=stripQualifiers(*typeStringPtr,NULL,NULL);
	struct InitializerMapEntry* ime=initializerMap.entries+root;
	bool useExpressionForString=false;
	if (initializerMap.typeOfInit==2 & localTypeString[0]=='*'){
		useExpressionForString=true;
		genExpressionsForInitializerMap(root);
	}
	if (initializerMap.typeOfInit==1){
		
		char* crackedType = crackTypeString(*typeStringPtr);
		if (crackedType==NULL){
			printInformativeMessageAtSourceContainerIndex(true,strMerge2(strMerge2("typeString \'",*typeStringPtr),"\' is or contains an incomplete struct or union"),ime->strStart,0);
			exit(1);
		}
		for (int32_t i=0;crackedType[i];i++){
			if (crackedType[i]=='?' & i!=0){
				printInformativeMessageAtSourceContainerIndex(false,"Initializing an incomplete type like this is typically invalid.\n    However, this compiler can probably still calculate the array size",ime->strStart,0);
				break;
			}
		}
		initializerPreImplementList(root,crackedType);
		for (int32_t i=0;crackedType[i];i++){
			if (crackedType[i]=='?'){
				printInformativeMessageAtSourceContainerIndex(true,"An array type remains incomplete, this initializer is insufficient for this type declaration",ime->strStart,0);
				exit(1);
			}
		}
		uint16_t arrayCount=0;
		for (int32_t i=0;crackedType[i];i++){
			char c=crackedType[i];
			if (c=='!'){
				char* new=insertSizeToEmptySizedArrayForTypeStringToNew(*typeStringPtr,arrayCount,readHexInString(crackedType+(i+1)));
				cosmic_free(*typeStringPtr);
				*typeStringPtr=new;
				arrayCount++;
			} else if (c=='#'){
				arrayCount++;
			} else if (c=='<'){
				break;
			}
		}
		uint32_t typeSize = getSizeofForTypeString(*typeStringPtr,true);
		assert(typeSize!=0); // sizeof failure not expected
		typeSize+=typeSize&1;
		if (isStatic){
			initMemoryOrganizerForInitializer(&mofi,typeSize);
			initializerImplementList(&mofi,NULL,0,root,crackedType,true);
			InstructionBuffer ib_temp=finalizeMemoryOrganizerForInitializer(&mofi,labelNumber,false);
			singleMergeIB(ib,&ib_temp);
			destroyInstructionBuffer(&ib_temp);
		} else {
			uint32_t temp=typeSize;
			while (temp){
				// todo: switch to false and see if it generates identical code with the optimizer (if it does then false is more efficient)
				if (true){
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,0);
				insert_IB_load_dword(ib,typeSize-temp);
				dualMergeIB(ib,&ib_i_32add,&ib_mem_word_write_n);
				addVoidPop(ib);
				temp-=2;
				} else {
				temp-=2;
				insert_IB_load_word(ib,0);
				insert_IB_STPI(ib,temp);
				singleMergeIB(ib,&ib_mem_word_write_n);
				addVoidPop(ib);
				}
			}
			initializerImplementList(NULL,ib,0,root,crackedType,false);
		}
		cosmic_free(crackedType);
		return isStatic;
	} else if (initializerMap.typeOfInit==2 &!useExpressionForString){
		if (localTypeString[0]!='['){
			printInformativeMessageAtSourceContainerIndex(true,"Initializer invalid for declared type",ime->strStart,ime->strEnd);
			exit(1);
		}
		bool isWideLiteral=sourceContainer.string[ime->strStart]=='L';
		if (isWideLiteral){
			printInformativeMessageAtSourceContainerIndex(true,"Wide string literals not ready yet",ime->strStart,ime->strEnd);
			exit(1);
		}
		int32_t enclosementIndex=getIndexOfMatchingEnclosement(localTypeString,0);
		char* temp=stripQualifiers(localTypeString+enclosementIndex+2,NULL,NULL);
		if (!doStringsMatch(temp,"char") & !doStringsMatch(temp,"unsigned char")){
			printInformativeMessageAtSourceContainerIndex(true,"String literal is trying to initialize a type that isn\'t a char[] or char*",ime->strStart,ime->strEnd);
			exit(1);
		}
		uint32_t suggestedLength=findLengthOfDataOfStringLiteral(ime->strStart+isWideLiteral,ime->strEnd,isWideLiteral);
		uint8_t* data=cosmic_malloc(suggestedLength*sizeof(uint8_t));
		writeStringLiteralData(data,ime->strStart+isWideLiteral,ime->strEnd,suggestedLength,isWideLiteral);
		uint32_t typeSize;
		if (enclosementIndex==2){
			localTypeString=insertSizeToEmptySizedArrayForTypeStringToNew(localTypeString,0,suggestedLength);
			cosmic_free(*typeStringPtr);
			*typeStringPtr=localTypeString;
			typeSize=suggestedLength;
			assert(suggestedLength==getSizeofForTypeString(localTypeString,true));
		} else {
			typeSize=getSizeofForTypeString(localTypeString,true);
			assert(typeSize!=0); // typeString corruption
			if (typeSize<suggestedLength-1){
				printInformativeMessageAtSourceContainerIndex(true,"String literal is too large for the indicated array size",ime->strStart,ime->strEnd);
				exit(1);
			}
		}
		InstructionSingle IS_temp;
		if (isStatic){
			IS_temp.id=I_NSNB;
			IS_temp.arg.D.a_0=typeSize;
			addInstruction(ib,IS_temp);
			IS_temp.id=I_LABL;
			IS_temp.arg.D.a_0=labelNumber;
			addInstruction(ib,IS_temp);
			IS_temp.id=I_BYTE;
			for (uint32_t i=0;i<typeSize;i++){
				IS_temp.arg.B.a_0=data[i];
				addInstruction(ib,IS_temp);
			}
		} else {
			for (uint32_t i=0;i<typeSize;i++){
				insert_IB_load_byte(ib,data[i]);
				insert_IB_STPI(ib,i);
				singleMergeIB(ib,&ib_mem_byte_write_n);
				addVoidPop(ib);
			}
		}
		cosmic_free(data);
		return isStatic;
	} else {
		if (localTypeString[0]=='['){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize array with expression",ime->strStart,ime->strEnd);
			exit(1);
		}
		uint32_t typeSize=getSizeofForTypeString(localTypeString,true);
		if (typeSize==0){
			printInformativeMessageAtSourceContainerIndex(true,"Cannot initialize incomplete struct or union",ime->strStart,0);
			exit(1);
		}
		typeSize+=typeSize&1;
		if (isStatic){
			if (isTypeStringOfStructOrUnion(localTypeString)){
				printInformativeMessageAtSourceContainerIndex(true,"Initializing by expression to a struct or union of static storage\n    is not possible with the current implementation of constant expressions",ime->strStart,0);
				exit(1);
			}
			initMemoryOrganizerForInitializer(&mofi,typeSize);
			initializerImplementStaticExpression(root,&mofi,*typeStringPtr,0,false);
			InstructionBuffer ib_temp=finalizeMemoryOrganizerForInitializer(&mofi,labelNumber,false);
			singleMergeIB(ib,&ib_temp);
			destroyInstructionBuffer(&ib_temp);
			return true;
		} else {
			initializerImplementNonstaticExpression(root,ib,*typeStringPtr,false,false,false);
			return false;
		}
	}
}



