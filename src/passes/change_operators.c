
#include "../nodes.h"
#include "../parser_utils.h"
#include "passes.h"

// TODO: rewrite this pass, because this is unsafe
// TODO: do not change type of existing node, because it is error prone
static void do_change_operator(Node_operator* operator) {
    log_tree(LOG_DEBUG, node_wrap(operator));
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
                    TOKEN_INT_LITERAL,
                    pos
                ));
                binary->token_type = TOKEN_DOUBLE_EQUAL;
                binary->lang_type = get_lang_type(temp.child);
                assert(binary->lang_type.str.count > 0);
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
