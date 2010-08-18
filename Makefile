CFLAGS=-g -Wall -Isrc 
LIBS=-lzmq -lsqlite3


ASM=$(wildcard src/**/*.S src/*.S)
RAGEL_TARGETS=src/state.c src/http11/http11_parser.c
SOURCES=$(wildcard src/**/*.c src/*.c) $(RAGEL_TARGETS)
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.S,%.o,${ASM})
LIB_SRC=$(filter-out src/mongrel2.c,${SOURCES})
LIB_OBJ=$(filter-out src/mongrel2.o,${OBJECTS})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})

all: bin/mongrel2 tests

release: CFLAGS=-g -O2 -Wall -Isrc -DNDEBUG
release: all

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
	find . -name "*.gc*" -exec rm {} \;

pristine: clean
	sudo rm -rf examples/python/build examples/python/dist examples/python/m2py.egg-info
	sudo find . -name "*.pyc" -exec rm {} \;
	cd docs/manual && make clean
	cd docs/ && make clean
	cd examples/kegogi && make clean
	rm -f logs/*
	rm -f run/*

tests: build/libm2.a tests/config.sqlite ${TESTS}
	sh ./tests/runtests.sh

tests/config.sqlite: src/config/config.sql src/config/example.sql src/config/mimetypes.sql
	sqlite3 $@ < src/config/config.sql
	sqlite3 $@ < src/config/example.sql
	sqlite3 $@ < src/config/mimetypes.sql

$(TESTS): %: %.c build/libm2.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< build/libm2.a

src/state.c: src/state.rl src/state_machine.rl

src/http11/http11_parser.c: src/http11/http11_parser.rl src/http11/http11_parser_common.rl

check:
	@echo Files with potentially dangerous functions.
	@egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' $(filter-out src/bstr/bsafe.c,${SOURCES})

install: python all
	sudo install bin/mongrel2 /usr/local/bin/

python: examples/python/mongrel2/sql/config.sql
	cd examples/python && sudo python setup.py install

examples/python/mongrel2/sql/config.sql: src/config/config.sql src/config/mimetypes.sql
	cat src/config/config.sql src/config/mimetypes.sql > $@

ragel:
	ragel -G2 src/state.rl
	ragel -G2 src/http11/http11_parser.rl
	ragel -G2 src/handler_parser.rl

valgrind:
	valgrind --leak-check=full --show-reachable=yes --log-file=valgrind.log --suppressions=tests/valgrind.sup ./bin/mongrel2 tests/config.sqlite localhost

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: clean all
	rm -rf tests/m2.zcov tests/coverage
	zcov-scan tests/m2.zcov
	zcov-genhtml tests/m2.zcov tests/coverage
	zcov-summarize tests/m2.zcov
