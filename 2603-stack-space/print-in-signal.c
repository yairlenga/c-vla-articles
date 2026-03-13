// Get STack Size with Brute Force
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

size_t stack_size ;

static void print_stack(int signum) ;

static void find_stack_size()
{
    signal(SIGSEGV, print_stack) ;
    volatile char probe[1] ;
    size_t page_size = sysconf(_SC_PAGESIZE) ;
    for (size_t i = 0 ; (probe[-i] = 1) ; i += page_size ) stack_size = i;
}

static void print_stack([[maybe_unused]] int signum)
{
    printf("%s: stack_size=%zd\n", __FILE__, stack_size) ;
    exit(0) ;
}


int main(void)
{
    find_stack_size() ;
}