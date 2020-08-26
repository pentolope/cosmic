f=open('_intrinsics_functions.bin','rb')
s=f.read()
f.close()

f=open('_intrinsic_c_functions.c','wb');
f.write('static uint8_t _intrinsic_c_functions[]={'+','.join(map(str,map(ord,s)))+'};\n')
f.close()

print("\'_intrinsic_c_functions.c\' has been written")
