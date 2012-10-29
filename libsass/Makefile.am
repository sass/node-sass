ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libsass.la
libsass_la_SOURCES = context.cpp functions.cpp document.cpp \
	constants.cpp document_parser.cpp eval_apply.cpp node.cpp \
	node_factory.cpp node_emitters.cpp prelexer.cpp \
	selector.cpp sass_interface.cpp
libsass_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = sass_interface.h
