#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static void insert_store_assignment(Node* node_to_insert_before, Node* assignment, bool is_for_struct_literal_member);

static void do_struct_literal(Node* struct_literal) {
    assert(struct_literal->type == NODE_STRUCT_LITERAL);

    Node* member = nodes_get_child(struct_literal, 0);
    while (member) {
        bool go_to_next = true;
        switch (member->type) {
            case NODE_ASSIGNMENT: {
                Node* assignment = member;
                insert_store_assignment(assignment, assignment, true);
                member = assignment->prev;
                nodes_remove(assignment, true);
                go_to_next = false;
                break;
            }
            case NODE_STORE_VARIABLE:
                break;
            default:
                unreachable(NODE_FMT"\n", node_print(member));
        }

        if (go_to_next) {
            member = member->next;
        }
    }
}

// returns node of element pointer that should then be loaded
static Node* do_load_struct_element_ptr(Node* node_to_insert_before, Node* symbol_call) {
    Node* prev_struct_sym = symbol_call;
    Node* node_element_ptr_to_load = get_storage_location(symbol_call);
    Node* load_element_ptr = NULL;
    nodes_foreach_child(element_sym, symbol_call) {
        assert(element_sym->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED);

        load_element_ptr = node_new(element_sym->pos);
        load_element_ptr->type = NODE_LOAD_STRUCT_ELEMENT_PTR;
        load_element_ptr->name = prev_struct_sym->name;
        Node* var_def;
        if (!sym_tbl_lookup(&var_def, prev_struct_sym->name)) {
            unreachable("");
        }
        Node* struct_def;
        if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
            unreachable("");
        }
        const Node* member_def = get_member_def(struct_def, element_sym);
        symbol_call->lang_type = member_def->lang_type;
        load_element_ptr->lang_type = member_def->lang_type;
        load_element_ptr->node_src = node_element_ptr_to_load;
        nodes_append_child(load_element_ptr, node_clone(element_sym));
        nodes_insert_before(node_to_insert_before, load_element_ptr);

        prev_struct_sym = element_sym;
        node_element_ptr_to_load = load_element_ptr;
    }
    assert(load_element_ptr);
    return load_element_ptr;
}

