#include <vector>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

namespace Sass {
  using namespace std;

  struct Token;
  struct Node_Impl;
  
  class Node_Factory {
    vector<Node_Impl*> pool_;
    Node_Impl* alloc_Node_Impl(Node::Type type, string* file, size_t line);
  public:
    Node operator()(Node::Type type, string* file, size_t line, const Token& t);
    Node operator()(Node::Type type, string* file, size_t line, size_t size);
    Node operator()(string* file, size_t line, double v);
    Node operator()(string* file, size_t line, double v, const Token& t);
    Node operator()(string* file, size_t line, double r, double g, double b, double a = 1.0);
  };
  
}