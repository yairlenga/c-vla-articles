// Compile with: GCC
// gcc -std=c11 -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE    vla-x4.c   -o vla-x4.exe -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none
// Compile with: CLANG
// clang -std=c11 -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE    vla-x4.c   -o vla-x4.exe -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none   
// Run:
// ./vla-x4.exe
// Output: - time with VLA, time with malloc.
// Speed (1M): VLA=0.016586, MALLOC=0.023549
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

static inline void concat(char *result, int sz, const char *s1, const char *s2)
{
	int l1 = strlen(s1) ; if ( l1 >= sz ) l1=sz-1 ;
	memcpy(result, s1, l1) ; result += l1 ; sz -= l1 ;
	int l2 = strlen(s2) ; if ( l2 >= sz ) l2=sz-1 ;
	memcpy(result, s2, l2) ;
	result[l2] = 0 ;
}

#ifndef FLEX_STR_MAX
#define FLEX_STR_MAX 64
#endif

#define FLEX_STR_INIT(var_, sz_) \
	int var_##_sz = sz_ ; \
	char var_##_vla[var_##_sz >FLEX_STR_MAX ? 0 : var_##_sz] ; \
	char *var_ = sizeof(var_##_vla) ? var_##_vla : malloc(var_##_sz)

#define FLEX_STR_FREE(var_) \
	if ( !sizeof(var_##_vla) ) { free(var_) ; var_ = NULL ; }

#define FLEX_STR_SIZE(var_) ((int) (var_##_sz))
#define FLEX_STR_BUF(var_) (var_),FLEX_STR_SIZE(var_)

static void test1(bool show, const char *s1, const char *s2)
{
	FLEX_STR_INIT(result, 1+strlen(s1) + strlen(s2)) ;
	concat(FLEX_STR_BUF(result), s1, s2) ;
	if ( show) printf("S1(%d)=%s (%d)\n", FLEX_STR_SIZE(result), result, (int) strlen(result)) ;
	FLEX_STR_FREE(result) ;
}

static double now(void)
{
	struct timespec ts ;
	clock_gettime(CLOCK_MONOTONIC, &ts) ;
	return ts.tv_sec + ts.tv_nsec*1e-9 ;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	test1(true, "aaaaa", "bbbbb") ;
	test1(true, "aaaaaaaaaabbbbbbbbbbcccccccccc", "ddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhh");

	const int N=10000000 ;
	double t0= now() ;
	for (int i=0 ; i<N ; i++ ) {
		test1(i==0, "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff", "xxx") ;
	}
	double t1 = now() ;
	for (int i=0 ; i<N ; i++ ) {
		test1(i==0, "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffff", "xxxx") ;
	}
	double t2= now() ;

	printf("Speed (1M): VLA=%.6f, MALLOC=%.6f\n", 1000000*(t1-t0)/N, 1000000*(t2-t1)/N) ;

	exit(EXIT_SUCCESS) ;
}