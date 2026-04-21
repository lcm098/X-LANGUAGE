CC=gcc
CFLAGS=-Wall -Wextra -std=c11

SRC=src/main.c src/x.c src/scanner.c src/parser.c src/debug_token.c src/ast_printer.c src/interpreter.c src/environment.c
OUT=x

build:
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)

run:
	./x

clean:
	rm -f $(OUT)