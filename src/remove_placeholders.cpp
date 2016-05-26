#include "sass.hpp"
#include "remove_placeholders.hpp"
#include "context.hpp"
#include "inspect.hpp"
#include <iostream>

namespace Sass {

    Remove_Placeholders::Remove_Placeholders(Context& ctx)
    : ctx(ctx)
    { }

    void Remove_Placeholders::operator()(Block* b) {
        for (size_t i = 0, L = b->length(); i < L; ++i) {
            (*b)[i]->perform(this);
        }
    }

    CommaSequence_Selector* Remove_Placeholders::remove_placeholders(CommaSequence_Selector* sl)
    {
      CommaSequence_Selector* new_sl = SASS_MEMORY_NEW(ctx.mem, CommaSequence_Selector, sl->pstate());

      for (size_t i = 0, L = sl->length(); i < L; ++i) {
          if (!(*sl)[i]->contains_placeholder()) {
              *new_sl << (*sl)[i];
          }
      }

      return new_sl;

    }


    void Remove_Placeholders::operator()(Ruleset* r) {
        // Create a new selector group without placeholders
        CommaSequence_Selector* sl = static_cast<CommaSequence_Selector*>(r->selector());

        if (sl) {
          // Set the new placeholder selector list
          r->selector(remove_placeholders(sl));
          // Remove placeholders in wrapped selectors
          for (Sequence_Selector* cs : *sl) {
            while (cs) {
              if (cs->head()) {
                for (Simple_Selector* ss : *cs->head()) {
                  if (Wrapped_Selector* ws = dynamic_cast<Wrapped_Selector*>(ss)) {
                    if (CommaSequence_Selector* sl = dynamic_cast<CommaSequence_Selector*>(ws->selector())) {
                      CommaSequence_Selector* clean = remove_placeholders(sl);
                      // also clean superflous parent selectors
                      // probably not really the correct place
                      clean->remove_parent_selectors();
                      ws->selector(clean);
                    }
                  }
                }
              }
              cs = cs->tail();
            }
          }
        }

        // Iterate into child blocks
        Block* b = r->block();

        for (size_t i = 0, L = b->length(); i < L; ++i) {
            if ((*b)[i]) (*b)[i]->perform(this);
        }
    }

    void Remove_Placeholders::operator()(Media_Block* m) {
        operator()(m->block());
    }
    void Remove_Placeholders::operator()(Supports_Block* m) {
        operator()(m->block());
    }

    void Remove_Placeholders::operator()(Directive* a) {
        if (a->block()) a->block()->perform(this);
    }

}
