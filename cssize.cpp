#include "cssize.hpp"
#include "to_string.hpp"

#include <iostream>
#include <typeinfo>

#ifndef SASS_CONTEXT
#include "context.hpp"
#endif

namespace Sass {

  Cssize::Cssize(Context& ctx, Env* env)
  : ctx(ctx),
    env(env),
    block_stack(vector<Block*>()),
    p_stack(vector<Statement*>())
  {  }

  Statement* Cssize::parent()
  {
    return p_stack.size() ? p_stack.back() : block_stack.front();
  }

  Statement* Cssize::operator()(Block* b)
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

  Statement* Cssize::operator()(Ruleset* r)
  {
    p_stack.push_back(r);
    Ruleset* rr = new (ctx.mem) Ruleset(r->path(),
                                        r->position(),
                                        r->selector(),
                                        r->block()->perform(this)->block());
    p_stack.pop_back();

    Block* props = new Block(rr->block()->path(), rr->block()->position());
    for (size_t i = 0, L = rr->block()->length(); i < L; i++)
    {
      Statement* s = (*rr->block())[i];
      if (!bubblable(s)) *props << s;
    }

    Block* rules = new Block(rr->block()->path(), rr->block()->position());
    for (size_t i = 0, L = rr->block()->length(); i < L; i++)
    {
      Statement* s = (*rr->block())[i];
      if (bubblable(s)) *rules << s;
    }

    if (props->length())
    {
      Block* bb = new Block(rr->block()->path(), rr->block()->position());
      *bb += props;
      rr->block(bb);

      for (size_t i = 0, L = rules->length(); i < L; i++)
      {
        (*rules)[i]->tabs((*rules)[i]->tabs() + 1);
      }

      rules->unshift(rr);
    }

    rules = debubble(rules)->block();

    if (!(!rules->length() ||
          !bubblable(rules->last()) ||
          parent()->statement_type() == Statement::RULESET))
    {
      rules->last()->group_end(true);
    }

    return rules;
  }

  Statement* Cssize::operator()(Media_Block* m)
  {
    if (parent()->statement_type() == Statement::RULESET)
    { return bubble(m); }

    if (parent()->statement_type() == Statement::MEDIA)
    { return new (ctx.mem) Bubble(m->path(), m->position(), m); }

    p_stack.push_back(m);

    Media_Block* mm = new (ctx.mem) Media_Block(m->path(),
                                                m->position(),
                                                m->media_queries(),
                                                m->block()->perform(this)->block());
    mm->tabs(m->tabs());

    p_stack.pop_back();

    return debubble(mm->block(), mm)->block();
  }

  Statement* Cssize::bubble(Media_Block* m)
  {
    Ruleset* parent = static_cast<Ruleset*>(this->parent());

    Block* bb = new (ctx.mem) Block(parent->block()->path(), parent->block()->position());
    Ruleset* new_rule = new (ctx.mem) Ruleset(parent->path(),
                                              parent->position(),
                                              parent->selector(),
                                              bb);
    new_rule->tabs(parent->tabs());

    for (size_t i = 0, L = m->block()->length(); i < L; ++i) {
      *new_rule->block() << (*m->block())[i];
    }

    Block* wrapper_block = new (ctx.mem) Block(m->block()->path(), m->block()->position());
    *wrapper_block << new_rule;
    Media_Block* mm = new (ctx.mem) Media_Block(m->path(),
                                                m->position(),
                                                m->media_queries(),
                                                wrapper_block,
                                                m->selector());

    Bubble* bubble = new (ctx.mem) Bubble(mm->path(), mm->position(), mm);

    return bubble;
  }

  bool Cssize::bubblable(Statement* s)
  {
    return s->statement_type() == Statement::RULESET || s->bubbles();
  }

  Statement* Cssize::flatten(Statement* s)
  {
    Block* bb = s->block();
    Block* result = new (ctx.mem) Block(bb->path(), bb->position(), 0, bb->is_root());
    for (size_t i = 0, L = bb->length(); i < L; ++i) {
      Statement* ss = (*bb)[i];
      if (ss->block()) {
        ss = flatten(ss);
        for (size_t j = 0, K = ss->block()->length(); j < K; ++j) {
          *result << (*ss->block())[j];
        }
      }
      else {
        *result << ss;
      }
    }
    return result;
  }

  vector<pair<bool, Block*>> Cssize::slice_by_bubble(Statement* b)
  {
    vector<pair<bool, Block*>> results;
    for (size_t i = 0, L = b->block()->length(); i < L; ++i) {
      Statement* value = (*b->block())[i];
      bool key = value->statement_type() == Statement::BUBBLE;

      if (!results.empty() && results.back().first == key)
      {
        Block* wrapper_block = results.back().second;
        *wrapper_block << value;
      }
      else
      {
        Block* wrapper_block = new (ctx.mem) Block(value->path(), value->position());
        *wrapper_block << value;
        results.push_back(make_pair(key, wrapper_block));
      }
    }
    return results;
  }

