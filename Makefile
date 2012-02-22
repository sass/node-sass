CC=gcc
CFLAGS=-I.

BIN=bin/

sassc: sassc.o 
	$(CC) -o $(BIN)sassc sassc.o libsass.o

sassc.o: libsass.o
libsass.o: bstr

bstr: bstr/bsafe.o
bstr/bsafe.o:

build: prefix.o context.o exception.o

prefix.o:
context.o:
exception.o:


test_context: context.o test_context.o
	$(CC) -o $(BIN)test_context test_context.o context.o
	./$(BIN)test_context
test_context.o: 

clean:
	rm -rf *.o
	rm -rf sassc