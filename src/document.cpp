#include <cstdio>
#include "document.hpp"

namespace Sass {
  Document::Document(char* _path, char* _source) {
    path = _path;
    if (!_source) {
      std::FILE *f;
      f = std::fopen(path, "rb");
      // if (!f) {
      //   printf("ERROR: could not open file %s", path);
      //   abort();
      // }
      std::fseek(f, 0, SEEK_END);
      int len = std::ftell(f);
      std::rewind(f);
      source = new char[len + 1];
      std::fread(source, sizeof(char), len, f);
      source[len] = '\0';
      std::fclose(f);
    }
    else {
      source = _source;
    }
    position = source;
    line_number = 1;
  }
  Document::~Document() {
    delete [] source;
  }
}