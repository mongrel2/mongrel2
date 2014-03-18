CFLAGS=-g -I../../src -Isrc -Wall -Wextra
LIBS=-lzmq -lsqlite3

all: kegogi

install: all
	sudo install build/kegogi /usr/local/bin/

kegogi: src/kegogi.o src/fuzzrnd.o src/kegogi_parser.o src/httpclient.o src/param.o src/kegogi_lexer.o src/kegogi_tokens.o
	mkdir -p build
	$(CC) ${CFLAGS} -o build/kegogi src/kegogi.o src/kegogi_parser.o src/kegogi_lexer.o src/fuzzrnd.o src/httpclient.o src/param.o src/kegogi_tokens.o ${LIBS} ../../build/libm2.a

src/kegogi_lexer.o: src/kegogi_lexer.c
	$(CC) $(CFLAGS) -c src/kegogi_lexer.c -o src/kegogi_lexer.o

src/kegogi_lexer.c: src/kegogi_lexer.rl src/kegogi_parser.h
	ragel src/kegogi_lexer.rl -C -o src/kegogi_lexer.c

src/kegogi_parser.c: src/kegogi_parser.y src/kegogi_parser_extra.c lemon
	./lemon src/kegogi_parser.y
	cat src/kegogi_parser_extra.c >> src/kegogi_parser.c
	echo "foo"
clean:
	rm -f src/*.o build/*

