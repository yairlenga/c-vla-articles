# How Much Stack Space Do You Have? Estimating Remaining Stack on Linux

In a previous article I suggested using **stack allocation (VLAs)** for
small temporary buffers in C as an alternative to `malloc()`.

One of the most common concerns in the comments was:

> *"Stack allocations are dangerous because you cannot know how much
> stack space is available."*

This concern is understandable. If a program accidentally exceeds the
stack limit, the result is usually a **segmentation fault**.

However, the assumption that stack size is completely unknowable is not
entirely accurate. While the **C standard does not provide any way to
query stack limits**, modern operating systems --- including Linux ---
expose enough information to **estimate available stack space**.

This article explores a few practical techniques to answer the question:

> **How much stack space does my program have left?**

The goal is not perfect precision, but **good enough estimates** to
guide decisions such as whether to allocate memory on the stack or the
heap.

------------------------------------------------------------------------

# The Stack on Modern Linux

On most modern Linux systems the default stack size for a process is
typically around:

    8 MB

You can confirm this using:

    ulimit -s

On modern Linux platforms (X86-64, ARM, RISK-V, PowerPC) the stack grows **downward in memory**,
meaning that as functions are called and local variables are allocated, the stack
pointer moves toward lower addresses.

Conceptually, the stack looks something like this:

    Higher addresses
    ┌───────────────────────────┐
    │  stack start / top        │
    │  main() frame             │
    │  caller frames            │
    │  local variables          │
    │  current stack pointer    │
    │                           │
    │  unused stack space       │
    │                           │
    │  stack limit (guard page) │
    └───────────────────────────┘
    Lower addresses

When the stack grows beyond the guard page, the operating system raises a segmentation fault.

To estimate remaining stack space we need two pieces of information:

1.  **Stack boundaries** (base and size)
2.  **Current stack pointer**

Once we know those, the remaining stack can be approximated by measuring
the distance between them.

------------------------------------------------------------------------

# Getting the Current Stack Pointer

C does not provide an official API to query the stack pointer.

In practice, the address of a **local variable** provides a very good
approximation of the current stack position, since local variables are
typically stored in the current stack frame.

Example:

``` c
static char * stack_marker_addr(void)
{
    char marker;
    return &marker;
}
```

This address is usually close to the current position of the stack
pointer.

When compiling with higher optimization levels, the compiler may
rearrange stack layout or inline helper functions in ways that make the
measurement less predictable. To reduce this effect, it helps to:

-   take the address of a `volatile` local variable
-   place the logic in a `noinline` helper function

A small helper like the following works well in practice:

``` c
__attribute__((noinline))
static char * stack_marker_addr(void)
{
    [[maybe_unused]] volatile char marker;
    return &marker;
}
```

The returned address can then be compared to the **stack boundaries**
reported by the operating system to estimate remaining stack space.

------------------------------------------------------------------------

# Method 1: Query the Stack Limit with `getrlimit`

Linux exposes the maximum stack size through the `getrlimit()` system
call.

Example usage:

``` c
// CODE EXAMPLE PLACEHOLDER
```

This returns the **maximum stack size** configured for the process.

By capturing the stack pointer early in the program and combining it with
maximum stack size, we can estimate the stack base, and the reamining stack
space:

Conceptually:

    stack_top = stack_marker_addr() // at program start.
    stack_base = stack_top - stack_size
    remaining_stack ≈ stack_marker_addr() − stack_base

This method is simple and portable across Unix systems, but it has one
limitation:

> It requires capturing the stack position early in the program to
> establish a reference point.

If the first opportunity to capture the address of the "top" of the stack occurs after
significant stack allocations were executed, we might over-estimate the remaining stack space
as there is no easy way to estimate the space already been used. In those cases, an alternative
method exists exists.

------------------------------------------------------------------------

# Method 2: Using `pthread_getattr_np`

Linux systems using glibc provide a convenient non-standard extension,
`pthread_getattr_np()`, which allows a thread to query its own stack
attributes, including the stack base address and stack size.


