#ifndef SASS_FILE_H
#define SASS_FILE_H

#include <string>
#include <vector>

namespace Sass {

  class Context;

  struct Sass_Queued {
    std::string abs_path;
    std::string load_path;
    const char* source;
  public:
    Sass_Queued(const std::string& load_path, const std::string& abs_path, const char* source);
  };

  namespace File {

    // return the current directory
    // always with forward slashes
    std::string get_cwd();

    // test if path exists and is a file
    bool file_exists(const std::string& file);

    // return if given path is absolute
    // works with *nix and windows paths
    bool is_absolute_path(const std::string& path);

    // return only the directory part of path
    std::string dir_name(const std::string& path);

    // return only the filename part of path
    std::string base_name(const std::string&);

    // do a locigal clean up of the path
    // no physical check on the filesystem
    std::string make_canonical_path (std::string path);

    // join two path segments cleanly together
    // but only if right side is not absolute yet
    std::string join_paths(std::string root, std::string name);

    // create an absolute path by resolving relative paths with cwd
    std::string make_absolute_path(const std::string& path, const std::string& cwd = ".");

    // create a path that is relative to the given base directory
    // path and base will first be resolved against cwd to make them absolute
    std::string resolve_relative_path(const std::string& path, const std::string& base, const std::string& cwd = ".");

    // try to find/resolve the filename
    std::vector<Sass_Queued> resolve_file(const std::string& root, const std::string& file);

    // helper function to resolve a filename
    std::string find_file(const std::string& file, const std::vector<std::string> paths);
    // inc paths can be directly passed from C code
    std::string find_file(const std::string& file, const char** paths);

    // try to load the given filename
    // returned memory must be freed
    // will auto convert .sass files
    char* read_file(const std::string& file);

  }
}

#endif
