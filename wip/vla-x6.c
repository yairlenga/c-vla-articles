// hidden variables: var_##_fs (s=(char *) string start, sz(int, size), var_##_vla (char [n] or char [0])
// Better concat.
// Used in Article

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

typedef struct str_slice { char *s; int sz:31; bool is_malloced:1 ; int len: 31; bool valid_len: 1 ;} *StrSlice ;

#ifndef STR_SLICE_MAX_VLA
#define STR_SLICE_MAX_VLA 64
#endif

static inline struct str_slice str_slice_create(int sz)
{
    struct str_slice sv = (struct str_slice) { .sz = sz, .is_malloced = sz > STR_SLICE_MAX_VLA ? 1 : 0 } ;
    if ( sv.is_malloced ) sv.s = malloc(sz) ;
    return sv ;
}

static inline void str_slice_init(StrSlice sv, int sz)
{
    *sv = str_slice_create(sz) ;
}

static inline int str_slice_size(StrSlice sv) { return sv->sz ; }
static inline char *str_slice_v(StrSlice sv) { return sv->s ; }

static inline int str_slice_len(StrSlice sv)
{
    if ( !sv->valid_len ) { sv->len = strnlen(sv->s, sv->sz) ; sv->valid_len = true ;}
    return sv->len ;
}

static inline void str_slice_reset_len(StrSlice sv) { sv->valid_len = false ; }
static inline void str_slice_set_len(StrSlice sv, int len) { sv->valid_len = len > 0 ; sv->len = len ; }

static inline void str_slice_free(StrSlice sv)
{
	if ( sv->is_malloced && sv->s ) { free(sv->s) ; sv->s = NULL ; }
}

#define STR_SLICE_INIT(var_, sz_) \
    struct str_slice var_[1] = { str_slice_create(sz_)}; \
	char var_##_vla[ var_[0].is_malloced ? 0 : var_[0].sz ]  ; \
    if ( !var_[0].is_malloced ) var_[0].s = var_##_vla ;

#define STR_SLICE_FREE(var_) \
    str_slice_free(var_)

#define STR_SLICE_BUF(var_) str_slice_v(var_), str_slice_size(var_)
#define STR_SLICE_VL(var_) str_slice_v(var_), str_slice_len(var_)
#define STR_SLICE_LV(var_) str_slice_len(var_), str_slice_v(var_)

#define FLEX_STR_ASSIGN(var_, func, ...) \
	STR_SLICE_INIT(var_, func(NULL, 0, __VA_ARGS__)) ; \
	func(STR_SLICE_BUF(var_), __VA_ARGS__)

[[maybe_unused]] static int concat(char *result, int sz, const char *s1, const char *s2)
{
    if ( !result || !sz) {
        return 1 + strlen(s1) + strlen(s2) ;
    }
    int l1 = strnlen(s1, sz-1) ;
    memcpy(result, s1, l1) ; result += l1 ; sz -= l1 ;
	int l2 = strnlen(s2, sz-1) ;
    memcpy(result, s2, l2) ;
    result[l2] = 0 ;

    return (1+l1+l2) + strlen(s1+l1) + strlen(s2+l2) ;
}

static int concat_ss(char *result, int sz, StrSlice s1, StrSlice s2)
{
    int l1 = str_slice_len(s1) ;
    int l2 = str_slice_len(s2) ;
    int new_size = 1 + l1 + l2 ;
    if ( !result || !sz) return new_size ;

    if ( l1 >= sz ) l1 = sz-1 ;
    memcpy(result, str_slice_v(s1), l1) ; result += l1 ; sz -= l1 ;
	if ( l2 > sz ) l2= sz -1 ;
    memcpy(result, str_slice_v(s2), l2) ;
    result[l2] = 0 ;

    return new_size ;
}

static int midstr(char *result, int sz, const char *src, int pos, int len)
{
	int src_len = strlen(src) ;
	if ( pos > src_len ) pos = src_len ; else if ( pos < 0 ) pos = 0 ;
	if ( pos + len > src_len ) len = src_len - pos ;
	if ( len < 0 ) len = 0 ;
	if ( result && sz ) {
		int act_len = sz <= len ? sz-1 : len ;
		memcpy(result, src+pos, act_len) ;
		result[act_len] = 0 ;
	}
	return 1+len ;
}

static void test1(bool show, const char *s1, const char *s2)
{
	FLEX_STR_ASSIGN(m1, midstr, s1, 2, 4) ;
	FLEX_STR_ASSIGN(m2, midstr, s2, 5, 555) ;
//	FLEX_STR_ASSIGN(result, concat, str_slice_v(m1), str_slice_v(m2)) ;
	FLEX_STR_ASSIGN(result, concat_ss, m1, m2) ;
	if ( show) printf("S1(size=%d, len=%d,M=%d)=%.*s\n", str_slice_size(result), str_slice_len(result), result->is_malloced, STR_SLICE_LV(result)) ;
	STR_SLICE_FREE(result) ;
	STR_SLICE_FREE(m2) ;
	STR_SLICE_FREE(m1) ;
}


static double now(void)
{
	struct timespec ts ;
	clock_gettime(CLOCK_MONOTONIC, &ts) ;
	return ts.tv_sec + ts.tv_nsec*1e-9 ;
}

int main(int argc, char **argv)
{
	test1(true, "aaaaa", "bbbbb") ;
	test1(true, "aaaaaaaaaabbbbbbbbbbcccccccccc", "ddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhh");

	const int N=argc > 1 ? atoi(argv[1]) : 1000000 ;
	double t0= now() ;
	for (int i=0 ; i<N ; i++ ) {
		test1(i==0, "aaaaaaaaaa", "bbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffggggggggggZ");
	}
	double t1 = now() ;
	for (int i=0 ; i<N ; i++ ) {
		test1(i==0, "aaaaaaaaaa", "bbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhZ");
	}
	double t2= now() ;

    	for (int i=0 ; i<N ; i++ ) {
		test1(i==0, "aaaaaaaaaabbbbbbbbbb", "bbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiiZ");
	}
	double t3= now() ;

	printf("Speed (N=%d) (per 1M): VLA=%.6f, MALLOC=%.6f, LARGE=%.6f\n", N, 1000000*(t1-t0)/N, 1000000*(t2-t1)/N, 1000000*(t3-t2)/N) ;

	exit(EXIT_SUCCESS) ;
}
