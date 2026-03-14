#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/resource.h>
#include <stdio.h>
#include <pthread.h>

[[gnu::noinline]] static size_t get_rlimit_stack_size(volatile char *top_probe)
{
    volatile char probe[1] ;
    struct rlimit stack_limit ;
    if ( getrlimit(RLIMIT_STACK, &stack_limit)) {
        perror("getrlimit") ;
        abort() ;
    }
    size_t used = top_probe ? (top_probe - probe) : 0 ;
    return stack_limit.rlim_cur - used ;
}

static size_t get_pthread_stack_size(void)
{
    pthread_attr_t attr ;
    void *stack_base ;
    size_t stack_size ;
    pthread_getattr_np(pthread_self(), &attr) ;
    pthread_attr_getstack(&attr, &stack_base, &stack_size) ;
    volatile char probe[1] ;
    size_t unused_stack = probe - (char *) stack_base ;
    return unused_stack ;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    size_t rlimit_stack_size = get_rlimit_stack_size(NULL) ;

    printf("%s: getrlimit: rlimit_stack_size= %zu\n", __FILE__, rlimit_stack_size) ;

    volatile char top_probe[1] ;
    size_t rlimit_unused_stack = get_rlimit_stack_size(top_probe) ;
    size_t rlimit_used = rlimit_stack_size - rlimit_unused_stack ;
    printf("%s: getrlimit: rlimit_unused_stack= %zu used=%zd\n", __FILE__, rlimit_unused_stack, rlimit_used) ;

    size_t pthread_unused_stack = get_pthread_stack_size() ;
    size_t pthread_used = rlimit_stack_size - pthread_unused_stack ;
    printf("%s: getrlimit: pthread_unused_stack= %zu, used=%zd\n", __FILE__, pthread_unused_stack, pthread_used) ;

}