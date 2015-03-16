CFLAGS=-g -O2 -Wall -Wextra -Isrc -Isrc/polarssl/include -pthread -rdynamic -DNDEBUG $(OPTFLAGS) -D_FILE_OFFSET_BITS=64
LIBS=-lzmq -ldl -lsqlite3 $(OPTLIBS)
PREFIX?=/usr/local

get_objs = $(addsuffix .o,$(basename $(wildcard $(1))))

ASM=$(wildcard src/**/*.S src/*.S)
RAGEL_TARGETS=src/state.c src/http11/http11_parser.c
SOURCES=$(wildcard src/polarssl/library/*.c src/**/*.c src/*.c) $(RAGEL_TARGETS)
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.S,%.o,${ASM})
OBJECTS_EXTERNAL+=$(call get_objs,src/polarssl/library/*.c)
OBJECTS_NOEXT=$(filter-out ${OBJECTS_EXTERNAL},${OBJECTS})
LIB_SRC=$(filter-out src/mongrel2.c,${SOURCES})
LIB_OBJ=$(filter-out src/mongrel2.o,${OBJECTS})
TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})
MAKEOPTS=OPTFLAGS="${NOEXTCFLAGS} ${OPTFLAGS}" OPTLIBS="${OPTLIBS}" LIBS="${LIBS}" DESTDIR="${DESTDIR}" PREFIX="${PREFIX}"
ifdef $($(shell sh init.sh))
endif

all: bin/mongrel2 tests m2sh procer

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS) -D_FILE_OFFSET_BITS=64
dev: all

${OBJECTS_NOEXT}: CFLAGS += ${NOEXTCFLAGS}

src/polarssl/include/polarssl/config.h: src/polarssl_config.h
	cp src/polarssl_config.h src/polarssl/include/polarssl/config.h

bin/mongrel2: build/libm2.a src/mongrel2.o
	$(CC) $(CFLAGS) src/mongrel2.o -o $@ $< $(LIBS)

build/libm2.a: CFLAGS += -fPIC
build/libm2.a: build ${LIB_OBJ}
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

build: src/polarssl/include/polarssl/config.h
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
	find . \( -name "*.gcno" -o -name "*.gcda" \) -exec rm {} \;
	${MAKE} -C tools/m2sh OPTLIB=${OPTLIB} clean
	${MAKE} -C tools/filters OPTLIB=${OPTLIB} clean
	${MAKE} -C tests/filters OPTLIB=${OPTLIB} clean
	${MAKE} -C tools/config_modules OPTLIB=${OPTLIB} clean
	${MAKE} -C tools/procer OPTLIB=${OPTLIB} clean

pristine: clean
	sudo rm -rf examples/python/build examples/python/dist examples/python/m2py.egg-info
	sudo find . -name "*.pyc" -exec rm {} \;
	${MAKE} -C docs/manual clean
	cd docs/ && ${MAKE} clean
	${MAKE} -C examples/kegogi clean
	rm -f logs/*
	rm -f run/*
	${MAKE} -C tools/m2sh pristine
	${MAKE} -C tools/procer pristine

.PHONY: tests
tests: tests/config.sqlite ${TESTS} test_filters filters config_modules
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

m2sh: build/libm2.a
	${MAKE} ${MAKEOPTS} -C tools/m2sh all

procer: build/libm2.a
	${MAKE} ${MAKEOPTS} -C tools/procer all

test_filters: build/libm2.a
	${MAKE} ${MAKEOPTS} -C tests/filters all

filters: build/libm2.a
	${MAKE} ${MAKEOPTS} -C tools/filters all

config_modules: build/libm2.a
	${MAKE} ${MAKEOPTS} -C tools/config_modules all

# Try to install first before creating target directory and trying again
install: all
	install bin/mongrel2 $(DESTDIR)/$(PREFIX)/bin/ \
	    || ( install -d $(DESTDIR)/$(PREFIX)/bin/ \
	        && install bin/mongrel2 $(DESTDIR)/$(PREFIX)/bin/ )
	${MAKE} ${MAKEOPTS} -C tools/m2sh install
	${MAKE} ${MAKEOPTS} -C tools/config_modules install
	${MAKE} ${MAKEOPTS} -C tools/filters install
	${MAKE} ${MAKEOPTS} -C tools/procer install

examples/python/mongrel2/sql/config.sql: src/config/config.sql src/config/mimetypes.sql
	cat src/config/config.sql src/config/mimetypes.sql > $@

ragel:
	ragel -G2 src/state.rl
	ragel -G2 src/http11/http11_parser.rl
	ragel -G2 src/handler_parser.rl
	ragel -G2 src/http11/httpclient_parser.rl

valgrind:
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" ${MAKE}

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

coverage: NOEXTCFLAGS += -fprofile-arcs -ftest-coverage
coverage: LIBS += -lgcov
coverage: LDFLAGS += -fprofile-arcs
coverage: clean all coverage_report

coverage_report:
	rm -rf tests/m2.zcov tests/coverage
	zcov-scan tests/m2.zcov
	zcov-genhtml --root $(CURDIR) tests/m2.zcov tests/coverage
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

tarball:
	sh maketar.sh mongrel2-${VERSION}

release: tarball
	#git archive --format=tar --prefix=mongrel2-${VERSION}/ v${VERSION} | bzip2 -9 > mongrel2-${VERSION}.tar.bz2
	scp mongrel2-${VERSION}.tar.bz2 ${USER}@mongrel2.org:/var/www/mongrel2.org/static/downloads/
	md5sum mongrel2-${VERSION}.tar.bz2
	curl http://mongrel2.org/static/downloads/mongrel2-${VERSION}.tar.bz2 | md5sum

netbsd: OPTFLAGS += -I/usr/local/include -I/usr/pkg/include
netbsd: OPTLIBS += -L/usr/local/lib -L/usr/pkg/lib
netbsd: LIBS=-lzmq -lsqlite3 $(OPTLIBS)
netbsd: dev


freebsd: OPTFLAGS += -I/usr/local/include
freebsd: OPTLIBS += -L/usr/local/lib -pthread
freebsd: LIBS=-lzmq -lsqlite3 $(OPTLIBS)
freebsd: all

openbsd: OPTFLAGS += -I/usr/local/include
openbsd: OPTLIBS += -L/usr/local/lib -pthread
openbsd: LIBS=-lzmq -lsqlite3 $(OPTLIBS)
openbsd: all

solaris: OPTFLAGS += -I/usr/local/include
solaris: OPTLIBS += -L/usr/local/lib -R/usr/local/lib -lsocket -lnsl -lsendfile
solaris: OPTLIBS += -L/lib -R/lib
solaris: all


macports: OPTFLAGS += -I/opt/local/include
macports: OPTLIBS += -L/opt/local/lib -undefined dynamic_lookup
macports: all

