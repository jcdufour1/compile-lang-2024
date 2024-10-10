#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static Node* get_store_assignment(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment,
    bool is_for_struct_literal_member
);

static void do_struct_literal(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_struct_literal* struct_literal
) {
    for (size_t idx = 0; idx < struct_literal->members.info.count; idx++) {
        Node** curr_memb = node_ptr_vec_at_ref(&struct_literal->members, idx);

        switch ((*curr_memb)->type) {
            case NODE_ASSIGNMENT:
                *curr_memb = get_store_assignment(
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_assignment(*curr_memb),
                    true
                );
                break;
            case NODE_LITERAL:
                break;
            default:
                unreachable(NODE_FMT"\n", node_print(curr_memb));
        }
    }
}

// returns node of element pointer that should then be loaded
static Node_load_element_ptr* do_load_struct_element_ptr(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_struct_member_sym_typed* symbol_call
) {
    Node* prev_struct_sym = node_wrap(symbol_call);
    Node* sym_def_;
    if (!sym_tbl_lookup(&sym_def_, symbol_call->name)) {
        unreachable("symbol definition not found");
    }
    Node* node_element_ptr_to_load = get_storage_location(symbol_call->name);
    Node_load_element_ptr* load_element_ptr = NULL;
    for (size_t idx = 0; idx < symbol_call->children.info.count; idx++) {
        Node* element_sym_ = node_ptr_vec_at(&symbol_call->children, idx);
        Node_struct_member_sym_piece_typed* element_sym = node_unwrap_struct_member_sym_piece_typed(element_sym_);

        load_element_ptr = node_unwrap_load_elem_ptr(node_new(node_wrap(element_sym)->pos, NODE_LOAD_STRUCT_ELEMENT_PTR));
        load_element_ptr->name = get_node_name(prev_struct_sym);
        load_element_ptr->lang_type = element_sym->lang_type;
        load_element_ptr->struct_index = element_sym->struct_index;
        load_element_ptr->node_src = node_element_ptr_to_load;
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            node_wrap(load_element_ptr)
        );

        prev_struct_sym = node_wrap(element_sym);
        node_element_ptr_to_load = node_wrap(load_element_ptr);
    }
    assert(load_element_ptr);
    return load_element_ptr;
}

static void sym_typed_to_load_sym_rtn_value_sym(Node_symbol_typed* symbol, Node_load_another_node* node_src) {
    Node_symbol_typed temp = *symbol;
    node_wrap(symbol)->type = NODE_LLVM_REGISTER_SYM;
    Node_llvm_register_sym* load_ref = node_unwrap_llvm_register_sym(node_wrap(symbol));
    load_ref->lang_type = temp.lang_type;
    load_ref->node_src = node_wrap(node_src);
}

static Lang_type get_member_sym_piece_final_lang_type(const Node_struct_member_sym_typed* struct_memb_sym) {
    Lang_type lang_type = {0};
    for (size_t idx = 0; idx < struct_memb_sym->children.info.count; idx++) {
        const Node* memb_piece_ = node_ptr_vec_at_const(&struct_memb_sym->children, idx);
        const Node_struct_member_sym_piece_typed* memb_piece = 
            node_unwrap_struct_member_sym_piece_typed_const(memb_piece_);
        lang_type = memb_piece->lang_type;
    }
    assert(lang_type.str.count > 0);
    return lang_type;
}

static void struct_memb_to_load_memb_rtn_val_sym(Node_struct_member_sym_typed* struct_memb, Node_load_another_node* node_src) {
    //Node_struct_member_sym_typed temp = *struct_memb;
    Lang_type lang_type_to_load = get_member_sym_piece_final_lang_type(struct_memb);
    node_wrap(struct_memb)->type = NODE_LLVM_REGISTER_SYM;
    Node_llvm_register_sym* load_ref = node_unwrap_llvm_register_sym(node_wrap(struct_memb));
    load_ref->lang_type = lang_type_to_load;
    load_ref->node_src = node_wrap(node_src);
}

