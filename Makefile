CC=gcc
CFLAGS=-I.

SRC=src/

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o build/$@

sassc: sassc.o
	$(CC) -o $(BIN)sassc src/sassc.o
sassc.o: libsass.o
	$(CC) -c -g $(SRC)sassc.c
libsass.o: