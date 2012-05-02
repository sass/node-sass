#include <iostream>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#include "node_factory.hpp"

int main()
{
  using namespace Sass;
  using namespace std;
  
  cout << sizeof(Node) << endl;
  cout << sizeof(Node_Impl) << endl << endl;
  
  Node_Factory make = Node_Factory();
  
  Node interior(make.node(Node::block, 0, 0, 3));
  
  cout << interior.size() << endl;
  cout << interior.has_children() << endl;
  cout << interior.should_eval() << endl << endl;
  
  Node num(make.node(0, 0, 255, 123, 32));
  Node num2(make.node(0, 0, 255, 123, 32));
  Node num3(make.node(0, 0, 255, 122, 20, .75));
  
  cout << num.size() << endl;
  cout << num.has_children() << endl;
  cout << num.has_statements() << endl << endl;
  
  cout << num[1].is_numeric() << endl;
  cout << num[1].numeric_value() << endl << endl;
  
  cout << (num == num2) << endl;
  cout << (num == num3) << endl << endl;
  
  return 0;
}