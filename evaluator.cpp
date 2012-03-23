#include "evaluator.hpp"
#include <iostream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;

  Node eval(const Node& expr)
  {
    switch (expr.type)
    {
      case Node::comma_list:
      case Node::space_list: {
        if (expr.eval_me) {
          *(expr.children->begin()) = eval(expr.children->front());
        }
        return expr;
      } break;

      case Node::expression: {
        Node acc(expr.line_number, Node::expression, eval(expr.children->at(0)));
        Node rhs(eval(expr.children->at(2)));
        accumulate(expr.children->at(1).type, acc, rhs);
        for (int i = 3; i < expr.children->size(); i += 2) {
          Node rhs(eval(expr.children->at(i+1)));
          accumulate(expr.children->at(i).type, acc, rhs);
        }
        return acc.children->size() == 1 ? acc.children->front() : acc;
      } break;

      case Node::term: {
        if (expr.eval_me) {
          Node acc(expr.line_number, Node::expression, eval(expr.children->at(0)));
          Node rhs(eval(expr.children->at(2)));
          accumulate(expr.children->at(1).type, acc, rhs);
          for (int i = 3; i < expr.children->size(); i += 2) {
            Node rhs(eval(expr.children->at(i+1)));
            accumulate(expr.children->at(i).type, acc, rhs);
          }
          return acc.children->size() == 1 ? acc.children->front() : acc;
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
      
      default: {
        return expr;
      }
    }
  }

  Node accumulate(const Node::Type op, Node& acc, Node& rhs)
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

  double operate(const Node::Type op, double lhs, double rhs)
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
  
}