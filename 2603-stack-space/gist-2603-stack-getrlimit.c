// Example for acticle: "How Much Stack Space Does my Program have Left"
// Compile: gist-2603-stack-getrlimit.c
// Run: ./a.out
#include <stdlib.h>
#include "sys/resource.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

/* Integer form of a pointer used for stack address comparisons. */
typedef uintptr_t StackAddr ; 

struct stack_info {
    StackAddr base ;     // estimated starting address of the stack
    size_t size ;        // stack size as reported by system calls/API.
    size_t max_use ;     // Estimated max use for queries
    size_t margin ;      // safety margin to compensate for estimation
    StackAddr low_mark ; // Captured low mark of the stack
} ;

static struct stack_info s_stack_info ;

[[gnu::noinline]]
static StackAddr stack_marker_addr(void)
{
    volatile char marker[1];
    StackAddr sp = (StackAddr) marker ;
    if ( sp < s_stack_info.low_mark ) s_stack_info.low_mark = sp ;
    return sp ;
}

static void stack_setup_getrlimit(void)
{
    StackAddr stack_top = stack_marker_addr() ;
    struct rlimit stack_limit ;
    getrlimit(RLIMIT_STACK, &stack_limit) ;
    size_t page_size = sysconf(_SC_PAGE_SIZE) ;
    s_stack_info = (struct stack_info) { .base = stack_top - stack_limit.rlim_cur, .size = stack_limit.rlim_cur, .margin = 2*page_size } ;
}

static size_t stack_remaining(void)
{
    StackAddr stack_marker = stack_marker_addr() ;
    return stack_marker - s_stack_info.base - s_stack_info.margin ;
}

static size_t stack_inuse(void)
{
    StackAddr stack_marker = stack_marker_addr() ;
    return s_stack_info.base + s_stack_info.size - stack_marker ;
}

void show_space(const char *where) {
    size_t remaining = stack_remaining() ;
    printf("%s: remaining %'zd (used=%zd)\n", where, remaining, stack_inuse()) ;
}

void test1() {
    [[maybe_unused]] char x[30000] ;
    show_space(__func__) ;

}

void test2()
{
    [[maybe_unused]] char x[60000] ;
    show_space(__func__) ;
}

int main(void)
{
    stack_setup_getrlimit() ;
    show_space(__func__) ;
    test1() ;
    test2() ;

}