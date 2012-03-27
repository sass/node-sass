#include "document.hpp"
#include "evaluator.hpp"

namespace Sass {
  
  void Document::eval_pending()
  {
    for (int i = 0; i < context.pending.size(); ++i) {
      Node n(context.pending[i]);
      switch (n.type)
      {
        case Node::assignment:
          context.environment[n[0].token] = eval(n[1], context.environment);
          break;
        case Node::rule:
          n[1] = eval(n[1], context.environment);
          break;
      }
    }
  }
}