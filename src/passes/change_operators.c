
#include "../tasts.h"
#include "../parser_utils.h"
#include "passes.h"

static Tast* change_op_tast(Env* env, Tast* root);

static Tast_block* change_op_block(Env* env, Tast_block* root);

static Tast_expr* change_op_expr(Env* env, Tast_expr* root);

static Tast_binary* change_op_binary(Env* env, Tast_binary* root) {
    (void) env;
    return root;
}

static Tast_operator* change_op_unary(Env* env, Tast_unary* unary) {
    switch (unary->token_type) {
        case TOKEN_NOT: {
            Tast_expr* new_lhs = change_op_expr(env, unary->child);
            Tast_binary* new_bin = tast_binary_new(
                tast_get_pos_unary(unary),
                new_lhs,
                tast_wrap_literal(util_tast_literal_new_from_int64_t(
                    0,
                    TOKEN_INT_LITERAL,
                    tast_get_pos_unary(unary)
                )),
                TOKEN_DOUBLE_EQUAL,
                tast_get_lang_type_expr(new_lhs)
            );

            assert(new_bin->lang_type.str.count > 0);
            return tast_wrap_binary(new_bin);
        }
        default:
            return tast_wrap_unary(unary);
    }
    unreachable("");
}

static Tast_operator* change_op_operator(Env* env, Tast_operator* root) {
    switch (root->type) {
        case TAST_BINARY:
            return tast_wrap_binary(change_op_binary(env, tast_unwrap_binary(root)));
        case TAST_UNARY:
            return change_op_unary(env, tast_unwrap_unary(root));
    }
    unreachable("");
}

static Tast_expr* change_op_expr(Env* env, Tast_expr* root) {
    switch (root->type) {
        case TAST_OPERATOR:
            return tast_wrap_operator(change_op_operator(env, tast_unwrap_operator(root)));
        case TAST_SYMBOL_UNTYPED:
            return root;
        case TAST_SYMBOL_TYPED:
            return root;
        case TAST_MEMBER_ACCESS_UNTYPED:
            return root;
        case TAST_MEMBER_ACCESS_TYPED:
            return root;
        case TAST_INDEX_UNTYPED:
            return root;
        case TAST_INDEX_TYPED:
            return root;
        case TAST_LITERAL:
            return root;
        case TAST_FUNCTION_CALL:
            return root;
        case TAST_STRUCT_LITERAL:
            return root;
    }
    unreachable("");
}

