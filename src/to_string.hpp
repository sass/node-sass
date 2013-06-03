#include <string>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
	using namespace std;

	class To_String : public Operation<string> {
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
		virtual string operator()(String_Constant*);
		virtual string operator()(Media_Query_Expression*);
		virtual string operator()(Argument*);
		virtual string operator()(Arguments*);
		virtual string operator()(Selector_Schema*);
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