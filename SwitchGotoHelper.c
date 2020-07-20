


/*

This file is to be included inside StatementWalk.c

it is a seperate (but interlocking) component to help with managing switch statements

*/


struct SwitchManagment {
	InstructionBuffer ibSwitchItem;
	InstructionBuffer ibFinal;
	struct CaseLabelPair{
		uint16_t value;
		uint32_t label;
	} *caseLabelPair;
	uint16_t caseLabelPairLen;
	bool inSwitch;
	bool hasDefault;
	uint32_t defaultLabel;
	struct {
		uint16_t minValue;
		uint16_t maxValue;
	} jumpTableDetails;
} currentSwitchManagment;

void unpackCurrentSwitchManagment(struct SwitchManagment packedSwitchManagment){
	currentSwitchManagment=packedSwitchManagment;
}

struct SwitchManagment packCurrentSwitchManagment(){
	struct SwitchManagment packedSwitchManagment=currentSwitchManagment;
	const struct SwitchManagment n={0};
	currentSwitchManagment=n;
	return packedSwitchManagment;
}


uint32_t insertSwitchCase(int32_t errLocation, uint16_t value){
	for (uint16_t i=0;i<currentSwitchManagment.caseLabelPairLen;i++){
		if (currentSwitchManagment.caseLabelPair[i].value==value){
			printInformativeMessageAtSourceContainerIndex(true,"Identical value already in this switch",errLocation,-1);
			exit(1);
		}
	}
	uint16_t thisIndex=currentSwitchManagment.caseLabelPairLen;
	currentSwitchManagment.caseLabelPair = cosmic_realloc(currentSwitchManagment.caseLabelPair,++currentSwitchManagment.caseLabelPairLen*sizeof(struct CaseLabelPair));
	struct CaseLabelPair thisCaseLabelPair={
		.value=value,
		.label=++globalLabelID
	};
	currentSwitchManagment.caseLabelPair[thisIndex]=thisCaseLabelPair;
	return thisCaseLabelPair.label;
}


/*
returns a value that indicates what kind of method the switch should use (this is a heuristic)
0 - if chain
1 - straightforward jump table

*/
uint16_t switchDetermineMethod(){
	uint16_t valueI=currentSwitchManagment.caseLabelPair[0].value;
	currentSwitchManagment.jumpTableDetails.minValue=valueI;
	currentSwitchManagment.jumpTableDetails.maxValue=valueI;
	for (uint16_t i=1;i<currentSwitchManagment.caseLabelPairLen;i++){
		valueI=currentSwitchManagment.caseLabelPair[i].value;
		if (currentSwitchManagment.jumpTableDetails.minValue>valueI){
			currentSwitchManagment.jumpTableDetails.minValue=valueI;
		} else if (currentSwitchManagment.jumpTableDetails.maxValue<valueI){
			currentSwitchManagment.jumpTableDetails.maxValue=valueI;
		}
	}
	
	
	
	
	if (currentSwitchManagment.jumpTableDetails.maxValue==0xFFFF & currentSwitchManagment.jumpTableDetails.minValue==0U){
		return 0; // definitely too big for a jump table and some methods in type 1 would not work correctly in this case
	}
	/*
	printf("%u,%u,%u,%u\n",
		currentSwitchManagment.jumpTableDetails.maxValue,
		currentSwitchManagment.jumpTableDetails.minValue,
		currentSwitchManagment.caseLabelPairLen/4U,
		(1+(currentSwitchManagment.jumpTableDetails.maxValue-currentSwitchManagment.jumpTableDetails.minValue))/5U);
	*/
	if (currentSwitchManagment.caseLabelPairLen/4U >= (1+(currentSwitchManagment.jumpTableDetails.maxValue-currentSwitchManagment.jumpTableDetails.minValue))/5U){
		return 1;
	}
	return 0;
}

// returns currentSwitchManagment.defaultLabel if the value does not exist
uint32_t switchGetLabelForValue(uint16_t value){
	for (uint16_t i=0;i<currentSwitchManagment.caseLabelPairLen;i++){
		if (value==currentSwitchManagment.caseLabelPair[i].value) return currentSwitchManagment.caseLabelPair[i].label;
	}
	return currentSwitchManagment.defaultLabel;
}

