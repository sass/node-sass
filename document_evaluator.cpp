#include "document.hpp"
#include <iostream>

Node eval(const Node& expr)
{
  switch (expr.type)
  {
    case expression: {
      Node acc(expr.line_number, Node::expression, eval(expr.children->at(0)));
      Node rhs(eval(expr.children->at(2)));
      accumulate(expr.children->at(1).type, acc, rhs);
      for (int i = 3; i < expr.children->size(); i += 2) {
        Node rhs(eval(expr.children->at(i+1)));
        accumulate(expr.children->at(i), acc, rhs);
      }      
      return acc;
    } break;
    
    case term: {
      
      
    } break;
  }
}


Node accumulate(const Node::Type op, Node& acc, const Node& rhs)
{
  Node lhs(acc.children->back());
  
  if (lhs.type == Node::number && rhs.type == Node::number) {
    Node result(acc.line_number, Node::number, operate(op, lhs, rhs));
    acc.children->pop_back();
    acc.children->push_back(result);
  }
  else if (lhs.type == Node::number && rhs.type == Node::numeric_dimension) {
    Node result(acc.line_number, Node::numeric_dimension, operate(op, lhs, rhs), rhs.token);
    acc.children->pop_back();
    acc.children->push_back(result);
  }
  else if (lhs.type == Node::numeric_dimension && rhs.type == Node::number) {
    Node result(acc.line_number, Node::numeric_dimension, operate(op, lhs, rhs), lhs.token);
    acc.children->pop_back();
    acc.children->push_back(result);
  }
  else if (lhs.type == Node::numeric_dimension && rhs.type == Node::numeric_dimension) {
    // CHECK FOR MISMATCHED UNITS HERE
    Node result(acc.line_number, Node::numeric_dimension, operate(op, lhs, rhs), lhs.token);
    acc.children->pop_back();
    acc.children->push_back(result);
  }
  else {
    acc.children->push_back(rhs);
  }
  
  return acc;
}

double operate(const Node::Type op, double lhs, double rhs)
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