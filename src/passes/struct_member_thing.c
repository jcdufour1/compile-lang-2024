#include "passes.h"
#include "../node.h"
#include "../nodes.h"
#include "../parser_utils.h"

static Node* struct_element_ptr_def_new(Str_view name, Str_view struct_def_type) {
    Node* var_def = node_new();
    var_def->name = name;
    var_def->type = NODE_STRUCT_ELEMENT_PTR_DEF;
    assert(struct_def_type.count > 0);
    var_def->lang_type = struct_def_type;
    var_def->token_type = TOKEN_NONTYPE;

    try(sym_tbl_add(var_def));
    return var_def;
}

static void replace_struct_member_call(Node* node_to_insert_before, Node* old_struct_memb_call) {
    assert(old_struct_memb_call->type == NODE_STRUCT_MEMBER_CALL);

    Node* var_def;
    try(sym_tbl_lookup(&var_def, old_struct_memb_call->name));

    Node* lhs = struct_element_ptr_def_new(literal_name_new(), var_def->lang_type);
    Node* new_thing = node_clone_self_and_children(old_struct_memb_call);
    new_thing->type = NODE_STRUCT_MEMBER_CALL_LOW_LEVEL;
    Node* assignment = assignment_new(lhs, new_thing);
    nodes_insert_before(node_to_insert_before, assignment);

    old_struct_memb_call->name = lhs->name;
    nodes_remove(nodes_single_child(old_struct_memb_call), true);
    //log(LOG_DEBUG, NODE_FMT"\n", node_print(old_struct_memb_call));
}

static void do_function_call(Node* node_to_insert_before, Node* fun_call) {
    assert(fun_call->type == NODE_FUNCTION_CALL);

    nodes_foreach_child(arg, fun_call) {
        switch (arg->type) {
            case NODE_STRUCT_MEMBER_CALL:
                replace_struct_member_call(node_to_insert_before, arg);
                break;
            default:
                break;
        }
    }
}

static void do_return_statement(Node* node_to_insert_before, Node* rtn_statement) {
    assert(rtn_statement->type == NODE_RETURN_STATEMENT);

    Node* child = nodes_single_child(rtn_statement);
    switch (child->type) {
        case NODE_LITERAL:
            break;
        case NODE_SYMBOL:
            break;
        case NODE_STRUCT_MEMBER_CALL:
            replace_struct_member_call(node_to_insert_before, child);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(child));
    }
}

static void do_assignment_operand_symbol(Node* node_to_insert_before, Node* operand) {
    assert(operand->type == NODE_SYMBOL);

    Node* var_def;
    try(sym_tbl_lookup(&var_def, operand->name));

    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }

    switch (var_def->type) {
        case NODE_VARIABLE_DEFINITION:
            return;
        default:
            unreachable(NODE_FMT"\n", node_print(var_def));
    }
}

static void do_assignment_operand(Node* node_to_insert_before, Node* operand) {
    switch (operand->type) {
        case NODE_STRUCT_ELEMENT_PTR_DEF:
            break;
        case NODE_SYMBOL:
            do_assignment_operand_symbol(node_to_insert_before, operand);
            break;
        case NODE_VARIABLE_DEFINITION:
            break;
        case NODE_FUNCTION_CALL:
            do_function_call(node_to_insert_before, operand);
            break;
        case NODE_STRUCT_MEMBER_CALL:
            replace_struct_member_call(node_to_insert_before, operand);
            break;
        case NODE_LITERAL:
            break;
        case NODE_STRUCT_LITERAL:
            break;
        case NODE_STRUCT_MEMBER_CALL_LOW_LEVEL:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static void do_assignment(Node* node_to_insert_before, Node* assignment) {
    do_assignment_operand(node_to_insert_before, nodes_get_child(assignment, 0));
    do_assignment_operand(node_to_insert_before, nodes_get_child(assignment, 1));
}

static void struct_member_thing_internal(Node* block) {
    assert(block->type == NODE_BLOCK);

    Node* block_element = nodes_get_local_rightmost(block->left_child);
    while (block_element) {
        bool advance_to_prev = true;
        switch (block_element->type) {
            case NODE_NO_TYPE:
                todo();
            case NODE_STRUCT_DEFINITION:
                break;
            case NODE_STRUCT_LITERAL:
                todo();
            case NODE_STRUCT_MEMBER_CALL:
                todo();
            case NODE_BLOCK:
                todo();
            case NODE_FUNCTION_DEFINITION:
                node_printf(block_element);
                node_printf(nodes_get_child_of_type(block_element, NODE_BLOCK))
                struct_member_thing_internal(nodes_get_child_of_type(block_element, NODE_BLOCK));
                break;
            case NODE_FUNCTION_PARAMETERS:
                todo();
            case NODE_FUNCTION_RETURN_TYPES:
                todo();
            case NODE_FUNCTION_CALL:
                do_function_call(block_element, block_element);
                break;
            case NODE_FUNCTION_PARAM_CALL:
                todo();
            case NODE_LITERAL:
                todo();
            case NODE_LANG_TYPE:
                todo();
            case NODE_OPERATOR:
                todo();
            case NODE_SYMBOL:
                todo();
            case NODE_RETURN_STATEMENT:
                do_return_statement(block_element, block_element);
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_FUNCTION_DECLARATION:
                break;
            case NODE_FOR_LOOP:
                todo();
            case NODE_FOR_LOWER_BOUND:
                todo();
            case NODE_FOR_UPPER_BOUND:
                todo();
            case NODE_FOR_VARIABLE_DEF:
                todo();
            case NODE_IF_STATEMENT:
                todo();
            case NODE_IF_CONDITION:
                todo();
            case NODE_ASSIGNMENT:
                node_printf(block_element);
                do_assignment(block_element, block_element);
                break;
            case NODE_GOTO:
                todo();
            case NODE_COND_GOTO:
                todo();
            case NODE_LABEL:
                todo();
            case NODE_ALLOCA:
                todo();
            case NODE_LOAD:
                todo();
            case NODE_LOAD_STRUCT_MEMBER:
                todo();
            case NODE_STORE:
                todo();
            case NODE_STORE_STRUCT_MEMBER:
                todo();
            default:
                unreachable(NODE_FMT"\n", node_print(block_element));
        }

        if (advance_to_prev) {
            block_element = block_element->prev;
        }
    }
}


bool struct_member_thing(Node* curr_node) {
    if (curr_node != root_of_tree) {
        return false;
    }

    struct_member_thing_internal(curr_node);
    return false;

}
