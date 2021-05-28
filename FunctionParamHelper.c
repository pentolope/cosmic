

// this file is included in BlockFrame.c

struct FunctionParam{
	char* typeString;
	char* noIdentifierTypeString;
	bool hasIdentifier;
};

struct FunctionTypeAnalysis{
	uint16_t numberOfParameters;
	bool usesVaArgs;
	struct FunctionParam* params;
	char* returnType;
};

// will not return a comma or paren at i
int32_t nextUnshieldedEndForFunctionTypeString(char* typeString, int32_t i){
	char c;
	while ((c=typeString[++i])){
		if (c==',' | c==')'){
			return i;
		} else if (c=='('){
			i=getIndexOfMatchingEnclosement(typeString,i);
		}
	}
	printf("Internal Error: function type string corrupted\n");
	exit(1);
}

// fills the struct it is given
void analyseFunctionTypeString(
		struct FunctionTypeAnalysis* functionTypeAnalysis,
		const char* typeString,
		bool removeSingularVoidArgument){
	
	char* typeStringInternal;
	if (doesThisTypeStringHaveAnIdentifierAtBeginning(typeString)){
		typeStringInternal = applyToTypeStringRemoveIdentifierToNew(typeString);
	} else {
		typeStringInternal = copyStringToHeapString(typeString);
	}
	int32_t parenEnd = getIndexOfMatchingEnclosement(typeStringInternal,0);
	int32_t walkingIndex = 0;
	functionTypeAnalysis->numberOfParameters = 0;
	if (parenEnd!=2){
		while (walkingIndex!=parenEnd){
			walkingIndex = nextUnshieldedEndForFunctionTypeString(typeStringInternal,walkingIndex);
			functionTypeAnalysis->numberOfParameters++;
		}
		functionTypeAnalysis->params = cosmic_calloc(functionTypeAnalysis->numberOfParameters,sizeof(struct FunctionParam));
		walkingIndex = 0;
		for (uint16_t i=0;i<functionTypeAnalysis->numberOfParameters;i++){
			uint32_t endIndex = nextUnshieldedEndForFunctionTypeString(typeStringInternal,walkingIndex);
			char* s = copyStringSegmentToHeap(typeStringInternal,walkingIndex+1,endIndex);
			uint32_t l=strlen(s);
			if (s[l-1]==' ') s[l-1]=26;
			if (s[0]==' ') s[0]=26;
			copyDownForInPlaceEdit(s);
			applyToTypeStringArrayDecayToSelf(s);
			char* t;
			bool b=false;
			if (!doStringsMatch(s,"...") && (b=doesThisTypeStringHaveAnIdentifierAtBeginning(s))){
				t = applyToTypeStringRemoveIdentifierToNew(s);
			} else {
				t = copyStringToHeapString(s);
			}
			functionTypeAnalysis->params[i].typeString = s;
			functionTypeAnalysis->params[i].noIdentifierTypeString = t;
			functionTypeAnalysis->params[i].hasIdentifier = b;
			walkingIndex=endIndex;
		}
	}
	functionTypeAnalysis->returnType = copyStringToHeapString(typeStringInternal+(parenEnd+2));
	cosmic_free(typeStringInternal);
	functionTypeAnalysis->usesVaArgs = functionTypeAnalysis->numberOfParameters!=0 && doStringsMatch(functionTypeAnalysis->params[functionTypeAnalysis->numberOfParameters-1].noIdentifierTypeString,"...");
	if ((removeSingularVoidArgument & functionTypeAnalysis->numberOfParameters==1) &&
		doStringsMatch(functionTypeAnalysis->params[0].noIdentifierTypeString,"void")){
		
		functionTypeAnalysis->numberOfParameters=0;
		cosmic_free(functionTypeAnalysis->params[0].typeString);
		cosmic_free(functionTypeAnalysis->params[0].noIdentifierTypeString);
		cosmic_free(functionTypeAnalysis->params);
	}
}


void destroyFunctionTypeAnalysis(struct FunctionTypeAnalysis* functionTypeAnalysis){
	if (functionTypeAnalysis->numberOfParameters!=0){
		for (uint16_t i=0;i<functionTypeAnalysis->numberOfParameters;i++){
			cosmic_free(functionTypeAnalysis->params[i].typeString);
			cosmic_free(functionTypeAnalysis->params[i].noIdentifierTypeString);
		}
		cosmic_free(functionTypeAnalysis->params);
	}
	cosmic_free(functionTypeAnalysis->returnType);
}






