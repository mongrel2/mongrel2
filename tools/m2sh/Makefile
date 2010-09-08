CFLAGS=-g -I../../src -Isrc # -Wall -Wextra
LIBS=-lzmq -lsqlite3 ../../build/libm2.a

PREFIX?=/usr/local
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})
TEST_SRC=$(wildcard tests/*.c)
TESTS=$(patsubst %.c,%,${TEST_SRC})
LIB_SRC=$(filter-out src/m2sh.c,${SOURCES})
LIB_OBJ=$(filter-out src/m2sh.o,${OBJECTS})

all: ../lemon/lemon tests m2sh

install: all
	install build/m2sh ${PREFIX}

build/libm2sh.a: ${LIB_OBJ}
	mkdir -p build
	ar rcs $@ ${LIB_OBJ}
	ranlib $@

m2sh: ../lemon/lemon ../../build/libm2.a build/libm2sh.a src/m2sh.o
	mkdir -p build
	$(CC) ${CFLAGS} -o build/m2sh ${LIBS} build/libm2sh.a src/m2sh.o

../lemon/lemon: ../lemon/lemon.c
	$(CC) -O2 ../lemon/lemon.c -o ../lemon/lemon

tests: build/libm2sh.a ${TESTS}
	sh ./tests/runtests.sh

%.c: %.y
	../lemon/lemon -s $< 

%.c: %.rl
	ragel -T1 $<

$(TESTS): %: %.c build/libm2sh.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< build/libm2sh.a ../../build/libm2.a

../../build/libm2.a:
	cd ../.. && make clean all

clean:
	rm -rf src/*.o build tests/*_tests tests/*.o

pristine: clean
	rm ../lemon/lemon
