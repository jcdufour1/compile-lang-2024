
#include "../nodes.h"
#include "../parser_utils.h"
#include "passes.h"

// TODO: rewrite this pass, because this is unsafe
// TODO: do not change type of existing node (or find better way), because this is error prone
static void do_change_operator(Node_operator* operator) {
    Pos pos = node_wrap_expr(node_wrap_operator(operator))->pos;
    if (operator->type == NODE_UNARY) {
        Node_unary* unary = node_unwrap_unary(operator);
        switch (unary->token_type) {
            case TOKEN_NOT: {
                Node_unary temp = *unary;
                operator->type = NODE_BINARY;
                Node_binary* binary = node_unwrap_binary(operator);
                binary->lhs = temp.child;
                binary->rhs = node_wrap_literal(util_literal_new_from_int64_t(
                    0,
                    TOKEN_INT_LITERAL,
                    pos
                ));
                binary->token_type = TOKEN_DOUBLE_EQUAL;
                binary->lang_type = get_lang_type_expr(temp.child);
                assert(binary->lang_type.str.count > 0);
                break;
            }
            default:
                break;
                //unreachable(TOKEN_TYPE_FMT"\n", token_type_print(unary->token_type));
        }
    } else if (operator->type == NODE_BINARY) {
        return;
    } else {
        unreachable("");
    }
}

void change_operators(Env* env) {
    Node* curr_node = vec_top(&env->ancesters);
    if (!curr_node) {
        return;
    }
    if (curr_node->type != NODE_EXPR) {
        return;
    }

    Node_expr* expr = node_unwrap_expr(curr_node);
    switch (expr->type) {
        case NODE_OPERATOR:
            do_change_operator(node_unwrap_operator(expr));
            break;
        default:
            break;
    }

    return;
}
