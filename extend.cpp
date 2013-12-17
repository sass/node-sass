#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include <iostream>

namespace Sass {

  Extend::Extend(Context& ctx, multimap<Compound_Selector, Complex_Selector*>& extensions, Subset_Map<string, pair<Complex_Selector*, Compound_Selector*> >& ssm, Backtrace* bt)
  : ctx(ctx), extensions(extensions), subset_map(ssm), backtrace(bt)
  { }

  void Extend::operator()(Block* b)
  {
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      (*b)[i]->perform(this);
    }
  }

  void Extend::operator()(Ruleset* r)
  {
    Selector_List* sg = static_cast<Selector_List*>(r->selector());
    Selector_List* ng = 0;
    bool extended = false;
    if (sg->has_placeholder()) {
      // To_String to_string;
      Compound_Selector* placeholder = new (ctx.mem) Compound_Selector(sg->path(), sg->line(), 1);
      *placeholder << sg->find_placeholder();
      // cerr << "placeholder: " << placeholder->perform(&to_string) << endl;
      // if the placeholder needs to be subbed
      if (extensions.count(*placeholder)) {
        // cerr << "need to sub " << placeholder->perform(&to_string) << " " << extensions.count(*placeholder) << " times" << endl;
        // perform each substitution and append it to the selector group of the ruleset
        ng = new (ctx.mem) Selector_List(sg->path(), sg->line(), extensions.count(*placeholder));
        for (multimap<Compound_Selector, Complex_Selector*>::iterator extender = extensions.lower_bound(*placeholder), E = extensions.upper_bound(*placeholder);
             extender != E;
             ++extender) {
          // cerr << "performing a substitution: " << placeholder->perform(&to_string) << " -> " << extender->second->perform(&to_string) << endl;
          Contextualize sub_plc(ctx, 0, 0, backtrace, placeholder, extender->second);
          Selector_List* subbed = static_cast<Selector_List*>(sg->perform(&sub_plc));
          // if (subbed) cerr << "subbed: " << subbed->perform(&to_string) << endl;
          *ng += subbed;
          extended = true;
        }
        ng->has_placeholder(false);
      }
      // otherwise prevent it from rendering
      else {
        // r->selector(0);
      }
    }
    else {
      To_String to_string;
      ng = new (ctx.mem) Selector_List(sg->path(), sg->line(), sg->length());
      // for each selector in the group
      for (size_t i = 0, L = sg->length(); i < L; ++i) {
        Complex_Selector* sel = (*sg)[i];
        *ng << sel;
        // if it's supposed to be extended
        Compound_Selector* sel_base = sel->base();
        if (sel_base && extensions.count(*sel_base)) {
          // extend it wrt each of its extenders
          for (multimap<Compound_Selector, Complex_Selector*>::iterator extender = extensions.lower_bound(*sel_base), E = extensions.upper_bound(*sel_base);
               extender != E;
               ++extender) {
            *ng += generate_extension(sel, extender->second);
            extended = true;
          }
        }
      }
    }
    if (extended) r->selector(ng);

    // let's try the new stuff here; eventually it should replace the preceding
    set<Compound_Selector> seen;
    Selector_List* new_list = new (ctx.mem) Selector_List(sg->path(), sg->line());
    for (size_t i = 0, L = sg->length(); i < L; ++i)
    {
      *new_list += extend_complex((*sg)[i], seen);
    }

    r->block()->perform(this);
  }

  void Extend::operator()(Media_Block* m)
  {
    m->block()->perform(this);
  }

  void Extend::operator()(At_Rule* a)
  {
    if (a->block()) a->block()->perform(this);
  }

  Selector_List* Extend::generate_extension(Complex_Selector* extendee, Complex_Selector* extender)
  {
    To_String to_string;
    Selector_List* new_group = new (ctx.mem) Selector_List(extendee->path(), extendee->line());
    Complex_Selector* extendee_context = extendee->context(ctx);
    Complex_Selector* extender_context = extender->context(ctx);
    if (extendee_context && extender_context) {
      Complex_Selector* base = new (ctx.mem) Complex_Selector(new_group->path(), new_group->line(), Complex_Selector::ANCESTOR_OF, extender->base(), 0);
      extendee_context->innermost()->tail(extender);
      *new_group << extendee_context;
      // make another one so we don't erroneously share tails
      extendee_context = extendee->context(ctx);
      extendee_context->innermost()->tail(base);
      extender_context->innermost()->tail(extendee_context);
      *new_group << extender_context;
    }
    else if (extendee_context) {
      extendee_context->innermost()->tail(extender);
      *new_group << extendee_context;
    }
    else if (extender_context) {
      *new_group << extender;
    }
    else {
      *new_group << extender;
    }
    return new_group;
  }

  Selector_List* Extend::extend_complex(Complex_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    cerr << "EXTENDING COMPLEX: " << sel->perform(&to_string) << endl;
    Selector_List* choices = new (ctx.mem) Selector_List(sel->path(), sel->line());
    Selector_List* extended;

    Compound_Selector* h = sel->head();
    Complex_Selector* t = sel->tail();
    if (h && !h->is_empty_reference())
    {
      Selector_List* extended = extend_compound(h, seen);
      // TODO: check extended for something or other
      *choices += extended;
    }
    while(t)
    {
      h = t->head();
      t = t->tail();
      if (h && !h->is_empty_reference())
      {
        Selector_List* extended = extend_compound(h, seen);
        // TODO: check extended for something or other
        *choices += extended;
      }
    }

    return choices;
  }

  Selector_List* Extend::extend_compound(Compound_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    cerr << "EXTENDING COMPOUND: " << sel->perform(&to_string) << endl;
    return new (ctx.mem) Selector_List(sel->path(), sel->line());
  }

}