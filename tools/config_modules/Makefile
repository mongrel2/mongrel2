PREFIX?=/usr/local
CFLAGS=-I../../src -I../../src/polarssl/include $(OPTFLAGS) -fPIC -shared -nostartfiles -L../../build
LDFLAGS=$(OPTLIBS)

MONGO_SRC = mongo-c-driver/src/bson.c \
            mongo-c-driver/src/encoding.c \
            mongo-c-driver/src/gridfs.c \
            mongo-c-driver/src/md5.c \
            mongo-c-driver/src/mongo.c \
            mongo-c-driver/src/numbers.c \
            mongo-c-driver/src/env_posix.c
MONGO_CFLAGS = -fPIC -Imongo-c-driver/src -DMONGO_HAVE_STDINT

all: null.so zmq.so

.PHONY: fetch_mongo_c_driver
fetch_mongo_c_driver:
	if test ! -d mongo-c-driver; then git clone https://github.com/mongodb/mongo-c-driver.git; fi
	cd mongo-c-driver && git checkout v0.6
	

mongodb.so: CFLAGS += $(MONGO_CFLAGS)
mongodb.so: fetch_mongo_c_driver
mongodb.so: mongodb.c
	$(CC) $(CFLAGS) -c $(MONGO_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< bson.o encoding.o gridfs.o md5.o mongo.o env_posix.o numbers.o ../../build/libm2.a

%.so : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../../build/libm2.a

clean:
	rm -f *.o *.so
	rm -Rf mongo-c-driver

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/
	install *.so $(DESTDIR)/$(PREFIX)/lib/mongrel2/config_modules/

