CFLAGS=-g -O2 -Wall -Isrc -rdynamic -DNDEBUG $(OPTFLAGS)
LIBS=-lzmq -ldl -lsqlite3 $(OPTLIBS)
PREFIX?=/usr/local

ASM=$(wildcard src/**/*.S src/*.S)
RAGEL_TARGETS=src/state.c src/http11/http11_parser.c
SOURCES=$(wildcard src/**/*.c src/*.c) $(RAGEL_TARGETS)
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.S,%.o,${ASM})
LIB_SRC=$(filter-out src/mongrel2.c,${SOURCES})
LIB_OBJ=$(filter-out src/mongrel2.o,${OBJECTS})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})

all: bin/mongrel2 tests m2sh

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: all

bin/mongrel2: build/libm2.a src/mongrel2.o
	$(CC) $(CFLAGS) src/mongrel2.o -o $@ $< $(LIBS)

build/libm2.a: OPTFLAGS += -fPIC
build/libm2.a: build ${LIB_OBJ}
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

build:
	@mkdir -p build
	@mkdir -p bin

clean:
	rm -rf build bin lib ${OBJECTS} ${TESTS} tests/config.sqlite
	rm -f tests/perf.log 
	rm -f tests/test.pid 
	rm -f tests/tests.log 
	rm -f tests/empty.sqlite 
	rm -f tools/lemon/lemon
	rm -f tools/m2sh/tests/tests.log 
	find . -name "*.gc*" -exec rm {} \;
	${MAKE} -C tools/m2sh OPTLIB=${OPTLIB} clean
	${MAKE} -C tools/filters OPTLIB=${OPTLIB} clean

pristine: clean
	sudo rm -rf examples/python/build examples/python/dist examples/python/m2py.egg-info
	sudo find . -name "*.pyc" -exec rm {} \;
	${MAKE} -C docs/manual clean
	cd docs/ && ${MAKE} clean
	${MAKE} -C examples/kegogi clean
	rm -f logs/*
	rm -f run/*
	${MAKE} -C tools/m2sh pristine

.PHONY: tests
tests: build/libm2.a tests/config.sqlite ${TESTS} filters
	sh ./tests/runtests.sh

tests/config.sqlite: src/config/config.sql src/config/example.sql src/config/mimetypes.sql
	sqlite3 $@ < src/config/config.sql
	sqlite3 $@ < src/config/example.sql
	sqlite3 $@ < src/config/mimetypes.sql

$(TESTS): %: %.c build/libm2.a
	$(CC) $(CFLAGS) -o $@ $< build/libm2.a $(LIBS)

src/state.c: src/state.rl src/state_machine.rl
src/http11/http11_parser.c: src/http11/http11_parser.rl
src/http11/httpclient_parser.c: src/http11/httpclient_parser.rl

check:
	@echo Files with potentially dangerous functions.
	@egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' $(filter-out src/bstr/bsafe.c,${SOURCES})

m2sh: 
	${MAKE} OPTFLAGS="${OPTFLAGS}" OPTLIBS="${OPTLIBS}" -C tools/m2sh all

filters: 
	${MAKE} OPTFLAGS="${OPTFLAGS}" OPTLIBS="${OPTLIBS}" -C tools/filters all

install: all install-bin install-m2sh

install-bin:
	install -d $(DESTDIR)/$(PREFIX)/bin/
	install bin/mongrel2 $(DESTDIR)/$(PREFIX)/bin/

install-m2sh:
	${MAKE} -C tools/m2sh install

examples/python/mongrel2/sql/config.sql: src/config/config.sql src/config/mimetypes.sql
	cat src/config/config.sql src/config/mimetypes.sql > $@

ragel:
	ragel -G2 src/state.rl
	ragel -G2 src/http11/http11_parser.rl
	ragel -G2 src/handler_parser.rl
	ragel -G2 src/http11/httpclient_parser.rl

valgrind:
	valgrind --leak-check=full --show-reachable=yes --log-file=valgrind.log --suppressions=tests/valgrind.sup ./bin/mongrel2 tests/config.sqlite localhost

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -fprofile-arcs
coverage: clean all coverage_report

coverage_report:
	rm -rf tests/m2.zcov tests/coverage
	zcov-scan tests/m2.zcov
	zcov-genhtml tests/m2.zcov tests/coverage
	zcov-summarize tests/m2.zcov

system_tests:
	./tests/system_tests/curl_tests
	./tests/system_tests/chat_tests

manual:
	dexy
	cp docs/manual/Makefile output/docs/manual/
	cp docs/manual/pastie.sty output/docs/manual/
	${MAKE} -C output/docs/manual clean book-final.pdf
	rm -rf output/docs/manual/*.dvi output/docs/manual/*.pdf
	${MAKE} -C output/docs/manual book-final.pdf
	${MAKE} -C output/docs/manual draft

netbsd: OPTFLAGS += -I/usr/local/include -I/usr/pkg/include
netbsd: OPTLIBS += -L/usr/local/lib -L/usr/pkg/lib
netbsd: dev


freebsd: OPTFLAGS += -I/usr/local/include
freebsd: OPTLIBS += -L/usr/local/lib -pthread
freebsd: all

openbsd: OPTFLAGS += -I/usr/local/include
openbsd: OPTLIBS += -L/usr/local/lib -pthread
openbsd: all

solaris: OPTFLAGS += -I/usr/local/include
solaris: OPTLIBS += -L/usr/local/lib -R/usr/local/lib -lsocket -lnsl -lsendfile
solaris: all


macports: OPTFLAGS += -I/opt/local/include
macports: OPTLIBS += -L/opt/local/lib
macports: all

