CC      = g++
CFLAGS  = -Wall -O2 -fPIC
LDFLAGS = -fPIC

PREFIX  = /usr/local
LIBDIR  = $(PREFIX)/lib

SOURCES = ast.cpp bind.cpp constants.cpp context.cpp contextualize.cpp \
	copy_c_str.cpp error_handling.cpp eval.cpp expand.cpp extend.cpp file.cpp \
	functions.cpp inspect.cpp output_compressed.cpp output_nested.cpp \
	parser.cpp prelexer.cpp sass.cpp sass_interface.cpp to_c.cpp to_string.cpp \
	units.cpp

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

bin: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) sassc++.cpp -o sassc++

clean:
	rm -f $(OBJECTS) *.a *.so sassc++


.PHONY: static shared install install-shared clean
