PREFIX?=/usr/local
CFLAGS=-I../../src $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
LDFLAGS=$(OPTLIBS)

LDFLAGS_mongodb.so = -L./mongo-c-driver -lmongoc -lbson
CFLAGS_mongodb.so = -I./mongo-c-driver/src

sub_folders:
	cd mongo-c-driver && scons

all: sub_folders null.so zmq.so mongodb.so

%.so : %.c
	$(CC) $(CFLAGS) $(CFLAGS_$@) $(LDFLAGS) $(LDFLAGS_$@) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.so


install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/

