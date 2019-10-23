// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include <iostream>
#include <typeinfo>

#include "ast.hpp"
#include "expand.hpp"
#include "bind.hpp"
#include "eval.hpp"
#include "backtrace.hpp"
#include "context.hpp"
#include "parser.hpp"
#include "sass_functions.hpp"

namespace Sass {

  // simple endless recursion protection
  const size_t maxRecursion = 500;

  Expand::Expand(Context& ctx, Env* env, SelectorStack* stack, SelectorStack* originals)
  : ctx(ctx),
    traces(ctx.traces),
    eval(Eval(*this)),
    recursions(0),
    in_keyframes(false),
    at_root_without_rule(false),
    old_at_root_without_rule(false),
    env_stack(),
    block_stack(),
    call_stack(),
    selector_stack(),
    originalStack(),
    mediaStack()
  {
    env_stack.push_back(nullptr);
    env_stack.push_back(env);
    block_stack.push_back(nullptr);
    call_stack.push_back({});
    if (stack == NULL) { pushToSelectorStack({}); }
    else {
      for (auto item : *stack) {
        if (item.isNull()) pushToSelectorStack({});
        else pushToSelectorStack(item);
      }
    }
    if (originals == NULL) { pushToOriginalStack({}); }
    else {
      for (auto item : *stack) {
        if (item.isNull()) pushToOriginalStack({});
        else pushToOriginalStack(item);
      }
    }
    mediaStack.push_back({});
  }

  Env* Expand::environment()
  {
    if (env_stack.size() > 0)
      return env_stack.back();
    return 0;
  }

  void Expand::pushNullSelector()
  {
    pushToSelectorStack({});
    pushToOriginalStack({});
  }

  void Expand::popNullSelector()
  {
    popFromOriginalStack();
    popFromSelectorStack();
  }

  SelectorStack Expand::getOriginalStack()
  {
    return originalStack;
  }

  SelectorStack Expand::getSelectorStack()
  {
    return selector_stack;
  }

  SelectorListObj& Expand::selector()
  {
    if (selector_stack.size() > 0) {
      auto& sel = selector_stack.back();
      if (sel.isNull()) return sel;
      return sel;
    }
    // Avoid the need to return copies
    // We always want an empty first item
    selector_stack.push_back({});
    return selector_stack.back();;
  }

  SelectorListObj& Expand::original()
  {
    if (originalStack.size() > 0) {
      auto& sel = originalStack.back();
      if (sel.isNull()) return sel;
      return sel;
    }
    // Avoid the need to return copies
    // We always want an empty first item
    originalStack.push_back({});
    return originalStack.back();
  }

  SelectorListObj Expand::popFromSelectorStack()
  {
    SelectorListObj last = selector_stack.back();
    if (selector_stack.size() > 0)    
      selector_stack.pop_back();
    if (last.isNull()) return {};
    return last;
  }

  void Expand::pushToSelectorStack(SelectorListObj selector)
  {
    selector_stack.push_back(selector);
  }

  SelectorListObj Expand::popFromOriginalStack()
  {
    SelectorListObj last = originalStack.back();
    if (originalStack.size() > 0)
      originalStack.pop_back();
    if (last.isNull()) return {};
    return last;
  }

  void Expand::pushToOriginalStack(SelectorListObj selector)
  {
    originalStack.push_back(selector);
  }

  // blocks create new variable scopes
  Block* Expand::operator()(Block* b)
  {
    // create new local environment
    // set the current env as parent
    Env env(environment());
    // copy the block object (add items later)
    Block_Obj bb = SASS_MEMORY_NEW(Block,
                                b->pstate(),
                                b->length(),
                                b->is_root());
    // setup block and env stack
    this->block_stack.push_back(bb);
    this->env_stack.push_back(&env);
    // operate on block
    // this may throw up!
    this->append_block(b);
    // revert block and env stack
    this->block_stack.pop_back();
    this->env_stack.pop_back();
    // return copy
    return bb.detach();
  }

