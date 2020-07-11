

// free()'s it's input, string[start] should be the 's' or 'u' for struct or union
char* crackStructUnion(char* string, int32_t start){
	uint8_t type=string[start]=='u';
	assert(string[start]=='s' | string[start]=='u');
	int32_t middle=getIndexOfNthSpace(string+start,0)+start;
	int32_t end=getIndexOfNthSpace(string+start,1);
	if (end==-1) end=strlen(string);
	else end+=start;
	char* name=copyStringSegmentToHeap(string,middle+1,end);
	struct TypeSearchResult tsr;
	searchForType(&tsr,name,type);
	cosmic_free(name);
	if (!tsr.didExist) return NULL;
	struct TypeMemberEntry *arrayOfMemberEntries;
	uint16_t numberOfMemberEntries;
	if (tsr.isGlobal){
		struct GlobalTypeEntry* gte=blockFrameArray.globalBlockFrame.globalTypeEntries+tsr.typeEntryIndex;
		numberOfMemberEntries=gte->numberOfMemberEntries;
		arrayOfMemberEntries=gte->arrayOfMemberEntries;
	} else {
		struct BlockFrameTypeEntry* bfte=blockFrameArray.entries[tsr.blockFrameEntryIndex].typeEntries+tsr.typeEntryIndex;
		numberOfMemberEntries=bfte->numberOfMemberEntries;
		arrayOfMemberEntries=bfte->arrayOfMemberEntries;
	}
	char* workingString=copyStringSegmentToHeap(string,start,end);
	char* t0;
	char* t1;
	char* t2;
	t0=strMerge3(" < ",workingString," { ");
	cosmic_free(workingString);
	workingString=t0;
	for (uint16_t i=0;i<numberOfMemberEntries;i++){
		t0=applyToTypeStringRemoveIdentifierToNew(arrayOfMemberEntries[i].typeString);
		t1=applyToTypeStringGetIdentifierToNew(arrayOfMemberEntries[i].typeString);
		t2=strMerge5(workingString,t0," ",t1," ; ");
		cosmic_free(workingString);
		cosmic_free(t0);
		cosmic_free(t1);
		workingString=t2;
	}
	t0=copyStringSegmentToHeap(string,0,start);
	t1=copyStringSegmentToHeap(string,end,strlen(string));
	cosmic_free(string);
	t2=strMerge4(t0,workingString," } > ",t1);
	cosmic_free(t0);
	cosmic_free(t1);
	cosmic_free(workingString);
	return t2;
}

// runs in place
void crackArraysAndEnum(char* string){
	for (int32_t i=0;string[i];i++){
		if (isSectionOfStringEquivalent(string,i,"[ ]")){
			string[i  ]='?';
			string[i+1]=26;
			string[i+2]=26;
		}
	}
	copyDownForInPlaceEdit(string);
	for (int32_t i=0;string[i];i++){
		if (string[i]=='['){
			int32_t i1=getIndexOfMatchingEnclosement(string,i);
			assert(i1!=-1); // type string corruption
			
			string[i   ]=26;
			string[i+1 ]='#';
			string[i1  ]=26;
			string[i1-1]=26;
		}
	}
	copyDownForInPlaceEdit(string);
	for (int32_t i=0;string[i];i++){
		if (isSectionOfStringEquivalent(string,i,"enum ")){
			int32_t end=getIndexOfNthSpace(string+i,1);
			assert(end!=-1); // type string corruption
			end+=i;
			for (int32_t i1=i;i1<end;i1++) string[i1]=26;
			string[i  ]='i';
			string[i+1]='n';
			string[i+2]='t';
		}
	}
	copyDownForInPlaceEdit(string);
}

