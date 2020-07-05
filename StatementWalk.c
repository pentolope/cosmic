

//#include "UtilitiesLevel2.c"

#include "TestAssemblyBuilder.c"









// more specifically, findIndexOfTypicalStatementEnd() looks for an unenclosed semicolon and returns the index if it
// if findIndexOfTypicalStatementEnd() fails in some way it returns -1
// this doesn't work on some things such as 'if' and 'for' statements
int32_t findIndexOfTypicalStatementEnd(char* string, int32_t indexToStartAt){
	int32_t i=indexToStartAt;
	uint16_t enclosementLevel = 0;
	while (string[i]){
		char c = string[i];
		if ((c==';') & (enclosementLevel==0)){
			return i;
		}
		if ((c=='(') | (c=='[') | (c=='{')){
			enclosementLevel++;
		} else if ((c==')') | (c==']') | (c=='}')){
			if (enclosementLevel==0){
				printf("BAD TERMINATE (0.0)\n");
				return -1;
			}
			enclosementLevel--;
		} else if ((c=='\'') | (c=='\"')){
			i = getEndOfToken(string,i);
			if (i==-1){
				printf("BAD TERMINATE (0.1)\n");
				return -1;
			}
			i--;
		}
		i++;
	}
	printf("BAD TERMINATE (0.2)\n");
	return -1;
}


// returns -1 if it fails
int32_t findNextUnenclosedCommaOrEndBracket(char* string, int32_t indexToStartAt){
	int32_t i=indexToStartAt;
	uint16_t enclosementLevel = 0;
	while (string[i]){
		char c = string[i];
		if (((c==',') | (c=='}')) & (enclosementLevel==0)){
			return i;
		}
		if ((c=='(') | (c=='[') | (c=='{')){
			enclosementLevel++;
		} else if ((c==')') | (c==']') | (c=='}')){
			if (enclosementLevel==0){
				printf("BAD TERMINATE (1.0)\n");
				return -1;
			}
			enclosementLevel--;
		} else if ((c=='\'') | (c=='\"')){
			i = getEndOfToken(string,i);
			if (i==-1){
				printf("BAD TERMINATE (1.1)\n");
				return -1;
			}
		}
	}
	printf("BAD TERMINATE (1.2)\n");
	return -1;
}


// if string terminates, returns -1
int32_t advanceToNonNewlineSpace(char* string, int32_t index){
	char c;
	int32_t i=index;
	while ((c=string[i])){
		if ((c==' ') | (c=='\n')){
			i++;
		} else {
			return i;
		}
	}
	printf("BAD TERMINATE (2)\n");
	return -1;
}


