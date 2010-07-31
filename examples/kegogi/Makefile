CFLAGS=-g -I../../src -Isrc
LIBS=-lzmq -lsqlite3 ../../build/libm2.a

all: kegogi

kegogi: src/kegogi.o src/fuzzrnd.o src/httpclient_parser.o
	mkdir -p build
	gcc ${CFLAGS} -o build/kegogi $< ${LIBS}

clean:
	rm -f src/*.o build/*

