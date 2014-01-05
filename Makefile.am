ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = support/libsass.pc

lib_LTLIBRARIES = libsass.la
libsass_la_SOURCES = \
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

libsass_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = src/sass_interface.h src/sass.h

check_PROGRAMS = sassc++

sassc___SOURCES = sassc++.cpp
sassc___LDADD = libsass.la

if ENABLE_TESTS

noinst_PROGRAMS = sass-tester

sass_tester_SOURCES = $(SASS_SASSC_PATH)/sassc.c
sass_tester_LDADD = libsass.la
sass_tester_LDFLAGS = -no-install

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

$(SASSC_BIN): libsass.la
	cd $(SASS_SASSC_PATH) && make

test: $(SASSC_BIN) libsass.la
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_build: $(SASSC_BIN) libsass.la
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) -s --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH)

test_issues: $(SASSC_BIN) libsass.la
	ruby $(SASS_SPEC_PATH)/sass-spec.rb -c $(SASSC_BIN) $(LOG_FLAGS) $(SASS_SPEC_PATH)/spec/issues

endif
