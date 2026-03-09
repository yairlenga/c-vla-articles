// Hidden variable: var_##_fs - size (int 31 bits), is_malloc: (bool 1 bit)
// var_ is pointer to N bytes (char *)[N]


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void concat(char *result, int sz, const char *s1, const char *s2)
{
	strlcpy(result, s1, sz) ;
	strlcat(result, s2, sz) ;
}

struct flex_str { int sz:31; bool is_malloced ;} ;
#ifndef FLEX_STR_MAX
#define FLEX_STR_MAX 64
#endif

#define STR_VIEW_INIT(var_, sz_) \
	const struct flex_str var_##_fs = ( { int sz = sz_ ; (struct flex_str) { .sz = sz, .is_malloced = sz > FLEX_STR_MAX } ; } ) ; \
	char var_##_vla[var_##_fs.is_malloced ? 0 : var_##_fs.sz] ; \
	char (*var_)[var_##_fs.sz] = var_##_fs.is_malloced ? malloc(var_##_fs.sz) : var_##_vla ;

#define STR_VIEW_FREE(var_) \
	if ( var_ && var_##_fs.is_malloced ) { free(var_) ; var_ = NULL ; }

#define STR_VIEW_SIZE(var_) (var_##_fs.sz)


static void test1(const char *s1, const char *s2)
{
	int req_sz = 1+strlen(s1) + strlen(s2) ;
	STR_VIEW_INIT(result, req_sz) ;
	concat(*result, STR_VIEW_SIZE(result), s1, s2) ;
	printf("S1(%d)=%s (%d)\n", STR_VIEW_SIZE(result), *result, (int) strlen(*result)) ;

	STR_VIEW_FREE(result) ;
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	test1("aaaaa", "bbbbb") ;
	test1("aaaaaaaaaabbbbbbbbbbcccccccccc", "ddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhh");
	exit(EXIT_SUCCESS) ;
}
