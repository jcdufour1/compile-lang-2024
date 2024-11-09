#ifndef ENV_H
#define ENV_H

#include "symbol_table_struct.h"

typedef struct {
    Node_ptr_vec ancesters; // index 0 is the root of the tree
                            // index len - 1 is the current node
    Node_ptr_vec defered_symbols_to_add;
    Symbol_table global_literals; // this is populated during add_load_and_store pass
    int recursion_depth;

    int curly_brace_depth_curr_block;
} Env;

#endif // ENV_H
