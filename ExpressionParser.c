
#include "Common.c"

// doesn't check much. returns the index directly after the token ends
int32_t getEndOfToken(int32_t startIndex){
	char* string=sourceContainer.string;
	char c0;
	char c1 = 0;
	char c2 = 0;
	if ((c0=string[startIndex])!=0){
		if ((c1=string[startIndex+1])!=0){
			c2=string[startIndex+2];
		}
	}
	if (c0=='\"' | c0=='\'' | isLetter(c0) | isDigit(c0) | (c0=='.' & isDigit(c1))){
		int32_t i=startIndex;
		char c;
		if (c0=='\'' | c0=='\"' | (c0=='L' & c1=='\"')){
			char end=c0=='L'?'\"':c0;
			while ((c=string[++i])){
				if (c=='\\' | c==end | c=='\n'){
					++i;
					if (c!='\\' & c==end) return i;
					if (c=='\n'){
						err_1101_("string and character literals must not span multiple lines (use \\n instead)",startIndex);
					}
				}
			}
		} else {
			--i;
			if (c0=='.' | isDigit(c0)){ // this is to seperate between if the '.' will stop the token, as well as handling the floating point "e+" and "e-" part
				while ((c=string[++i])){
					char cn=string[i+1];
					if ((c=='e' | c=='E') & (cn=='+' | cn=='-')) i+=2; // advance past the e and the + or -
					else if (!(isLetter(c) | isDigit(c) | c=='.')) return i;
				}
			} else {
				while ((c=string[++i])){
					if (!(isLetter(c) | isDigit(c))) return i;
				}
			}
		}
		err_1101_("Unexpected EOF while finding end of token that starts here",startIndex);
		return 0; // this return is unreachable
	}
	if ((c0=='>' & c1=='>' & c2=='=') | (c0=='<' & c1=='<' & c2=='=')
		){
		return startIndex+3;
	}
	if ((c1=='=' & (c0=='!' | c0=='=' | c0=='*' | c0=='/' | c0=='%' | c0=='&' | c0=='^' | c0=='|' | c0=='+' | c0=='-' | c0=='<' | c0=='>')) |
		(c0=='-' & (c1=='-' | c1=='>')) | (c0=='+' & c1=='+') | (c0=='&' & c1=='&') |
		(c0=='|' & c1=='|') | (c0=='<' & c1=='<') | (c0=='>' & c1=='>')
		){
		return startIndex+2;
	}
	if (c0=='(' | c0==')' | c0=='[' | c0==']' | c0=='{' | c0=='}' | c0=='.' |
		c0=='+' | c0=='-' | c0=='*' | c0=='/' | c0=='%' | c0=='&' | c0=='^' |
		c0=='|' | c0=='?' | c0==':' | c0=='=' | c0==',' | c0=='<' | c0=='>' |
		c0=='!' | c0=='~' | c0==' ' | c0==';' | c0=='\n'
		){
		return startIndex+1;
	}
	err_1101_("Unexpected character while attempting to find end of token",startIndex);
	return 0; // this return is unreachable
}





bool isSubstringANameForAValidType(char *string, int32_t startIndex, int32_t endIndex){
	int32_t length = endIndex-startIndex;
	if (length==8){
		if (isSectionOfStringEquivalent(string,startIndex,"unsigned")){
			return true;
		}
	} else if (length==6){
		if (isSectionOfStringEquivalent(string,startIndex,"signed") ||
			isSectionOfStringEquivalent(string,startIndex,"struct") ||
			isSectionOfStringEquivalent(string,startIndex,"double")){
			return true;
		}
	} else if (length==5){
		if (isSectionOfStringEquivalent(string,startIndex,"short") ||
			isSectionOfStringEquivalent(string,startIndex,"_Bool") ||
			isSectionOfStringEquivalent(string,startIndex,"union") ||
			isSectionOfStringEquivalent(string,startIndex,"float")){
			
			return true;
		}
	}else if (length==4){
		if (isSectionOfStringEquivalent(string,startIndex,"long") ||
			isSectionOfStringEquivalent(string,startIndex,"char") ||
			isSectionOfStringEquivalent(string,startIndex,"enum") ||
			isSectionOfStringEquivalent(string,startIndex,"void")){
			
			return true;
		}
	} else if (length==3){
		if (isSectionOfStringEquivalent(string,startIndex,"int")){
			return true;
		}
	}
	return isSectionOfStringTypedefed(string,startIndex,endIndex);
}



typedef struct ExpressionToken{
	int32_t tokenStart;
	int32_t tokenEnd;
	int16_t operatorTargetRoot; // initialized and used when building expression tree. See those functions for explanation
	int16_t matchingIndex;      // this is valid for enclosements ()[]{}   initialized by the enclosement matcher
	bool isOpenEnclosement;  // this is true even for things like function calls or type casts. initialized by the enclosement matcher
	bool isCloseEnclosement; // this is true even for things like function calls or type casts. initialized by the enclosement matcher
	bool containedNewlineAfterThisToken; // currently, this is initialized, but not used
	bool isThisPieceOfTernaryBonded; // only used for the '?' on ternaries, and is used for error checking
	uint8_t operatorID;              // this is a valid number for all tokens, however 
	// it mostly only contains operator information and possibly a little information on the nature of the token
	
	int16_t precedenceToAddImmediatelyBeforeToken;
	int16_t precedenceToRemoveImmediatelyAfterToken;
	int16_t precedenceTotal; // calculated after operator order is mapped using the other 2 precedence numbers in this struct. Does not include ternaryPrecedenceTotal
	int16_t nextTokenIndexWithMatchingPrecedence; // initialized right before building expression tree. If it equals -1, there is no next token
} ExpressionToken;

struct {
	ExpressionToken* tokens;
	int16_t len;
	int16_t allocLen;
} expressionTokenArray;
// expressionTokenArray is a temporary storage for when an expression tree is being built


/*
operatorID list

ID = token = details

 0 =     = Invalid
 1 = ++  = postfix
 2 = --  = postfix
 3 = (   = function call
 4 = [   = array item
 5 = .   = structure and union member
 6 = ->  = structure and union member through pointer
 7 = (   = (reserved, not detected as of now) compound literal
 8 = ++  = prefix
 9 = --  = prefix
10 = +   = unary
11 = -   = unary
12 = !   = logical not
13 = ~   = bitwise not
14 = (   = type cast
15 = *   = dereference
16 = &   = address of
17 = sizeof
18 = *   = multiplication
19 = /   = division
20 = %   = modulus
21 = +   = addition
22 = -   = subtraction
23 = <<  = bitshift left  (to more significant bits)
24 = >>  = bitshift right (to less significant bits)
25 = <=  = relational, bool return
26 = >=  = relational, bool return
27 = <   = relational, bool return
28 = >   = relational, bool return
29 = ==  = relational, bool return
30 = !=  = relational, bool return
31 = &   = bitwise and
32 = ^   = bitwise xor
33 = |   = bitwise or
34 = &&  = logical and (involves internal conditional jump)
35 = ||  = logical or  (involves internal conditional jump)
36 = ?   = ternary conditional (part 1)
37 = :   = ternary conditional (part 2)
38 = =   = simple assignment
39 = +=  = assignment by sum
40 = -=  = assignment by difference
41 = *=  = assignment by multiplication
42 = /=  = assignment by division
43 = %=  = assignment by modulus
44 = <<= = assignment by bitwise shift left
45 = >>= = assignment by bitwise shift right
46 = &=  = assignemnt by bitwise and
47 = ^=  = assignment by bitwise xor
48 = |=  = assignment by bitwise or
49 = ,   = used as operator
50 = ,   = used as seperator
51 = ,   = unknown if used as operator or seperator
52 = ++  = unknown if prefix or postfix
53 = --  = unknown if prefix or postfix
54 = +   = unknown if unary
55 = -   = unknown if unary
56 = *   = unknown if unary
57 = &   = unknown if unary
58 =     = symbol that is not an operator
59 =     = name that is not a type
60 =     = name that is a type
61 =     = string constant
62 =     = number constant
63 = (   = open parenthese with possible meaning
64 = (   = contains parenthesized argument for sizeof
65 =     = name that is expected to be a function name
66 = (   = dual sided function call (doesn't have a specific name, the address of the function is gotten by a function pointer)
*/


#ifdef COMPILE_EXP_DEBUG_PRINTOUT
bool isOperatorForExpressionTree(uint8_t oID);
void internalDebugForExpressionParser(){
	printf("In progress results follow:\n");
	for (int32_t i=0;i<expressionTokenArray.len;i++){
		char *tokenStringContents = copyStringSegmentToHeap(sourceContainer.string,expressionTokenArray.tokens[i].tokenStart,expressionTokenArray.tokens[i].tokenEnd);
		ExpressionToken token = expressionTokenArray.tokens[i];
		printf("token number %3d has precedenceTotal   :%2d:%2d, catagory %2d, isOperator %1d , and contents \"%s\"\n",
			i,
			token.precedenceToAddImmediatelyBeforeToken,
			token.precedenceToRemoveImmediatelyAfterToken,
			token.operatorID,
			isOperatorForExpressionTree(token.operatorID),
			tokenStringContents);
		cosmic_free(tokenStringContents);
	}
}
#endif


bool isExpressionTokenName(int16_t index){ // this detection is a little more complicated then normal, so it is in a function
	ExpressionToken expressionToken = expressionTokenArray.tokens[index];
	
	char firstCharacter = sourceContainer.string[expressionToken.tokenStart];
	if (isLetter(firstCharacter)){
		if (firstCharacter=='L'){
			return sourceContainer.string[expressionToken.tokenStart+1]!='\"'; // this should not cause an out of bounds error
		} else {
			return true; // normally, we would also check for if it was not sizeof, however that check is done elsewhere
		}
	}
	return false;
}


