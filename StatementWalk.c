

//#include "UtilitiesLevel2.c"

#include "TestAssemblyBuilder.c"









// more specifically, findIndexOfTypicalStatementEnd() looks for an unenclosed semicolon and returns the index if it
// this doesn't work on some things such as 'if' and 'for' statements
int32_t findIndexOfTypicalStatementEnd(int32_t indexToStartAt){
	int32_t i=indexToStartAt;
	uint16_t enclosementLevel = 0;
	while (sourceContainer.string[i]){
		char c = sourceContainer.string[i];
		if (c==';' & enclosementLevel==0){
			return i;
		}
		if (c=='(' | c=='[' | c=='{'){
			enclosementLevel++;
		} else if (c==')' | c==']' | c=='}'){
			if (enclosementLevel==0){
				err_11000("Expected ';' prior to this character",i);
				goto UnableToFindEnd;
			}
			enclosementLevel--;
		} else if (c=='\'' | c=='\"'){
			int32_t backup=i;
			i = getEndOfToken(sourceContainer.string,i);
			if (i==-1){
				err_11000("Unable to find the end of this string",backup);
				goto UnableToFindEnd;
			}
			i--;
		}
		i++;
	}
	err_11000("Unexpected source termination",i-1);
	UnableToFindEnd:
	err_1101_("Unable to find the end of this statement",indexToStartAt);
	return 0; // unreachable
}





int32_t advanceToNonNewlineSpace(int32_t index){
	char c;
	int32_t i=index;
	while ((c=sourceContainer.string[i])){
		if (c==' ' | c=='\n'){
			i++;
		} else {
			return i;
		}
	}
	err_1101_("Unexpected source termination after here",index-1);
	return 0; // unreachable
}

// if called with hasRecursed==false, then the value -1 will never be returned
int32_t parenEndWithLiteralSkip(int32_t indexAfterOpenParen,bool hasRecursed){
	int32_t i=indexAfterOpenParen;
	char c;
	while ((c=sourceContainer.string[i])){
		if (c==')'){
			return i;
		} else if (c=='('){
			int32_t b=parenEndWithLiteralSkip(i+1,1);
			i=b+1;
			if (b==-1) break;
		} else if ((c=='\"') | (c=='\'')){
			i=getEndOfToken(sourceContainer.string,i);
		} else {
			i++;
		}
	}
	--indexAfterOpenParen;
	if (hasRecursed) err_11000("This parenthesis does not have a closing match",indexAfterOpenParen);
	else err_1101_("This parenthesis does not have a closing match",indexAfterOpenParen);
	return -1;
}


// expects typeStringOfFunction to be a function, have an identifier, have types broken down, and have arrays decayed.
void addFunctionParemetersAndInternalValuesToBlockFrame(char* typeStringOfFunction){
	
	// add parameters last to first by stack size amounts
	// if function does not return void, then put __FUNCTION_RET_VALUE_PTR into blockframe (this is one word of size)
	// put __FUNCTION_ARG_SIZE into blockframe (this is one word of size)
	// put __FUNCTION_RET_INSTRUCTION_ADDRESS into blockframe (this is two words of size)
	
	char* internalTypeString = applyToTypeStringRemoveIdentifierToNew(typeStringOfFunction);
	if (internalTypeString[0]!='('){
		printf("addFunctionParemetersAndInternalValuesToBlockFrame() got a type string that wasn\'t a function with an identifier\n");
		exit(1);
	}
	int32_t lengthOfInternalTypeString = strlen(internalTypeString);
	int32_t indexOfOtherParenthese = getIndexOfMatchingEnclosement(internalTypeString,0);
	if (indexOfOtherParenthese==-1){
		printf("addFunctionParemetersAndInternalValuesToBlockFrame() got invalid type string\n");
		exit(1);
	}
	bool isReturnValueNonVoid = !doStringsMatch(internalTypeString+(indexOfOtherParenthese+2),"void");
	bool hasArguments = !specificStringEqualCheck(internalTypeString,0,indexOfOtherParenthese+1,"( )") && !specificStringEqualCheck(internalTypeString,0,indexOfOtherParenthese+1,"( void )");
	if (hasArguments){
		char* tempTypeString = cosmic_malloc(lengthOfInternalTypeString);
		int32_t indexOfEndOfThisArgument = indexOfOtherParenthese-1;
		int32_t lengthOfArgument;
		for (int32_t i=indexOfEndOfThisArgument;i>=0;i--){
			char c = internalTypeString[i];
			bool isThisAnArgument = false;
			if (i==0 | c==','){
				isThisAnArgument = true;
			} else if (c==')'){
				i = getIndexOfMatchingEnclosement(internalTypeString,i);
				// that should always succeed, it basically already succeeded above
			}
			if (isThisAnArgument){
				for (int32_t j=0;j<lengthOfInternalTypeString;j++){
					tempTypeString[j]=0;
				}
				int32_t indexOfStartOfThisArgument = i+1;
				lengthOfArgument = indexOfEndOfThisArgument-indexOfStartOfThisArgument;
				for (int32_t j=0;j<lengthOfArgument;j++){
					tempTypeString[j]=internalTypeString[j+indexOfStartOfThisArgument];
				}
				if (tempTypeString[0]==' '){
					tempTypeString[0]=26;
					copyDownForInPlaceEdit(tempTypeString);
				}
				if (!doStringsMatch(tempTypeString,"...")){
					if (!doesThisTypeStringHaveAnIdentifierAtBeginning(tempTypeString)){
						// this should just never happen, the error should have been caught elsewhere
						printf("identifier required for function parameter when adding to block frame\n");
						exit(1);
					}
					// TODO: support register keyword in function parameters. There are several places that would need to be handled at
					addVariableToBlockFrame(tempTypeString,0,false,false);
				} else {
					// is there something to add if it is the variadic argument?
				}
				indexOfEndOfThisArgument = i-1;
			}
		}
		cosmic_free(tempTypeString);
	}
	cosmic_free(internalTypeString);
	if (isReturnValueNonVoid){
		addVariableToBlockFrame("__FUNCTION_RET_VALUE_PTR unsigned int",0,false,false);
	}
	addVariableToBlockFrame("__FUNCTION_ARG_SIZE unsigned int",0,false,false);
	addVariableToBlockFrame("__FUNCTION_CALLER_RET_STACK_ADDRESS unsigned int",0,false,false);
	addVariableToBlockFrame("__FUNCTION_CALLER_FRAME_STACK_ADDRESS unsigned int",0,false,false);
	addVariableToBlockFrame("__FUNCTION_RET_INSTRUCTION_ADDRESS unsigned long",0,false,false);
	
}

