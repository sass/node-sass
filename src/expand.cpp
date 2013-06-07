#include "expand.hpp"
#include "ast.hpp"

#ifndef SASS_TO_STRING
#include "to_string.hpp"
#endif
// #include "apply.hpp"
#include <iostream>
#include <typeinfo>

namespace Sass {

	Expand::Expand(Mem& mem, /*To_String& ts,*/ Env& env)
	: mem(mem), /*to_string(ts),*/ global_env(env), env_stack(vector<Env>()), block_stack(vector<Block*>()), property_stack(vector<String*>()), selector_stack(vector<Selector*>()) { }

	Statement* Expand::operator()(Block* b)
	{
		Block* bb = new (mem) Block(b->path(), b->line(), b->length(), b->is_root());
		block_stack.push_back(bb);
		for (size_t i = 0, L = b->length(); i < L; ++i) {
			(*bb) << (*b)[i]->perform(this);
		}
		block_stack.pop_back();
		return bb;
	}

	Statement* Expand::operator()(Ruleset* r)
	{
		return new (mem) Ruleset(r->path(),
		                         r->line(),
		                         r->selector(), // TODO: expand the selector
		                         r->block()->perform(this)->block());
	}

	Statement* Expand::operator()(Propset* p)
	{
		Block* current_block = block_stack.back();
		String_Schema* combined_prop = new (mem) String_Schema(p->path(), p->line());
		if (!property_stack.empty()) {
			*combined_prop << property_stack.back()
		                 << new (mem) String_Constant(p->path(), p->line(), "-")
		                 << p->property_fragment(); // TODO: eval the prop into a string constant
		}
		else {
			*combined_prop << p->property_fragment();
		}
		for (size_t i = 0, S = p->declarations().size(); i < S; ++i) {
			Declaration* dec = static_cast<Declaration*>(p->declarations()[i]->perform(this));
			String_Schema* final_prop = new (mem) String_Schema(dec->path(), dec->line());
			*final_prop += combined_prop;
			*final_prop << new (mem) String_Constant(dec->path(), dec->line(), "-");
			*final_prop << dec->property();
			dec->property(final_prop);
			*current_block << dec;
		}
		for (size_t i = 0, S = p->propsets().size(); i < S; ++i) {
			property_stack.push_back(combined_prop);
			p->propsets()[i]->perform(this);
			property_stack.pop_back();
		}
		return p;
	}

	Statement* Expand::operator()(At_Rule* a)
	{
		Block* ab = a->block();
		Block* bb = ab ? ab->perform(this)->block() : 0;
		return new (mem) At_Rule(a->path(),
		                         a->line(),
		                         a->keyword(),
		                         a->selector(),
		                         bb);
	}

	Statement* Expand::operator()(Declaration* d)
	{
		// TODO: eval the property and value
		return new (mem) Declaration(d->path(),
		                             d->line(),
		                             d->property(),
		                             d->value());
	}

	Statement* Expand::operator()(Definition* d)
	{
		return d;
	}

	inline Statement* Expand::fallback_impl(AST_Node* n)
	{
		String_Constant* msg = new (mem) String_Constant("", 0, string("`Expand` doesn't handle ") + typeid(*n).name());
		return new (mem) Warning("", 0, msg);
		// TODO: fallback should call `eval` on Expression nodes
	}

}