// if string terminates, returns -1
int32_t parenEndWithLiteralSkip(char* string, int32_t indexAfterOpenParen){
	int32_t i=indexAfterOpenParen;
	char c;
	while ((c=string[i])){
		if (c==')'){
			return i;
		} else if (c=='('){
			i=parenEndWithLiteralSkip(string,i+1)+1;
		} else if ((c=='\"') | (c=='\'')){
			i=getEndOfToken(string,i);
		} else {
			i++;
		}
	}
	printf("BAD TERMINATE (3)\n");
	exit(1);
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
			if ((i==0) | (c==',')){
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
					if (addVariableToBlockFrame(tempTypeString,false,false)){
						printf("parameter name conflict for function\n");
						exit(1);
					}
				} else {
					// is there something to add if it is the variadic argument?
				}
				indexOfEndOfThisArgument = i-1;
			}
		}
		cosmic_free(tempTypeString);
	}
	cosmic_free(internalTypeString);
	bool didAddingAllInternalParametersNotSucceed = false;
	if (isReturnValueNonVoid){
		didAddingAllInternalParametersNotSucceed = addVariableToBlockFrame("__FUNCTION_RET_VALUE_PTR unsigned int",false,false);
	}
	didAddingAllInternalParametersNotSucceed |= addVariableToBlockFrame("__FUNCTION_ARG_SIZE unsigned int",false,false);
	didAddingAllInternalParametersNotSucceed |= addVariableToBlockFrame("__FUNCTION_CALLER_RET_STACK_ADDRESS unsigned int",false,false);
	didAddingAllInternalParametersNotSucceed |= addVariableToBlockFrame("__FUNCTION_CALLER_FRAME_STACK_ADDRESS unsigned int",false,false);
	didAddingAllInternalParametersNotSucceed |= addVariableToBlockFrame("__FUNCTION_RET_INSTRUCTION_ADDRESS unsigned long",false,false);
	
	if (didAddingAllInternalParametersNotSucceed){
		printf("There was a parameter to that function with a reserved identifier\n");
		exit(1);
	}
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
		
		
		
		
		// this is debug for valgrind
		if (++statementCount>=statementCountCutoff & isStatementCutoffActive){
			printf("Debug: terminating due to statement count cutoff\n");
			exit(0);
		}
		#ifdef STATEMENT_DEBUG
		printInformativeMessageAtSourceContainerIndex(
						false,"function scope walk starting here:",walkingIndex,0);
		#endif
		
		
		int32_t endOfToken = getEndOfToken(sourceContainer.string,walkingIndex);
		if (endOfToken==-1){
			printf("token splitter failed in functionStatementsWalk()\n");
			exit(1);
		}
		if (firstCharacter=='}'){
			if (stopAfterSingleExpression){
				printf("Where is the start bracket for this end bracket?\n");
				exit(1);
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
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+2);
			if (sourceContainer.string[walkingIndex]!='('){
				printf("expected \'(\'\n");
				exit(1);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(sourceContainer.string,walkingIndex);
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
			int32_t startOfNextStatement_1 = advanceToNonNewlineSpace(sourceContainer.string,endOfIfStatement_1+1);
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
			printf("keyword \'else\' encountered without a matching \'if\'\n");
			exit(1);
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"for")){
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+3);
			if (sourceContainer.string[walkingIndex]!='('){
				printf("expected \'(\'\n");
				exit(1);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(sourceContainer.string,walkingIndex);
			walkingIndex=advanceToNonNewlineSpace(sourceContainer.string,walkingIndex);
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
							printf("\'register\' keyword cannot be specified twice\n");
							exit(1);
						}
						usedRegister = true;
						walkingIndex += 9;
					}
					if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+7,"static ")){
						if (usedStatic){
							printf("\'static\' keyword cannot be specified twice\n");
							exit(1);
						}
						usedStatic = true;
						walkingIndex += 7;
					}
					if (usedStatic){
						printf("static not quite ready in functions\n");
						exit(1);
					}
					if (sourceContainer.string[indexOfEndOfDeclaration]=='='){
						int32_t indexOfStartOfInitializer = advanceToNonNewlineSpace(sourceContainer.string,indexOfEndOfDeclaration+1);
						int32_t indexOfEndOfInitializer = findIndexOfTypicalStatementEnd(sourceContainer.string,indexOfStartOfInitializer);
						if (indexOfStartOfInitializer==-1 | indexOfEndOfInitializer==-1){
							printf("Could not find end to statement with declaration\n");
							exit(1);
						}
						expressionToAssemblyWithInitializer(&instructionBufferLocalTemp_0,indexOfStartOfInitializer,indexOfEndOfInitializer,walkingIndex,indexOfEndOfDeclaration,usedRegister,usedStatic);
						walkingIndex=indexOfEndOfInitializer;
					} else {
						char* typeString = fullTypeParseAndAdd(walkingIndex,indexOfEndOfDeclaration,false);
						addVariableToBlockFrame(typeString,usedRegister,usedStatic);
						cosmic_free(typeString);
						walkingIndex=indexOfEndOfDeclaration;
					}
				} else {
					// then assume the first statement is an expression
					int32_t endingSemicolon = findIndexOfTypicalStatementEnd(sourceContainer.string,walkingIndex);
					if (walkingIndex!=endingSemicolon){
						expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"void",walkingIndex,endingSemicolon);
					}
					walkingIndex=endingSemicolon;
				}
				walkingIndex++;
			} else if (sourceContainer.string[walkingIndex]==')'){
				printf("Expected \';\' or expression\n");
				exit(1);
			} else {
				walkingIndex++;
			}
			walkingIndex=advanceToNonNewlineSpace(sourceContainer.string,walkingIndex);
			if (sourceContainer.string[walkingIndex]!=';'){
				int32_t endingSemicolon = findIndexOfTypicalStatementEnd(sourceContainer.string,walkingIndex);
				expressionToAssemblyWithCast(&instructionBufferLocalTemp_1,"void",walkingIndex,endingSemicolon);
				walkingIndex=endingSemicolon+1;
			} else if (sourceContainer.string[walkingIndex]==')'){
				printf("Expected \';\' or expression\n");
				exit(1);
			} else {
				printf("Expression Required\n");
				exit(1);
			}
			walkingIndex=advanceToNonNewlineSpace(sourceContainer.string,walkingIndex);
			if (sourceContainer.string[walkingIndex]!=')'){
				expressionToAssemblyWithCast(&instructionBufferLocalTemp_2,"void",walkingIndex,otherParenIndex);
				walkingIndex=otherParenIndex+1;
			} else if (sourceContainer.string[walkingIndex]==';'){
				printf("Expected \')\' or expression\n");
				exit(1);
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
				printf("\'break\' keyword has nothing to break to\n");
				exit(1);
			}
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!=';'){
				printf("expected \';\'\n");
				exit(1);
			}
			insert_IB_address_label(parentInstructionBufferToInsertTo,labelNumberForBreak);
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"continue")){
			if (labelNumberForContinue==0){
				printf("\'continue\' keyword has nothing to continue to\n");
				exit(1);
			}
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+8);
			if (sourceContainer.string[walkingIndex]!=';'){
				printf("expected \';\'\n");
				exit(1);
			}
			insert_IB_address_label(parentInstructionBufferToInsertTo,labelNumberForContinue);
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"while")){
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!='('){
				printf("expected \'(\'\n");
				exit(1);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(sourceContainer.string,walkingIndex);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
			initInstructionBuffer(&instructionBufferLocalTemp_1);
			expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"_Bool",walkingIndex,otherParenIndex);
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,otherParenIndex+1);
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
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+2);
			initInstructionBuffer(&instructionBufferLocalTemp_0);
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
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+1);
			int32_t endOfNextToken = getEndOfToken(sourceContainer.string,walkingIndex);
			if (!specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfNextToken,"while")){
				printf("expected \'while\'\n");
				exit(1);
			}
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+5);
			if (sourceContainer.string[walkingIndex]!='('){
				printf("expected \'(\'\n");
				exit(1);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(sourceContainer.string,walkingIndex);
			expressionToAssemblyWithCast(&instructionBufferLocalTemp_0,"_Bool",walkingIndex,otherParenIndex);
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,otherParenIndex+1);
			if (sourceContainer.string[walkingIndex]!=';'){
				printf("expected \';\'\n");
				exit(1);
			}
			insert_IB_statement_do_while(parentInstructionBufferToInsertTo,&instructionBufferLocalTemp_0,&instructionBufferLocalTemp_1,internalLabel,newBreakLabel,newContinueLabel);
			destroyInstructionBuffer(&instructionBufferLocalTemp_1);
			destroyInstructionBuffer(&instructionBufferLocalTemp_0);
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"return")){
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+6);
			if (doStringsMatch(returnTypeString,"void")){
				if (sourceContainer.string[walkingIndex]!=';'){
					printf("expected \';\'\n");
					exit(1);
				}
			} else {
				if (sourceContainer.string[walkingIndex]==';'){
					printf("expected expression\n");
					exit(1);
				}
				if (labelNumberForInlinedReturn!=0){
					// TODO
					/*
					Note for when I do implement this:
					the global buffer for expressions will be overritten when the function walk occurs 
					so it will need to be copied, both for a backup so the current expression can continue and to replace it back
					*/
					printf("inlined functions are not ready yet\n");
					exit(1);
				} else {
					int32_t endingSemicolon = findIndexOfTypicalStatementEnd(sourceContainer.string,walkingIndex);
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
				instructionSingleTemp.id=I_RET_;
				addInstruction(parentInstructionBufferToInsertTo,instructionSingleTemp);
			}
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"switch")){
			struct SwitchManagment packedSwitchManagment = packCurrentSwitchManagment();
			currentSwitchManagment.inSwitch=true;
			walkingIndex = advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+6);
			if (sourceContainer.string[walkingIndex]!='('){
				printf("expected \'(\'\n");
				exit(1);
			}
			walkingIndex++;
			int32_t otherParenIndex = parenEndWithLiteralSkip(sourceContainer.string,walkingIndex);
			int32_t startBracketIndex = advanceToNonNewlineSpace(sourceContainer.string,otherParenIndex+1);
			if (sourceContainer.string[startBracketIndex]!='{'){
				printf("expected \'{\'\n");
				exit(1);
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
				printf("At least one case is required per switch statement\n");
				exit(1);
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
				printf("keyword \'case\' must be inside a switch\n");
				exit(1);
			}
			int32_t tempWalkingIndex0=advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+4);
			int32_t tempWalkingIndex1=tempWalkingIndex0;
			{
				char c;
				while ((c=sourceContainer.string[tempWalkingIndex1]),(c!=':' & c!=0)){
					if (c=='?'){
						printf("ternaries are not allowed in case constant-expression\n");
						exit(1);
					}
					if (c=='\"'){
						printf("string literals are not allowed in case constant-expression\n");
						exit(1);
					}
					tempWalkingIndex1++;
				}
			}
			if (sourceContainer.string[tempWalkingIndex1]!=':'){
				printf("expected \':\' after keyword \'case\'\n");
				exit(1);
			}
			int16_t expRoot=buildExpressionTreeFromSubstringToGlobalBufferAndReturnRootIndex(sourceContainer.string,tempWalkingIndex0,tempWalkingIndex1,true);
			if (expRoot==-1){
				printf("expected const-expression after keyword \'case\'\n");
				exit(1);
			}
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
			walkingIndex=tempWalkingIndex1;
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"default")){
			if (!currentSwitchManagment.inSwitch){
				printf("keyword \'default\' must be inside a switch\n");
				exit(1);
			}
			int32_t tempWalkingIndex=advanceToNonNewlineSpace(sourceContainer.string,walkingIndex+7);
			if (sourceContainer.string[tempWalkingIndex]!=':'){
				printf("expected \':\' after keyword \'default\'\n");
				exit(1);
			}
			if (currentSwitchManagment.hasDefault){
				printf("There cannot be two \'default\' for a switch\n");
				exit(1);
			}
			currentSwitchManagment.hasDefault=true;
			insert_IB_raw_label(
				parentInstructionBufferToInsertTo,
				currentSwitchManagment.defaultLabel=++globalLabelID
				);
			walkingIndex=tempWalkingIndex;
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"typedef")){
			
			printf("typedef inside functions is not ready yet\n");
			exit(1);
			
		} else if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"goto")){
			
			int32_t labelStart=advanceToNonNewlineSpace(sourceContainer.string,endOfToken);
			int32_t labelEnd=getEndOfToken(sourceContainer.string,labelStart);
			int32_t semiColon=advanceToNonNewlineSpace(sourceContainer.string,labelEnd);
			if (sourceContainer.string[semiColon]!=';'){
				printInformativeMessageAtSourceContainerIndex(true,"Expected \';\' after goto statement",semiColon,0);
				exit(1);
			}
			walkingIndex=semiColon;
			insert_IB_address_label(parentInstructionBufferToInsertTo,addGotoOrLabel(copyStringSegmentToHeap(sourceContainer.string,labelStart,labelEnd),true));
			singleMergeIB(parentInstructionBufferToInsertTo,&ib_direct_jump);
			
		} else if (isSectionLabel(walkingIndex,endOfToken)){
			
			int32_t labelStart=walkingIndex;
			int32_t labelEnd=endOfToken;
			int32_t colon=advanceToNonNewlineSpace(sourceContainer.string,endOfToken);
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
						printf("\'register\' keyword cannot be specified twice\n");
						exit(1);
					}
					usedRegister = true;
					walkingIndex += 9;
				}
				if (specificStringEqualCheck(sourceContainer.string,walkingIndex,walkingIndex+7,"static ")){
					if (usedStatic){
						printf("\'static\' keyword cannot be specified twice\n");
						exit(1);
					}
					usedStatic = true;
					walkingIndex += 7;
				}
				if (sourceContainer.string[indexOfEndOfDeclaration]=='='){
					int32_t indexOfStartOfInitializer = advanceToNonNewlineSpace(sourceContainer.string,indexOfEndOfDeclaration+1);
					int32_t indexOfEndOfInitializer = findIndexOfTypicalStatementEnd(sourceContainer.string,indexOfStartOfInitializer);
					if (indexOfStartOfInitializer==-1 | indexOfEndOfInitializer==-1){
						printf("Could not find end to statement with declaration\n");
						exit(1);
					}
					expressionToAssemblyWithInitializer(parentInstructionBufferToInsertTo,indexOfStartOfInitializer,indexOfEndOfInitializer,walkingIndex,indexOfEndOfDeclaration,usedRegister,usedStatic);
					walkingIndex=indexOfEndOfInitializer;
				} else {
					char* typeString = fullTypeParseAndAdd(walkingIndex,indexOfEndOfDeclaration,false);
					addVariableToBlockFrame(typeString,usedRegister,usedStatic);
					cosmic_free(typeString);
					walkingIndex=indexOfEndOfDeclaration;
				}
			} else {
				// then assume it is an expression
				int32_t endingSemicolon = findIndexOfTypicalStatementEnd(sourceContainer.string,walkingIndex);
				if (walkingIndex!=endingSemicolon){
					expressionToAssemblyWithCast(parentInstructionBufferToInsertTo,"void",walkingIndex,endingSemicolon);
				}
				walkingIndex=endingSemicolon;
			}
		}
		
		if (stopAfterSingleExpression){
			removeBlockFrame();
			return walkingIndex;
		}
		walkingIndex++;
	}
	printf("functionStatementsWalk() hit end of source string\n");
	exit(1);
}












