#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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
StackAddr stack_marker_addr(void)
{
    volatile char marker[1];
    StackAddr sp = (StackAddr) marker ;
    if ( sp < s_stack_info.low_mark ) s_stack_info.low_mark = sp ;
    return sp ;
}


size_t stack_remaining(void)
{
    StackAddr stack_marker = stack_marker_addr() ;
    return stack_marker - s_stack_info.base - s_stack_info.margin ;
}

size_t stack_inuse(void)
{
    StackAddr stack_marker = stack_marker_addr() ;
    return s_stack_info.base + s_stack_info.size - stack_marker ;
}

struct stack_info get_stack_info(void)
{
    s_stack_info.max_use = s_stack_info.base + s_stack_info.size - s_stack_info.low_mark ;
    return s_stack_info ;
}

void show_space(const char *where) {
    size_t remaining = stack_remaining() ;
    printf("%s: remaining %'zd (used=%zd, max=%zd)\n", where, remaining, stack_inuse(), get_stack_info().max_use) ;
}