static Tast_for_lower_bound* change_op_for_lower_bound(Env* env, Tast_for_lower_bound* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Tast_for_upper_bound* change_op_for_upper_bound(Env* env, Tast_for_upper_bound* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Tast_function_def* change_op_function_def(Env* env, Tast_function_def* root) {
    root->body = change_op_block(env, root->body);
    return root;
}

static Tast_def* change_op_def(Env* env, Tast_def* root) {
    switch (root->type) {
        case TAST_FUNCTION_DEF:
            return tast_wrap_function_def(change_op_function_def(env, tast_unwrap_function_def(root)));
        case TAST_VARIABLE_DEF:
            return root;
        case TAST_STRUCT_DEF:
            return root;
        case TAST_RAW_UNION_DEF:
            return root;
        case TAST_ENUM_DEF:
            return root;
        case TAST_PRIMITIVE_DEF:
            return root;
        case TAST_FUNCTION_DECL:
            return root;
        case TAST_LITERAL_DEF:
            return root;
    }
    unreachable("");
}

static Tast_condition* change_op_condition(Env* env, Tast_condition* root) {
    root->child = change_op_operator(env, root->child);
    return root;
}

static Tast_for_range* change_op_for_range(Env* env, Tast_for_range* root) {
    root->lower_bound = change_op_for_lower_bound(env, root->lower_bound);
    root->upper_bound = change_op_for_upper_bound(env, root->upper_bound);
    root->body = change_op_block(env, root->body);
    return root;
}

static Tast_for_with_cond* change_op_for_with_cond(Env* env, Tast_for_with_cond* root) {
    root->condition = change_op_condition(env, root->condition);
    root->body = change_op_block(env, root->body);
    return root;
}

static Tast_assignment* change_op_assignment(Env* env, Tast_assignment* root) {
    root->lhs = change_op_tast(env, root->lhs);
    root->rhs = change_op_expr(env, root->rhs);
    return root;
}

static Tast_if* change_op_if(Env* env, Tast_if* root) {
    root->condition = change_op_condition(env, root->condition);
    root->body = change_op_block(env, root->body);
    return root;
}

static Tast_return* change_op_return(Env* env, Tast_return* root) {
    root->child = change_op_expr(env, root->child);
    return root;
}

static Tast_if_else_chain* change_op_if_else_chain(Env* env, Tast_if_else_chain* root) {
    Tast_if_vec new_children = {0};

    for (size_t idx = 0; idx < root->tasts.info.count; idx++) {
        Tast_if* curr = vec_at(&root->tasts, idx);
        vec_append(&a_main, &new_children, change_op_if(env, curr));
    }

    root->tasts = new_children;
    return root;
}

static Tast* change_op_tast(Env* env, Tast* root) {
    switch (root->type) {
        case TAST_BLOCK:
            return tast_wrap_block(change_op_block(env, tast_unwrap_block(root)));
        case TAST_EXPR:
            return tast_wrap_expr(change_op_expr(env, tast_unwrap_expr(root)));
        case TAST_FUNCTION_PARAMS:
            return root;
        case TAST_LANG_TYPE:
            return root;
        case TAST_FOR_LOWER_BOUND:
            return tast_wrap_for_lower_bound(change_op_for_lower_bound(env, tast_unwrap_for_lower_bound(root)));
        case TAST_FOR_UPPER_BOUND:
            return tast_wrap_for_upper_bound(change_op_for_upper_bound(env, tast_unwrap_for_upper_bound(root)));
        case TAST_DEF:
            return tast_wrap_def(change_op_def(env, tast_unwrap_def(root)));
        case TAST_CONDITION:
            return tast_wrap_condition(change_op_condition(env, tast_unwrap_condition(root)));
        case TAST_FOR_RANGE:
            return tast_wrap_for_range(change_op_for_range(env, tast_unwrap_for_range(root)));
        case TAST_FOR_WITH_COND:
            return tast_wrap_for_with_cond(change_op_for_with_cond(env, tast_unwrap_for_with_cond(root)));
        case TAST_BREAK:
            return root;
        case TAST_CONTINUE:
            return root;
        case TAST_ASSIGNMENT:
            return tast_wrap_assignment(change_op_assignment(env, tast_unwrap_assignment(root)));
        case TAST_IF:
            return tast_wrap_if(change_op_if(env, tast_unwrap_if(root)));
        case TAST_RETURN:
            return tast_wrap_return(change_op_return(env, tast_unwrap_return(root)));
        case TAST_IF_ELSE_CHAIN:
            return tast_wrap_if_else_chain(change_op_if_else_chain(env, tast_unwrap_if_else_chain(root)));
    }
    unreachable("");
}

static Tast_block* change_op_block(Env* env, Tast_block* root) {
    Tast_block* new_block = tast_block_new(
        root->pos,
        root->is_variadic,
        (Tast_vec) {0},
        root->symbol_collection,
        root->pos_end
    );

    vec_append(&a_main, &env->ancesters, &root->symbol_collection);

    for (size_t idx = 0; idx < root->children.info.count; idx++) {
        Tast* curr = vec_at(&root->children, idx);
        vec_append(&a_main, &new_block->children, change_op_tast(env, curr));
    }

    vec_rem_last(&env->ancesters);

    return new_block;
}

Tast_block* change_operators(Env* env, Tast_block* root) {
    return change_op_block(env, root);
}