void strInPlaceReplaceForTypeCrack(char* string,const char* from,const char to){
	int32_t fromLen=strlen(from);
	for (int32_t i=0;string[i];i++){
		if (string[i]==from[0]){ // this check reduces amount of function call overhead
			if (isSectionOfStringEquivalent(string,i,from)){
				for (int32_t i1=i;i1<i+fromLen;i1++) string[i1]=26;
				string[i  ]=' ';
				string[i+1]=to;
				string[i+2]=' ';
				copyDownForInPlaceEdit(string+i);
			}
		}
	}
}

void crackTypeNames(char* string){
	const char* from[]={
		/*z*/" unsigned long long ",
		/*x*/" unsigned long ",
		/*c*/" unsigned int ",
		/*v*/" signed char ",
		/*b*/" long long ",
		/*n*/" long ",
		/*m*/" int ",
		/*l*/" char ",
		/*k*/" _Bool ",
		/*j*/" struct ",
		/*h*/" union ",
		/*g*/" const ",
		/*f*/" volatile ",
		/*d*/" void "
	};
	const char to[]={'z','x','c','v','b','n','m','l','k','j','h','g','f','d',0};
	for (uint16_t i=0;to[i];i++){
		strInPlaceReplaceForTypeCrack(string,from[i],to[i]);
	}
}

void removeDupeSpaces(char* string){
	for (int32_t i=1;string[i];i++){
		if (string[i-1]==' ' & string[i]==' '){
			string[i-1]=26;
		}
	}
	copyDownForInPlaceEdit(string);
}

// will always allocate, will never give an identifier in it's return type string
char* advancedCrackedTypeToTypeString(char* crackedType){
	char* sub;
	char* this;
	switch (crackedType[0]){
		case 'z':return copyStringToHeapString("unsigned long long");
		case 'x':return copyStringToHeapString("unsigned long");
		case 'c':return copyStringToHeapString("unsigned int");
		case 'v':return copyStringToHeapString("signed char");
		case 'b':return copyStringToHeapString("long long");
		case 'n':return copyStringToHeapString("long");
		case 'm':return copyStringToHeapString("int");
		case 'l':return copyStringToHeapString("char");
		case 'k':return copyStringToHeapString("_Bool");
		case 'd':return copyStringToHeapString("void");
		case 'g':
		sub=advancedCrackedTypeToTypeString(crackedType+1);
		this=strMerge2("const ",sub);
		cosmic_free(sub);
		return this;
		case 'f':
		sub=advancedCrackedTypeToTypeString(crackedType+1);
		this=strMerge2("volatile ",sub);
		cosmic_free(sub);
		return this;
		case '*':
		sub=advancedCrackedTypeToTypeString(crackedType+1);
		this=strMerge2("* ",sub);
		cosmic_free(sub);
		return this;
		case '#':
		{
		char numberBuffer[14] = {0};
		snprintf(numberBuffer,13,"%lu",(unsigned long)readHexInString(crackedType+1));
		sub=advancedCrackedTypeToTypeString(crackedType+9);
		this=strMerge4("[ ",numberBuffer," ] ",sub);
		cosmic_free(sub);
		return this;
		}
		case 'j':
		case 'h':
		crackedType--;
		case '<':
		{
		bool isStruct=crackedType[1]=='j';
		assert(crackedType[1]=='j' | crackedType[1]=='h');
		uint32_t nameEnd=2;
		while (crackedType[nameEnd]!='{' & crackedType[nameEnd]!='|'){
			assert(crackedType[nameEnd]!=0);
			nameEnd++;
		}
		sub=copyStringSegmentToHeap(crackedType,2,nameEnd);
		this=strMerge2(isStruct?"struct ":"union ",sub);
		cosmic_free(sub);
		return this;
		}
		case '(':
		{
		this=copyStringToHeapString("( ");
		char c;
		bool isFirst=true;
		while ((c=crackedType[1])!=')'){
			{
			assert(c!=0);
			sub=advancedCrackedTypeToTypeString(++crackedType);
			char* temp;
			temp=strMerge4(this,isFirst?" ":" , ",sub," ");
			isFirst=false;
			cosmic_free(this);
			cosmic_free(sub);
			this=temp;
			}
			{
			while ((c=*crackedType)!=','){
				assert(c!=0);
				if (c=='('){
					assert(getIndexOfMatchingEnclosement(crackedType,0)!=-1);
					crackedType=getIndexOfMatchingEnclosement(crackedType,0)+crackedType;
				} else if (c==')'){
					crackedType--;
					break;
				}
				crackedType++;
			}
			}
		}
		sub=strMerge2(this,")");
		cosmic_free(this);
		this=sub;
		removeDupeSpaces(this);
		return this;
		}
	}
	printf("Internal Error: invalid crackedCharacter\n");
	exit(1);
}

