

#include "Common.c"

// this file is to have the stuff in it for managing raw data (and being able to save space when parts are the same and unmodifiable)



struct DataLiteralEntries{
	struct DataLiteralEntry{
		uint32_t label;
		int32_t startIndex; // this refers to the start index in the source string (it will not include the L for wide string literals)
		bool isInitializer; // this refers to {} and never strings
		bool isModifiable;
		bool isWideStringLiteral;
		uint32_t lengthOfData; // in bytes, may not be word alligned
		uint16_t* data;
		// DON'T DO THE COMMENT BELOW
		/*
		for data, each element is one byte,
		(data[i]&0x00FF) will get the i'th byte
		((data[i]&0x0100)!=0) will get if the i'th byte has been set
		((data[i]&0x0200)!=0) will get if the i'th byte is modifiable
		((data[i]&0x0400)!=0) will get if the i'th byte must be word alligned
		((data[i]&0x0800)!=0) will get if the i'th byte is part of a 4 byte sequence of a symbolic label number
		*/		
	}* entries;
	int32_t numberOfValidEntries;
	int32_t numberOfAllocatedSlots;
} globalDataLiteralEntries;


// only one of asOctal and asHex should be true, obviously
// returns 200 if the character is not in range or neither boolean was true
uint8_t ucharAsciiAsNumberDigit(uint8_t character, bool asOctal, bool asHex){
	if (asOctal){
		if ((character>='0') & (character<='8')) return character-'0';
		else return 200;
	} else if (asHex){
		if ((character>='0') & (character<='9')) return character-'0';
		else if ((character>='a') & (character<='f')) return (character-'a')+10;
		else if ((character>='A') & (character<='F')) return (character-'A')+10;
		else return 200;
	}
	return 200;
}

// does include a null terminator
uint32_t findLengthOfDataOfStringLiteral(int32_t indexOfFirstQuote, int32_t indexOfLastQuote, bool isWideLiteral){
	if (isWideLiteral){
		printf("Wide literals not completely supported yet\n");
		exit(1);
	}
	uint32_t numberOfCharactersLong = indexOfLastQuote-indexOfFirstQuote;
	for (int32_t i=indexOfFirstQuote+1;i<indexOfLastQuote;i++){
		uint8_t c0 = (uint8_t)(sourceContainer.string[i]);
		if (c0=='\\'){
			uint8_t c1 = (uint8_t)(sourceContainer.string[i+1]);
			if (c1=='0'){
				numberOfCharactersLong-=3;
			} else if (c1=='x'){
				if (isWideLiteral) numberOfCharactersLong-=5;
				else numberOfCharactersLong-=3;
			} else {
				numberOfCharactersLong-=1;
			}
		}
	}
	return numberOfCharactersLong;
}


void fillDataLiteralEntry(struct DataLiteralEntry* dataLiteralEntry, int32_t indexOfFirstQuote, int32_t indexOfLastQuote, bool isWideLiteral){
	uint32_t numberOfCharactersLong = findLengthOfDataOfStringLiteral(indexOfFirstQuote,indexOfLastQuote,isWideLiteral);
	dataLiteralEntry->lengthOfData = numberOfCharactersLong;
	dataLiteralEntry->data = cosmic_calloc(numberOfCharactersLong,1);
	uint32_t walkingIndex = 0;
	for (int32_t i=indexOfFirstQuote+1;i<indexOfLastQuote;i++){
		uint8_t c0 = (uint8_t)(sourceContainer.string[i  ]);
		uint8_t valueToSet = c0;
		if (c0=='\\'){
			uint8_t c1 = (uint8_t)(sourceContainer.string[i+1]);
			if (c1=='0'){
				uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
				uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
				c2 = ucharAsciiAsNumberDigit(c2,true,false);
				c3 = ucharAsciiAsNumberDigit(c3,true,false);
				if (c2==200 | c3==200){
					printf("escape sequence number not in octal range\n");
					exit(1);
				}
				valueToSet=c2*8+c3;
				i+=2;
			} else if (c1=='x'){
				uint8_t c2 = (uint8_t)(sourceContainer.string[i+2]);
				uint8_t c3 = (uint8_t)(sourceContainer.string[i+3]);
				c2 = ucharAsciiAsNumberDigit(c2,false,true);
				c3 = ucharAsciiAsNumberDigit(c3,false,true);
				if (c2==200 | c3==200){
					printf("escape sequence number not in hexadecimal range\n");
					exit(1);
				}
				if (isWideLiteral){
					uint8_t c4 = (uint8_t)(sourceContainer.string[i+4]);
					uint8_t c5 = (uint8_t)(sourceContainer.string[i+5]);
					c4 = ucharAsciiAsNumberDigit(c4,false,true);
					c5 = ucharAsciiAsNumberDigit(c5,false,true);
					if (c4==200 | c5==200){
						printf("escape sequence number not in hexadecimal range\n");
						exit(1);
					}
					// not implemented further, as it is currently not supported
					i+=2;
				} else {
					valueToSet=c2*16+c3;
				}
				i+=2;
			} else if (c1=='a'){
				valueToSet=7;
			} else if (c1=='b'){
				valueToSet=8;
			} else if (c1=='f'){
				valueToSet=12;
			} else if (c1=='n'){
				valueToSet=10;
			} else if (c1=='r'){
				valueToSet=13;
			} else if (c1=='t'){
				valueToSet=9;
			} else if (c1=='v'){
				valueToSet=11;
			} else if (c1=='\''){
				valueToSet=39;
			} else if (c1=='\"'){
				valueToSet=34;
			} else if (c1=='\\'){
				valueToSet=92;
			} else if (c1=='?'){
				valueToSet=63;
			} else {
				printf("unrecognized escape sequence for string literal\n");
				exit(1);
			}
			i+=1;
		}
		dataLiteralEntry->data[walkingIndex++]=valueToSet;
	}
}

// returns the label number, if the target is a string then startIndex should be at the quote and never the prefix
uint32_t addEntryForInitData(int32_t startIndex, bool isInitializer, bool isWideStringLiteral, bool isModifiable){
	if (globalDataLiteralEntries.numberOfValidEntries>=globalDataLiteralEntries.numberOfAllocatedSlots){
		globalDataLiteralEntries.numberOfAllocatedSlots += 50;
		globalDataLiteralEntries.entries = cosmic_realloc(globalDataLiteralEntries.entries,sizeof(struct DataLiteralEntry)*globalDataLiteralEntries.numberOfAllocatedSlots);
	}
	struct DataLiteralEntry* thisEntry = globalDataLiteralEntries.entries+globalDataLiteralEntries.numberOfValidEntries++;
	{
		const struct DataLiteralEntry n={0};
		*thisEntry=n;
	}
	uint32_t label = ++globalLabelID;
	thisEntry->label = label;
	thisEntry->startIndex=startIndex;
	thisEntry->isInitializer=isInitializer;
	thisEntry->isWideStringLiteral=isWideStringLiteral;
	thisEntry->isModifiable=isModifiable;
	return label;
}





