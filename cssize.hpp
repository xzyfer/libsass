#ifndef SASS_CSSIZE
#define SASS_CSSIZE

#include <vector>
#include <iostream>

#ifndef SASS_AST
#include "ast.hpp"
#endif

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
#endif

namespace Sass {
  using namespace std;

  struct Context;
  typedef Environment<AST_Node*> Env;

  class Cssize : public Operation_CRTP<Statement*, Cssize> {

    Context&            ctx;
    Env*                env;
    vector<Block*>      block_stack;
    vector<Statement*>  p_stack;

    Statement* fallback_impl(AST_Node* n);

  public:
    Cssize(Context&, Env*);
    virtual ~Cssize() { }

    using Operation<Statement*>::operator();

    Statement* operator()(Block*);
    Statement* operator()(Ruleset*);
    // Statement* operator()(Propset*);
    // Statement* operator()(Bubble*);
    Statement* operator()(Media_Block*);
    // Statement* operator()(Feature_Block*);
    // Statement* operator()(At_Rule*);
    // Statement* operator()(Declaration*);
    // Statement* operator()(Assignment*);
    // Statement* operator()(Import*);
    // Statement* operator()(Import_Stub*);
    // Statement* operator()(Warning*);
    // Statement* operator()(Error*);
    // Statement* operator()(Comment*);
    // Statement* operator()(If*);
    // Statement* operator()(For*);
    // Statement* operator()(Each*);
    // Statement* operator()(While*);
    // Statement* operator()(Return*);
    // Statement* operator()(Extension*);
    // Statement* operator()(Definition*);
    // Statement* operator()(Mixin_Call*);
    // Statement* operator()(Content*);

    Statement* parent();
    vector<pair<bool, Block*>> slice_by_bubble(Statement*);
    Statement* bubble(Media_Block*);
    Statement* debubble(Block* children, Statement* parent = 0);
    Statement* flatten(Statement*);
    bool bubblable(Statement*);

    List* merge_media_queries(Media_Block*, Media_Block*);
    Media_Query* merge_media_query(Media_Query*, Media_Query*);

    template <typename U>
    Statement* fallback(U x) { return fallback_impl(x); }

    void append_block(Block*);
  };

}

#endif
