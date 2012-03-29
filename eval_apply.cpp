#include "eval_apply.hpp"
#include <iostream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;

  Node eval(Node& expr, Environment& env)
  {
    switch (expr.type)
    {
      case Node::mixin: {
        env[expr[0].token] = expr;
        // cerr << "DEFINED MIXIN: " << string(expr[0].token) << endl << endl;
        return expr;
      } break;
      
      case Node::expansion: {
        Token name(expr[0].token);
        // cerr << "EVALUATING EXPANSION: " << string(name) << endl;
        Node args(expr[1]);
        // Node parent(expr[2]);
        Node mixin(env[name]);
        // Node expansion(apply(mixin, args, env));
        // parent.has_rules_or_comments |= expansion.has_rules_or_comments;
        // parent.has_rulesets          |= expansion.has_rulesets;
        // parent.has_propsets          |= expansion.has_propsets;
        expr.children->pop_back();
        expr.children->pop_back();
        // expr.children->pop_back();
        expr += Node(apply(mixin, args, env));
        return expr;
      } break;
      
      case Node::assignment: {
        Node val(expr[1]);
        if (val.type == Node::comma_list || val.type == Node::space_list) {
          for (int i = 0; i < val.size(); ++i) {
            if (val[i].eval_me) val[i] = eval(val[i], env);
          }
        }
        else {
          val = eval(val, env);
        }
        if (env.query(expr[0].token)) {
          env[expr[0].token] = val;
        }
        else {
          env.frame[expr[0].token] = val;
        }
        return expr;
      } break;

      case Node::rule: {
        Node rhs(expr[1]);
        if (rhs.type == Node::comma_list || rhs.type == Node::space_list) {
          for (int i = 0; i < rhs.size(); ++i) {
            if (rhs[i].eval_me) rhs[i] = eval(rhs[i], env);
          }
        }
        else {
          expr[1] = eval(rhs, env);
        }
        return expr;
      } break;

      case Node::comma_list:
      case Node::space_list: {
        if (expr.eval_me) {
          // *(expr.children->begin()) = eval(expr[0], env);
          expr[0] = eval(expr[0], env);
        }
        return expr;
      } break;

      case Node::expression: {
        Node acc(expr.line_number, Node::expression, eval(expr[0], env));
        Node rhs(eval(expr[2], env));
        accumulate(expr[1].type, acc, rhs);
        for (int i = 3; i < expr.size(); i += 2) {
          Node rhs(eval(expr[i+1], env));
          accumulate(expr[i].type, acc, rhs);
        }
        return acc.size() == 1 ? acc[0] : acc;
      } break;

      case Node::term: {
        if (expr.eval_me) {
          Node acc(expr.line_number, Node::expression, eval(expr[0], env));
          Node rhs(eval(expr[2], env));
          accumulate(expr[1].type, acc, rhs);
          for (int i = 3; i < expr.size(); i += 2) {
            Node rhs(eval(expr[i+1], env));
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
        double numval = std::atof(expr.token.begin);
        Token unit(Prelexer::number(expr.token.begin), expr.token.end);
        return Node(expr.line_number, numval, unit);
      } break;
      
      case Node::textual_number: {
        double numval = std::atof(expr.token.begin);
        return Node(expr.line_number, numval);
      } break;

      case Node::textual_hex: {        
        Node triple(expr.line_number, Node::hex_triple, 3);
        Token hext(expr.token.begin+1, expr.token.end);
        if (hext.length() == 6) {
          for (int i = 0; i < 6; i += 2) {
            Node thing(expr.line_number, static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
            triple << Node(expr.line_number, static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
          }
        }
        else {
          for (int i = 0; i < 3; ++i) {
            triple << Node(expr.line_number, static_cast<double>(std::strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
          }
        }
        return triple;       
      } break;
      
      case Node::variable: {
        return env[expr.token];
      } break;
      
      default: {
        return expr;
      }
    }
  }

  Node accumulate(Node::Type op, Node& acc, Node& rhs)
  {
    Node lhs(acc.children->back());
    double lnum = lhs.numeric_value;
    double rnum = rhs.numeric_value;
    
    if (lhs.type == Node::number && rhs.type == Node::number) {
      Node result(acc.line_number, operate(op, lnum, rnum));
      acc.children->pop_back();
      acc.children->push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type == Node::number && rhs.type == Node::numeric_dimension) {
      // TO DO: disallow division
      Node result(acc.line_number, operate(op, lnum, rnum), rhs.token);
      acc.children->pop_back();
      acc.children->push_back(result);
    }
    else if (lhs.type == Node::numeric_dimension && rhs.type == Node::number) {
      Node result(acc.line_number, operate(op, lnum, rnum), lhs.token);
      acc.children->pop_back();
      acc.children->push_back(result);
    }
    else if (lhs.type == Node::numeric_dimension && rhs.type == Node::numeric_dimension) {
      // TO DO: CHECK FOR MISMATCHED UNITS HERE
      Node result;
      if (op == Node::div)
      { result = Node(acc.line_number, operate(op, lnum, rnum)); }
      else
      { result = Node(acc.line_number, operate(op, lnum, rnum), lhs.token); }
      acc.children->pop_back();
      acc.children->push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type == Node::number && rhs.type == Node::hex_triple) {
      if (op != Node::sub && op != Node::div) {
        double h1 = operate(op, lhs.numeric_value, rhs.children->at(0).numeric_value);
        double h2 = operate(op, lhs.numeric_value, rhs.children->at(1).numeric_value);
        double h3 = operate(op, lhs.numeric_value, rhs.children->at(2).numeric_value);
        acc.children->pop_back();
        acc << Node(acc.line_number, h1, h2, h3);
      }
      // trying to handle weird edge cases ... not sure if it's worth it
      else if (op == Node::div) {
        acc << Node(acc.line_number, Node::div);
        acc << rhs;
      }
      else if (op == Node::sub) {
        acc << Node(acc.line_number, Node::sub);
        acc << rhs;
      }
      else {
        acc << rhs;
      }
    }
    else if (lhs.type == Node::hex_triple && rhs.type == Node::number) {
      double h1 = operate(op, lhs.children->at(0).numeric_value, rhs.numeric_value);
      double h2 = operate(op, lhs.children->at(1).numeric_value, rhs.numeric_value);
      double h3 = operate(op, lhs.children->at(2).numeric_value, rhs.numeric_value);
      acc.children->pop_back();
      acc << Node(acc.line_number, h1, h2, h3);
    }
    else if (lhs.type == Node::hex_triple && rhs.type == Node::hex_triple) {
      double h1 = operate(op, lhs.children->at(0).numeric_value, rhs.children->at(0).numeric_value);
      double h2 = operate(op, lhs.children->at(1).numeric_value, rhs.children->at(1).numeric_value);
      double h3 = operate(op, lhs.children->at(2).numeric_value, rhs.children->at(2).numeric_value);
      acc.children->pop_back();
      acc << Node(acc.line_number, h1, h2, h3);
    }
    else {
      // TO DO: disallow division and multiplication on lists
      acc.children->push_back(rhs);
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
  
  Node apply(Node& mixin, const Node& args, Environment& env)
  {
    // cerr << "APPLYING MIXIN: " << string(mixin[0].token) << endl;
    Node params(mixin[1]);
    Node body(mixin[2].clone());
    Environment m_env;
    // cerr << "CLONED BODY" << endl;
    // bind arguments
    for (int i = 0, j = 0; i < args.size(); ++i) {
      if (args[i].type == Node::assignment) {
        Node arg(args[i]);
        Token name(arg[0].token);
        if (!m_env.query(name)) {
          m_env[name] = eval(arg[1], env);
        }
      }
      else {
        // TO DO: ensure (j < params.size())
        Node param(params[j]);
        Token name(param.type == Node::variable ? param.token : param[0].token);
        m_env[name] = eval(args[i], env);
        ++j;
      }
    }
    // cerr << "BOUND ARGS FOR " << string(mixin[0].token) << endl;
    // plug the holes with default arguments if any
    for (int i = 0; i < params.size(); ++i) {
      if (params[i].type == Node::assignment) {
        Node param(params[i]);
        Token name(param[0].token);
        if (!m_env.query(name)) {
          m_env[name] = eval(param[1], env);
        }
      }
    }
    // cerr << "BOUND DEFAULT ARGS FOR " << string(mixin[0].token) << endl;
    m_env.link(env.parent ? *env.parent : env);
    // cerr << "LINKED ENVIRONMENT FOR " << string(mixin[0].token) << endl << endl;
    for (int i = 0; i < body.size(); ++i) {
      body[i] = eval(body[i], m_env);
    }
    return body;
  }
}