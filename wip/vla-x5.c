#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static int concat(char *result, int sz, const char *s1, const char *s2)
{
	int new_size = strlen(s1)+strlen(s2) ;
	if ( result ) {
		strlcpy(result, s1, sz) ;
		strlcat(result, s2, sz) ;
	}
	return 1+new_size ;
}

static int midstr(char *result, int sz, const char *src, int pos, int len)
{
	int src_len = strlen(src) ;
	if ( pos > src_len ) pos = src_len ; else if ( pos < 0 ) pos = 0 ;
	if ( pos + len > src_len ) len = src_len - pos ;
	if ( len < 0 ) len = 0 ;
	if ( result ) {
		int act_len = sz <= len ? sz-1 : len ;
		memcpy(result, src+pos, act_len) ;
		result[act_len] = 0 ;
	}
	return 1+len ;
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

#define STR_VIEW_SIZE(var_) ((int) (var_##_sz))
#define STR_VIEW_BUF(var_) (var_),STR_VIEW_SIZE(var_)

#define FLEX_STR_ASSIGN(var_, func, ...) \
	int var_##_sz = func(NULL, 0, __VA_ARGS__) ; \
	char var_##_vla[var_##_sz >FLEX_STR_MAX ? 0 : var_##_sz] ; \
	char *var_ = sizeof(var_##_vla) ? var_##_vla : malloc(var_##_sz) ; \
	func(var_, var_##_sz, __VA_ARGS__)


static void test1(const char *s1, const char *s2)
{
	FLEX_STR_ASSIGN(m1, midstr, s1, 2, 4) ;
	FLEX_STR_ASSIGN(m2, midstr, s2, 5, 555) ;
	FLEX_STR_ASSIGN(result, concat, m1, m2) ;
	printf("S1(%d)=%s (%d)\n", STR_VIEW_SIZE(result), result, (int) strlen(result)) ;
	STR_VIEW_FREE(result) ;
	STR_VIEW_FREE(m2) ;
	STR_VIEW_FREE(m1) ;
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	test1("aaaaa", "bbbbb") ;
	test1("aaaaaaaaaabbbbbbbbbbcccccccccc", "ddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhZ");
	exit(EXIT_SUCCESS) ;
}
