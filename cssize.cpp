#include "cssize.hpp"
#include "expand.hpp"
#include "bind.hpp"
#include "eval.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "debug.hpp"

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
    // DEBUG_PRINTLN(ALL, "Cssize::Block");
    Env new_env;
    new_env.link(*env);
    env = &new_env;
    Block* bb = new (ctx.mem) Block(b->path(), b->position(), b->length(), b->is_root());
    block_stack.push_back(bb);
    append_block(b);
    block_stack.pop_back();
    if (b->is_root()) {
      // To_String to_string;
      // DEBUG_PRINTLN(ALL, "★★★★★★★★★★");
      // DEBUG_PRINTLN(ALL, b->perform(&to_string));
      // DEBUG_PRINTLN(ALL, "★★★★★★★★★★");
      // DEBUG_PRINTLN(ALL, bb->perform(&to_string));
    }
    env = env->parent();
    return bb;
  }

  Statement* Cssize::operator()(Media_Block* m)
  {
    To_String to_string;
    // DEBUG_PRINTLN(ALL, "Cssize::Media_Block");
    // DEBUG_PRINTLN(ALL, "Cssize::Media_Block::parent()");
    // DEBUG_PRINTLN(ALL, parent()->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "Cssize::Media_Block::node()");
    // DEBUG_PRINTLN(ALL, m->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "Cssize::Media_Block::node()->length => " << m->block()->length());

    if (parent()->statement_type() == Statement::MEDIA)
    {
      // DEBUG_PRINTLN(ALL, "Cssize::Media_Block::parent()->is_media? => true");
      return new (ctx.mem) Bubble(m->path(), m->position(), m); }

    // DEBUG_PRINTLN(ALL, "Cssize::Media_Block::parent()->is_media? => false");

    p_stack.push_back(m);

    Media_Block* mm = new (ctx.mem) Media_Block(m->path(),
                                                m->position(),
                                                m->media_queries(),
                                                m->block()->perform(this)->block());
    p_stack.pop_back();

    Statement* foo = debubble(mm->block(), m)->block();
    // DEBUG_PRINTLN(ALL, "++++++++");
    // DEBUG_PRINTLN(ALL, foo->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "++++++++");
    return foo;

    // Media_Block* mmm = new (ctx.mem) Media_Block(mm->path(),
    //                                              mm->position(),
    //                                              mm->media_queries(),
    //                                              foo->block());
    // DEBUG_PRINTLN(ALL, "********");
    // DEBUG_PRINTLN(ALL, mmm->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "********");
    // return mmm;
  }

  inline bool Cssize::bubblable(Statement* s)
  {
    return s->statement_type() == Statement::MEDIA || s->bubbles();
  }

  inline Statement* Cssize::fallback_impl(AST_Node* n)
  {
    return static_cast<Statement*>(n);
  }

  inline Statement* Cssize::flatten(Media_Block* b)
  {
    // DEBUG_PRINTLN(ALL, "Cssize::flatten::Media_Block");
    return flatten(b->block());
  }

  inline Statement* Cssize::flatten(Statement* s)
  {
    // DEBUG_PRINTLN(ALL, "Cssize::flatten::Statement");
    return flatten(s->block());
  }

  inline Statement* Cssize::flatten(Block* bb)
  {
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

  inline vector<pair<bool, Block*>> Cssize::slice_by(Statement* b)
  {
    vector<pair<bool, Block*>> results;
    for (size_t i = 0, L = b->block()->length(); i < L; ++i) {
      Statement* value = (*b->block())[i];
      bool key = value->statement_type() == Statement::BUBBLE;
      if (!results.empty() && results.back().first == key)
      {
        Block* foo = results.back().second;
        *foo << value;
      }
      else
      {
        Block* foo = new (ctx.mem) Block(value->path(), value->position());
        *foo << value;
        results.push_back(make_pair(key, foo));
      }
    }
    return results;
  }

  inline Statement* Cssize::debubble(Block* children, Media_Block* parent = 0)
  {
    To_String to_string;
    // DEBUG_PRINTLN(ALL, "Cssize::debubble");
    // DEBUG_PRINTLN(ALL, "Cssize::debubble::children");
    // DEBUG_PRINTLN(ALL, children->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "Cssize::debubble::parent");
    // DEBUG_PRINTLN(ALL, parent->perform(&to_string));

    Media_Block* previous_parent = 0;
    vector<pair<bool, Block*>> baz = slice_by(children);
    Block* bar = new (ctx.mem) Block(parent->path(), parent->position());

    // DEBUG_PRINTLN(ALL, "Cssize::debubble::slice_by->length => " << baz.size());

    for (size_t i = 0, L = baz.size(); i < L; ++i) {
      bool is_bubble = baz[i].first;
      Block* slice = baz[i].second;

      // DEBUG_PRINTLN(ALL, "Cssize::foo::is_bubble => " << is_bubble);
      // DEBUG_PRINTLN(ALL, "Cssize::foo( " << is_bubble << " )->length() => " << slice->length());
      // DEBUG_PRINTLN(ALL, "Cssize::foo( " << is_bubble << " ) => " << slice->perform(&to_string));
      // DEBUG_PRINTLN(ALL, "Cssize::foo::previous_parent => " << previous_parent);

      if (!is_bubble) {
        if (!parent) {
          *bar << slice;
          continue;
        }
        if (!previous_parent) {
          previous_parent = new (ctx.mem) Media_Block(parent->path(),
                                                      parent->position(),
                                                      parent->media_queries(),
                                                      parent->block());
          previous_parent->selector(parent->selector());

          Media_Block* new_parent = new (ctx.mem) Media_Block(parent->path(),
                                                              parent->position(),
                                                              parent->media_queries(),
                                                              slice);
          new_parent->selector(parent->selector());

          // for (size_t j = 0, K = slice->length(); j < K; ++j) {
          //   *new_parent << (*slice)[j];
          // }

          // DEBUG_PRINTLN(ALL, "Cssize::foo::adding => " << new_parent->perform(&to_string));
          // DEBUG_PRINTLN(ALL, " to " << bar->perform(&to_string));

          *bar << new_parent;
          continue;
        }
      }

      // DEBUG_PRINTLN(ALL, "Cssize::foo::bubbling");

      Block* foo = new (ctx.mem) Block(parent->block()->path(),
                                        parent->block()->position(),
                                        parent->block()->length(),
                                        parent->block()->is_root());

      for (size_t j = 0, K = slice->length(); j < K; ++j) {
        Statement* ss = 0;
        Bubble* b = static_cast<Bubble*>((*slice)[j]);

        // DEBUG_PRINTLN(ALL, "Cssize::foo::is_bubble? => " << ((*slice)[j]->statement_type() == Statement::BUBBLE));
        if (b->node()->statement_type() != Statement::MEDIA) ss = b->node();
        else if (static_cast<Media_Block*>(b->node())->media_queries() == parent->media_queries()) ss = b->node();
        else {
          List* mq = merge_media_queries(static_cast<Media_Block*>(b->node()), parent);
          if (!mq->length()) continue;
          static_cast<Media_Block*>(b->node())->media_queries(mq);
          ss = b->node();
        }

        if (!ss) continue;
        // *foo << ss->perform(this);
        Statement* ssss = ss->perform(this);
        // DEBUG_PRINTLN(ALL, "Cssize::foo::before_flatten => " << ssss->perform(&to_string));
        // DEBUG_PRINTLN(ALL, ssss->perform(&to_string));
        Statement* sssss = flatten(ssss);
        *foo << sssss;
        // DEBUG_PRINTLN(ALL, "Cssize::foo::after_flatten => " << sssss->perform(&to_string));
      }

      if (foo) {
        // DEBUG_PRINTLN(ALL, "Cssize::foo::adding -1 => " << flatten(foo)->perform(&to_string))
        // DEBUG_PRINTLN(ALL, " to " << bar->perform(&to_string));
        *bar << flatten(foo);
      }
    }

    // return bar;
    // DEBUG_PRINTLN(ALL, "Cssize::foo::before_flatten - 1 => " << bar->perform(&to_string));
    Statement* ssss = flatten(bar);
    // DEBUG_PRINTLN(ALL, "Cssize::foo::after_flatten - 1 => " << ssss->perform(&to_string));
    return ssss;
  }

  inline void Cssize::append_block(Block* b)
  {
    To_String to_string;
    Block* current_block = block_stack.back();

    // DEBUG_PRINTLN(ALL, "$$$$$$$");
    // DEBUG_PRINTLN(ALL, current_block->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "$$$$$$$");

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* ith = (*b)[i]->perform(this);
      if (ith && ith->block()) {
        for (size_t j = 0, K = ith->block()->length(); j < K; ++j) {
          // DEBUG_PRINTLN(ALL, "Cssize::foo::adding => " << (*ith->block())[j]->perform(&to_string))
          // DEBUG_PRINTLN(ALL, " to " << current_block->perform(&to_string));
          *current_block << (*ith->block())[j];
        }
      }
      else if (ith) {
        // DEBUG_PRINTLN(ALL, "Cssize::foo::adding -1 => " << ith->perform(&to_string))
        // DEBUG_PRINTLN(ALL, " to " << current_block->perform(&to_string));
        *current_block << ith;
      }
    }

    // DEBUG_PRINTLN(ALL, "^^^^^^^^");
    // DEBUG_PRINTLN(ALL, current_block->perform(&to_string));
    // DEBUG_PRINTLN(ALL, "^^^^^^^^");
  }

  inline List* Cssize::merge_media_queries(Media_Block* m1, Media_Block* m2)
  {
    To_String to_string;
    List* qq = new (ctx.mem) List(m1->media_queries()->path(),
                                  m1->media_queries()->position(),
                                  m1->media_queries()->length());

    for (size_t i = 0, L = m1->media_queries()->length(); i < L; i++) {
      for (size_t j = 0, K = m2->media_queries()->length(); j < K; j++) {
        Media_Query* mq1 = static_cast<Media_Query*>((*m1->media_queries())[i]);
        Media_Query* mq2 = static_cast<Media_Query*>((*m2->media_queries())[j]);
        Media_Query* mq = merge_media_query(mq1, mq2);

        if (mq) *qq << mq;
      }
    }

    DEBUG_PRINTLN(ALL, "++++");
    DEBUG_PRINTLN(ALL, m1->media_queries()->perform(&to_string));
    DEBUG_PRINTLN(ALL, m2->media_queries()->perform(&to_string));
    DEBUG_PRINTLN(ALL, qq->perform(&to_string));
    DEBUG_PRINTLN(ALL, "----");

    return qq;
  }


  inline Media_Query* Cssize::merge_media_query(Media_Query* mq1, Media_Query* mq2)
  {
    // DEBUG_PRINTLN(ALL, "foo");
    To_String to_string;

    string type;
    string mod;

    string m1 = string(mq1->is_restricted() ? "only" : mq1->is_negated() ? "not" : "");
    string t1 = mq1->media_type() ? mq1->media_type()->perform(&to_string) : "";
    string m2 = string(mq2->is_restricted() ? "only" : mq1->is_negated() ? "not" : "");
    string t2 = mq2->media_type() ? mq2->media_type()->perform(&to_string) : "";


    if (t1.empty()) t1 = t2;
    if (t2.empty()) t2 = t1;

      DEBUG_PRINTLN(ALL, "---- foo");
    DEBUG_PRINTLN(ALL, mq1->perform(&to_string));
    DEBUG_PRINTLN(ALL, mq2->perform(&to_string));
    DEBUG_PRINTLN(ALL, "t1 => " << t1);
    DEBUG_PRINTLN(ALL, "t2 => " << t2);
    DEBUG_PRINTLN(ALL, "m1 => " << m1);
    DEBUG_PRINTLN(ALL, "m2 => " << m2);
    DEBUG_PRINTLN(ALL, (bool)((m1 == "not") ^ (m2 == "not")));
    DEBUG_PRINTLN(ALL, (bool)(m1 == "not" && m2 == "not"));
    DEBUG_PRINTLN(ALL, (bool)(t1 != t2));
      DEBUG_PRINTLN(ALL, "---- bar");

    // DEBUG_PRINTLN(ALL, "2");

    if ((m1 == "not") ^ (m2 == "not")) {
      if (t1 == t2) {
        // DEBUG_PRINTLN(ALL, "oh noes!");
        return 0;
      }
      type = m1 == "not" ? t2 : t1;
      mod = m1 == "not" ? m2 : m1;
    }
    else if (m1 == "not" && m2 == "not") {
      if (t1 != t2) {
        // DEBUG_PRINTLN(ALL, "bar");
        return 0;
      }
      type = t1;
      mod = "not";
    }
    else if (t1 != t2) {
      // DEBUG_PRINTLN(ALL, "bar");
      return 0;
    } else {
      type = t1;
      mod = m1.empty() ? m2 : m1;
      // DEBUG_PRINTLN(ALL, "type => " << type);
      // DEBUG_PRINTLN(ALL, "mod => " << mod);
    }

    Media_Query* mm = new (ctx.mem) Media_Query(
      mq1->path(), mq1->position(),
      new (ctx.mem) String_Constant(mq1/*->media_type()*/->path(), mq1/*->media_type()*/->position(), type),
      mq1->length() + mq2->length(), mod == "not", mod == "only"
    );
      // DEBUG_PRINTLN(ALL, "bar");
    *mm += mq2;
    *mm += mq1;
    return mm;
  }
}
