CFLAGS=-g -Isrc

all: build/mongrel2

build/mongrel2: src/http11/http11_parser.o src/server.o src/adt/tst.o src/b64/b64.o src/task/libtask.a src/adt/hash.o
	$(CC) $(CLAGS) -o build/mongrel2 -lzmq -pthread -lsqlite3 src/http11/http11_parser.o src/server.o src/adt/tst.o src/adt/hash.o src/b64/b64.o src/task/libtask.a

build/mqshell: src/mqshell.o
	$(CC) $(CFLAGS) -o build/mqshell -lzmq -pthread src/mqshell.o

src/task/libtask.a:
	cd src/task && make clean && make

clean:
	find . -name "*.o" -exec rm {} \;
	cd src/task && make clean
	rm -f build/*

