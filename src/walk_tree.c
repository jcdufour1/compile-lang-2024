
#include "node.h"
#include "nodes.h"
#include "util.h"
#include "do_passes.h"

INLINE void walk_node_ptr_vec(Node_ptr_vec* vector, Env* env, void (callback)(Env* env));

INLINE void walk_tree_traverse(Env* env, Node** new_curr_node, void (callback)(Env* env)) {
}

void walk_tree(Env* env, void (callback)(Env* env)) {
    if (!node_ptr_vec_top(&env->ancesters)) {
        todo();
        return;
    }

    callback(env);

    env->recursion_depth++;

    switch (input_node->type) {
        case NODE_FUNCTION_PARAMETERS:
            walk_node_ptr_vec(&node_unwrap_function_params(input_node)->params, env, callback);
            break;
        case NODE_FUNCTION_RETURN_TYPES:
            start_walk(node_wrap(node_unwrap_function_return_types(input_node)->child), env, callback);
            break;
        case NODE_ASSIGNMENT:
            start_walk(node_wrap(node_unwrap_assignment(input_node)->lhs), env, callback);
            start_walk(node_wrap(node_unwrap_assignment(input_node)->rhs), env, callback);
            break;
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operation(input_node);
            if (operator->type == NODE_OP_UNARY) {
                start_walk(node_wrap(node_unwrap_op_unary(operator)->child), env, callback);
            } else if (operator->type == NODE_OP_BINARY) {
                start_walk(node_wrap(node_unwrap_op_binary(operator)->lhs), env, callback);
                start_walk(node_wrap(node_unwrap_op_binary(operator)->rhs), env, callback);
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
            start_walk(node_wrap(node_unwrap_for_range(input_node)->var_def), env, callback);
            start_walk(node_wrap(node_unwrap_for_range(input_node)->lower_bound), env, callback);
            start_walk(node_wrap(node_unwrap_for_range(input_node)->upper_bound), env, callback);
            start_walk(node_wrap(node_unwrap_for_range(input_node)->body), env, callback);
            break;
        case NODE_FOR_WITH_CONDITION:
            start_walk(node_wrap(node_unwrap_for_with_condition(input_node)->condition), env, callback);
            start_walk(node_wrap(node_unwrap_for_with_condition(input_node)->body), env, callback);
            break;
        case NODE_IF_STATEMENT:
            start_walk(node_wrap(node_unwrap_if(input_node)->condition), env, callback);
            start_walk(node_wrap(node_unwrap_if(input_node)->body), env, callback);
            break;
        case NODE_FUNCTION_DEFINITION:
            start_walk(node_wrap(node_unwrap_function_definition(input_node)->declaration), env, callback);
            start_walk(node_wrap(node_unwrap_function_definition(input_node)->body), env, callback);
            break;
        case NODE_FUNCTION_DECLARATION:
            start_walk(node_wrap(node_unwrap_function_declaration(input_node)->parameters), env, callback);
            start_walk(node_wrap(node_unwrap_function_declaration(input_node)->return_types), env, callback);
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            start_walk(node_wrap(node_unwrap_llvm_store_struct_literal(input_node)->child), env, callback);
            break;
        case NODE_GOTO:
            break;
        case NODE_COND_GOTO:
            start_walk(node_wrap(node_unwrap_cond_goto(input_node)->if_true), env, callback);
            start_walk(node_wrap(node_unwrap_cond_goto(input_node)->if_false), env, callback);
            break;
        case NODE_BLOCK: {
            Node_ptr_vec* vector = &node_unwrap_block(input_node)->children;
            walk_node_ptr_vec(vector, recursion_depth, callback);
            break;
        }
        case NODE_FOR_LOWER_BOUND:
            start_walk(node_wrap(node_unwrap_for_lower_bound(input_node)->child), env, callback);
            break;
        case NODE_FOR_UPPER_BOUND:
            start_walk(node_wrap(node_unwrap_for_upper_bound(input_node)->child), env, callback);
            break;
        case NODE_IF_CONDITION:
            start_walk(node_wrap(node_unwrap_if_condition(input_node)->child), env, callback);
            break;
        case NODE_LITERAL:
            break;
        case NODE_STRUCT_LITERAL: {
            Node_ptr_vec* vector = &node_unwrap_struct_literal(input_node)->members;
            walk_node_ptr_vec(vector, recursion_depth, callback);
            break;
        }
        case NODE_FUNCTION_CALL: {
            Node_ptr_vec* vector = &node_unwrap_function_call(input_node)->args;
            walk_node_ptr_vec(vector, recursion_depth, callback);
            break;
        }
        case NODE_RETURN_STATEMENT:
            start_walk(node_wrap(node_unwrap_return_statement(input_node)->child), env, callback);
            break;
        case NODE_LLVM_REGISTER_SYM:
            break;
        case NODE_BREAK:
            start_walk(node_wrap(node_unwrap_break(input_node)->child), env, callback);
            break;
        case NODE_LLVM_STORE_LITERAL:
            start_walk(node_wrap(node_unwrap_llvm_store_literal(input_node)->child), env, callback);
            break;
        case NODE_STRUCT_DEFINITION: {
            Node_ptr_vec* vector = &node_unwrap_struct_def(input_node)->members;
            walk_node_ptr_vec(vector, recursion_depth, callback);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(input_node));
            break;
    }
    return;
}

INLINE void walk_node_ptr_vec(Node_ptr_vec* vector, Env* env, void (callback)(Env* env)) {
    node_ptr_assert_no_null(vector);
    //log(LOG_DEBUG, "-------------------------\n");
    for (size_t idx = 0; idx < vector->info.count; idx++) {
        //log_tree(LOG_DEBUG, node_ptr_vec_at(vector, idx));
        assert(node_ptr_vec_at(vector, idx) && "a null element is in this vector");
        walk_tree(node_ptr_vec_at(vector, idx), recursion_depth + 1, callback);
    }
    //log(LOG_DEBUG, "-------------------------\n");
}
