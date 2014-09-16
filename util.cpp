#include "util.hpp"

namespace Sass {
  namespace Util {
    using std::string;
    
    string normalize_underscores(const string& str) {
      string normalized = str;
      for(size_t i = 0, L = normalized.length(); i < L; ++i) {
        if(normalized[i] == '_') {
          normalized[i] = '-';
        }
      }
      return normalized;
    }
    
    // A Block is considered printable if a Declaration or At_Rule is found anywhere under its recursive structure
    bool containsAnyPrintableStatements(Block* b) {
      
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          return true;
        }
        else if (dynamic_cast<Has_Block*>(stm) && containsAnyPrintableStatements(((Has_Block*)stm)->block())) {
          return true;
        }
      }
      
      return false;
    }
    
  }
}
