
#include "node.h"
#include "nodes.h"
#include "util.h"

INLINE void walk_node_ptr_vec(Env* env, Node_ptr_vec* vector, void (callback)(Env* env));

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env));

void walk_tree(Env* env, void (callback)(Env* env)) {
    if (!node_ptr_vec_top(&env->ancesters)) {
        return;
    }

    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);

    callback(env);

    Node* curr_node = node_ptr_vec_top(&env->ancesters);
    switch (curr_node->type) {
        case NODE_FUNCTION_PARAMETERS:
            walk_node_ptr_vec(env, &node_unwrap_function_params(curr_node)->params, callback);
            break;
        case NODE_FUNCTION_RETURN_TYPES:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_return_types(curr_node)->child), callback);
            break;
        case NODE_ASSIGNMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_assignment(curr_node)->lhs), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_assignment(curr_node)->rhs), callback);
            break;
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operation(curr_node);
            if (operator->type == NODE_OP_UNARY) {
                walk_tree_traverse(env, node_wrap(node_unwrap_op_unary(operator)->child), callback);
            } else if (operator->type == NODE_OP_BINARY) {
                walk_tree_traverse(env, node_wrap(node_unwrap_op_binary(operator)->lhs), callback);
                walk_tree_traverse(env, node_wrap(node_unwrap_op_binary(operator)->rhs), callback);
            } else {
                unreachable("");
            }
            break;
        }
        case NODE_SYMBOL_TYPED:
            break;
        case NODE_SYMBOL_UNTYPED:
            break;
        case NODE_ALLOCA:
            break;
        case NODE_LOAD_ANOTHER_NODE:
            break;
        case NODE_STORE_ANOTHER_NODE:
            break;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            break;
        case NODE_LABEL:
            break;
        case NODE_VARIABLE_DEFINITION:
            break;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            break;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            break;
        case NODE_LANG_TYPE:
            break;
        case NODE_FOR_RANGE:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->var_def), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->lower_bound), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->upper_bound), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->body), callback);
            break;
        case NODE_FOR_WITH_CONDITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_with_condition(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_with_condition(curr_node)->body), callback);
            break;
        case NODE_IF_STATEMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_if(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_if(curr_node)->body), callback);
            break;
        case NODE_FUNCTION_DEFINITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_definition(curr_node)->declaration), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_function_definition(curr_node)->body), callback);
            break;
        case NODE_FUNCTION_DECLARATION:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_declaration(curr_node)->parameters), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_function_declaration(curr_node)->return_types), callback);
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            walk_tree_traverse(env, node_wrap(node_unwrap_llvm_store_struct_literal(curr_node)->child), callback);
            break;
        case NODE_GOTO:
            break;
        case NODE_COND_GOTO:
            walk_tree_traverse(env, node_wrap(node_unwrap_cond_goto(curr_node)->if_true), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_cond_goto(curr_node)->if_false), callback);
            break;
        case NODE_BLOCK: {
            Node_ptr_vec* vector = &node_unwrap_block(curr_node)->children;
            walk_node_ptr_vec(env, vector, callback);
            break;
        }
        case NODE_FOR_LOWER_BOUND:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_lower_bound(curr_node)->child), callback);
            break;
        case NODE_FOR_UPPER_BOUND:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_upper_bound(curr_node)->child), callback);
            break;
        case NODE_IF_CONDITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_if_condition(curr_node)->child), callback);
            break;
        case NODE_LITERAL:
            break;
        case NODE_STRUCT_LITERAL: {
            Node_ptr_vec* vector = &node_unwrap_struct_literal(curr_node)->members;
            walk_node_ptr_vec(env, vector, callback);
            break;
        }
        case NODE_FUNCTION_CALL: {
            Node_ptr_vec* vector = &node_unwrap_function_call(curr_node)->args;
            walk_node_ptr_vec(env, vector, callback);
            break;
        }
        case NODE_RETURN_STATEMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_return_statement(curr_node)->child), callback);
            break;
        case NODE_LLVM_REGISTER_SYM:
            break;
        case NODE_BREAK:
            walk_tree_traverse(env, node_wrap(node_unwrap_break(curr_node)->child), callback);
            break;
        case NODE_LLVM_STORE_LITERAL:
            walk_tree_traverse(env, node_wrap(node_unwrap_llvm_store_literal(curr_node)->child), callback);
            break;
        case NODE_STRUCT_DEFINITION: {
            Node_ptr_vec* vector = &node_unwrap_struct_def(curr_node)->members;
            walk_node_ptr_vec(env, vector, callback);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(curr_node));
            break;
    }
    return;
}

INLINE void walk_node_ptr_vec(Env* env, Node_ptr_vec* vector, void (callback)(Env* env)) {
    node_ptr_assert_no_null(vector);
    //log(LOG_DEBUG, "-------------------------\n");
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        //log_tree(LOG_DEBUG, node_ptr_vec_at(vector, idx));
        assert(node_ptr_vec_at(vector, idx) && "a null element is in this vector");
        walk_tree_traverse(env, node_ptr_vec_at(vector, idx), callback);
    }
    //log(LOG_DEBUG, "-------------------------\n");
}

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env)) {
    node_ptr_vec_append(&env->ancesters, new_curr_node);
    env->recursion_depth++;
    walk_tree(env, callback);
    node_ptr_vec_pop(&env->ancesters);
    env->recursion_depth--;
}