  Statement* Expand::operator()(Ruleset* r)
  {
    LOCAL_FLAG(old_at_root_without_rule, at_root_without_rule);

    if (in_keyframes) {
      Block* bb = operator()(r->block());
      Keyframe_Rule_Obj k = SASS_MEMORY_NEW(Keyframe_Rule, r->pstate(), bb);
      if (r->schema()) {
        pushNullSelector();
        k->name(eval(r->schema()));
        popNullSelector();
      }
      else if (r->selector()) {
        if (SelectorListObj s = r->selector()) {
          pushNullSelector();
          k->name(eval(s));
          popNullSelector();
        }
      }

      return k.detach();
    }

    if (r->schema()) {
      SelectorListObj sel = eval(r->schema());
      r->selector(sel);
      bool chroot = sel->has_real_parent_ref();
      for (auto complex : sel->elements()) {
        complex->chroots(chroot);
      }

    }

    // reset when leaving scope
    LOCAL_FLAG(at_root_without_rule, false);

    SelectorListObj evaled = eval(r->selector());
    // do not connect parent again
    Env env(environment());
    if (block_stack.back()->is_root()) {
      env_stack.push_back(&env);
    }
    Block_Obj blk;
    pushToSelectorStack(evaled);
    // The copy is needed for parent reference evaluation
    // dart-sass stores it as `originalSelector` member
    pushToOriginalStack(SASS_MEMORY_COPY(evaled));
    ctx.extender.addSelector(evaled, mediaStack.back());
    if (r->block()) blk = operator()(r->block());
    popFromOriginalStack();
    popFromSelectorStack();
    Ruleset* rr = SASS_MEMORY_NEW(Ruleset,
                                  r->pstate(),
                                  evaled,
                                  blk);

    if (block_stack.back()->is_root()) {
      env_stack.pop_back();
    }

    rr->is_root(r->is_root());
    rr->tabs(r->tabs());

    return rr;
  }

  Statement* Expand::operator()(Supports_Block* f)
  {
    Expression_Obj condition = f->condition()->perform(&eval);
    Supports_Block_Obj ff = SASS_MEMORY_NEW(Supports_Block,
                                       f->pstate(),
                                       Cast<Supports_Condition>(condition),
                                       operator()(f->block()));
    return ff.detach();
  }

  std::vector<CssMediaQuery_Obj> Expand::mergeMediaQueries(
    const std::vector<CssMediaQuery_Obj>& lhs,
    const std::vector<CssMediaQuery_Obj>& rhs)
  {
    std::vector<CssMediaQuery_Obj> queries;
    for (CssMediaQuery_Obj query1 : lhs) {
      for (CssMediaQuery_Obj query2 : rhs) {
        CssMediaQuery_Obj result = query1->merge(query2);
        if (result && !result->empty()) {
          queries.push_back(result);
        }
      }
    }
    return queries;
  }

  Statement* Expand::operator()(MediaRule* m)
  {
    Expression_Obj mq = eval(m->schema());
    std::string str_mq(mq->to_css(ctx.c_options));
    char* str = sass_copy_c_string(str_mq.c_str());
    ctx.strings.push_back(str);
    Parser parser(Parser::from_c_str(str, ctx, traces, mq->pstate()));
    // Create a new CSS only representation of the media rule
    CssMediaRuleObj css = SASS_MEMORY_NEW(CssMediaRule, m->pstate(), m->block());
    std::vector<CssMediaQuery_Obj> parsed = parser.parseCssMediaQueries();
    if (mediaStack.size() && mediaStack.back()) {
      auto& parent = mediaStack.back()->elements();
      css->concat(mergeMediaQueries(parent, parsed));
    }
    else {
      css->concat(parsed);
    }
    mediaStack.push_back(css);
    css->block(operator()(m->block()));
    mediaStack.pop_back();
    return css.detach();

  }

  Statement* Expand::operator()(At_Root_Block* a)
  {
    Block_Obj ab = a->block();
    Expression_Obj ae = a->expression();

    if (ae) ae = ae->perform(&eval);
    else ae = SASS_MEMORY_NEW(At_Root_Query, a->pstate());

    LOCAL_FLAG(at_root_without_rule, Cast<At_Root_Query>(ae)->exclude("rule"));
    LOCAL_FLAG(in_keyframes, false);

                                       ;

    Block_Obj bb = ab ? operator()(ab) : NULL;
    At_Root_Block_Obj aa = SASS_MEMORY_NEW(At_Root_Block,
                                        a->pstate(),
                                        bb,
                                        Cast<At_Root_Query>(ae));
    return aa.detach();
  }

  Statement* Expand::operator()(Directive* a)
  {
    LOCAL_FLAG(in_keyframes, a->is_keyframes());
    Block* ab = a->block();
    SelectorList* as = a->selector();
    Expression* av = a->value();
    pushNullSelector();
    if (av) av = av->perform(&eval);
    if (as) as = eval(as);
    popNullSelector();
    Block* bb = ab ? operator()(ab) : NULL;
    Directive* aa = SASS_MEMORY_NEW(Directive,
                                  a->pstate(),
                                  a->keyword(),
                                  as,
                                  bb,
                                  av);
    return aa;
  }