Example:

``` c
// CODE EXAMPLE PLACEHOLDER
```

From this we can obtain the stack_base, which can now use for estimating the remaining stack

With the current stack pointer we can estimate the remaining stack:

    remaining_stack ≈ stack_marker − stack_base

This method has several advantages:

-   Works **In Multi Threaded programs** (different threads may have different stack)
-   Does not require change to program startup
-   Provides **direct access to stack boundaries**

For Linux programs that already use `pthread`, this is often the
**cleanest approach**. Using this technique on single threaded program requires
the program to link with the pthread library, but **does not** launch extra
threads, or introduce thread-safety issues into code that does not otherwise
launch additional threads.

------------------------------------------------------------------------

# Method 3: Capturing the Stack Position at Program Startup

The previous method uses `pthread_getattr_np()` to query stack
boundaries directly. While convenient, it requires linking with the
pthread library and relies on a non-standard GNU extension.

In many programs, especially single-threaded utilities, it may be
desirable to estimate stack usage **without introducing a dependency on
pthread**.

One simple technique is to capture the stack position **very early in
the program's lifetime**, before additional call frames are created. On
systems using GCC or Clang this can be done using a *constructor
function*.

Functions marked with the `constructor` attribute are executed
automatically before `main()`.

Example:

``` c
static char *stack_base;

__attribute__((constructor))
static void capture_stack_start(void)
{
    char *stack_start = stack_marker_address() ;
    size_t stack_size = // from getrlimit - method 1
    stack_base = stack_start - stack_size ;
}
```

Because this function runs during program startup, the recorded address
is typically very close to the **top of the initial stack**. Combining this address with the configured stack size provides a good approximation of the
stack base.

Later in the program we can compare this value with the current stack
position to estimate stack usage, and remaining stack

``` c
size_t stack_space = stack_marker_addr() - stack_base ;
```

This approach avoids the need for `pthread`, and the measurement can be
implemented entirely inside a helper module without requiring any
changes to `main()`.

Like the other techniques presented here, this method provides an
**estimate** rather than an exact measurement, but it is often
sufficient to guide decisions such as whether a temporary buffer should
be placed on the stack or the heap.



# Turning This into a Small Utility

Once the stack boundaries are known, it is easy to wrap the calculation
into a small helper function.

Conceptually:

``` c
size_t stack_remaining();
```

Example implementation:

``` c
// CODE EXAMPLE PLACEHOLDER
```

This allows programs to check available stack space dynamically.

------------------------------------------------------------------------

# Using Stack Estimates to Guide Allocation Decisions

The practical motivation for estimating stack space is simple:

Some allocations are small enough that placing them on the stack is
faster and simpler than using the heap.

However, we want to avoid risking stack overflow.

A simple strategy is to allocate on the stack **only when sufficient
space remains**.

Example logic:

``` c
// CODE EXAMPLE PLACEHOLDER
```

This allows the program to use the stack when it is safe, and fall back
to the heap otherwise.

------------------------------------------------------------------------

# How Accurate Are These Estimates?

These methods provide **estimates**, not guarantees.

A few factors can influence stack usage:

-   deep call stacks\
-   recursion\
-   large local variables\
-   compiler optimizations\
-   thread stack sizes

Because of this, it is wise to leave a **safety margin** when making
decisions based on remaining stack space.

In practice, leaving a few kilobytes (8-32) of buffer is usually sufficient.

------------------------------------------------------------------------

# Conclusion

Although the C language itself does not expose stack information, modern
Linux systems provide enough primitives to estimate stack usage.

Using APIs and features such as:

    getrlimit()
    pthread_getattr_np()
    GCC/CLANG [[constructor]] attribute.

a program can determine stack limits and approximate the remaining stack
space at runtime.

This does not eliminate the need for careful programming, but it does
make stack allocation decisions **far more informed than commonly
assumed**.

In a follow-up article we will explore a more experimental approach:
**actively probing the stack itself to discover its limits**.
