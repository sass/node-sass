#include <string>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
	using namespace std;

	class To_String : public Operation<string> {
		// import all the class-specific methods and override as desired
		using Operation<string>::operator();
		// override this to define a catch-all
		virtual string fallback(AST_Node* n);

	public:
		virtual ~To_String() { }

		virtual string operator()(List*);
		virtual string operator()(Function_Call*);
		virtual string operator()(Textual*);
		virtual string operator()(Number*);
		virtual string operator()(Percentage*);
		virtual string operator()(Dimension*);
		virtual string operator()(Color*);
		virtual string operator()(Boolean*);
		virtual string operator()(String_Schema*);
		virtual string operator()(String_Constant*);
		virtual string operator()(Argument*);
		virtual string operator()(Arguments*);
		virtual string operator()(Selector_Reference*);
		virtual string operator()(Selector_Placeholder*);
		virtual string operator()(Type_Selector*);
		virtual string operator()(Selector_Qualifier*);
		virtual string operator()(Attribute_Selector*);
		virtual string operator()(Pseudo_Selector*);
		virtual string operator()(Negated_Selector*);
		virtual string operator()(Simple_Selector_Sequence*);
		virtual string operator()(Selector_Combination*);
		virtual string operator()(Selector_Group*);
	};
}