#include "expand.hpp"
#include "bind.hpp"
#include "eval.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"

#include <iostream>
#include <typeinfo>

#ifndef SASS_CONTEXT
#include "context.hpp"
#endif

#include "parser.hpp"

namespace Sass {

  Expand::Expand(Context& ctx, Eval* eval, Contextualize* contextualize, Env* env, Backtrace* bt)
  : ctx(ctx),
    eval(eval),
    contextualize(contextualize),
    env(env),
    block_stack(vector<Block*>()),
    property_stack(vector<String*>()),
    selector_stack(vector<Selector*>()),
    backtrace(bt)
  { selector_stack.push_back(0); }

  Statement* Expand::operator()(Block* b)
  {
    Env new_env;
    new_env.link(*env);
    env = &new_env;
    Block* bb = new (ctx.mem) Block(b->path(), b->position(), b->length(), b->is_root());
    block_stack.push_back(bb);
    append_block(b);
    block_stack.pop_back();
    env = env->parent();
    return bb;
  }

  Statement* Expand::operator()(Ruleset* r)
  {
    To_String to_string;
    // if (selector_stack.back()) cerr << "expanding " << selector_stack.back()->perform(&to_string) << " and " << r->selector()->perform(&to_string) << endl;
    Selector* sel_ctx = r->selector()->perform(contextualize->with(selector_stack.back(), env, backtrace));
    // re-parse in order to restructure parent nodes correctly
    sel_ctx = Parser::from_c_str((sel_ctx->perform(&to_string) + ";").c_str(), ctx, r->selector()->path(), r->selector()->position()).parse_selector_group();
    selector_stack.push_back(sel_ctx);
    Ruleset* rr = new (ctx.mem) Ruleset(r->path(),
                                        r->position(),
                                        sel_ctx,
                                        r->block()->perform(this)->block());
    selector_stack.pop_back();
    return rr;
  }

  Statement* Expand::operator()(Propset* p)
  {
    property_stack.push_back(p->property_fragment());
    Block* expanded_block = p->block()->perform(this)->block();

    Block* current_block = block_stack.back();
    for (size_t i = 0, L = expanded_block->length(); i < L; ++i) {
      Statement* stm = (*expanded_block)[i];
      if (typeid(*stm) == typeid(Declaration)) {
        Declaration* dec = static_cast<Declaration*>(stm);
        String_Schema* combined_prop = new (ctx.mem) String_Schema(p->path(), p->position());
        if (!property_stack.empty()) {
          *combined_prop << property_stack.back()
                         << new (ctx.mem) String_Constant(p->path(), p->position(), "-")
                         << dec->property(); // TODO: eval the prop into a string constant
        }
        else {
          *combined_prop << dec->property();
        }
        dec->property(combined_prop);
        *current_block << dec;
      }
      else {
        error("contents of namespaced properties must result in style declarations only", stm->path(), stm->position(), backtrace);
      }
    }

    property_stack.pop_back();

    return 0;
  }

  Statement* Expand::operator()(Feature_Block* f)
  {
    Expression* feature_queries = f->feature_queries()->perform(eval->with(env, backtrace));
    Feature_Block* ff = new (ctx.mem) Feature_Block(f->path(),
                                                    f->position(),
                                                    static_cast<Feature_Query*>(feature_queries),
                                                    f->block()->perform(this)->block());
    ff->selector(selector_stack.back());
    return ff;
  }

  Statement* Expand::operator()(Media_Block* m)
  {
    Expression* media_queries = m->media_queries()->perform(eval->with(env, backtrace));
    Media_Block* mm = new (ctx.mem) Media_Block(m->path(),
                                                m->position(),
                                                static_cast<List*>(media_queries),
                                                m->block()->perform(this)->block());
    mm->selector(selector_stack.back());
    return mm;
  }

  Statement* Expand::operator()(At_Rule* a)
  {
    Block* ab = a->block();
    selector_stack.push_back(0);
    Selector* as = a->selector();
    Expression* av = a->value();
    if (as) as = as->perform(contextualize->with(0, env, backtrace));
    else if (av) av = av->perform(eval->with(env, backtrace));
    Block* bb = ab ? ab->perform(this)->block() : 0;
    At_Rule* aa = new (ctx.mem) At_Rule(a->path(),
                                        a->position(),
                                        a->keyword(),
                                        as,
                                        bb);
    if (av) aa->value(av);
    selector_stack.pop_back();
    return aa;
  }

