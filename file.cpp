#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#else
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <sys/stat.h>
#include "file.hpp"
#include "context.hpp"
#include "utf8_string.hpp"
#include "sass2scss.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef FS_CASE_SENSITIVE
#ifdef _WIN32
#define FS_CASE_SENSITIVE 0
#else
#define FS_CASE_SENSITIVE 1
#endif
#endif

namespace Sass {
  namespace File {
    using namespace std;

    string get_cwd()
    {
      const size_t wd_len = 1024;
      char wd[wd_len];
      string cwd = getcwd(wd, wd_len);
#ifdef _WIN32
      //convert backslashes to forward slashes
      replace(cwd.begin(), cwd.end(), '\\', '/');
#endif
      if (cwd[cwd.length() - 1] != '/') cwd += '/';
      return cwd;
    }

    // no physical check on filesystem
    // only a logical cleanup of a path
    string make_canonical_path (string path)
    {

      // declarations
      size_t pos;

      #ifdef _WIN32
        //convert backslashes to forward slashes
        replace(path.begin(), path.end(), '\\', '/');
      #endif

      pos = 0; // remove all self references inside the path string
      while((pos = path.find("/./", pos)) != string::npos) path.erase(pos, 2);

      pos = 0; // remove all leading and trailing self references
      while(path.length() > 1 && path.substr(0, 2) == "./") path.erase(0, 2);
      while((pos = path.length()) > 1 && path.substr(pos - 2) == "/.") path.erase(pos - 2);

      pos = 0; // collapse multiple delimiters into a single one
      while((pos = path.find("//", pos)) != string::npos) path.erase(pos, 1);

      return path;

    }

    size_t find_last_folder_separator(const string& path, size_t limit = string::npos)
    {
      size_t pos = string::npos;
      size_t pos_p = path.find_last_of('/', limit);
      #ifdef _WIN32
      size_t pos_w = path.find_last_of('\\', limit);
      #else
      size_t pos_w = string::npos;
      #endif
      if (pos_p != string::npos && pos_w != string::npos) {
        pos = max(pos_p, pos_w);
      }
      else if (pos_p != string::npos) {
        pos = pos_p;
      }
      else {
        pos = pos_w;
      }
      return pos;
    }

    string base_name(string path)
    {
      size_t pos = find_last_folder_separator(path);
      if (pos == string::npos) return path;
      else                     return path.substr(pos+1);
    }

    string dir_name(string path)
    {
      size_t pos = find_last_folder_separator(path);
      if (pos == string::npos) return "";
      else                     return path.substr(0, pos+1);
    }

    string join_paths(string l, string r)
    {
      if (l.empty()) return r;
      if (r.empty()) return l;
      if (is_absolute_path(r)) return r;

      if (l[l.length()-1] != '/') l += '/';

      while ((r.length() > 3) && ((r.substr(0, 3) == "../") || (r.substr(0, 3)) == "..\\")) {
        r = r.substr(3);
        size_t pos = find_last_folder_separator(l, l.length() - 2);
        l = l.substr(0, pos == string::npos ? pos : pos + 1);
      }

      return l + r;
    }

    bool is_absolute_path(const string& path)
    {
      if (path[0] == '/') return true;
      // TODO: UN-HACKIFY THIS
      #ifdef _WIN32
      if (path.length() >= 2 && isalpha(path[0]) && path[1] == ':') return true;
      #endif
      return false;
    }

    string make_absolute_path(const string& path, const string& cwd)
    {
      return make_canonical_path((is_absolute_path(path) ? path : join_paths(cwd, path)));
    }

