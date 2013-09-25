PREFIX?=/usr/local
CFLAGS=-I../../src -I../../src/polarssl/include $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
LDFLAGS=$(OPTLIBS)

all: test_filter.so test_filter_a.so test_filter_b.so test_filter_c.so


%.so : %.c ../../build/libm2.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.so

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/filters/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/filters/

