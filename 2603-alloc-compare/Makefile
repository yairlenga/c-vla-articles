default: all

EXE = alloc-bench glibc-bench musl-bench mimalloc-bench jemalloc-bench tcmalloc-bench clang-bench

all: $(EXE) alloc_bench.s

CFLAGS = -g -D_DEFAULT_SOURCE -O -march=native -std=c11 -fno-stack-protector -fno-stack-clash-protection
LDFLAGS =-lm

alloc-bench: alloc_bench.c
	gcc $(CFLAGS) $< $(LDFLAGS) -Wall -Werror -Wextra -Og -o $@

alloc_bench.s: alloc_bench.c
	gcc -S $(CFLAGS) $< -fverbose-asm -fno-omit-frame-pointer


glibc-bench: alloc_bench.c
	gcc $(CFLAGS) $< $(LDFLAGS) -o $@

clang-bench: alloc_bench.c
	clang $(CFLAGS) $< $(LDFLAGS) -o $@

mimalloc-bench: alloc_bench.c
	gcc $(CFLAGS) $< $(LDFLAGS) -lmimalloc -o $@

musl-bench: alloc_bench.c
	musl-gcc $(CFLAGS) $< $(LDFLAGS) -o $@

JEMALLOC_LDFLAGS = -L"$(jemalloc-config --libdir)" -Wl,-rpath,"$(jemalloc-config --libdir)" -ljemalloc $(jemalloc-config --libs)

jemalloc-bench: alloc_bench.c
	gcc $(CFLAGS) $< $(LDFLAGS) $(JEMALLOC_LDFLAGS) -o $@

tcmalloc-bench: alloc_bench.c
	gcc $(CFLAGS) $< -ltcmalloc -lm -o $@

clean:
	rm -f $(EXE)

SP = $E $E

test: $(EXE)
	./alloc-bench $(ARGS) | sed -e "s/^strategy:/result alloc-bench $(LABEL) /"
	./glibc-bench $(ARGS) | sed -e "s/^strategy:/result gcc-glibc $(LABEL) /"
	./clang-bench $(ARGS) | sed -e "s/^strategy:/result clang-glibc $(LABEL) /"
	./musl-bench $(ARGS) | sed -e "s/^strategy:/result gcc-musl $(LABEL) /"
	./mimalloc-bench $(ARGS) | sed -e "s/^strategy:/result gcc-mimalloc $(LABEL) /"
	./jemalloc-bench $(ARGS) | sed -e "s/^strategy:/result gcc-jemalloc $(LABEL) /"
	./tcmalloc-bench $(ARGS) | sed -e "s/^strategy:/result gcc-tcmalloc $(LABEL) /"

test.txt: $(EXE)
	$(MAKE) test | tee $@

test-s.txt: $(EXE)
	$(MAKE) test ARGS="0 1000 8000" LABEL="small" | tee $@

test-m.txt: $(EXE)
	$(MAKE) test ARGS="0 1000 12000" LABEL="medium" | tee $@

test-l.txt: $(EXE)
	$(MAKE) test ARGS="0 1000 16000" LABEL="large" | tee $@

report.txt: test-s.txt test-m.txt test-l.txt
	grep -h ^result $^ > report.txt

report: report.txt
