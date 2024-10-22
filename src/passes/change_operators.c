
#include "../nodes.h"
#include "../parser_utils.h"
#include "passes.h"

static void do_change_operator(Node_operator* operator) {
    Pos pos = node_wrap(operator)->pos;
    if (operator->type == NODE_OP_UNARY) {
        Node_unary* unary = node_unwrap_op_unary(operator);
        switch (unary->token_type) {
            case TOKEN_NOT: {
                Node_unary temp = *unary;
                operator->type = NODE_OP_BINARY;
                Node_binary* binary = node_unwrap_op_binary(operator);
                binary->lhs = temp.child;
                binary->rhs = node_wrap(literal_new(
                    str_view_from_cstr("0"),
                    TOKEN_NUM_LITERAL,
                    pos
                ));
                binary->token_type = TOKEN_DOUBLE_EQUAL;
                break;
            }
            default:
                break;
                //unreachable(TOKEN_TYPE_FMT"\n", token_type_print(unary->token_type));
        }
    } else if (operator->type == NODE_OP_BINARY) {
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
    switch (curr_node->type) {
        case NODE_OPERATOR:
            do_change_operator(node_unwrap_operation(curr_node));
            break;
        default:
            break;
    }

    return;
}
