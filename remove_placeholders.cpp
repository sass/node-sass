#include "remove_placeholders.hpp"
#include "context.hpp"
#include "inspect.hpp"
#include "to_string.hpp"
#include <iostream>

namespace Sass {

    RemovePlaceholders::RemovePlaceholders(Context& ctx)
    : ctx(ctx)
    { }

    void RemovePlaceholders::operator()(Ruleset* r) {

        // Create a new selector group without placeholders
        Selector_List* sl = static_cast<Selector_List*>(r->selector());
        Selector_List* new_sl = new (ctx.mem) Selector_List(sl->path(), sl->position());

        for (size_t i = 0, L = sl->length(); i < L; ++i) {
            if (!(*sl)[i]->has_placeholder()) {
                *new_sl << (*sl)[i];
            }
        }

        // Set the new placeholder selector list
        r->selector(new_sl);

        // Iterate into child blocks
        Block* b = r->block();

        for (size_t i = 0, L = b->length(); i < L; ++i) {
            Statement* stm = (*b)[i];
            stm->perform(this);
        }
    }

    void RemovePlaceholders::operator()(Block* b)
    {
        for (size_t i = 0, L = b->length(); i < L; ++i) {
            (*b)[i]->perform(this);
        }
    }

    void RemovePlaceholders::operator()(Media_Block* m)
    {
        m->block()->perform(this);
    }

    void RemovePlaceholders::operator()(At_Rule* a)
    {
        if (a->block()) a->block()->perform(this);
    }
}
