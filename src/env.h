#ifndef ENV_H
#define ENV_H

typedef struct {
    Node_ptr_vec ancesters; // index 0 is the root of the tree
                            // index len - 1 is the current node
    Node_ptr_vec defered_symbols_to_add;
    int recursion_depth;
} Env;

#endif // ENV_H
