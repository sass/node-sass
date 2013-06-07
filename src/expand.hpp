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
	class String;
	class Selector;

	typedef Environment<AST_Node*> Env;
	typedef Memory_Manager<AST_Node*> Mem;

	class Expand : public Operation_CRTP<Statement*, Expand> {

		Mem&              mem;
		// To_String&        to_string;
		// Apply&            apply;
		Env&              global_env;
		vector<Env>       env_stack;
		vector<Block*>    block_stack;
		vector<String*>   property_stack;
		vector<Selector*> selector_stack;

		Statement* fallback_impl(AST_Node* n);

	public:
		Expand(Mem&, /*To_String&,*/ Env&);
		virtual ~Expand() { }

		using Operation<Statement*>::operator();

		Statement* operator()(Block*);
		Statement* operator()(Ruleset*);
		Statement* operator()(Propset*);
		Statement* operator()(At_Rule*);
		Statement* operator()(Declaration*);
		Statement* operator()(Definition*);

		template <typename U>
		Statement* fallback(U x) { return fallback_impl(x); }
	};

}