// returns true if it could not resolve due to having no case labels
bool resolveSwitch(){
	if (currentSwitchManagment.caseLabelPairLen==0) return true;
	if (!currentSwitchManagment.hasDefault) currentSwitchManagment.defaultLabel=++globalLabelID;
	initInstructionBuffer(&currentSwitchManagment.ibFinal);
	uint16_t switchMethod=switchDetermineMethod();
	InstructionSingle IS;
	switch (switchMethod){
		case 0:
		{
		singleMergeIB(&currentSwitchManagment.ibFinal,&currentSwitchManagment.ibSwitchItem);
		IS.id=I_POP1;
		IS.arg.B1.a_0=3;
		addInstruction(&currentSwitchManagment.ibFinal,IS);
		for (uint16_t i=0;i<currentSwitchManagment.caseLabelPairLen;i++){
			struct CaseLabelPair* clp = currentSwitchManagment.caseLabelPair+i;
			insert_IB_CJMP_list_item(&currentSwitchManagment.ibFinal,clp->label,clp->value);
		}
		insert_IB_address_label(&currentSwitchManagment.ibFinal,currentSwitchManagment.defaultLabel);
		singleMergeIB(&currentSwitchManagment.ibFinal,&ib_direct_jump);
		break;
		}
		case 1:
		{
		// this has NOT been verified
		uint32_t labelForJJMP = ++globalLabelID;
		
		singleMergeIB(&currentSwitchManagment.ibFinal,&currentSwitchManagment.ibSwitchItem);
		insert_IB_load_word(&currentSwitchManagment.ibFinal,currentSwitchManagment.jumpTableDetails.minValue);
		dualMergeIB(&currentSwitchManagment.ibFinal,&ib_i_16sub,&ib_stack_dupe_word);
		insert_IB_load_word(&currentSwitchManagment.ibFinal,
			currentSwitchManagment.jumpTableDetails.maxValue-currentSwitchManagment.jumpTableDetails.minValue);
		dualMergeIB(&currentSwitchManagment.ibFinal,&ib_stack_swp_11,&ib_comp_l_int_u);
		insert_IB_load_word(&currentSwitchManagment.ibFinal,0xFFFF);
		dualMergeIB(&currentSwitchManagment.ibFinal,&ib_i_16mul,&ib_stack_dupe_word);
		insert_IB_load_word(&currentSwitchManagment.ibFinal,0xFFFF);
		dualMergeIB(&currentSwitchManagment.ibFinal,&ib_b_xor_int,&ib_stack_swp_11);
		insert_IB_load_word(&currentSwitchManagment.ibFinal,
			currentSwitchManagment.jumpTableDetails.maxValue-currentSwitchManagment.jumpTableDetails.minValue);
		quadMergeIB(&currentSwitchManagment.ibFinal,&ib_b_and_int,&ib_stack_swp_21,&ib_b_and_int,&ib_b_or_int);
		singleMergeIB(&currentSwitchManagment.ibFinal,&ib_int_u_TO_long);
		insert_IB_load_dword(&currentSwitchManagment.ibFinal,4);
		singleMergeIB(&currentSwitchManagment.ibFinal,&ib_i_32mul);
		insert_IB_address_label(&currentSwitchManagment.ibFinal,labelForJJMP);
		singleMergeIB(&currentSwitchManagment.ibFinal,&ib_i_32add);
		
		IS.id=I_POP2;
		IS.arg.B2.a_0=2;
		IS.arg.B2.a_1=3;
		addInstruction(&currentSwitchManagment.ibFinal,IS);
		IS.id=I_JJMP;
		IS.arg.BBD.a_0=2;
		IS.arg.BBD.a_1=3;
		IS.arg.BBD.a_2=labelForJJMP;
		addInstruction(&currentSwitchManagment.ibFinal,IS);
		IS.id=I_JTEN;
		for (uint32_t val=currentSwitchManagment.jumpTableDetails.minValue;val<(currentSwitchManagment.jumpTableDetails.maxValue+1);val++){
			IS.arg.D.a_0=switchGetLabelForValue(val);
			addInstruction(&currentSwitchManagment.ibFinal,IS);
		}
		IS.arg.D.a_0=currentSwitchManagment.defaultLabel;
		addInstruction(&currentSwitchManagment.ibFinal,IS);
		IS.id=I_JEND;
		addInstruction(&currentSwitchManagment.ibFinal,IS);
		break;
		}
		default:
		printf("Internal Error: switch method invalid\n");
		exit(1);
	}
	cosmic_free(currentSwitchManagment.caseLabelPair);
	currentSwitchManagment.caseLabelPair=NULL;
	currentSwitchManagment.caseLabelPairLen=0;
	return false;
	
}




struct LabelNamePair{
	uint32_t label;
	char* name;
};
struct GotoManagment {
	struct LabelNamePair* labelsMade;
	struct LabelNamePair* labelsUsed;
	uint16_t madeLen;
	uint16_t usedLen;
} currentGotoManagment;