  Statement* Expand::operator()(Declaration* d)
  {
    String* old_p = d->property();
    String* new_p = static_cast<String*>(old_p->perform(eval->with(env, backtrace)));
    Expression* value = d->value()->perform(eval->with(env, backtrace));

    if (value->is_invisible() && !d->is_important()) return 0;

    return new (ctx.mem) Declaration(d->path(),
                                     d->position(),
                                     new_p,
                                     value,
                                     d->is_important());
  }

  Statement* Expand::operator()(Assignment* a)
  {
    string var(a->variable());
    if (env->has(var)) {
      Expression* v = static_cast<Expression*>((*env)[var]);
      if (!a->is_guarded() || v->concrete_type() == Expression::NULL_VAL) (*env)[var] = a->value()->perform(eval->with(env, backtrace));
    }
    else {
      env->current_frame()[var] = a->value()->perform(eval->with(env, backtrace));
    }
    return 0;
  }

  Statement* Expand::operator()(Import* imp)
  {
    Import* result = new (ctx.mem) Import(imp->path(), imp->position());
    for ( size_t i = 0, S = imp->urls().size(); i < S; ++i) {
      result->urls().push_back(imp->urls()[i]->perform(eval->with(env, backtrace)));
    }
    return result;
  }

  Statement* Expand::operator()(Import_Stub* i)
  {
    append_block(ctx.style_sheets[i->file_name()]);
    return 0;
  }

  Statement* Expand::operator()(Warning* w)
  {
    // eval handles this too, because warnings may occur in functions
    w->perform(eval->with(env, backtrace));
    return 0;
  }

  Statement* Expand::operator()(Error* e)
  {
    // eval handles this too, because errors may occur in functions
    e->perform(eval->with(env, backtrace));
    return 0;
  }

  Statement* Expand::operator()(Debug* d)
  {
    // eval handles this too, because warnings may occur in functions
    d->perform(eval->with(env, backtrace));
    return 0;
  }

  Statement* Expand::operator()(Comment* c)
  {
    // TODO: eval the text, once we're parsing/storing it as a String_Schema
    return new (ctx.mem) Comment(c->path(), c->position(), static_cast<String*>(c->text()->perform(eval->with(env, backtrace))));
  }

