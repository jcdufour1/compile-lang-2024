#ifndef DO_PASSES_H
#define DO_PASSES_H

#include "node.h"

void do_passes(Node_block** root);

bool walk_tree(Node* curr_node, int recursion_depth, bool (callback)(Node* curr_node, int recursion_depth));

#endif // DO_PASSES_H