static Node_load_another_node* insert_load(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* symbol_call
) {
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
        Node_load_element_ptr* load_element_ptr = do_load_struct_element_ptr(block_children, idx_to_insert_before, node_unwrap_struct_member_sym_typed(symbol_call));
        load->node_src = node_wrap(load_element_ptr);
        load->lang_type = load_element_ptr->lang_type;
        assert(load->lang_type.str.count > 0);
        node_ptr_vec_insert(block_children, *idx_to_insert_before, node_wrap(load));
        struct_memb_to_load_memb_rtn_val_sym(node_unwrap_struct_member_sym_typed(symbol_call), load);
        return load;
    } else {
        Node* sym_def;
        try(sym_tbl_lookup(&sym_def, get_node_name(symbol_call)));
        Node_load_another_node* load = node_unwrap_load_another_node(node_new(sym_def->pos, NODE_LOAD_ANOTHER_NODE));
        load->node_src = get_storage_location(get_node_name(symbol_call));
        load->lang_type = node_unwrap_variable_def(sym_def)->lang_type;
        assert(load->lang_type.str.count > 0);
        switch (symbol_call->type) {
            case NODE_SYMBOL_TYPED:
                sym_typed_to_load_sym_rtn_value_sym(node_unwrap_symbol_typed(symbol_call), load);
                break;
            default:
                unreachable(NODE_FMT, node_print(symbol_call));
        }
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            node_wrap(load)
        );
        return load;
    }
}

static void insert_store(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* symbol_call /* src */
) {
    Str_view dest_name = {0};
    switch (symbol_call->type) {
        case NODE_LITERAL:
            return;
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            dest_name = get_node_name(symbol_call);
            break;
        case NODE_LLVM_REGISTER_SYM:
            dest_name = get_node_name(node_unwrap_llvm_register_sym(symbol_call)->node_src);
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
        store->node_dest = get_storage_location(dest_name);
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            node_wrap(store)
        );
    }
}

