#include "node_factory.hpp"

namespace Sass {
  
  Node_Impl* Node_Factory::alloc_Node_Impl(Node::Type type, string file, size_t line)
  {
    Node_Impl* ip = new Node_Impl();
    ip->type = type;
    ip->file_name = file;
    ip->line_number = line;
    pool_.push_back(ip);
    return ip;
  }

  // returns a deep-copy of its argument
  Node_Impl* Node_Factory::alloc_Node_Impl(Node_Impl* ip)
  {
    Node_Impl* ip_cpy = new Node_Impl(*ip);
    if (ip_cpy->has_children) {
      for (size_t i = 0, S = ip_cpy->size(); i < S; ++i) {
        Node n(ip_cpy->at(i));
        n.ip_ = alloc_Node_Impl(n.ip_);
      }
    }
    return ip_cpy;
  }

  // for cloning nodes
  Node Node_Factory::operator()(const Node& n1)
  {
    Node_Impl* ip_cpy = alloc_Node_Impl(n1.ip_); // deep-copy the implementation object
    return Node(ip_cpy);
  }

  // for making leaf nodes out of terminals/tokens
  Node Node_Factory::operator()(Node::Type type, string file, size_t line, const Token& t)
  {
    Node_Impl* ip = alloc_Node_Impl(type, file, line);
    ip->value.token = t;
    return Node(ip);
  }

  // for making interior nodes that have children
  Node Node_Factory::operator()(Node::Type type, string file, size_t line, size_t size)
  {
    Node_Impl* ip = alloc_Node_Impl(type, file, line);
    ip->children.reserve(size);
    return Node(ip);
  }

  // for making nodes representing numbers
  Node Node_Factory::operator()(string file, size_t line, double v)
  {
    Node_Impl* ip = alloc_Node_Impl(Node::number, file, line);
    ip->value.numeric = v;
    return Node(ip);
  }

  // for making nodes representing numeric dimensions (e.g. 5px, 3em)
  Node Node_Factory::operator()(string file, size_t line, double v, const Token& t)
  {
    Node_Impl* ip = alloc_Node_Impl(Node::numeric_dimension, file, line);
    ip->value.dimension.numeric = v;
    ip->value.dimension.unit = t;
    return Node(ip);
  }
  
  // for making nodes representing rgba color quads
  Node Node_Factory::operator()(string file, size_t line, double r, double g, double b, double a)
  {
    Node color((*this)(Node::numeric_color, file, line, 4));
    color << (*this)(file, line, r)
          << (*this)(file, line, g)
          << (*this)(file, line, b)
          << (*this)(file, line, a);
    return color;
  }

  void Node_Factory::free()
  { for (size_t i = 0, S = pool_.size(); i < S; ++i) delete pool_[i]; }

}