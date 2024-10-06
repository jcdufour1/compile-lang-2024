#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static void insert_store_assignment(Node* node_to_insert_before, Node_assignment* assignment, bool is_for_struct_literal_member);

static void do_struct_literal(Node_struct_literal* struct_literal) {
    Node* member = nodes_get_child(node_wrap(struct_literal), 0);
    while (member) {
        bool go_to_next = true;
        switch (member->type) {
            case NODE_ASSIGNMENT: {
                Node* assignment = member;
                insert_store_assignment(assignment, node_unwrap_assignment(assignment), true);
                member = assignment->prev;
                nodes_remove(assignment, true);
                go_to_next = false;
                break;
            }
            case NODE_LITERAL:
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
static Node_load_element_ptr* do_load_struct_element_ptr(Node* node_to_insert_before, Node* symbol_call) {
    Node* prev_struct_sym = symbol_call;
    node_printf(symbol_call);
    Node* sym_def_;
    if (!sym_tbl_lookup(&sym_def_, get_node_name(symbol_call))) {
        unreachable("symbol definition not found");
    }
    node_printf(sym_def_);
    Node* node_element_ptr_to_load = get_storage_location(symbol_call);
    Node_load_element_ptr* load_element_ptr = NULL;
    nodes_foreach_child(element_sym_, symbol_call) {
        Node_struct_member_sym_piece_typed* element_sym = node_unwrap_struct_member_sym_piece_typed(element_sym_);

        load_element_ptr = node_unwrap_load_elem_ptr(node_new(node_wrap(element_sym)->pos, NODE_LOAD_STRUCT_ELEMENT_PTR));
        load_element_ptr->name = get_node_name(prev_struct_sym); // put this in switch
        load_element_ptr->lang_type = element_sym->lang_type;
        load_element_ptr->struct_index = element_sym->struct_index;
        load_element_ptr->node_src = node_element_ptr_to_load;
        nodes_append_child(node_wrap(load_element_ptr), node_clone(node_wrap(element_sym)));
        nodes_insert_before(node_wrap(node_to_insert_before), node_wrap(load_element_ptr));

        prev_struct_sym = node_wrap(element_sym);
        node_element_ptr_to_load = node_wrap(load_element_ptr);
    }
    assert(load_element_ptr);
    return load_element_ptr;
}

static Node_load_another_node* insert_load(Node* node_insert_load_before, Node* symbol_call) {
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
        Node_load_another_node* load = node_unwrap_load_another_node(node_new(symbol_call->pos, NODE_LOAD_ANOTHER_NODE));
        Node_load_element_ptr* load_element_ptr = do_load_struct_element_ptr(node_insert_load_before, symbol_call);
        load->node_src = node_wrap(load_element_ptr);
        load->lang_type = load_element_ptr->lang_type;
        assert(load->lang_type.str.count > 0);
        nodes_insert_before(node_insert_load_before, node_wrap(load));
        node_unwrap_struct_member_sym_typed(symbol_call)->node_src = node_wrap(load);
        return load;
    } else {
        Node* sym_def;
        try(sym_tbl_lookup(&sym_def, get_node_name(symbol_call)));
        Node_load_another_node* load = node_unwrap_load_another_node(node_new(sym_def->pos, NODE_LOAD_ANOTHER_NODE));
        load->node_src = get_storage_location(symbol_call);
        load->lang_type = node_unwrap_variable_def(sym_def)->lang_type;
        assert(load->lang_type.str.count > 0);
        switch (symbol_call->type) {
            case NODE_SYMBOL_TYPED:
                node_unwrap_symbol_typed(symbol_call)->node_src = node_wrap(load);
                break;
            default:
                unreachable(NODE_FMT, node_print(symbol_call));
        }
        //symbol_call->node_src = load; // TODO: place this in switch
        nodes_insert_before(node_insert_load_before, node_wrap(load));
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
            assert(get_node_name(symbol_call).count > 0);
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
        Node_store_another_node* store = node_unwrap_store_another_node(node_new(symbol_call->pos, NODE_STORE_ANOTHER_NODE));
        store->node_src = get_node_src(symbol_call);
        store->lang_type = get_lang_type(symbol_call);
        assert(store->lang_type.str.count > 0);
        node_printf(symbol_call);
        store->node_dest = get_storage_location(symbol_call);
        assert(store->node_src);
        assert(store->node_dest);
        store->storage_location = get_storage_location(symbol_call);
        //symbol_call->node_to_load = store;
        nodes_append_child(node_wrap(store), symbol_call);
        nodes_insert_before(node_insert_store_before, node_wrap(store));

        switch (symbol_call->type) {
            case NODE_SYMBOL_TYPED:
                symbol_call->type = NODE_LLVM_SYMBOL;
                node_unwrap_llvm_symbol(symbol_call)->node_src = get_storage_location(symbol_call);
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

static Node_operator* load_operator_operands(Node* node_insert_before, Node_operator* operator) {
    assert(nodes_count_children(node_wrap(operator)) == 2);

    Node* lhs = nodes_get_child(node_wrap(operator), 0);
    Node* rhs = nodes_get_child(node_wrap(operator), 1);

    load_operator_operand(node_insert_before, lhs);
    load_operator_operand(node_insert_before, rhs);

    operator->lang_type = lang_type_from_cstr("i32", 0);
    return operator;
}

static void add_load_foreach_arg(Node* node_insert_before, Node* function_call) {
    nodes_foreach_child(argument, function_call) {
        insert_load(node_insert_before, argument);
    }
}

static void insert_store_assignment(Node* node_to_insert_before, Node_assignment* assignment, bool is_for_struct_literal_member) {
    Node* lhs = assignment->lhs;
    Node* rhs = assignment->rhs;
    Node* rhs_load = NULL;
    Lang_type rhs_load_lang_type = {0};

    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
            nodes_remove(lhs, false);
            nodes_insert_before(node_to_insert_before, lhs);
            break;
        case NODE_SYMBOL_TYPED:
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
            do_struct_literal(node_unwrap_struct_literal(rhs));
            //rhs_load = insert_load(node_to_insert_before, rhs);
            //assert(rhs_load->lang_type.count > 0);
            rhs_load_lang_type = node_unwrap_struct_literal(rhs)->lang_type;
            assert(rhs_load_lang_type.str.count > 0);
            break;
        case NODE_SYMBOL_TYPED:
            rhs_load = node_wrap(insert_load(node_to_insert_before, rhs));
            assert(rhs_load);
            rhs_load_lang_type = node_unwrap_symbol_typed(rhs)->lang_type;
            break;
        case NODE_LITERAL:
            rhs_load_lang_type = node_unwrap_literal(rhs)->lang_type;
            break;
        case NODE_OPERATOR:
            unreachable("operator should not still be present");
            break;
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            rhs_load = node_unwrap_operator_rtn_val_sym(rhs)->node_src;
            rhs_load_lang_type = node_unwrap_operator_rtn_val_sym(rhs)->lang_type;
            break;
        case NODE_FUNCTION_CALL:
            rhs_load = rhs;
            nodes_remove(rhs_load, true);
            nodes_insert_before(node_to_insert_before, rhs_load);
            Node* fun_def;
            try(sym_tbl_lookup(&fun_def, get_node_name(rhs)));
            Node_lang_type* function_rtn_type = node_unwrap_function_return_types(nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES))->child;
            node_unwrap_function_call(rhs_load)->lang_type = function_rtn_type->lang_type;
            assert(rhs_load);
            assert(node_unwrap_function_call(rhs_load)->lang_type.str.count > 0);
            rhs_load_lang_type = node_unwrap_function_call(rhs)->lang_type;
            break;
        default:
            todo();
    }

    if (is_for_struct_literal_member) {
        unreachable("");
        //Node_store_variable* store = node_unwrap_store_variable(node_new(lhs->pos, NODE_STORE_VARIABLE));
        //store->name = get_node_name(lhs);
        //nodes_remove(rhs, true);
        //nodes_append_child(node_wrap(store), rhs);
        //nodes_insert_before(node_to_insert_before, node_wrap(store));
    } else if (lhs->type == NODE_STRUCT_MEMBER_SYM_TYPED) {
        if (rhs->type == NODE_STRUCT_LITERAL) {
            todo();
        }
        nodes_remove(lhs, true);

        Node* generic_store;
        if (rhs->type == NODE_LITERAL) {
            Node_llvm_store_literal* store = node_unwrap_llvm_store_literal(node_new(lhs->pos, NODE_LLVM_STORE_LITERAL));
            Node_load_element_ptr* store_element_ptr = do_load_struct_element_ptr(node_to_insert_before, lhs);
            nodes_append_child(node_wrap(store), node_clone(rhs));
            store->node_dest = node_wrap(store_element_ptr);
            store->lang_type = store_element_ptr->lang_type;
            generic_store = node_wrap(store);
        } else {
            Node_store_another_node* store = node_unwrap_store_another_node(node_new(lhs->pos, NODE_STORE_ANOTHER_NODE));
            Node_load_element_ptr* store_element_ptr = do_load_struct_element_ptr(node_to_insert_before, lhs);
            store->node_src = rhs_load;
            assert(store->node_src);
            store->node_dest = node_wrap(store_element_ptr);
            store->lang_type = store_element_ptr->lang_type;
            generic_store = node_wrap(store);
        }
        //nodes_remove(rhs, true);
        nodes_insert_after(node_wrap(assignment), generic_store);
        nodes_insert_after(node_wrap(assignment), lhs);
    } else {
        if (rhs->type == NODE_LITERAL) {
            Node_llvm_store_literal* store = node_unwrap_llvm_store_literal(node_new(rhs->pos, NODE_LLVM_STORE_LITERAL));
            store->name = get_node_name(lhs);
            store->node_dest = get_storage_location(lhs);
            store->lang_type = node_unwrap_literal(rhs)->lang_type;
            assert(store->lang_type.str.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            nodes_remove(rhs, true);
            nodes_append_child(node_wrap(store), rhs);
            nodes_insert_before(node_to_insert_before, node_wrap(store));
        } else if (rhs->type == NODE_STRUCT_LITERAL) {
            Node_llvm_store_struct_literal* store = node_unwrap_llvm_store_struct_literal(node_new(lhs->pos, NODE_LLVM_STORE_STRUCT_LITERAL));
            store->name = get_node_name(lhs);
            nodes_remove(rhs, true);
            nodes_append_child(node_wrap(store), rhs);
            nodes_insert_before(node_to_insert_before, node_wrap(store));
        } else {
            Node_store_another_node* store = node_unwrap_store_another_node(node_new(lhs->pos, NODE_STORE_ANOTHER_NODE));
            store->node_dest = get_storage_location(lhs);
            store->node_src = rhs_load;
            if (rhs->type == NODE_STRUCT_LITERAL) {
                unreachable("");
            }
            store->lang_type = rhs_load_lang_type;
            assert(store->lang_type.str.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            //nodes_remove(rhs, true);
            nodes_insert_before(node_to_insert_before, node_wrap(store));
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

static void load_function_parameters(Node_function_definition* fun_def) {
    Node* fun_params = nodes_get_child_of_type(node_wrap(fun_def), NODE_FUNCTION_PARAMETERS);
    Node* fun_block = nodes_get_child_of_type(node_wrap(fun_def), NODE_BLOCK);

    nodes_foreach_child(param, fun_params) {
        if (is_corresponding_to_a_struct(param)) {
            continue;
        }
        Node_function_param_sym* fun_param_call = node_unwrap_function_param_sym(node_new(param->pos, NODE_FUNCTION_PARAM_SYM));
        fun_param_call->name = get_node_name(param);
        fun_param_call->node_src = param;
        fun_param_call->lang_type = get_lang_type(param);
        insert_store(get_node_after_last_alloca(fun_block), node_wrap(fun_param_call));
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

bool add_load_and_store(Node* start_start_node, int recursion_depth) {
    (void) recursion_depth;
    if (start_start_node->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(start_start_node);
    if (!block->child) {
        return false;
    }

    Node* curr_node = nodes_get_local_rightmost(node_wrap(block->child));
    while (curr_node) {
        bool go_to_prev = true;
        switch (curr_node->type) {
            case NODE_STRUCT_LITERAL:
                do_struct_literal(node_unwrap_struct_literal(curr_node));
                break;
            case NODE_LITERAL:
                break;
            case NODE_FUNCTION_CALL:
                load_function_arguments(curr_node);
                break;
            case NODE_FUNCTION_DEFINITION:
                load_function_parameters(node_unwrap_function_definition(curr_node));
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
                load_operator_operands(curr_node, node_unwrap_operator(curr_node));
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
                insert_store_assignment(assignment, node_unwrap_assignment(assignment), false);
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
            case NODE_LOAD_ANOTHER_NODE:
                break;
            case NODE_STORE_ANOTHER_NODE:
                break;
            case NODE_LOAD_STRUCT_ELEMENT_PTR:
                break;
            case NODE_IF_STATEMENT:
                unreachable("if statement node should not still exist at this point\n");
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
