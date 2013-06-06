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
#include "parser.hpp"
#include "file.hpp"
#include "inspector.hpp"
#include "copy_c_str.hpp"

#ifndef SASS_PRELEXER
#include "prelexer.hpp"
#endif

namespace Sass {
  using namespace Constants;
  using std::cerr;
  using std::endl;

  Context::Context(Context::Data initializers)
  : mem(Memory_Manager<AST_Node*>()),
    sources         (vector<const char*>()),
    source_c_str    (initializers.source_c_str()),
    include_paths   (initializers.include_paths()),
    queue           (vector<pair<string, const char*> >()),
    style_sheets    (map<string, Block*>()),
    image_path      (initializers.image_path()),
    source_comments (initializers.source_comments()),
    source_maps     (initializers.source_maps()),
    output_style    (initializers.output_style())
  {
    collect_include_paths(initializers.include_paths_c_str());
    collect_include_paths(initializers.include_paths_array());

    string entry_point = initializers.entry_point();
    if (!entry_point.empty()) add_file(entry_point);
  }

  Context::~Context()
  { for (size_t i = 0; i < sources.size(); ++i) delete[] sources[i]; }

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

  string Context::add_file(string path)
  {
    using namespace File;
    char* contents = 0;
    for (size_t i = 0, S = include_paths.size(); i < S; ++i) {
      string full_path(join_paths(include_paths[i], path));
      if (style_sheets.count(full_path)) return full_path;
      contents = resolve_and_load(full_path);
      if (contents) {
        sources.push_back(contents);
        queue.push_back(pair<string, const char*>(full_path, contents));
        style_sheets[full_path] = 0;
        return full_path;
      }
    }
    return string();
  }

  char* Context::compile_file()
  {
    Block* root;
    for (size_t i = 0; i < queue.size(); ++i) {
      Parser p(Parser::from_c_str(queue[i].second, *this, queue[i].first));
      Block* ast = p.parse();
      if (i == 0) root = ast;
      style_sheets[queue[i].first] = ast;
    }
    Inspector* inspect = new Inspector();
    root->perform(inspect);
    char* result = copy_c_str(inspect->get_buffer().c_str());
    delete inspect;
    return result;
  }

  char* Context::compile_string()
  {
    if (!source_c_str) return 0;
    queue.clear();
    queue.push_back(pair<string, const char*>("source string", source_c_str));
    return compile_file();
  }
}