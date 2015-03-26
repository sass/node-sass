#ifndef SASS_ENVIRONMENT_H
#define SASS_ENVIRONMENT_H

#include <map>
#include <string>
#include <iostream>

#include "ast_def_macros.hpp"

namespace Sass {
  using std::string;
  using std::map;
  using std::cerr;
  using std::endl;

  template <typename T>
  class Environment {
    // TODO: test with unordered_map
    map<string, T> current_frame_;
    ADD_PROPERTY(Environment*, parent);

  public:
    Environment() : current_frame_(map<string, T>()), parent_(0) { }

    map<string, T>& current_frame() { return current_frame_; }

    void link(Environment& env) { parent_ = &env; }
    void link(Environment* env) { parent_ = env; }

    bool has(const string key) const
    {
      if (current_frame_.count(key))  return true;
      else if (parent_)               return parent_->has(key);
      else                            return false;
    }

    bool current_frame_has(const string key) const
    { return !!current_frame_.count(key); }

    void current_frame_set(const string key, T val)
    { current_frame_[key] = val; }

    void global_frame_set(const string key, T val)
    { global_frame()->current_frame_[key] = val; }

    Environment* grandparent() const
    {
      if(parent_ && parent_->parent_) return parent_->parent_;
      else return 0;
    }

    Environment* global_frame()
    {
      Environment* cur = this;
      // looks like global variables
      // are in the second last parent
      while (cur->grandparent()) {
        cur = cur->parent_;
      }
      return cur;
    }

    bool global_frame_has(const string key) const
    {
      if(parent_ && !grandparent()) {
        return has(key);
      }
      else if(parent_) {
        return parent_->global_frame_has(key);
      }
      else {
        return false;
      }
    }

    T& operator[](const string key)
    {
      if (current_frame_.count(key))  return current_frame_[key];
      else if (parent_)               return (*parent_)[key];
      else                            return current_frame_[key];
    }

#ifdef DEBUG
    void print()
    {
      for (typename map<string, T>::iterator i = current_frame_.begin(); i != current_frame_.end(); ++i) {
        cerr << i->first << endl;
      }
      if (parent_) {
        cerr << "---" << endl;
        parent_->print();
      }
    }
#endif

  };
}

#endif
