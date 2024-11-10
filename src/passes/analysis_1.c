#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

void analysis_1(Env* env) {
    Node* block_ = vec_top(&env->ancesters);
    if (block_->type != NODE_BLOCK) {
        return;
    }
    Node_block* block = node_unwrap_block(block_);
    Node_ptr_vec* block_children = &block->children;

    bool need_add_return = false;
    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Node** curr_node = vec_at_ref(block_children, idx);
        Lang_type dummy;
        switch ((*curr_node)->type) {
            case NODE_ASSIGNMENT:
                //fallthrough
            case NODE_E_FUNCTION_CALL:
                //fallthrough
            case NODE_E_SYMBOL_UNTYPED:
                //fallthrough
            case NODE_RETURN:
                //fallthrough
            case NODE_E_OPERATOR:
                //fallthrough
            case NODE_IF:
                //fallthrough
            case NODE_FOR_WITH_COND: {
                Node* new_node;
                try_set_node_lang_type(env, &new_node, &dummy, *curr_node);
                *curr_node = new_node;
                break;
            }
            default:
                break;
        }

        if (idx == block_children->info.count - 1 
            && (*curr_node)->type != NODE_RETURN
            && env->ancesters.info.count > 1
            && vec_at(&env->ancesters, env->ancesters.info.count - 2)->type == NODE_FUNCTION_DEF
        ) {
            need_add_return = true;
        }
    }

    if (need_add_return && 
        env->ancesters.info.count > 1 && vec_at(&env->ancesters, env->ancesters.info.count - 2)
    ) {
        Pos pos = vec_top(block_children)->pos;
        Node_return* rtn_statement = node_unwrap_return(
            node_new(pos, NODE_RETURN)
        );
        rtn_statement->auto_inserted = true;
        rtn_statement->child = node_wrap_e_literal(literal_new(str_view_from_cstr(""), TOKEN_VOID, pos));
        Lang_type dummy;
        Node* new_rtn_statement;
        try_set_node_lang_type(env, &new_rtn_statement, &dummy, node_wrap_return(rtn_statement));
        vec_append(&a_main, block_children, new_rtn_statement);
    }

    return;
}
