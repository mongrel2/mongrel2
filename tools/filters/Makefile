PREFIX?=/usr/local
CFLAGS=-I../../src $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
LDFLAGS=$(OPTLIBS)

all: null.so xff.so


%.so : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.so

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/filters/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/filters/

