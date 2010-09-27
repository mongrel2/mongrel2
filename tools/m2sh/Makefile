CFLAGS=-DNDEBUG -DNO_LINENOS -g -I../../src -Isrc -Wall $(OPTFLAGS)
LIBS=-lzmq -lsqlite3 ../../build/libm2.a $(OPTLIBS)

PREFIX?=/usr/local
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})
LIB_SRC=$(filter-out src/m2sh.c,${SOURCES})
LIB_OBJ=$(filter-out src/m2sh.o,${OBJECTS})

all: ../lemon/lemon tests build/m2sh

dev: CFLAGS=-g -Wall -Wextra -Isrc -I../../src $(OPTFLAGS)
dev: all

install: build/m2sh
	install build/m2sh ${PREFIX}/bin

build/libm2sh.a: ${LIB_OBJ}
	mkdir -p build
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

build/m2sh: ../lemon/lemon ../../build/libm2.a build/libm2sh.a src/m2sh.o
	mkdir -p build
	$(CC) $(CFLAGS) $(LIBS) -o build/m2sh src/m2sh.o build/libm2sh.a ../../build/libm2.a

../lemon/lemon: ../lemon/lemon.c
	$(CC) -O2 ../lemon/lemon.c -o ../lemon/lemon

tests: build/libm2sh.a ${TESTS}
	sh ./tests/runtests.sh

%.c: %.y
	../lemon/lemon -s $< 

%.c: %.rl
	ragel -G2 $<

$(TESTS): %: %.c build/libm2sh.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< build/libm2sh.a ../../build/libm2.a

../../build/libm2.a:
	cd ../.. && make clean all

clean:
	rm -rf src/*.o build tests/*_tests tests/*.o

pristine: clean
	rm -f ../lemon/lemon

netbsd: OPTFLAGS=-I/usr/local/include -I/usr/pkg/include
netbsd: OPTLIBS=-L/usr/local/lib -L/usr/pkg/lib
netbsd: all

freebsd: OPTFLAGS=-I/usr/local/include
freebsd: OPTLIBS=-L/usr/local/lib
freebsd: all

