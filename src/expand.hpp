#define SASS_EXPAND

#include <vector>
#include <iostream>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
#endif

#ifndef SASS_MEMORY_MANAGER
#include "memory_manager.hpp"
#endif

namespace Sass {
	using namespace std;

	class To_String;
	class Apply;
	class Statement;
	class Selector;

	typedef Environment<AST_Node*> Env;
	typedef Memory_Manager<AST_Node*> Mem;

	class Expand : public Operation<Statement*> {

		Mem&              mem;
		// To_String&        to_string;
		// Apply&            apply;
		Env&              global_env;
		vector<Env>       env_stack;
		vector<Block*>    block_stack;
		vector<Selector*> selector_stack;

	public:
		Expand(Mem&, /*To_String&,*/ Env&);
		virtual ~Expand() { }

		using Operation<Statement*>::operator();

		Statement* fallback(Statement* n) { cerr << "identity!" << endl; return n; } // TODO: implement this

		Statement* operator()(Block* b);
		// Statement* operator()(Definition* d) { return fallback(d); }
	};

}