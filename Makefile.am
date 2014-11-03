ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -fPIC
AM_CXXFLAGS = -Wall -fPIC

if ENABLE_COVERAGE
  AM_CFLAGS += -O0 --coverage
  AM_CXXFLAGS += -O0 --coverage
else
  AM_CFLAGS += -O2
  AM_CXXFLAGS += -O2
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
	sass_interface.cpp \
	sass_util.cpp \
	sass2scss.cpp \
	source_map.cpp \
	to_c.cpp \
	to_string.cpp \
	units.cpp \
	utf8_string.cpp \
	util.cpp

libsass_la_CXXFLAGS = $(AM_CXXFLAGS) -std=c++0x
libsass_la_LDFLAGS = -no-undefined -version-info 0:0:0

if ENABLE_COVERAGE
libsass_la_LDFLAGS += -lgcov
endif

include_HEADERS = sass2scss.h sass_interface.h sass.h

if ENABLE_TESTS

noinst_PROGRAMS = sass-tester

sass_tester_SOURCES = $(SASS_SASSC_PATH)/sassc.c
sass_tester_LDADD = libsass.la
sass_tester_LDFLAGS = -no-install
if ENABLE_COVERAGE
sass_tester_LDFLAGS += -lgcov
nodist_EXTRA_sass_tester_SOURCES = non-existent-file-to-force-CXX-linking.cxx
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
