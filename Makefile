CXX      = g++
CXXFLAGS = -Wall -O2 -fPIC -g
LDFLAGS  = -fPIC

PREFIX    = /usr/local
LIBDIR    = $(PREFIX)/lib

SASS_SASSC_PATH ?= sassc
SASS_SPEC_PATH ?= sass-spec
SASSC_BIN = $(SASS_SASSC_PATH)/bin/sassc

SOURCES = \
	src/ast.cpp \
	src/base64vlq.cpp \
	src/bind.cpp \
	src/constants.cpp \
	src/context.cpp \
	src/contextualize.cpp \
	src/copy_c_str.cpp \
	src/error_handling.cpp \
	src/eval.cpp \
	src/expand.cpp \
	src/extend.cpp \
	src/file.cpp \
	src/functions.cpp \
	src/inspect.cpp \
	src/output_compressed.cpp \
	src/output_nested.cpp \
	src/parser.cpp \
	src/prelexer.cpp \
	src/sass.cpp \
	src/sass_interface.cpp \
	src/source_map.cpp \
	src/to_c.cpp \
	src/to_string.cpp \
	src/units.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: static

static: libsass.a
shared: libsass.so

libsass.a: $(OBJECTS)
	ar rvs $@ $(OBJECTS)

libsass.so: $(OBJECTS)
	$(CXX) -shared $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.o libsass.a
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

install: libsass.a
	install -Dpm0755 $< $(DESTDIR)$(LIBDIR)/$<

install-shared: libsass.so
	install -Dpm0755 $< $(DESTDIR)$(LIBDIR)/$<

$(SASSC_BIN): libsass.a
	cd $(SASS_SASSC_PATH) && make

test: $(SASSC_BIN) libsass.a
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_build: $(SASSC_BIN) libsass.a
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_issues: $(SASSC_BIN) libsass.a
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) $(LOG_FLAGS) $(SASS_SPEC_PATH)/spec/issues

clean:
	rm -f $(OBJECTS) *.a *.so

.PHONY: all static shared bin install install-shared clean
