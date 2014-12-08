ACLOCAL_AMFLAGS = -I m4

AM_LDFLAGS = -lstdc++ -lm
AM_CFLAGS = -Wall -fPIC
AM_CXXFLAGS = -Wall -fPIC
AM_CFLAGS += -DLIBSASS_VERSION="\"$(LIBSASS_VERSION)\""
AM_CXXFLAGS += -DLIBSASS_VERSION="\"$(LIBSASS_VERSION)\""

AM_CXXFLAGS += -std=c++0x

if ENABLE_COVERAGE
  AM_CFLAGS += -O0 --coverage
  AM_CXXFLAGS += -O0 --coverage
  AM_LDFLAGS += -O0 --coverage -lgcov
else
  AM_CFLAGS += -O2
  AM_CXXFLAGS += -O2
  AM_LDFLAGS += -O2
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = support/libsass.pc

lib_LTLIBRARIES = libsass.la

libsass_la_SOURCES = \
	cencode.c \
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
	json.cpp \
	output_compressed.cpp \
	output_nested.cpp \
	parser.cpp \
	prelexer.cpp \
	remove_placeholders.cpp \
	sass.cpp \
	sass_util.cpp \
	sass_values.cpp \
	sass_context.cpp \
	sass_functions.cpp \
	sass_interface.cpp \
	sass2scss.cpp \
	source_map.cpp \
	to_c.cpp \
	to_string.cpp \
	units.cpp \
	utf8_string.cpp \
	util.cpp

libsass_la_CFLAGS = $(AM_CFLAGS)
libsass_la_CXXFLAGS = $(AM_CXXFLAGS)
libsass_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined -version-info 0:9:0

include_HEADERS = sass2scss.h sass_context.h sass_functions.h sass_values.h sass.h

if ENABLE_TESTS

noinst_PROGRAMS = sassc_bin

sassc_bin_SOURCES = $(SASS_SASSC_PATH)/sassc.c
sassc_bin_LDADD = libsass.la
sassc_bin_CFLAGS = $(AM_CFLAGS)
sassc_bin_CXXFLAGS = $(AM_CXXFLAGS)
sassc_bin_LDFLAGS = $(AM_LDFLAGS) -no-install

if ENABLE_COVERAGE
nodist_EXTRA_sassc_bin_SOURCES = non-existent-file-to-force-CXX-linking.cxx
nodist_EXTRA_libsass_la_SOURCES = non-existent-file-to-force-CXX-linking.cxx
endif

TESTS = \
	$(SASS_SPEC_PATH)/spec/basic \
	$(SASS_SPEC_PATH)/spec/benchmarks \
	$(SASS_SPEC_PATH)/spec/bourbon \
	$(SASS_SPEC_PATH)/spec/libsass \
	$(SASS_SPEC_PATH)/spec/scss \
	$(SASS_SPEC_PATH)/spec/todo

LOG_COMPILER = $(RUBY) $(SASS_SPEC_PATH)/sass-spec.rb
AM_LOG_FLAGS = -c ./sass-tester

SASS_SASSC_PATH ?= sassc
SASS_SPEC_PATH ?= sass-spec
SASSC_BIN = $(SASS_SASSC_PATH)/bin/sassc
RUBY_BIN = ruby

$(SASSC_BIN): libsass.la
	cd $(SASS_SASSC_PATH) && $(MAKE)

test: $(SASSC_BIN)
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_build: $(SASSC_BIN)
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_issues: $(SASSC_BIN)
	$(RUBY_BIN) $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) $(LOG_FLAGS) $(SASS_SPEC_PATH)/spec/issues

endif
