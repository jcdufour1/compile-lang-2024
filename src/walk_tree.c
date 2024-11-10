
#include "node.h"
#include "nodes.h"
#include "util.h"
#include "do_passes.h"
#include "symbol_table.h"

INLINE void walk_expr_ptr_vec(Env* env, Expr_ptr_vec* vector, void (callback)(Env* env));

INLINE void walk_node_ptr_vec(Env* env, Node_ptr_vec* vector, void (callback)(Env* env));

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env));

INLINE void walk_tree_expr(Env* env, void (callback)(Env* env)) {
    Node_expr* expr = node_unwrap_expr(vec_top(&env->ancesters));
    switch (expr->type) {
        case NODE_E_OPERATOR: {
            Node_e_operator* operator = node_unwrap_e_operator(expr);
            if (operator->type == NODE_OP_UNARY) {
                walk_tree_traverse(env, node_wrap_expr(node_unwrap_op_unary(operator)->child), callback);
            } else if (operator->type == NODE_OP_BINARY) {
                walk_tree_traverse(env, node_wrap_expr(node_unwrap_op_binary(operator)->lhs), callback);
                walk_tree_traverse(env, node_wrap_expr(node_unwrap_op_binary(operator)->rhs), callback);
            } else {
                unreachable("");
            }
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_E_SYMBOL_TYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_E_SYMBOL_UNTYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_E_MEMBER_SYM_UNTYPED: {
            Node_ptr_vec* vector = &node_unwrap_e_member_sym_untyped(expr)->children;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_E_MEMBER_SYM_TYPED: {
            Node_ptr_vec* vector = &node_unwrap_e_member_sym_typed(expr)->children;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_E_LITERAL:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_E_STRUCT_LITERAL: {
            Node_ptr_vec* vector = &node_unwrap_e_struct_literal(expr)->members;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_E_FUNCTION_CALL: {
            Expr_ptr_vec* vector = &node_unwrap_e_function_call(expr)->args;
            walk_expr_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_E_LLVM_REGISTER_SYM:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
    }
}

void walk_tree(Env* env, void (callback)(Env* env)) {
    if (!vec_top(&env->ancesters)) {
        return;
    }

    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);

    callback(env);

    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);

    Node* curr_node = vec_top(&env->ancesters);
    switch (curr_node->type) {
        case NODE_FUNCTION_PARAMS:
            walk_node_ptr_vec(env, &node_unwrap_function_params(curr_node)->params, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_ASSIGNMENT:
            walk_tree_traverse(env, (node_unwrap_assignment(curr_node)->lhs), callback);
            walk_tree_traverse(env, node_wrap_expr(node_unwrap_assignment(curr_node)->rhs), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_EXPR: 
            walk_tree_expr(env, callback);
            break;
        case NODE_ALLOCA:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LOAD_ANOTHER_NODE:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STORE_ANOTHER_NODE:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LOAD_ELEMENT_PTR:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LABEL:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_VARIABLE_DEF:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LANG_TYPE:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_RANGE:
            walk_tree_traverse(env, node_wrap_variable_def(node_unwrap_for_range(curr_node)->var_def), callback);
            walk_tree_traverse(env, node_wrap_for_lower_bound(node_unwrap_for_range(curr_node)->lower_bound), callback);
            walk_tree_traverse(env, node_wrap_for_upper_bound(node_unwrap_for_range(curr_node)->upper_bound), callback);
            walk_tree_traverse(env, node_wrap_block(node_unwrap_for_range(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_WITH_COND:
            walk_tree_traverse(env, node_wrap_condition(node_unwrap_for_with_cond(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap_block(node_unwrap_for_with_cond(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_IF:
            walk_tree_traverse(env, node_wrap_condition(node_unwrap_if(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap_block(node_unwrap_if(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FUNCTION_DEF:
            walk_tree_traverse(env, node_wrap_function_decl(node_unwrap_function_def(curr_node)->declaration), callback);
            walk_tree_traverse(env, node_wrap_block(node_unwrap_function_def(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FUNCTION_DECL:
            walk_tree_traverse(env, node_wrap_function_params(node_unwrap_function_decl(curr_node)->parameters), callback);
            walk_tree_traverse(env, node_wrap_lang_type(node_unwrap_function_decl(curr_node)->return_type), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            walk_tree_traverse(env, (Node*)node_wrap_e_struct_literal(node_unwrap_llvm_store_struct_literal(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_MEMBER_SYM_PIECE_TYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_GOTO:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_COND_GOTO:
            walk_tree_traverse(env, (Node*)node_wrap_e_symbol_untyped(node_unwrap_cond_goto(curr_node)->if_true), callback);
            walk_tree_traverse(env, (Node*)node_wrap_e_symbol_untyped(node_unwrap_cond_goto(curr_node)->if_false), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_BLOCK: {
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            Node_ptr_vec* vector = &node_unwrap_block(curr_node)->children;
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_FOR_LOWER_BOUND:
            walk_tree_traverse(env, node_wrap_expr(node_unwrap_for_lower_bound(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_UPPER_BOUND:
            walk_tree_traverse(env, node_wrap_expr(node_unwrap_for_upper_bound(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_CONDITION:
            walk_tree_traverse(env, (Node*)node_unwrap_condition(curr_node)->child, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_RETURN:
            walk_tree_traverse(env, (Node*)node_unwrap_return(curr_node)->child, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_BREAK:
            walk_tree_traverse(env, node_unwrap_break(curr_node)->child, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LLVM_STORE_LITERAL:
            walk_tree_traverse(env, (Node*)node_wrap_e_literal(node_unwrap_llvm_store_literal(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STRUCT_DEF: {
            Node_ptr_vec* vector = &node_unwrap_struct_def(curr_node)->members;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
    }
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    return;
}

INLINE void walk_node_ptr_vec(Env* env, Node_ptr_vec* vector, void (callback)(Env* env)) {
    //log(LOG_DEBUG, "-------------------------\n");
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
        //log_tree(LOG_DEBUG, node_ptr_vec_at(vector, idx));
        assert(vec_at(vector, idx) && "a null element is in this vector");
        walk_tree_traverse(env, vec_at(vector, idx), callback);
        assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    }
    //log(LOG_DEBUG, "-------------------------\n");
}

INLINE void walk_expr_ptr_vec(Env* env, Expr_ptr_vec* vector, void (callback)(Env* env)) {
    //log(LOG_DEBUG, "-------------------------\n");
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
        //log_tree(LOG_DEBUG, expr_ptr_vec_at(vector, idx));
        assert(vec_at(vector, idx) && "a null element is in this vector");
        walk_tree_traverse(env, (Node*)vec_at(vector, idx), callback);
        assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    }
    //log(LOG_DEBUG, "-------------------------\n");
}

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env)) {
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    vec_append(&a_main, &env->ancesters, new_curr_node);
    env->recursion_depth++;
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    walk_tree(env, callback);
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    vec_rem_last(&env->ancesters);
    env->recursion_depth--;
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
}
