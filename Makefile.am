ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libsass.la
libsass_la_SOURCES = ast.cpp bind.cpp constants.cpp context.cpp contextualize.cpp \
  copy_c_str.cpp error_handling.cpp eval.cpp expand.cpp extend.cpp file.cpp \
  functions.cpp inspect.cpp output_compressed.cpp output_nested.cpp \
  parser.cpp prelexer.cpp sass.cpp sass_interface.cpp to_c.cpp to_string.cpp \
  units.cpp
libsass_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = sass_interface.h sass.h

check_PROGRAMS = sassc++

sassc___SOURCES = sassc++.cpp
sassc___LDADD = libsass.la

if ENABLE_TESTS

noinst_PROGRAMS = sass-tester

sass_tester_SOURCES = $(SASS_SASSC_PATH)/sassc.c
sass_tester_LDADD = libsass.la
sass_tester_LDFLAGS = -no-install

test: sass-tester
	$(RUBY) $(SASS_SPEC_PATH)/sass-spec.rb -d $(SASS_SPEC_PATH) -c ./sass-tester $(TEST_FLAGS)

endif

