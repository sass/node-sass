#define SASS_OPERATION

#include "ast_fwd_decl.hpp"

namespace Sass {

	template<typename T>
	class Operation {
	public:
		virtual T operator()(AST_Node*);
		virtual ~Operation() = 0;
		// statements
		virtual T operator()(Block*) = 0;
		virtual T operator()(Ruleset*) = 0;
		virtual T operator()(Propset*) = 0;
		virtual T operator()(Media_Block*) = 0;
		virtual T operator()(At_Rule*) = 0;
		virtual T operator()(Declaration*) = 0;
		virtual T operator()(Assignment*) = 0;
		virtual T operator()(Import*) = 0;
		virtual T operator()(Import_Stub*) = 0;
		virtual T operator()(Warning*) = 0;
		virtual T operator()(Comment*) = 0;
		virtual T operator()(If*) = 0;
		virtual T operator()(For*) = 0;
		virtual T operator()(Each*) = 0;
		virtual T operator()(While*) = 0;
		virtual T operator()(Return*) = 0;
		virtual T operator()(Content*) = 0;
		virtual T operator()(Extend*) = 0;
		virtual T operator()(Definition*) = 0;
		virtual T operator()(Mixin_Call*) = 0;
		// expressions
		virtual T operator()(List*) = 0;
		virtual T operator()(Binary_Expression*) = 0;
		virtual T operator()(Unary_Expression*) = 0;
		virtual T operator()(Function_Call*) = 0;
		virtual T operator()(Variable*) = 0;
		virtual T operator()(Textual*) = 0;
		virtual T operator()(Number*) = 0;
		virtual T operator()(Percentage*) = 0;
		virtual T operator()(Dimension*) = 0;
		virtual T operator()(Color*) = 0;
		virtual T operator()(Boolean*) = 0;
		virtual T operator()(String_Schema*) = 0;
		virtual T operator()(String_Constant*) = 0;
		virtual T operator()(Media_Query_Expression*) = 0;
		// parameters and arguments
		virtual T operator()(Parameter*) = 0;
		virtual T operator()(Parameters*) = 0;
		virtual T operator()(Argument*) = 0;
		virtual T operator()(Arguments*) = 0;
		// selectors
		virtual T operator()(Selector_Schema*) = 0;
		virtual T operator()(Selector_Reference*) = 0;
		virtual T operator()(Selector_Placeholder*) = 0;
		virtual T operator()(Type_Selector*) = 0;
		virtual T operator()(Selector_Qualifier*) = 0;
		virtual T operator()(Attribute_Selector*) = 0;
		virtual T operator()(Pseudo_Selector*) = 0;
		virtual T operator()(Negated_Selector*) = 0;
		virtual T operator()(Simple_Selector_Sequence*) = 0;
		virtual T operator()(Selector_Combination*) = 0;
		virtual T operator()(Selector_Group*) = 0;
	};
	template<typename T>
	inline Operation<T>::~Operation() { }

}