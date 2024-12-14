#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <nodes.h>
#include "passes.h"

static size_t llvm_id_for_next_var = 1;
static size_t prev_num = 1;

void assign_llvm_ids_expr(Env* env) {
    Node_expr* curr_node = node_unwrap_expr(vec_top(&env->ancesters));
    switch (curr_node->type) {
        case NODE_SYMBOL_UNTYPED:
            return;
        case NODE_MEMBER_ACCESS_UNTYPED:
            unreachable("");
        case NODE_MEMBER_ACCESS_TYPED:
            node_unwrap_member_access_typed(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STRUCT_LITERAL:
            return;
        case NODE_SYMBOL_TYPED:
            return;
        case NODE_LITERAL:
            return;
        case NODE_FUNCTION_CALL:
            node_unwrap_function_call(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operator(curr_node);
            if (operator->type == NODE_UNARY) {
                node_unwrap_unary(operator)->llvm_id = llvm_id_for_next_var;
                llvm_id_for_next_var += 2;
                return;
            } else if (operator->type == NODE_BINARY) {
                node_unwrap_binary(operator)->llvm_id = llvm_id_for_next_var;
                llvm_id_for_next_var += 2;
                return;
            } else {
                unreachable("");
            }
            unreachable("");
        }
        case NODE_LLVM_PLACEHOLDER:
            return;
    }
    unreachable("");
}

void assign_llvm_ids_def(Env* env) {
    Node_def* curr_node = node_unwrap_def(vec_top(&env->ancesters));
    switch (curr_node->type) {
        case NODE_VARIABLE_DEF:
            node_unwrap_variable_def(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LABEL:
            node_unwrap_label(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STRUCT_DEF:
            return;
        case NODE_RAW_UNION_DEF:
            return;
        case NODE_ENUM_DEF:
            return;
        case NODE_FUNCTION_DECL:
            return;
        case NODE_FUNCTION_DEF:
            return;
        case NODE_PRIMITIVE_DEF:
            return;
        case NODE_LITERAL_DEF:
            return;
    }
    unreachable("");
}

void assign_llvm_ids(Env* env) {
    Node* curr_node = vec_top(&env->ancesters);

    //log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
    assert(llvm_id_for_next_var == prev_num);

    switch (curr_node->type) {
        case NODE_EXPR:
            assign_llvm_ids_expr(env);
            break;
        case NODE_DEF:
            assign_llvm_ids_def(env);
            break;
        case NODE_FUNCTION_PARAMS:
            break;
        case NODE_BLOCK:
            break;
        case NODE_RETURN:
            break;
        case NODE_LANG_TYPE:
            break;
        case NODE_LOAD_ELEMENT_PTR:
            node_unwrap_load_element_ptr(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_LOAD_ANOTHER_NODE:
            node_unwrap_load_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_STORE_ANOTHER_NODE:
            node_unwrap_store_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            //log_tree(LOG_DEBUG, curr_node);
            //log_tree(LOG_DEBUG, node_unwrap_store_another_node(curr_node)->node_src.node);
            //log_tree(LOG_DEBUG, node_unwrap_store_another_node(curr_node)->node_dest.node);
            break;
        case NODE_ASSIGNMENT:
            log_tree(LOG_ERROR, curr_node);
            unreachable("");
        case NODE_ALLOCA:
            assert(llvm_id_for_next_var == prev_num);
            node_unwrap_alloca(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_GOTO:
            node_unwrap_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_CONDITION:
            unreachable("");
        case NODE_LLVM_STORE_LITERAL:
            node_unwrap_llvm_store_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            node_unwrap_llvm_store_struct_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
    }

    prev_num = llvm_id_for_next_var;
}
