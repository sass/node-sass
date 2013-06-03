#define SASS_OPERATION

#include "ast_fwd_decl.hpp"

namespace Sass {

	template<typename T>
	class Operation {
	public:
		virtual T operator()(AST_Node*);
		virtual ~Operation() = 0;
		// statements
		virtual T operator()(Block*);
		virtual T operator()(Ruleset*);
		virtual T operator()(Propset*);
		virtual T operator()(Media_Block*);
		virtual T operator()(At_Rule*);
		virtual T operator()(Declaration*);
		virtual T operator()(Assignment*);
		virtual T operator()(Import*);
		virtual T operator()(Import_Stub*);
		virtual T operator()(Warning*);
		virtual T operator()(Comment*);
		virtual T operator()(If*);
		virtual T operator()(For*);
		virtual T operator()(Each*);
		virtual T operator()(While*);
		virtual T operator()(Return*);
		virtual T operator()(Content*);
		virtual T operator()(Extend*);
		virtual T operator()(Definition*);
		virtual T operator()(Mixin_Call*);
		// expressions
		virtual T operator()(List*);
		virtual T operator()(Binary_Expression*);
		virtual T operator()(Unary_Expression*);
		virtual T operator()(Function_Call*);
		virtual T operator()(Variable*);
		virtual T operator()(Textual*);
		virtual T operator()(Number*);
		virtual T operator()(Percentage*);
		virtual T operator()(Dimension*);
		virtual T operator()(Color*);
		virtual T operator()(Boolean*);
		virtual T operator()(String_Schema*);
		virtual T operator()(String_Constant*);
		virtual T operator()(Media_Query_Expression*);
		// parameters and arguments
		virtual T operator()(Parameter*);
		virtual T operator()(Parameters*);
		virtual T operator()(Argument*);
		virtual T operator()(Arguments*);
		// selectors
		virtual T operator()(Selector_Schema*);
		virtual T operator()(Selector_Reference*);
		virtual T operator()(Selector_Placeholder*);
		virtual T operator()(Type_Selector*);
		virtual T operator()(Selector_Qualifier*);
		virtual T operator()(Attribute_Selector*);
		virtual T operator()(Pseudo_Selector*);
		virtual T operator()(Negated_Selector*);
		virtual T operator()(Simple_Selector_Sequence*);
		virtual T operator()(Selector_Combination*);
		virtual T operator()(Selector_Group*);
	};
	template<typename T>
	inline Operation<T>::~Operation() { }

}