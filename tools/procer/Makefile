CFLAGS=-DNDEBUG -pthread -g -I../../src -Wall $(OPTFLAGS)
PREFIX?=/usr/local
LIBS?=-lzmq 
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})

all: procer


procer: ../../build/libm2.a ${OBJECTS}
	$(CC) $(OPTFLAGS) $(OPTLIBS) -o $@ ${OBJECTS} ../../build/libm2.a ${LIBS}

clean:
	rm -f *.o procer

pristine:
	rm -f *.o procer

install: procer
	install procer $(DESTDIR)$(PREFIX)/bin/
