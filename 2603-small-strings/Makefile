default: all

ALL = vla-str-1.exe

all: $(ALL)

CFLAGS = -std=c11 -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE -O2
CFLAGS += -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none
%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(ALL)
	./vla-str-1.exe

clean:
	rm -f *.exe
