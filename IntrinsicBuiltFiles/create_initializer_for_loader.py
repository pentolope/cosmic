f=open('_intrinsics_functions.bin','rb')
s=f.read()
f.close()

f=open('_intrinsic_c_functions.c','wb');
f.write(('static uint8_t _intrinsic_c_functions[]={'+','.join(map(lambda i:str(s[i]),range(len(s))))+'};\n').encode())
f.close()

print("\'_intrinsic_c_functions.c\' has been written")