void addBlankStaticVariable(const char* typeString){
	struct IdentifierSearchResult isr;
	searchForIdentifier(&isr,typeString,false,true,true,false,false);
	assert(isr.didExist);
	struct GlobalVariableEntry* globalVariableEntry=
		blockFrameArray.globalBlockFrame.globalVariableEntries+
		isr.reference.variableReference.variableEntryIndex;
	struct MemoryOrganizerForInitializer mofi;
	initMemoryOrganizerForInitializer(&mofi,globalVariableEntry->thisSizeof);
	finalizeMemoryOrganizerForInitializer(&mofi,globalVariableEntry->labelID,false);
}

#include "SwitchGotoHelper.c"


/*
some todos

should return the index of the end bracket (unless stopAfterSingleExpression==true, then it returns the index of a bracket or a semicolon)

if labelNumberForBreak==0 then there is nothing to break to
if labelNumberForContinue==0 then there is nothing to continue to
if labelNumberForInlinedReturn==0 then the function being walked is not being inlined

*/
int32_t functionStatementsWalk(
		char* returnTypeString,
		int32_t indexOfStart, // usually, right after bracket
		InstructionBuffer* parentInstructionBufferToInsertTo,
		uint32_t labelNumberForBreak,
		uint32_t labelNumberForContinue,
		uint32_t labelNumberForInlinedReturn,
		bool stopAfterSingleExpression)
{
	int32_t walkingIndex = indexOfStart;
	InstructionBuffer instructionBufferLocalTemp_0;
	InstructionBuffer instructionBufferLocalTemp_1;
	InstructionBuffer instructionBufferLocalTemp_2;
	InstructionBuffer instructionBufferLocalTemp_3;
	InstructionSingle instructionSingleTemp;
	addBlockFrame();
	
	
	char firstCharacter;
	while ((firstCharacter=sourceContainer.string[walkingIndex])){
		if (firstCharacter==' ' | firstCharacter=='\n'){
			walkingIndex++;
			continue;
		}
		
		#ifdef STATEMENT_DEBUG
		printInformativeMessageAtSourceContainerIndex(
						false,"function scope walk starting here:",walkingIndex,0);
		#endif
		
		int32_t endOfToken = getEndOfToken(sourceContainer.string,walkingIndex);
		if (endOfToken==-1) err_1101_("Couldn\'t find end of this token",walkingIndex);
		if (firstCharacter=='}'){
			
			if (stopAfterSingleExpression){
				err_1101_("Expected expression",walkingIndex);
			}
			removeBlockFrame();
			return walkingIndex;
			
		} else if (firstCharacter=='{'){
			
			walkingIndex = functionStatementsWalk(
				returnTypeString,
				walkingIndex+1,
				parentInstructionBufferToInsertTo,
				labelNumberForBreak,
				labelNumberForContinue,
				labelNumberForInlinedReturn,
				false);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"if")){
			
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+2);
			if (sourceContainer.string[walkingIndex]!='('){
				err_1101_("expected \'(\'",walkingIndex);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(walkingIndex,0);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			initInstructionBuffer(&instructionBufferLocalTemp_1);
			expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"_Bool",walkingIndex,otherParenIndex);
			int32_t endOfIfStatement_1 = functionStatementsWalk(
				returnTypeString,
				otherParenIndex+1,
				&instructionBufferLocalTemp_1,
				labelNumberForBreak,
				labelNumberForContinue,
				labelNumberForInlinedReturn,
				true);
			int32_t startOfNextStatement_1 = advanceToNonNewlineSpace(endOfIfStatement_1+1);
			int32_t endOfNextToken = getEndOfToken(sourceContainer.string,startOfNextStatement_1);
			if (specificStringEqualCheck(sourceContainer.string,startOfNextStatement_1,endOfNextToken,"else")){
				initInstructionBuffer(&instructionBufferLocalTemp_2);
				int32_t endOfIfStatement_2 = functionStatementsWalk(
					returnTypeString,
					startOfNextStatement_1+4,
					&instructionBufferLocalTemp_2,
					labelNumberForBreak,
					labelNumberForContinue,
					labelNumberForInlinedReturn,
					true);
				uint32_t tempLabel_0=++globalLabelID;
				uint32_t tempLabel_1=++globalLabelID;
				walkingIndex = endOfIfStatement_2;
				insert_IB_statement_if_else(parentInstructionBufferToInsertTo,&instructionBufferLocalTemp_0,&instructionBufferLocalTemp_1,&instructionBufferLocalTemp_2,tempLabel_0,tempLabel_1);
				destroyInstructionBuffer(&instructionBufferLocalTemp_2);
			} else {
				uint32_t tempLabel_0=++globalLabelID;
				walkingIndex = endOfIfStatement_1;
				insert_IB_statement_if(parentInstructionBufferToInsertTo,&instructionBufferLocalTemp_0,&instructionBufferLocalTemp_1,tempLabel_0);
			}
			destroyInstructionBuffer(&instructionBufferLocalTemp_1);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"else")){
			
			err_1101_("keyword \'else\' encountered without a matching \'if\'",walkingIndex);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"for")){
			
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+3);
			if (sourceContainer.string[walkingIndex]!='('){
				err_1101_("expected \'(\'",walkingIndex);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(walkingIndex,0);
			walkingIndex=advanceToNonNewlineSpace(walkingIndex);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			initInstructionBuffer(&instructionBufferLocalTemp_1);
			initInstructionBuffer(&instructionBufferLocalTemp_2);
			initInstructionBuffer(&instructionBufferLocalTemp_3);
			addBlockFrame();
			if (sourceContainer.string[walkingIndex]!=';'){
				endOfToken = getEndOfToken(sourceContainer.string,walkingIndex);
				if (isSegmentOfStringTypeLike(sourceContainer.string,walkingIndex,endOfToken)){
					// then the first statement is a declaration
					int32_t indexOfEndOfDeclaration = findEndIndexForConvertType(sourceContainer.string,walkingIndex);
					bool usedRegister = false;
					bool usedStatic = false;
					if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+9,"register ")){
						if (usedRegister){
							err_1101_("\'register\' keyword cannot be specified twice",walkingIndex);
						}
						usedRegister = true;
						walkingIndex += 9;
					}
					if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+7,"static ")){
						if (usedStatic){
							err_1101_("\'static\' keyword cannot be specified twice",walkingIndex);
						}
						usedStatic = true;
						walkingIndex += 7;
					}
					if (usedStatic&usedRegister){
						err_1101_("Cannot use 'register' and 'static' at the same time",walkingIndex);
					}
					if (sourceContainer.string[indexOfEndOfDeclaration]=='='){
						int32_t indexOfStartOfInitializer = advanceToNonNewlineSpace(indexOfEndOfDeclaration+1);
						int32_t indexOfEndOfInitializer = findIndexOfTypicalStatementEnd(indexOfStartOfInitializer);
						expressionToAssemblyWithInitializer(&instructionBufferLocalTemp_0,indexOfStartOfInitializer,indexOfEndOfInitializer,walkingIndex,indexOfEndOfDeclaration,usedRegister,usedStatic);
						walkingIndex=indexOfEndOfInitializer;
					} else {
						char* typeString = fullTypeParseAndAdd(walkingIndex,indexOfEndOfDeclaration,false);
						addVariableToBlockFrame(typeString,walkingIndex,usedRegister,usedStatic);
						if (usedStatic) addBlankStaticVariable(typeString);
						cosmic_free(typeString);
						walkingIndex=indexOfEndOfDeclaration;
					}
				} else {
					// then assume the first statement is an expression
					int32_t endingSemicolon = findIndexOfTypicalStatementEnd(walkingIndex);
					expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"void",walkingIndex,endingSemicolon);
					walkingIndex=endingSemicolon;
				}
				walkingIndex++;
			} else if (sourceContainer.string[walkingIndex]==')'){
				err_1101_("Expected \';\' or expression",walkingIndex);
			} else {
				walkingIndex++;
			}
			walkingIndex=advanceToNonNewlineSpace(walkingIndex);
			if (sourceContainer.string[walkingIndex]!=';'){
				int32_t endingSemicolon = findIndexOfTypicalStatementEnd(walkingIndex);
				expressionToAssemblyWithCast(&instructionBufferLocalTemp_1,"_Bool",walkingIndex,endingSemicolon);
				walkingIndex=endingSemicolon+1;
			} else if (sourceContainer.string[walkingIndex]==')'){
				err_1101_("Expected \';\' or expression",walkingIndex);
			} else {
				err_1101_("An expression is required here",walkingIndex);
			}
			walkingIndex=advanceToNonNewlineSpace(walkingIndex);
			if (sourceContainer.string[walkingIndex]!=')'){
				expressionToAssemblyWithCast(&instructionBufferLocalTemp_2,"void",walkingIndex,otherParenIndex);
				walkingIndex=otherParenIndex+1;
			} else if (sourceContainer.string[walkingIndex]==';'){
				err_1101_("Expected \')\' or expression",walkingIndex);
			} else {
				walkingIndex++;
			}
			uint32_t newBreakLabel=++globalLabelID;
			uint32_t newContinueLabel=++globalLabelID;
			uint32_t internalLabel=++globalLabelID;
			walkingIndex = functionStatementsWalk(
				returnTypeString,
				walkingIndex,
				&instructionBufferLocalTemp_3,
				newBreakLabel,
				newContinueLabel,
				labelNumberForInlinedReturn,
				true);
			insert_IB_statement_for(
				parentInstructionBufferToInsertTo,
				&instructionBufferLocalTemp_0,
				&instructionBufferLocalTemp_1,
				&instructionBufferLocalTemp_2,
				&instructionBufferLocalTemp_3,
				newContinueLabel,
				newBreakLabel,
				internalLabel);
			removeBlockFrame();
			destroyInstructionBuffer(&instructionBufferLocalTemp_3);
			destroyInstructionBuffer(&instructionBufferLocalTemp_2);
			destroyInstructionBuffer(&instructionBufferLocalTemp_1);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"break")){
			
			if (labelNumberForBreak==0){
				err_1101_("\'break\' keyword has nothing to break to",walkingIndex);
			}
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!=';'){
				err_1101_("expected \';\'",walkingIndex);
			}
			insert_IB_address_label(parentInstructionBufferToInsertTo,labelNumberForBreak);
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"continue")){
			
			if (labelNumberForContinue==0){
				err_1101_("\'continue\' keyword has nothing to continue to",walkingIndex);
			}
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+8);
			if (sourceContainer.string[walkingIndex]!=';'){
				err_1101_("expected \';\'",walkingIndex);
			}
			insert_IB_address_label(parentInstructionBufferToInsertTo,labelNumberForContinue);
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"while")){
			
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!='('){
				err_1101_("expected \'(\'",walkingIndex);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(walkingIndex,0);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			initInstructionBuffer(&instructionBufferLocalTemp_1);
			expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"_Bool",walkingIndex,otherParenIndex);
			walkingIndex = advanceToNonNewlineSpace(otherParenIndex+1);
			uint32_t newBreakLabel=++globalLabelID;
			uint32_t newContinueLabel=++globalLabelID;
			walkingIndex = functionStatementsWalk(
					returnTypeString,
					walkingIndex,
					&instructionBufferLocalTemp_1,
					newBreakLabel,
					newContinueLabel,
					labelNumberForInlinedReturn,
					true);
			
			insert_IB_statement_while(parentInstructionBufferToInsertTo,&instructionBufferLocalTemp_0,&instructionBufferLocalTemp_1,newContinueLabel,newBreakLabel);
			destroyInstructionBuffer(&instructionBufferLocalTemp_1);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"do")){
			
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+2);
			initInstructionBuffer(&instructionBufferLocalTemp_1);
			uint32_t internalLabel=++globalLabelID;
			uint32_t newBreakLabel=++globalLabelID;
			uint32_t newContinueLabel=++globalLabelID;
			walkingIndex = functionStatementsWalk(
					returnTypeString,
					walkingIndex,
					&instructionBufferLocalTemp_1,
					newBreakLabel,
					newContinueLabel,
					labelNumberForInlinedReturn,
					true);
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+1);
			int32_t endOfNextToken = getEndOfToken(sourceContainer.string,walkingIndex);
			if (!specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfNextToken,"while")){
				err_1101_("expected \'while\'",walkingIndex);
			}
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!='('){
				err_1101_("expected \'(\'",walkingIndex);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(walkingIndex,0);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"_Bool",walkingIndex,otherParenIndex);
			walkingIndex = advanceToNonNewlineSpace(otherParenIndex+1);
			if (sourceContainer.string[walkingIndex]!=';'){
				err_1101_("expected \';\'",walkingIndex);
			}
			insert_IB_statement_do_while(parentInstructionBufferToInsertTo,&instructionBufferLocalTemp_0,&instructionBufferLocalTemp_1,internalLabel,newBreakLabel,newContinueLabel);
			destroyInstructionBuffer(&instructionBufferLocalTemp_1);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"return")){
			
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+6);
			if (doStringsMatch(returnTypeString,"void")){
				if (sourceContainer.string[walkingIndex]!=';'){
					err_1101_("expected \';\'",walkingIndex);
				}
			} else {
				if (sourceContainer.string[walkingIndex]==';'){
					err_1101_("expected expression here",walkingIndex);
				}
				if (labelNumberForInlinedReturn!=0){
					// TODO
					printf("inlined functions are not ready yet\n");
					exit(1);
					/*
					Note for when I do implement this:
					the global buffer for expressions will be overritten when the function walk occurs, so
					use packExpressionTreeGlobalBuffer() and unpackExpressionTreeGlobalBuffer() here
					for the expression 
					*/
				} else {
					int32_t endingSemicolon = findIndexOfTypicalStatementEnd(walkingIndex);
					expressionToAssemblyWithReturn(parentInstructionBufferToInsertTo,walkingIndex,endingSemicolon,returnTypeString);
					walkingIndex=endingSemicolon;
				}
			}
			if (labelNumberForInlinedReturn!=0){
				printf("inlined functions are not ready yet\n");
				exit(1);
				// this part of inlined function walks is NOT TODO, it's done
				insert_IB_address_label(parentInstructionBufferToInsertTo,labelNumberForInlinedReturn);
				singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
			} else {
				InstructionSingle IS_temp;
				IS_temp.id=I_RET_;
				addInstruction(parentInstructionBufferToInsertTo,IS_temp);
			}
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"switch")){
			
			struct SwitchManagment packedSwitchManagment = packCurrentSwitchManagment();
			currentSwitchManagment.inSwitch=true;
			walkingIndex = advanceToNonNewlineSpace(walkingIndex+6);
			if (sourceContainer.string[walkingIndex]!='('){
				err_1101_("expected \'(\'",walkingIndex);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(walkingIndex,0);
			int32_t startBracketIndex = advanceToNonNewlineSpace(otherParenIndex+1);
			if (sourceContainer.string[startBracketIndex]!='{'){
				err_1101_("expected \'{\'",walkingIndex);
			}
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			initInstructionBuffer(&currentSwitchManagment.ibSwitchItem);
			addBlockFrame();
			expressionToAssemblyWithCast(&currentSwitchManagment.ibSwitchItem,"unsigned int",walkingIndex,otherParenIndex);
			uint32_t newBreakLabel=++globalLabelID;
			walkingIndex = functionStatementsWalk(
				returnTypeString,
				startBracketIndex+1,
				&instructionBufferLocalTemp_0,
				newBreakLabel,
				labelNumberForContinue,
				labelNumberForInlinedReturn,
				false
			);
			if (resolveSwitch()){
				err_1101_("At least one case is required per switch statement",walkingIndex);
			}
			dualMergeIB(parentInstructionBufferToInsertTo,&currentSwitchManagment.ibFinal,&instructionBufferLocalTemp_0);
			insert_IB_raw_label(parentInstructionBufferToInsertTo,newBreakLabel);
			if (!currentSwitchManagment.hasDefault) insert_IB_raw_label(parentInstructionBufferToInsertTo,currentSwitchManagment.defaultLabel);
			removeBlockFrame();
			destroyInstructionBuffer(&currentSwitchManagment.ibFinal);
			destroyInstructionBuffer(&currentSwitchManagment.ibSwitchItem);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
			unpackCurrentSwitchManagment(packedSwitchManagment);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"case")){
			
			if (!currentSwitchManagment.inSwitch){
				err_1101_("keyword \'case\' must be inside a switch",walkingIndex);
			}
			int32_t tempWalkingIndex0=advanceToNonNewlineSpace(walkingIndex+4);
			int32_t tempWalkingIndex1=tempWalkingIndex0;
			{
				char c;
				while ((c=sourceContainer.string[tempWalkingIndex1]),(c!=':' & c!=0)){
					if (c=='?')  err_1101_("ternaries are not allowed in case constant-expression",tempWalkingIndex1);
					if (c=='\"') err_1101_("string literals are not allowed in case constant-expression",tempWalkingIndex1);
					if (c=='\''){
while (true){
	c=sourceContainer.string[++tempWalkingIndex1];
	if (c=='\'') break;
	// this next if statement depends on short circuiting || and &&
	if (c==0 || (c=='\\' && sourceContainer.string[++tempWalkingIndex1]==0)) err_1101_("Unexpected source termination",tempWalkingIndex1-1);
}
					}
					tempWalkingIndex1++;
				}
			}
			if (sourceContainer.string[tempWalkingIndex1]!=':'){
				err_1101_("expected \':\' after keyword \'case\'",tempWalkingIndex0);
			}
			int16_t expRoot=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,tempWalkingIndex0,tempWalkingIndex1,true);
			if (expRoot==-1){
				err_1101_("expected const-expression after keyword \'case\'",tempWalkingIndex0);
			}
			walkingIndex=tempWalkingIndex1;
			insert_IB_raw_label(
				parentInstructionBufferToInsertTo,
				insertSwitchCase(
					tempWalkingIndex0,
					expressionToConstantValue(
						"unsigned int",
						expRoot
						)
					)
				);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"default")){
			
			if (!currentSwitchManagment.inSwitch){
				err_1101_("keyword \'default\' must be inside a switch",walkingIndex);
			}
			int32_t tempWalkingIndex=advanceToNonNewlineSpace(walkingIndex+7);
			if (sourceContainer.string[tempWalkingIndex]!=':'){
				err_1101_("expected \':\' after keyword \'default\'",tempWalkingIndex);
			}
			if (currentSwitchManagment.hasDefault){
				err_1101_("There cannot be two \'default\' for a switch",walkingIndex);
			}
			currentSwitchManagment.hasDefault=true;
			walkingIndex=tempWalkingIndex;
			insert_IB_raw_label(
				parentInstructionBufferToInsertTo,
				currentSwitchManagment.defaultLabel=++globalLabelID
				);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"typedef")){
			
			err_1101_("typedef inside functions is not ready yet",walkingIndex);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"goto")){
			
			int32_t labelStart=advanceToNonNewlineSpace(endOfToken);
			int32_t labelEnd=getEndOfToken(sourceContainer.string,labelStart);
			int32_t semiColon=advanceToNonNewlineSpace(labelEnd);
			if (sourceContainer.string[semiColon]!=';'){
				err_1101_("Expected \';\' after goto statement",semiColon);
			}
			walkingIndex=semiColon;
			insert_IB_address_label(parentInstructionBufferToInsertTo,addGotoOrLabel(copyStringSegmentToHeap(sourceContainer.string,labelStart,labelEnd),true));
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
			
		} else if (isSectionLabel(walkingIndex,endOfToken)){
			
			int32_t labelStart=walkingIndex;
			int32_t labelEnd=endOfToken;
			int32_t colon=advanceToNonNewlineSpace(endOfToken);
			walkingIndex=labelEnd;
			insert_IB_raw_label(parentInstructionBufferToInsertTo,addGotoOrLabel(copyStringSegmentToHeap(sourceContainer.string,labelStart,labelEnd),false));
			
		} else {
			
			if (isSegmentOfStringTypeLike(sourceContainer.string,walkingIndex,endOfToken)){
				// then it is a declaration
				int32_t indexOfEndOfDeclaration = findEndIndexForConvertType(sourceContainer.string,walkingIndex);
				bool usedRegister = false;
				bool usedStatic = false;
				if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+9,"register ")){
					if (usedRegister){
						err_1101_("\'register\' keyword cannot be specified twice",walkingIndex);
					}
					usedRegister = true;
					walkingIndex += 9;
				}
				if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+7,"static ")){
					if (usedStatic){
						err_1101_("\'static\' keyword cannot be specified twice",walkingIndex);
					}
					usedStatic = true;
					walkingIndex += 7;
				}
				if (usedStatic&usedRegister){
					err_1101_("Cannot use 'register' and 'static' at the same time",walkingIndex);
				}
				if (sourceContainer.string[indexOfEndOfDeclaration]=='='){
					int32_t indexOfStartOfInitializer = advanceToNonNewlineSpace(indexOfEndOfDeclaration+1);
					int32_t indexOfEndOfInitializer = findIndexOfTypicalStatementEnd(indexOfStartOfInitializer);
					expressionToAssemblyWithInitializer(parentInstructionBufferToInsertTo,indexOfStartOfInitializer,indexOfEndOfInitializer,walkingIndex,indexOfEndOfDeclaration,usedRegister,usedStatic);
					walkingIndex=indexOfEndOfInitializer;
				} else {
					char* typeString = fullTypeParseAndAdd(walkingIndex,indexOfEndOfDeclaration,false);
					addVariableToBlockFrame(typeString,walkingIndex,usedRegister,usedStatic);
					if (usedStatic) addBlankStaticVariable(typeString);
					cosmic_free(typeString);
					walkingIndex=indexOfEndOfDeclaration;
				}
			} else {
				// then assume it is an expression
				int32_t endingSemicolon = findIndexOfTypicalStatementEnd(walkingIndex);
				expressionToAssemblyWithCast(parentInstructionBufferToInsertTo,"void",walkingIndex,endingSemicolon);
				walkingIndex=endingSemicolon;
			}
			
		}
		if (stopAfterSingleExpression){
			removeBlockFrame();
			return walkingIndex;
		}
		walkingIndex++;
	}
	err_1101_("Unexpected source termination which parsing function at this segment",indexOfStart);
	return 0; // unreachable
}












