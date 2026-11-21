
CC     ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pthread

.PHONY: all test run deadlock tsan examples web clean

all: philosophers

philosophers: src/philosophers.c
	$(CC) $(CFLAGS) -o $@ $<

test: philosophers
	bash tests/run_tests.sh

run: philosophers
	./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42

deadlock: philosophers
	./philosophers --naive -n 5 -m 3 -t 10,10 -e 10,10; rc=$$?; \
	if [ $$rc -eq 3 ]; then echo "rc=3: deadlock илэрлээ (хүлээгдсэн үр дүн)"; else echo "rc=$$rc"; fi

tsan: src/philosophers.c
	$(CC) -O1 -g -fsanitize=thread -pthread -o philosophers_tsan $<
	./philosophers_tsan -q -n 5  -m 20 -t 1,3 -e 1,3
	./philosophers_tsan -q -n 2  -m 30 -t 1,2 -e 1,2
	./philosophers_tsan -q -n 64 -m 3  -t 1,5 -e 1,5
	@echo "tsan: data race олдсонгүй"

examples: philosophers
	./philosophers -n 5 -m 2 -t 50,100 -e 50,100 -s 42 > examples/sample_run_n5.log
	./philosophers --naive -n 5 -m 3 -t 10,10 -e 10,10 > examples/naive_deadlock_n5.log || [ $$? -eq 3 ]
	for n in 2 3 5 8 12; do ./philosophers -q -n $$n -m 5 -t 20,60 -e 20,60 -s 7; done > examples/scaling.txt

# Онлайн хувилбарын WebAssembly-г дахин үүсгэнэ (src/philosophers.c өөрчлөгдсөн үед л).
# Хэрэгтэй: sudo apt install clang lld wasi-libc libclang-rt-18-dev-wasm32 binaryen
web: src/philosophers.c web/gt.c web/pthread.h
	sh web/build.sh

clean:
	rm -f philosophers philosophers_tsan tests/results.txt
