# quick Makefile to get this going, more advanced one on the way

clean:
	rm test_filter.so

all:
	gcc -I../../src -L../../build -fPIC -shared -nostartfiles -o test_filter.so test_filter.c ../../build/libm2.a
