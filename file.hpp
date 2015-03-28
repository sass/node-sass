#ifndef SASS_FILE_H
#define SASS_FILE_H

#include <string>

namespace Sass {
  using namespace std;
  class Context;
  namespace File {
    string get_cwd();
    string base_name(string);
    string dir_name(string);
    string join_paths(string, string);
    bool file_exists(const string& path);
    bool is_absolute_path(const string& path);
    string make_canonical_path (string path);
    string make_absolute_path(const string& path, const string& cwd);
    string resolve_relative_path(const string& uri, const string& base, const string& cwd);
    string resolve_file_name(const string& base, const string& name);
    char* resolve_and_load(string path, string& real_path);
    char* read_file(string path);
  }
}

#endif
