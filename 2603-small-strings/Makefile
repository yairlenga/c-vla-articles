default: all

ALL = gcc-vla-str-1.exe clang-vla-str-1.exe musl-vla-str-1.exe

all: $(ALL)

CFLAGS = -std=c11 -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE -O2
CFLAGS += -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none

gcc-%: CC=gcc
gcc-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

musl-%: CC=musl-gcc
musl-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

clang-%: CC=clang
clang-%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(ALL)
	./gcc-vla-str-1.exe
	./clang-vla-str-1.exe
	./musl-vla-str-1.exe
	LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libmimalloc.so ./gcc-vla-str-1.exe
	LD_PRELOAD=/usr/lib/clang/18/lib/linux/libclang_rt.scudo_standalone-x86_64.so ./gcc-vla-str-1.exe

clean:
	rm -f *.exe