  Statement* Expand::operator()(Declaration* d)
  {
    Block_Obj ab = d->block();
    String_Obj old_p = d->property();
    Expression_Obj prop = old_p->perform(&eval);
    String_Obj new_p = Cast<String>(prop);
    // we might get a color back
    if (!new_p) {
      std::string str(prop->to_string(ctx.c_options));
      new_p = SASS_MEMORY_NEW(String_Constant, old_p->pstate(), str);
    }
    Expression_Obj value = d->value();
    if (value) value = value->perform(&eval);
    Block_Obj bb = ab ? operator()(ab) : NULL;
    if (!bb) {
      if (!value || (value->is_invisible() && !d->is_important())) {
        if (d->is_custom_property()) {
          error("Custom property values may not be empty.", d->value()->pstate(), traces);
        } else {
          return nullptr;
        }
      }
    }
    Declaration* decl = SASS_MEMORY_NEW(Declaration,
                                        d->pstate(),
                                        new_p,
                                        value,
                                        d->is_important(),
                                        d->is_custom_property(),
                                        bb);
    decl->tabs(d->tabs());
    return decl;
  }

  Statement* Expand::operator()(Assignment* a)
  {
    Env* env = environment();
    const std::string& var(a->variable());
    if (a->is_global()) {
      if (a->is_default()) {
        if (env->has_global(var)) {
          Expression_Obj e = Cast<Expression>(env->get_global(var));
          if (!e || e->concrete_type() == Expression::NULL_VAL) {
            env->set_global(var, a->value()->perform(&eval));
          }
        }
        else {
          env->set_global(var, a->value()->perform(&eval));
        }
      }
      else {
        env->set_global(var, a->value()->perform(&eval));
      }
    }
    else if (a->is_default()) {
      if (env->has_lexical(var)) {
        auto cur = env;
        while (cur && cur->is_lexical()) {
          if (cur->has_local(var)) {
            if (AST_Node_Obj node = cur->get_local(var)) {
              Expression_Obj e = Cast<Expression>(node);
              if (!e || e->concrete_type() == Expression::NULL_VAL) {
                cur->set_local(var, a->value()->perform(&eval));
              }
            }
            else {
              throw std::runtime_error("Env not in sync");
            }
            return 0;
          }
          cur = cur->parent();
        }
        throw std::runtime_error("Env not in sync");
      }
      else if (env->has_global(var)) {
        if (AST_Node_Obj node = env->get_global(var)) {
          Expression_Obj e = Cast<Expression>(node);
          if (!e || e->concrete_type() == Expression::NULL_VAL) {
            env->set_global(var, a->value()->perform(&eval));
          }
        }
      }
      else if (env->is_lexical()) {
        env->set_local(var, a->value()->perform(&eval));
      }
      else {
        env->set_local(var, a->value()->perform(&eval));
      }
    }
    else {
      env->set_lexical(var, a->value()->perform(&eval));
    }
    return 0;
  }

  Statement* Expand::operator()(Import* imp)
  {
    Import_Obj result = SASS_MEMORY_NEW(Import, imp->pstate());
    if (imp->import_queries() && imp->import_queries()->size()) {
      Expression_Obj ex = imp->import_queries()->perform(&eval);
      result->import_queries(Cast<List>(ex));
    }
    for ( size_t i = 0, S = imp->urls().size(); i < S; ++i) {
      result->urls().push_back(imp->urls()[i]->perform(&eval));
    }
    // all resources have been dropped for Input_Stubs
    // for ( size_t i = 0, S = imp->incs().size(); i < S; ++i) {}
    return result.detach();
  }

