
/*
this file is included inside BlockFrame.c
*/

// returns 0 if there is an error (which includes trying to get the size of "void")
// does not modify input
uint32_t getSizeofForTypeString(const char* typeStringIn, bool failIfHasIdentifier){
	uint32_t ret=0;
	char* stringInternal = copyStringToHeapString(typeStringIn);
	if (doesThisTypeStringHaveAnIdentifierAtBeginning(stringInternal)){
		if (failIfHasIdentifier){
			err_00__1(strMerge3(
				"sizeof failed on type `",
				stringInternal,
				"` by \"had identifier with identifier rejection\""));
			goto End;
		}
		applyToTypeStringRemoveIdentifierToSelf(stringInternal);
	}
	// if it has an identifier, then it is now removed
	{
		bool hadVolatile;
		bool hadConst;
		char* stringInternalNQ = stripQualifiers(stringInternal,&hadVolatile,&hadConst);
		if (hadVolatile | hadConst){
			ret=getSizeofForTypeString(stringInternalNQ,true);
			goto End;
		}
	}
	if (getIndexOfNthSpace(stringInternal,0)==-1){
		if (doStringsMatch(stringInternal,"int") ||
			doStringsMatch(stringInternal,"short")){
			ret=2;goto End;
		}
		if (doStringsMatch(stringInternal,"_Bool") ||
			doStringsMatch(stringInternal,"char")){
			ret=1;goto End;
		}
		if (doStringsMatch(stringInternal,"long")){
			ret=4;goto End;
		}
		if (doStringsMatch(stringInternal,"float")){
			ret=4;goto End;
		}
		if (doStringsMatch(stringInternal,"double")){
			ret=8;goto End;
		}
		if (doStringsMatch(stringInternal,"void")){
			err_00__0("sizeof failed because getting the size of `void` is not allowed");
			goto End; // void gives the error value of 0
		}
	} else {
		if (isSectionOfStringEquivalent(stringInternal,0,"* ")){
			ret=4;goto End; // pointers are always 4
		}
		if (isSectionOfStringEquivalent(stringInternal,0,"( ")){
			ret=4;goto End; // not sure when this would come up, but I think that should be 4 (same as pointer)
		}
		if (isSectionOfStringEquivalent(stringInternal,0,"[ ")){
			int32_t indexOfOtherBrace = getIndexOfMatchingEnclosement(stringInternal,0);
			if (indexOfOtherBrace<0 | indexOfOtherBrace==2 | (indexOfOtherBrace+2)>=strlen(stringInternal)){
				// indexOfOtherBrace<0 is if the bracket match failed
				// indexOfOtherBrace==2 is if there is nothing inside the brackets
				// (indexOfOtherBrace+2)>=strlen(stringInternal) is if there is nothing after the brackets
				err_00__1(strMerge3(
					"sizeof failed on type `",
					stringInternal,
					"` by \"array brackets invalid\""));
				goto End;
			}
			struct NumberParseResult numberParseResult;
			parseNumber(&numberParseResult,stringInternal,2,indexOfOtherBrace-1);
			if ((numberParseResult.errorCode!=0) | (numberParseResult.typeOfDecimal!=0)){
				// todo: change to hex format for type string array sizes
				printf("Info: sizeof failed on type `%s` by \"unable to parse number for array size\"\n",stringInternal);
				goto End;
			}
			ret = getSizeofForTypeString(stringInternal+(indexOfOtherBrace+2),true) * numberParseResult.valueUnion.value;
			goto End;
		}
		if (isSectionOfStringEquivalent(stringInternal,0,"enum ")){
			ret=2;goto End; // enums are of same size as int
		}
		{
			bool isUnion = isSectionOfStringEquivalent(stringInternal,0,"union ");
			bool isStruct = isSectionOfStringEquivalent(stringInternal,0,"struct ");
			if (isUnion | isStruct){
				uint8_t typeOfThis=isUnion;
				char* stringInternalSkipStart=stringInternal+(6+(1-typeOfThis));
				if (getIndexOfFirstSpaceInString(stringInternalSkipStart)>0){
					// this should not happen. sizeof should not get a struct/union definition either
					err_10_01(strMerge3(
						"sizeof failed on type `",
						stringInternal,
						"` by \"struct/union corruption\""));
					goto End;
				}
				struct TypeSearchResult typeSearchResult;
				searchForType(&typeSearchResult,stringInternalSkipStart,typeOfThis);
				if (!typeSearchResult.didExist){
					err_00__1(strMerge3(
						"sizeof failed on type `",
						stringInternal,
						"` by \"incomplete type\""));
					goto End;
				}
				ret=typeSearchResult.isGlobal?
					blockFrameArray.globalBlockFrame.globalTypeEntries[typeSearchResult.typeEntryIndex].thisSizeof:
					blockFrameArray.entries[typeSearchResult.blockFrameEntryIndex].typeEntries[typeSearchResult.typeEntryIndex].thisSizeof;
				goto End;
			}
		}
		if (doStringsMatch(stringInternal,"signed char")){
			ret=1;goto End;
		}
		if (doStringsMatch(stringInternal,"unsigned int") ||
			doStringsMatch(stringInternal,"unsigned short")){
			ret=2;goto End;
		}
		if (doStringsMatch(stringInternal,"unsigned long")){
			ret=4;goto End;
		}
		if (doStringsMatch(stringInternal,"unsigned long long") ||
			doStringsMatch(stringInternal,"long long")){
			ret=8;goto End;
		}
		if (doStringsMatch(stringInternal,"long double")){
			ret=8;goto End;
		}
	}
	err_10_01(strMerge3(
		"sizeof failed on type `",
		stringInternal,
		"` by \"corruption\""));
	End:
	cosmic_free(stringInternal);
	return ret;
}




