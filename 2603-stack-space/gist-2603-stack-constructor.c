// Example for acticle: "How Much Stack Space Does my Program have Left"
// Compile: gcc gist-2603-stack-constructor.c
// Run: ./a.out

#define _GNU_SOURCE
#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#include "stack-util.c"

static void stack_setup_getrlimit(void)
{
    StackAddr stack_top = stack_marker_addr() ;
    struct rlimit stack_limit ;
    getrlimit(RLIMIT_STACK, &stack_limit) ;
    size_t page_size = sysconf(_SC_PAGE_SIZE) ;
    s_stack_info = (struct stack_info) {
        .base = stack_top - stack_limit.rlim_cur,
        .size = stack_limit.rlim_cur,
        .margin = 2*page_size
    } ;
}

[[gnu::constructor]]
static void stack_setup_atstart(void)
{
    stack_setup_getrlimit() ;
}

void test1() {
    [[maybe_unused]]  char x[30000] ;
    show_space(__func__) ;
}

void test2()
{
    [[maybe_unused]]  char x[60000] ;
    show_space(__func__) ;
}

int main(void)
{
    show_space(__func__) ;
    test1() ;
    test2() ;

}