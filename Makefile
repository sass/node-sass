SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build
CPP_FILES = \
	$(SRC_DIR)/context.cpp \
	$(SRC_DIR)/functions.cpp \
	$(SRC_DIR)/document.cpp \
	$(SRC_DIR)/document_parser.cpp \
	$(SRC_DIR)/eval_apply.cpp \
	$(SRC_DIR)/node.cpp \
	$(SRC_DIR)/node_comparisons.cpp \
	$(SRC_DIR)/values.cpp \
	$(SRC_DIR)/prelexer.cpp

sassc: sassc_obj libsass
	gcc -o $(BIN_DIR)/sassc $(BUILD_DIR)/sassc.o libsass.a -lstdc++

sassc_obj: build_dir $(SRC_DIR)/sassc.c
	mv *.o $(BUILD_DIR)

libsass: libsass_objs
	ar rvs libsass.a \
			$(BUILD_DIR)/sass_interface.o \
			$(BUILD_DIR)/context.o \
			$(BUILD_DIR)/functions.o \
			$(BUILD_DIR)/document.o \
			$(BUILD_DIR)/document_parser.o \
			$(BUILD_DIR)/eval_apply.o \
			$(BUILD_DIR)/node.o \
			$(BUILD_DIR)/node_comparisons.o \
			$(BUILD_DIR)/values.o \
			$(BUILD_DIR)/prelexer.o

libsass_objs: build_dir $(SRC_DIR)/sass_interface.cpp $(CPP_FILES)
	g++ -c -combine $(SRC_DIR)/sass_interface.cpp $(CPP_FILES)
	mv *.o $(BUILD_DIR)/

test: sassc
	ruby spec.rb spec/basic/

test_all: sassc
	ruby spec.rb spec/

build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf *.o build/*.o *.a
	rm -rf bin/*