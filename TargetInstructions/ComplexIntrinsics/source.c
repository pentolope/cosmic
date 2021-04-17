

// the bytecode produced by this file will be included as intrinsics used by the backend
#include <stdint.h>
#include <stdbool.h>

typedef bool     u1;
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;





/*
type values are:
 0-> (de)normalized float/double [precision lost, real value is more - then represented]
 1-> (de)normalized float/double [precision lost, real value is more + then represented]
 2-> inf (sign determines +/-)
 3-> NaN
 4-> (de)normalized float/double [no precision lost]
 
sign values are:
 0-> +
 1-> -
*/

typedef struct {u32 mantissa; s16 exponent; u1 sign; u8 type;} _float;
//typedef struct {u64 mantissa; s16 exponent; u1 sign; u8 type;} _double;

#if 0

static void dp_print(const char* s0,_float f,const char* s1){
	if (!bd) return;
	printf("%s",s0);
	switch (f.type){
		case 0:printf("{--}");break;
		case 1:printf("{++}");break;
		case 2:printf("%cinf",f.sign?'-':'+');goto End;
		case 3:printf("nan");goto End;
		case 4:printf("{==}");break;
		default:printf("{ERROR}");goto End;
	}
	if (f.sign) printf("-");
	if (0){
		printf("%u@",(unsigned int)(f.mantissa>>23));
		union {float f; u32 u;} fu;
		fu.u=(f.mantissa&0x007FFFFFlu)|(127lu<<23);
		printf("%.60f",fu.f-1);
	} else {
		printf("%08X",(unsigned int)f.mantissa);
	}
	printf("*2^%d",f.exponent);
	End:
	printf("%s",s1);
}




// f_normalize_internal is not for converting to the external representation, it is for intermediate representation
static void f_normalize_internal(_float* f){
	dp_print("f_normalize_internal [start] ",*f,"\n");
	if ((f->type&2)==0){
		if (f->mantissa==0){
			f->exponent=0;
		} else if ((u1)(f->mantissa&0xC0000000lu)){
			if (f->mantissa&1){
				f->type=f->sign;
			}
			f->exponent+=1;
			f->mantissa=f->mantissa>>1;
		} else {
			while ((f->mantissa&0x20000000lu)==0){
				f->mantissa=f->mantissa<<1;
				f->exponent-=1;
			}
		}
	}
	dp_print("f_normalize_internal [ end ] ",*f,"\n");
}


static _float f_construct(u32 v){
	_float f;
	f.sign=(v&0x80000000lu)!=0lu;
	f.exponent=(v&0x7F800000lu)>>23;
	f.mantissa=(v&0x007FFFFFlu);
	f.type=(((f.exponent==255)*3) & ((f.mantissa!=0) | 2)) | ((f.exponent!=255)*4);
	// now apply implicit convertions that are assumed from external format
	f.mantissa|=0x00800000lu * (f.exponent!=0);
	f.exponent+=(-127 * ((f.type&2)==0)) + ((f.exponent==0) & ((f.type&2)==0));
	return f;
}