int16_t findIndexForAppropriatePrecedenceToLeft(bool includeThisPrecedence, int16_t operatorIndex){
	if (operatorIndex==0){
		printf("an expression operator requiring a left argument was encountered as the first token in an expression\n");
		exit(1);
	}
	// in this one, we are going backwards, so we add instead of subtract and subtract instead of add (when dealing with the relativePrecedence)
	int16_t relativePrecedence = 0;
	if (includeThisPrecedence){
		relativePrecedence = -expressionTokenArray.tokens[operatorIndex].precedenceToAddImmediatelyBeforeToken;
	}
	int16_t operatorIndexStart=operatorIndex-1;
	if (relativePrecedence+expressionTokenArray.tokens[operatorIndexStart].precedenceToRemoveImmediatelyAfterToken<=0){
		return operatorIndexStart;
	}
	for (int16_t i=operatorIndexStart;i>-1;i--){
		relativePrecedence += expressionTokenArray.tokens[i].precedenceToRemoveImmediatelyAfterToken;
		// `relativePrecedence>0` is always true at this point
		relativePrecedence -= expressionTokenArray.tokens[i].precedenceToAddImmediatelyBeforeToken;
		if (relativePrecedence<=0 & i!=operatorIndex-1) return i;
	}
	return 0;
}

int16_t findIndexForAppropriatePrecedenceToRight(bool includeThisPrecedence, int16_t operatorIndex){
	int16_t lengthOfExpressionTokens=expressionTokenArray.len;
	if (operatorIndex==lengthOfExpressionTokens-1){
		printf("an expression operator requiring a right argument was encountered as the last token in an expression\n");
		exit(1);
	}
	int16_t relativePrecedence = 0;
	if (includeThisPrecedence){
		relativePrecedence = -expressionTokenArray.tokens[operatorIndex].precedenceToRemoveImmediatelyAfterToken;
	}
	int16_t operatorIndexStart=operatorIndex+1;
	if (relativePrecedence+expressionTokenArray.tokens[operatorIndexStart].precedenceToAddImmediatelyBeforeToken<=0){
		return operatorIndexStart;
	}
	for (int16_t i=operatorIndexStart;i<lengthOfExpressionTokens;i++){
		relativePrecedence += expressionTokenArray.tokens[i].precedenceToAddImmediatelyBeforeToken;
		// `relativePrecedence>0` is always true at this point
		relativePrecedence -= expressionTokenArray.tokens[i].precedenceToRemoveImmediatelyAfterToken;
		if (relativePrecedence<=0 & i!=operatorIndex-1) return i;
	}
	return lengthOfExpressionTokens-1;
}

void markValidCommasAsSeperatorsForFunction(int16_t start){
	assert(expressionTokenArray.tokens[start].isOpenEnclosement); // called wrong
	int16_t end = expressionTokenArray.tokens[start].matchingIndex;
	ExpressionToken *currentTokenPtr;
	for (int16_t i=start+1;i<end;i++){
		currentTokenPtr = &(expressionTokenArray.tokens[i]);
		
		if (currentTokenPtr->isOpenEnclosement){ // skip enclosed areas
			i = currentTokenPtr->matchingIndex - 1; // -1 is for the fact that i is incremented
		} else if (currentTokenPtr->operatorID==51){
			currentTokenPtr->operatorID = 50;
		}
	}
}


