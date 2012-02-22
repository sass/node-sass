CC=gcc
CFLAGS=-I.


sassc: sassc.o 
	$(CC) -o $(BIN)sassc sassc.o libsass.o

sassc.o: libsass.o
libsass.o: bstr

bstr: bstr/bsafe.o
bstr/bsafe.o:

clean:
	rm -rf *.o
	rm -rf sassc