  Statement* Expand::operator()(If* i)
  {
    if (*i->predicate()->perform(eval->with(env, backtrace))) {
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
    Expression* low = f->lower_bound()->perform(eval->with(env, backtrace));
    if (low->concrete_type() != Expression::NUMBER) {
      error("lower bound of `@for` directive must be numeric", low->path(), low->position(), backtrace);
    }
    Expression* high = f->upper_bound()->perform(eval->with(env, backtrace));
    if (high->concrete_type() != Expression::NUMBER) {
      error("upper bound of `@for` directive must be numeric", high->path(), high->position(), backtrace);
    }
    double start = static_cast<Number*>(low)->value();
    double end = static_cast<Number*>(high)->value();
    Env new_env;
    new_env[variable] = new (ctx.mem) Number(low->path(), low->position(), start);
    new_env.link(env);
    env = &new_env;
    Block* body = f->block();
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start;
           i < end;
           (*env)[variable] = new (ctx.mem) Number(low->path(), low->position(), ++i)) {
        append_block(body);
      }
    } else {
      if (f->is_inclusive()) --end;
      for (double i = start;
           i > end;
           (*env)[variable] = new (ctx.mem) Number(low->path(), low->position(), --i)) {
        append_block(body);
      }
    }
    env = new_env.parent();
    return 0;
  }

  Statement* Expand::operator()(Each* e)
  {
    vector<string> variables(e->variables());
    Expression* expr = e->list()->perform(eval->with(env, backtrace));
    List* list = 0;
    Map* map = 0;
    if (expr->concrete_type() == Expression::MAP) {
      map = static_cast<Map*>(expr);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = new (ctx.mem) List(expr->path(), expr->position(), 1, List::COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    Env new_env;
    for (size_t i = 0, L = variables.size(); i < L; ++i) new_env[variables[i]] = 0;
    new_env.link(env);
    env = &new_env;
    Block* body = e->block();

    if (map) {
      for (auto key : map->keys()) {
        Expression* k = key->perform(eval->with(env, backtrace));
        Expression* v = map->at(key)->perform(eval->with(env, backtrace));

        if (variables.size() == 1) {
          List* variable = new (ctx.mem) List(map->path(), map->position(), 2, List::SPACE);
          *variable << k;
          *variable << v;
          (*env)[variables[0]] = variable;
        } else {
          (*env)[variables[0]] = k;
          (*env)[variables[1]] = v;
        }
        append_block(body);
      }
    }
    else {
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        List* variable = 0;
        if ((*list)[i]->concrete_type() != Expression::LIST  || variables.size() == 1) {
          variable = new (ctx.mem) List((*list)[i]->path(), (*list)[i]->position(), 1, List::COMMA);
          *variable << (*list)[i];
        }
        else {
          variable = static_cast<List*>((*list)[i]);
        }
        for (size_t j = 0, K = variables.size(); j < K; ++j) {
          if (j < variable->length()) {
            (*env)[variables[j]] = (*variable)[j]->perform(eval->with(env, backtrace));
          }
          else {
            (*env)[variables[j]] = new (ctx.mem) Null(expr->path(), expr->position());
          }
        }
        append_block(body);
      }
    }
    env = new_env.parent();
    return 0;
  }

  Statement* Expand::operator()(While* w)
  {
    Expression* pred = w->predicate();
    Block* body = w->block();
    while (*pred->perform(eval->with(env, backtrace))) {
      append_block(body);
    }
    return 0;
  }

  Statement* Expand::operator()(Return* r)
  {
    error("@return may only be used within a function", r->path(), r->position(), backtrace);
    return 0;
  }

  Statement* Expand::operator()(Extension* e)
  {
    To_String to_string;
    Selector_List* extender = static_cast<Selector_List*>(selector_stack.back());
    if (!extender) return 0;
    Selector_List* extendee = static_cast<Selector_List*>(e->selector()->perform(contextualize->with(0, env, backtrace)));
    if (extendee->length() != 1) {
      error("selector groups may not be extended", extendee->path(), extendee->position(), backtrace);
    }
    Complex_Selector* c = (*extendee)[0];
    if (!c->head() || c->tail()) {
      error("nested selectors may not be extended", c->path(), c->position(), backtrace);
    }
    Compound_Selector* s = c->head();

    // // need to convert the compound selector into a by-value data structure
    // vector<string> target_vec;
    // for (size_t i = 0, L = s->length(); i < L; ++i)
    // { target_vec.push_back((*s)[i]->perform(&to_string)); }

    for (size_t i = 0, L = extender->length(); i < L; ++i) {
      // let's test this out
      // cerr << "REGISTERING EXTENSION REQUEST: " << (*extender)[i]->perform(&to_string) << " <- " << s->perform(&to_string) << endl;
      ctx.subset_map.put(s->to_str_vec(), make_pair((*extender)[i], s));
    }
    return 0;
  }

  Statement* Expand::operator()(Definition* d)
  {
    Definition* dd = new (ctx.mem) Definition(*d);
    env->current_frame()[d->name() +
                        (d->type() == Definition::MIXIN ? "[m]" : "[f]")] = dd;
    // set the static link so we can have lexical scoping
    dd->environment(env);
    return 0;
  }

  Statement* Expand::operator()(Mixin_Call* c)
  {
    string full_name(c->name() + "[m]");
    if (!env->has(full_name)) {
      error("no mixin named " + c->name(), c->path(), c->position(), backtrace);
    }
    Definition* def = static_cast<Definition*>((*env)[full_name]);
    Block* body = def->block();
    Parameters* params = def->parameters();
    Arguments* args = static_cast<Arguments*>(c->arguments()
                                               ->perform(eval->with(env, backtrace)));
    Backtrace here(backtrace, c->path(), c->position(), ", in mixin `" + c->name() + "`");
    backtrace = &here;
    Env new_env;
    new_env.link(def->environment());
    if (c->block()) {
      // represent mixin content blocks as thunks/closures
      Definition* thunk = new (ctx.mem) Definition(c->path(),
                                                   c->position(),
                                                   "@content",
                                                   new (ctx.mem) Parameters(c->path(), c->position()),
                                                   c->block(),
                                                   Definition::MIXIN);
      thunk->environment(env);
      new_env.current_frame()["@content[m]"] = thunk;
    }
    bind("mixin " + c->name(), params, args, ctx, &new_env, eval);
    Env* old_env = env;
    env = &new_env;
    append_block(body);
    env = old_env;
    backtrace = here.parent;
    return 0;
  }

  Statement* Expand::operator()(Content* c)
  {
    // convert @content directives into mixin calls to the underlying thunk
    if (!env->has("@content[m]")) return 0;
    Mixin_Call* call = new (ctx.mem) Mixin_Call(c->path(),
                                                c->position(),
                                                "@content",
                                                new (ctx.mem) Arguments(c->path(), c->position()));
    return call->perform(this);
  }

  inline Statement* Expand::fallback_impl(AST_Node* n)
  {
    error("unknown internal error; please contact the LibSass maintainers", n->path(), n->position(), backtrace);
    String_Constant* msg = new (ctx.mem) String_Constant("", Position(), string("`Expand` doesn't handle ") + typeid(*n).name());
    return new (ctx.mem) Warning("", Position(), msg);
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