  Statement* Expand::operator()(Import_Stub* i)
  {
    traces.push_back(Backtrace(i->pstate()));
    // get parent node from call stack
    AST_Node_Obj parent = call_stack.back();
    if (Cast<Block>(parent) == NULL) {
      error("Import directives may not be used within control directives or mixins.", i->pstate(), traces);
    }
    // we don't seem to need that actually afterall
    Sass_Import_Entry import = sass_make_import(
      i->imp_path().c_str(),
      i->abs_path().c_str(),
      0, 0
    );
    ctx.import_stack.push_back(import);

    Block_Obj trace_block = SASS_MEMORY_NEW(Block, i->pstate());
    Trace_Obj trace = SASS_MEMORY_NEW(Trace, i->pstate(), i->imp_path(), trace_block, 'i');
    block_stack.back()->append(trace);
    block_stack.push_back(trace_block);

    const std::string& abs_path(i->resource().abs_path);
    append_block(ctx.sheets.at(abs_path).root);
    sass_delete_import(ctx.import_stack.back());
    ctx.import_stack.pop_back();
    block_stack.pop_back();
    traces.pop_back();
    return 0;
  }

  Statement* Expand::operator()(Warning* w)
  {
    // eval handles this too, because warnings may occur in functions
    w->perform(&eval);
    return 0;
  }

  Statement* Expand::operator()(Error* e)
  {
    // eval handles this too, because errors may occur in functions
    e->perform(&eval);
    return 0;
  }

  Statement* Expand::operator()(Debug* d)
  {
    // eval handles this too, because warnings may occur in functions
    d->perform(&eval);
    return 0;
  }

  Statement* Expand::operator()(Comment* c)
  {
    if (ctx.output_style() == COMPRESSED) {
      // comments should not be evaluated in compact
      // https://github.com/sass/libsass/issues/2359
      if (!c->is_important()) return NULL;
    }
    eval.is_in_comment = true;
    Comment* rv = SASS_MEMORY_NEW(Comment, c->pstate(), Cast<String>(c->text()->perform(&eval)), c->is_important());
    eval.is_in_comment = false;
    // TODO: eval the text, once we're parsing/storing it as a String_Schema
    return rv;
  }

  Statement* Expand::operator()(If* i)
  {
    Env env(environment(), true);
    env_stack.push_back(&env);
    call_stack.push_back(i);
    Expression_Obj rv = i->predicate()->perform(&eval);
    if (*rv) {
      append_block(i->block());
    }
    else {
      Block* alt = i->alternative();
      if (alt) append_block(alt);
    }
    call_stack.pop_back();
    env_stack.pop_back();
    return 0;
  }