/*
parses expressions into the precedence total data structure.
expects that matchEnclosementsOnExpressionTokenArray() has matched the enclosements.
does not handle declarations.
*/
void generatePrecedenceTotal(){
	int16_t i;
	int16_t tokenLen = expressionTokenArray.len;
	int32_t lengthOfStringInCurrentToken;
	int16_t leftIndexForPairing;
	int16_t rightIndexForPairing;
	ExpressionToken *tokens = expressionTokenArray.tokens;
	ExpressionToken *currentTokenPtr;
	char fcct; // first character of current token
	uint8_t operatorIDofCurrentToken;
	bool doPairing = false;
	/* 
	This will be looping a lot so those variable declarations up there can just go there for common names between loops.
	sometimes the variable is set for a loop, sometimes it is not. check the beginning of the loop to see which ones are set for that loop
	*/
	
	for (i=0;i<tokenLen;i++){ // initialize part 1: ensure zero
		currentTokenPtr = tokens+i;
		
		currentTokenPtr->precedenceToAddImmediatelyBeforeToken = 0;
		currentTokenPtr->precedenceToRemoveImmediatelyAfterToken = 0;
		currentTokenPtr->operatorID = 0;
		currentTokenPtr->isThisPieceOfTernaryBonded = false;
	}
	for (i=0;i<tokenLen;i++){ // initialize part 2: catagorize
		currentTokenPtr = tokens+i;
		int32_t ifcct; // indexOfFirstCharacterInCurrentToken
		ifcct = currentTokenPtr->tokenStart;
		lengthOfStringInCurrentToken = currentTokenPtr->tokenEnd - ifcct;
		fcct = sourceContainer.string[ifcct];
		
		uint8_t n_oID = 0;
		if (lengthOfStringInCurrentToken==6){
			if (isSectionOfStringEquivalent(sourceContainer.string,ifcct,"sizeof"))   n_oID = 17;
		} else if (lengthOfStringInCurrentToken==3){
			if (fcct=='<' | fcct=='>'){
				if (isSectionOfStringEquivalent(sourceContainer.string,ifcct,"<<="))      n_oID = 44;
				else if (isSectionOfStringEquivalent(sourceContainer.string,ifcct,">>=")) n_oID = 45;
			}
		} else if (lengthOfStringInCurrentToken==2){
			switch ((fcct<<8)|sourceContainer.string[ifcct+1]){
				case ('+'<<8)|'+':n_oID = 52;break;
				case ('-'<<8)|'-':n_oID = 53;break;
				case ('-'<<8)|'>':n_oID =  6;break;
				case ('<'<<8)|'<':n_oID = 23;break;
				case ('>'<<8)|'>':n_oID = 24;break;
				case ('&'<<8)|'&':n_oID = 34;break;
				case ('|'<<8)|'|':n_oID = 35;break;
				case ('<'<<8)|'=':n_oID = 25;break;
				case ('>'<<8)|'=':n_oID = 26;break;
				case ('='<<8)|'=':n_oID = 29;break;
				case ('!'<<8)|'=':n_oID = 30;break;
				case ('+'<<8)|'=':n_oID = 39;break;
				case ('-'<<8)|'=':n_oID = 40;break;
				case ('*'<<8)|'=':n_oID = 41;break;
				case ('/'<<8)|'=':n_oID = 42;break;
				case ('%'<<8)|'=':n_oID = 43;break;
				case ('&'<<8)|'=':n_oID = 46;break;
				case ('^'<<8)|'=':n_oID = 47;break;
				case ('|'<<8)|'=':n_oID = 48;break;
			}
		} else if (lengthOfStringInCurrentToken==1){
			switch (fcct){
				case '(':n_oID = 63;break;
				case '[':n_oID =  4;break;
				case '.':n_oID =  5;break;
				case '+':n_oID = 54;break;
				case '-':n_oID = 55;break;
				case '!':n_oID = 12;break;
				case '~':n_oID = 13;break;
				case '&':n_oID = 57;break;
				case '*':n_oID = 56;break;
				case '/':n_oID = 19;break;
				case '%':n_oID = 20;break;
				case '<':n_oID = 27;break;
				case '>':n_oID = 28;break;
				case '^':n_oID = 32;break;
				case '|':n_oID = 33;break;
				case '?':n_oID = 36;break;
				case ':':n_oID = 37;break;
				case '=':n_oID = 38;break;
				case ',':n_oID = 51;break;
			}
		}
		if (n_oID==0){
			if (isLetter(fcct) |
				isDigit(fcct) |
				fcct=='\"' |
				fcct=='\'' |
				fcct=='.'){
				
				if (isExpressionTokenName(i)){
					n_oID = 59+(bool)isSubstringANameForAValidType(sourceContainer.string,ifcct,currentTokenPtr->tokenEnd);
				} else {
					if (fcct=='\"' |
					    fcct=='\'' |
						(fcct=='L' & 
						 sourceContainer.string[ifcct+1]=='\"')){
						
						n_oID = 61;
					} else if (isDigit(fcct) || (isDigit(sourceContainer.string[ifcct+1]) & fcct=='.')){
						n_oID = 62;
					} else {
						printf("ill-formed token detected: cannot discern between number, name, or literal\n");
						exit(1);
					}
				}
			} else {
				n_oID = 58; // or it is something very invalid. but i'm not checking that right now because I don't want to write the code for it
			}
		}
		currentTokenPtr->operatorID = n_oID;
	}
	
	// currently, function calls are detected by the token to the left of an open parenthese being a name or through a pair of tokens like this: ")(" with some additional checks
	
	for (i=0;i<tokenLen;i++){ // initialize part 3: detect function calls, typecasts, and sizeof(type) for the token '(' . also set names that are expected to be functions
		currentTokenPtr = tokens+i;
		operatorIDofCurrentToken = currentTokenPtr->operatorID;
		
		if (operatorIDofCurrentToken==63){
			bool isFirstToken = i==0;       // can also be seen as "does not have previous token"
			bool isLastToken = i==tokenLen-1; // can also be seen as "does not have next token"
			bool voteForFunctionCall = false;
			bool voteForTypeCast = false;
			bool voteForSizeofArgument = false;
			bool voteForDualSidedFunctionCall = false;
			uint8_t totalNumberOfVotes = 0;
			uint8_t n_oID = 0;
			if (!isFirstToken && tokens[i-1].operatorID==59){
				voteForFunctionCall = true;
				totalNumberOfVotes++;
			}
			if (!isFirstToken && tokens[i-1].operatorID==17){
				voteForSizeofArgument = true;
				totalNumberOfVotes++;
			} else if (i<tokenLen-3 && tokens[i+1].operatorID==60 &&
				(isFirstToken || tokens[i-1].operatorID!=17)){
				
				// by the way, the reason this is an "else if" is because a token sequence "sizeof" "(" "type" should only be recognized as the sizeof sequence and not a type cast
				int16_t matchingIndexForEnclosement = currentTokenPtr->matchingIndex;
				if (matchingIndexForEnclosement<tokenLen-1){
					voteForTypeCast = true;
					totalNumberOfVotes++;
				} else {
					printf("a token series that was detected to be in the form of a type cast fails to contain a token as it's argument\n");
					exit(1);
				}
			}
			if (!isFirstToken && (
				tokens[i-1].isCloseEnclosement & 
				sourceContainer.string[tokens[i-1].tokenStart]==')' & 
				totalNumberOfVotes==0 & // this check is mostly for not having a vote for a type cast
				tokens[tokens[i-1].matchingIndex].operatorID==58)){
				
				voteForDualSidedFunctionCall = true;
				totalNumberOfVotes++;
			}
			if (totalNumberOfVotes==0){
				// it is a parenthese that doesn't have special meaning
				n_oID = 58;
			} else if (totalNumberOfVotes==1){
				if (voteForFunctionCall){
					bool isActuallyDualSidedFunctionCall=false;
					if (i>=2){
						uint8_t farOpID=tokens[i-2].operatorID;
						isActuallyDualSidedFunctionCall= farOpID==5 | farOpID==6;
					}
					if (isActuallyDualSidedFunctionCall){
						n_oID = 66;
					} else {
						n_oID = 3;
						tokens[i-1].operatorID = 65; // couldn't have been first token due to previous checks, so no index error
					}
				} else if (voteForTypeCast){
					n_oID = 14;
				} else if (voteForSizeofArgument){
					n_oID = 64;
				} else {
					assert(voteForDualSidedFunctionCall);
					n_oID = 66;
				}
			} else {
				// too many votes, the compiler can't tell what this parenthese is supposed to mean
				printf("an ambigous parenthese is not okay (%d,%d,%d,%d)\n",voteForFunctionCall,voteForTypeCast,voteForSizeofArgument,voteForDualSidedFunctionCall);
				exit(1);
			}
			currentTokenPtr->operatorID = n_oID;
			bool doSetToCatagory60 = false;
			if (n_oID==14){
				// if it is a typecast, then we must set the catagories of all tokens inside the parentheses to be catagory 60
				doSetToCatagory60 = true;
			} else if ((n_oID==64 & !isLastToken) && tokens[i+1].operatorID==60){
				// then this is a sizeof argument with types in it, so we must set the catagories of all tokens inside the parentheses to be catagory 60
				doSetToCatagory60 = true;
			}
			if (doSetToCatagory60){
				int16_t matchingIndexForEnclosement = currentTokenPtr->matchingIndex;
				for (i=i+1;i<matchingIndexForEnclosement;i++){
					tokens[i].operatorID = 60;
				}
			}
		}
	}
	for (i=0;i<tokenLen;i++){ // initialize part 4: mark comma tokens used as seperators for function calls
		operatorIDofCurrentToken = tokens[i].operatorID;
		if (operatorIDofCurrentToken==3 | operatorIDofCurrentToken==66) markValidCommasAsSeperatorsForFunction(i);
	}
	for (i=0;i<tokenLen;i++){ // initialize part 5: set comma tokens with unknown meaning to have operator meaning
		if (tokens[i].operatorID==51) tokens[i].operatorID = 49;
	}
	for (i=0;i<tokenLen;i++){ // initialize part 6: for operators with multiple meanings, discern which meaning it to be used
		currentTokenPtr = tokens+i;
		operatorIDofCurrentToken = currentTokenPtr->operatorID;
		uint8_t n_oID;
		
		if (operatorIDofCurrentToken==52 |
			operatorIDofCurrentToken==53){
			
			bool isPostfix = true;
			if (i==0){
				isPostfix = false;
			} else {
				ExpressionToken *previousTokenPtr = tokens+(i-1);
				uint8_t previousOperatorID = previousTokenPtr->operatorID;
				if (previousTokenPtr->isOpenEnclosement | (previousOperatorID<=50 & previousOperatorID>=8)) isPostfix = false;
			}
			if (isPostfix)
				tokens[i].operatorID -= 51; // this is relative, not absolute
			else
				tokens[i].operatorID -= 44; // this is relative, not absolute
			
		} else if (operatorIDofCurrentToken>=54 & operatorIDofCurrentToken<=57){
			bool isUnary = true;
			if (i!=0){
				ExpressionToken *previousTokenPtr = &(tokens[i-1]);
				uint8_t previousOperatorID = previousTokenPtr->operatorID;
				if (
					(previousOperatorID==1 |
					previousOperatorID==2 |
					previousOperatorID==59 | 
					previousOperatorID==61 | 
					previousOperatorID==62) ||
					(previousTokenPtr->isCloseEnclosement && 
					tokens[previousTokenPtr->matchingIndex].operatorID!=14)){
						// should the string literal be a check here?
					
					isUnary = false;
				}
			}
			if (isUnary){
				if (operatorIDofCurrentToken==54)      n_oID = 10;
				else if (operatorIDofCurrentToken==55) n_oID = 11;
				else if (operatorIDofCurrentToken==56) n_oID = 15;
				else if (operatorIDofCurrentToken==57) n_oID = 16;
				// no other conditions should be possible
			} else {
				if (operatorIDofCurrentToken==54)      n_oID = 21;
				else if (operatorIDofCurrentToken==55) n_oID = 22;
				else if (operatorIDofCurrentToken==56) n_oID = 18;
				else if (operatorIDofCurrentToken==57) n_oID = 31;
				// no other conditions should be possible
			}
			currentTokenPtr->operatorID = n_oID;
		}
	}
	/* 
	initialization done
	from now on, these loops build the precedence order so the expression tree can be built much easier
	sometimes, the loops below go backward. That is because the associativity of those operators is right to left instead of left to right
	*/
	for (i=0;i<tokenLen;i++){ // enclosements (these aren't operators, but these need to be done because enclosements enclose things inside them)
		// also note that this must be reversed after everything else is built
		currentTokenPtr = tokens+i;
		
		if (currentTokenPtr->isOpenEnclosement){
			++currentTokenPtr->precedenceToAddImmediatelyBeforeToken;
			++tokens[currentTokenPtr->matchingIndex].precedenceToRemoveImmediatelyAfterToken;
		}
	}
	for (i=0;i<tokenLen;i++){ // precedence level 1
		currentTokenPtr = tokens+i;
		operatorIDofCurrentToken = currentTokenPtr->operatorID;
		
		if (operatorIDofCurrentToken==1 | operatorIDofCurrentToken==2){ // postfix ++ and --
			leftIndexForPairing = findIndexForAppropriatePrecedenceToLeft(true,i);
			rightIndexForPairing = i;
			doPairing = true;
		} else if (operatorIDofCurrentToken==5 | operatorIDofCurrentToken==6){ // . and ->
			leftIndexForPairing  =  findIndexForAppropriatePrecedenceToLeft(true,i);
			rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
			doPairing = true;
		} else if (operatorIDofCurrentToken==3){ // function call
			leftIndexForPairing = i-1;
			rightIndexForPairing = currentTokenPtr->matchingIndex;
			doPairing = true;
		} else if (operatorIDofCurrentToken==66){ // dual sided function call
			leftIndexForPairing  = findIndexForAppropriatePrecedenceToLeft(false,i); // I think don't include precedence?
			rightIndexForPairing = currentTokenPtr->matchingIndex;
			doPairing = true;
		} else if (operatorIDofCurrentToken==4){ // array item
			leftIndexForPairing  = findIndexForAppropriatePrecedenceToLeft(false,i); // don't include the enclosement precedence for array
			rightIndexForPairing = currentTokenPtr->matchingIndex;
			doPairing = true;
		}
		if (doPairing){
			doPairing = false;
			++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
			++tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken;
		}
	}
	for (i=tokenLen-1;i>=0;i--){ // precedence level 2
		currentTokenPtr = tokens+i;
		operatorIDofCurrentToken = currentTokenPtr->operatorID;
		
		if (operatorIDofCurrentToken>=8 & operatorIDofCurrentToken<=17){
			if (operatorIDofCurrentToken==14){ // type cast
				leftIndexForPairing = i;
				rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(false,currentTokenPtr->matchingIndex);
			} else { // prefixOperators ++ and --    and unaryOperators + and - and ! and ~ and * and & and sizeof
				leftIndexForPairing = i;
				rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
			}
			++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
			++tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken;
		}
	}

	for (uint8_t precedenceLevel=3;precedenceLevel<13;precedenceLevel++){ // precedence levels 3 through 12
		uint8_t lowerOperaterID;
		uint8_t upperOperatorID;
		switch (precedenceLevel){
			case  3:lowerOperaterID=18;upperOperatorID=20;break;
			case  4:lowerOperaterID=21;upperOperatorID=22;break;
			case  5:lowerOperaterID=23;upperOperatorID=24;break;
			case  6:lowerOperaterID=25;upperOperatorID=28;break;
			case  7:lowerOperaterID=29;upperOperatorID=30;break;
			case  8:lowerOperaterID=31;upperOperatorID=31;break;
			case  9:lowerOperaterID=32;upperOperatorID=32;break;
			case 10:lowerOperaterID=33;upperOperatorID=33;break;
			case 11:lowerOperaterID=34;upperOperatorID=34;break;
			case 12:lowerOperaterID=35;upperOperatorID=35;break;
		}
		for (i=0;i<tokenLen;i++){
			operatorIDofCurrentToken = tokens[i].operatorID;
			
			if (operatorIDofCurrentToken>=lowerOperaterID & operatorIDofCurrentToken<=upperOperatorID){
				leftIndexForPairing  =  findIndexForAppropriatePrecedenceToLeft(true,i);
				rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
				++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
				++tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken;
			}
		}
	}
	// precedence level 13 (ternary)
	for (i=tokenLen-1;i>=0;i--){
		operatorIDofCurrentToken = tokens[i].operatorID;
		if (operatorIDofCurrentToken==36 & !tokens[i].isThisPieceOfTernaryBonded){
			printf("This ternary is wrong, the \'?\' goes before the \':\', not after\n");
			exit(1);
		}
		if (operatorIDofCurrentToken==37){
			// now we need to find the '?' to the ':'
			int16_t middleIndex;
			uint16_t tempTernaryLevel = 0;
			bool didEndProperly = false;
			for (int16_t i2=i-1;i2>=0;i2--){
				uint8_t tempOperatorID = tokens[i2].operatorID;
				if (tempOperatorID==36){
					if (tempTernaryLevel==0){
						if (i2+1==i){
							printf("For a ternary, the left side of the \':\' cannot be directly next to the \'?\'\n");
							exit(1);
						}
						if (tokens[i2].isThisPieceOfTernaryBonded){
							printf("Error when parsing ternary\n"); // this case would be really weird (might even be impossible), and certainly wrong
							exit(1);
						}
						tokens[i2].isThisPieceOfTernaryBonded = true;
						middleIndex = i2;
						didEndProperly = true;
						break;
					} else {
						tempTernaryLevel--; 
					}
				} else if (tempOperatorID==37){
					tempTernaryLevel++;
				}
			}
			if (!didEndProperly){
				printf("This ternary is wrong, couldn\'t find the \'?\' for the \':\'\n");
				exit(1);
			}
			// then we found the '?', and it's index is in middleIndex
			// the operators are bonded in the tree as if the '?' and ':' are 2 operand operators, with the ':' on the right side the '?'
			leftIndexForPairing  =  findIndexForAppropriatePrecedenceToLeft(true,middleIndex);
			rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
			
			++tokens[middleIndex+1].precedenceToAddImmediatelyBeforeToken;
			
			++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
			tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken+=2;
		}
	}
	for (i=tokenLen-1;i>=0;i--){ // precedence level 14
		operatorIDofCurrentToken = tokens[i].operatorID;
		
		if (operatorIDofCurrentToken>=38 && operatorIDofCurrentToken<=48){
			leftIndexForPairing  =  findIndexForAppropriatePrecedenceToLeft(true,i);
			rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
			++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
			++tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken;
		}
	}
	for (i=0;i<tokenLen;i++){ // precedence level 15
		if (tokens[i].operatorID==49){
			leftIndexForPairing  =  findIndexForAppropriatePrecedenceToLeft(true,i);
			rightIndexForPairing = findIndexForAppropriatePrecedenceToRight(true,i);
			++tokens[leftIndexForPairing].precedenceToAddImmediatelyBeforeToken;
			++tokens[rightIndexForPairing].precedenceToRemoveImmediatelyAfterToken;
		}
	}
	// all operators should now be bonded in the correct precedence order
	
	for (i=0;i<tokenLen;i++){ // enclosement reversal (needed for future parsing, especially array operators)
		currentTokenPtr = tokens+i;
		
		if (currentTokenPtr->isOpenEnclosement){
			--currentTokenPtr->precedenceToAddImmediatelyBeforeToken;
			--tokens[currentTokenPtr->matchingIndex].precedenceToRemoveImmediatelyAfterToken;
		}
	}
	
	// just a little more processing to get the result to something that is easier to read elsewhere
	int16_t precedenceWalkingTotal = 0;
	for (i=0;i<tokenLen;i++){
		currentTokenPtr = tokens+i;
		
		precedenceWalkingTotal += currentTokenPtr->precedenceToAddImmediatelyBeforeToken;
		currentTokenPtr->precedenceTotal = precedenceWalkingTotal;
		precedenceWalkingTotal -= currentTokenPtr->precedenceToRemoveImmediatelyAfterToken;
	}
	// error checking pass(es). These help to verify the validity of the expression
	if (precedenceWalkingTotal!=0){
		printf("There was some sort of error when doing precedence evaluation on an expression. That is all that is known about this error.\n");
		exit(1);
	}
	for (i=0;i<tokenLen;i++){
		currentTokenPtr = tokens+i;
		if (currentTokenPtr->isOpenEnclosement && tokens[currentTokenPtr->matchingIndex].precedenceTotal != currentTokenPtr->precedenceTotal){
			printf("enclosement bounding check failed. This expression is ill-formed.\n");
			exit(1);
		}
	}
}

