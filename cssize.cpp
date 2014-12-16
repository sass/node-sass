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

  Statement* Cssize::operator()(Media_Block* m)
  {
    if (parent()->statement_type() == Statement::MEDIA)
    { return new (ctx.mem) Bubble(m->path(), m->position(), m); }

    p_stack.push_back(m);

    Media_Block* mm = new (ctx.mem) Media_Block(m->path(),
                                                m->position(),
                                                m->media_queries(),
                                                m->block()->perform(this)->block());
    p_stack.pop_back();

    return debubble(mm->block(), mm)->block();
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
    Block* result = new (ctx.mem) Block(parent->path(), parent->position());

    for (size_t i = 0, L = baz.size(); i < L; ++i) {
      bool is_bubble = baz[i].first;
      Block* slice = baz[i].second;

      if (!is_bubble) {
        if (!previous_parent) {
          previous_parent = static_cast<Has_Block*>(parent);

          Has_Block* new_parent = static_cast<Has_Block*>(parent);
          new_parent->block(slice);

          *result << new_parent;
        }
        continue;
      }

      Block* wrapper_block = new (ctx.mem) Block(parent->block()->path(),
                                                 parent->block()->position(),
                                                 parent->block()->length(),
                                                 parent->block()->is_root());

      for (size_t j = 0, K = slice->length(); j < K; ++j)
      {
        Statement* ss = 0;
        if ((*slice)[j]->statement_type() == Statement::BUBBLE) {
          ss = static_cast<Bubble*>((*slice)[j])->node();
        }

        if (!ss) continue;

        Statement* ssss = ss->perform(this);
        Statement* wrapper = flatten(ssss);
        *wrapper_block << wrapper;
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
}
