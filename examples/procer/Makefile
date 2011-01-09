CFLAGS=-I../../src -g $(OPTFLAGS) $(OPTLIBS)
LIBS=../../build/libm2.a -lzmq -lsqlite3
PREFIX?=/usr/local

all: procer


procer: rampart.o profile.o procer.o
	gcc $(OPTFLAGS) $(OPTLIBS) -g -o $@ rampart.o profile.o procer.o ${LIBS}

clean:
	rm -f *.o procer

install:
	sudo install procer $(PREFIX)/bin/