void checkForZeroPrecedenceInPrecedenceTotal(){
	int16_t tokenLen=expressionTokenArray.len;
	for (int16_t i=0;i<tokenLen;i++){
		if (expressionTokenArray.tokens[i].precedenceTotal==0){
			printInformativeMessageAtSourceContainerIndex(true,"This expression should be two seperate expressions, the seperation must be about here",expressionTokenArray.tokens[i].tokenStart-1,0);
			exit(1);
		}
	}
}

int16_t matchSpecificEnclosementOnExpressionTokenArray(int16_t tokenIndexToMatch){
	int16_t tokenLen=expressionTokenArray.len;
	char startTarget = sourceContainer.string[expressionTokenArray.tokens[tokenIndexToMatch].tokenStart];
	char endTarget;
	if (startTarget=='('){
		endTarget=')';
	} else if (startTarget=='['){
		endTarget=']';
	} else if (startTarget=='{'){
		endTarget='}';
	} else {
		printf("Internal Error: sub enclosement matcher called with invalid start.\n");
		exit(1); // then this function was called with an invalid start.
	}
	for (int16_t i=tokenIndexToMatch+1;i<tokenLen;i++){
		if (expressionTokenArray.tokens[i].isOpenEnclosement){
			i = matchSpecificEnclosementOnExpressionTokenArray(i);
		} else if (sourceContainer.string[expressionTokenArray.tokens[i].tokenStart]==endTarget){ // if the first character of token[i] matches the endTarget
			expressionTokenArray.tokens[tokenIndexToMatch].matchingIndex = i;
			expressionTokenArray.tokens[i].matchingIndex = tokenIndexToMatch;
			return i;
		} else if (expressionTokenArray.tokens[i].isCloseEnclosement){
			printf("enclosement mismatch.\n");
			exit(1); // then there would be an enclosement mismatch. The correct end would have been caught by the above if statement
		}
	}
	printf("enclosement at %d (in string) does not have a match\n",tokenIndexToMatch);
	exit(1);
}

void matchEnclosementsOnExpressionTokenArray(){
	int16_t tokenLen=expressionTokenArray.len;
	for (int16_t i=0;i<tokenLen;i++){
		char c = sourceContainer.string[expressionTokenArray.tokens[i].tokenStart];
		expressionTokenArray.tokens[i].isOpenEnclosement  = c=='(' | c=='[' | c=='{';
		expressionTokenArray.tokens[i].isCloseEnclosement = c==')' | c==']' | c=='}';
	}
	for (int16_t i=0;i<tokenLen;i++){
		if (expressionTokenArray.tokens[i].isOpenEnclosement){
			i = matchSpecificEnclosementOnExpressionTokenArray(i);
		} else if (expressionTokenArray.tokens[i].isCloseEnclosement){
			printf("enclosement mismatch.\n");
			exit(1);
		}
	}
}


/*
when the expression is terminated with a semicolon, then the following should be true:  sourceContainer.string[endIndex]==';'
also removes spaces and newlines (but notates if a newline existed)
does not call the enclosement matcher or the expression parsing functions
*/
void generateTokenArrayForExpression(int32_t startIndex, int32_t endIndex){
	bool terminatedAtCorrectIndex = false;
	int16_t lengthCount = 0;
	for (int32_t i=startIndex;i<endIndex;){
		char c=sourceContainer.string[i];
		if (!(c==' ' | c=='\n')){
			if (++lengthCount>10000){
				err_1111_("This expression is too large (>10000 tokens)",startIndex,endIndex);
			}
		}
		i = getEndOfToken(i);
		terminatedAtCorrectIndex=i==endIndex;
	}
	if (!terminatedAtCorrectIndex){
		printf("Internal Error: generateTokenArrayForExpression() was told to end at a character that is inside a token.\n");
		exit(1); // the endIndex is inside of a token. That's the calling function's problem, and it shouldn't happen
	}
	if (expressionTokenArray.allocLen<lengthCount){
		expressionTokenArray.allocLen=lengthCount;
		cosmic_free(expressionTokenArray.tokens);
		expressionTokenArray.tokens = cosmic_calloc(lengthCount,sizeof(ExpressionToken));
	} else {
		memZero(expressionTokenArray.tokens,lengthCount*sizeof(ExpressionToken));
	}
	expressionTokenArray.len = lengthCount;
	int32_t walkingIndex = 0;
	for (int32_t i=startIndex;i<endIndex;){
		int32_t tokenStart = i;
		i = getEndOfToken(i);
		char c=sourceContainer.string[tokenStart];
		if (!(c==' ' | c=='\n')){
			expressionTokenArray.tokens[walkingIndex].tokenStart = tokenStart;
			expressionTokenArray.tokens[walkingIndex].tokenEnd = i;
			walkingIndex++;
		} else if (c=='\n' & walkingIndex!=0){
			expressionTokenArray.tokens[walkingIndex-1].containedNewlineAfterThisToken = true;
		}
	}
	return;
}