// this function runs the parser and compiler, and starts at the file scope
void fileScopeStatementsWalk(){
	int32_t walkingIndex = 0;
	char firstCharacter;
	initInstructionBuffer(&global_static_data);
	while ((firstCharacter=sourceContainer.string[walkingIndex])){
		if (firstCharacter==' ' | firstCharacter=='\n'){
			walkingIndex++;
			continue;
		}
		#ifdef STATEMENT_DEBUG
		printInformativeMessageAtSourceContainerIndex(
						false,"file scope walk starting here:",walkingIndex,0);
		#endif
		int32_t endOfToken = getEndOfToken(sourceContainer.string,walkingIndex);
		if (endOfToken==-1){
			err_1101_("Couldn\'t find end of this token",walkingIndex);
		}
		if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"typedef")){
			// then this is a typedef entry
			int32_t startIndexForDeclaration = walkingIndex+8;
			int32_t endIndexForDeclaration = findEndIndexForConvertType(sourceContainer.string,startIndexForDeclaration);
			char* typeString = fullTypeParseAndAdd(startIndexForDeclaration,endIndexForDeclaration,true);
			addTypedefEntry(typeString);
			cosmic_free(typeString);
			walkingIndex=1+endIndexForDeclaration;
			
		} else if (isSegmentOfStringTypeLike(sourceContainer.string,walkingIndex,endOfToken)){
			// then it is some sort of declaration
			if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"auto")){
				err_1101_("keyword \'auto\' not allowed at file scope",walkingIndex);
			}
			if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"register")){
				err_1101_("keyword \'register\' not allowed at file scope",walkingIndex);
			}
			bool hasStatic=false;
			bool hasExtern=false;
			bool hasInline=false;
			int32_t startIndexForDeclaration = walkingIndex;
			{
				uint8_t staticCount=0;
				uint8_t externCount=0;
				uint8_t inlineCount=0;
				bool checkForStorageClassSpecifier;
				while (true){
					checkForStorageClassSpecifier = false;
					if (specificStringEqualCheck(sourceContainer.string,startIndexForDeclaration,endOfToken,"static")){
						checkForStorageClassSpecifier=true;
						staticCount++;
					} else if (specificStringEqualCheck(sourceContainer.string,startIndexForDeclaration,endOfToken,"extern")){
						checkForStorageClassSpecifier=true;
						externCount++;
					} else if (specificStringEqualCheck(sourceContainer.string,startIndexForDeclaration,endOfToken,"inline")){
						checkForStorageClassSpecifier=true;
						inlineCount++;
					}
					if (checkForStorageClassSpecifier){
						startIndexForDeclaration+=7;
						endOfToken=getEndOfToken(sourceContainer.string,startIndexForDeclaration);
						if (endOfToken==-1){
							err_1101_("Couldn\'t find end of this token",startIndexForDeclaration);
						}
						continue;
					} else {
						break;
					}
				}
				if (staticCount>1 | externCount>1 | (staticCount!=0 & externCount!=0)){
					err_1101_("storage class specifier list is invalid for this declaration",startIndexForDeclaration);
				}
				hasStatic=staticCount!=0;
				hasExtern=externCount!=0;
				hasInline=inlineCount!=0;
			}
			int32_t endIndexForDeclaration = findEndIndexForConvertType(sourceContainer.string,startIndexForDeclaration);
			char* typeString = fullTypeParseAndAdd(startIndexForDeclaration,endIndexForDeclaration,true);
			int32_t gotoFailIndex=startIndexForDeclaration;
			walkingIndex=1+endIndexForDeclaration;
			if (doesThisTypeStringHaveAnIdentifierAtBeginning(typeString)){
				int32_t indexOfFirstSpace = getIndexOfFirstSpaceInString(typeString);
				if (typeString[indexOfFirstSpace+1]=='('){
					// then this is a function declaration or definition
					bool isDefinitionBeingGiven;
					if (sourceContainer.string[endIndexForDeclaration]=='{'){
						isDefinitionBeingGiven=true;
					} else if (sourceContainer.string[endIndexForDeclaration]==';'){
						isDefinitionBeingGiven=false;
					} else {
						printf("Internal Error: It seems that a function was being declared at file scope, but something went wrong when trying to figure out if it was being defined\n");
						exit(1);
					}
					applyToTypeStringArrayDecayToSelf(typeString);
					uint8_t retValForAddingFunction = addGlobalFunction(typeString,gotoFailIndex,isDefinitionBeingGiven,hasStatic,hasExtern,hasInline);
					if (retValForAddingFunction!=0){
						if (retValForAddingFunction==1){
							err_1101_("function declaration has type conflicts with previous declaration",gotoFailIndex);
						} else if (retValForAddingFunction==2){
							err_1101_("function was already defined",gotoFailIndex);
						} else if (retValForAddingFunction==3){
							err_1101_("that identifier is already used for a global variable, it cannot be used for a function",gotoFailIndex);
						} else if (retValForAddingFunction==4){
							err_1101_("function definitions must have identifiers for all parameters",gotoFailIndex);
						} else if (retValForAddingFunction==4){
							err_1101_("functions declared 'extern' cannot be given a definition",gotoFailIndex);
						}
						assert(false);
					}
					if (isDefinitionBeingGiven){
						InstructionBuffer instructionBufferForFunction;
						InstructionSingle instructionSingle;
						struct IdentifierSearchResult identifierSearchResult;
						searchForIdentifier(&identifierSearchResult,typeString,true,false,false,false,true);
						assert(identifierSearchResult.didExist);
						assert(blockFrameArray.numberOfValidSlots==0);
						addBlockFrame();
						initInstructionBuffer(&instructionBufferForFunction);
						instructionSingle.id = I_FCST;
						instructionSingle.arg.BWD.a_0=0;
						instructionSingle.arg.BWD.a_1=0;
						instructionSingle.arg.BWD.a_2 = blockFrameArray.globalBlockFrame.globalFunctionEntries[identifierSearchResult.reference.functionReference.functionEntryIndex].labelID;
						addInstruction(&instructionBufferForFunction,instructionSingle);
						instructionSingle.id = I_ALOC;
						addInstruction(&instructionBufferForFunction,instructionSingle);
						
						addFunctionParemetersAndInternalValuesToBlockFrame(typeString);
						//printf("Starting Function Statement Walk\n");
						
						int32_t tempIndex = getIndexOfNthSpace(typeString,0);
						char* returnTypeString = (getIndexOfMatchingEnclosement(typeString,tempIndex+1)+2)+typeString;
						walkingIndex = functionStatementsWalk(
							returnTypeString,
							endIndexForDeclaration,
							&instructionBufferForFunction,
							0,0,0,true
							);
						
						//printf("Finished Function Walk\n");
						removeBlockFrame();
						assert(blockFrameArray.numberOfValidSlots==0);
						instructionSingle.id = I_RET_; // ensure implicit return exists at the end
						addInstruction(&instructionBufferForFunction,instructionSingle);
						instructionSingle.id = I_FCEN;
						addInstruction(&instructionBufferForFunction,instructionSingle);
						
						assert(instructionBufferForFunction.buffer[0].id==I_FCST);
						instructionBufferForFunction.buffer[0].arg.BWD.a_0=blockFrameArray.initialStackSize;
						instructionBufferForFunction.buffer[0].arg.BWD.a_1=blockFrameArray.maxStackSize;
						
						resolveGoto(gotoFailIndex);
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("%s\n",typeString);
#endif
						runOptimizerOnFunctionPriorToGlobalIntegration(&instructionBufferForFunction);
#ifdef OPT_DEBUG_GENERAL_ACTIVE
printf("-----\n\n");
#endif
#ifdef PRINT_EACH_FUNCTION
printInstructionBufferWithMessageAndNumber(&instructionBufferForFunction,typeString,instructionBufferForFunction.numberOfSlotsTaken);
#endif

						addEntryToInstructionBuffersOfFunctions(&instructionBufferForFunction);
						
						walkingIndex++;
					}
				} else {
					// then this isn't a function declaration, so it must be a variable declaration
					if (hasInline){
						err_1101_("Global variables cannot use inline keyword",gotoFailIndex);
					}
					bool isInitializationBeingGiven;
					if (sourceContainer.string[endIndexForDeclaration]=='='){
						isInitializationBeingGiven = true;
					} else if (sourceContainer.string[endIndexForDeclaration]==';'){
						isInitializationBeingGiven = false;
					} else {
						err_1101_("It seems that a variable was being declared at file scope,\n  but something went wrong when trying to figure out if it had an initalizer",gotoFailIndex);
					}
					int32_t indexOfSemicolon = endIndexForDeclaration;
					uint8_t retValForAddingVariable;
					uint32_t labelID=++globalLabelID;
					if (isInitializationBeingGiven){
						indexOfSemicolon = findIndexOfTypicalStatementEnd(endIndexForDeclaration);
						walkingIndex = indexOfSemicolon+1;
						int32_t startOfInitializer = endIndexForDeclaration+1;
						char c = sourceContainer.string[startOfInitializer];
						while (c==' ' | c=='\n'){
							c = sourceContainer.string[++startOfInitializer];
						}
						char* typeStringNI = applyToTypeStringRemoveIdentifierToNew(typeString);
						char* typeStringIdentifier = applyToTypeStringGetIdentifierToNew(typeString);
						cosmic_free(typeString);
						int32_t initRoot = initializerMapRoot(startOfInitializer,indexOfSemicolon);
						if (!initializerImplementRoot(&global_static_data,initRoot,labelID,&typeStringNI,true)){
							assert(false);
						}
						typeString=strMerge3(typeStringIdentifier," ",typeStringNI);
						cosmic_free(typeStringIdentifier);
						cosmic_free(typeStringNI);
						retValForAddingVariable = addGlobalVariable(typeString,0,labelID,gotoFailIndex,hasStatic,hasExtern,true);
					} else {
						retValForAddingVariable = addGlobalVariable(typeString,0,labelID,gotoFailIndex,hasStatic,hasExtern,false);
					}
					if (retValForAddingVariable==1){
						err_1101_("identifier collision at file scope for variable, a variable was already declared with the same identifier",gotoFailIndex);
					} else if (retValForAddingVariable==2){
						err_1101_("identifier collision at file scope for variable, a function was already declared with the same identifier",gotoFailIndex);
					}
				}
			}
			cosmic_free(typeString);
		} else {
			// technically, it is possible for an initalizer without a declaration to occur. We should add a check for something like a global variable being the first token
			err_1111_("Unknown starting token at file scope",walkingIndex,endOfToken);
		}
	}
	//printInstructionBufferWithMessageAndNumber(&global_static_data,"global static data printout",0);
}



