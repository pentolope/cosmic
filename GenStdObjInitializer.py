
functions='''
strlen
memset
memmove
memcpy
strcpy
strncpy
strcat
strncat
strchr
strrchr
strcmp
strncmp
memcmp
memchr
strspn
strcspn
strpbrk
strstr
strtok

malloc
calloc
free
realloc

vsprintf
fprintf
printf
snprintf

_assert

fopen
fclose
fputc
fgetc
fgets
fflush
fgetpos
fsetpos
fread
fwrite
fseek
ftell
ungetc
rewind
clearerr
feof
ferror
abort
exit
'''

variables='''

stdout
stderr
stdin

_static_var_strtok
'''

functions = list(''.join(filter(lambda c:c!=' ',list(functions))).split('\n'))
functions = list(filter(lambda n:n!='',functions))

variables = list(''.join(filter(lambda c:c!=' ',list(variables))).split('\n'))
variables = list(filter(lambda n:n!='',variables))

nameTypePairList=[]
for k in functions:
	nameTypePairList.append((k,'5'))
for k in variables:
	nameTypePairList.append((k,'1'))

nameTypePairList=sorted(nameTypePairList,key=lambda v:v[0]) # sorting is important. it's the main reason why I made this generator code
nameTypePairList=[(str(i),v[0],v[1]) for i,v in enumerate(nameTypePairList)]
initalizerFinal='{\n'+(',\n'.join(['\t['+i+'].address=&'+n+',['+i+'].symbol="'+n+'",['+i+'].expectedType='+t for i,n,t in nameTypePairList]))+'\n};\n'

f=open('StdObjInitializer.c','w')
f.write("""
// this is an automatically generated file. see `GenStrObjInitializer.py` for it\'s generation code.
#define NUM_STD_SYMBOLS """+str(len(nameTypePairList))+"""
struct {
	const char* symbol;
	void* address;
	uint32_t label;
	uint8_t expectedType;
} stdObjects[NUM_STD_SYMBOLS]="""+initalizerFinal)
f.close()
