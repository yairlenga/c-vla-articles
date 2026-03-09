default: all

ALL = vla-str-1.exe
EXE = vla-x1.exe vla-x2.exe vla-x3.exe vla-x4.exe vla-x5.exe vla-x6.exe vla-d1.exe vla-d2.exe

all: $(ALL)

exe: $(EXE)

CFLAGS = -std=c11 -Wall -Wextra -Werror -g -D_DEFAULT_SOURCE -O2
CFLAGS += -fno-asynchronous-unwind-tables -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none
%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: vla-x1.exe vla-x2.exe vla-x3.exe
	./vla-x1.exe 
	./vla-x2.exe
	./vla-x3.exe
	./vla-x4.exe

clean:
	rm -f $(EXE)
