// Example for acticle: "How Much Stack Space Does my Program have Left"
// Compile: gist-2603-stack-pthread.c
// Run: ./a.out

#define _GNU_SOURCE
#include <stdlib.h>
#include "sys/resource.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdint.h>

typedef uintptr_t StackAddr ; 

struct stack_info {
    StackAddr base ;
    size_t size ;
    size_t max_use ;
    size_t margin ;
    StackAddr low_mark ;
} ;

static _Thread_local struct stack_info s_stack_info ;

[[gnu::noinline]]
static StackAddr stack_marker_addr(void)
{
    volatile char marker[1];
    StackAddr sp = (StackAddr) marker ;
    if ( sp < s_stack_info.low_mark ) s_stack_info.low_mark = sp ;
    return sp ;
}

static void stack_setup_pthread(void)
{
    pthread_attr_t attr ;
    void *stack_base ;
    size_t stack_size ;
    pthread_getattr_np(pthread_self(), &attr) ;
    pthread_attr_getstack(&attr, &stack_base, &stack_size) ;
    size_t page_size = sysconf(_SC_PAGE_SIZE) ;
    s_stack_info = (struct stack_info) { .base = (StackAddr) stack_base, .size = stack_size, .margin = 2*page_size  } ;
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
    stack_setup_pthread() ;
    show_space(__func__) ;
    test1() ;
    test2() ;

}