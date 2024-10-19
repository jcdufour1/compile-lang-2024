
#include "node.h"
#include "nodes.h"
#include "util.h"
#include "do_passes.h"
#include "symbol_table.h"

INLINE void walk_node_ptr_vec(Env* env, Node_ptr_vec* vector, void (callback)(Env* env));

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env));

void walk_tree(Env* env, void (callback)(Env* env)) {
    if (!vec_top(&env->ancesters)) {
        return;
    }

    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);

    callback(env);

    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);

    Node* curr_node = vec_top(&env->ancesters);
    switch (curr_node->type) {
        case NODE_FUNCTION_PARAMETERS:
            walk_node_ptr_vec(env, &node_unwrap_function_params(curr_node)->params, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FUNCTION_RETURN_TYPES:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_return_types(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_ASSIGNMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_assignment(curr_node)->lhs), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_assignment(curr_node)->rhs), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
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
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_SYMBOL_TYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_SYMBOL_UNTYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
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
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LABEL:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_VARIABLE_DEFINITION:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LANG_TYPE:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_RANGE:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->var_def), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->lower_bound), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->upper_bound), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_range(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_WITH_CONDITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_with_condition(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_for_with_condition(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_IF_STATEMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_if(curr_node)->condition), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_if(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FUNCTION_DEFINITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_definition(curr_node)->declaration), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_function_definition(curr_node)->body), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FUNCTION_DECLARATION:
            walk_tree_traverse(env, node_wrap(node_unwrap_function_declaration(curr_node)->parameters), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_function_declaration(curr_node)->return_types), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            walk_tree_traverse(env, node_wrap(node_unwrap_llvm_store_struct_literal(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_GOTO:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_COND_GOTO:
            walk_tree_traverse(env, node_wrap(node_unwrap_cond_goto(curr_node)->if_true), callback);
            walk_tree_traverse(env, node_wrap(node_unwrap_cond_goto(curr_node)->if_false), callback);
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
            walk_tree_traverse(env, node_wrap(node_unwrap_for_lower_bound(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_FOR_UPPER_BOUND:
            walk_tree_traverse(env, node_wrap(node_unwrap_for_upper_bound(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_IF_CONDITION:
            walk_tree_traverse(env, node_wrap(node_unwrap_if_condition(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LITERAL:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STRUCT_LITERAL: {
            Node_ptr_vec* vector = &node_unwrap_struct_literal(curr_node)->members;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_FUNCTION_CALL: {
            Node_ptr_vec* vector = &node_unwrap_function_call(curr_node)->args;
            walk_node_ptr_vec(env, vector, callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        }
        case NODE_RETURN_STATEMENT:
            walk_tree_traverse(env, node_wrap(node_unwrap_return_statement(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LLVM_REGISTER_SYM:
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_BREAK:
            walk_tree_traverse(env, node_wrap(node_unwrap_break(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_LLVM_STORE_LITERAL:
            walk_tree_traverse(env, node_wrap(node_unwrap_llvm_store_literal(curr_node)->child), callback);
            assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
            break;
        case NODE_STRUCT_DEFINITION: {
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

INLINE void walk_tree_traverse(Env* env, Node* new_curr_node, void (callback)(Env* env)) {
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    vec_append(&a_main, &env->ancesters, new_curr_node);
    env->recursion_depth++;
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    walk_tree(env, callback);
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
    Node* dummy = NULL;
    vec_pop(dummy, &env->ancesters);
    env->recursion_depth--;
    assert((size_t)env->recursion_depth + 1 == env->ancesters.info.count);
}
