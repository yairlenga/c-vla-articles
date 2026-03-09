// hidden variables: var_##_sz (int, size), var_##_vla (char [n] or char [0])


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void concat(char *result, int sz, const char *s1, const char *s2)
{
	strlcpy(result, s1, sz) ;
	strlcat(result, s2, sz) ;
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

static void test1(const char *s1, const char *s2)
{
	int req_sz = 1+strlen(s1) + strlen(s2) ;
	STR_VIEW_INIT(result, req_sz) ;
	concat(result, STR_VIEW_SIZE(result), s1, s2) ;
	printf("S1(%d)=%s (%d)\n", STR_VIEW_SIZE(result), result, (int) strlen(result)) ;
	STR_VIEW_FREE(result) ;
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	test1("aaaaa", "bbbbb") ;
	test1("aaaaaaaaaabbbbbbbbbbcccccccccc", "ddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhh");
	exit(EXIT_SUCCESS) ;
}
