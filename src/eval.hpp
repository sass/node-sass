#ifndef SASS_EVAL_H
#define SASS_EVAL_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"
#include "ast.hpp"

#include "context.hpp"
#include "listize.hpp"
#include "operation.hpp"
#include "environment.hpp"

namespace Sass {

  class Expand;
  class Context;

  class Eval : public Operation_CRTP<Expression*, Eval> {

   public:
    Expand& exp;
    Context& ctx;
    Backtraces& traces;
    Eval(Expand& exp);
    ~Eval();

    bool force;
    bool is_in_comment;
    bool is_in_selector_schema;

    Boolean_Obj bool_true;
    Boolean_Obj bool_false;

    Env* environment();
    EnvStack& env_stack();
    const std::string cwd();
    CalleeStack& callee_stack();
    struct Sass_Inspect_Options& options();
    struct Sass_Compiler* compiler();

    // for evaluating function bodies
    Expression* operator()(Block*);
    Expression* operator()(Assignment*);
    Expression* operator()(If*);
    Expression* operator()(For*);
    Expression* operator()(Each*);
    Expression* operator()(While*);
    Expression* operator()(Return*);
    Expression* operator()(Warning*);
    Expression* operator()(Error*);
    Expression* operator()(Debug*);

    Expression* operator()(List*);
    Expression* operator()(Map*);
    Expression* operator()(Binary_Expression*);
    Expression* operator()(Unary_Expression*);
    Expression* operator()(Function_Call*);
    Expression* operator()(Variable*);
    Expression* operator()(Number*);
    Expression* operator()(Color_RGBA*);
    Expression* operator()(Color_HSLA*);
    Expression* operator()(Boolean*);
    Expression* operator()(String_Schema*);
    Expression* operator()(String_Quoted*);
    Expression* operator()(String_Constant*);
    Media_Query* operator()(Media_Query*);
    Expression* operator()(Media_Query_Expression*);
    Expression* operator()(At_Root_Query*);
    Expression* operator()(Supports_Operator*);
    Expression* operator()(Supports_Negation*);
    Expression* operator()(Supports_Declaration*);
    Expression* operator()(Supports_Interpolation*);
    Expression* operator()(Null*);
    Expression* operator()(Argument*);
    Expression* operator()(Arguments*);
    Expression* operator()(Comment*);

    // these will return selectors
    SelectorList* operator()(SelectorList*);
    SelectorList* operator()(ComplexSelector*);
    CompoundSelector* operator()(CompoundSelector*);
    SelectorComponent* operator()(SelectorComponent*);
    SimpleSelector* operator()(SimpleSelector* s);
    Pseudo_Selector* operator()(Pseudo_Selector* s);

    // they don't have any specific implementation (yet)
    Id_Selector* operator()(Id_Selector* s) { return s; };
    Class_Selector* operator()(Class_Selector* s) { return s; };
    Type_Selector* operator()(Type_Selector* s) { return s; };
    Attribute_Selector* operator()(Attribute_Selector* s) { return s; };
    Placeholder_Selector* operator()(Placeholder_Selector* s) { return s; };

    // actual evaluated selectors
    SelectorList* operator()(Selector_Schema*);
    Expression* operator()(Parent_Reference*);

    // generic fallback
    template <typename U>
    Expression* fallback(U x)
    { return Cast<Expression>(x); }

  private:
    void interpolation(Context& ctx, std::string& res, Expression_Obj ex, bool into_quotes, bool was_itpl = false);

  };

}

#endif
