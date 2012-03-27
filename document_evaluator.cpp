#include "document.hpp"
#include "evaluator.hpp"

namespace Sass {
  
  void Document::eval_pending()
  {
    for (int i = 0; i < context.pending.size(); ++i) {
      Node n(context.pending[i]);
      switch (n.type)
      {
        case Node::assignment: {
          Node val(n[1]);
          if (val.type == Node::comma_list || val.type == Node::space_list) {
            for (int i = 0; i < val.size(); ++i) {
              if (val[i].eval_me) val[i] = eval(val[i], context.environment);
            }
          }
          else {
            val = eval(val, context.environment);
          }
          context.environment[n[0].token] = val;
        } break;

        case Node::rule: {
          // treat top-level lists differently from nested ones
          Node rhs(n[1]);
          if (rhs.type == Node::comma_list || rhs.type == Node::space_list) {
            for (int i = 0; i < rhs.size(); ++i) {
              if (rhs[i].eval_me) rhs[i] = eval(rhs[i], context.environment);
            }
          }
          else {
            n[1] = eval(n[1], context.environment);
          }
        } break;
      }
    }
  }
}