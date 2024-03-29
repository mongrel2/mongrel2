CFLAGS=-DNDEBUG -DNO_LINENOS -pthread -g -I../../src -Isrc -Wall $(OPTFLAGS)
LIBS=-lzmq -lsqlite3 ../../build/libm2.a $(OPTLIBS)

PREFIX?=/usr/local
SOURCES=$(wildcard src/*.c src/**/*.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})
LIB_SRC=$(filter-out src/m2sh.c,${SOURCES})
LIB_OBJ=$(filter-out src/m2sh.o,${OBJECTS})
RAGEL_OBJECTS=src/lexer.o src/cli.o

$(RAGEL_OBJECTS): CFLAGS += -Wno-unused-const-variable -Wimplicit-fallthrough=0 -Wno-unused-parameter

all: ../lemon/lemon tests build/m2sh

dev: CFLAGS=-g -Wall -Wextra -Isrc -I../../src $(OPTFLAGS)
dev: all

install: build/m2sh
	install build/m2sh ${DESTDIR}${PREFIX}/bin

src/parser.o: CFLAGS += -Wno-unused-const-variable -Wno-unused-parameter

build/libm2sh.a: ${LIB_OBJ}
	mkdir -p build
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

build/m2sh: ../lemon/lemon ../../build/libm2.a ${OBJECTS}
	mkdir -p build
	$(CC) $(CFLAGS) -o build/m2sh ${OBJECTS} ../../build/libm2.a $(LDFLAGS) $(LIBS)

../lemon/lemon: ../lemon/lemon.c
	$(CC) -O2 ../lemon/lemon.c -o ../lemon/lemon

tests: build/libm2sh.a ${TESTS}
	sh ./tests/runtests.sh

%.c: %.y
	../lemon/lemon -s $< 

%.c: %.rl
	ragel -G2 $<

$(TESTS): %: %.c build/libm2sh.a
	$(CC) $(CFLAGS) -o $@ $< build/libm2sh.a ../../build/libm2.a $(LDFLAGS) $(LIBS)

../../build/libm2.a:
	cd ../.. && make clean all

clean:
	rm -rf src/*.o src/*/*.o build tests/*_tests tests/*.o

pristine: clean
	rm -f ../lemon/lemon
