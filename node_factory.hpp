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
    Node_Impl* alloc_Node_Impl(Node::Type type, string file, size_t line);
    // returns a deep-copy of its argument
    Node_Impl* alloc_Node_Impl(Node_Impl* ip);
  public:
    // for cloning nodes
    Node operator()(const Node& n1);
    // for making leaf nodes out of terminals/tokens
    Node operator()(Node::Type type, string file, size_t line, const Token& t);
    // for making interior nodes that have children
    Node operator()(Node::Type type, string file, size_t line, size_t size);
    // for making nodes representing numbers
    Node operator()(string file, size_t line, double v);
    // for making nodes representing numeric dimensions (e.g. 5px, 3em)
    Node operator()(string file, size_t line, double v, const Token& t);
    // for making nodes representing rgba color quads
    Node operator()(string file, size_t line, double r, double g, double b, double a = 1.0);

    void free();
  };
  
}