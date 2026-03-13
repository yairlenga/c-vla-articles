// Get STack Size with Brute Force

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>


static void probe_fail(int signum) ;
static sigjmp_buf env_buf ;

static size_t find_stack_size()
{
    signal(SIGSEGV, probe_fail) ;
    volatile char probe[1] ;
    size_t page_size = sysconf(_SC_PAGESIZE) ;
    volatile size_t good_size = 0 ;
    if ( !sigsetjmp(env_buf, 1)) {
        // First time - probe, never return ;
        for (size_t i = 0 ; (probe[-i] = 1) ; i += page_size ) good_size = i;
    }
    return good_size ;
}

static void probe_fail([[maybe_unused]] int signum)
{
    siglongjmp(env_buf, 1) ;
}

int main(void)
{
    size_t stack_size = find_stack_size() ;
    printf("stack_size=%zd\n", stack_size) ;
}