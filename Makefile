CC=gcc
CFLAGS=-I. -Wall -g

BIN=bin/

sassc: sassc.o 
	$(CC) $(CFLAGS) -o $(BIN)sassc sassc.o libsass.o context.o parser.o emitter.o transforms.o

sassc.o: libsass.o
libsass.o: context.o parser.o emitter.o transforms.o

bstr: bstr/bsafe.o
bstr/bsafe.o:

build: prefix.o context.o exception.o parser.o

prefix.o:
exception.o:

context.o: tree.h
parser.o: context.o
emitter.o: context.o
transforms.o: context.o

test: clean build sassc
	./spec.rb spec/basic/

test_context: context.o test_context.o
	$(CC) $(CFLAGS) -o $(BIN)test_context test_context.o context.o
	./$(BIN)test_context
test_context.o: 

clean: clean_bin
	rm -rf *.o
clean_bin:
	rm -rf bin/*