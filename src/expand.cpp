#include "expand.hpp"
#include "ast.hpp"

#ifndef SASS_TO_STRING
#include "to_string.hpp"
#endif
// #include "apply.hpp"
#include <iostream>
#include <typeinfo>

#ifndef SASS_CONTEXT
#include "context.hpp"
#endif

namespace Sass {

  Expand::Expand(Context& ctx, /*To_String& ts,*/ Env* env)
  : ctx(ctx), /*to_string(ts),*/
    env(env),
    env_stack(vector<Env>()),
    block_stack(vector<Block*>()),
    content_stack(vector<Block*>()),
    property_stack(vector<String*>()),
    selector_stack(vector<Selector*>())
  { }

  Statement* Expand::operator()(Block* b)
  {
    Env new_env;
    new_env.link(*env);
    env = &new_env;
    Block* bb = new (ctx.mem) Block(b->path(), b->line(), b->length(), b->is_root());
    block_stack.push_back(bb);
    // for (size_t i = 0, L = b->length(); i < L; ++i) {
    //  (*bb) << (*b)[i]->perform(this);
    // }
    append_block(b);
    block_stack.pop_back();
    env = env->parent();
    return bb;
  }

  Statement* Expand::operator()(Ruleset* r)
  {
    return new (ctx.mem) Ruleset(r->path(),
                                 r->line(),
                                 r->selector(), // TODO: expand the selector
                                 r->block()->perform(this)->block());
  }

  Statement* Expand::operator()(Propset* p)
  {
    Block* current_block = block_stack.back();
    String_Schema* combined_prop = new (ctx.mem) String_Schema(p->path(), p->line());
    if (!property_stack.empty()) {
      *combined_prop << property_stack.back()
                     << new (ctx.mem) String_Constant(p->path(), p->line(), "-")
                     << p->property_fragment(); // TODO: eval the prop into a string constant
    }
    else {
      *combined_prop << p->property_fragment();
    }
    for (size_t i = 0, S = p->declarations().size(); i < S; ++i) {
      Declaration* dec = static_cast<Declaration*>(p->declarations()[i]->perform(this));
      String_Schema* final_prop = new (ctx.mem) String_Schema(dec->path(), dec->line());
      *final_prop += combined_prop;
      *final_prop << new (ctx.mem) String_Constant(dec->path(), dec->line(), "-");
      *final_prop << dec->property();
      dec->property(final_prop);
      *current_block << dec;
    }
    for (size_t i = 0, S = p->propsets().size(); i < S; ++i) {
      property_stack.push_back(combined_prop);
      p->propsets()[i]->perform(this);
      property_stack.pop_back();
    }
    return p;
  }

  Statement* Expand::operator()(Media_Block* m)
  {
    // TODO -- consider making a new Expander (also eval the media queries)
    return new (ctx.mem) Media_Block(m->path(),
                                     m->line(),
                                     m->media_queries(),
                                     m->block()->perform(this)->block());
  }

  Statement* Expand::operator()(At_Rule* a)
  {
    Block* ab = a->block();
    Block* bb = ab ? ab->perform(this)->block() : 0;
    return new (ctx.mem) At_Rule(a->path(),
                             a->line(),
                             a->keyword(),
                             a->selector(),
                             bb);
  }

  Statement* Expand::operator()(Declaration* d)
  {
    // TODO: eval the property and value
    return new (ctx.mem) Declaration(d->path(),
                                     d->line(),
                                     d->property(),
                                     d->value());
  }

  Statement* Expand::operator()(Assignment* a)
  {
    string var(a->variable());
    if (env->has(var)) {
      if(!a->is_guarded()) (*env)[var] = a->value(); // TODO: eval the value
    }
    else {
      env->current_frame()[var] = a->value();
    }
    return 0;
  }

  Statement* Expand::operator()(Import* i)
  {
    return i; // TODO: eval i->urls()
  }

  Statement* Expand::operator()(Import_Stub* i)
  {
    append_block(ctx.style_sheets[i->file_name()]);
  }

  Statement* Expand::operator()(Comment* c)
  {
    // TODO: eval the text, once we're parsing/storing it as a String_Schema
    return c;
  }

  // Statement* Expand::operator()(If* i)
  // {

  // }

  Statement* Expand::operator()(Definition* d)
  {
    env->current_frame()[d->name() +
                        (d->type() == Definition::MIXIN ? "[m]" : "[f]")] = d;
    // TODO: set the static link of the definition to get lexical scoping
    return 0;
  }

  Statement* Expand::operator()(Mixin_Call* c)
  {
    string full_name(c->name() + "[m]");
    if (!env->has(full_name)) /* TODO: throw an error */ ;
    Definition* def = static_cast<Definition*>((*env)[full_name]);
    // TODO: bind the arguments, etc -- all the hard work
    Block* body = def->block();
    if (c->block()) {
      content_stack.push_back(static_cast<Block*>(c->block()->perform(this)));
    }
    Block* current_block = block_stack.back();
    // for (size_t i = 0, L = body->length(); i < L; ++i) {
    //  *current_block << (*body)[i]->perform(this);
    // }
    append_block(body);
    if (c->block()) {
      content_stack.pop_back();
    }
    return 0;
  }

  Statement* Expand::operator()(Content* c)
  {
    if (content_stack.empty()) /* TODO: raise an error */ ;
    Block* current_block = block_stack.back();
    Block* content = content_stack.back();
    for (size_t i = 0, L = content->length(); i < L; ++i) {
      Statement* ith = (*content)[i];
      if (ith) *current_block << ith;
    }
    return 0;
  }

  inline Statement* Expand::fallback_impl(AST_Node* n)
  {
    String_Constant* msg = new (ctx.mem) String_Constant("", 0, string("`Expand` doesn't handle ") + typeid(*n).name());
    return new (ctx.mem) Warning("", 0, msg);
    // TODO: fallback should call `eval` on Expression nodes
  }

  inline void Expand::append_block(Block* b)
  {
    Block* current_block = block_stack.back();
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* ith = (*b)[i]->perform(this);
      if (ith) *current_block << ith;
    }
  }
}