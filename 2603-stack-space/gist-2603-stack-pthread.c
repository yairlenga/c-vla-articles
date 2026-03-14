// Example for acticle: "How Much Stack Space Does my Program have Left"
// Compile: gcc gist-2603-stack-pthread.c
// Run: ./a.out

#define _GNU_SOURCE
#include <stdlib.h>
#include "sys/resource.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include "stack-util.c"

static void stack_setup_pthread(void)
{
    pthread_attr_t attr ;
    void *stack_base ;
    size_t stack_size ;
    pthread_getattr_np(pthread_self(), &attr) ;
    pthread_attr_getstack(&attr, &stack_base, &stack_size) ;
    size_t page_size = sysconf(_SC_PAGE_SIZE) ;
    s_stack_info = (struct stack_info) {
        .base = (StackAddr) stack_base,
        .size = stack_size,
        .margin = 2*page_size
    } ;
}

    // Setup not impacted by what's already allocated
void setup() {
    [[maybe_unused]] char x[20000] ;
    stack_setup_pthread() ; 
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
    show_space(__func__) ;
    test1() ;
    test2() ;
}