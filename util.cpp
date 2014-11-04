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
      if (r == NULL) {
        return false;
      }

      Block* b = r->block();

      bool hasSelectors = static_cast<Selector_List*>(r->selector())->length() > 0;

      if (!hasSelectors) {
      	return false;
      }

      bool hasDeclarations = false;
      bool hasPrintableChildBlocks = false;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (dynamic_cast<Has_Block*>(stm)) {
        	Block* pChildBlock = ((Has_Block*)stm)->block();
          if (isPrintable(pChildBlock)) {
            hasPrintableChildBlocks = true;
          }
        } else {
        	hasDeclarations = true;
        }

        if (hasDeclarations || hasPrintableChildBlocks) {
        	return true;
        }
      }

      return false;
    }

    bool isPrintable(Feature_Block* f) {
      if (f == NULL) {
        return false;
      }

      Block* b = f->block();

      bool hasSelectors = f->selector() && static_cast<Selector_List*>(f->selector())->length() > 0;

      bool hasDeclarations = false;
      bool hasPrintableChildBlocks = false;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable() && f->selector() != NULL && !hasSelectors) {
          // If a statement isn't hoistable, the selectors apply to it. If there are no selectors (a selector list of length 0),
          // then those statements aren't considered printable. That means there was a placeholder that was removed. If the selector
          // is NULL, then that means there was never a wrapping selector and it is printable (think of a top level media block with
          // a declaration in it).
        }
        else if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          hasDeclarations = true;
        }
        else if (dynamic_cast<Has_Block*>(stm)) {
          Block* pChildBlock = ((Has_Block*)stm)->block();
          if (isPrintable(pChildBlock)) {
            hasPrintableChildBlocks = true;
          }
        }

        if (hasDeclarations || hasPrintableChildBlocks) {
          return true;
        }
      }

      return false;
    }

    bool isPrintable(Media_Block* m) {
      if (m == NULL) {
        return false;
      }

      Block* b = m->block();

      bool hasSelectors = m->selector() && static_cast<Selector_List*>(m->selector())->length() > 0;

      bool hasDeclarations = false;
      bool hasPrintableChildBlocks = false;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable() && m->selector() != NULL && !hasSelectors) {
        	// If a statement isn't hoistable, the selectors apply to it. If there are no selectors (a selector list of length 0),
          // then those statements aren't considered printable. That means there was a placeholder that was removed. If the selector
          // is NULL, then that means there was never a wrapping selector and it is printable (think of a top level media block with
          // a declaration in it).
        }
        else if (typeid(*stm) == typeid(Declaration) || typeid(*stm) == typeid(At_Rule)) {
          hasDeclarations = true;
        }
        else if (dynamic_cast<Has_Block*>(stm)) {
        	Block* pChildBlock = ((Has_Block*)stm)->block();
          if (isPrintable(pChildBlock)) {
            hasPrintableChildBlocks = true;
          }
        }

				if (hasDeclarations || hasPrintableChildBlocks) {
        	return true;
        }
      }

      return false;
    }

     bool isPrintable(Block* b) {
       if (b == NULL) {
         return false;
       }

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
         else if (typeid(*stm) == typeid(Feature_Block)) {
           Feature_Block* f = (Feature_Block*) stm;
           if (isPrintable(f)) {
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
