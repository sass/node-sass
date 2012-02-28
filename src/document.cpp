#include <cstdio>
#include "document.hpp"

namespace Sass {
  using namespace Prelexer;

  Document::Document(char* _path, char* _source) {
    path = _path;
    if (!_source) {
      std::FILE *f;
      // TO DO: CHECK f AGAINST NULL/0
      f = std::fopen(path, "rb");
      std::fseek(f, 0, SEEK_END);
      int len = std::ftell(f);
      std::rewind(f);
      // TO DO: WRAP THE new[] IN A TRY/CATCH BLOCK
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
    last_munch_succeeded = false;
  }
  Document::~Document() {
    delete [] source;
  }

}