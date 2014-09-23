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

    bool isPrintable(Ruleset* r) {

      Block* b = r->block();
      
      bool hasSelectors = r->selector()->length() > 0;

      bool hasDeclarations = false;
      bool hasPrintableChildBlocks = false;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          hasDeclarations = true;
        }
        else if (dynamic_cast<Has_Block*>(stm)) {
          if (isPrintable((Has_Block*)stm)->block()) {
            hasPrintableChildBlocks = true;
          }
        }
        // TODO: we can short circuit if both bools are true
      }
      
//      if (hasSelectors) {
//        return hasDeclarations || hasPrintableChildBlocks;
//      }
//      else {
//        return hasPrintableChildBlocks;
//      }
      
      return (hasSelectors && (hasDeclarations || hasPrintableChildBlocks));
    }

    bool isPrintable(Media_Block* r) {
  
      Block* b = r->block();

      bool hasSelectors = r->selector()->length() > 0;
      
      bool hasDeclarations = false;
      bool hasPrintableChildBlocks = false;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          hasDeclarations = true;
        }
        else if (dynamic_cast<Has_Block*>(stm)) {
          if (isPrintable((Has_Block*)stm)->block()) {
            hasPrintableChildBlocks = true;
          }
        }
        // TODO: we can short circuit if both bools are true
      }
      
      //      if (hasSelectors) {
      //        return hasDeclarations || hasPrintableChildBlocks;
      //      }
      //      else {
      //        return hasPrintableChildBlocks;
      //      }
      
      return (hasSelectors && (hasDeclarations || hasPrintableChildBlocks));
    }

    bool isPrintable(Block* b) {

      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          return true;
        }
        else if (typeid(*stm) == typeid(Ruleset)) {
          Ruleset* r = (Ruleset*) stm;
          if (isPrintable(r)) {
            return true;
          }
        }
        else if (typeid(*stm) == typeid(Media_Block)) {
          Media_Block* m = (Media_Block*) stm;
          if (isPrintable(m)) {
            return true;
          }
        }
        else if (dynamic_cast<Has_Block*>(stm) && isPrintable(((Has_Block*)stm)->block())) {
          return true;
        }
      }
      
      return false;
    }
    
  }
}
