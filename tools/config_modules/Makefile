PREFIX?=/usr/local
CFLAGS=-I../../src $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
LDFLAGS=$(OPTLIBS)

MONGO_SRC = mongo-c-driver

all: null.so zmq.so mongodb.so

mongodb.so:
	$(CC) $(CFLAGS) -c -Wall -DMONGO_HAVE_STDINT -I${MONGO_SRC}/src mongodb.c ${MONGO_SRC}/src/*.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a bson.o encoding.o gridfs.o md5.o mongo.o net.o numbers.o mongodb.o

%.so : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.so

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/

