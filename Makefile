SHELL := /bin/bash

# Deprecated test
build-test:
	gcc -o ss -msse2 old_tests/ss_test.c

# Deprecated test
build-multi-test:
	gcc -o multi-ss -msse2 old_tests/multithread_ss_test.c

build-ntest:
	gcc -pthread -O3 -o new_test silent_detect.c

clean:
	rm -f ss
	rm -f multi_ss
	rm -f new_test
