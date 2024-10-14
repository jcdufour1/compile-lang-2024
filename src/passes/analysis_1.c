#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

bool analysis_1(Node* block_, int recursion_depth) {
    (void) recursion_depth;
    if (block_->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(block_);
    Node_ptr_vec* block_children = &block->children;

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Node* curr_node = vec_at(block_children, idx);
        Lang_type dummy;
        switch (curr_node->type) {
            case NODE_ASSIGNMENT:
                //fallthrough
            case NODE_FUNCTION_CALL:
                //fallthrough
            case NODE_SYMBOL_UNTYPED:
                //fallthrough
            case NODE_RETURN_STATEMENT:
                //fallthrough
            case NODE_OPERATOR:
                //fallthrough
            case NODE_IF_STATEMENT:
                //fallthrough
            case NODE_FOR_WITH_CONDITION:
                try_set_node_type(&dummy, curr_node);
                break;
            default:
                break;
        }
    }

    return false;
}
