#include "passes.h"
#include "../node.h"
#include "../nodes.h"

void for_loop_to_branch(Node_id curr_node) {
    if (nodes_at(curr_node)->type != NODE_FOR_LOOP) {
        return;
    }

    Node_id for_block = nodes_get_child_of_type(curr_node, NODE_BLOCK);

    Node_id lang_goto = node_new();
    nodes_at(lang_goto)->type = NODE_GOTO;

    nodes_append_child(for_block, lang_goto);
}
