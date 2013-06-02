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
#include "file.hpp"

#ifndef SASS_PRELEXER
#include "prelexer.hpp"
#endif

namespace Sass {
  using namespace Constants;
  using std::cerr;
  using std::endl;

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

  void Context::add_file(string path)
  {
    if (style_sheets.count(path)) return;
    using namespace File;
    char* contents = 0;
    for (size_t i = 0, S = include_paths.size(); i < S; ++i) {
      string full_path(join_paths(include_paths[i], path));
      contents = resolve_and_load(full_path);
      if (contents) {
        sources.push_back(contents);
        queue.push_back(pair<string, char*>(path, contents));
        style_sheets[path] = 0;
        return;
      }
    }
    // TODO: throw an error if we get here
  }

  Context::~Context()
  {
    for (size_t i = 0; i < sources.size(); ++i) delete[] sources[i];
  }
}