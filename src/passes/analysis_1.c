#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

static void do_symbol(const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL);

    Node* sym_def;
    try(sym_tbl_lookup(&sym_def, symbol->name));
}

void try_get_member_def_from_struct_member_symbol(Node** member_def, const Node* struct_memb_sym) {
    Node* curr_memb_def = NULL;
    Node* struct_def;
    Node* struct_var;
    if (!sym_tbl_lookup(&struct_var, struct_memb_sym->name)) {
        msg_undefined_symbol(struct_memb_sym);
        return;
    }
    if (!sym_tbl_lookup(&struct_def, struct_var->lang_type)) {
        todo();
    }
    bool is_struct = true;
    nodes_foreach_child(memb_sym, struct_memb_sym) {
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, struct_def, memb_sym)) {
            msg_invalid_struct_member(memb_sym);
        }
        if (!try_get_struct_definition(&struct_def, curr_memb_def)) {
            is_struct = false;
        }
    }
}

static void do_struct_member_symbol(const Node* memb_sym) {
    Node* memb_def;
    log_tree(LOG_DEBUG, memb_sym);
    try_get_member_def_from_struct_member_symbol(&memb_def, memb_sym);
}

static void do_assignment(Node* assignment) {
    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);
    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            rhs->lang_type = lhs->lang_type;
            break;
        case NODE_SYMBOL: {
            log_tree(LOG_DEBUG, assignment);
            Node* sym_def;
            try(sym_tbl_lookup(&sym_def, lhs->name));
            rhs->lang_type = sym_def->lang_type;
            break;
        }
        case NODE_STRUCT_MEMBER_SYM: {
            Node* struct_var_def;
            if (!try_get_struct_definition(&struct_var_def, lhs)) {
                msg_undefined_symbol(lhs);
                todo();
            }
            //rhs->lang_type = struct->lang_type;
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(lhs));
    }
}

static void do_function_call(Node* fun_call) {
    Node* fun_def;
    try(sym_tbl_lookup(&fun_def, fun_call->name));
    Node* params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    size_t params_idx = 0;

    nodes_foreach_child(argument, fun_call) {
        switch (argument->type) {
            case NODE_LITERAL:
                break;
            case NODE_SYMBOL:
                do_symbol(argument);
                break;
            case NODE_STRUCT_MEMBER_SYM:
                do_struct_member_symbol(argument);
                break;
            default:
                unreachable(NODE_FMT, node_print(argument));
        }

        Node* corresponding_param = nodes_get_child(params, params_idx);
        switch (corresponding_param->pointer_depth) {
            case 0:
                break;
            case 1:
                argument->pointer_depth = 1;
                break;
            default:
                todo();
        }

        if (!corresponding_param->is_variadic) {
            params_idx++;
        }
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
                do_function_call(curr_node);
                break;
            default:
                break;
        }
    }

    return false;
}
