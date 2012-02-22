CC=gcc
CFLAGS=-I. -Wall -g

BIN=bin/

sassc: sassc.o 
	$(CC) $(CFLAGS) -o $(BIN)sassc sassc.o libsass.o context.o

sassc.o: libsass.o
libsass.o: context.o

bstr: bstr/bsafe.o
bstr/bsafe.o:

build: prefix.o context.o exception.o

prefix.o:
context.o:
exception.o:


test_context: context.o test_context.o
	$(CC) $(CFLAGS) -o $(BIN)test_context test_context.o context.o
	./$(BIN)test_context
test_context.o: 

clean:
	rm -rf *.o
	rm -rf sassc