static void load_operator_operand(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* operand
) {
    switch (operand->type) {
        case NODE_OPERATOR:
            unreachable("nested operators should not still be present at this point");
        case NODE_LITERAL:
            break;
        case NODE_VARIABLE_DEFINITION:
            todo();
        case NODE_SYMBOL_TYPED:
            insert_load(block_children, idx_to_insert_before, operand);
            break;
        case NODE_LLVM_REGISTER_SYM:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static Node_operator* load_operator_operands(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operator
) {
    load_operator_operand(block_children, idx_to_insert_before, operator->lhs);
    load_operator_operand(block_children, idx_to_insert_before, operator->rhs);
    return operator;
}

static void add_load_foreach_arg(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_function_call* function_call
) {
    for (size_t idx = 0; idx < function_call->args.info.count; idx++) {
        Node* argument = node_ptr_vec_at(&function_call->args, idx);
        insert_load(block_children, idx_to_insert_before, argument);
    }
}

static Node* get_store_assignment(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment,
    bool is_for_struct_literal_member
) {
    Node* lhs = assignment->lhs;
    Node* rhs = assignment->rhs;
    Node* rhs_load = NULL;
    Lang_type rhs_load_lang_type = {0};

    switch (lhs->type) {
        case NODE_VARIABLE_DEFINITION:
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
            do_struct_literal(block_children, idx_to_insert_before, node_unwrap_struct_literal(rhs));
            //rhs_load = insert_load(node_to_insert_before, rhs);
            //assert(rhs_load->lang_type.count > 0);
            rhs_load_lang_type = node_unwrap_struct_literal(rhs)->lang_type;
            assert(rhs_load_lang_type.str.count > 0);
            break;
        case NODE_SYMBOL_TYPED:
            rhs_load = node_wrap(insert_load(block_children, idx_to_insert_before, rhs));
            assert(rhs_load);
            rhs_load_lang_type = node_unwrap_llvm_register_sym(rhs)->lang_type;
            break;
        case NODE_LITERAL:
            rhs_load_lang_type = node_unwrap_literal(rhs)->lang_type;
            break;
        case NODE_LLVM_REGISTER_SYM:
            rhs_load = node_unwrap_llvm_register_sym(rhs)->node_src;
            rhs_load_lang_type = node_unwrap_llvm_register_sym(rhs)->lang_type;
            break;
        case NODE_OPERATOR:
            unreachable("operator should not still be present");
            break;
        case NODE_FUNCTION_CALL:
            rhs_load = rhs;
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                rhs
            );
            Node_function_declaration* fun_decl;
            {
                Node* fun_def;
                try(sym_tbl_lookup(&fun_def, get_node_name(rhs)));
                if (fun_def->type == NODE_FUNCTION_DEFINITION) {
                    fun_decl = node_unwrap_function_definition(fun_def)->declaration;
                } else {
                    fun_decl = node_unwrap_function_declaration(fun_def);
                }
            }
            Node_lang_type* function_rtn_type = fun_decl->return_types->child;
            node_unwrap_function_call(rhs_load)->lang_type = function_rtn_type->lang_type;
            assert(rhs_load);
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

        Node* generic_store;
        if (rhs->type == NODE_LITERAL) {
            Node_llvm_store_literal* store = node_unwrap_llvm_store_literal(node_new(lhs->pos, NODE_LLVM_STORE_LITERAL));
            Node_load_element_ptr* store_element_ptr = do_load_struct_element_ptr(
                block_children,
                idx_to_insert_before,
                node_unwrap_struct_member_sym_typed(lhs)
            );
            store->child = node_unwrap_literal(rhs);
            store->node_dest = node_wrap(store_element_ptr);
            store->lang_type = store_element_ptr->lang_type;
            generic_store = node_wrap(store);
        } else {
            Node_store_another_node* store = node_unwrap_store_another_node(node_new(lhs->pos, NODE_STORE_ANOTHER_NODE));
            Node_load_element_ptr* store_element_ptr = do_load_struct_element_ptr(
                block_children,
                idx_to_insert_before,
                node_unwrap_struct_member_sym_typed(lhs)
            );
            store->node_src = rhs_load;
            assert(store->node_src);
            store->node_dest = node_wrap(store_element_ptr);
            store->lang_type = store_element_ptr->lang_type;
            generic_store = node_wrap(store);
        }
        //nodes_remove(rhs, true);
        return generic_store;
    } else {
        if (rhs->type == NODE_LITERAL) {
            Node_llvm_store_literal* store = node_unwrap_llvm_store_literal(node_new(rhs->pos, NODE_LLVM_STORE_LITERAL));
            store->name = get_node_name(lhs);
            store->node_dest = get_storage_location(store->name);
            store->lang_type = node_unwrap_literal(rhs)->lang_type;
            assert(store->lang_type.str.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            store->child = node_unwrap_literal(rhs);
            return node_wrap(store);
        } else if (rhs->type == NODE_STRUCT_LITERAL) {
            Node_llvm_store_struct_literal* store = node_unwrap_llvm_store_struct_literal(node_new(lhs->pos, NODE_LLVM_STORE_STRUCT_LITERAL));
            store->child = node_unwrap_struct_literal(rhs);
            store->node_dest = get_storage_location(get_node_name(lhs));
            return node_wrap(store);
        } else {
            Node_store_another_node* store = node_unwrap_store_another_node(node_new(lhs->pos, NODE_STORE_ANOTHER_NODE));
            store->node_dest = get_storage_location(get_node_name(lhs));
            store->node_src = rhs_load;
            if (rhs->type == NODE_STRUCT_LITERAL) {
                unreachable("");
            }
            store->lang_type = rhs_load_lang_type;
            assert(store->lang_type.str.count > 0);
            //assert(store->lang_type.count > 0); // TODO: actually check for this
            //nodes_remove(rhs, true);
            return node_wrap(store);
        }
    }
}

static void add_load_return_statement(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_return_statement* return_statement
) {
    Node* node_to_return = return_statement->child;
    switch (node_to_return->type) {
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            insert_load(block_children, idx_to_insert_before, node_to_return);
            return;
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_LLVM_REGISTER_SYM:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_OPERATOR:
            unreachable("operator should not still be the child of return statement at this point");
        default:
            unreachable(NODE_FMT"\n", node_print(node_to_return));
    }
}

static size_t get_idx_node_after_last_alloca(Node_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Node* node = node_ptr_vec_at(&block->children, idx);
        if (node->type != NODE_ALLOCA) {
            return idx;
        }
    }
    unreachable("");
}

static void load_function_parameters(Node_function_definition* fun_def) {
    Node_ptr_vec* params = &fun_def->declaration->parameters->params;
    for (size_t idx = 0; idx < params->info.count; idx++) {
        Node* param = node_ptr_vec_at(params, idx);
        if (is_corresponding_to_a_struct(param)) {
            continue;
        }
        Node_llvm_register_sym* fun_param_call = node_unwrap_llvm_register_sym(node_new(param->pos, NODE_LLVM_REGISTER_SYM));
        fun_param_call->node_src = param;
        fun_param_call->lang_type = get_lang_type(param);
        size_t idx_insert_before = get_idx_node_after_last_alloca(fun_def->body);
        insert_store(&fun_def->body->children, &idx_insert_before, node_wrap(fun_param_call));
    }
}

static void load_function_arguments(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_function_call* fun_call
) {
    add_load_foreach_arg(block_children, idx_to_insert_before, fun_call);
}

bool add_load_and_store(Node* start_start_node, int recursion_depth) {
    (void) recursion_depth;
    if (start_start_node->type != NODE_BLOCK) {
        return false;
    }
    Node_block* block = node_unwrap_block(start_start_node);

    for (size_t idx = block->children.info.count - 1;; idx--) {
        Node* curr_node = node_ptr_vec_at(&block->children, idx);

        switch (curr_node->type) {
            case NODE_STRUCT_LITERAL:
                do_struct_literal(&block->children, &idx, node_unwrap_struct_literal(curr_node));
                break;
            case NODE_LITERAL:
                break;
            case NODE_FUNCTION_CALL:
                load_function_arguments(&block->children, &idx, node_unwrap_function_call(curr_node));
                break;
            case NODE_FUNCTION_DEFINITION:
                load_function_parameters(node_unwrap_function_definition(curr_node));
                break;
            case NODE_FUNCTION_PARAMETERS:
                break;
            case NODE_FUNCTION_RETURN_TYPES:
                break;
            case NODE_LANG_TYPE:
                break;
            case NODE_OPERATOR:
                load_operator_operands(&block->children, &idx, node_unwrap_operator(curr_node));
                break;
            case NODE_BLOCK:
                break;
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbol should not still be present");
            case NODE_SYMBOL_TYPED:
                break;
            case NODE_RETURN_STATEMENT:
                add_load_return_statement(&block->children, &idx, node_unwrap_return_statement(curr_node));
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_FUNCTION_DECLARATION:
                break;
            case NODE_ASSIGNMENT: {
                Node* thing = get_store_assignment(
                    &block->children,
                    &idx,
                    node_unwrap_assignment(curr_node),
                    false
                );
                *node_ptr_vec_at_ref(&block->children, idx) = thing;
                break;
            }
            case NODE_FOR_RANGE:
                unreachable("for loop node should not still exist at this point\n");
                todo();
            case NODE_FOR_WITH_CONDITION:
                unreachable("for loop node should not still exist at this point\n");
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
            case NODE_LLVM_REGISTER_SYM:
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

        if (idx < 1) {
            break;
        }
    }

    return false;
}
