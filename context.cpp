#include "context.hpp"
#include <iostream>
using std::cerr; using std::endl;

namespace Sass {
  
  Context::Context()
  : global_env(Environment()),
    function_env(map<string, Function>()),
    source_refs(vector<char*>()),
    ref_count(0)
  {
    register_functions();
  }
  
  Context::~Context()
  {
    for (int i = 0; i < source_refs.size(); ++i) {
      delete[] source_refs[i];
    }
  }
  
  inline void Context::register_function(Function_Descriptor d, Implementation ip)
  {
    function_env[d[0]] = Function(d, ip);
    cerr << "Registered function: " << d[0] << endl;
    cerr << "Verifying " << function_env[string(d[0])].name << endl;
  }
  
  void Context::register_functions()
  {
    using namespace Functions;
    register_function(rgb_descriptor,  rgb);
    register_function(rgba_descriptor, rgba);
  }
  
}