void unpackCurrentGotoManagment(struct GotoManagment packedGotoManagment){
	currentGotoManagment=packedGotoManagment;
}

struct GotoManagment packCurrentGotoManagment(){
	struct GotoManagment packedGotoManagment=currentGotoManagment;
	const struct GotoManagment n={0};
	currentGotoManagment=n;
	return packedGotoManagment;
}

// returns the associated label number
// wasGoto refers to if the keyword 'goto' was used
// name is not copied, don't cosmic_free it
uint32_t addGotoOrLabel(char* name,bool wasGoto){
	bool hasSeenMade=false;
	bool hasSeenUsed=false;
	uint32_t label;
	for (uint16_t i=0;i<currentGotoManagment.usedLen;i++){
		if (doStringsMatch(name,currentGotoManagment.labelsUsed[i].name)){
			hasSeenUsed=true;
			label=currentGotoManagment.labelsUsed[i].label;
			break;
		}
	}
	for (uint16_t i=0;i<currentGotoManagment.madeLen;i++){
		if (doStringsMatch(name,currentGotoManagment.labelsMade[i].name)){
			hasSeenMade=true;
			label=currentGotoManagment.labelsMade[i].label;
			break;
		}
	}
	if (!(hasSeenMade|hasSeenUsed)) label=++globalLabelID;
	if ((!hasSeenUsed&wasGoto)|(!hasSeenMade&!wasGoto)){
		struct LabelNamePair* next;
		if (wasGoto){
			currentGotoManagment.labelsUsed=cosmic_realloc(currentGotoManagment.labelsUsed,++currentGotoManagment.usedLen*sizeof(struct LabelNamePair));
			next=currentGotoManagment.labelsUsed+(currentGotoManagment.usedLen-1);
		} else {
			currentGotoManagment.labelsMade=cosmic_realloc(currentGotoManagment.labelsMade,++currentGotoManagment.madeLen*sizeof(struct LabelNamePair));
			next=currentGotoManagment.labelsMade+(currentGotoManagment.madeLen-1);
		}
		next->name=name;
		next->label=label;
	} else {
		cosmic_free(name);
	}
	return label;
}

// ensures all goto labels have a label and cosmic_free()'s memory used by goto managment
void resolveGoto(int32_t failIndex){
	for (uint16_t i0=0;i0<currentGotoManagment.usedLen;i0++){
		bool didFind=false;
		char* thisName=currentGotoManagment.labelsUsed[i0].name;
		for (uint16_t i1=0;i1<currentGotoManagment.madeLen;i1++){
			if (doStringsMatch(thisName,currentGotoManagment.labelsMade[i1].name)){
				didFind=true;
				break;
			}
		}
		if (!didFind){
			char* warnString;
			printInformativeMessageAtSourceContainerIndex(true,warnString=strMerge3("This function used, but did not have, a label \'",thisName,"\'"),failIndex,0);
			exit(1);
		}
	}
	for (uint16_t i0=0;i0<currentGotoManagment.madeLen;i0++){
		bool didFind=false;
		char* thisName=currentGotoManagment.labelsMade[i0].name;
		for (uint16_t i1=0;i1<currentGotoManagment.usedLen;i1++){
			if (doStringsMatch(thisName,currentGotoManagment.labelsUsed[i1].name)){
				didFind=true;
				break;
			}
		}
		if (!didFind){
			char* warnString;
			printInformativeMessageAtSourceContainerIndex(false,warnString=strMerge3("This function has a useless label \'",thisName,"\'"),failIndex,0);
			cosmic_free(warnString);
		}
	}
	for (uint16_t i=0;i<currentGotoManagment.madeLen;i++){
		cosmic_free(currentGotoManagment.labelsMade[i].name);
	}
	for (uint16_t i=0;i<currentGotoManagment.usedLen;i++){
		cosmic_free(currentGotoManagment.labelsUsed[i].name);
	}
	if (currentGotoManagment.labelsMade!=NULL) cosmic_free(currentGotoManagment.labelsMade);
	if (currentGotoManagment.labelsUsed!=NULL) cosmic_free(currentGotoManagment.labelsUsed);
	const struct GotoManagment n={0};
	currentGotoManagment=n;
}



// assumes that almost all other options have been checked
bool isSectionLabel(int32_t start,int32_t end){
	int32_t end2 = emptyIndexAdvance(end);
	return sourceContainer.string[end2]==':' && !isSegmentOfStringTypeLike(sourceContainer.string,start,end);
}































