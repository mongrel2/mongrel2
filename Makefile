CFLAGS=-fnested-functions -g -Isrc

all: server

server: src/http11/http11_parser.o src/server.o src/adt/tst.o src/b64/b64.o src/task/libtask.a
	$(CC) ${CLAGS} -o server src/task/libtask.a -lzmq -pthread -lsqlite3 src/http11/http11_parser.o src/server.o src/adt/tst.o src/b64/b64.o

src/task/libtask.a:
	cd src/task && make clean && make

clean:
	find . -name "*.o" -exec rm {} \;
	cd src/task && make clean
	rm -f server

