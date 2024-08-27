#ifndef PASSES_H
#define PASSES_H

#include "../node.h"

void walk_tree(Node_id root, void (callback)(Node_id curr_node));

void for_loop_to_branch(Node_id curr_node);

#endif // PASSES_H
