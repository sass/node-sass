

build: sassc.cpp document.cpp node.cpp token.cpp prelexer.cpp
	g++ -o bin/sassc sassc.cpp document.cpp document_parser.cpp document_eval_pending.cpp evaluator.cpp document_emitter.cpp node.cpp token.cpp prelexer.cpp

test: build
	ruby spec.rb spec/basic/

test_all: build
	ruby spec.rb spec/

clean:
	rm -rf *.o
	rm -rf bin/*