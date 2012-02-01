CC=gcc
CFLAGS=-I.

BIN=bin/
BUILD=build/
SRC=src/

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

libsass: libsass.o
	$(CC) -o $(BIN)main libsass.o
libsass.o:
	$(CC) -c -g $(SRC)libsass.c