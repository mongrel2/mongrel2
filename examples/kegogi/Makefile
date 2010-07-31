CFLAGS=-g -I../../src -Isrc
LIBS=-lzmq -lsqlite3

all: kegogi

kegogi: src/kegogi.o src/fuzzrnd.o src/httpclient_parser.o 
	mkdir -p build
	gcc ${CFLAGS} -o build/kegogi src/kegogi.o src/fuzzrnd.o src/httpclient_parser.o ${LIBS} ../../build/libm2.a

clean:
	rm -f src/*.o build/*

