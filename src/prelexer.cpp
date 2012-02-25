#include "prelexer.hpp"

namespace Sass {
  namespace Prelexer {
    
    // Matches zero characters (always succeeds without consuming input).
    char *epsilon(char *src) {
      return src;
    }
    
    // Matches the empty string.
    char *empty(char *src) {
      return *src ? 0 : src;
    }

  }
}