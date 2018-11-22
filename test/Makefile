CXX ?= c++
CXXFLAGS := -I ../include/ -std=c++11 -fsanitize=address -g -O1 -fno-omit-frame-pointer

test: test_shared_ptr

test_shared_ptr: build/test_shared_ptr
	@ASAN_OPTIONS="symbolize=1" build/test_shared_ptr

build:
	@mkdir build

build/test_shared_ptr: | build
	@$(CXX) $(CXXFLAGS) -o build/test_shared_ptr test_shared_ptr.cpp ../src/memory/SharedPtr.cpp

clean: | build
	rm -rf build

.PHONY: test test_shared_ptr clean
