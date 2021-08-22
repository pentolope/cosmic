
#ifndef __STD_IO
#define __STD_IO

#ifndef EOF
#define EOF (-1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define IOFBF 1
#define IOLBF 2
#define IONBF 3

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _MAX_UNGET 4

typedef struct {
	unsigned long position;
} fpos_t;

typedef struct {
	/*
	The simulator's definitions (ab)use this struct as well, but those definitions use the values for other purposes.
	`buffPos` is used as a position marker
	`buffType` is used to store the mode_id
	all other values are unused by the simulator. the simulator's definitions are less compliant to the standard.
	*/
	
	int file_descriptor; /* value represents: 
	: -1 : not taken slot for a file stream
	: 0,1,2 : the true form of stdout,stderr,stdin respectively
	: all_open_files.file_object_handles[{value} - 3] : an open file stream
	: otherwise, it is invalid */
	unsigned char* buffPtr;
	unsigned long buffLen;
	unsigned long buffPos;
	unsigned char errFlags; // bit 0 is eof, bit 1 is any other error
	unsigned char buffType;
	unsigned char curIObuffMode; // 0=unoriented , 1=input , 2=output
	unsigned char ungetBuffPos;
	unsigned char ungetBuff[_MAX_UNGET];
} FILE;

#ifdef __STD_REAL
#ifdef __BUILDING_SIM_LOADER

FILE __open_files[4]={[0].buffType=6,[1].buffType=6,[2].buffType=2};
FILE* stdout=__open_files+0;
FILE* stderr=__open_files+1;
FILE* stdin =__open_files+2;

#else // #ifdef __BUILDING_SIM_LOADER

#define MAX_AUTO_BUFF 4
#define AUTO_BUF_SIZE 64
#define MAX_FS_FILE_HANDLES 16
#define MAX_FILE_DESCRIPTORS (MAX_FS_FILE_HANDLES+3)

struct Auto_Buff{
	uint8_t buff[AUTO_BUF_SIZE];
};
struct All_Open_Files {
	struct Folder_File_Object file_object_handles[MAX_FS_FILE_HANDLES];
	FILE file_descriptor_handles[MAX_FILE_DESCRIPTORS];
	uint8_t* working_directory; // should be heap allocated and contain a slash as first character
	bool fs_init_performed;
	bool file_auto_buff_taken[MAX_AUTO_BUFF];
	struct Auto_Buff file_auto_buff_content[MAX_AUTO_BUFF];
}* all_open_files;
FILE* stdout,stderr,stdin;

#endif // #ifdef __BUILDING_SIM_LOADER

FILE* fopen(const char* file,const char* mode);
int fclose(FILE* stream);
int fputc(int c, FILE* stream);
int fgetc(FILE* stream);
int fflush(FILE* stream);
int fgetpos(FILE* inputFile,fpos_t* pos);
int fsetpos(FILE* inputFile,fpos_t* pos);

// the functions below are not implemented in the simulator. attempting to use them there will result in a runtime error

unsigned long fread(void* buffer,unsigned long size,unsigned long count,FILE* file);
unsigned long fwrite(const void* buffer,unsigned long size,unsigned long count,FILE* file);
int fseek(FILE* file,long offset,int whence);
long ftell(FILE* file);
int ungetc(int ch, FILE* file);
int rewind(FILE* file);
void clearerr(FILE* file);
int feof(FILE* file);
int ferror(FILE* file);

#else // #ifdef __STD_REAL

extern FILE* stdout;
extern FILE* stderr;
extern FILE* stdin;

extern FILE* fopen(const char* file,const char* mode);
extern int fclose(FILE* stream);
extern int fputc(int c, FILE* stream);
extern int fgetc(FILE* stream);
extern int fflush(FILE* stream);
extern int fgetpos(FILE* inputFile,fpos_t* pos);
extern int fsetpos(FILE* inputFile,fpos_t* pos);

// the functions below are not implemented in the simulator. attempting to use them there will result in a runtime error

extern unsigned long fread(void* buffer,unsigned long size,unsigned long count,FILE* file);
extern unsigned long fwrite(const void* buffer,unsigned long size,unsigned long count,FILE* file);
extern int fseek(FILE* file,long offset,int whence);
extern long ftell(FILE* file);
extern int ungetc(int ch, FILE* file);
extern int rewind(FILE* file);
extern void clearerr(FILE* file);
extern int feof(FILE* file);
extern int ferror(FILE* file);


#endif // #ifdef __STD_REAL

#endif // #ifndef __STD_IO

