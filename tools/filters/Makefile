# quick Makefile to get this going, more advanced one on the way

all:
	gcc -I../../src -L../../build -fPIC -shared -nostartfiles -o test_filter.so test_filter.c ../../build/libm2.a
	gcc -I../../src -L../../build -fPIC -shared -nostartfiles -o test_filter_a.so test_filter_a.c ../../build/libm2.a
	gcc -I../../src -L../../build -fPIC -shared -nostartfiles -o test_filter_b.so test_filter_b.c ../../build/libm2.a
	gcc -I../../src -L../../build -fPIC -shared -nostartfiles -o test_filter_c.so test_filter_c.c ../../build/libm2.a

clean:
	rm -f test_filter*.so

