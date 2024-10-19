#ifndef DO_PASSES_H
#define DO_PASSES_H

#include "util.h"
#include "node.h"
#include "node_ptr_vec.h"
#include "env.h"
#include "parameters.h"

void do_passes(Node_block** root, const Parameters* params);

void walk_tree(Env* env, void (callback)(Env* env));

INLINE void start_walk(Node_block** root, void (callback)(Env* env)) {
    Env env = {0};
    vec_append(&a_main, &env.ancesters, node_wrap(*root));
    walk_tree(&env, callback);
    Node* dummy = NULL;
    vec_pop(dummy, &env.ancesters);
}

#endif // DO_PASSES_H
