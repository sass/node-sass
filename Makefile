CC      = g++
CFLAGS  = -Wall -O2 -fPIC
LDFLAGS = -fPIC

PREFIX  = /usr/local
LIBDIR  = $(PREFIX)/lib

SOURCES = constants.cpp context.cpp functions.cpp document.cpp \
          document_parser.cpp eval_apply.cpp node.cpp \
          node_factory.cpp node_emitters.cpp prelexer.cpp \
          selector.cpp sass_interface.cpp

OBJECTS = $(SOURCES:.cpp=.o)

static: libsass.a
shared: libsass.so

libsass.a: $(OBJECTS)
	ar rvs $@ $(OBJECTS)

libsass.so: $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -o $@ $(OBJECTS)

.cpp.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install: libsass.a
	install -Dpm0755 $< $(DESTDIR)$(LIBDIR)/$<

install-shared: libsass.so
	install -Dpm0755 $< $(DESTDIR)$(LIBDIR)/$<

clean:
	rm -f $(OBJECTS) *.a *.so


.PHONY: static shared install install-shared clean
