#ifndef PASSES_H
#define PASSES_H

#include "../node.h"

bool walk_tree(Node* curr_node, bool (callback)(Node* curr_node));

bool for_and_if_to_branch(Node* curr_node);

bool struct_member_thing(Node* curr_node);

bool flatten_operations(Node* curr_node);

bool add_load_and_store(Node* curr_node);

bool assign_llvm_ids(Node* curr_node);

#endif // PASSES_H
