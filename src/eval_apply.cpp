#include "eval_apply.hpp"
#include "error.hpp"
#include <iostream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;
  
  static void eval_error(string message, size_t line_number, const char* file_name)
  {
    string fn;
    if (file_name) {
      const char* end = Prelexer::string_constant(file_name);
      if (end) fn = string(file_name, end - file_name);
      else fn = string(file_name);
    }
    throw Error(Error::evaluation, line_number, fn, message);
  }

  Node eval(Node& expr, Environment& env, map<pair<string, size_t>, Function>& f_env, vector<vector<Node>*>& registry)
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
        if (!env.query(name)) eval_error("mixin " + name.to_string() + " is undefined", expr.line_number, expr.file_name);
        Node mixin(env[name]);
        Node expansion(apply_mixin(mixin, args, env, f_env, registry));
        expr.content.children->pop_back();
        expr.content.children->pop_back();
        expr += expansion;
        return expr;
      } break;
      
      case Node::ruleset: {
        eval(expr[1], env, f_env, registry);
        return expr;
      } break;
      
      case Node::root: {
        for (int i = 0; i < expr.size(); ++i) {
          eval(expr[i], env, f_env, registry);
        }
        return expr;
      } break;
      
      case Node::block: {
        Environment current;
        current.link(env);
        for (int i = 0; i < expr.size(); ++i) {
          eval(expr[i], current, f_env, registry);
        }
        return expr;
      } break;
      
      case Node::assignment: {
        Node val(expr[1]);
        if (val.type == Node::comma_list || val.type == Node::space_list) {
          for (int i = 0; i < val.size(); ++i) {
            if (val[i].eval_me) val[i] = eval(val[i], env, f_env, registry);
          }
        }
        else {
          val = eval(val, env, f_env, registry);
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
            if (rhs[i].eval_me) rhs[i] = eval(rhs[i], env, f_env, registry);
          }
        }
        else if (rhs.type == Node::value_schema || rhs.type == Node::string_schema) {
          eval(rhs, env, f_env, registry);
        }
        else {
          if (rhs.eval_me) expr[1] = eval(rhs, env, f_env, registry);
        }
        return expr;
      } break;

      case Node::comma_list:
      case Node::space_list: {
        if (expr.eval_me) expr[0] = eval(expr[0], env, f_env, registry);
        return expr;
      } break;
      
      case Node::disjunction: {
        Node result;
        for (int i = 0; i < expr.size(); ++i) {
          // if (expr[i].type == Node::relation ||
          //     expr[i].type == Node::function_call && expr[0].content.token.to_string() == "not") {
          result = eval(expr[i], env, f_env, registry);
          if (result.type == Node::boolean && result.content.boolean_value == false) continue;
          else return result;
        }
        return result;
      } break;
      
      case Node::conjunction: {
        Node result;
        for (int i = 0; i < expr.size(); ++i) {
          result = eval(expr[i], env, f_env, registry);
          if (result.type == Node::boolean && result.content.boolean_value == false) return result;
        }
        return result;
      } break;
      
      case Node::relation: {
        
        Node lhs(eval(expr[0], env, f_env, registry));
        Node op(expr[1]);
        Node rhs(eval(expr[2], env, f_env, registry));
        
        Node T(Node::boolean);
        T.line_number = lhs.line_number;
        T.content.boolean_value = true;
        Node F(T);
        F.content.boolean_value = false;
        
        switch (op.type) {
          case Node::eq:  return (lhs == rhs) ? T : F;
          case Node::neq: return (lhs != rhs) ? T : F;
          case Node::gt:  return (lhs > rhs)  ? T : F;
          case Node::gte: return (lhs >= rhs) ? T : F;
          case Node::lt:  return (lhs < rhs)  ? T : F;
          case Node::lte: return (lhs <= rhs) ? T : F;
        }
      } break;

      case Node::expression: {
        Node acc(Node::expression, registry, expr.line_number, 1);
        acc << eval(expr[0], env, f_env, registry);
        Node rhs(eval(expr[2], env, f_env, registry));
        accumulate(expr[1].type, acc, rhs, registry);
        for (int i = 3; i < expr.size(); i += 2) {
          Node rhs(eval(expr[i+1], env, f_env, registry));
          accumulate(expr[i].type, acc, rhs, registry);
        }
        return acc.size() == 1 ? acc[0] : acc;
      } break;

      case Node::term: {
        if (expr.eval_me) {
          Node acc(Node::expression, registry, expr.line_number, 1);
          acc << eval(expr[0], env, f_env, registry);
          Node rhs(eval(expr[2], env, f_env, registry));
          accumulate(expr[1].type, acc, rhs, registry);
          for (int i = 3; i < expr.size(); i += 2) {
            Node rhs(eval(expr[i+1], env, f_env, registry));
            accumulate(expr[i].type, acc, rhs, registry);
          }
          return acc.size() == 1 ? acc[0] : acc;
        }
        else {
          return expr;
        }
      } break;

      case Node::textual_percentage: {
        Node pct(expr.line_number, std::atof(expr.content.token.begin));
        pct.type = Node::numeric_percentage;
        return pct;
      } break;

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
        Node triple(Node::numeric_color, registry, expr.line_number, 4);
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
        if (!env.query(expr.content.token)) eval_error("reference to unbound variable " + expr.content.token.to_string(), expr.line_number, expr.file_name);
        return env[expr.content.token];
      } break;
      
      case Node::function_call: {
        // TO DO: default-constructed Function should be a generic callback
        pair<string, size_t> sig(expr[0].content.token.to_string(), expr[1].size());
        if (!f_env.count(sig)) {
          stringstream ss;
          ss << "no function named " << expr[0].content.token.to_string() << " taking " << expr[1].size() << " arguments has been defined";
          eval_error(ss.str(), expr.line_number, expr.file_name);
        }
        return apply_function(f_env[sig], expr[1], env, f_env, registry);
      } break;
      
      case Node::string_schema:
      case Node::value_schema: {
        cerr << "evaluating schema of size " << expr.size() << endl;
        for (int i = 0; i < expr.size(); ++i) {
          expr[i] = eval(expr[i], env, f_env, registry);
        }
        return expr;
      } break;
      
      default: {
        return expr;
      }
    }
  }

  Node accumulate(Node::Type op, Node& acc, Node& rhs, vector<vector<Node>*>& registry)
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
        double r = operate(op, lhs.content.numeric_value, rhs[0].content.numeric_value);
        double g = operate(op, lhs.content.numeric_value, rhs[1].content.numeric_value);
        double b = operate(op, lhs.content.numeric_value, rhs[2].content.numeric_value);
        double a = rhs[3].content.numeric_value;
        acc.content.children->pop_back();
        acc << Node(registry, acc.line_number, r, g, b, a);
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
      double a = lhs[3].content.numeric_value;
      acc.content.children->pop_back();
      acc << Node(registry, acc.line_number, r, g, b, a);
    }
    else if (lhs.type == Node::numeric_color && rhs.type == Node::numeric_color) {
      if (lhs[3].content.numeric_value != rhs[3].content.numeric_value) eval_error("alpha channels must be equal for " + lhs.to_string("") + " + " + rhs.to_string(""), lhs.line_number, lhs.file_name);
      double r = operate(op, lhs[0].content.numeric_value, rhs[0].content.numeric_value);
      double g = operate(op, lhs[1].content.numeric_value, rhs[1].content.numeric_value);
      double b = operate(op, lhs[2].content.numeric_value, rhs[2].content.numeric_value);
      double a = lhs[3].content.numeric_value;
      acc.content.children->pop_back();
      acc << Node(registry, acc.line_number, r, g, b, a);
    }
    // else if (lhs.type == Node::concatenation) {
    //   lhs << rhs;
    // }
    // else if (lhs.type == Node::string_constant || rhs.type == Node::string_constant) {
    //   acc.content.children->pop_back();
    //   Node cat(Node::concatenation, lhs.line_number, 2);
    //   cat << lhs << rhs;
    //   acc << cat;
    // }
    else {
      // TO DO: disallow division and multiplication on lists
      acc.content.children->push_back(rhs);
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
  
  Node apply_mixin(Node& mixin, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, vector<vector<Node>*>& registry)
  {
    Node params(mixin[1]);
    Node body(mixin[2].clone(registry));
    Environment bindings;
    // bind arguments
    for (int i = 0, j = 0; i < args.size(); ++i) {
      if (args[i].type == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].content.token);
        // check that the keyword arg actually names a formal parameter
        bool valid_param = false;
        for (int k = 0; k < params.size(); ++k) {
          Node param_k = params[k];
          if (param_k.type == Node::assignment) param_k = param_k[0];
          if (arg[0] == param_k) {
            valid_param = true;
            break;
          }
        }
        if (!valid_param) eval_error("mixin " + mixin[0].to_string("") + " has no parameter named " + name.to_string(), arg.line_number, arg.file_name);
        if (!bindings.query(name)) {
          bindings[name] = eval(arg[1], env, f_env, registry);
        }
      }
      else {
        // ensure that the number of ordinal args < params.size()
        if (j >= params.size()) {
          stringstream ss;
          ss << "mixin " << mixin[0].to_string("") << " only takes " << params.size() << ((params.size() == 1) ? " argument" : " arguments");
          eval_error(ss.str(), args[i].line_number, args[i].file_name);
        }
        Node param(params[j]);
        Token name(param.type == Node::variable ? param.content.token : param[0].content.token);
        bindings[name] = eval(args[i], env, f_env, registry);
        ++j;
      }
    }
    // plug the holes with default arguments if any
    for (int i = 0; i < params.size(); ++i) {
      if (params[i].type == Node::assignment) {
        Node param(params[i]);
        Token name(param[0].content.token);
        if (!bindings.query(name)) {
          bindings[name] = eval(param[1], env, f_env, registry);
        }
      }
    }
    // lexically link the new environment and eval the mixin's body
    bindings.link(env.global ? *env.global : env);
    for (int i = 0; i < body.size(); ++i) {
      body[i] = eval(body[i], bindings, f_env, registry);
    }
    return body;
  }
  
  Node apply_function(const Function& f, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, vector<vector<Node>*>& registry)
  {
    map<Token, Node> bindings;
    // bind arguments
    for (int i = 0, j = 0; i < args.size(); ++i) {
      if (args[i].type == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].content.token);
        bindings[name] = eval(arg[1], env, f_env, registry);
      }
      else {
        // TO DO: ensure (j < f.parameters.size())
        bindings[f.parameters[j]] = eval(args[i], env, f_env, registry);
        ++j;
      }
    }
    return f(bindings, registry);
  }

}