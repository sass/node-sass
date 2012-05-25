#include "prelexer.hpp"
#include "eval_apply.hpp"
#include "error.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;

  static void throw_eval_error(string message, string path, size_t line)
  {
    if (!path.empty() && Prelexer::string_constant(path.c_str()))
      path = path.substr(1, path.size() - 1);

    throw Error(Error::evaluation, path, line, message);
  }

  Node eval(Node& expr, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node)
  {
    switch (expr.type())
    {
      case Node::mixin: {
        env[expr[0].token()] = expr;
        return expr;
      } break;
      
      case Node::expansion: {
        Token name(expr[0].token());
        Node args(expr[1]);
        if (!env.query(name)) throw_eval_error("mixin " + name.to_string() + " is undefined", expr.path(), expr.line());
        Node mixin(env[name]);
        Node expansion(apply_mixin(mixin, args, env, f_env, new_Node));
        expr.pop_back();
        expr.pop_back();
        expr += expansion;
        return expr;
      } break;
      
      case Node::propset:
      case Node::ruleset: {
        eval(expr[1], env, f_env, new_Node);
        return expr;
      } break;
      
      case Node::root: {
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          eval(expr[i], env, f_env, new_Node);
        }
        return expr;
      } break;
      
      case Node::block: {
        Environment new_frame;
        new_frame.link(env);
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          eval(expr[i], new_frame, f_env, new_Node);
        }
        return expr;
      } break;
      
      case Node::assignment: {
        Node val(expr[1]);
        if (val.type() == Node::comma_list || val.type() == Node::space_list) {
          for (size_t i = 0, S = val.size(); i < S; ++i) {
            if (val[i].should_eval()) val[i] = eval(val[i], env, f_env, new_Node);
          }
        }
        else {
          val = eval(val, env, f_env, new_Node);
        }
        Node var(expr[0]);
        if (env.query(var.token())) {
          env[var.token()] = val;
        }
        else {
          env.current_frame[var.token()] = val;
        }
        return expr;
      } break;

      case Node::rule: {
        Node rhs(expr[1]);
        if (rhs.type() == Node::comma_list || rhs.type() == Node::space_list) {
          for (size_t i = 0, S = rhs.size(); i < S; ++i) {
            if (rhs[i].should_eval()) rhs[i] = eval(rhs[i], env, f_env, new_Node);
          }
        }
        else if (rhs.type() == Node::value_schema || rhs.type() == Node::string_schema) {
          eval(rhs, env, f_env, new_Node);
        }
        else {
          if (rhs.should_eval()) expr[1] = eval(rhs, env, f_env, new_Node);
        }
        return expr;
      } break;

      case Node::comma_list:
      case Node::space_list: {
        if (expr.should_eval()) expr[0] = eval(expr[0], env, f_env, new_Node);
        return expr;
      } break;
      
      case Node::disjunction: {
        Node result;
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          result = eval(expr[i], env, f_env, new_Node);
          if (result.type() == Node::boolean && result.boolean_value() == false) continue;
          else return result;
        }
        return result;
      } break;
      
      case Node::conjunction: {
        Node result;
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          result = eval(expr[i], env, f_env, new_Node);
          if (result.type() == Node::boolean && result.boolean_value() == false) return result;
        }
        return result;
      } break;
      
      case Node::relation: {
        
        Node lhs(eval(expr[0], env, f_env, new_Node));
        Node op(expr[1]);
        Node rhs(eval(expr[2], env, f_env, new_Node));
        
        Node T(new_Node(Node::boolean, lhs.path(), lhs.line(), true));
        Node F(new_Node(Node::boolean, lhs.path(), lhs.line(), false));
        
        switch (op.type()) {
          case Node::eq:  return (lhs == rhs) ? T : F;
          case Node::neq: return (lhs != rhs) ? T : F;
          case Node::gt:  return (lhs > rhs)  ? T : F;
          case Node::gte: return (lhs >= rhs) ? T : F;
          case Node::lt:  return (lhs < rhs)  ? T : F;
          case Node::lte: return (lhs <= rhs) ? T : F;
          default:
            throw_eval_error("unknown comparison operator " + expr.token().to_string(), expr.path(), expr.line());
            return Node();
        }
      } break;

      case Node::expression: {
        Node acc(new_Node(Node::expression, expr.path(), expr.line(), 1));
        acc << eval(expr[0], env, f_env, new_Node);
        Node rhs(eval(expr[2], env, f_env, new_Node));
        accumulate(expr[1].type(), acc, rhs, new_Node);
        for (size_t i = 3, S = expr.size(); i < S; i += 2) {
          Node rhs(eval(expr[i+1], env, f_env, new_Node));
          accumulate(expr[i].type(), acc, rhs, new_Node);
        }
        return acc.size() == 1 ? acc[0] : acc;
      } break;

      case Node::term: {
        if (expr.should_eval()) {
          Node acc(new_Node(Node::expression, expr.path(), expr.line(), 1));
          acc << eval(expr[0], env, f_env, new_Node);
          Node rhs(eval(expr[2], env, f_env, new_Node));
          accumulate(expr[1].type(), acc, rhs, new_Node);
          for (size_t i = 3, S = expr.size(); i < S; i += 2) {
            Node rhs(eval(expr[i+1], env, f_env, new_Node));
            accumulate(expr[i].type(), acc, rhs, new_Node);
          }
          return acc.size() == 1 ? acc[0] : acc;
        }
        else {
          return expr;
        }
      } break;

      case Node::textual_percentage: {
        return new_Node(expr.path(), expr.line(), std::atof(expr.token().begin), Node::numeric_percentage);
      } break;

      case Node::textual_dimension: {
        return new_Node(expr.path(), expr.line(),
                        std::atof(expr.token().begin),
                        Token::make(Prelexer::number(expr.token().begin),
                                    expr.token().end));
      } break;
      
      case Node::textual_number: {
        return new_Node(expr.path(), expr.line(), std::atof(expr.token().begin));
      } break;

      case Node::textual_hex: {        
        Node triple(new_Node(Node::numeric_color, expr.path(), expr.line(), 4));
        Token hext(Token::make(expr.token().begin+1, expr.token().end));
        if (hext.length() == 6) {
          for (int i = 0; i < 6; i += 2) {
            triple << new_Node(expr.path(), expr.line(), static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
          }
        }
        else {
          for (int i = 0; i < 3; ++i) {
            triple << new_Node(expr.path(), expr.line(), static_cast<double>(std::strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
          }
        }
        triple << new_Node(expr.path(), expr.line(), 1.0);
        return triple;
      } break;
      
      case Node::variable: {
        if (!env.query(expr.token())) throw_eval_error("reference to unbound variable " + expr.token().to_string(), expr.path(), expr.line());
        return env[expr.token()];
      } break;
      
      case Node::function_call: {
        // TO DO: default-constructed Function should be a generic callback (maybe)
        pair<string, size_t> sig(expr[0].token().to_string(), expr[1].size());
        if (!f_env.count(sig)) return expr;
        return apply_function(f_env[sig], expr[1], env, f_env, new_Node);
      } break;
      
      case Node::unary_plus: {
        Node arg(eval(expr[0], env, f_env, new_Node));
        if (arg.is_numeric()) {
          return arg;
        }
        else {
          expr[0] = arg;
          return expr;
        }
      } break;
      
      case Node::unary_minus: {
        Node arg(eval(expr[0], env, f_env, new_Node));
        if (arg.is_numeric()) {
          return new_Node(expr.path(), expr.line(), -arg.numeric_value());
        }
        else {
          expr[0] = arg;
          return expr;
        }
      } break;
      
      case Node::string_schema:
      case Node::value_schema: {
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          expr[i] = eval(expr[i], env, f_env, new_Node);
        }
        return expr;
      } break;
      
      case Node::css_import: {
        expr[0] = eval(expr[0], env, f_env, new_Node);
        return expr;
      } break;     

      default: {
        return expr;
      } break;
    }

    return expr;
  }

  Node accumulate(Node::Type op, Node& acc, Node& rhs, Node_Factory& new_Node)
  {
    Node lhs(acc.back());
    double lnum = lhs.numeric_value();
    double rnum = rhs.numeric_value();
    
    if (lhs.type() == Node::number && rhs.type() == Node::number) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum)));
      acc.pop_back();
      acc.push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type() == Node::number && rhs.type() == Node::numeric_dimension) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), rhs.unit()));
      acc.pop_back();
      acc.push_back(result);
    }
    else if (lhs.type() == Node::numeric_dimension && rhs.type() == Node::number) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), lhs.unit()));
      acc.pop_back();
      acc.push_back(result);
    }
    else if (lhs.type() == Node::numeric_dimension && rhs.type() == Node::numeric_dimension) {
      // TO DO: CHECK FOR MISMATCHED UNITS HERE
      Node result;
      if (op == Node::div)
      { result = new_Node(acc.path(), acc.line(), operate(op, lnum, rnum)); }
      else
      { result = new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), lhs.unit()); }
      acc.pop_back();
      acc.push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type() == Node::number && rhs.type() == Node::numeric_color) {
      if (op != Node::sub && op != Node::div) {
        double r = operate(op, lhs.numeric_value(), rhs[0].numeric_value());
        double g = operate(op, lhs.numeric_value(), rhs[1].numeric_value());
        double b = operate(op, lhs.numeric_value(), rhs[2].numeric_value());
        double a = rhs[3].numeric_value();
        acc.pop_back();
        acc << new_Node(acc.path(), acc.line(), r, g, b, a);
      }
      // trying to handle weird edge cases ... not sure if it's worth it
      else if (op == Node::div) {
        acc << new_Node(Node::div, acc.path(), acc.line(), 0);
        acc << rhs;
      }
      else if (op == Node::sub) {
        acc << new_Node(Node::sub, acc.path(), acc.line(), 0);
        acc << rhs;
      }
      else {
        acc << rhs;
      }
    }
    else if (lhs.type() == Node::numeric_color && rhs.type() == Node::number) {
      double r = operate(op, lhs[0].numeric_value(), rhs.numeric_value());
      double g = operate(op, lhs[1].numeric_value(), rhs.numeric_value());
      double b = operate(op, lhs[2].numeric_value(), rhs.numeric_value());
      double a = lhs[3].numeric_value();
      acc.pop_back();
      acc << new_Node(acc.path(), acc.line(), r, g, b, a);
    }
    else if (lhs.type() == Node::numeric_color && rhs.type() == Node::numeric_color) {
      if (lhs[3].numeric_value() != rhs[3].numeric_value()) throw_eval_error("alpha channels must be equal for " + lhs.to_string("") + " + " + rhs.to_string(""), lhs.path(), lhs.line());
      double r = operate(op, lhs[0].numeric_value(), rhs[0].numeric_value());
      double g = operate(op, lhs[1].numeric_value(), rhs[1].numeric_value());
      double b = operate(op, lhs[2].numeric_value(), rhs[2].numeric_value());
      double a = lhs[3].numeric_value();
      acc.pop_back();
      acc << new_Node(acc.path(), acc.line(), r, g, b, a);
    }
    else {
      // TO DO: disallow division and multiplication on lists
      acc.push_back(rhs);
    }

    return acc;
  }

  double operate(Node::Type op, double lhs, double rhs)
  {
    switch (op)
    {
      case Node::add: return lhs + rhs; break;
      case Node::sub: return lhs - rhs; break;
      case Node::mul: return lhs * rhs; break;
      case Node::div: return lhs / rhs; break;
      default:        return 0;         break;
    }
  }
  
  Node apply_mixin(Node& mixin, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node)
  {
    Node params(mixin[1]);
    Node body(new_Node(mixin[2])); // clone the body
    Environment bindings;
    // bind arguments
    for (size_t i = 0, j = 0, S = args.size(); i < S; ++i) {
      if (args[i].type() == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].token());
        // check that the keyword arg actually names a formal parameter
        bool valid_param = false;
        for (size_t k = 0, S = params.size(); k < S; ++k) {
          Node param_k = params[k];
          if (param_k.type() == Node::assignment) param_k = param_k[0];
          if (arg[0] == param_k) {
            valid_param = true;
            break;
          }
        }
        if (!valid_param) throw_eval_error("mixin " + mixin[0].to_string("") + " has no parameter named " + name.to_string(), arg.path(), arg.line());
        if (!bindings.query(name)) {
          bindings[name] = eval(arg[1], env, f_env, new_Node);
        }
      }
      else {
        // ensure that the number of ordinal args < params.size()
        if (j >= params.size()) {
          stringstream ss;
          ss << "mixin " << mixin[0].to_string("") << " only takes " << params.size() << ((params.size() == 1) ? " argument" : " arguments");
          throw_eval_error(ss.str(), args[i].path(), args[i].line());
        }
        Node param(params[j]);
        Token name(param.type() == Node::variable ? param.token() : param[0].token());
        bindings[name] = eval(args[i], env, f_env, new_Node);
        ++j;
      }
    }
    // plug the holes with default arguments if any
    for (size_t i = 0, S = params.size(); i < S; ++i) {
      if (params[i].type() == Node::assignment) {
        Node param(params[i]);
        Token name(param[0].token());
        if (!bindings.query(name)) {
          bindings[name] = eval(param[1], env, f_env, new_Node);
        }
      }
    }
    // lexically link the new environment and eval the mixin's body
    bindings.link(env.global ? *env.global : env);
    for (size_t i = 0, S = body.size(); i < S; ++i) {
      body[i] = eval(body[i], bindings, f_env, new_Node);
    }
    return body;
  }
  
  Node apply_function(const Function& f, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node)
  {
    map<Token, Node> bindings;
    // bind arguments
    for (size_t i = 0, j = 0, S = args.size(); i < S; ++i) {
      if (args[i].type() == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].token());
        bindings[name] = eval(arg[1], env, f_env, new_Node);
      }
      else {
        // TO DO: ensure (j < f.parameters.size())
        bindings[f.parameters[j]] = eval(args[i], env, f_env, new_Node);
        ++j;
      }
    }
    return f(bindings, new_Node);
  }

}