typedef struct {
	int32_t startIndexInString; // this and the next indexes are copied from the token. most important for error locations, and is important for function calls with a name, because the name is effectively stored here
	int32_t endIndexInString;
	int32_t argumentIndexStart; // this and the next component are used for types (for type cast and sizeof), and are an index into the source string
	int32_t argumentIndexEnd;
	int16_t indexOfComponent1; // this is the only one used for unary operators (type casts are a little different).
	// the first parameter of a function is placed in indexOfComponent1
	int16_t indexOfComponent2;
	int16_t nextInChain; // function parameters are chained together using just nextInChain
	bool hasNextChainElement; // used for function parameter chaining
	bool isEndNode;
	bool isArgumentTypeForSizeof; // if this node is a sizeof operator, this is true if the argument is a type rather then an expression
	uint8_t operatorID; // copied from the token
/*
the following are newer pieces. They are typically initialized and handled by expressionToInstructions()
.pre , however, is initialized by buildExpressionTreeToGlobalBufferAndReturnRootIndex()
*/
	struct ExpTreePreWalkData{
		int16_t leftNode;
		int16_t rightNode;
		int16_t chainNode;
		bool hasLeftNode;
		bool hasRightNode;
		bool hasChainNode;
	} pre;
	
	struct ExpTreePostWalkData{
		char* typeStringNQ; // no identifer, volatile, or const. comes from the same allocation as typeString, and should NOT have cosmic_free() called on it
		char* typeString; // no identifier, may have volatile and const. is heap allocated
		// next two refer to the typeString. it makes type detection easier
		bool isRValueConst;
		bool isRValueVolatile;
		// next five refer to the object of the value
		bool isLValue;
		bool isLValueConst;
		bool isLValueVolatile;
		bool isLValueStructResultOfAssignment; // this indicates that it cannot be assigned, but it also is not nessassarily a const lvalue (the data this lvalue points to is not nessassarily const)
		uint16_t sizeOnStack; // this is the number of bytes that were pushed to the stack for function parameter chains
		uint16_t operatorTypeID; // this is a value set by applyAutoTypeConversion() that indicates the kind of operation to be performed. It is used in conjunction with the operatorID by applyOperator()
		uint32_t constVal; // for constant expressions
		// these next two members are values set by applyAutoTypeConversion() that indicates various things, it may be used by applyOperator()
		uint32_t extraVal;
		void* extraPtr;
	} post;
	
	InstructionBuffer ib;
	
} ExpressionTreeNode;

typedef struct {
	ExpressionTreeNode* expressionTreeNodes;
	int16_t numberOfNodesAllocatedToArray;
	int16_t walkingIndexForNextOpenSlot;
} ExpressionTreeGlobalBuffer;
ExpressionTreeGlobalBuffer expressionTreeGlobalBuffer;

void packExpressionTreeGlobalBuffer(ExpressionTreeGlobalBuffer* pack){
	*pack=expressionTreeGlobalBuffer;
	const ExpressionTreeGlobalBuffer n={0};
	expressionTreeGlobalBuffer=n;
}

void unpackExpressionTreeGlobalBuffer(ExpressionTreeGlobalBuffer* pack){
	cosmic_free(expressionTreeGlobalBuffer.expressionTreeNodes);
	expressionTreeGlobalBuffer=*pack;
}

void genSinglePreWalkData(int16_t nodeIndex){
	ExpressionTreeNode* etn = expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	uint8_t oID = etn->operatorID;
	etn->pre.chainNode = etn->nextInChain;
	etn->pre.hasChainNode = etn->hasNextChainElement;
	assert(oID!=3); // oID should not be 3 at this stage
	if (((oID>=8 & oID<=17) | oID==65)){
		// if it is a normal function call, then rightNode gets the first parameter (and the parameters are chained on that node)
		// the name of the function is in argumentIndexStart and argumentIndexEnd
		
		// if it is a unary operator (that includes type casts and prefix), then rightNode gets the argument
		
		etn->pre.leftNode = -1;
		etn->pre.hasLeftNode = false;
		
		if ((oID==17 & etn->isArgumentTypeForSizeof) | 
			(oID==65 & etn->isEndNode)){
			// if this is sizeof and the argument is a type,
			// then there is no argument stored in the components, it is in the argumentIndexStart and argumentIndexEnd
			
			// if it is a function call and it is an end node, then there is no arguments
			etn->pre.rightNode = -1;
			etn->pre.hasRightNode = false;
		} else {
			etn->pre.rightNode = etn->indexOfComponent1;
			etn->pre.hasRightNode = true;
		}
	} else if (oID==1 | oID==2){
		// if is is a postfix operator, then leftNode gets the argument
		etn->pre.leftNode = etn->indexOfComponent1;
		etn->pre.rightNode = -1;
		etn->pre.hasLeftNode = true;
		etn->pre.hasRightNode = false;
	} else if (oID==66 & etn->isEndNode){
		// if this is a complex function call and it an end node,
		// then this complex function call has no arguments (but still has the expression instead of the name)
		etn->pre.leftNode = etn->indexOfComponent2;
		etn->pre.rightNode = -1;
		etn->pre.hasLeftNode = true;
		etn->pre.hasRightNode = false;
	} else if (!(oID>=59 & oID<=62)){
		etn->pre.hasLeftNode = true;
		etn->pre.hasRightNode = true;
		etn->pre.leftNode = etn->indexOfComponent1;
		etn->pre.rightNode = etn->indexOfComponent2;
		if (oID==66){
			// if it is a complex function call, then switch the component names to reflect which side it is on
			etn->pre.leftNode = etn->indexOfComponent2;
			etn->pre.rightNode = etn->indexOfComponent1;
		}
	} else {
		etn->pre.hasLeftNode = false;
		etn->pre.hasRightNode = false;
	}
}


// called in buildExpressionTreeToGlobalBufferAndReturnRootIndex()
void genAllPreWalkData(int16_t nodeIndex){
	genSinglePreWalkData(nodeIndex);
	ExpressionTreeNode* expTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+nodeIndex;
	if (expTreeNode->pre.hasLeftNode) genAllPreWalkData(expTreeNode->pre.leftNode);
	if (expTreeNode->pre.hasRightNode) genAllPreWalkData(expTreeNode->pre.rightNode);
	if (expTreeNode->pre.hasChainNode) genAllPreWalkData(expTreeNode->pre.chainNode);
}


int16_t partitionNewExpressionTreeNodeInGlobalBufferAndReturnIndex(){
	if (expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray<=(expressionTreeGlobalBuffer.walkingIndexForNextOpenSlot+1)){
		if (expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray==0){
			expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray=2;
		}
		expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray*=2;
		if (expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray>=20000){
			printf("That is a ton of expression nodes, I am not allocating more. How big of an expression were you trying to make me do?\n");
			exit(1);
		}
		expressionTreeGlobalBuffer.expressionTreeNodes = cosmic_realloc(
			expressionTreeGlobalBuffer.expressionTreeNodes,
			expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray*sizeof(ExpressionTreeNode));
	}
	return expressionTreeGlobalBuffer.walkingIndexForNextOpenSlot++;
}

static inline bool isOperatorForExpressionTree(const uint8_t oID){
	return (oID<=49 & oID!=3) | oID==65 | oID==66;
}

static inline bool isOneOperandOperatorForExpressionTree(const uint8_t oID){
	return (oID>=8 & oID<=16) | oID==1 | oID==2;
}

static inline bool isTwoOperandOperatorForExpressionTree(const uint8_t oID){
	return (oID>=4 & oID<=6) | (oID>=18 & oID<=49);
}


