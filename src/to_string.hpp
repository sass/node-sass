#define SASS_TO_STRING

#include <string>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
  using namespace std;

  class To_String : public Operation_CRTP<string, To_String> {
    // import all the class-specific methods and override as desired
    using Operation<string>::operator();
    // override this to define a catch-all
    string fallback_impl(AST_Node* n);

  public:
    virtual ~To_String() { }

    virtual string operator()(List*);
    virtual string operator()(Function_Call*);
    virtual string operator()(Function_Call_Schema*);
    virtual string operator()(Textual*);
    virtual string operator()(Number*);
    virtual string operator()(Percentage*);
    virtual string operator()(Dimension*);
    virtual string operator()(Color*);
    virtual string operator()(Boolean*);
    virtual string operator()(String_Schema*);
    virtual string operator()(String_Constant*);
    virtual string operator()(Media_Query*);
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

    template <typename U>
    string fallback(U n) { return fallback_impl(n); }
    //   Inspector i;
    //   n->perform(&i);
    //   return i.get_buffer();
    // }
  };
}