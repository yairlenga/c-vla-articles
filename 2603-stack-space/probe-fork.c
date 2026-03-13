// Get STack Size with Brute Force

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>

static void probe_fail(int signum) ;
static size_t stack_size ;
static int probe_pipe[2] ;

static size_t find_stack_size()
{
    if (pipe(probe_pipe)) {
        perror("pipe") ;
        exit(2) ;
    }

    pid_t probe_pid = fork() ;
    if ( probe_pid == (pid_t) -1 ) {
        perror("fork") ;
        exit(2) ;
    }
    
    if ( !probe_pid ) {
        // In Child process, probe stack
        signal(SIGSEGV, probe_fail) ;
        volatile char probe[1] ;
        size_t page_size = sysconf(_SC_PAGESIZE) ;
        // Probe until you drop :-)
        for (size_t i = 0 ; (probe[-i] = 1) ; i += page_size ) stack_size = i;
        abort() ;    // Should not get here ...
    }
    // Parent
    close(probe_pipe[1]) ;
    int n_read = read(probe_pipe[0], &stack_size, sizeof(stack_size)) ;
    close(probe_pipe[0]) ;
    if ( n_read != sizeof(stack_size)) {
        perror("read") ;
        abort() ;
    } ;
    return stack_size ;
}

static void probe_fail([[maybe_unused]] int signum)
{
    if ( write(probe_pipe[1], &stack_size, sizeof(stack_size)) != sizeof(stack_size)) {
        perror("write") ;
        abort() ;
    }
    // Return quiely, job done!
    exit(0) ;
}

int main(void)
{
    size_t stack_size = find_stack_size() ;
    printf("%s: stack_size=%zd\n", __FILE__, stack_size) ;
}