
#include "../nodes.h"
#include "../parser_utils.h"
#include "passes.h"

static Node* change_op_node(Env* env, Node* root);

static Node_block* change_op_block(Env* env, Node_block* root);

static Node_expr* change_op_expr(Env* env, Node_expr* root);

static Node_binary* change_op_binary(Env* env, Node_binary* root) {
    (void) env;
    return root;
}

static Node_operator* change_op_unary(Env* env, Node_unary* unary) {
    switch (unary->token_type) {
        case TOKEN_NOT: {
            Node_binary* new_bin = node_binary_new(node_get_pos_unary(unary));
            new_bin->lhs = change_op_expr(env, unary->child);
            new_bin->rhs = node_wrap_literal(util_literal_new_from_int64_t(
                0,
                TOKEN_INT_LITERAL,
                node_get_pos_unary(unary)
            ));
            new_bin->token_type = TOKEN_DOUBLE_EQUAL;
            new_bin->lang_type = node_get_lang_type_expr(new_bin->lhs);
            assert(new_bin->lang_type.str.count > 0);
            return node_wrap_binary(new_bin);
        }
        default:
            return node_wrap_unary(unary);
    }
    unreachable("");
}

static Node_operator* change_op_operator(Env* env, Node_operator* root) {
    switch (root->type) {
        case NODE_BINARY:
            return node_wrap_binary(change_op_binary(env, node_unwrap_binary(root)));
        case NODE_UNARY:
            return change_op_unary(env, node_unwrap_unary(root));
    }
    unreachable("");
}

static Node_expr* change_op_expr(Env* env, Node_expr* root) {
    switch (root->type) {
        case NODE_OPERATOR:
            return node_wrap_operator(change_op_operator(env, node_unwrap_operator(root)));
        case NODE_SYMBOL_UNTYPED:
            return root;
        case NODE_SYMBOL_TYPED:
            return root;
        case NODE_MEMBER_ACCESS_UNTYPED:
            return root;
        case NODE_MEMBER_ACCESS_TYPED:
            return root;
        case NODE_INDEX_UNTYPED:
            return root;
        case NODE_INDEX_TYPED:
            return root;
        case NODE_LITERAL:
            return root;
        case NODE_FUNCTION_CALL:
            return root;
        case NODE_STRUCT_LITERAL:
            return root;
    }
    unreachable("");
}

static Node_for_lower_bound* change_op_for_lower_bound(Env* env, Node_for_lower_bound* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Node_for_upper_bound* change_op_for_upper_bound(Env* env, Node_for_upper_bound* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Node_function_def* change_op_function_def(Env* env, Node_function_def* root) {
    root->body = change_op_block(env, root->body);
    return root;
}

static Node_def* change_op_def(Env* env, Node_def* root) {
    switch (root->type) {
        case NODE_FUNCTION_DEF:
            return node_wrap_function_def(change_op_function_def(env, node_unwrap_function_def(root)));
        case NODE_VARIABLE_DEF:
            return root;
        case NODE_STRUCT_DEF:
            return root;
        case NODE_RAW_UNION_DEF:
            return root;
        case NODE_ENUM_DEF:
            return root;
        case NODE_PRIMITIVE_DEF:
            return root;
        case NODE_FUNCTION_DECL:
            return root;
        case NODE_LABEL:
            return root;
        case NODE_LITERAL_DEF:
            return root;
    }
    unreachable("");
}

static Node_condition* change_op_condition(Env* env, Node_condition* root) {
    root->child = change_op_operator(env, root->child);
    return root;
}

static Node_for_range* change_op_for_range(Env* env, Node_for_range* root) {
    root->lower_bound = change_op_for_lower_bound(env, root->lower_bound);
    root->upper_bound = change_op_for_upper_bound(env, root->upper_bound);
    root->body = change_op_block(env, root->body);
    return root;
}

static Node_for_with_cond* change_op_for_with_cond(Env* env, Node_for_with_cond* root) {
    root->condition = change_op_condition(env, root->condition);
    root->body = change_op_block(env, root->body);
    return root;
}

static Node_assignment* change_op_assignment(Env* env, Node_assignment* root) {
    root->lhs = change_op_node(env, root->lhs);
    root->rhs = change_op_expr(env, root->rhs);
    return root;
}

static Node_if* change_op_if(Env* env, Node_if* root) {
    root->condition = change_op_condition(env, root->condition);
    root->body = change_op_block(env, root->body);
    return root;
}

static Node_return* change_op_return(Env* env, Node_return* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Node_if_else_chain* change_op_if_else_chain(Env* env, Node_if_else_chain* root) {
    Node_if_ptr_vec new_children = {0};

    for (size_t idx = 0; idx < root->nodes.info.count; idx++) {
        Node_if* curr = vec_at(&root->nodes, idx);
        vec_append(&a_main, &new_children, change_op_if(env, curr));
    }

    root->nodes = new_children;
    return root;
}

static Node* change_op_node(Env* env, Node* root) {
    switch (root->type) {
        case NODE_BLOCK:
            return node_wrap_block(change_op_block(env, node_unwrap_block(root)));
        case NODE_EXPR:
            return node_wrap_expr(change_op_expr(env, node_unwrap_expr(root)));
        case NODE_FUNCTION_PARAMS:
            return root;
        case NODE_LANG_TYPE:
            return root;
        case NODE_FOR_LOWER_BOUND:
            return node_wrap_for_lower_bound(change_op_for_lower_bound(env, node_unwrap_for_lower_bound(root)));
        case NODE_FOR_UPPER_BOUND:
            return node_wrap_for_upper_bound(change_op_for_upper_bound(env, node_unwrap_for_upper_bound(root)));
        case NODE_DEF:
            return node_wrap_def(change_op_def(env, node_unwrap_def(root)));
        case NODE_CONDITION:
            return node_wrap_condition(change_op_condition(env, node_unwrap_condition(root)));
        case NODE_FOR_RANGE:
            return node_wrap_for_range(change_op_for_range(env, node_unwrap_for_range(root)));
        case NODE_FOR_WITH_COND:
            return node_wrap_for_with_cond(change_op_for_with_cond(env, node_unwrap_for_with_cond(root)));
        case NODE_BREAK:
            return root;
        case NODE_CONTINUE:
            return root;
        case NODE_ASSIGNMENT:
            return node_wrap_assignment(change_op_assignment(env, node_unwrap_assignment(root)));
        case NODE_IF:
            return node_wrap_if(change_op_if(env, node_unwrap_if(root)));
        case NODE_RETURN:
            return node_wrap_return(change_op_return(env, node_unwrap_return(root)));
        case NODE_IF_ELSE_CHAIN:
            return node_wrap_if_else_chain(change_op_if_else_chain(env, node_unwrap_if_else_chain(root)));
    }
    unreachable("");
}

static Node_block* change_op_block(Env* env, Node_block* root) {
    Node_block* new_block = node_block_new(root->pos);
    new_block->is_variadic = root->is_variadic;
    new_block->symbol_collection = root->symbol_collection;
    new_block->pos_end = root->pos_end;

    vec_append(&a_main, &env->ancesters, &root->symbol_collection);

    for (size_t idx = 0; idx < root->children.info.count; idx++) {
        Node* curr = vec_at(&root->children, idx);
        vec_append(&a_main, &new_block->children, change_op_node(env, curr));
    }

    vec_rem_last(&env->ancesters);

    return new_block;
}

Node_block* change_operators(Env* env, Node_block* root) {
    return change_op_block(env, root);
}