// returns index of specific node in expression tree
int16_t generateSpecificNodeForExpressionTree(int16_t targetIndex){
	ExpressionToken *targetExpressionToken = expressionTokenArray.tokens+targetIndex;
	ExpressionToken *currentExpressionToken;
	int16_t indexOfThisExpressionTreeNode = partitionNewExpressionTreeNodeInGlobalBufferAndReturnIndex(); // going to have to happen unless there is an error, might as well do it now
	int16_t tokenLen=expressionTokenArray.len;
	ExpressionTreeNode *thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode; // this must be recalculated every time this recurses because of the potential for cosmic_realloc
	uint8_t thisOperatorID = targetExpressionToken->operatorID;
	// basic set up and clearing of the node we are given
	thisExpressionTreeNode->isEndNode = false; // will be changed to true if it is
	thisExpressionTreeNode->hasNextChainElement = false; // will be changed to true if it is
	thisExpressionTreeNode->isArgumentTypeForSizeof = false; // will be changed to true if it is
	thisExpressionTreeNode->operatorID = thisOperatorID;
	thisExpressionTreeNode->startIndexInString = targetExpressionToken->tokenStart;
	thisExpressionTreeNode->endIndexInString = targetExpressionToken->tokenEnd;
	uint8_t countOfNonEnclosementAttached = 0;
	int16_t tempStorageForRecursiveReturnValue;
	
	for (int16_t i=0;i<tokenLen;i++){
		currentExpressionToken = expressionTokenArray.tokens+i;
		if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58){
			if (targetIndex==i){
				printf("NOPE, go change function parameter chaining for statement in this function to be to be i>=targetIndex instead of i>targetIndex");
			}
			countOfNonEnclosementAttached++;
			if (countOfNonEnclosementAttached>250){
				printf("This expression node has too many things pointing to it (possibly a called function with way too many parameters)\n");
				exit(1);
			}
		}
	}
	if (isOperatorForExpressionTree(thisOperatorID)){
		if (thisOperatorID==17){ // sizeof
			if (targetIndex+1>=expressionTokenArray.len){
				printf("sizeof cannot be last token.\n");
				exit(1);
			}
			if (expressionTokenArray.tokens[targetIndex+1].operatorID==64){
				int16_t matchingParenthese = expressionTokenArray.tokens[targetIndex+1].matchingIndex;
				if (targetIndex+2>=expressionTokenArray.len){
					printf("sizeof parenthese cannot be last token (how did you even do this without failing somewhere else first?).\n");
					exit(1);
				}
				thisExpressionTreeNode->isArgumentTypeForSizeof = expressionTokenArray.tokens[targetIndex+2].operatorID==60;
				if (thisExpressionTreeNode->isArgumentTypeForSizeof){
					thisExpressionTreeNode->argumentIndexStart = expressionTokenArray.tokens[targetIndex+2].tokenStart;
					thisExpressionTreeNode->argumentIndexEnd = expressionTokenArray.tokens[matchingParenthese-1].tokenEnd;
					return indexOfThisExpressionTreeNode;
				} else {
					if (countOfNonEnclosementAttached!=2){
						printf("sizeof without type inside with parentheses has wrong number of attachments to it\'s token\n");
						exit(1);
					}
					for (int16_t i=targetIndex+2;i<matchingParenthese;i++){
						currentExpressionToken = expressionTokenArray.tokens+i;
						if (currentExpressionToken->operatorTargetRoot==targetIndex && currentExpressionToken->operatorID!=58){
							tempStorageForRecursiveReturnValue = generateSpecificNodeForExpressionTree(i);
							thisExpressionTreeNode = &(expressionTreeGlobalBuffer.expressionTreeNodes[indexOfThisExpressionTreeNode]);
							thisExpressionTreeNode->indexOfComponent1 = tempStorageForRecursiveReturnValue;
							return indexOfThisExpressionTreeNode;
						}
					}
					printf("Didn\'t find the indicated attached token in bounds for sizeof parentheses.\n");
					exit(1);
				}
			} else {
				if (countOfNonEnclosementAttached!=1){
					printf("sizeof without type inside without parentheses has wrong number of attachments to it\'s token\n");
					exit(1);
				}
				for (int16_t i=targetIndex+1;i<tokenLen;i++){
					currentExpressionToken = expressionTokenArray.tokens+i;
					if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58){
						tempStorageForRecursiveReturnValue = generateSpecificNodeForExpressionTree(i);
						thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
						thisExpressionTreeNode->indexOfComponent1 = tempStorageForRecursiveReturnValue;
						return indexOfThisExpressionTreeNode;
					}
				}
				printf("Didn\'t find the indicated attached token in bounds for sizeof.\n");
				exit(1);
			}
		} else if (thisOperatorID==14){ // type cast
			int16_t matchingParenthese = expressionTokenArray.tokens[targetIndex].matchingIndex;
			thisExpressionTreeNode->argumentIndexStart = expressionTokenArray.tokens[targetIndex+1].tokenStart; // that shouldn't fail
			thisExpressionTreeNode->argumentIndexEnd = expressionTokenArray.tokens[matchingParenthese-1].tokenEnd;
			uint8_t alternateAttachCount=0;
			int16_t indexForComponent1;
			for (int16_t i=0;i<tokenLen;i++){
				currentExpressionToken = expressionTokenArray.tokens+i;
				if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58 & (i<targetIndex | i>matchingParenthese)){
					alternateAttachCount++;
					indexForComponent1=i;
				}
			}
			if (alternateAttachCount!=1){
				printf("type cast doesn\'t have single attachment\n");
				exit(1);
			}
			tempStorageForRecursiveReturnValue = generateSpecificNodeForExpressionTree(indexForComponent1);
			thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
			thisExpressionTreeNode->indexOfComponent1 = tempStorageForRecursiveReturnValue;
			return indexOfThisExpressionTreeNode;
		} else if (thisOperatorID==65 | thisOperatorID==66){ // function call
			bool hasChainBeenStarted = false;
			bool hasSeperatorOccured = true; // we start with true because function calls have one less comma then parameters
			int16_t lastItemInChain;
			int16_t tempIndex;
			ExpressionTreeNode *tempPtr;
			for (int16_t i=tokenLen-1;i>targetIndex;i--){ 
				// we go backwards to get the chain going forwards (so we can push in an order that more easily gives us the ability to do variable argument functions, because converting to instructions runs the chain backwards)
				// also, we start at this index because of the dual sided function calls, where we don't want to add the operator to the left as a parameter
				currentExpressionToken = expressionTokenArray.tokens+i;
				if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58 & currentExpressionToken->operatorID!=3){
					if (currentExpressionToken->operatorID==50){
						if (hasSeperatorOccured){
							if (hasChainBeenStarted){
								printInformativeMessageAtSourceContainerIndex(
									true,"A seperator comma cannot exist directly next to the closing parentheses of a function call",currentExpressionToken->tokenStart,0);
								exit(1);
							} else {
								printf("Two seperator commas without a parameter? That is an error.\n");
								exit(1);
							}
						}
						hasSeperatorOccured = true;
					} else {
						if (!hasSeperatorOccured){
							printf("(%d)",(int)currentExpressionToken->operatorID);
							printf("Two parameters without a seperating comma in between them? That is an error.\n");
							exit(1);
						}
						hasSeperatorOccured = false;
						if (hasChainBeenStarted){
							tempIndex = generateSpecificNodeForExpressionTree(i);
							tempPtr = expressionTreeGlobalBuffer.expressionTreeNodes+tempIndex;
							tempPtr->hasNextChainElement = true;
							tempPtr->nextInChain = lastItemInChain;
							lastItemInChain = tempIndex;
						} else {
							lastItemInChain = generateSpecificNodeForExpressionTree(i);
						}
						hasChainBeenStarted = true;
					}
				}
			}
			thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
			if (hasChainBeenStarted){
				if (hasSeperatorOccured){
					printf("A seperator comma cannot exist directly next to the opening parentheses of a function call.\n");
					exit(1);
				}
				thisExpressionTreeNode->indexOfComponent1 = lastItemInChain;
			} else {
				thisExpressionTreeNode->isEndNode = true;
			}
			if (thisOperatorID==66){
				// if this is a function call where the name is not known,
				// then we need to add the expression that gives us the address of the function
				int16_t indexForLeft;
				bool hasFoundIndexForLeft = false;
				for (int16_t i=0;i<targetIndex;i++){
					// we don't want to add the parameters, so we stop at the index of this 
					currentExpressionToken = expressionTokenArray.tokens+i;
					if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58){
						if (hasFoundIndexForLeft){
							printf("That is wrong 1\n"); // I don't feel like making an error message right now, this shouldn't happen...
							exit(1);
						} else {
							indexForLeft = i;
							hasFoundIndexForLeft = true;
						}
					}
				}
				tempStorageForRecursiveReturnValue = generateSpecificNodeForExpressionTree(indexForLeft);
				thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
				thisExpressionTreeNode->indexOfComponent2 = tempStorageForRecursiveReturnValue;
			} else {
				// lets just make sure there are no tokens that say they are attached to this but should not be
				// we don't want to add the parameters, so we stop at the index of this
				for (int16_t i=0;i<targetIndex;i++){ 
					currentExpressionToken = expressionTokenArray.tokens+i;
					if (currentExpressionToken->operatorTargetRoot==targetIndex){
						printf("That is wrong 2\n"); // I don't feel like making an error message right now, this shouldn't happen...
						exit(1);
					}
				}
			}
			return indexOfThisExpressionTreeNode;
		} else if (isOneOperandOperatorForExpressionTree(thisOperatorID)){ // detect operators that should have one operand
			if (countOfNonEnclosementAttached==1){
				// good, it has the proper number of operators
				for (int16_t i=0;i<tokenLen;i++){
					currentExpressionToken = expressionTokenArray.tokens+i;
					if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58){
						tempStorageForRecursiveReturnValue = generateSpecificNodeForExpressionTree(i);
						thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
						thisExpressionTreeNode->indexOfComponent1 = tempStorageForRecursiveReturnValue;
						return indexOfThisExpressionTreeNode;
					}
				}
			} else {
				printf("That operator should have had one operand.\n");
				exit(1);
			}
		} else if (isTwoOperandOperatorForExpressionTree(thisOperatorID)){ // detect operators that should have two operands
			if (countOfNonEnclosementAttached==2){
				// good, it has the proper number of operators
				bool isFirst = true;
				for (int16_t i=0;i<tokenLen;i++){
					currentExpressionToken = expressionTokenArray.tokens+i;
					if (currentExpressionToken->operatorTargetRoot==targetIndex & currentExpressionToken->operatorID!=58){
						int16_t indexOfComponent = generateSpecificNodeForExpressionTree(i);
						thisExpressionTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+indexOfThisExpressionTreeNode;
						if (isFirst){
							isFirst = false;
							thisExpressionTreeNode->indexOfComponent1 = indexOfComponent;
						} else {
							thisExpressionTreeNode->indexOfComponent2 = indexOfComponent;
							return indexOfThisExpressionTreeNode;
						}
					}
				}
				printf("Internal Error: That operator said it had two operands, but then they were not detected\n");
				exit(1);
			} else {
				printf("That operator should have had two operands.\n");
				exit(1);
			}
		} else {
			printf("Internal Error: Expression tree generator got invalid target.\n");
			exit(1);
		}
	} else {
		thisExpressionTreeNode->isEndNode = true;
		return indexOfThisExpressionTreeNode;
	}
	printf("Internal Error: This should be unreachable code space.\n");
	exit(1);
}

// returns index in expression tree of root
int16_t generateExpressionTree(){
	int16_t tokenLen=expressionTokenArray.len;
	int16_t rootIndex;
	bool hasFoundRoot = false;
	for (int16_t i=0;i<tokenLen;i++){
		if (expressionTokenArray.tokens[i].operatorTargetRoot==-2){
			if (hasFoundRoot){
				printf("generateExpressionTree() found two roots to an expression.\n");
				exit(1);
			} else {
				hasFoundRoot = true;
				rootIndex = generateSpecificNodeForExpressionTree(i);
			}
		}
	}
	if (!hasFoundRoot){
		printf("generateExpressionTree() could not find root.\n");
		exit(1);
	}
	return rootIndex;
}



