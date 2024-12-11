#ifndef DO_PASSES_H
#define DO_PASSES_H

#include "util.h"
#include <node.h>
#include "node_ptr_vec.h"
#include "env.h"
#include "parameters.h"

void do_passes(Str_view file_text, const Parameters* params);

void walk_tree(Env* env, Node* node, void (callback)(Env* env, Node* node));

INLINE void start_walk(Env* env, Node_block** root, void (callback)(Env* env, Node* node)) {
    assert(env->recursion_depth == 0);
    assert(env->ancesters.info.count == 0);
    vec_append(&a_main, &env->ancesters, (*root)->symbol_table);
    walk_tree(env, node_wrap_block(*root), callback);
    vec_rem_last(&env->ancesters);
}

#endif // DO_PASSES_H
