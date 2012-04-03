

build: sassc.cpp document.cpp node.cpp values.cpp prelexer.cpp
	g++ -o bin/sassc sassc.cpp document.cpp document_parser.cpp eval_apply.cpp node.cpp values.cpp prelexer.cpp

test: build
	ruby spec.rb spec/basic/

test_all: build
	ruby spec.rb spec/

clean:
	rm -rf *.o
	rm -rf bin/*