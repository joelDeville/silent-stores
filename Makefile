build-test:
	gcc -o ss -msse2 ss_test.c

build-multi-test:
	gcc -o multi-ss -msse2 multithread_ss_test.c

clean:
	rm -f ss
	rm -f multi_ss
