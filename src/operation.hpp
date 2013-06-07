#define SASS_OPERATION

#include "ast_fwd_decl.hpp"

#include <iostream>
using namespace std;
#include <typeinfo>

namespace Sass {

	template<typename T>
	class Operation {
	public:
		virtual T operator()(AST_Node* x)                 { return fallback(x); }
		virtual ~Operation();
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

		// virtual T fallback(AST_Node* x)
		// { cerr << "null fallback!" << endl; /* TODO: throw an error */ return T(); }

		template <typename U>
		T fallback(U x)
		{ cerr << "fallback for " << typeid(*x).name() << endl; return T(); }
	};
	template<typename T>
	inline Operation<T>::~Operation() { }


	template <typename T, typename D>
	class Operation_CRTP : public Operation<T> {
	public:
		// virtual T operator()(AST_Node* x)                 { return static_cast<D*>(this)->fallback(x); }
		virtual ~Operation_CRTP();
		// statements
		virtual T operator()(Block* x)                    { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Ruleset* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Propset* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Media_Block* x)              { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(At_Rule* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Declaration* x)              { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Assignment* x)               { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Import* x)                   { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Import_Stub* x)              { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Warning* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Comment* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(If* x)                       { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(For* x)                      { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Each* x)                     { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(While* x)                    { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Return* x)                   { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Content* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Extend* x)                   { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Definition* x)               { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Mixin_Call* x)               { return static_cast<D*>(this)->fallback(x); }
		// expressions
		virtual T operator()(List* x)                     { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Binary_Expression* x)        { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Unary_Expression* x)         { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Function_Call* x)            { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Function_Call_Schema* x)     { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Variable* x)                 { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Textual* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Number* x)                   { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Percentage* x)               { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Dimension* x)                { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Color* x)                    { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Boolean* x)                  { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(String_Schema* x)            { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(String_Constant* x)          { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Media_Query* x)              { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Media_Query_Expression* x)   { return static_cast<D*>(this)->fallback(x); }
		// parameters and arguments
		virtual T operator()(Parameter* x)                { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Parameters* x)               { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Argument* x)                 { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Arguments* x)                { return static_cast<D*>(this)->fallback(x); }
		// selectors
		virtual T operator()(Selector_Schema* x)          { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Selector_Reference* x)       { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Selector_Placeholder* x)     { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Type_Selector* x)            { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Selector_Qualifier* x)       { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Attribute_Selector* x)       { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Pseudo_Selector* x)          { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Negated_Selector* x)         { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Simple_Selector_Sequence* x) { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Selector_Combination* x)     { return static_cast<D*>(this)->fallback(x); }
		virtual T operator()(Selector_Group* x)           { return static_cast<D*>(this)->fallback(x); }

		template <typename U>
		T fallback(U x)
		{ cerr << typeid(*this).name() << "::fallback(" << typeid(*x).name() << ")" << endl; return T(); }
	};
	template<typename T, typename D>
	inline Operation_CRTP<T, D>::~Operation_CRTP() { }

}