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
          // bind arguments
          for (int i = 0, j = 0; i < args.size(); ++i) {
            if (args[i].type == Node::assignment) {
              Node arg(args[i]);
              Token name(arg[0].token);
              if (!m_env.query(name)) {
                m_env[name] = eval(arg[1], context.global_env);
              }
            }
            else {
              // TO DO: ensure (j < params.size())
              Node param(params[j]);
              Token name(param.type == Node::variable ? param.token : param[0].token);
              m_env[name] = eval(args[i], context.global_env);
              ++j;
            }
          }
          // plug the holes with default arguments if any
          for (int i = 0; i < params.size(); ++i) {
            if (params[i].type == Node::assignment) {
              Node param(params[i]);
              Token name(param[0].token);
              if (!m_env.query(name)) {
                m_env[name] = eval(param[1], context.global_env);
              }
            }
          }
          m_env.link(context.global_env);
          
          for (int i = 0; i < body.size(); ++i) {
            body[i] = eval(body[i], m_env);
          }
          
          n += body;
          // ideally say: n += apply(mixin, args, context.global_env);
        } break;
      }
    }
  }
}