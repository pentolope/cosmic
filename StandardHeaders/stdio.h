
#ifndef __STD_IO
#define __STD_IO

#ifndef EOF
#define EOF (-1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct {
	const char* path;
	unsigned long position;
	unsigned char mode_id;
	_Bool ignorePosition;
	
	// should I make a buffer built in here?
} FILE;

typedef struct {
	unsigned long position;
} fpos_t;



#ifdef __STD_REAL

static FILE* fopen(const char* file,const char* mode);
static int fclose(FILE* stream);
static int fputc(int c, FILE* stream);
static int fgetc(FILE* stream);
static int fflush(FILE* stream);
static int fgetpos(FILE* inputFile,fpos_t* pos);
static int fsetpos(FILE* inputFile,fpos_t* pos);

static FILE __open_files[256]={[0].mode_id=6,[1].mode_id=6,[2].mode_id=2};

static FILE* stdout=__open_files+0;
static FILE* stderr=__open_files+1;
static FILE* stdin =__open_files+2;


#else // #ifdef __STD_REAL

extern FILE* fopen(const char* file,const char* mode);
extern int fclose(FILE* stream);
extern int fputc(int c, FILE* stream);
extern int fgetc(FILE* stream);
extern int fflush(FILE* stream);
extern int fgetpos(FILE* inputFile,fpos_t* pos);
extern int fsetpos(FILE* inputFile,fpos_t* pos);

extern FILE* stdout;
extern FILE* stderr;
extern FILE* stdin;

#endif // #ifdef __STD_REAL


#endif // #ifndef __STD_IO