void resetForOperatorTargetRoot(){
	int16_t tokenLen=expressionTokenArray.len;
	int16_t i;
	ExpressionToken *expressionToken;
	for (i=0;i<tokenLen;i++){
		expressionTokenArray.tokens[i].operatorTargetRoot=-3;
	}
	for (i=0;i<tokenLen;i++){
		expressionToken = expressionTokenArray.tokens+i;
		if (expressionToken->precedenceTotal==1){
			if (isOperatorForExpressionTree(expressionToken->operatorID)){
				expressionToken->operatorTargetRoot=-2;
				i++;
				break;
			}
		}
	}
	for (;i<tokenLen;i++){
		expressionToken = expressionTokenArray.tokens+i;
		if (expressionToken->precedenceTotal==1){
			if (isOperatorForExpressionTree(expressionToken->operatorID)){
				printf("Multiple roots of expression is wrong (the expression must be malformed).\n");
				exit(1);
			}
		}
		
	}
}




/*
generateOperatorTargetRoot initialize operatorTargetRoot to a proper value 


operatorTargetRoot is a valid index (in the token list) when not negitive, -2 when the token is the root operator. (-3 is used for a reset state)
when isOperatorForExpressionTree()==true for the token,  operatorTargetRoot gives the index of the token containing the operator that holds this operator as it's operand.
when isOperatorForExpressionTree()==false for the token, operatorTargetRoot gives the index of the token that holds this token as it's operand (which may not be an operator, but usually is).

Also, it expects operatorTargetRoot==-3 for all tokens, except that it also expects the root token (at precedence 1) to have operatorTargetRoot==-2
when started, it should be started with startIndex==0 , previousOperatorIndex==-2 , precedenceLevel==1
*/
void generateOperatorTargetRoot(int16_t startIndex, int16_t previousOperatorIndex, int16_t precedenceLevel){
	int16_t tokenLen=expressionTokenArray.len;
	int16_t thisOperatorIndex = -3;
	int16_t modifiedStartIndex = startIndex;
	int16_t i = startIndex;
	while (i<tokenLen && expressionTokenArray.tokens[i].precedenceTotal!=precedenceLevel){
		modifiedStartIndex = ++i;
	}
	i=modifiedStartIndex;
	while (i<tokenLen & i>=0){
		if (isOperatorForExpressionTree(expressionTokenArray.tokens[i].operatorID)){
			thisOperatorIndex = i;
			expressionTokenArray.tokens[i].operatorTargetRoot = previousOperatorIndex;
			i=expressionTokenArray.tokens[i].nextTokenIndexWithMatchingPrecedence; // this advancement is for the next while loop (which checks for an error)
			break;
		}
		i=expressionTokenArray.tokens[i].nextTokenIndexWithMatchingPrecedence;
	}
	if (thisOperatorIndex==-3){
		printf("generateOperatorTargetRoot() failed to find operator on precedenceLevel==%d\n",precedenceLevel);
		exit(1);
	}
	while (i<tokenLen & i>=0){
		if (isOperatorForExpressionTree(expressionTokenArray.tokens[i].operatorID)){
			printf("generateOperatorTargetRoot() failed with multiple operators on same precedenceLevel , precedenceLevel==%d\n",precedenceLevel);
			exit(1);
		}
		i=expressionTokenArray.tokens[i].nextTokenIndexWithMatchingPrecedence;
	}
	i=modifiedStartIndex;
	while (i<tokenLen & i>=0){
		if (!isOperatorForExpressionTree(expressionTokenArray.tokens[i].operatorID)){
			expressionTokenArray.tokens[i].operatorTargetRoot = thisOperatorIndex;
		}
		i=expressionTokenArray.tokens[i].nextTokenIndexWithMatchingPrecedence;
	}
	if (expressionTokenArray.tokens[startIndex].precedenceTotal>precedenceLevel){
		generateOperatorTargetRoot(startIndex,thisOperatorIndex,precedenceLevel+1); // now that we know where this operator is, we can recurse
	}
	int16_t nextI;
	i=modifiedStartIndex;
	while (i<expressionTokenArray.len & i>=0){
		nextI=expressionTokenArray.tokens[i].nextTokenIndexWithMatchingPrecedence;
		if (nextI!=i+1 & nextI!=-1 & nextI!=-3){
			generateOperatorTargetRoot(i+1,thisOperatorIndex,precedenceLevel+1);
		}
		i=nextI;
	}
}


/*
nextTokenIndexWithMatchingPrecedence is typically an index of the next token with the same precedence (that is aware of possible dips that seperate operators)

in addition, 
if nextTokenIndexWithMatchingPrecedence==-1 , there is no next token with the same precedence
if nextTokenIndexWithMatchingPrecedence==-2 , there is no next token with the same precedence, but the next token's precedence goes up (so it is attached, but is of a higher precedence)
if nextTokenIndexWithMatchingPrecedence==-3 , this is the last token, so it definatly has no next token

nextTokenIndexWithMatchingPrecedence==-2 is generated from nextTokenIndexWithMatchingPrecedence==-1 after generateNextTokensUsingPrecedence() runs

*/
int16_t generateNextTokensUsingPrecedence(int16_t startIndex, int16_t precedenceLevel){
	int16_t tokenLen=expressionTokenArray.len;
	int16_t previousI = -2;
	for (int16_t i=startIndex;i<tokenLen;i++){
		if (expressionTokenArray.tokens[i].precedenceTotal>precedenceLevel){
			i = generateNextTokensUsingPrecedence(i,precedenceLevel+1);
			if (i==-1){
				if (previousI!=-2){
					expressionTokenArray.tokens[previousI].nextTokenIndexWithMatchingPrecedence = -1;
				}
				return -1;
			} else if (expressionTokenArray.tokens[i].precedenceTotal>precedenceLevel){
				printf("generateNextTokensUsingPrecedence() sanity check fail (1)\n");
				exit(1);
			}
		}
		if (expressionTokenArray.tokens[i].precedenceTotal<precedenceLevel){
			if (previousI!=-2){
				expressionTokenArray.tokens[previousI].nextTokenIndexWithMatchingPrecedence = -1;
			}
			return i;
		}
		if (previousI!=-2){
			expressionTokenArray.tokens[previousI].nextTokenIndexWithMatchingPrecedence = i;
		}
		previousI = i;
	}
	return -1;
}

void generateAllNextTokensUsingPrecedence(){
	generateNextTokensUsingPrecedence(0,1);
	expressionTokenArray.tokens[expressionTokenArray.len-1].nextTokenIndexWithMatchingPrecedence = -3;
	int16_t tokenLen=expressionTokenArray.len;
	for (int16_t i=0;i<tokenLen-1;i++){
		if (expressionTokenArray.tokens[i  ].nextTokenIndexWithMatchingPrecedence==-1 & 
			expressionTokenArray.tokens[i  ].precedenceTotal <
			expressionTokenArray.tokens[i+1].precedenceTotal){
			
			expressionTokenArray.tokens[i  ].nextTokenIndexWithMatchingPrecedence=-2;
		}
	}
}


#ifdef COMPILE_EXP_DEBUG_PRINTOUT

void printLinesForDebugType2(int16_t numberOfSpaces){
	printf("  ");
	for (int16_t i=0;i<numberOfSpaces;i++){
		if (i%2==0){
			printf(" ");
		} else {
			printf("|");
		}
	}
}

// this function was made before the abstractions to make walking the tree easier
void debugPrintOfExpressionTreeFromTarget(char *string, int16_t targetIndex, int16_t currentNumberOfPrependSpaces){
	ExpressionTreeNode *exTreeNode = expressionTreeGlobalBuffer.expressionTreeNodes+targetIndex;
	uint8_t thisOperatorID = exTreeNode->operatorID;
	char *tempString = copyStringSegmentToHeap(string,exTreeNode->startIndexInString,exTreeNode->endIndexInString);
	if (thisOperatorID==17){ // sizeof
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("%s ",tempString);
		if (exTreeNode->isArgumentTypeForSizeof){
			printf("{TYPE}\n");
		} else {
			printf("{EXP}\n");
		}
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" \\\n");
		if (exTreeNode->isArgumentTypeForSizeof){
			char *tempString2 = copyStringSegmentToHeap(string,exTreeNode->argumentIndexStart,exTreeNode->argumentIndexEnd);
			printLinesForDebugType2(currentNumberOfPrependSpaces+2);
			printf("\"%s\"",tempString2);
			cosmic_free(tempString2);
		} else {
			debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
		}
	} else if (thisOperatorID==14){ // type cast
		char *tempString2 = copyStringSegmentToHeap(string,exTreeNode->argumentIndexStart,exTreeNode->argumentIndexEnd);
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("(%s)\n",tempString2);
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" \\\n");
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
		cosmic_free(tempString2);
		
	} else if (thisOperatorID==65){ // function call (with name)
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("%s",tempString);
		if (exTreeNode->isEndNode){
			// then this function call has no arguments
			printf("()\n");
		} else {
			printf("\n");
			printLinesForDebugType2(currentNumberOfPrependSpaces);
			printf("\\\\\n");
			debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
		}
	} else if (thisOperatorID==66){ // function call (no name)
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent2,currentNumberOfPrependSpaces+2); // for complex function call, the left side is in component2, which is different from usual
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" /\n");
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("()\n");
		if (!exTreeNode->isEndNode){
			// then this function call has arguments
			printLinesForDebugType2(currentNumberOfPrependSpaces);
			printf("\\\\\n");
			debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
		}
	} else if (isOneOperandOperatorForExpressionTree(thisOperatorID)){
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("%s\n",tempString);
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" \\\n");
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
	} else if (isTwoOperandOperatorForExpressionTree(thisOperatorID)){
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent1,currentNumberOfPrependSpaces+2);
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" /\n");
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("%s\n",tempString);
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf(" \\\n");
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->indexOfComponent2,currentNumberOfPrependSpaces+2);
	} else {
		printLinesForDebugType2(currentNumberOfPrependSpaces);
		printf("%s\n",tempString);
	}
	if (exTreeNode->hasNextChainElement){
		printLinesForDebugType2(currentNumberOfPrependSpaces-1);
		printf("!\n");
		debugPrintOfExpressionTreeFromTarget(string,exTreeNode->nextInChain,currentNumberOfPrependSpaces);
	}
	cosmic_free(tempString);
}

