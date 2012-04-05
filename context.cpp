#include "context.hpp"
#include <iostream>
using std::cerr; using std::endl;

namespace Sass {
  using std::pair;
  
  Context::Context()
  : global_env(Environment()),
    function_env(map<pair<string, size_t>, Function>()),
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
    Function f(d, ip);
    function_env[pair<string, size_t>(f.name, f.parameters.size())] = f;
  }
  
  void Context::register_functions()
  {
    using namespace Functions;
    register_function(rgb_descriptor,  rgb);
    register_function(rgba_4_descriptor, rgba_4);
    register_function(rgba_2_descriptor, rgba_2);
  }
  
}