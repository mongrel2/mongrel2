
CFLAGS=-g -Wall -Isrc
LIBS=-lzmq -lsqlite3

ASM=$(wildcard src/**/*.S src/*.S)
SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.S,%.o,${ASM})
LIB_SRC=$(filter-out src/mongrel2.c,${SOURCES})
LIB_OBJ=$(filter-out src/mongrel2.o,${OBJECTS})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})

all: bin/mongrel2 tests

bin/mongrel2: build/libm2.a src/mongrel2.o
	$(CC) $(CFLAGS) $(LIBS) src/mongrel2.o -o $@ $<

build/libm2.a: build ${LIB_OBJ}
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

build:
	@mkdir -p build
	@mkdir -p bin

clean:
	rm -rf build bin lib ${OBJECTS} ${TESTS} tests/config.sqlite

tests: build/libm2.a tests/config.sqlite ${TESTS}

tests/config.sqlite: src/config/config.sql src/config/example.sql
	sqlite3 $@ < src/config/config.sql
	sqlite3 $@ < src/config/example.sql

$(TESTS): %: %.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< build/libm2.a
	$@

check:
	@echo Files with potentially dangerous functions.
	@egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' $(filter-out src/bstr/bsafe.c,${SOURCES})

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@
