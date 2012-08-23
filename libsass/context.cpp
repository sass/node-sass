#include "context.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "prelexer.hpp"
#include "color_names.hpp"
using std::cerr; using std::endl;

namespace Sass {
  using std::pair;
  
  void Context::collect_include_paths(const char* paths_str)
  {
    const size_t wd_len = 1024;
    char wd[wd_len];
    include_paths.push_back(getcwd(wd, wd_len));
    if (*include_paths.back().rbegin() != '/') include_paths.back() += '/';

    if (paths_str) {
      const char* beg = paths_str;
      const char* end = Prelexer::find_first<':'>(beg);

      while (end) {
        string path(beg, end - beg);
        if (!path.empty()) {
          if (*path.rbegin() != '/') path += '/';
          include_paths.push_back(path);
        }
        beg = end + 1;
        end = Prelexer::find_first<':'>(beg);
      }

      string path(beg);
      if (!path.empty()) {
        if (*path.rbegin() != '/') path += '/';
        include_paths.push_back(path);
      }
    }

    // for (int i = 0; i < include_paths.size(); ++i) {
    //   cerr << include_paths[i] << endl;
    // }
  }
  
  Context::Context(const char* paths_str, const char* img_path_str)
  : global_env(Environment()),
    function_env(map<string, Function>()),
    extensions(multimap<Node, Node>()),
    pending_extensions(vector<pair<Node, Node> >()),
    source_refs(vector<char*>()),
    include_paths(vector<string>()),
    color_names_to_values(map<string, Node>()),
    color_values_to_names(map<Node, string>()),
    new_Node(Node_Factory()),
    image_path(0),
    ref_count(0),
    has_extensions(false)
  {
    register_functions();
    collect_include_paths(paths_str);
    setup_color_map();

    string path_string(img_path_str);
    path_string = "'" + path_string + "/'";
    image_path = new char[path_string.length() + 1];
    std::strcpy(image_path, path_string.c_str());
  }
  
  Context::~Context()
  {
    for (size_t i = 0; i < source_refs.size(); ++i) {
      delete[] source_refs[i];
    }
    delete[] image_path;
    new_Node.free();
    // cerr << "Deallocated " << i << " source string(s)." << endl;
  }
  
  inline void Context::register_function(Function_Descriptor d, Primitive ip)
  {
    Function f(d, ip, new_Node);
    // function_env[pair<string, size_t>(f.name, f.parameters.size())] = f;
    function_env[f.name] = f;
  }
  
  inline void Context::register_function(Function_Descriptor d, Primitive ip, size_t arity)
  {
    Function f(d, ip, new_Node);
    // function_env[pair<string, size_t>(f.name, arity)] = f;
    function_env[f.name] = f;
  }

  inline void Context::register_overload_stub(string name)
  {
    function_env[name] = Function(name, true);
  }
  
  void Context::register_functions()
  {
    using namespace Functions;
    // RGB Functions
    register_function(rgb_descriptor,  rgb);
    register_overload_stub("rgba");
    register_function(rgba_4_descriptor, rgba_4);
    register_function(rgba_2_descriptor, rgba_2);
    register_function(red_descriptor, red);
    register_function(green_descriptor, green);
    register_function(blue_descriptor, blue);
    register_overload_stub("mix");
    register_function(mix_2_descriptor, mix_2);
    register_function(mix_3_descriptor, mix_3);
    // HSL Functions
    register_function(hsla_descriptor, hsla);
    register_function(hsl_descriptor, hsl);
    register_function(invert_descriptor, invert);
    // Opacity Functions
    register_function(alpha_descriptor, alpha);
    register_function(opacity_descriptor, alpha);
    register_function(opacify_descriptor, opacify);
    register_function(fade_in_descriptor, opacify);
    register_function(transparentize_descriptor, transparentize);
    register_function(fade_out_descriptor, transparentize);
    // String Functions
    register_function(unquote_descriptor, unquote);
    register_function(quote_descriptor, quote);
    // Number Functions
    register_function(percentage_descriptor, percentage);
    register_function(round_descriptor, round);
    register_function(ceil_descriptor, ceil);
    register_function(floor_descriptor, floor);
    register_function(abs_descriptor, abs);
    // List Functions
    register_function(length_descriptor, length);
    register_function(nth_descriptor, nth);
    register_overload_stub("join");
    register_function(join_2_descriptor, join_2);
    register_function(join_3_descriptor, join_3);
    register_overload_stub("append");
    register_function(append_2_descriptor, append_2);
    register_function(append_3_descriptor, append_3);
    register_overload_stub("compact");
    register_function(compact_1_descriptor, compact);
    register_function(compact_2_descriptor, compact);
    register_function(compact_3_descriptor, compact);
    register_function(compact_4_descriptor, compact);
    register_function(compact_5_descriptor, compact);
    register_function(compact_6_descriptor, compact);
    register_function(compact_7_descriptor, compact);
    register_function(compact_8_descriptor, compact);
    register_function(compact_9_descriptor, compact);
    register_function(compact_10_descriptor, compact);
    register_function(compact_11_descriptor, compact);
    register_function(compact_12_descriptor, compact);
    // Introspection Functions
    register_function(type_of_descriptor, type_of);
    register_function(unit_descriptor, unit);
    register_function(unitless_descriptor, unitless);
    register_function(comparable_descriptor, comparable);
    // Boolean Functions
    register_function(not_descriptor, not_impl);
  }

  void Context::setup_color_map()
  {
    size_t i = 0;
    while (color_names[i]) {
      string name(color_names[i]);
      Node value(new_Node("[COLOR TABLE]", 0,
                          color_values[i*3],
                          color_values[i*3+1],
                          color_values[i*3+2],
                          1));
      color_names_to_values[name] = value;
      color_values_to_names[value] = name;
      ++i;
    }
  }
  
}
