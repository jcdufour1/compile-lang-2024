#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <nodes.h>
#include "passes.h"

static size_t llvm_id_for_next_var = 1;

void assign_llvm_ids_expr(Env* env) {
    Node_expr* curr_node = node_unwrap_expr(vec_top(&env->ancesters));
    switch (curr_node->type) {
        case NODE_SYMBOL_UNTYPED:
            return;
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            node_unwrap_member_sym_typed(curr_node)->llvm_id = llvm_id_for_next_var;
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

void assign_llvm_ids(Env* env) {
    Node* curr_node = vec_top(&env->ancesters);

    switch (curr_node->type) {
        case NODE_EXPR:
            assign_llvm_ids_expr(env);
            return;
        case NODE_STRUCT_DEF:
            return;
        case NODE_RAW_UNION_DEF:
            return;
        case NODE_FUNCTION_PARAMS:
            return;
        case NODE_BLOCK:
            return;
        case NODE_FUNCTION_DECL:
            return;
        case NODE_FUNCTION_DEF:
            return;
        case NODE_RETURN:
            return;
        case NODE_LANG_TYPE:
            return;
        case NODE_LOAD_ELEMENT_PTR:
            node_unwrap_load_element_ptr(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LOAD_ANOTHER_NODE:
            node_unwrap_load_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_STORE_ANOTHER_NODE:
            node_unwrap_store_another_node(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            log_tree(LOG_DEBUG, curr_node);
            log_tree(LOG_DEBUG, node_unwrap_store_another_node(curr_node)->node_src.node);
            log_tree(LOG_DEBUG, node_unwrap_store_another_node(curr_node)->node_dest.node);
            return;
        case NODE_VARIABLE_DEF:
            node_unwrap_variable_def(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_ASSIGNMENT:
            log_tree(LOG_ERROR, curr_node);
            unreachable("");
        case NODE_ALLOCA:
            node_unwrap_alloca(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_GOTO:
            node_unwrap_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_COND_GOTO:
            node_unwrap_cond_goto(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_CONDITION:
            unreachable("");
        case NODE_LABEL:
            node_unwrap_label(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LLVM_STORE_LITERAL:
            node_unwrap_llvm_store_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            node_unwrap_llvm_store_struct_literal(curr_node)->llvm_id = llvm_id_for_next_var;
            llvm_id_for_next_var += 2;
            return;
        case NODE_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_ENUM_DEF:
            return;
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
    }
}
