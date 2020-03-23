#ifndef SASS_EXPAND_H
#define SASS_EXPAND_H

#include <vector>

#include "ast.hpp"
#include "eval.hpp"
#include "operation.hpp"
#include "environment.hpp"

namespace Sass {

  class Listize;
  class Context;
  class Eval;
  struct Backtrace;

  class Expand : public Operation_CRTP<Statement*, Expand> {
  public:

    Env* environment();
    SelectorListObj& selector();
    SelectorListObj& original();
    SelectorListObj popFromSelectorStack();
    SelectorStack getOriginalStack();
    SelectorStack getSelectorStack();
    void pushNullSelector();
    void popNullSelector();
    void pushToSelectorStack(SelectorListObj selector);

    SelectorListObj popFromOriginalStack();

    void pushToOriginalStack(SelectorListObj selector);

    Context&          ctx;
    Backtraces&       traces;
    Eval              eval;
    size_t            recursions;
    bool              in_keyframes;
    bool              at_root_without_rule;
    bool              old_at_root_without_rule;

    // it's easier to work with vectors
    EnvStack      env_stack;
    BlockStack    block_stack;
    CallStack     call_stack;
  private:
    SelectorStack selector_stack;
  public:
    SelectorStack originalStack;
    MediaStack    mediaStack;

    Boolean_Obj bool_true;

  private:

    std::vector<CssMediaQuery_Obj> mergeMediaQueries(const std::vector<CssMediaQuery_Obj>& lhs, const std::vector<CssMediaQuery_Obj>& rhs);

  public:
    Expand(Context&, Env*, SelectorStack* stack = nullptr, SelectorStack* original = nullptr);
    ~Expand() { }

    Block* operator()(Block*);
    Statement* operator()(Ruleset*);

    Statement* operator()(MediaRule*);

    // Css Ruleset is already static
    // Statement* operator()(CssMediaRule*);

    Statement* operator()(Supports_Block*);
    Statement* operator()(At_Root_Block*);
    Statement* operator()(Directive*);
    Statement* operator()(Declaration*);
    Statement* operator()(Assignment*);
    Statement* operator()(Import*);
    Statement* operator()(Import_Stub*);
    Statement* operator()(Warning*);
    Statement* operator()(Error*);
    Statement* operator()(Debug*);
    Statement* operator()(Comment*);
    Statement* operator()(If*);
    Statement* operator()(For*);
    Statement* operator()(Each*);
    Statement* operator()(While*);
    Statement* operator()(Return*);
    Statement* operator()(ExtendRule*);
    Statement* operator()(Definition*);
    Statement* operator()(Mixin_Call*);
    Statement* operator()(Content*);

    void append_block(Block*);

  };

}

#endif