// if possible, rounds to nearest
static u32 f_destruct(_float f){
	u32 v=f.sign?0x80000000lu:0x00000000lu;
	if ((u1)(f.type&2)){
		//types of NaN, +/- inf
		return v | 0x7F800000lu | (f.type&1);
	}
	if (f.mantissa!=0){
		// when mantissa!=0 then normalizing mantissa (with rounding) is performed
		
		while ((f.mantissa&0xFE000000lu)!=0){
			if (f.mantissa&1){
				if (bd) printf(">>(1,%d) ",f.sign);
				dp_print("before ",f,"\n");
				f.type=f.sign;
			} else {
				if (bd) printf(">>(0,0) ");
				dp_print("before ",f,"\n");
			}
			f.exponent+=1;
			f.mantissa=f.mantissa>>1;
			dp_print("after ",f,"\n");
		}
		if (f.mantissa!=0){
			while ((f.mantissa&0x01000000lu)==0){
				f.exponent-=1;
				f.mantissa=f.mantissa<<1;
			}
		} else {
			goto mantissaZero;
		}
		u8 type1;
		u8 type0=f.type;
		u1 bit0=0;
		do {
			type1=type0;
			if ((bit0=(u1)(f.mantissa&1))){
				f.type=f.sign;
				type0=f.type;
			}
			f.exponent+=1;
			f.mantissa=f.mantissa>>1;
		} while ((f.mantissa&0xFF000000lu)!=0);
		
		while (f.exponent<-126){
			type1=type0;
			if ((bit0=(u1)(f.mantissa&1))){
				f.type=f.sign;
				type0=f.type;
			}
			f.exponent+=1;
			f.mantissa=f.mantissa>>1;
		}
		if (bit0){
			if ((type1==4)?((unsigned)(f.mantissa&1u)==1u):(1^(type1==0) ^ !f.sign)){
				dp_print("before round ",f,"\n");
				f.mantissa+=1;
				if ((f.mantissa&0xFF000000lu)!=0){
					f.exponent+=1;
					f.mantissa=f.mantissa>>1;
				}
				dp_print("after round ",f,"\n");
			} else {
				dp_print("didn\'t round (shouldn\'t)",f,"\n");
			}
		} else {
			dp_print("didn\'t round (wouldn\'t matter)",f,"\n");
		}
	} else {
		goto mantissaZero;
	}
	if (f.mantissa==0){
		mantissaZero:
		return v;
	} else if (f.exponent>127){
		// overflow, therefore signal inf
		return v | 0x7F800000lu;
	}
	
	dp_print("f_destruct ",f,"\n");
	
	u32 converted_exponent=((u32)((f.exponent+127)*((f.mantissa&0x00800000lu)!=0))<<23);
	return v | (f.mantissa&0x007FFFFFlu) | converted_exponent;
}


static _float f_add_internal(_float l,_float r){
	f_normalize_internal(&l);
	f_normalize_internal(&r);
	_float out;
	if ((u1)((l.type | r.type)&2)){
		// this handles all cases for +/- inf and NaN
		if ((u1)((l.type & r.type)&2)){
			out.sign=l.sign;
			out.type=((l.type==3 | r.type==3 | (l.sign^r.sign))?1:l.sign) | 2;
		} else if ((u1)(l.type&2)){
			out.sign=l.sign;
			out.type=l.type;
		} else {
			out.sign=r.sign;
			out.type=r.type;
		}
		return out;
	} else {
		out.type=4;
		if (l.mantissa==0){
			l.exponent=r.exponent;
			if (r.mantissa==0){
				out.sign=l.sign & r.sign;
				out.exponent=0;
				out.mantissa=0;
				return out;
			}
		} else if (r.mantissa==0){
			r.exponent=l.exponent;
		}
		if (l.exponent!=r.exponent){
			s16 diff;
			if (l.exponent<r.exponent){
				diff=r.exponent-l.exponent;
				l.exponent+=diff;
				if (diff>=32){
					if (l.mantissa!=0){
						out.type=l.sign;
						l.type=out.type;
					}
					l.mantissa=0;
				} else {
					if (((l.mantissa&(((u32)1lu<<diff)-1)))!=0){
						out.type=l.sign;
						l.type=out.type;
					}
					l.mantissa=l.mantissa>>diff;
				}
			} else {
				diff=l.exponent-r.exponent;
				r.exponent+=diff;
				if (diff>=32){
					if (r.mantissa!=0){
						out.type=r.sign;
						r.type=out.type;
					}
					r.mantissa=0;
				} else {
					if (((r.mantissa&(((u32)1lu<<diff)-1)))!=0){
						out.type=r.sign;
						r.type=out.type;
					}
					r.mantissa=r.mantissa>>diff;
				}
			}
		}
		dp_print("f_add_internal [ l ] ", l ,"\n");
		dp_print("f_add_internal [ r ] ", r ,"\n");
		out.exponent=l.exponent;
		out.mantissa=((r.mantissa^((u32)0xFFFFFFFFlu*r.sign))+(l.mantissa^((u32)0xFFFFFFFFlu*l.sign)))+(r.sign+l.sign);
		out.sign=(out.mantissa&(u32)0x80000000lu)!=0;
		out.mantissa=(out.mantissa^((u32)0xFFFFFFFFlu*out.sign))+out.sign;
		dp_print("f_add_internal [out] ",out,"\n");
		return out;
	}
}

static _float f_sub_internal(_float l,_float r){
	r.sign^=1;
	return f_add_internal(l,r);
}