char* crackArraySizes(char* string){
	for (int32_t i=0;string[i];i++){
		if (string[i]=='#' | string[i]=='?'){
			int32_t e=i+1;
			uint32_t value=0;
			bool isValueKnown=string[i]=='#';
			if (isValueKnown){
				while (string[e]!=0 & string[e]!=' ') e++;
				for (uint16_t t=i+1;t<e;t++){
					assert(string[t]>='0' & string[t]<='9'); // for decimal numbers as array sizes in type strings
					value*=10;
					value+=string[t]-'0';
				}
			}
			char* t0=copyStringSegmentToHeap(string,0,i+1);
			char* t1=copyStringSegmentToHeap(string,e,strlen(string));
			char* t2=strMerge3(t0,"00000000",t1);
			cosmic_free(string);
			cosmic_free(t0);
			cosmic_free(t1);
			string=t2;
			if (isValueKnown) writeHexInString(string+(i+1),value);
		}
	}
	return string;
}

void removeSpaces(char* string){
	for (int32_t i=0;string[i];i++){
		if (string[i]==' ') string[i]=26;
	}
	copyDownForInPlaceEdit(string);
}

int32_t findUncrackedStructOrUnion(char* string){
	for (int32_t i=0;string[i];i++){
		int32_t p=i-2;
		char c=string[i];
		if (c=='('){
			int32_t functionJump=getIndexOfMatchingEnclosement(string,i);
			assert(functionJump!=-1);
			i=functionJump;
		} else if ((
			c=='s' && isSectionOfStringEquivalent(string,i,"struct ")) || (
			c=='u' && isSectionOfStringEquivalent(string,i,"union "))){
			
			if (i==0 || string[i-1]==' '){
				if (p<0 || (string[p]!='<' & string[p]!='*')){
					return i;
				}
			}
		}
	}
	return -1;
}

void splitUncrackedStructUnion(char** stringPtr){
	Start:;
	char* string=*stringPtr;
	for (int32_t i=0;string[i];i++){
		int32_t p=i-2;
		char c=string[i];
		if ((
			c=='s' && isSectionOfStringEquivalent(string,i,"struct ")) || (
			c=='u' && isSectionOfStringEquivalent(string,i,"union "))){
			
			if (i==0 || string[i-1]==' '){
				if (p>=0 && string[p]!='<'){
					int32_t endCheck=getIndexOfNthSpace(string+i,1);
					assert(endCheck!=-1);
					endCheck+=i;
					if (string[endCheck+1]!='|'){
						char* t0=copyStringSegmentToHeap(string,0,endCheck);
						char* t1=copyStringSegmentToHeap(string,endCheck,strlen(string));
						char* t2=strMerge3(t0," |",t1);
						cosmic_free(string);
						cosmic_free(t0);
						cosmic_free(t1);
						*stringPtr=t2;
						goto Start;
					}
				}
			}
		}
	}
}




