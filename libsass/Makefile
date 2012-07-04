CC=g++
CFLAGS=-c -Wall -O2 -fPIC
LDFLAGS= -fPIC
SOURCES = \
	context.cpp functions.cpp document.cpp \
	document_parser.cpp eval_apply.cpp node.cpp \
	node_factory.cpp node_emitters.cpp prelexer.cpp \
	sass_interface.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(OBJECTS)
	ar rvs libsass.a $(OBJECTS)

shared: $(OBJECTS)
	$(CC) -shared -o libsass.so *.o

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *.a *.so