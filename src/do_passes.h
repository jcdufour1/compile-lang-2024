#ifndef DO_PASSES_H
#define DO_PASSES_H

#include "node.h"
#include "node_ptr_vec.h"

typedef struct {
    Node_ptr_vec ancesters; // index 0 is the root of the tree
                            // index len - 1 is the current node
    int recursion_depth;
} Env;

void do_passes(Node_block** root);

void walk_tree(Env* env, void (callback)(Env* env));

static inline void start_walk(Node_block** root, void (callback)(Env* env)) {
    Env env = {0};
    vec_append(&a_main, &env.ancesters, node_wrap(*root));
    walk_tree(&env, callback);
}

#endif // DO_PASSES_H
