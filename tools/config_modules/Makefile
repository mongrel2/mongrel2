PREFIX?=/usr/local

# quick Makefile to get this going, more advanced one on the way

all:
	gcc -I../../src $(OPTFLAGS) -L../../build $(OPTLIBS) -fPIC -shared -nostartfiles -o zmq.so zmq.c ../../build/libm2.a
	gcc -I../../src $(OPTFLAGS) -L../../build $(OPTLIBS) -fPIC -shared -nostartfiles -o null.so null.c ../../build/libm2.a

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/

clean:
	rm -f *.so

