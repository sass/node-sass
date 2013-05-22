#define SASS_ENVIRONMENT

#include <string>
#include <map>
#include "boilerplate_macros.hpp"

namespace Sass {
  using std::string;
  using std::map;

  template <typename T>
  class Environment {
    // TODO: test with unordered_map
    map<string, T> current_frame_;
    ADD_PROPERTY(Environment*, parent);

    Environment() : current_frame_(map<string, T>()), parent_(0) { }

    void link(Environment& env) { parent_ = &env; }
    void link(Environment* env) { parent_ = env; }

    bool query(const string& key) const
    {
      if (current_frame_.count(key)) return true;
      else if (parent)              return parent->query(key);
      else                          return false;
    }

    T& operator[](const string& key)
    {
      if (current_frame_.count(key)) return current_frame_[key];
      else if (parent)               return (*parent)[key];
      else                           return current_frame_[key];
    }

    // void print()
    // {
    //   for (map<string, T>::iterator i = current_frame.begin(); i != current_frame.end(); ++i) {
    //     cerr << i->first.to_string() << ": " << i->second.to_string() << endl;
    //   }
    //   if (parent) {
    //     cerr << "---" << endl;
    //     parent->print();
    //   }
    // }
  };
}