#include "document.hpp"
#include "eval_apply.hpp"

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
              if (val[i].eval_me) val[i] = eval(val[i], context.global_env);
            }
          }
          else {
            val = eval(val, context.global_env);
          }
          context.global_env[n[0].token] = val;
        } break;

        case Node::rule: {
          // treat top-level lists differently from nested ones
          // Node rhs(n[1]);
          // if (rhs.type == Node::comma_list || rhs.type == Node::space_list) {
          //   for (int i = 0; i < rhs.size(); ++i) {
          //     if (rhs[i].eval_me) rhs[i] = eval(rhs[i], context.global_env);
          //   }
          // }
          // else {
          //   n[1] = eval(n[1], context.global_env);
          // }
          
          eval(n, context.global_env);
        } break;
        
        case Node::mixin: {
          context.global_env[n[0].token] = n;
        } break;
        
        case Node::expansion: {
          Token name(n[0].token);
          Node args(n[1]);
          Node mixin(context.global_env[name]);
          n.children->pop_back();
          n.children->pop_back();
          n += apply(mixin, args, context.global_env);
        } break;
      }
    }
  }
}