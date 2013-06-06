#define SASS_OPERATION

#include "ast_fwd_decl.hpp"

namespace Sass {

	template<typename T>
	class Operation {
	public:
		virtual T operator()(AST_Node* x)                 { return fallback(x); }
		virtual ~Operation() = 0;
		// statements
		virtual T operator()(Block* x)                    { return fallback(x); }
		virtual T operator()(Ruleset* x)                  { return fallback(x); }
		virtual T operator()(Propset* x)                  { return fallback(x); }
		virtual T operator()(Media_Block* x)              { return fallback(x); }
		virtual T operator()(At_Rule* x)                  { return fallback(x); }
		virtual T operator()(Declaration* x)              { return fallback(x); }
		virtual T operator()(Assignment* x)               { return fallback(x); }
		virtual T operator()(Import* x)                   { return fallback(x); }
		virtual T operator()(Import_Stub* x)              { return fallback(x); }
		virtual T operator()(Warning* x)                  { return fallback(x); }
		virtual T operator()(Comment* x)                  { return fallback(x); }
		virtual T operator()(If* x)                       { return fallback(x); }
		virtual T operator()(For* x)                      { return fallback(x); }
		virtual T operator()(Each* x)                     { return fallback(x); }
		virtual T operator()(While* x)                    { return fallback(x); }
		virtual T operator()(Return* x)                   { return fallback(x); }
		virtual T operator()(Content* x)                  { return fallback(x); }
		virtual T operator()(Extend* x)                   { return fallback(x); }
		virtual T operator()(Definition* x)               { return fallback(x); }
		virtual T operator()(Mixin_Call* x)               { return fallback(x); }
		// expressions
		virtual T operator()(List* x)                     { return fallback(x); }
		virtual T operator()(Binary_Expression* x)        { return fallback(x); }
		virtual T operator()(Unary_Expression* x)         { return fallback(x); }
		virtual T operator()(Function_Call* x)            { return fallback(x); }
		virtual T operator()(Function_Call_Schema* x)     { return fallback(x); }
		virtual T operator()(Variable* x)                 { return fallback(x); }
		virtual T operator()(Textual* x)                  { return fallback(x); }
		virtual T operator()(Number* x)                   { return fallback(x); }
		virtual T operator()(Percentage* x)               { return fallback(x); }
		virtual T operator()(Dimension* x)                { return fallback(x); }
		virtual T operator()(Color* x)                    { return fallback(x); }
		virtual T operator()(Boolean* x)                  { return fallback(x); }
		virtual T operator()(String_Schema* x)            { return fallback(x); }
		virtual T operator()(String_Constant* x)          { return fallback(x); }
		virtual T operator()(Media_Query* x)              { return fallback(x); }
		virtual T operator()(Media_Query_Expression* x)   { return fallback(x); }
		// parameters and arguments
		virtual T operator()(Parameter* x)                { return fallback(x); }
		virtual T operator()(Parameters* x)               { return fallback(x); }
		virtual T operator()(Argument* x)                 { return fallback(x); }
		virtual T operator()(Arguments* x)                { return fallback(x); }
		// selectors
		virtual T operator()(Selector_Schema* x)          { return fallback(x); }
		virtual T operator()(Selector_Reference* x)       { return fallback(x); }
		virtual T operator()(Selector_Placeholder* x)     { return fallback(x); }
		virtual T operator()(Type_Selector* x)            { return fallback(x); }
		virtual T operator()(Selector_Qualifier* x)       { return fallback(x); }
		virtual T operator()(Attribute_Selector* x)       { return fallback(x); }
		virtual T operator()(Pseudo_Selector* x)          { return fallback(x); }
		virtual T operator()(Negated_Selector* x)         { return fallback(x); }
		virtual T operator()(Simple_Selector_Sequence* x) { return fallback(x); }
		virtual T operator()(Selector_Combination* x)     { return fallback(x); }
		virtual T operator()(Selector_Group* x)           { return fallback(x); }

		virtual T fallback(AST_Node* x)
		{ /* TODO: throw an error */ return T(); }
	};
	template<typename T>
	inline Operation<T>::~Operation() { }

}