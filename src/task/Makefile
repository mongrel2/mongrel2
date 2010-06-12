LIB=libtask.a
TCPLIBS=-lzmq -pthread

ASM=asm.o
OFILES=\
	$(ASM)\
	context.o\
	fd.o\
	net.o\
	print.o\
	qlock.o\
	rendez.o\
	task.o\

all: $(LIB)

$(OFILES): taskimpl.h task.h 386-ucontext.h power-ucontext.h

AS=gcc -c
CC=gcc
CFLAGS=-Wall -c -I. -ggdb

%.o: %.S
	$(AS) $*.S

%.o: %.c
	$(CC) $(CFLAGS) $*.c

$(LIB): $(OFILES)
	ar rvc $(LIB) $(OFILES)
	ranlib $(LIB)

clean:
	rm -f *.o $(LIB)

install: $(LIB)
	cp $(LIB) /usr/local/lib
	cp task.h /usr/local/include