void fixFunctionParamsSub(char** stringPtr){
	char* string=*stringPtr;
	for (int32_t i0=0;string[i0];i0++){
		if (string[i0]=='('){
			int32_t functionEnd=getIndexOfMatchingEnclosement(string,i0);
			assert(functionEnd!=-1);
			char* t0=copyStringSegmentToHeap(string,0,i0);
			char* t1=copyStringSegmentToHeap(string,functionEnd+1,strlen(string));
			char* t2=copyStringSegmentToHeap(string,i0,functionEnd+1);
			char* t3=copyStringToHeapString("");
			char* t4;
			char* t5;
			char* t6;
			int32_t prev=1;
			for (int32_t cur=1;t2[cur];cur++){
				char c=t2[cur];
				if (c=='('){
					// this is for function parameters that are function pointers, which causes recursion
					int32_t next=getIndexOfMatchingEnclosement(t2,cur);
					assert(next!=-1);
					char* t7=copyStringSegmentToHeap(t2,0,cur);
					char* t8=copyStringSegmentToHeap(t2,cur,next+1);
					t6=strMerge2(t8," ");
					cosmic_free(t8);
					t8=t6;
					char* t9=copyStringSegmentToHeap(t2,next+1,strlen(t2));
					fixFunctionParamsSub(&t8);
					t6=strMerge3(t7,t8,t9);
					cosmic_free(t7);
					cosmic_free(t8);
					cosmic_free(t9);
					cosmic_free(t2);
					t2=t6;
					cur=next;
				}
			}
			for (int32_t cur=1;t2[cur];cur++){
				char c=t2[cur];
				if (c==',' | c==')'){
					// this performs the identifier removal
					if (prev<cur-1){
						t4=copyStringSegmentToHeap(t2,prev,cur-1);
						while (t4[0]==' '){
							t4[0]=26;
							copyDownForInPlaceEdit(t4);
						}
						uint32_t l;
						while ((l=strlen(t4))!=0 && t4[l-1]==' ') t4[l-1]=0;
						if (t4[0]!=0 && doesThisTypeStringHaveAnIdentifierAtBeginning(t4)){
							applyToTypeStringRemoveIdentifierToSelf(t4);
						}
						if (prev!=1) t5=strMerge5(" ",t3," , ",t4," ");
						else         t5=strMerge3(" ",t4," ");
						cosmic_free(t3);
						cosmic_free(t4);
						t3=t5;
					}
					prev=cur+1;
				} else if (c=='('){
					cur=getIndexOfMatchingEnclosement(t2,cur);
					assert(cur!=-1);
				}
			}
			cosmic_free(t2);
			t6=strMerge5(t0," ( ",t3," ) ",t1);
			cosmic_free(t0);
			cosmic_free(t1);
			cosmic_free(t3);
			cosmic_free(string);
			string=t6;
			*stringPtr=string;
			i0=functionEnd;
		}
	}
}


/*
fixFunctionParams() removes identifiers from function parameters
I think this would work on type strings in addition to it's usage in crackTypeString()
It cannot be used on fully cracked type strings, however.
*/
void fixFunctionParams(char** stringPtr){
	fixFunctionParamsSub(stringPtr);
	removeDupeSpaces(*stringPtr);
}

// returns a new string. it is kinda like a type string, but it is in a very different format.
char* crackTypeString(char* typeStringIn){
	assert(!doesThisTypeStringHaveAnIdentifierAtBeginning(typeStringIn)); // should not get identifiers at the beginning of it's input type string
	char* stringWorking=strMerge3(" ",typeStringIn," ");
	int32_t next;
	while ((next=findUncrackedStructOrUnion(stringWorking))!=-1){
		stringWorking=crackStructUnion(stringWorking,next);
		if (stringWorking==NULL) return NULL;
	}
	fixFunctionParams(&stringWorking);
	splitUncrackedStructUnion(&stringWorking);
	crackArraysAndEnum(stringWorking);
	crackTypeNames(stringWorking);
	stringWorking=crackArraySizes(stringWorking);
	removeSpaces(stringWorking);
	return stringWorking;
}






