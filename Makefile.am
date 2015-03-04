ACLOCAL_AMFLAGS = -I m4

AM_LDFLAGS = -lstdc++ -lm
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

if COMPILER_IS_MINGW32
  AM_CXXFLAGS += -std=gnu++0x
else
  AM_CFLAGS += -fPIC
  AM_CXXFLAGS += -fPIC
  AM_CXXFLAGS += -std=c++0x
  AM_LDFLAGS += -ldl
endif

AM_CFLAGS += -DLIBSASS_VERSION="\"$(VERSION)\""
AM_CXXFLAGS += -DLIBSASS_VERSION="\"$(VERSION)\""

if ENABLE_COVERAGE
	AM_CFLAGS += -O0 --coverage
	AM_CXXFLAGS += -O0 --coverage
	AM_LDFLAGS += -O0 --coverage -lgcov
else
	AM_CFLAGS += -O2
	AM_CXXFLAGS += -O2
	AM_LDFLAGS += -O2
endif

EXTRA_DIST = \
	COPYING \
	INSTALL \
	LICENSE \
	Readme.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = support/libsass.pc

lib_LTLIBRARIES = libsass.la

libsass_la_SOURCES = \
	ast_fwd_decl.hpp ast_def_macros.hpp \
	kwd_arg_macros.hpp memory_manager.hpp \
	position.cpp position.hpp \
	operation.hpp \
	subset_map.hpp mapping.hpp \
	color_names.hpp backtrace.hpp \
	cencode.c b64/cencode.h b64/encode.h \
	token.hpp environment.hpp \
	paths.hpp debug.hpp \
	utf8.h utf8/core.h \
	utf8/checked.h utf8/unchecked.h \
	ast.cpp ast.hpp \
	base64vlq.cpp base64vlq.hpp \
	bind.cpp bind.hpp \
	constants.cpp constants.hpp \
	context.cpp context.hpp \
	contextualize.cpp contextualize.hpp \
	copy_c_str.cpp copy_c_str.hpp \
	error_handling.cpp error_handling.hpp \
	eval.cpp eval.hpp \
	expand.cpp expand.hpp \
	extend.cpp extend.hpp \
	cssize.cpp cssize.hpp \
	file.cpp file.hpp \
	functions.cpp functions.hpp \
	inspect.cpp inspect.hpp \
	node.cpp node.hpp \
	json.cpp json.hpp \
	emitter.cpp emitter.hpp \
	output.cpp output.hpp \
	parser.cpp parser.hpp \
	plugins.cpp plugins.hpp \
	prelexer.cpp prelexer.hpp \
	remove_placeholders.cpp remove_placeholders.hpp \
	sass.cpp sass.h \
	sass_util.cpp sass_util.hpp \
	sass_values.cpp sass_values.h \
	sass_context.cpp sass_context.h \
	sass_functions.cpp sass_functions.h \
	sass_interface.cpp sass_interface.h \
	sass2scss.cpp sass2scss.h \
	source_map.cpp source_map.hpp \
	to_c.cpp to_c.hpp \
	to_string.cpp to_string.hpp \
	units.cpp units.hpp \
	utf8_string.cpp utf8_string.hpp \
	util.cpp util.hpp

libsass_la_CFLAGS = $(AM_CFLAGS)
libsass_la_CXXFLAGS = $(AM_CXXFLAGS)
libsass_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined -version-info 0:9:0

include_HEADERS = sass2scss.h sass_context.h sass_functions.h sass_values.h sass.h

if ENABLE_TESTS

noinst_PROGRAMS = tester

tester_LDADD = libsass.la
tester_SOURCES = $(SASS_SASSC_PATH)/sassc.c
SASSC_VERSION=`cd "$(SASS_SASSC_PATH)" && ./version.sh`
tester_CFLAGS = $(AM_CFLAGS) -DSASSC_VERSION="\"$(SASSC_VERSION)\""
tester_CXXFLAGS = $(AM_CXXFLAGS) -DSASSC_VERSION="\"$(SASSC_VERSION)\""
tester_LDFLAGS = $(AM_LDFLAGS) -no-install

if ENABLE_COVERAGE
nodist_EXTRA_tester_SOURCES = non-existent-file-to-force-CXX-linking.cxx
nodist_EXTRA_libsass_la_SOURCES = non-existent-file-to-force-CXX-linking.cxx
endif

SASS_SASSC_PATH ?= sassc
SASS_SPEC_PATH ?= sass-spec

TESTS = \
	$(SASS_SPEC_PATH)/spec/basic \
	$(SASS_SPEC_PATH)/spec/css \
	$(SASS_SPEC_PATH)/spec/extend-tests \
	$(SASS_SPEC_PATH)/spec/extends \
	$(SASS_SPEC_PATH)/spec/libsass \
	$(SASS_SPEC_PATH)/spec/libsass-closed-issues \
	$(SASS_SPEC_PATH)/spec/maps \
	$(SASS_SPEC_PATH)/spec/misc \
	$(SASS_SPEC_PATH)/spec/regressions \
	$(SASS_SPEC_PATH)/spec/scss \
	$(SASS_SPEC_PATH)/spec/scss-tests \
	$(SASS_SPEC_PATH)/spec/types

SASS_TEST_FLAGS =
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) ./script/tap-driver
AM_LOG_FLAGS = -c ./tester --ignore-todo $(LOG_FLAGS)
if USE_TAP
	AM_LOG_FLAGS += -t
	SASS_TEST_FLAGS += -t | tapout
	LOG_COMPILER = ./script/tap-runner $(RUBY) $(SASS_SPEC_PATH)/sass-spec.rb
else
	LOG_COMPILER = $(RUBY) $(SASS_SPEC_PATH)/sass-spec.rb
endif

SASS_TESTER = $(RUBY) $(SASS_SPEC_PATH)/sass-spec.rb

if COMPILER_IS_MINGW32
  SASS_TESTER += -c $(SASS_LIBSASS_PATH)/tester.exe
else
  SASS_TESTER += -c $(SASS_LIBSASS_PATH)/tester
endif

test:
	$(SASS_TESTER) --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH) $(SASS_TEST_FLAGS)

test_build:
	$(SASS_TESTER) --ignore-todo $(LOG_FLAGS) $(SASS_SPEC_PATH) $(SASS_TEST_FLAGS)

test_full:
	$(SASS_TESTER) $(LOG_FLAGS) $(SASS_SPEC_PATH) $(SASS_TEST_FLAGS)

test_issues:
	$(SASS_TESTER) $(LOG_FLAGS) $(SASS_SPEC_PATH)/spec/issues $(SASS_TEST_FLAGS)

endif
