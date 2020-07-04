


// free()'s it's input, string[start] should be the 's' or 'u' for struct or union
char* crackStructUnion(char* string, int32_t start){
	uint8_t type;
	if (string[start]=='s'){
		type=0;
	} else if (string[start]=='u'){
		type=1;
	} else {
		printf("Internal Error: crackStructUnion() called wrong\n");
		exit(1);
	}
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
	char* t3;
	char* t4;
	char* t5;
	t0=concatStrings(" < ",workingString);
	cosmic_free(workingString);
	workingString=t0;
	t0=concatStrings(workingString," { ");
	cosmic_free(workingString);
	workingString=t0;
	for (uint16_t i=0;i<numberOfMemberEntries;i++){
		t0=applyToTypeStringRemoveIdentifierToNew(arrayOfMemberEntries[i].typeString);
		t1=applyToTypeStringGetIdentifierToNew(arrayOfMemberEntries[i].typeString);
		t2=concatStrings(t0," ");
		t3=concatStrings(t2,t1);
		t4=concatStrings(t3," ; ");
		t5=concatStrings(workingString,t4);
		cosmic_free(t0);
		cosmic_free(t1);
		cosmic_free(t2);
		cosmic_free(t3);
		cosmic_free(t4);
		cosmic_free(workingString);
		workingString=t5;
	}
	t0=concatStrings(workingString," } ");
	cosmic_free(workingString);
	workingString=t0;
	t0=concatStrings(workingString," > ");
	cosmic_free(workingString);
	workingString=t0;
	t0=copyStringSegmentToHeap(string,0,start);
	t1=copyStringSegmentToHeap(string,end,strlen(string));
	cosmic_free(string);
	t2=concatStrings(t0,workingString);
	cosmic_free(t0);
	cosmic_free(workingString);
	workingString=concatStrings(t2,t1);
	cosmic_free(t2);
	cosmic_free(t1);
	return workingString;
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
			if (i1==-1){
				printf("Internal Error: typeString corruption\n");
				exit(1);
			}
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
			if (end==-1){
				printf("Internal Error: typeString corruption\n");
				exit(1);
			} else {
				end+=i;
			}
			for (int32_t i1=i;i1<end;i1++){
				string[i1]=26;
			}
			string[i  ]='i';
			string[i+1]='n';
			string[i+2]='t';
		}
	}
	copyDownForInPlaceEdit(string);
}

void strInPlaceReplaceForTypeCrack(char* string,const char* from,const char to){
	int32_t fromLen=strlen(from);
	Start:
	for (int32_t i=0;string[i];i++){
		if (isSectionOfStringEquivalent(string,i,from)){
			for (int32_t i1=i;i1<i+fromLen;i1++){
				string[i1]=26;
			}
			string[i  ]=' ';
			string[i+1]=to;
			string[i+2]=' ';
			copyDownForInPlaceEdit(string);
			goto Start;
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
		/*_*/" const ",
		/*_*/" volatile "
	};
	const char to[]={'z','x','c','v','b','n','m','l','k','j','h',26,26,0};
	for (uint16_t i=0;to[i];i++){
		strInPlaceReplaceForTypeCrack(string,from[i],to[i]);
	}
}

const char* crackedTypeToTypeString(char* crackedType,bool* didAllocate){
	char c=*crackedType;
	if (c=='*' | c=='#'){
		if (c=='#'){
			crackedType+=8;
			printf("Internal Warning: using array decay when converting from cracked type to type string (are you initializing a pointer to array?)\n");
		}
		*didAllocate=true;
		bool didSubAllocate;
		const char* subString=crackedTypeToTypeString(crackedType+1,&didSubAllocate);
		char* workingString=concatStrings("* ",subString);
		if (didSubAllocate) cosmic_free((char*)subString);
		return workingString;
	}
	*didAllocate=false;
	switch (c){
		case 'z':return "unsigned long long";
		case 'x':return "unsigned long";
		case 'c':return "unsigned int";
		case 'v':return "signed char";
		case 'b':return "long long";
		case 'n':return "long";
		case 'm':return "int";
		case 'l':return "char";
		case 'k':return "_Bool";
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
				while (string[e]!=0 & string[e]!=' '){
					e++;
				}
				for (uint16_t t=i+1;t<e;t++){
					if (!(string[t]>='0' & string[t]<='9')){
						printf("Internal Error: array size had bad character\n");
						exit(1);
					}
					value*=10;
					value+=string[t]-'0';
				}
			}
			char* t0=copyStringSegmentToHeap(string,0,i+1);
			char* t1=copyStringSegmentToHeap(string,e,strlen(string));
			char* t2=concatStrings(t0,"00000000");
			char* t3=concatStrings(t2,t1);
			cosmic_free(string);
			cosmic_free(t0);
			cosmic_free(t1);
			cosmic_free(t2);
			string=t3;
			if (isValueKnown) writeHexInString(string+(i+1),value);
		}
	}
	return string;
}

void removeSpaces(char* string){
	for (int32_t i=0;string[i];i++){
		if (string[i]==' '){
			string[i]=26;
		}
	}
	copyDownForInPlaceEdit(string);
}


int32_t findUncrackedStructOrUnion(char* string){
	for (int32_t i=0;string[i];i++){
		int32_t p=i-2;
		if (isSectionOfStringEquivalent(string,i,"struct ")){
			if (p<0 || string[p]!='<'){
				return i;
			}
		} else if (isSectionOfStringEquivalent(string,i,"union ")){
			if (p<0 || string[p]!='<'){
				return i;
			}
		}
	}
	return -1;
}


// returns a new string. it is kinda like a type string, but it is in a very different format.
char* crackTypeString(char* typeStringIn){
	assert(!doesThisTypeStringHaveAnIdentifierAtBeginning(typeStringIn)); // should not get identifiers at the beginning of it's input type string
	char* stringWorking=tripleConcatStrings(" ",typeStringIn," ");
	int32_t next;
	while ((next=findUncrackedStructOrUnion(stringWorking))!=-1){
		stringWorking=crackStructUnion(stringWorking,next);
		if (stringWorking==NULL) return NULL;
	}
	crackArraysAndEnum(stringWorking);
	crackTypeNames(stringWorking);
	stringWorking=crackArraySizes(stringWorking);
	removeSpaces(stringWorking);
	for (int32_t i=0;stringWorking[i];i++){
		if (stringWorking[i]=='('){
			printf("Functions not supported in the cracked type format. They may be supported in the future. In other words, do not use an initializer with an array and a function pointer\n");
			exit(1);
		}
	}
	return stringWorking;
}








