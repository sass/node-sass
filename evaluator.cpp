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
        // long numval = std::strtol(expr.token.begin + 1, NULL, 16);
        // Node result(expr.line_number, numval);
        // result.is_hex = true;
        // return result;
        
        Node triple(expr.line_number, Node::hex_triple, 3);
        Token hext(expr.token.begin+1, expr.token.end);
        
        if (hext.length() == 6) {
          for (int i = 0; i < 6; i += 2) {
            Node thing(expr.line_number, static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
            cerr << "evaled a hex triplet: " << thing.to_string("") << endl;
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