static Node* insert_load(Node* node_insert_load_before, Node* symbol_call) {
    switch (symbol_call->type) {
        case NODE_STRUCT_LITERAL:
            // fallthrough
        case NODE_LITERAL:
            return NULL;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    if (symbol_call->type == NODE_STRUCT_MEMBER_SYM_TYPED) {
        Node* load = node_new(symbol_call->pos);
        load->type = NODE_LOAD_ANOTHER_NODE;
        Node* load_element_ptr = do_load_struct_element_ptr(node_insert_load_before, symbol_call);
        load->node_src = load_element_ptr;
        load->lang_type = load_element_ptr->lang_type;
        assert(load->lang_type.count > 0);
        nodes_insert_before(node_insert_load_before, load);
        symbol_call->node_src = load;
        return load;
    } else {
        Node* sym_def;
        try(sym_tbl_lookup(&sym_def, symbol_call->name));
        Node* load = node_new(sym_def->pos);
        load->type = NODE_LOAD_ANOTHER_NODE;
        load->name = symbol_call->name;
        load->node_src = get_storage_location(symbol_call);
        load->lang_type = sym_def->lang_type;
        assert(load->lang_type.count > 0);
        symbol_call->node_src = load;
        nodes_insert_before(node_insert_load_before, load);
        return load;
    }
}

static void insert_store(Node* node_insert_store_before, Node* symbol_call /* src */) {
    switch (symbol_call->type) {
        case NODE_LITERAL:
            return;
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            node_printf(symbol_call);
            assert(symbol_call->name.count > 0);
            break;
        case NODE_FUNCTION_PARAM_SYM:
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    if (symbol_call->type == NODE_STRUCT_MEMBER_SYM_TYPED) {
        todo();
    } else {
        Node* store = node_new(symbol_call->pos);
        store->type = NODE_STORE_ANOTHER_NODE;
        store->name = symbol_call->name;
        store->node_src = symbol_call->node_src;
        store->lang_type = symbol_call->lang_type;
        assert(store->lang_type.count > 0);
        store->node_dest = get_storage_location(symbol_call);
        assert(store->node_src);
        assert(store->node_dest);
        store->storage_location = get_storage_location(symbol_call);
        //symbol_call->node_to_load = store;
        nodes_append_child(store, symbol_call);
        nodes_insert_before(node_insert_store_before, store);

        switch (symbol_call->type) {
            case NODE_SYMBOL_TYPED:
                todo();
                symbol_call->type = NODE_LLVM_SYMBOL;
                symbol_call->node_src = get_storage_location(symbol_call);
                assert(symbol_call->node_src);
                // fallthrough
            case NODE_VARIABLE_DEFINITION:
                //todo();
                break;
            case NODE_FUNCTION_PARAM_SYM:
                //todo();
                break;
            default:
                node_printf(symbol_call);
                todo();
        }
    }
    //Node* store = node_new();
    //store->type = NODE_STORE;
    //store->name = symbol_call->name;
    //nodes_append_child(store, symbol_call);
    //nodes_insert_before(node_insert_store_before, store);
}

static void load_operator_operand(Node* node_insert_before, Node* operand) {
    switch (operand->type) {
        case NODE_OPERATOR:
            unreachable("nested operators should not still be present at this point");
        case NODE_LITERAL:
            break;
        case NODE_VARIABLE_DEFINITION:
            todo();
        case NODE_SYMBOL_TYPED:
            insert_load(node_insert_before, operand);
            break;
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            break;
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static Node* load_operator_operands(Node* node_insert_before, Node* operator) {
    node_printf(operator);
    assert(operator->type == NODE_OPERATOR);
    assert(nodes_count_children(operator) == 2);

    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

    load_operator_operand(node_insert_before, lhs);
    load_operator_operand(node_insert_before, rhs);

    operator->lang_type = str_view_from_cstr("i32");
    return operator;
}

static void add_load_foreach_arg(Node* node_insert_before, Node* function_call) {
    nodes_foreach_child(argument, function_call) {
        insert_load(node_insert_before, argument);
    }
}

static void insert_store_assignment(Node* node_to_insert_before, Node* assignment, bool is_for_struct_literal_member) {
    assert(assignment->type == NODE_ASSIGNMENT);

    log_tree(LOG_DEBUG, assignment);
    Node* lhs = nodes_get_child(assignment, 0);
    Node* rhs = nodes_get_child(assignment, 1);
    Node* rhs_load = NULL;

    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            nodes_remove(lhs, false);
            nodes_insert_before(node_to_insert_before, lhs);
            break;
        case NODE_SYMBOL_TYPED:
            node_printf(lhs);
            break;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            break;
        default:
            unreachable(NODE_FMT, node_print(lhs));
    }
   
    switch (rhs->type) {
        case NODE_VARIABLE_DEFINITION:
            todo();
            break;
        case NODE_STRUCT_LITERAL:
            do_struct_literal(rhs);
            //rhs_load = insert_load(node_to_insert_before, rhs);
            //assert(rhs_load->lang_type.count > 0);
            break;
        case NODE_SYMBOL_TYPED:
            rhs_load = insert_load(node_to_insert_before, rhs);
            assert(rhs_load);
            assert(rhs_load->lang_type.count > 0);
            break;
        case NODE_LITERAL:
            break;
        case NODE_OPERATOR:
            unreachable("operator should not still be present");
            break;
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            rhs_load = rhs->node_src;
            break;
        case NODE_FUNCTION_CALL:
            rhs_load = rhs;
            nodes_remove(rhs_load, true);
            nodes_insert_before(node_to_insert_before, rhs_load);
            Node* fun_def;
            try(sym_tbl_lookup(&fun_def, rhs->name));
            Node* function_rtn_type = nodes_single_child(nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES));
            rhs_load->lang_type = function_rtn_type->lang_type;
            assert(rhs_load);
            assert(rhs_load->lang_type.count > 0);
            break;
        default:
            node_printf(rhs)
            todo();
    }

    if (is_for_struct_literal_member) {
        Node* store = node_new(lhs->pos);
        store->name = lhs->name;
        node_printf(lhs);
        nodes_remove(rhs, true);
        store->type = NODE_STORE_VARIABLE;
        nodes_append_child(store, rhs);
        nodes_insert_before(node_to_insert_before, store);
    } else if (lhs->type == NODE_STRUCT_MEMBER_SYM_TYPED) {
        if (rhs->type == NODE_STRUCT_LITERAL) {
            todo();
        }
        nodes_remove(lhs, true);
        //nodes_remove(rhs, true);
        Node* store = node_new(lhs->pos);
        Node* store_element_ptr = do_load_struct_element_ptr(node_to_insert_before, lhs);
        if (rhs->type == NODE_LITERAL) {
            store->type = NODE_LLVM_STORE_LITERAL;
            nodes_append_child(store, node_clone(rhs));
        } else {
            store->type = NODE_STORE_ANOTHER_NODE;
            store->node_src = rhs_load;
            assert(store->node_src);
        }
        store->node_dest = store_element_ptr;
        store->lang_type = store_element_ptr->lang_type;
        nodes_insert_after(assignment, store);
        nodes_insert_after(assignment, lhs);
    } else {
        if (rhs->type == NODE_LITERAL) {
            Node* store = node_new(rhs->pos);
            store->type = NODE_LLVM_STORE_LITERAL;
            store->name = lhs->name;
            store->node_dest = get_storage_location(lhs);
            store->lang_type = rhs->lang_type;
            assert(store->lang_type.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            node_printf(lhs);
            nodes_remove(rhs, true);
            nodes_append_child(store, rhs);
            nodes_insert_before(node_to_insert_before, store);
        } else if (rhs->type == NODE_STRUCT_LITERAL) {
            Node* store = node_new(lhs->pos);
            store->name = lhs->name;
            node_printf(lhs);
            nodes_remove(rhs, true);
            store->type = NODE_LLVM_STORE_STRUCT_LITERAL;
            nodes_append_child(store, rhs);
            nodes_insert_before(node_to_insert_before, store);
        } else {
            Node* store = node_new(lhs->pos);
            store->type = NODE_STORE_ANOTHER_NODE;
            store->name = lhs->name;
            store->node_dest = get_storage_location(lhs);
            store->node_src = rhs_load;
            if (rhs->type == NODE_STRUCT_LITERAL) {
                unreachable("");
            }
            store->lang_type = rhs_load->lang_type;
            assert(store->lang_type.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            node_printf(lhs);
            //nodes_remove(rhs, true);
            nodes_insert_before(node_to_insert_before, store);
        }
    }
}

static void add_load_return_statement(Node* return_statement) {
    Node* node_to_return = nodes_single_child(return_statement);
    switch (node_to_return->type) {
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            insert_load(return_statement, node_to_return);
            return;
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_OPERATOR:
            unreachable("operator should not still be the child of return statement at this point");
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_to_return));
    }
}

static Node* get_node_after_last_alloca(Node* fun_block) {
    nodes_foreach_child(node, fun_block) {
        if (node->type != NODE_ALLOCA) {
            return node;
        }
    }
    unreachable("");
}

static void load_function_parameters(Node* fun_def) {
    assert(fun_def->type == NODE_FUNCTION_DEFINITION);
    Node* fun_params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    Node* fun_block = nodes_get_child_of_type(fun_def, NODE_BLOCK);

    nodes_foreach_child(param, fun_params) {
        if (is_corresponding_to_a_struct(param)) {
            continue;
        }
        Node* fun_param_call = symbol_new(param->name, param->pos);
        fun_param_call->type = NODE_FUNCTION_PARAM_SYM;
        fun_param_call->node_src = param;
        fun_param_call->lang_type = param->lang_type;
        insert_store(get_node_after_last_alloca(fun_block), fun_param_call);
    }
}

static void load_function_arguments(Node* fun_call) {
    switch (fun_call->parent->type) {
        case NODE_STORE_VARIABLE:
            add_load_foreach_arg(fun_call->parent, fun_call);
            return;
        case NODE_BLOCK:
            // fallthrough
        case NODE_ASSIGNMENT:
            add_load_foreach_arg(fun_call, fun_call);
            return;
        default:
            unreachable(NODE_FMT"\n", node_print(fun_call->parent));
    }
}

bool add_load_and_store(Node* start_start_node) {
    if (!start_start_node->left_child) {
        return false;
    }
    if (start_start_node->type != NODE_BLOCK) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(start_start_node->left_child);
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_STRUCT_LITERAL:
                do_struct_literal(curr_node);
                break;
            case NODE_LITERAL:
                break;
            case NODE_FUNCTION_CALL:
                load_function_arguments(curr_node);
                break;
            case NODE_FUNCTION_DEFINITION:
                load_function_parameters(curr_node);
                break;
            case NODE_FUNCTION_PARAMETERS:
                break;
            case NODE_FUNCTION_PARAM_SYM:
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                break;
            case NODE_LANG_TYPE:
                break;
            case NODE_OPERATOR:
                load_operator_operands(curr_node, curr_node);
                break;
            case NODE_BLOCK:
                break;
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbol should not still be present");
            case NODE_SYMBOL_TYPED:
                break;
            case NODE_RETURN_STATEMENT:
                add_load_return_statement(curr_node);
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_FUNCTION_DECLARATION:
                break;
            case NODE_ASSIGNMENT: {
                Node* assignment = curr_node;
                insert_store_assignment(assignment, assignment, false);
                curr_node = assignment->prev;
                nodes_remove(assignment, true);
                go_to_prev = false;
                break;
            }
            case NODE_FOR_LOOP:
                unreachable("for loop node should not still exist at this point\n");
                todo();
            case NODE_FOR_VARIABLE_DEF:
                todo();
            case NODE_FOR_LOWER_BOUND:
                todo();
            case NODE_FOR_UPPER_BOUND:
                todo();
            case NODE_GOTO:
                break;
            case NODE_COND_GOTO:
                break;
            case NODE_NO_TYPE:
                todo();
            case NODE_LABEL:
                break;
            case NODE_ALLOCA:
                break;
            case NODE_STORE_VARIABLE:
                break;
            case NODE_LOAD_VARIABLE:
                break;
            case NODE_LOAD_ANOTHER_NODE:
                break;
            case NODE_STORE_ANOTHER_NODE:
                break;
            case NODE_LOAD_STRUCT_ELEMENT_PTR:
                break;
            case NODE_IF_STATEMENT:
                unreachable("if statement node should not still exist at this point\n");
            case NODE_LOAD_STRUCT_MEMBER:
                break;
            case NODE_STORE_STRUCT_MEMBER:
                break;
            case NODE_STRUCT_MEMBER_SYM_TYPED:
                break;
            case NODE_OPERATOR_RETURN_VALUE_SYM:
                break;
            case NODE_STRUCT_DEFINITION:
                break;
            case NODE_LLVM_STORE_LITERAL:
                break;
            case NODE_LLVM_STORE_STRUCT_LITERAL:
                break;
            default:
                unreachable(NODE_FMT"\n", node_print(curr_node));
        }

        if (go_to_prev) {
            curr_node = curr_node->prev;
        }
    }

    return false;
}
