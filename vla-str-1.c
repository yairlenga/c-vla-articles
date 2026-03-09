// hidden variables: var_##_sz (int, size), var_##_vla (char [n] or char [0])
// Better concat.
// Used in Article

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

static inline void concat(char *result, int sz, const char *s1, const char *s2)
{
//	while ( sz > 1 && *s1 ) { *result++ = *s1++ ; sz-- ; } ;
//	while ( sz > 1 && *s2 ) { *result++ = *s2++ ; sz-- ; } ;
//	if ( sz ) *result = 0 ;
	int l1 = strlen(s1) ; if ( l1 >= sz ) l1=sz-1 ;
	memcpy(result, s1, l1) ; result += l1 ; sz -= l1 ;
	int l2 = strlen(s2) ; if ( l2 >= sz ) l2=sz-1 ;
	memcpy(result, s2, l2) ;
	result[l2] = 0 ;
}

#ifndef FLEX_STR_MAX
#define FLEX_STR_MAX 64
#endif

#define STR_VIEW_INIT(var_, sz_) \
	int var_##_sz = sz_ ; \
	char var_##_vla[var_##_sz >FLEX_STR_MAX ? 0 : var_##_sz] ; \
	char *var_ = sizeof(var_##_vla) ? var_##_vla : malloc(var_##_sz)

#define STR_VIEW_FREE(var_) \
	if ( !sizeof(var_##_vla) ) { free(var_) ; var_ = NULL ; }

#define STR_VIEW_SIZE(var_) (var_##_sz)
#define STR_VIEW_BUF(var_) (var_),STR_VIEW_SIZE(var_)

static void test1(bool show, const char *s1, const char *s2)
{
	STR_VIEW_INIT(result, 1+strlen(s1) + strlen(s2)) ;
	concat(STR_VIEW_BUF(result), s1, s2) ;
	if ( show) printf("S1(%d)=%s (%d)\n", STR_VIEW_SIZE(result), result, (int) strlen(result)) ;
	STR_VIEW_FREE(result) ;
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
