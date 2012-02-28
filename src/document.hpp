#include <vector>
#include "node.hpp"

namespace Sass {
  using std::vector;

  struct Document {
    char* path;
    char* source;
    unsigned int position;
    unsigned int line_number;
    vector<Node> statements;
    
    Document(char* _path, char* _source = 0);
  };
}