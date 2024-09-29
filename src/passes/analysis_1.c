#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

static void do_assignment(Node* assignment) {
    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);


    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            if (!set_assignment_operand_types(lhs, rhs)) {
                todo();
                return;
            }
            assert(lhs->type != NODE_SYMBOL_UNTYPED);
            assert(rhs->type != NODE_SYMBOL_UNTYPED);
            break;
        case NODE_SYMBOL_UNTYPED: {
            if (!set_assignment_operand_types(lhs, rhs)) {
                return;
            }
            break;
        }
        case NODE_STRUCT_MEMBER_SYM: {
            set_struct_member_symbol_types(lhs);
            if (rhs->type == NODE_SYMBOL_UNTYPED) {
                set_symbol_type(rhs);
            }
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(lhs));
    }
}

static void do_function_definition(Node* fun_def) {
    (void) fun_def;
}

static void set_if_statement_types(Node* if_statement) {
    assert(if_statement->type == NODE_IF_STATEMENT);

    Node* if_condition = nodes_get_child_of_type(if_statement, NODE_IF_CONDITION);
    Node* if_cond_child = nodes_single_child(if_condition);
    switch (if_cond_child->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(if_cond_child);
            break;
        case NODE_OPERATOR: {
            Str_view dummy;
            try_set_operator_lang_type(&dummy, if_cond_child);
            break;
        }
        default:
            unreachable(NODE_FMT, node_print(if_cond_child));
    }
}

bool analysis_1(Node* start_node) {
    if (start_node->type != NODE_BLOCK) {
        return false;
    }

    nodes_foreach_child(curr_node, start_node) {
        switch (curr_node->type) {
            case NODE_ASSIGNMENT:
                do_assignment(curr_node);
                break;
            case NODE_FUNCTION_CALL:
                set_function_call_types(curr_node);
                break;
            case NODE_FUNCTION_DEFINITION:
                do_function_definition(curr_node);
                break;
            case NODE_SYMBOL_UNTYPED:
                todo();
                set_symbol_type(curr_node);
                break;
            case NODE_RETURN_STATEMENT:
                set_return_statement_types(curr_node);
                break;
            case NODE_OPERATOR:
                todo();
                break;
            case NODE_IF_STATEMENT:
                set_if_statement_types(curr_node);
                break;
            default:
                break;
        }
    }

    return false;
}
