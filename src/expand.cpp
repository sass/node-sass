#include "expand.hpp"
#include "ast.hpp"

#ifndef SASS_TO_STRING
#include "to_string.hpp"
#endif
// #include "apply.hpp"

namespace Sass {

	Expand::Expand(Mem& mem, /*To_String& ts,*/ Env& env)
	: mem(mem), /*to_string(ts),*/ global_env(env), env_stack(vector<Env>()), block_stack(vector<Block*>()), selector_stack(vector<Selector*>()) { }

	Statement* Expand::operator()(Block* b)
	{
		block_stack.push_back(b);
		Block* bb = new (mem) Block(b->path(), b->line(), b->length());
		for (size_t i = 0, L = b->length(); i < L; ++i) {
			(*bb) << (*b)[i]->perform(this);
		}
		block_stack.pop_back();
		return bb;
	}

}