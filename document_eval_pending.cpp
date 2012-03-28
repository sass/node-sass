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
          Node rhs(n[1]);
          if (rhs.type == Node::comma_list || rhs.type == Node::space_list) {
            for (int i = 0; i < rhs.size(); ++i) {
              if (rhs[i].eval_me) rhs[i] = eval(rhs[i], context.global_env);
            }
          }
          else {
            n[1] = eval(n[1], context.global_env);
          }
        } break;
        
        case Node::mixin: {
          context.global_env[n[0].token] = n;
        } break;
        
        case Node::expansion: {
          Token name(n[0].token);
          Node args(n[1]);
          Node mixin(context.global_env[name]);
          Node params(mixin[1]);
          Node body(mixin[2].clone());

          n.children->pop_back();
          n.children->pop_back();

          Environment m_env;
          for (int i = 0, j = 0; i < args.size(); ++i) {
            if (args[i].type == Node::assignment) {
              Node arg(args[i]);
              Token key(arg[0].token);
              if (!m_env.query(key)) {
                m_env[key] = eval(arg[1], context.global_env);
              }
            }
            else {
              // TO DO: ensure (j < params.size())
              m_env[params[j].token] = eval(args[i], context.global_env);
              ++j;
            }
          }
          m_env.link(context.global_env);
          
          for (int i = 0; i < body.size(); ++i) {
            body[i] = eval(body[i], m_env);
          }
          
          n += body;
        } break;
      }
    }
  }
}