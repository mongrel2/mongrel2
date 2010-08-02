CFLAGS=-g -I../../src -Isrc -Wall
LIBS=-lzmq -lsqlite3

all: kegogi

kegogi: src/kegogi.o src/fuzzrnd.o src/httpclient_parser.o src/kegogi_parser.o src/httpclient.o
	mkdir -p build
	gcc ${CFLAGS} -o build/kegogi src/kegogi.o src/kegogi_parser.o src/fuzzrnd.o src/httpclient_parser.o src/httpclient.o ${LIBS} ../../build/libm2.a
src/kegogi_parser.o: src/kegogi_parser.c
	gcc ${CFLAGS} -c src/kegogi_parser.c -o src/kegogi_parser.o
src/kegogi_parser.c: src/kegogi_parser.rl
	ragel src/kegogi_parser.rl -C -o src/kegogi_parser.c
clean:
	rm -f src/*.o build/*