  Statement* Cssize::debubble(Block* children, Statement* parent)
  {
    Has_Block* previous_parent = 0;
    vector<pair<bool, Block*>> baz = slice_by_bubble(children);
    Block* result = new (ctx.mem) Block(children->path(), children->position());

    for (size_t i = 0, L = baz.size(); i < L; ++i) {
      bool is_bubble = baz[i].first;
      Block* slice = baz[i].second;

      if (!is_bubble) {
        if (!parent) {
          *result << slice;
        }
        else if (previous_parent) {
          *previous_parent->block() += slice;
        }
        else {
          previous_parent = static_cast<Has_Block*>(parent);
          previous_parent->tabs(parent->tabs());

          Has_Block* new_parent = static_cast<Has_Block*>(parent);
          new_parent->block(slice);
          new_parent->tabs(parent->tabs());

          *result << new_parent;
        }
        continue;
      }

      Block* wrapper_block = new (ctx.mem) Block(children->block()->path(),
                                                 children->block()->position(),
                                                 children->block()->length(),
                                                 children->block()->is_root());

      for (size_t j = 0, K = slice->length(); j < K; ++j)
      {
        Statement* ss = 0;
        Bubble* b = static_cast<Bubble*>((*slice)[j]);

        if (!parent ||
            parent->statement_type() != Statement::MEDIA ||
            b->node()->statement_type() != Statement::MEDIA ||
            static_cast<Media_Block*>(b->node())->media_queries() == static_cast<Media_Block*>(parent)->media_queries())
        {
          ss = b->node();
        }
        else
        {
          List* mq = merge_media_queries(static_cast<Media_Block*>(b->node()), static_cast<Media_Block*>(parent));
          static_cast<Media_Block*>(b->node())->media_queries(mq);
          ss = b->node();
        }

        ss->tabs(ss->tabs() + b->tabs());
        ss->group_end(b->group_end());

        if (!ss) continue;

        Block* bb = new (ctx.mem) Block(children->block()->path(),
                                        children->block()->position(),
                                        children->block()->length(),
                                        children->block()->is_root());
        *bb << ss->perform(this);
        Statement* wrapper = flatten(bb);
        *wrapper_block << wrapper;

        if (wrapper->block()->length()) {
          previous_parent = 0;
        }
      }

      if (wrapper_block) {
        *result << flatten(wrapper_block);
      }
    }

    return flatten(result);
  }

  Statement* Cssize::fallback_impl(AST_Node* n)
  {
    return static_cast<Statement*>(n);
  }

  void Cssize::append_block(Block* b)
  {
    Block* current_block = block_stack.back();

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* ith = (*b)[i]->perform(this);
      if (ith && ith->block()) {
        for (size_t j = 0, K = ith->block()->length(); j < K; ++j) {
          *current_block << (*ith->block())[j];
        }
      }
      else if (ith) {
        *current_block << ith;
      }
    }
  }

  List* Cssize::merge_media_queries(Media_Block* m1, Media_Block* m2)
  {
    List* qq = new (ctx.mem) List(m1->media_queries()->path(),
                                  m1->media_queries()->position(),
                                  m1->media_queries()->length(),
                                  List::COMMA);

    for (size_t i = 0, L = m1->media_queries()->length(); i < L; i++) {
      for (size_t j = 0, K = m2->media_queries()->length(); j < K; j++) {
        Media_Query* mq1 = static_cast<Media_Query*>((*m1->media_queries())[i]);
        Media_Query* mq2 = static_cast<Media_Query*>((*m2->media_queries())[j]);
        Media_Query* mq = merge_media_query(mq1, mq2);

        if (mq) *qq << mq;
      }
    }

    return qq;
  }


  Media_Query* Cssize::merge_media_query(Media_Query* mq1, Media_Query* mq2)
  {
    To_String to_string;

    string type;
    string mod;

    string m1 = string(mq1->is_restricted() ? "only" : mq1->is_negated() ? "not" : "");
    string t1 = mq1->media_type() ? mq1->media_type()->perform(&to_string) : "";
    string m2 = string(mq2->is_restricted() ? "only" : mq1->is_negated() ? "not" : "");
    string t2 = mq2->media_type() ? mq2->media_type()->perform(&to_string) : "";


    if (t1.empty()) t1 = t2;
    if (t2.empty()) t2 = t1;

    if ((m1 == "not") ^ (m2 == "not")) {
      if (t1 == t2) {
        return 0;
      }
      type = m1 == "not" ? t2 : t1;
      mod = m1 == "not" ? m2 : m1;
    }
    else if (m1 == "not" && m2 == "not") {
      if (t1 != t2) {
        return 0;
      }
      type = t1;
      mod = "not";
    }
    else if (t1 != t2) {
      return 0;
    } else {
      type = t1;
      mod = m1.empty() ? m2 : m1;
    }

    Media_Query* mm = new (ctx.mem) Media_Query(
      mq1->path(), mq1->position(), 0,
      mq1->length() + mq2->length(), mod == "not", mod == "only"
    );

    if (!type.empty()) {
      mm->media_type(new (ctx.mem) String_Constant(mq1->path(), mq1->position(), type));
    }

    *mm += mq2;
    *mm += mq1;
    return mm;
  }
}
