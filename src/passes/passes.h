#ifndef PASSES_H
#define PASSES_H

#include "../node.h"

bool walk_tree(Node_id root, bool (callback)(Node_id curr_node));

bool for_and_if_to_branch(Node_id curr_node);

bool add_load_and_store(Node_id curr_node);

bool assign_llvm_ids(Node_id curr_node);

#endif // PASSES_H
