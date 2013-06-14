#include "expand.hpp"
#include "ast.hpp"
#include "bind.hpp"
#include "eval.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"

#include <iostream>
#include <typeinfo>

#ifndef SASS_CONTEXT
#include "context.hpp"
#endif

namespace Sass {

  Expand::Expand(Context& ctx, Eval* eval, Contextualize* contextualize, Env* env)
  : ctx(ctx),
    eval(eval),
    contextualize(contextualize),
    env(env),
    block_stack(vector<Block*>()),
    content_stack(vector<Block*>()),
    property_stack(vector<String*>()),
    selector_stack(vector<Selector*>())
  { selector_stack.push_back(0); }

  Statement* Expand::operator()(Block* b)
  {
    Env new_env;
    new_env.link(*env);
    env = &new_env;
    Block* bb = new (ctx.mem) Block(b->path(), b->line(), b->length(), b->is_root());
    block_stack.push_back(bb);
    append_block(b);
    block_stack.pop_back();
    env = env->parent();
    return bb;
  }

  Statement* Expand::operator()(Ruleset* r)
  {
    To_String to_string;
    Selector* sel_ctx = r->selector()->perform(contextualize->with(selector_stack.back(), env));
    selector_stack.push_back(sel_ctx);
    Ruleset* rr = new (ctx.mem) Ruleset(r->path(),
                                        r->line(),
                                        sel_ctx,
                                        r->block()->perform(this)->block());
    selector_stack.pop_back();
    return rr;
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
      dec->property(static_cast<String*>(final_prop->perform(eval->with(env))));
      *current_block << dec;
    }
    for (size_t i = 0, S = p->propsets().size(); i < S; ++i) {
      property_stack.push_back(combined_prop);
      p->propsets()[i]->perform(this);
      property_stack.pop_back();
    }
    return 0;
  }

  Statement* Expand::operator()(Media_Block* m)
  {
    Expression* media_queries = m->media_queries()->perform(eval->with(env));
    return new (ctx.mem) Media_Block(m->path(),
                                     m->line(),
                                     static_cast<List*>(media_queries),
                                     m->block()->perform(this)->block());
  }

  Statement* Expand::operator()(At_Rule* a)
  {
    Block* ab = a->block();
    selector_stack.push_back(0);
    Block* bb = ab ? ab->perform(this)->block() : 0;
    At_Rule* aa = new (ctx.mem) At_Rule(a->path(),
                                        a->line(),
                                        a->keyword(),
                                        a->selector(),
                                        bb);
    selector_stack.pop_back();
    return aa;
  }

  Statement* Expand::operator()(Declaration* d)
  {
    String* old_p = d->property();
    String* new_p = static_cast<String*>(old_p->perform(eval->with(env)));
    return new (ctx.mem) Declaration(d->path(),
                                     d->line(),
                                     new_p,
                                     d->value()->perform(eval->with(env)));
  }

  Statement* Expand::operator()(Assignment* a)
  {
    string var(a->variable());
    if (env->has(var)) {
      if(!a->is_guarded()) (*env)[var] = a->value()->perform(eval->with(env));
    }
    else {
      env->current_frame()[var] = a->value()->perform(eval->with(env));
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
    return 0;
  }

  Statement* Expand::operator()(Warning* w)
  {
    // eval handles this too, because warnings may occur in functions
    w->perform(eval->with(env));
    return 0;
  }

  Statement* Expand::operator()(Comment* c)
  {
    // TODO: eval the text, once we're parsing/storing it as a String_Schema
    return c;
  }

  Statement* Expand::operator()(If* i)
  {
    if (*i->predicate()->perform(eval->with(env))) {
      append_block(i->consequent());
    }
    else {
      Block* alt = i->alternative();
      if (alt) append_block(alt);
    }
    return 0;
  }

  Statement* Expand::operator()(For* f)
  {
    string variable(f->variable());
    Expression* low = f->lower_bound()->perform(eval->with(env));
    if (low->concrete_type() != Expression::NUMBER) {
      error("lower bound of `@for` directive must be numeric", low->path(), low->line());
    }
    Expression* high = f->upper_bound()->perform(eval->with(env));
    if (high->concrete_type() != Expression::NUMBER) {
      error("upper bound of `@for` directive must be numeric", high->path(), high->line());
    }
    double lo = static_cast<Number*>(low)->value();
    double hi = static_cast<Number*>(high)->value();
    if (f->is_inclusive()) ++hi;
    Env new_env;
    new_env[variable] = new (ctx.mem) Number(low->path(), low->line(), lo);
    new_env.link(env);
    env = &new_env;
    Block* body = f->block();
    for (size_t i = lo;
         i < hi;
         (*env)[variable] = new (ctx.mem) Number(low->path(), low->line(), ++i)) {
      append_block(body);
    }
    env = new_env.parent();
    return 0;
  }

  Statement* Expand::operator()(Each* e)
  {
    string variable(e->variable());
    Expression* expr = e->list()->perform(eval->with(env));
    List* list = 0;
    if (expr->concrete_type() != Expression::LIST) {
      list = new (ctx.mem) List(expr->path(), expr->line(), 1, List::COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    Env new_env;
    new_env[variable] = 0;
    new_env.link(env);
    env = &new_env;
    Block* body = e->block();
    for (size_t i = 0, L = list->length(); i < L; ++i) {
      (*env)[variable] = (*list)[i];
      append_block(body);
    }
    env = new_env.parent();
    return 0;
  }

  Statement* Expand::operator()(While* w)
  {
    Expression* pred = w->predicate();
    Block* body = w->block();
    while (*pred->perform(eval->with(env))) {
      append_block(body);
    }
    return 0;
  }

  Statement* Expand::operator()(Return* r)
  {
    error("@return may only be used within a function", r->path(), r->line());
    return 0;
  }

  Statement* Expand::operator()(Extend* e)
  {
    // TODO: implement this, obviously
    return 0;
  }

  Statement* Expand::operator()(Definition* d)
  {
    env->current_frame()[d->name() +
                        (d->type() == Definition::MIXIN ? "[m]" : "[f]")] = d;
    // evaluate the default args
    Parameters* params = d->parameters();
    for (size_t i = 0, L = params->length(); i < L; ++i) {
      Parameter* param = (*params)[i];
      Expression* dflt = param->default_value();
      if (dflt) param->default_value(dflt->perform(eval->with(env)));
    }
    // set the static link so we can have lexical scoping
    d->environment(env);
    return 0;
  }

  Statement* Expand::operator()(Mixin_Call* c)
  {
    string full_name(c->name() + "[m]");
    if (!env->has(full_name)) {
      error("no mixin named " + c->name(), c->path(), c->line());
    }
    Definition* def = static_cast<Definition*>((*env)[full_name]);
    Block* body = def->block();
    if (c->block()) {
      content_stack.push_back(static_cast<Block*>(c->block()->perform(this)));
    }
    Block* current_block = block_stack.back();
    Parameters* params = def->parameters();
    Arguments* args = static_cast<Arguments*>(c->arguments()
                                               ->perform(eval->with(env)));
    Env new_env;
    bind("mixin " + c->name(), params, args, ctx, &new_env);
    new_env.link(def->environment());
    Env* old_env = env;
    env = &new_env;
    append_block(body);
    env = old_env;
    if (c->block()) {
      content_stack.pop_back();
    }
    return 0;
  }

  Statement* Expand::operator()(Content* c)
  {
    if (content_stack.empty()) return 0;
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
    error("internal error; please contact the LibSass maintainers", n->path(), n->line());
    String_Constant* msg = new (ctx.mem) String_Constant("", 0, string("`Expand` doesn't handle ") + typeid(*n).name());
    return new (ctx.mem) Warning("", 0, msg);
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