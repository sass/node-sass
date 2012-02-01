CC=gcc
CFLAGS=-I.

BIN=bin/
BUILD=build/
SRC=src/

sassc: sassc.o
	$(CC) -o $(BIN)sassc libsass.o
sassc.o:
	$(CC) -c -g $(SRC)sassc.c
libsass.o:
	$(CC) -c -g $(SRC)libsass.c