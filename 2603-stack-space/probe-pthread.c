// Get STack Size with Brute Force

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

static void probe_fail(int signum) ;

static volatile size_t stack_size ;
pthread_t probe_tid ;
static void *probe_stack(void *p_where)
{
    volatile char *probe = p_where ;
    size_t page_size = sysconf(_SC_PAGESIZE) ;

    for (size_t i = 0 ; (probe[-i] = 1) ; i += page_size ) stack_size = i ;
    // Not Reached
    return NULL;
}

static size_t find_stack_size()
{
    volatile char probe[1] ;
    sig_t old_sigh = signal(SIGSEGV, probe_fail) ;
    if ( pthread_create(&probe_tid, NULL, probe_stack, (void *) probe) ) {
        perror("pthread_create") ;
        abort() ;
    }
    pthread_join(probe_tid, NULL) ;
    signal(SIGSEGV, old_sigh) ;

    return stack_size ;
}

static void probe_fail([[maybe_unused]] int signum)
{
    if ( pthread_equal(pthread_self(), probe_tid) ) {
        pthread_exit(NULL) ;
    }
    abort() ;
}



int main(void)
{
    size_t stack_size = find_stack_size() ;
    printf("%s: stack_size=%zd\n", __FILE__, stack_size) ;
}