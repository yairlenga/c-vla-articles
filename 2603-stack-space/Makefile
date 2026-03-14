default: all

GIST_ALL = gist-2603-stack-getrlimit.exe gist-2603-stack-pthread.exe gist-2603-stack-constructor.exe
ALL = brute-force.exe print-in-signal.exe probe-fork.exe probe-longjump.exe probe-pthread.exe query-stack.exe

all: $(ALL) $(GIST_ALL)

CFLAGS = -std=c2x -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE -O2
CFLAGS += -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none

%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

gcc-%: CC=gcc
gcc-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

musl-%: CC=musl-gcc
musl-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

clang-%: CC=clang
clang-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(GISTS_ALL)
	./gist-2603-stack-getrlimit.exe
	./gist-2603-stack-pthread.exe
	./gist-2603-stack-constructor.exe

gists:
	gcc gist-2603-stack-getrlimit.c && ./a.out
	gcc gist-2603-stack-pthread.c && ./a.out
	gcc gist-2603-stack-constructor.c && ./a.out


clean:
	rm -f *.exe
