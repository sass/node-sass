CXX      ?= g++
CXXFLAGS = -std=c++11 -Wall -fPIC -O2 $(EXTRA_CFLAGS)
LDFLAGS  = -fPIC $(EXTRA_LDFLAGS)

ifneq (,$(findstring /cygdrive/,$(PATH)))
	UNAME := Cygwin
else
ifneq (,$(findstring WINDOWS,$(PATH)))
	UNAME := Windows
else
	UNAME := $(shell uname -s)
endif
endif

ifeq ($(UNAME),Darwin)
	CXXFLAGS += -stdlib=libc++
endif

PREFIX    = /usr/local
LIBDIR    = $(PREFIX)/lib

SASS_SASSC_PATH ?= sassc
SASS_SPEC_PATH ?= sass-spec
SASS_SPEC_SPEC_DIR ?= spec
SASSC_BIN = $(SASS_SASSC_PATH)/bin/sassc
RUBY_BIN = ruby

SOURCES = \
	ast.cpp \
	base64vlq.cpp \
	bind.cpp \
	constants.cpp \
	context.cpp \
	contextualize.cpp \
	copy_c_str.cpp \
	error_handling.cpp \
	eval.cpp \
	expand.cpp \
	extend.cpp \
	file.cpp \
	functions.cpp \
	inspect.cpp \
	node.cpp \
	output_compressed.cpp \
	output_nested.cpp \
	parser.cpp \
	prelexer.cpp \
	remove_placeholders.cpp \
	sass.cpp \
	sass_interface.cpp \
	sass_util.cpp \
	sass2scss.cpp \
	source_map.cpp \
	to_c.cpp \
	to_string.cpp \
	units.cpp \
	utf8_string.cpp \
	util.cpp

OBJECTS = $(SOURCES:.cpp=.o)

DEBUG_LVL ?= NONE

all: static

debug: LDFLAGS := -g
debug: CXXFLAGS := -g -DDEBUG -DDEBUG_LVL="$(DEBUG_LVL)" $(filter-out -O2,$(CXXFLAGS))
debug: static

debug-shared: LDFLAGS := -g
debug-shared: CXXFLAGS := -g -DDEBUG -DDEBUG_LVL="$(DEBUG_LVL)" $(filter-out -O2,$(CXXFLAGS))
debug-shared: shared

static: libsass.a
shared: libsass.so

libsass.a: $(OBJECTS)
	$(AR) rvs $@ $(OBJECTS)

libsass.so: $(OBJECTS)
	$(CXX) -shared $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.o libsass.a
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

install: libsass.a
	mkdir -p $(DESTDIR)$(LIBDIR)/
	install -pm0755 $< $(DESTDIR)$(LIBDIR)/$<

install-shared: libsass.so
	mkdir -p $(DESTDIR)$(LIBDIR)/
	install -pm0755 $< $(DESTDIR)$(LIBDIR)/$<

$(SASSC_BIN): libsass.a
	cd $(SASS_SASSC_PATH) && $(MAKE)

test: $(SASSC_BIN) libsass.a
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s $(LOG_FLAGS) $(SASS_SPEC_PATH)/$(SASS_SPEC_SPEC_DIR)

test_build: $(SASSC_BIN) libsass.a
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH)/$(SASS_SPEC_SPEC_DIR)

test_issues: $(SASSC_BIN) libsass.a
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) $(LOG_FLAGS) $(SASS_SPEC_PATH)/spec/issues

clean:
	rm -f $(OBJECTS) *.a *.so


.PHONY: all debug debug-shared static shared bin install install-shared clean
