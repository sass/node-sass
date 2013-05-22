#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#define PATH_SEP ';'
#else
#include <unistd.h>
#define PATH_SEP ':'
#endif

#include <cstring>
#include <iostream>
#include <sstream>
#include "context.hpp"
#include "constants.hpp"
#include "color_names.hpp"
#include "prelexer.hpp"


namespace Sass {
  using namespace Constants;
  using std::cerr; using std::endl;

  void Context::collect_include_paths(const char* paths_str)
  {
    const size_t wd_len = 1024;
    char wd[wd_len];
    include_paths.push_back(getcwd(wd, wd_len));
    if (*include_paths.back().rbegin() != '/') include_paths.back() += '/';

    if (paths_str) {
      const char* beg = paths_str;
      const char* end = Prelexer::find_first<PATH_SEP>(beg);

      while (end) {
        string path(beg, end - beg);
        if (!path.empty()) {
          if (*path.rbegin() != '/') path += '/';
          include_paths.push_back(path);
        }
        beg = end + 1;
        end = Prelexer::find_first<PATH_SEP>(beg);
      }

      string path(beg);
      if (!path.empty()) {
        if (*path.rbegin() != '/') path += '/';
        include_paths.push_back(path);
      }
    }
  }

  void Context::collect_include_paths(const char* paths_array[])
  {
    const size_t wd_len = 1024;
    char wd[wd_len];
    include_paths.push_back(getcwd(wd, wd_len));
    if (*include_paths.back().rbegin() != '/') include_paths.back() += '/';

    if (paths_array) {
      for (size_t i = 0; paths_array[i]; ++i) {
        string path(paths_array[i]);
        if (!path.empty()) {
          if (*path.rbegin() != '/') path += '/';
          include_paths.push_back(path);
        }
      }
    }
  }

  Context::~Context()
  {
    for (size_t i = 0; i < source_refs.size(); ++i) {
      delete[] source_refs[i];
    }
    delete[] image_path;
    new_Node.free();
  }