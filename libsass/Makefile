
CPP_FILES = \
	context.cpp \
	functions.cpp \
	document.cpp \
	document_parser.cpp \
	eval_apply.cpp \
	node.cpp \
	node_comparisons.cpp \
	values.cpp \
	prelexer.cpp

libsass: libsass_objs
	ar rvs libsass.a \
			sass_interface.o \
			context.o \
			functions.o \
			document.o \
			document_parser.o \
			eval_apply.o \
			node.o \
			node_comparisons.o \
			values.o \
			prelexer.o

libsass_objs: sass_interface.cpp $(CPP_FILES)
	g++ -O2 -c -combine sass_interface.cpp $(CPP_FILES)

clean:
	rm -rf *.o *.a