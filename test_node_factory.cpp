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
  cout << sizeof(Node_Impl) << endl;
  
  return 0;
}