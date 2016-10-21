#ifndef SASS_EVAL_H
#define SASS_EVAL_H

#include "ast.hpp"
#include "context.hpp"
#include "listize.hpp"
#include "operation.hpp"
#include "environment.hpp"

namespace Sass {

  class Expand;
  class Context;

  class Eval : public Operation_CRTP<Expression*, Eval> {

   private:
    Expression* fallback_impl(AST_Node* n);

   public:
    Expand&  exp;
    Context& ctx;
    Eval(Expand& exp);
    ~Eval();

    bool force;
    bool is_in_comment;

    Env* environment();
    Context& context();
    CommaSequence_Selector* selector();
    Backtrace* backtrace();

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
    Expression* operator()(Function_Call_Schema*);
    Expression* operator()(Variable*);
    Expression* operator()(Textual*);
    Expression* operator()(Number*);
    Expression* operator()(Color*);
    Expression* operator()(Boolean*);
    Expression* operator()(String_Schema*);
    Expression* operator()(String_Quoted*);
    Expression* operator()(String_Constant*);
    // Expression* operator()(CommaSequence_Selector*);
    Expression* operator()(Media_Query*);
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
    CommaSequence_Selector* operator()(CommaSequence_Selector*);
    CommaSequence_Selector* operator()(Sequence_Selector*);
    Attribute_Selector* operator()(Attribute_Selector*);
    // they don't have any specific implementatio (yet)
    Element_Selector* operator()(Element_Selector* s) { return s; };
    Pseudo_Selector* operator()(Pseudo_Selector* s) { return s; };
    Wrapped_Selector* operator()(Wrapped_Selector* s) { return s; };
    Class_Selector* operator()(Class_Selector* s) { return s; };
    Id_Selector* operator()(Id_Selector* s) { return s; };
    Placeholder_Selector* operator()(Placeholder_Selector* s) { return s; };
    // actual evaluated selectors
    CommaSequence_Selector* operator()(Selector_Schema*);
    Expression* operator()(Parent_Selector*);

    template <typename U>
    Expression* fallback(U x) { return fallback_impl(x); }

    // -- only need to define two comparisons, and the rest can be implemented in terms of them
    static bool eq(Expression*, Expression*);
    static bool lt(Expression*, Expression*, std::string op);
    // -- arithmetic on the combinations that matter
    static Value* op_numbers(Memory_Manager&, enum Sass_OP, const Number&, const Number&, struct Sass_Inspect_Options opt, ParserState* pstate = 0);
    static Value* op_number_color(Memory_Manager&, enum Sass_OP, const Number&, const Color&, struct Sass_Inspect_Options opt, ParserState* pstate = 0);
    static Value* op_color_number(Memory_Manager&, enum Sass_OP, const Color&, const Number&, struct Sass_Inspect_Options opt, ParserState* pstate = 0);
    static Value* op_colors(Memory_Manager&, enum Sass_OP, const Color&, const Color&, struct Sass_Inspect_Options opt, ParserState* pstate = 0);
    static Value* op_strings(Memory_Manager&, Sass::Operand, Value&, Value&, struct Sass_Inspect_Options opt, ParserState* pstate = 0, bool interpolant = false);

  private:
    void interpolation(Context& ctx, std::string& res, Expression* ex, bool into_quotes, bool was_itpl = false);

  };

  Expression* cval_to_astnode(Memory_Manager& mem, union Sass_Value* v, Context& ctx, Backtrace* backtrace, ParserState pstate = ParserState("[AST]"));

}

#endif
