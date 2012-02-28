#include <vector>
#include "node.hpp"

namespace Sass {
  struct Document {
    using std::vector;
    
    char* source;
    unsigned int position;
    vector<Node> statements;
  };
}