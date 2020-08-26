f1=open('a1.out','rb')
f2=open('a2.out','rb')
s1=f1.read()
s2=f2.read()
b=s1==s2
f1.close()
f2.close()
print("IsIden:"+str(b));
if not b:
	lm=len(s1)==len(s2)
	print('len match:'+str(lm))
	if lm:
		print('len:'+str(len(s1)))
		ll=[]
		for i in range(len(s1)):
			if s1[i]!=s2[i]:
				ll.append((i,s1[i],s2[i],ord(s1[i]),ord(s2[i])))
		print('diff count:'+str(len(ll)))
		for i in range(len(ll)):
			print(ll[i])

