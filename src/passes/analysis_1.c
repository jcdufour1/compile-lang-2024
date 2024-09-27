#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"
#include "../error_msg.h"

static void do_symbol(const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL);

    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, symbol->name)) {
        msg_undefined_symbol(symbol);
    }
}

static void do_struct_member_symbol(const Node* struct_memb_sym) {
    Node* curr_memb_def = NULL;
    Node* struct_def;
    Node* struct_var;
    if (!sym_tbl_lookup(&struct_var, struct_memb_sym->name)) {
        msg_undefined_symbol(struct_memb_sym);
        return;
    }
    if (!sym_tbl_lookup(&struct_def, struct_var->lang_type)) {
        todo(); // this should possibly never happen
    }
    bool is_struct = true;
    nodes_foreach_child(memb_sym, struct_memb_sym) {
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, struct_def, memb_sym)) {
            msg_invalid_struct_member(memb_sym);
            return;
        }
        if (!try_get_struct_definition(&struct_def, curr_memb_def)) {
            is_struct = false;
        }
    }
}

static bool try_get_operator_literal_lang_type(Str_view* result, const Node* literal) {
    assert(literal->type == NODE_LITERAL);

    switch (literal->token_type) {
        case TOKEN_STRING_LITERAL:
            *result = str_view_from_cstr("ptr");
            return true;
        case TOKEN_NUM_LITERAL:
            *result = str_view_from_cstr("i32");
            return true;
        default:
            unreachable("");
    }
}

// returns false if unsuccessful
static bool try_get_operator_lang_type(Str_view* result, const Node* operator) {
    assert(operator->type == NODE_OPERATOR);

    const Node* lhs = nodes_get_child_const(operator, 0);
    switch (lhs->type) {
        case NODE_LITERAL: {
            if (!try_get_operator_literal_lang_type(result, lhs)) {
                todo();
            }
            return true;
        }
        case NODE_OPERATOR:
            return try_get_operator_lang_type(result, lhs);
        case NODE_SYMBOL: {
            Node* sym_def;
            if (!sym_tbl_lookup(&sym_def, lhs->name)) {
                msg_undefined_symbol(lhs);
                return false;
            }
            if (is_struct_variable_definition(sym_def)) {
                todo();
            }
            *result = sym_def->lang_type;
            return true;
        }
        default:
            unreachable(NODE_FMT, node_print(lhs));
    }
    unreachable("");
}

static bool symbol_assignment_types_are_compatible(const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL);
    bool are_compatible = true;

    switch (rhs->type) {
        case NODE_LITERAL: {
            if (is_corresponding_to_a_struct(lhs)) {
                // struct assigned literal of invalid type
                meg_struct_assigned_to_invalid_literal(lhs, rhs);
                todo();
                are_compatible = false;
                break;
            }
            Node* lhs_def;
            try(sym_tbl_lookup(&lhs_def, lhs->name));
            Str_view lhs_lang_type = lhs_def->lang_type;

            Str_view rhs_lang_type;
            if (!try_get_operator_literal_lang_type(&rhs_lang_type, rhs)) {
                msg_invalid_assignment_to_literal(lhs, rhs);
                are_compatible = false;
            }
            if (!str_view_is_equal(lhs_lang_type, rhs_lang_type)) {
                msg_invalid_assignment_to_literal(lhs, rhs);
            }
            break;
        }
        case NODE_STRUCT_LITERAL: {
            if (!is_corresponding_to_a_struct(lhs)) {
                todo(); // non_struct assigned struct literal
            }
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            Node* struct_def;
            try(sym_tbl_lookup(&struct_def, lhs_var_def->lang_type));
            size_t idx = 0;
            nodes_foreach_child(memb_sym_def, struct_def) {
                const Node* assign_memb_sym = nodes_get_child_const(rhs, idx);
                const Node* memb_sym = nodes_get_child_const(assign_memb_sym, 0);
                const Node* assign_memb_sym_rhs = nodes_get_child_const(assign_memb_sym, 1);
                if (assign_memb_sym_rhs->type != NODE_LITERAL) {
                    todo();
                }
                if (!str_view_is_equal(memb_sym_def->name, memb_sym->name)) {
                    msg_invalid_struct_member_assignment_in_literal(
                        lhs_var_def,
                        memb_sym_def,
                        memb_sym
                    );
                    are_compatible = false;
                }

                idx++;
            }
            break;
        }
        case NODE_SYMBOL: {
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            Node* rhs_var_def;
            try(sym_tbl_lookup(&rhs_var_def, rhs->name));
            if (!str_view_is_equal(lhs_var_def->lang_type, rhs_var_def->lang_type)) {
                todo();
            }
            break;
        }
        case NODE_OPERATOR: {
            Str_view rhs_lang_type;
            if (!try_get_operator_lang_type(&rhs_lang_type, rhs)) {
                break;
            }
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            if (!str_view_is_equal(rhs_lang_type, lhs_var_def->lang_type)) {
                msg_invalid_assignment_to_operation(lhs, rhs, rhs_lang_type);
                are_compatible = false;
            }
            break;
        }
        case NODE_FUNCTION_CALL: {
            Node* fun_def;
            if (!sym_tbl_lookup(&fun_def, rhs->name)) {
                msg_undefined_function(rhs);
            }
            break;
        }
        default:
            unreachable("rhs: "NODE_FMT"\n", node_print(rhs));
    }

    return are_compatible;
}

static void do_assignment(Node* assignment) {
    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);

    Str_view lhs_lang_type;
    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            rhs->lang_type = lhs->lang_type;
            break;
        case NODE_SYMBOL: {
            Node* sym_def;
            if (!sym_tbl_lookup(&sym_def, lhs->name)) {
                msg_undefined_symbol(lhs);
                return;
            }
            if (!symbol_assignment_types_are_compatible(lhs, rhs)) {
                return;
            }
            lhs_lang_type = sym_def->lang_type;
            rhs->lang_type = lhs_lang_type;
            break;
        }
        case NODE_STRUCT_MEMBER_SYM: {
            Node* struct_var_def;
            if (!try_get_struct_definition(&struct_var_def, lhs)) {
                msg_undefined_symbol(lhs);
                return;
            }
            lhs_lang_type = struct_var_def->lang_type;
            //rhs->lang_type = struct->lang_type;
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(lhs));
    }
}

static void do_function_call(Node* fun_call) {
    Node* fun_def;
    if (!sym_tbl_lookup(&fun_def, fun_call->name)) {
        msg_undefined_function(fun_call);
        return;
    }
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