#endif


void clearPreviousExpressions(){
	expressionTreeGlobalBuffer.walkingIndexForNextOpenSlot = 0; // this effectivly clears any previous expressions.
	
	
	// this next loop is for helping valgrind. It is not needed.
#ifdef FORCE_EXP_BUFFER_NULL
	const ExpressionTreeNode n={0};
	for (int16_t ti=0;ti<expressionTreeGlobalBuffer.numberOfNodesAllocatedToArray;ti++){
		expressionTreeGlobalBuffer.expressionTreeNodes[ti]=n;
	}
#endif
}





/*
this will be the public function that gets used most
returns -1 if there is no expression to be processed there
*/
int16_t buildExpressionTreeToGlobalBufferAndReturnRootIndex(int32_t startIndexInString, int32_t endIndexInString, bool doClearPreviousExpressions){
	if (startIndexInString>=endIndexInString ||
		(sourceContainer.string[startIndexInString]==' ' & startIndexInString+1>=endIndexInString)){
#ifdef COMPILE_EXP_DEBUG_PRINTOUT
		printf("Expression parser debug ::\n  no expression to parse, so no further information is given.\n");
#endif
		return -1;
	}
	generateTokenArrayForExpression(startIndexInString,endIndexInString);
	matchEnclosementsOnExpressionTokenArray();
	if (expressionTokenArray.tokens[0].isOpenEnclosement==true && expressionTokenArray.tokens[0].matchingIndex==expressionTokenArray.len-1){
		// then the entire expression is in parentheses. This messes with some of the other algorithms used below, so lets try again without those unneeded parentheses.
#ifdef COMPILE_EXP_DEBUG_PRINTOUT
		{
		char *tempString = copyStringSegmentToHeap(sourceContainer.string,startIndexInString,endIndexInString);
		printf("Expression parser debug on \"%s\" ::\n  expression was surrounded in parentheses, so a reparse is required.\n",tempString);
		cosmic_free(tempString);
		}
#endif
		int32_t newStartIndex = expressionTokenArray.tokens[0].tokenEnd;
		int32_t newEndIndex = expressionTokenArray.tokens[expressionTokenArray.len-1].tokenStart;
		return buildExpressionTreeToGlobalBufferAndReturnRootIndex(newStartIndex,newEndIndex,doClearPreviousExpressions);
	}
	generatePrecedenceTotal();
	if (expressionTokenArray.len==1 & expressionTokenArray.tokens[0].operatorID>=59 & expressionTokenArray.tokens[0].operatorID<=62){
		/*
		The algorithms used later on in this function to build the tree don't work for expressions without operators. 
		And if it doesn't have operators, then it's just a single node on the tree. 
		So let's just make that single tree node here, and that is all that needs to be done to build the expression tree.
		*/
#ifdef COMPILE_EXP_DEBUG_PRINTOUT
		{
		char *tempString = copyStringSegmentToHeap(sourceContainer.string,startIndexInString,endIndexInString);
		printf("Expression parser debug on \"%s\" ::\n  expression was a single node, so no further information is given.\n",tempString);
		cosmic_free(tempString);
		}
#endif
		int16_t treeIndexForSingle = partitionNewExpressionTreeNodeInGlobalBufferAndReturnIndex();
		
		ExpressionTreeNode expressionTreeNodeForSingle = {0};
		ExpressionToken expressionTokenOfSingle = expressionTokenArray.tokens[0];
		expressionTreeNodeForSingle.startIndexInString = expressionTokenOfSingle.tokenStart;
		expressionTreeNodeForSingle.endIndexInString = expressionTokenOfSingle.tokenEnd;
		expressionTreeNodeForSingle.operatorID = expressionTokenOfSingle.operatorID;
		expressionTreeNodeForSingle.isEndNode = true;
		expressionTreeGlobalBuffer.expressionTreeNodes[treeIndexForSingle] = expressionTreeNodeForSingle;
		genAllPreWalkData(treeIndexForSingle);
		return treeIndexForSingle;
	}
	
#ifdef COMPILE_EXP_DEBUG_PRINTOUT
	{
	char *tempString = copyStringSegmentToHeap(sourceContainer.string,startIndexInString,endIndexInString);
	printf("Expression parser debug on \"%s\" ::\n",tempString);
	cosmic_free(tempString);
	}
	printf("  Operation on tokens partially completed (1/3). Partial results follow:\n");
	for (int16_t i=0;i<expressionTokenArray.length;i++){
		char *tokenStringContents = copyStringSegmentToHeap(
			sourceContainer.string,
			expressionTokenArray.tokens[i].tokenStart,
			expressionTokenArray.tokens[i].tokenEnd);
		ExpressionToken token = expressionTokenArray.tokens[i];
		printf("    Token %3d has precedence %2d:%2d:%2d, catagory %2d, isOperator %1d , and contents \"%s\"\n",
			i,
			token.precedenceTotal,
			token.precedenceToAddImmediatelyBeforeToken,
			token.precedenceToRemoveImmediatelyAfterToken,
			token.operatorID,
			isOperatorForExpressionTree(token.operatorID),
			tokenStringContents);
		cosmic_free(tokenStringContents);
	}
#endif


	checkForZeroPrecedenceInPrecedenceTotal();
	generateAllNextTokensUsingPrecedence();
	
#ifdef COMPILE_EXP_DEBUG_PRINTOUT
	
	printf("  Operation on tokens partially completed (2/3). Partial results follow:\n");
	for (int16_t i=0;i<expressionTokenArray.length;i++){
		char *tokenStringContents = copyStringSegmentToHeap(
			sourceContainer.string,
			expressionTokenArray.tokens[i].tokenStart,
			expressionTokenArray.tokens[i].tokenEnd);
		ExpressionToken token = expressionTokenArray.tokens[i];
		printf("    Token %3d has precedence %2d:%2d:%2d, catagory %2d, isOperator %1d , nextWithPrecMatch %3d, and contents \"%s\"\n",
			i,
			token.precedenceTotal,
			token.precedenceToAddImmediatelyBeforeToken,
			token.precedenceToRemoveImmediatelyAfterToken,
			token.operatorID,
			isOperatorForExpressionTree(token.operatorID),
			token.nextTokenIndexWithMatchingPrecedence,
			tokenStringContents);
		cosmic_free(tokenStringContents);
	}
#endif

	resetForOperatorTargetRoot();
	generateOperatorTargetRoot(0,-2,1);
	
	if (doClearPreviousExpressions){
		clearPreviousExpressions();
	}
	
	int16_t rootIndex = generateExpressionTree();

#ifdef COMPILE_EXP_DEBUG_PRINTOUT
	
	printf("  Operation on tokens completed (3/3). Full results follow:\n");
	for (int16_t i=0;i<expressionTokenArray.length;i++){
		char *tokenStringContents = copyStringSegmentToHeap(
		sourceContainer.string,
		expressionTokenArray.tokens[i].tokenStart,
		expressionTokenArray.tokens[i].tokenEnd);
		ExpressionToken token = expressionTokenArray.tokens[i];
		printf("    Token %3d has precedence %2d:%2d:%2d, catagory %2d, operatorTargetRoot %3d, isOperator %1d , nextWithPrecMatch %3d, and contents \"%s\"\n",
			i,
			token.precedenceTotal,
			token.precedenceToAddImmediatelyBeforeToken,
			token.precedenceToRemoveImmediatelyAfterToken,
			token.operatorID,
			token.operatorTargetRoot,
			isOperatorForExpressionTree(token.operatorID),
			token.nextTokenIndexWithMatchingPrecedence,
			tokenStringContents);
		cosmic_free(tokenStringContents);
	}
	printf("\n  Expression tree printout:\n");
	debugPrintOfExpressionTreeFromTarget(sourceContainer.string,rootIndex,0);
	printf("\n  End of printout for this expression.\n\n");
#endif

	genAllPreWalkData(rootIndex);
	return rootIndex;
}



#ifdef COMPILE_ONLY_EXP_DEBUG

void runTestOnString(char *testString){
	sourceContainer.string=testString;
	int16_t rootIndex = buildExpressionTreeToGlobalBufferAndReturnRootIndex(0,strlen(testString)-1,true);
	//printf("rootIndex was %d\n\n",rootIndex);
}

int main(){
	printf("Starting...\n\n");
	
	
	runTestOnString("num1=num2==num3+++2*7;");
	runTestOnString("num1.num2[i]=num3*5.5*.5;");
	runTestOnString("name1=name2*name3(6+name4,name7)+7*n5;");
	runTestOnString("num=sizeof(int);");
	runTestOnString("num=sizeof(long int i);");
	runTestOnString("num=sizeof num1+4;");
	runTestOnString("num=sizeof(num1+4);");
	runTestOnString("num=function(num2+3)*7+5;");
	runTestOnString("num=array[i+1]+function(num2+3)*7+5;");
	runTestOnString("num=str.array[i+1]+function(num2+3)*7+5;");
	runTestOnString("num1=str.array[i+1][i+2][i]+function(num2+3,9*num3)*7+5;");
	runTestOnString("num=(int)num1;");
	runTestOnString("num=(long int)num1;");
	runTestOnString("num=(int)num1+4;");
	runTestOnString("num=(int)(num1+4);");
	runTestOnString("(num=(int)(num1+4));");
	runTestOnString("n=f();");
	runTestOnString("n=f()+4;");
	runTestOnString("f();");
	runTestOnString("s.n=c?f:t;");
	runTestOnString("a ? b: c ? d : e ? f : g ? h : i;");
	runTestOnString("a ? b ? c : d : e;");
	
	
	
	runTestOnString("n=(*funPtr)(parem);");
	runTestOnString("(*funPtr)(parem);");
	runTestOnString("(**funPtr)(parem);");
	
	printf("Done\n");
}
#endif



