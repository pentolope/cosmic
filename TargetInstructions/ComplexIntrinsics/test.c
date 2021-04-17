

#include <stdbool.h>
bool bd;
bool print_success=0;

unsigned long printCount=0;


#include <stdio.h>
#include <stdlib.h>
#include "source.c"
#include <float.h>
#include <math.h>

typedef union {float f; u32 u;} fu32;

uint32_t myRand(){
	uint32_t returnValue = 0;
	for (uint8_t i=0;i<4;i++){
		returnValue|=(255&rand())<<(i*8);
	}
	return returnValue;
}

float randFloat(){
	fu32 fu;
	fu.u=myRand();
	return fu.f;
}


static u32 conv_fu(float f){
	fu32 fu;fu.f=f;
	return fu.u;
}

static float conv_uf(u32 u){
	fu32 fu;fu.u=u;
	return fu.f;
}


static struct {
	fu32 left;
	fu32 right;
	fu32 expected;
	fu32 result;
} last;

//returns true if the test failed
static bool f_add_test(float l, float r){
	last.left.f=l;
	last.right.f=r;
	last.result.f=conv_uf(f_add_intrinsic(conv_fu(l),conv_fu(r)));
	last.expected.f=(float)l + (float)r;
	return (last.expected.f != last.result.f) & !(isnan(last.expected.f) & isnan(last.result.f));
}


static void f_add_test_redo(){
	bd=true;
	printCount++;
	if (printCount>10){
		printf("printCount exceeded. Terminating...\n");
		exit(0);
	}
	conv_uf(f_add_intrinsic(conv_fu(last.left.f),conv_fu(last.right.f)));
	bd=false;
}

static bool f_sub_test(float l, float r){
	last.left.f=l;
	last.right.f=r;
	last.result.f=conv_uf(f_sub_intrinsic(conv_fu(l),conv_fu(r)));
	last.expected.f=(float)l - (float)r;
	return (last.expected.f != last.result.f) & !(isnan(last.expected.f) & isnan(last.result.f));
}


static void f_sub_test_redo(){
	bd=true;
	printCount++;
	if (printCount>10){
		printf("printCount exceeded. Terminating...\n");
		exit(0);
	}
	conv_uf(f_sub_intrinsic(conv_fu(last.left.f),conv_fu(last.right.f)));
	bd=false;
}


static bool f_mul_test(float l, float r){
	last.left.f=l;
	last.right.f=r;
	last.result.f=conv_uf(f_mul_intrinsic(conv_fu(l),conv_fu(r)));
	last.expected.f=(float)l * (float)r;
	return (last.expected.f != last.result.f) & !(isnan(last.expected.f) & isnan(last.result.f));
}


static void f_mul_test_redo(){
	bd=true;
	printCount++;
	if (printCount>10){
		printf("printCount exceeded. Terminating...\n");
		exit(0);
	}
	conv_uf(f_mul_intrinsic(conv_fu(last.left.f),conv_fu(last.right.f)));
	bd=false;
}

static bool f_div_test(float l, float r){
	last.left.f=l;
	last.right.f=r;
	last.result.f=conv_uf(f_div_intrinsic(conv_fu(l),conv_fu(r)));
	last.expected.f=(float)l / (float)r;
	return (last.expected.f != last.result.f) & !(isnan(last.expected.f) & isnan(last.result.f));
}


static void f_div_test_redo(){
	bd=true;
	printCount++;
	if (printCount>10){
		printf("printCount exceeded. Terminating...\n");
		exit(0);
	}
	conv_uf(f_div_intrinsic(conv_fu(last.left.f),conv_fu(last.right.f)));
	bd=false;
}



static void print_last_add_as_err(){
	f_add_test_redo();
	printf(
		"ERR: %08X + %08X = %08X : %08X ; %.60f + %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_add_as_yes(){
	if (!print_success) return;
	f_add_test_redo();
	printf(
		"YES: %08X + %08X = %08X : %08X ; %.60f + %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_sub_as_err(){
	f_sub_test_redo();
	printf(
		"ERR: %08X - %08X = %08X : %08X ; %.60f - %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_sub_as_yes(){
	if (!print_success) return;
	f_sub_test_redo();
	printf(
		"YES: %08X - %08X = %08X : %08X ; %.60f - %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_mul_as_err(){
	f_mul_test_redo();
	printf(
		"ERR: %08X * %08X = %08X : %08X ; %.60f * %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_mul_as_yes(){
	if (!print_success) return;
	f_mul_test_redo();
	printf(
		"YES: %08X * %08X = %08X : %08X ; %.60f * %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_div_as_err(){
	f_div_test_redo();
	printf(
		"ERR: %08X / %08X = %08X : %08X ; %.60f / %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

static void print_last_div_as_yes(){
	if (!print_success) return;
	f_div_test_redo();
	printf(
		"YES: %08X / %08X = %08X : %08X ; %.60f / %.60f = %.60f : %.60f  \n",
		(unsigned int)last.left.u,
		(unsigned int)last.right.u,
		(unsigned int)last.expected.u,
		(unsigned int)last.result.u,
		last.left.f,
		last.right.f,
		last.expected.f,
		last.result.f
		);
}

/*

TODO: directly test all things with -/+inf and -/+0 for division



*/

int main(){
	printf("\n\n\n\n");
	printf("FLT_EVAL_METHOD==%d::0\n",(int)(FLT_EVAL_METHOD));
	printf("FLT_ROUNDS==%d::1\n",(int)(FLT_ROUNDS));
	printf("Running...\n");
	
	for (uint64_t i=0;i<10000000000;i++){//100000000
		float f0=randFloat();
		float f1=randFloat();
		if (0){
			if (f_add_test(f0,f1)){
				print_last_add_as_err();
			} else {
				print_last_add_as_yes();
			}
		}
		if (0){
			if (f_sub_test(f0,f1)){
				print_last_sub_as_err();
			} else {
				print_last_sub_as_yes();
			}
		}
		if (0){
			if (f_mul_test(f0,f1)){
				print_last_mul_as_err();
			} else {
				print_last_mul_as_yes();
			}
		}
		if (1){
			if (f_div_test(f0,f1)){
				print_last_div_as_err();
			} else {
				print_last_div_as_yes();
			}
		}
	}
	
	/*
	{
		float f0=conv_uf(0x80000000);
		float f1=conv_uf(0x00000000);
		if (0){
			if (f_add_test(f0,f1)){
				print_last_add_as_err();
			} else {
				print_last_add_as_yes();
			}
		}
		if (0){
			if (f_sub_test(f0,f1)){
				print_last_sub_as_err();
			} else {
				print_last_sub_as_yes();
			}
		}
		if (1){
			if (f_mul_test(f0,f1)){
				print_last_mul_as_err();
			} else {
				print_last_mul_as_yes();
			}
		}
	}
	{
		float f0=conv_uf(0x00000000);
		float f1=conv_uf(0x80000000);
		if (0){
			if (f_add_test(f0,f1)){
				print_last_add_as_err();
			} else {
				print_last_add_as_yes();
			}
		}
		if (0){
			if (f_sub_test(f0,f1)){
				print_last_sub_as_err();
			} else {
				print_last_sub_as_yes();
			}
		}
		if (1){
			if (f_mul_test(f0,f1)){
				print_last_mul_as_err();
			} else {
				print_last_mul_as_yes();
			}
		}
	}
	*/

	printf("\nFinished\n");
	return 0;
}