static _float f_mul_internal(_float l,_float r){
	f_normalize_internal(&l);
	f_normalize_internal(&r);
	_float out;
	out.sign=l.sign^r.sign;
	if ((u1)((l.type | r.type)&2)){
		out.type=2 | (((l.type==3) | (r.type==3)) | ((l.type==2) & (r.type==2) & (l.sign!=r.sign)) | (((l.type==2) & (r.mantissa==0)) | ((r.type==2) & (l.mantissa==0))));
		return out;
	} else {
		out.type=4;
		out.exponent=(l.exponent+r.exponent)-23;
		u64 vom;
		{
			u32 vlm0=(l.mantissa&(u32)0x0000FFFFlu)>> 0;
			u32 vlm1=(l.mantissa&(u32)0xFFFF0000lu)>>16;
			u32 vrm0=(r.mantissa&(u32)0x0000FFFFlu)>> 0;
			u32 vrm1=(r.mantissa&(u32)0xFFFF0000lu)>>16;
			vom=(((u64)(vlm0*vrm0)) | ((u64)(vlm1*vrm1)<<32)) + (((u64)(vlm0*vrm1)<<16) + ((u64)(vlm1*vrm0)<<16));
		}
		u16 nz_count=0;
		if (vom!=0u){
			nz_count=64;
			u64 t=vom;
			while ((t&(u64)0x8000000000000000llu)==0u){
				t+=t;
				nz_count--;
			}
		}
		if (nz_count<=32){
			out.mantissa=(u32)vom;
		} else {
			u16 shift_amount=nz_count-32;
			out.exponent+=(s16)shift_amount;
			out.mantissa=(u32)(vom>>shift_amount);
			if (((u64)out.mantissa<<shift_amount)!=vom){
				out.type=out.sign;
			}
		}
		return out;
	}
}

static _float f_div_internal(_float l,_float r){
	f_normalize_internal(&l);
	f_normalize_internal(&r);
	_float out;
	out.sign=l.sign^r.sign;
	if ((u1)((l.type | r.type)&2)){
		if ((l.type==3) | (r.type==3) | ((l.type==2) & (r.type==2))){
			out.type=3;
		} else {
			out.type=4;
			out.exponent=0;
			out.mantissa=0;
		}
	} else {
		switch ((l.mantissa!=0)*2 | (r.mantissa!=0)){
			case 0://(u1)((l.type | r.type)&2)==0  &&  l.mantissa==0  &&  r.mantissa==0
			out.type=3;
			break;
			case 1://(u1)((l.type | r.type)&2)==0  &&  l.mantissa==0  &&  r.mantissa!=0
			out.type=4;
			out.exponent=0;
			out.mantissa=0;
			break;
			case 2://(u1)((l.type | r.type)&2)==0  &&  l.mantissa!=0  &&  r.mantissa==0
			out.type=2;
			out.sign=r.sign;
			break;
			case 3://(u1)((l.type | r.type)&2)==0  &&  l.mantissa!=0  &&  r.mantissa!=0
			{
				out.type=4;
				out.exponent=(l.exponent-r.exponent)+(23-32);
				u16 nz_count=0;
				u64 vlm=(u64)l.mantissa<<32;
				u64 vom=vlm / (u64)r.mantissa;
				if (vlm % (u64)r.mantissa  !=0){
					out.type=out.sign;
				}
				if (vom!=0u){
					nz_count=64;
					u64 t=vom;
					while ((t&(u64)0x8000000000000000llu)==0u){
						t+=t;
						nz_count--;
					}
				}
				if (nz_count<=32){
					out.mantissa=(u32)vom;
				} else {
					u16 shift_amount=nz_count-32;
					out.exponent+=(s16)shift_amount;
					out.mantissa=(u32)(vom>>shift_amount);
					if (((u64)out.mantissa<<shift_amount)!=vom){
						out.type=out.sign;
					}
				}
			}
			break;
		}
	}
	return out;
}


u32 f_add_intrinsic(u32 l,u32 r){
	return f_destruct(f_add_internal(f_construct(l),f_construct(r)));
}

u32 f_sub_intrinsic(u32 l,u32 r){
	return f_destruct(f_sub_internal(f_construct(l),f_construct(r)));
}

u32 f_mul_intrinsic(u32 l,u32 r){
	return f_destruct(f_mul_internal(f_construct(l),f_construct(r)));
}

u32 f_div_intrinsic(u32 l,u32 r){
	return f_destruct(f_div_internal(f_construct(l),f_construct(r)));
}


#endif










