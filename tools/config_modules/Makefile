PREFIX?=/usr/local
CFLAGS=-I../../src $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
CFLAGS+= -Imongo-c-driver/src -DMONGO_HAVE_STDINT 
LDFLAGS=$(OPTLIBS)

MONGO_SRC = mongo-c-driver

all: null.so zmq.so mongodb.so

.PHONY: fetch_mongo_c_driver
fetch_mongo_c_driver:
	if test -d mongo-c-driver; then git fetch; else git clone https://github.com/mongodb/mongo-c-driver.git; fi
	cd mongo-c-driver && git checkout v0.4
	gcc -fPIC -c mongo-c-driver/src/*.c -Imongo-c-driver/src/ -DMONGO_HAVE_STDINT

mongodb.so: fetch_mongo_c_driver
mongodb.so: mongodb.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< bson.o encoding.o gridfs.o md5.o mongo.o net.o numbers.o ../../build/libm2.a

%.so : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.o *.so

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/