  // For does not create a new env scope
  // But iteration vars are reset afterwards
  Statement* Expand::operator()(For* f)
  {
    std::string variable(f->variable());
    Expression_Obj low = f->lower_bound()->perform(&eval);
    if (low->concrete_type() != Expression::NUMBER) {
      traces.push_back(Backtrace(low->pstate()));
      throw Exception::TypeMismatch(traces, *low, "integer");
    }
    Expression_Obj high = f->upper_bound()->perform(&eval);
    if (high->concrete_type() != Expression::NUMBER) {
      traces.push_back(Backtrace(high->pstate()));
      throw Exception::TypeMismatch(traces, *high, "integer");
    }
    Number_Obj sass_start = Cast<Number>(low);
    Number_Obj sass_end = Cast<Number>(high);
    // check if units are valid for sequence
    if (sass_start->unit() != sass_end->unit()) {
      std::stringstream msg; msg << "Incompatible units: '"
        << sass_start->unit() << "' and '"
        << sass_end->unit() << "'.";
      error(msg.str(), low->pstate(), traces);
    }
    double start = sass_start->value();
    double end = sass_end->value();
    // only create iterator once in this environment
    Env env(environment(), true);
    env_stack.push_back(&env);
    call_stack.push_back(f);
    Block* body = f->block();
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start;
           i < end;
           ++i) {
        Number_Obj it = SASS_MEMORY_NEW(Number, low->pstate(), i, sass_end->unit());
        env.set_local(variable, it);
        append_block(body);
      }
    } else {
      if (f->is_inclusive()) --end;
      for (double i = start;
           i > end;
           --i) {
        Number_Obj it = SASS_MEMORY_NEW(Number, low->pstate(), i, sass_end->unit());
        env.set_local(variable, it);
        append_block(body);
      }
    }
    call_stack.pop_back();
    env_stack.pop_back();
    return 0;
  }

  // Eval does not create a new env scope
  // But iteration vars are reset afterwards
  Statement* Expand::operator()(Each* e)
  {
    std::vector<std::string> variables(e->variables());
    Expression_Obj expr = e->list()->perform(&eval);
    List_Obj list;
    Map_Obj map;
    if (expr->concrete_type() == Expression::MAP) {
      map = Cast<Map>(expr);
    }
    else if (SelectorList * ls = Cast<SelectorList>(expr)) {
      Expression_Obj rv = Listize::perform(ls);
      list = Cast<List>(rv);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = SASS_MEMORY_NEW(List, expr->pstate(), 1, SASS_COMMA);
      list->append(expr);
    }
    else {
      list = Cast<List>(expr);
    }
    // remember variables and then reset them
    Env env(environment(), true);
    env_stack.push_back(&env);
    call_stack.push_back(e);
    Block* body = e->block();

    if (map) {
      for (auto key : map->keys()) {
        Expression_Obj k = key->perform(&eval);
        Expression_Obj v = map->at(key)->perform(&eval);

        if (variables.size() == 1) {
          List_Obj variable = SASS_MEMORY_NEW(List, map->pstate(), 2, SASS_SPACE);
          variable->append(k);
          variable->append(v);
          env.set_local(variables[0], variable);
        } else {
          env.set_local(variables[0], k);
          env.set_local(variables[1], v);
        }
        append_block(body);
      }
    }
    else {
      // bool arglist = list->is_arglist();
      if (list->length() == 1 && Cast<SelectorList>(list)) {
        list = Cast<List>(list);
      }
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        Expression_Obj item = list->at(i);
        // unwrap value if the expression is an argument
        if (Argument_Obj arg = Cast<Argument>(item)) item = arg->value();
        // check if we got passed a list of args (investigate)
        if (List_Obj scalars = Cast<List>(item)) {
          if (variables.size() == 1) {
            List_Obj var = scalars;
            // if (arglist) var = (*scalars)[0];
            env.set_local(variables[0], var);
          } else {
            for (size_t j = 0, K = variables.size(); j < K; ++j) {
              Expression_Obj res = j >= scalars->length()
                ? SASS_MEMORY_NEW(Null, expr->pstate())
                : (*scalars)[j]->perform(&eval);
              env.set_local(variables[j], res);
            }
          }
        } else {
          if (variables.size() > 0) {
            env.set_local(variables.at(0), item);
            for (size_t j = 1, K = variables.size(); j < K; ++j) {
              Expression_Obj res = SASS_MEMORY_NEW(Null, expr->pstate());
              env.set_local(variables[j], res);
            }
          }
        }
        append_block(body);
      }
    }
    call_stack.pop_back();
    env_stack.pop_back();
    return 0;
  }

  Statement* Expand::operator()(While* w)
  {
    Expression_Obj pred = w->predicate();
    Block* body = w->block();
    Env env(environment(), true);
    env_stack.push_back(&env);
    call_stack.push_back(w);
    Expression_Obj cond = pred->perform(&eval);
    while (!cond->is_false()) {
      append_block(body);
      cond = pred->perform(&eval);
    }
    call_stack.pop_back();
    env_stack.pop_back();
    return 0;
  }

  Statement* Expand::operator()(Return* r)
  {
    error("@return may only be used within a function", r->pstate(), traces);
    return 0;
  }

  Statement* Expand::operator()(ExtendRule* e)
  {

    // evaluate schema first
    if (e->schema()) {
      e->selector(eval(e->schema()));
      e->isOptional(e->selector()->is_optional());
    }
    // evaluate the selector
    e->selector(eval(e->selector()));

    if (e->selector()) {

      for (auto complex : e->selector()->elements()) {

        if (complex->length() != 1) {
          error("complex selectors may not be extended.", complex->pstate(), traces);
        }

        if (const CompoundSelector* compound = complex->first()->getCompound()) {

          if (compound->length() != 1) {

            std::stringstream sels; bool addComma = false;
            sels << "Compound selectors may no longer be extended.\n";
            sels << "Consider `@extend ";
            for (auto sel : compound->elements()) {
              if (addComma) sels << ", ";
              sels << sel->to_sass();
              addComma = true;
            }
            sels << "` instead.\n";
            sels << "See http://bit.ly/ExtendCompound for details.";

            warning(sels.str(), compound->pstate());

            // Make this an error once deprecation is over
            for (SimpleSelectorObj simple : compound->elements()) {
              // Pass every selector we ever see to extender (to make them findable for extend)
              ctx.extender.addExtension(selector(), simple, mediaStack.back(), e->isOptional());
            }

          }
          else {
            // Pass every selector we ever see to extender (to make them findable for extend)
            ctx.extender.addExtension(selector(), compound->first(), mediaStack.back(), e->isOptional());
          }

        }
        else {
          error("complex selectors may not be extended.", complex->pstate(), traces);
        }
      }
    }

    return nullptr;

  }

  Statement* Expand::operator()(Definition* d)
  {
    Env* env = environment();
    Definition_Obj dd = SASS_MEMORY_COPY(d);
    env->local_frame()[d->name() +
                        (d->type() == Definition::MIXIN ? "[m]" : "[f]")] = dd;

    if (d->type() == Definition::FUNCTION && (
      Prelexer::calc_fn_call(d->name().c_str()) ||
      d->name() == "element"    ||
      d->name() == "expression" ||
      d->name() == "url"
    )) {
      deprecated(
        "Naming a function \"" + d->name() + "\" is disallowed and will be an error in future versions of Sass.",
        "This name conflicts with an existing CSS function with special parse rules.",
        false, d->pstate()
      );
    }

    // set the static link so we can have lexical scoping
    dd->environment(env);
    return 0;
  }

  Statement* Expand::operator()(Mixin_Call* c)
  {

    if (recursions > maxRecursion) {
      throw Exception::StackError(traces, *c);
    }

    recursions ++;

    Env* env = environment();
    std::string full_name(c->name() + "[m]");
    if (!env->has(full_name)) {
      error("no mixin named " + c->name(), c->pstate(), traces);
    }
    Definition_Obj def = Cast<Definition>((*env)[full_name]);
    Block_Obj body = def->block();
    Parameters_Obj params = def->parameters();

    if (c->block() && c->name() != "@content" && !body->has_content()) {
      error("Mixin \"" + c->name() + "\" does not accept a content block.", c->pstate(), traces);
    }
    Expression_Obj rv = c->arguments()->perform(&eval);
    Arguments_Obj args = Cast<Arguments>(rv);
    std::string msg(", in mixin `" + c->name() + "`");
    traces.push_back(Backtrace(c->pstate(), msg));
    ctx.callee_stack.push_back({
      c->name().c_str(),
      c->pstate().path,
      c->pstate().line + 1,
      c->pstate().column + 1,
      SASS_CALLEE_MIXIN,
      { env }
    });

    Env new_env(def->environment());
    env_stack.push_back(&new_env);
    if (c->block()) {
      Parameters_Obj params = c->block_parameters();
      if (!params) params = SASS_MEMORY_NEW(Parameters, c->pstate());
      // represent mixin content blocks as thunks/closures
      Definition_Obj thunk = SASS_MEMORY_NEW(Definition,
                                          c->pstate(),
                                          "@content",
                                          params,
                                          c->block(),
                                          Definition::MIXIN);
      thunk->environment(env);
      new_env.local_frame()["@content[m]"] = thunk;
    }

    bind(std::string("Mixin"), c->name(), params, args, &new_env, &eval, traces);

    Block_Obj trace_block = SASS_MEMORY_NEW(Block, c->pstate());
    Trace_Obj trace = SASS_MEMORY_NEW(Trace, c->pstate(), c->name(), trace_block);

    env->set_global("is_in_mixin", bool_true);
    if (Block* pr = block_stack.back()) {
      trace_block->is_root(pr->is_root());
    }
    block_stack.push_back(trace_block);
    for (auto bb : body->elements()) {
      if (Ruleset* r = Cast<Ruleset>(bb)) {
        r->is_root(trace_block->is_root());
      }
      Statement_Obj ith = bb->perform(this);
      if (ith) trace->block()->append(ith);
    }
    block_stack.pop_back();
    env->del_global("is_in_mixin");

    ctx.callee_stack.pop_back();
    env_stack.pop_back();
    traces.pop_back();

    recursions --;
    return trace.detach();
  }

  Statement* Expand::operator()(Content* c)
  {
    Env* env = environment();
    // convert @content directives into mixin calls to the underlying thunk
    if (!env->has("@content[m]")) return 0;
    Arguments_Obj args = c->arguments();
    if (!args) args = SASS_MEMORY_NEW(Arguments, c->pstate());

    Mixin_Call_Obj call = SASS_MEMORY_NEW(Mixin_Call,
                                       c->pstate(),
                                       "@content",
                                       args);

    Trace_Obj trace = Cast<Trace>(call->perform(this));
    return trace.detach();
  }

  // process and add to last block on stack
  inline void Expand::append_block(Block* b)
  {
    if (b->is_root()) call_stack.push_back(b);
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = b->at(i);
      Statement_Obj ith = stm->perform(this);
      if (ith) block_stack.back()->append(ith);
    }
    if (b->is_root()) call_stack.pop_back();
  }

}
