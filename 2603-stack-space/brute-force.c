// Get STack Size with Brute Force
#include <stdio.h>
#include <unistd.h>

static void find_stack_size()
{
    volatile char probe[1] ;
    size_t page_size = sysconf(_SC_PAGESIZE) ;
    size_t stack_size = 0 ;
    while ( (probe[-stack_size] = 1) ) {
        printf("%s: stack_size=%zd\n", __FILE__, stack_size) ;
        stack_size += page_size ;
    }
}


int main(void)
{
    find_stack_size() ;
}