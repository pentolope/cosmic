
#ifndef __STD_IO
#define __STD_IO

#ifndef EOF
#define EOF (-1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct FILE{
	unsigned long placeholder_definition;
} FILE;

typedef struct fpos_t{
	unsigned long placeholder_definition;
} fpos_t;

FILE stderr={0};
FILE stdout={0};

FILE* fopen(const char* file,const char* mode){
	// placeholder definition
	unsigned long i=0;
	while (file[i++]){
	}
	i=0;
	while (mode[i++]){
	}
	i=0;
	return NULL;
}


int fclose(FILE* stream){
	// placeholder definition
	stream++;
	return 0;
}

int fputc(int c, FILE* stream){
	// placeholder definition
	return c;
}

int fgetc(FILE* stream){
	// placeholder definition
	return EOF;
}


int fgetpos(FILE* inputFile,fpos_t* pos){
	return 0;
}

int fsetpos(FILE* inputFile,fpos_t* pos){
	return 0;
}

#endif

