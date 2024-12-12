#include "passes.h"
#include <node.h>
#include <nodes.h>
#include <parser_utils.h>

Node_alloca* add_alloca_alloca_new(Node_variable_def* var_def) {
    Node_alloca* alloca = node_alloca_new(node_wrap_def(node_wrap_variable_def(var_def))->pos);
    alloca->name = var_def->name;
    alloca->lang_type = var_def->lang_type;
    var_def->storage_location = llvm_register_sym_new(node_wrap_alloca(alloca));
    assert(alloca);
    return alloca;
}

static void do_function_def(Env* env, Node_function_def* fun_def) {
    Node_function_params* params = fun_def->declaration->parameters;
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Node_variable_def* param = node_unwrap_variable_def(node_unwrap_def(vec_at(&params->params, idx)));

        if (lang_type_is_struct(env, param->lang_type)) {
            param->storage_location = llvm_register_sym_new(node_wrap_def(node_wrap_variable_def(param)));
        } else if (lang_type_is_enum(env, param->lang_type)) {
            vec_insert(&a_main, &fun_def->body->children, 0, node_wrap_alloca(add_alloca_alloca_new(param)));
        } else if (lang_type_is_raw_union(env, param->lang_type)) {
            param->storage_location = llvm_register_sym_new(node_wrap_def(node_wrap_variable_def(param)));
        } else if (lang_type_is_primitive(env, param->lang_type)) {
            vec_insert(&a_main, &fun_def->body->children, 0, node_wrap_alloca(add_alloca_alloca_new(param)));
        } else {
            todo();
        }
    }
}


static void insert_alloca(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_variable_def* var_def
) {
    insert_into_node_ptr_vec(block_children, idx_to_insert_before, 0, node_wrap_alloca(
        add_alloca_alloca_new(var_def)
    ));
}

static void do_assignment(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
) {
    if (assignment->lhs->type != NODE_DEF) {
        return;
    }
    Node_def* lhs_def = node_unwrap_def(assignment->lhs);

    if (lhs_def->type == NODE_VARIABLE_DEF) {
        insert_alloca(block_children, idx_to_insert_before, node_unwrap_variable_def(lhs_def));
    }
}

void add_alloca_def(Env* env, Node_ptr_vec* block_children, size_t* idx, Node_def* def) {
    switch (def->type) {
        case NODE_VARIABLE_DEF:
            insert_alloca(block_children, idx, node_unwrap_variable_def(def));
            return;
        case NODE_FUNCTION_DEF: {
            Node_block* fun_block = node_unwrap_function_def(def)->body;
            vec_append(&a_main, &env->ancesters, node_wrap_block(fun_block));
            do_function_def(env, node_unwrap_function_def(def));
            vec_rem_last(&env->ancesters);
            return;
        }
        default:
            return;
    }
    unreachable("");
}

void add_alloca(Env* env) {
    Node* block_ = vec_top(&env->ancesters);
    if (block_->type != NODE_BLOCK) {
        return;
    }
    Node_block* block = node_unwrap_block(block_);
    Node_ptr_vec* block_children = &block->children;

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Node* curr_node = vec_at(block_children, idx);

        switch (curr_node->type) {
            case NODE_DEF:
                add_alloca_def(env, block_children, &idx, node_unwrap_def(curr_node));
                break;
            case NODE_ASSIGNMENT:
                do_assignment(block_children, &idx, node_unwrap_assignment(curr_node));
            default:
                break;
        }
    }

    return;
}