    string resolve_relative_path(const string& uri, const string& base, const string& cwd)
    {

      string absolute_uri = make_absolute_path(uri, cwd);
      string absolute_base = make_absolute_path(base, cwd);

      string stripped_uri = "";
      string stripped_base = "";

      size_t index = 0;
      size_t minSize = min(absolute_uri.size(), absolute_base.size());
      for (size_t i = 0; i < minSize; ++i) {
        #ifdef FS_CASE_SENSITIVE
          if (absolute_uri[i] != absolute_base[i]) break;
        #else
          // compare the charactes in a case insensitive manner
          // windows fs is only case insensitive in ascii ranges
          if (tolower(absolute_uri[i]) != tolower(absolute_base[i])) break;
        #endif
        if (absolute_uri[i] == '/') index = i + 1;
      }
      for (size_t i = index; i < absolute_uri.size(); ++i) {
        stripped_uri += absolute_uri[i];
      }
      for (size_t i = index; i < absolute_base.size(); ++i) {
        stripped_base += absolute_base[i];
      }

      size_t left = 0;
      size_t directories = 0;
      for (size_t right = 0; right < stripped_base.size(); ++right) {
        if (stripped_base[right] == '/') {
          if (stripped_base.substr(left, 2) != "..") {
            ++directories;
          }
          else if (directories > 1) {
            --directories;
          }
          else {
            directories = 0;
          }
          left = right + 1;
        }
      }

      string result = "";
      for (size_t i = 0; i < directories; ++i) {
        result += "../";
      }
      result += stripped_uri;

      return result;
    }

    char* resolve_and_load(string path, string& real_path)
    {
      // Resolution order for ambiguous imports:
      // (1) filename as given
      // (2) underscore + given
      // (3) underscore + given + extension
      // (4) given + extension
      char* contents = 0;
      real_path = path;
      // if the file isn't found with the given filename ...
      if (!(contents = read_file(real_path))) {
        string dir(dir_name(path));
        string base(base_name(path));
        string _base("_" + base);
        real_path = dir + _base;
        // if the file isn't found with '_' + filename ...
        if (!(contents = read_file(real_path))) {
          string _base_scss(_base + ".scss");
          real_path = dir + _base_scss;
          // if the file isn't found with '_' + filename + ".scss" ...
          if (!(contents = read_file(real_path))) {
            string _base_sass(_base + ".sass");
            real_path = dir + _base_sass;
            // if the file isn't found with '_' + filename + ".sass" ...
            if (!(contents = read_file(real_path))) {
              string base_scss(base + ".scss");
              real_path = dir + base_scss;
              // if the file isn't found with filename + ".scss" ...
              if (!(contents = read_file(real_path))) {
                string base_sass(base + ".sass");
                real_path = dir + base_sass;
                // if the file isn't found with filename + ".sass" ...
                if (!(contents = read_file(real_path))) {
                  // default back to scss version
                  real_path = dir + base_scss;
                }
              }
            }
          }
        }
      }
#ifdef _WIN32
      // convert Windows backslashes to URL forward slashes
      replace(real_path.begin(), real_path.end(), '\\', '/');
#endif
      return contents;
    }

    char* read_file(string path)
    {
#ifdef _WIN32
      BYTE* pBuffer;
      DWORD dwBytes;
      // windows unicode filepaths are encoded in utf16
      wstring wpath = UTF_8::convert_to_utf16(path);
      HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hFile == INVALID_HANDLE_VALUE) return 0;
      DWORD dwFileLength = GetFileSize(hFile, NULL);
      if (dwFileLength == INVALID_FILE_SIZE) return 0;
      pBuffer = new BYTE[dwFileLength + 1];
      ReadFile(hFile, pBuffer, dwFileLength, &dwBytes, NULL);
      pBuffer[dwFileLength] = '\0';
      CloseHandle(hFile);
      // just convert from unsigned char*
      char* contents = (char*) pBuffer;
#else
      struct stat st;
      if (stat(path.c_str(), &st) == -1 || S_ISDIR(st.st_mode)) return 0;
      ifstream file(path.c_str(), ios::in | ios::binary | ios::ate);
      char* contents = 0;
      if (file.is_open()) {
        size_t size = file.tellg();
        contents = new char[size + 1]; // extra byte for the null char
        file.seekg(0, ios::beg);
        file.read(contents, size);
        contents[size] = '\0';
        file.close();
      }
#endif
      string extension;
      if (path.length() > 5) {
        extension = path.substr(path.length() - 5, 5);
      }
      for(size_t i=0; i<extension.size();++i)
        extension[i] = tolower(extension[i]);
      if (extension == ".sass" && contents != 0) {
        char * converted = sass2scss(contents, SASS2SCSS_PRETTIFY_1);
        delete[] contents; // free the indented contents
        return converted; // should be freed by caller
      } else {
        return contents;
      }
    }

  }
}
