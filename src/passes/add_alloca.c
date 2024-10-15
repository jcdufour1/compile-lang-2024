#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../parser_utils.h"

static Node_alloca* alloca_new(Node_variable_def* var_def) {
    Node_alloca* alloca = node_unwrap_alloca(node_new(node_wrap(var_def)->pos, NODE_ALLOCA));
    alloca->name = var_def->name;
    alloca->lang_type = var_def->lang_type;
    var_def->storage_location = node_wrap(alloca);
    assert(alloca);
    return alloca;
}

static void do_function_definition(Node_function_definition* fun_def) {
    Node_function_params* params = fun_def->declaration->parameters;
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Node_variable_def* param = node_unwrap_variable_def(node_ptr_vec_at(&params->params, idx));
        if (is_corresponding_to_a_struct(node_wrap(param))) {
            param->storage_location = node_wrap(param);
            continue;
        }
        node_ptr_vec_insert(&fun_def->body->children, 0, node_wrap(alloca_new(param)));
    }
}


static void insert_alloca(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_variable_def* var_def
) {
    insert_into_node_ptr_vec(block_children, idx_to_insert_before, 0, node_wrap(
        alloca_new(var_def)
    ));
}

static void do_assignment(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
) {
    node_ptr_assert_no_null(block_children);
    if (assignment->lhs->type == NODE_VARIABLE_DEFINITION) {
        assert(assignment->lhs);
        insert_alloca(block_children, idx_to_insert_before, node_unwrap_variable_def(assignment->lhs));
        node_ptr_assert_no_null(block_children);
    }
    node_ptr_assert_no_null(block_children);
}

void add_alloca(Env* env) {
    Node* block_ = node_ptr_vec_top(&env->ancesters);
    if (block_->type != NODE_BLOCK) {
        return;
    }
    Node_block* block = node_unwrap_block(block_);
    Node_ptr_vec* block_children = &block->children;

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        assert(node_ptr_vec_at(&block->children, idx));
    }

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        node_ptr_assert_no_null(&block->children);
        Node* curr_node = node_ptr_vec_at(block_children, idx);
        switch (curr_node->type) {
            case NODE_VARIABLE_DEFINITION:
                insert_alloca(block_children, &idx, node_unwrap_variable_def(curr_node));
                node_ptr_assert_no_null(&block->children);
                break;
            case NODE_FUNCTION_DEFINITION:
                do_function_definition(node_unwrap_function_definition(curr_node));
                node_ptr_assert_no_null(&block->children);
                break;
            case NODE_ASSIGNMENT:
                do_assignment(block_children, &idx, node_unwrap_assignment(curr_node));
                node_ptr_assert_no_null(&block->children);
            default:
                break;
        }
    }

    return;
}
