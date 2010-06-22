CFLAGS=-g -Isrc

all: build/mongrel2 tests

TESTS=tests/host_tests tests/register_tests tests/proxy_tests tests/handler_tests \
	  tests/listener_tests tests/server_tests tests/tst_tests tests/pattern_tests \
	  tests/routing_tests

OBJS=src/http11/http11_parser.o src/server.o src/adt/tst.o src/b64/b64.o src/task/libtask.a \
	 src/adt/hash.o src/proxy.o src/register.o src/listener.o \
	 src/handler.o src/adt/list.o src/host.o src/pattern.o \
	 src/routing.o

LIBS=-lzmq -pthread -lsqlite3

build/mongrel2: $(OBJS) src/mongrel2.o
	if [ ! -d build ]; then mkdir build; fi
	$(CC) $(CLAGS) -o build/mongrel2 $(LIBS) $(OBJS) src/mongrel2.o

# don't build this, it's not really ready yet
build/mqshell: src/mqshell.o
	$(CC) $(CFLAGS) -o build/mqshell $(LIBS) src/mqshell.o

src/task/libtask.a:
	cd src/task && make clean && make

clean:
	find . -name "*.o" -exec rm {} \;
	cd src/task && make clean
	rm -f build/*
	find tests -name "*_tests" -exec rm -rf {} \;
	cd docs && make clean


tests: $(TESTS)
	find tests -maxdepth 1 -name "*_tests" -a -exec {} \;	

%_tests: %_tests.c
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) $< -o $@
	
