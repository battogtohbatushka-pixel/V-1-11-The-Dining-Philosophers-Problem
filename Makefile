CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pthread
all: philosophers
philosophers: src/philosophers.c
	$(CC) $(CFLAGS) -o $@ $<
test: philosophers
	bash tests/run_tests.sh
clean:
	rm -f philosophers tests/results.txt
