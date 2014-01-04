#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "paths.hpp"
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
      Compound_Selector* placeholder = new (ctx.mem) Compound_Selector(sg->path(), sg->position(), 1);
      *placeholder << sg->find_placeholder();
      // cerr << "placeholder: " << placeholder->perform(&to_string) << endl;
      // if the placeholder needs to be subbed
      if (extensions.count(*placeholder)) {
        // cerr << "need to sub " << placeholder->perform(&to_string) << " " << extensions.count(*placeholder) << " times" << endl;
        // perform each substitution and append it to the selector group of the ruleset
        ng = new (ctx.mem) Selector_List(sg->path(), sg->position(), extensions.count(*placeholder));
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
      ng = new (ctx.mem) Selector_List(sg->path(), sg->position(), sg->length());
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
    // Selector_List* new_list = new (ctx.mem) Selector_List(sg->path(), sg->position());
    To_String to_string;
    for (size_t i = 0, L = sg->length(); i < L; ++i)
    {
      // /* *new_list += */ extend_complex((*sg)[i], seen);
      cerr << "checking " << (*sg)[i]->perform(&to_string) << endl;
      extend_complex((*sg)[i], seen);
      // cerr << "skipping for now!" << endl;
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
    Selector_List* new_group = new (ctx.mem) Selector_List(extendee->path(), extendee->position());
    Complex_Selector* extendee_context = extendee->context(ctx);
    Complex_Selector* extender_context = extender->context(ctx);
    if (extendee_context && extender_context) {
      Complex_Selector* base = new (ctx.mem) Complex_Selector(new_group->path(), new_group->position(), Complex_Selector::ANCESTOR_OF, extender->base(), 0);
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
    else {
      *new_group << extender;
    }
    return new_group;
  }

  Selector_List* Extend::extend_complex(Complex_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    cerr << "EXTENDING COMPLEX: " << sel->perform(&to_string) << endl;
    // vector<Selector_List*> choices; // 
    Selector_List* extended = new (ctx.mem) Selector_List(sel->path(), sel->position());

    Compound_Selector* h = sel->head();
    Complex_Selector* t = sel->tail();
    if (h && !h->is_empty_reference())
    {
      // Selector_List* extended = extend_compound(h, seen);
      *extended += extend_compound(h, seen);
      bool found = false;
      for (size_t i = 0, L = extended->length(); i < L; ++i)
      {
        if ((*extended)[i]->is_superselector_of(h))
        { found = true; break; }
      }
      if (!found)
      {
        *extended << new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, h, 0);
      }
      // choices.push_back(extended);
    }
    while(t)
    {
      h = t->head();
      t = t->tail();
      if (h && !h->is_empty_reference())
      {
        // Selector_List* extended = extend_compound(h, seen);
        *extended += extend_compound(h, seen);
        bool found = false;
        for (size_t i = 0, L = extended->length(); i < L; ++i)
        {
          if ((*extended)[i]->is_superselector_of(h))
          { found = true; break; }
        }
        if (!found)
        {
          *extended << new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, h, 0);
        }
        // choices.push_back(extended);
      }
    }
    return extended;
    // cerr << "CHOICES:" << endl;
    // for (size_t i = 0, L = choices.size(); i < L; ++i)
    // {
    //   cerr << choices[i]->perform(&to_string) << endl;
    // }

    // vector<vector<Complex_Selector*> > cs;
    // for (size_t i = 0, S = choices.size(); i < S; ++i)
    // {
    //   cs.push_back(choices[i]->elements());
    // }
    // vector<vector<Complex_Selector*> > ps = paths(cs);
    // cerr << "PATHS:" << endl;
    // for (size_t i = 0, S = ps.size(); i < S; ++i)
    // {
    //   for (size_t j = 0, T = ps[i].size(); j < T; ++j)
    //   {
    //     cerr << ps[i][j]->perform(&to_string) << ", ";
    //   }
    //   cerr << endl;
    // }
    // vector<Selector_List*> new_choices;
    // for (size_t i = 0, S = ps.size(); i < S; ++i)
    // {
    //   Selector_List* new_list = new (ctx.mem) Selector_List(sel->path(), sel->position());
    //   for (size_t j = 0, T = ps[i].size(); j < T; ++j)
    //   {
    //     *new_list << ps[i][j];
    //   }
    //   new_choices.push_back(new_list);
    // }
    // return new_choices;
  }

  Selector_List* Extend::extend_compound(Compound_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    cerr << "EXTEND_COMPOUND: " << sel->perform(&to_string) << endl;
    Selector_List* results = new (ctx.mem) Selector_List(sel->path(), sel->position());

    // TODO: Do we need to group the results by extender?
    vector<pair<Complex_Selector*, Compound_Selector*> > entries = subset_map.get_v(sel->to_str_vec());

    for (size_t i = 0, S = entries.size(); i < S; ++i)
    {
      if (seen.count(*entries[i].second)) continue;
      // cerr << "COMPOUND: " << sel->perform(&to_string) << " KEYS TO " << entries[i].first->perform(&to_string) << " AND " << entries[i].second->perform(&to_string) << endl;
      Compound_Selector* diff = sel->minus(entries[i].second, ctx);
      Compound_Selector* last = entries[i].first->base();
      if (!last) last = new (ctx.mem) Compound_Selector(sel->path(), sel->position());
      cerr << sel->perform(&to_string) << " - " << entries[i].second->perform(&to_string) << " = " << diff->perform(&to_string) << endl;
      // cerr << "LAST: " << last->perform(&to_string) << endl;
      Compound_Selector* unif;
      if (last->length() == 0) unif = diff;
      else if (diff->length() == 0) unif = last;
      else unif = last->unify_with(diff, ctx);
      // if (unif) cerr << "UNIFIED: " << unif->perform(&to_string) << endl;
      if (!unif || unif->length() == 0) continue;
      Complex_Selector* cplx = entries[i].first->clone(ctx);
      Complex_Selector* new_innermost = new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, unif, 0);
      cplx->set_innermost(new_innermost, cplx->clear_innermost());
      cerr << "FULL UNIFIED: " << cplx->perform(&to_string) << endl;
      // set<Compound_Selector> seen2 = seen;
      // seen2.insert(*entries[i].second);
      // cerr << "RECURSIVELY CALLING EXTEND_COMPLEX ON " << cplx->perform(&to_string) << endl;
      // vector<Selector_List*> ex2 = extend_complex(cplx, seen2);
      // for (size_t j = 0, T = ex2.size(); j < T; ++j)
      // {
      //   *results += ex2[i];
      // }
    }

    cerr << "RESULTS: " << results->perform(&to_string) << endl;
    return results;
  }

}