// this function runs the parser and compiler, and starts at the file scope
void fileScopeStatementsWalk(){
	int32_t walkingIndex = 0;
	char firstCharacter;
	
	while ((firstCharacter=sourceContainer.string[walkingIndex])){
		if ((firstCharacter==' ') | (firstCharacter=='\n')){
			walkingIndex++;
			continue;
		}
		#ifdef STATEMENT_DEBUG
		printInformativeMessageAtSourceContainerIndex(
						false,"file scope walk starting here:",walkingIndex,0);
		#endif
		int32_t endOfToken = getEndOfToken(sourceContainer.string,walkingIndex);
		if (endOfToken==-1){
			printf("file scope statement walk was using getEndOfToken() and it failed\n");
			exit(1);
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
				printf("keyword \'auto\' not allowed at file scope\n");
				exit(1);
			}
			if (specificStringEqualCheck(sourceContainer.string,walkingIndex,endOfToken,"register")){
				printf("keyword \'register\' not allowed at file scope\n");
				exit(1);
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
							printf("file scope statement walker was using getEndOfToken() while skipping storage class specifier and it failed\n");
							exit(1);
						}
						continue;
					} else {
						break;
					}
				}
				if (staticCount>1 | externCount>1 | (staticCount!=0 & externCount!=0)){
					printf("error related to storage class specifiers\n");
					exit(1);
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
					uint8_t retValForAddingFunction = addGlobalFunction(typeString,isDefinitionBeingGiven);
					if (retValForAddingFunction==1){
						printf("function declaration has type conflicts with previous declaration\n");
						exit(1);
					} else if (retValForAddingFunction==2){
						printf("function was already defined\n");
						exit(1);
					} else if (retValForAddingFunction==3){
						printf("that identifier is already used for a global variable, it cannot be used for a function\n");
						exit(1);
					} else if (retValForAddingFunction==4){
						printf("function definitions must have identifiers for all parameters\n");
						exit(1);
					}
					if (isDefinitionBeingGiven){
						InstructionBuffer instructionBufferForFunction;
						InstructionSingle instructionSingle;
						struct IdentifierSearchResult identifierSearchResult;
						searchForIdentifier(&identifierSearchResult,typeString,true,false,false,false,true);
						if (!identifierSearchResult.didExist){
							printf("Function could not find it\'s own entry\n");
							exit(1);
						}
						if (blockFrameArray.numberOfValidSlots!=0){
							printf("blockFrameArray.numberOfValidSlots should equal 0 at the start of a function statement walk\n");
							exit(1);
						}
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
						if (blockFrameArray.numberOfValidSlots!=0){
							printf("Internal Error: blockFrameArray.numberOfValidSlots should equal 0 at the end of a function statement walk\n");
							exit(1);
						}
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
						printf("Global variables cannot use inline keyword\n");
						exit(1);
					}
					bool isInitializationBeingGiven;
					if (sourceContainer.string[endIndexForDeclaration]=='='){
						isInitializationBeingGiven = true;
					} else if (sourceContainer.string[endIndexForDeclaration]==';'){
						isInitializationBeingGiven = false;
					} else {
						printf("It seems that a variable was being declared at file scope, but something went wrong when trying to figure out if it had an initalizer\n");
						exit(1);
					}
					int32_t indexOfSemicolon = endIndexForDeclaration;
					uint8_t retValForAddingVariable;
					if (isInitializationBeingGiven){
						indexOfSemicolon = findIndexOfTypicalStatementEnd(sourceContainer.string,endIndexForDeclaration);
						if (indexOfSemicolon==-1){
							printf("statement end detect failed for variable at file scope\n");
							exit(1);
						}
						walkingIndex = indexOfSemicolon+1;
						int32_t startOfInitializer = endIndexForDeclaration+1;
						char c = sourceContainer.string[startOfInitializer];
						while (c==' ' | c=='\n'){
							c = sourceContainer.string[++startOfInitializer];
						}
						struct RawMemoryForInitializer rmfi={0};
						char* typeStringNI = applyToTypeStringRemoveIdentifierToNew(typeString);
						char* typeStringIdentifier = applyToTypeStringGetIdentifierToNew(typeString);
						cosmic_free(typeString);
						int32_t initRoot = initializerMapRoot(startOfInitializer,indexOfSemicolon);
						if (!initializerImplementRoot(&rmfi,NULL,initRoot,&typeStringNI,true)){
							printf("Internal Error: how did that not crash?");
							exit(1);
						}
						typeString=tripleConcatStrings(typeStringIdentifier," ",typeStringNI);
						cosmic_free(typeStringIdentifier);
						cosmic_free(typeStringNI);
						retValForAddingVariable = addGlobalVariable(typeString,0,hasStatic,hasExtern,true);
						
						/*
						TODO: implement the rmfi (except that I probably will remove that and do it a different way)
						
						
						
						*/
						if (rmfi.mem==NULL){
							// this should not happen after initializers are fully supported
						} else {
							cosmic_free(rmfi.mem);
							cosmic_free(rmfi.isSet);
						}
					} else {
						retValForAddingVariable = addGlobalVariable(typeString,0,hasStatic,hasExtern,false);
					}
					if (retValForAddingVariable==1){
						printf("identifier collision at file scope for variable, a variable was already declared with the same identifier\n");
						exit(1);
					} else if (retValForAddingVariable==2){
						printf("identifier collision at file scope for variable, a function was already declared with the same identifier\n");
						exit(1);
					}
				}
			}
			cosmic_free(typeString);
		} else {
			// technically, it is possible for an initalizer to occur. We should add a check for something like a global variable being the first token
			printf("Unknown start at file scope\n");
			exit(1);
		}
	}
}



