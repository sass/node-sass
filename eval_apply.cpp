#include "eval_apply.hpp"
#include <iostream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;

  Node eval(Node& expr, Environment& env, map<pair<string, size_t>, Function>& f_env)
  {
    switch (expr.type)
    {
      case Node::mixin: {
        env[expr[0].content.token] = expr;
        return expr;
      } break;
      
      case Node::expansion: {
        Token name(expr[0].content.token);
        Node args(expr[1]);
        Node mixin(env[name]);
        Node expansion(apply_mixin(mixin, args, env, f_env));
        expr.content.children->pop_back();
        expr.content.children->pop_back();
        expr += expansion;
        return expr;
      } break;
      
      case Node::ruleset: {
        eval(expr[1], env, f_env);
        return expr;
      } break;
      
      case Node::root: {
        for (int i = 0; i < expr.size(); ++i) {
          eval(expr[i], env, f_env);
        }
        return expr;
      } break;
      
      case Node::block: {
        Environment current;
        current.link(env);
        for (int i = 0; i < expr.size(); ++i) {
          eval(expr[i], current, f_env);
        }
        return expr;
      } break;
      
      case Node::assignment: {
        Node val(expr[1]);
        if (val.type == Node::comma_list || val.type == Node::space_list) {
          for (int i = 0; i < val.size(); ++i) {
            if (val[i].eval_me) val[i] = eval(val[i], env, f_env);
          }
        }
        else {
          val = eval(val, env, f_env);
        }
        Node var(expr[0]);
        if (env.query(var.content.token)) {
          env[var.content.token] = val;
        }
        else {
          env.current_frame[var.content.token] = val;
        }
        return expr;
      } break;

      case Node::rule: {
        Node rhs(expr[1]);
        if (rhs.type == Node::comma_list || rhs.type == Node::space_list) {
          for (int i = 0; i < rhs.size(); ++i) {
            if (rhs[i].eval_me) rhs[i] = eval(rhs[i], env, f_env);
          }
        }
        else {
          if (rhs.eval_me) expr[1] = eval(rhs, env, f_env);
        }
        return expr;
      } break;

      case Node::comma_list:
      case Node::space_list: {
        if (expr.eval_me) expr[0] = eval(expr[0], env, f_env);
        return expr;
      } break;

      case Node::expression: {
        Node acc(Node::expression, expr.line_number, 1);
        acc << eval(expr[0], env, f_env);
        Node rhs(eval(expr[2], env, f_env));
        accumulate(expr[1].type, acc, rhs);
        for (int i = 3; i < expr.size(); i += 2) {
          Node rhs(eval(expr[i+1], env, f_env));
          accumulate(expr[i].type, acc, rhs);
        }
        return acc.size() == 1 ? acc[0] : acc;
      } break;

      case Node::term: {
        if (expr.eval_me) {
          Node acc(Node::expression, expr.line_number, 1);
          acc << eval(expr[0], env, f_env);
          Node rhs(eval(expr[2], env, f_env));
          accumulate(expr[1].type, acc, rhs);
          for (int i = 3; i < expr.size(); i += 2) {
            Node rhs(eval(expr[i+1], env, f_env));
            accumulate(expr[i].type, acc, rhs);
          }
          return acc.size() == 1 ? acc[0] : acc;
        }
        else {
          return expr;
        }
      } break;

      case Node::textual_percentage:
      case Node::textual_dimension: {
        return Node(expr.line_number,
                    std::atof(expr.content.token.begin),
                    Token::make(Prelexer::number(expr.content.token.begin),
                                expr.content.token.end));
      } break;
      
      case Node::textual_number: {
        return Node(expr.line_number, std::atof(expr.content.token.begin));
      } break;

      case Node::textual_hex: {        
        Node triple(Node::numeric_color, expr.line_number, 4);
        Token hext(Token::make(expr.content.token.begin+1, expr.content.token.end));
        if (hext.length() == 6) {
          for (int i = 0; i < 6; i += 2) {
            triple << Node(expr.line_number, static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
          }
        }
        else {
          for (int i = 0; i < 3; ++i) {
            triple << Node(expr.line_number, static_cast<double>(std::strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
          }
        }
        triple << Node(expr.line_number, 1.0);
        return triple;
      } break;
      
      case Node::variable: {
        return env[expr.content.token];
      } break;
      
      case Node::function_call: {
        // TO DO: default-constructed Function should be a generic callback
        pair<string, size_t> sig(expr[0].content.token.to_string(), expr[1].size());
        return apply_function(f_env[sig], expr[1], env, f_env);
      } break;
      
      default: {
        return expr;
      }
    }
  }

  Node accumulate(Node::Type op, Node& acc, Node& rhs)
  {
    Node lhs(acc.content.children->back());
    double lnum = lhs.content.numeric_value;
    double rnum = rhs.content.numeric_value;
    
    if (lhs.type == Node::number && rhs.type == Node::number) {
      Node result(acc.line_number, operate(op, lnum, rnum));
      acc.content.children->pop_back();
      acc.content.children->push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type == Node::number && rhs.type == Node::numeric_dimension) {
      // TO DO: disallow division
      Node result(acc.line_number, operate(op, lnum, rnum), Token::make(rhs.content.dimension.unit, Prelexer::identifier(rhs.content.dimension.unit)));
      acc.content.children->pop_back();
      acc.content.children->push_back(result);
    }
    else if (lhs.type == Node::numeric_dimension && rhs.type == Node::number) {
      Node result(acc.line_number, operate(op, lnum, rnum), Token::make(lhs.content.dimension.unit, Prelexer::identifier(rhs.content.dimension.unit)));
      acc.content.children->pop_back();
      acc.content.children->push_back(result);
    }
    else if (lhs.type == Node::numeric_dimension && rhs.type == Node::numeric_dimension) {
      // TO DO: CHECK FOR MISMATCHED UNITS HERE
      Node result;
      if (op == Node::div)
      { result = Node(acc.line_number, operate(op, lnum, rnum)); }
      else
      { result = Node(acc.line_number, operate(op, lnum, rnum), Token::make(lhs.content.dimension.unit, Prelexer::identifier(rhs.content.dimension.unit))); }
      acc.content.children->pop_back();
      acc.content.children->push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type == Node::number && rhs.type == Node::numeric_color) {
      if (op != Node::sub && op != Node::div) {
        // TO DO: check that alphas match
        double r = operate(op, lhs.content.numeric_value, rhs[0].content.numeric_value);
        double g = operate(op, lhs.content.numeric_value, rhs[1].content.numeric_value);
        double b = operate(op, lhs.content.numeric_value, rhs[2].content.numeric_value);
        acc.content.children->pop_back();
        acc << Node(acc.line_number, r, g, b);
      }
      // trying to handle weird edge cases ... not sure if it's worth it
      else if (op == Node::div) {
        acc << Node(Node::div);
        acc << rhs;
      }
      else if (op == Node::sub) {
        acc << Node(Node::sub);
        acc << rhs;
      }
      else {
        acc << rhs;
      }
    }
    else if (lhs.type == Node::numeric_color && rhs.type == Node::number) {
      double r = operate(op, lhs[0].content.numeric_value, rhs.content.numeric_value);
      double g = operate(op, lhs[1].content.numeric_value, rhs.content.numeric_value);
      double b = operate(op, lhs[2].content.numeric_value, rhs.content.numeric_value);
      acc.content.children->pop_back();
      acc << Node(acc.line_number, r, g, b);
    }
    else if (lhs.type == Node::numeric_color && rhs.type == Node::numeric_color) {
      double r = operate(op, lhs[0].content.numeric_value, rhs[0].content.numeric_value);
      double g = operate(op, lhs[1].content.numeric_value, rhs[1].content.numeric_value);
      double b = operate(op, lhs[2].content.numeric_value, rhs[2].content.numeric_value);
      acc.content.children->pop_back();
      acc << Node(acc.line_number, r, g, b);
    }
    else {
      // TO DO: disallow division and multiplication on lists
      acc.content.children->push_back(rhs);
    }

    return acc;
  }

  double operate(Node::Type op, double lhs, double rhs)
  {
    // TO DO: check for division by zero
    switch (op)
    {
      case Node::add: return lhs + rhs; break;
      case Node::sub: return lhs - rhs; break;
      case Node::mul: return lhs * rhs; break;
      case Node::div: return lhs / rhs; break;
      default:        return 0;         break;
    }
  }
  
  Node apply_mixin(Node& mixin, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env)
  {
    Node params(mixin[1]);
    Node body(mixin[2].clone());
    Environment bindings;
    // bind arguments
    for (int i = 0, j = 0; i < args.size(); ++i) {
      if (args[i].type == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].content.token);
        if (!bindings.query(name)) {
          bindings[name] = eval(arg[1], env, f_env);
        }
      }
      else {
        // TO DO: ensure (j < params.size())
        Node param(params[j]);
        Token name(param.type == Node::variable ? param.content.token : param[0].content.token);
        bindings[name] = eval(args[i], env, f_env);
        ++j;
      }
    }
    // plug the holes with default arguments if any
    for (int i = 0; i < params.size(); ++i) {
      if (params[i].type == Node::assignment) {
        Node param(params[i]);
        Token name(param[0].content.token);
        if (!bindings.query(name)) {
          bindings[name] = eval(param[1], env, f_env);
        }
      }
    }
    // lexically link the new environment and eval the mixin's body
    bindings.link(env.global ? *env.global : env);
    for (int i = 0; i < body.size(); ++i) {
      body[i] = eval(body[i], bindings, f_env);
    }
    return body;
  }
  
  Node apply_function(const Function& f, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env)
  {
    map<Token, Node> bindings;
    // bind arguments
    for (int i = 0, j = 0; i < args.size(); ++i) {
      if (args[i].type == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].content.token);
        bindings[name] = eval(arg[1], env, f_env);
      }
      else {
        // TO DO: ensure (j < f.parameters.size())
        bindings[f.parameters[j]] = eval(args[i], env, f_env);
        ++j;
      }
    }
    return f(bindings);
  }

}