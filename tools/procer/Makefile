CFLAGS=-DNDEBUG -pthread -g -I../../src -Wall $(OPTFLAGS)
PREFIX?=/usr/local
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})

all: procer


procer: ../../build/libm2.a ${OBJECTS}
	gcc $(OPTFLAGS) $(OPTLIBS) -o $@ ${OBJECTS} ${LIBS} ../../build/libm2.a

clean:
	rm -f *.o procer

pristine:
	rm -f *.o procer

install: procer
	install procer $(DESTDIR)$(PREFIX)/bin/
