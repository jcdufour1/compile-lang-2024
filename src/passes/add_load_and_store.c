#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>

#include "passes.h"

static Node* get_store_assignment(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
);

static void enum_to_i32(Node_number** new_literal, Node_enum_lit* old_enum) {
    node_wrap_enum_lit(old_enum)->type = NODE_NUMBER;
    *new_literal = node_unwrap_number(node_wrap_enum_lit(old_enum));
    node_wrap_number(*new_literal)->lang_type = lang_type_from_cstr("i32", 0);
}

static void do_struct_literal_thing(Node_struct_literal* struct_literal) {
    for (size_t idx = 0; idx < struct_literal->members.info.count; idx++) {
        Node* memb = vec_at(&struct_literal->members, idx);
        Node_expr* memb_expr = NULL;
        Node_literal* memb_literal = NULL;
        Node_enum_lit* memb_enum = NULL;
        switch (memb->type) {
            case NODE_EXPR:
                memb_expr = node_unwrap_expr(memb);
                break;
            default:
                return;
        }

        switch (memb_expr->type) {
            case NODE_LITERAL:
                memb_literal = node_unwrap_literal(memb_expr);
                break;
            default:
                return;
        }

        switch (memb_literal->type) {
            case NODE_ENUM_LIT:
                memb_enum = node_unwrap_enum_lit(memb_literal);
                Node_number* new_literal = NULL;
                enum_to_i32(&new_literal, memb_enum);
                *vec_at_ref(&struct_literal->members, idx) = node_wrap_expr(node_wrap_literal(node_wrap_number(new_literal)));
                break;
            default:
                return;
        }
    }
}

static void do_struct_literal(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_struct_literal* struct_literal
) {

    do_struct_literal_thing(struct_literal);
    try(sym_tbl_add(&env->global_literals, node_wrap_expr(node_wrap_struct_literal(struct_literal))));
    symbol_log_table_internal(LOG_DEBUG, env->global_literals, 4, __FILE__, __LINE__);

    for (size_t idx = 0; idx < struct_literal->members.info.count; idx++) {
        Node** curr_memb = vec_at_ref(&struct_literal->members, idx);

        if ((*curr_memb)->type == NODE_EXPR) {
            Node_expr* expr = node_unwrap_expr(*curr_memb);
            switch (expr->type) {
                case NODE_LITERAL:
                    break;
                default:
                    todo();
            }
        } else {
            switch ((*curr_memb)->type) {
                case NODE_ASSIGNMENT:
                    *curr_memb = get_store_assignment(
                        env,
                        block_children,
                        idx_to_insert_before,
                        node_unwrap_assignment(*curr_memb)
                    );
                    break;
                default:
                    unreachable(NODE_FMT"\n", node_print(*curr_memb));
            }
        }
    }
}

// returns node of element pointer that should then be loaded
static Llvm_register_sym do_load_struct_element_ptr(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_member_sym_typed* symbol_call
) {
    Llvm_register_sym result;

    Node* prev_struct_sym = node_wrap_expr(node_wrap_member_sym_typed(symbol_call));
    Llvm_register_sym node_element_ptr_to_load = get_storage_location(env, symbol_call->name);

    Node* struct_var_def = NULL;
    if (!symbol_lookup(&struct_var_def, env, symbol_call->name)) {
        todo();
    }
    Lang_type type_load_from = get_lang_type(struct_var_def);

    Node_load_element_ptr* load_element_ptr = NULL;
    for (size_t idx = 0; idx < symbol_call->children.info.count; idx++) {
        Node* element_sym_ = vec_at(&symbol_call->children, idx);
        Node_member_sym_piece_typed* element_sym = node_unwrap_member_sym_piece_typed(element_sym_);

        //log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(type_load_from));
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(type_load_from));
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(element_sym->lang_type));
        log_tree(LOG_DEBUG, node_wrap_expr(node_wrap_member_sym_typed(symbol_call)));
        log_tree(LOG_DEBUG, element_sym_);

        if (lang_type_is_struct(env, type_load_from)) {
            load_element_ptr = node_load_element_ptr_new(node_wrap_member_sym_piece_typed(element_sym)->pos);
            load_element_ptr->name = get_node_name(prev_struct_sym);
            load_element_ptr->lang_type = element_sym->lang_type;
            load_element_ptr->struct_index = element_sym->struct_index;
            load_element_ptr->node_src = node_element_ptr_to_load;
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                node_wrap_load_element_ptr(load_element_ptr)
            );
        }

        type_load_from = element_sym->lang_type;
        prev_struct_sym = node_wrap_member_sym_piece_typed(element_sym);
        if (load_element_ptr) {
            node_element_ptr_to_load = llvm_register_sym_new(node_wrap_load_element_ptr(load_element_ptr));
        } else {
            memset(&node_element_ptr_to_load, 0, sizeof(node_element_ptr_to_load));
        }
    }

    if (load_element_ptr) {
        result = llvm_register_sym_new(node_wrap_load_element_ptr(load_element_ptr));
        result.lang_type = load_element_ptr->lang_type;
        result.node = node_wrap_load_element_ptr(load_element_ptr);
        return result;
    } else {
        result = get_storage_location(env, get_expr_name(node_wrap_member_sym_typed(symbol_call)));
        Node* struct_var_def = NULL;
        if (!symbol_lookup(&struct_var_def, env, get_expr_name(node_wrap_member_sym_typed(symbol_call)))) {
            todo();
        }
        result.lang_type = get_member_sym_piece_final_lang_type(symbol_call);
        return result;
    }
    unreachable("");
}

static void sym_typed_to_load_sym_rtn_value_sym(Node_symbol_typed* symbol, Node_load_another_node* node_src) {
    Node_symbol_typed temp = *symbol;
    node_wrap_symbol_typed(symbol)->type = NODE_LLVM_PLACEHOLDER;
    Node_llvm_placeholder* load_ref = node_unwrap_llvm_placeholder(node_wrap_symbol_typed(symbol));
    load_ref->lang_type = temp.lang_type;
    load_ref->llvm_reg.node = node_wrap_load_another_node(node_src);
}

static void struct_memb_to_load_memb_rtn_val_sym(Node_member_sym_typed* struct_memb, Node_load_another_node* node_src) {
    //Node_member_sym_typed temp = *struct_memb;
    Lang_type lang_type_to_load = get_member_sym_piece_final_lang_type(struct_memb);
    node_wrap_member_sym_typed(struct_memb)->type = NODE_LLVM_PLACEHOLDER;
    Node_llvm_placeholder* load_ref = node_unwrap_llvm_placeholder(node_wrap_member_sym_typed(struct_memb));
    load_ref->lang_type = lang_type_to_load;
    load_ref->llvm_reg.node = node_wrap_load_another_node(node_src);
}

static Node_llvm_placeholder* refer_to_llvm_register_symbol(const Env* env, const Node_unary* refer) {
    assert(refer->token_type == TOKEN_REFER);

    //Node_member_sym_typed temp = *struct_memb;
    Lang_type lang_type_to_load = refer->lang_type;
    Node_llvm_placeholder* load_ref = node_llvm_placeholder_new(node_wrap_expr(node_wrap_operator_const(node_wrap_unary_const(refer)))->pos);
    load_ref->lang_type = lang_type_to_load;
    load_ref->llvm_reg = get_storage_location(env, get_node_name_expr(refer->child));
    return load_ref;
}

static Llvm_register_sym insert_load_literal(
    Env* env,
    Node** new_symbol_call,
    Node_literal* literal
) {
    switch (literal->type) {
        case NODE_NUMBER:
            *new_symbol_call = node_wrap_expr(node_wrap_literal(literal));
            return (Llvm_register_sym){0};
        case NODE_STRING:
            try(sym_tbl_add(&env->global_literals, node_wrap_expr(node_wrap_literal(literal))));
            *new_symbol_call = node_wrap_expr(node_wrap_literal(literal));
            return (Llvm_register_sym){0};
        case NODE_ENUM_LIT: {
            Node_number* new_literal = NULL;
            enum_to_i32(&new_literal, node_unwrap_enum_lit(literal));
            *new_symbol_call = node_wrap_expr(node_wrap_literal(node_wrap_number(new_literal)));
            return (Llvm_register_sym){0};
        }
        case NODE_VOID:
            *new_symbol_call = node_wrap_expr(node_wrap_literal(literal));
            return (Llvm_register_sym){0};
    }
    unreachable("");
}


static Llvm_register_sym insert_load(
    Env* env,
    Node** new_symbol_call,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* symbol_call
) {

    if (symbol_call->type == NODE_EXPR) {
        Node_expr* sym_call_expr = node_unwrap_expr(symbol_call);
        switch (sym_call_expr->type) {
            case NODE_STRUCT_LITERAL:
                do_struct_literal(env, block_children, idx_to_insert_before, node_unwrap_struct_literal(sym_call_expr));
                *new_symbol_call = symbol_call;
                return (Llvm_register_sym){0};
            case NODE_LITERAL:
                return insert_load_literal(env, new_symbol_call, node_unwrap_literal(sym_call_expr));
            case NODE_MEMBER_SYM_TYPED: {
                Node_load_another_node* load = node_load_another_node_new(symbol_call->pos);
                Llvm_register_sym llvm_reg = do_load_struct_element_ptr(
                    env,
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_member_sym_typed(sym_call_expr)
                );
                load->node_src = llvm_reg;
                load->lang_type = llvm_reg.lang_type;
                assert(load->lang_type.str.count > 0);
                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    node_wrap_load_another_node(load)
                );
                struct_memb_to_load_memb_rtn_val_sym(node_unwrap_member_sym_typed(sym_call_expr), load);
                *new_symbol_call = symbol_call;
                return llvm_register_sym_new(node_wrap_load_another_node(load));
            }
            case NODE_SYMBOL_TYPED: {
                Node* sym_def;
                log(LOG_DEBUG, NODE_FMT"\n", node_print(symbol_call));
                try(symbol_lookup(&sym_def, env, get_node_name(symbol_call)));
                Node_load_another_node* load = node_load_another_node_new(sym_def->pos);
                load->node_src = get_storage_location(env, get_node_name(symbol_call));
                load->lang_type = node_unwrap_variable_def(sym_def)->lang_type;
                assert(load->lang_type.str.count > 0);
                log_tree(LOG_DEBUG, symbol_call);
                sym_typed_to_load_sym_rtn_value_sym(node_unwrap_symbol_typed(sym_call_expr), load);
                log_tree(LOG_DEBUG, symbol_call);
                //switch (symbol_call->type) {
                //    case NODE_SYMBOL_TYPED:
                //        break;
                //    default:
                //        unreachable(NODE_FMT, node_print(symbol_call));
                //}
                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    node_wrap_load_another_node(load)
                );
                *new_symbol_call = symbol_call;
                return llvm_register_sym_new(node_wrap_load_another_node(load));
            }
            case NODE_OPERATOR: {
                Node_operator* operator = node_unwrap_operator(sym_call_expr);
                Node_unary* unary = node_unwrap_unary(operator);
                if (unary->token_type != TOKEN_REFER) {
                    todo();
                }
                *new_symbol_call = node_wrap_expr(node_wrap_llvm_placeholder(refer_to_llvm_register_symbol(env, unary)));
                return (Llvm_register_sym){0};
            }
            case NODE_LLVM_PLACEHOLDER:
                *new_symbol_call = symbol_call;
                return (Llvm_register_sym){0};
            default:
                unreachable(NODE_FMT"\n", node_print(node_wrap_expr(sym_call_expr)));
        }
    } else {
        switch (symbol_call->type) {
            case NODE_LOAD_ANOTHER_NODE: {
                Node_load_another_node* load = node_load_another_node_new(symbol_call->pos);
                load->node_src = llvm_register_sym_new(symbol_call);
                load->lang_type = node_unwrap_load_another_node(symbol_call)->lang_type;
                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    node_wrap_load_another_node(load)
                );
                *new_symbol_call = symbol_call;
                return llvm_register_sym_new(node_wrap_load_another_node(load));
            }
            case NODE_VARIABLE_DEF: {
                Node* sym_def;
                try(symbol_lookup(&sym_def, env, get_node_name(symbol_call)));
                Node_load_another_node* load = node_load_another_node_new(sym_def->pos);
                load->node_src = get_storage_location(env, get_node_name(symbol_call));
                load->lang_type = node_unwrap_variable_def(sym_def)->lang_type;
                assert(load->lang_type.str.count > 0);
                switch (symbol_call->type) {
                    case NODE_SYMBOL_TYPED:
                        log_tree(LOG_DEBUG, symbol_call);
                        sym_typed_to_load_sym_rtn_value_sym(node_unwrap_symbol_typed(node_unwrap_expr(symbol_call)), load);
                        log_tree(LOG_DEBUG, symbol_call);
                        break;
                    default:
                        unreachable(NODE_FMT, node_print(symbol_call));
                }
                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    node_wrap_load_another_node(load)
                );
                *new_symbol_call = symbol_call;
                return llvm_register_sym_new(node_wrap_load_another_node(load));
            }
            default:
                node_printf(symbol_call);
                todo();
        }
    }
}

static void insert_store(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* symbol_call /* src */
) {
    Str_view dest_name = {0};
    if (symbol_call->type == NODE_EXPR) {
        Node_expr* sym_call_expr = node_unwrap_expr(symbol_call);
        switch (sym_call_expr->type) {
            case NODE_LITERAL:
                return;
            case NODE_SYMBOL_TYPED:
                dest_name = get_node_name(symbol_call);
                break;
            case NODE_LLVM_PLACEHOLDER:
                dest_name = get_node_name(node_unwrap_llvm_placeholder(sym_call_expr)->llvm_reg.node);
                break;
            default:
                unreachable("");
        }
    } else {
        switch (symbol_call->type) {
            case NODE_VARIABLE_DEF:
                dest_name = get_node_name(symbol_call);
                break;
            default:
                node_printf(symbol_call);
                todo();
        }
    }

    if (symbol_call->type == NODE_EXPR && node_unwrap_expr(symbol_call)) {
        todo();
    } else {
        Node_store_another_node* store = node_store_another_node_new(symbol_call->pos);
        store->node_src = llvm_register_sym_new(symbol_call);
        store->lang_type = get_lang_type(symbol_call);
        store->node_dest = get_storage_location(env, dest_name);
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            node_wrap_store_another_node(store)
        );
    }
}

static void load_operator_operands(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operator
) {
    if (operator->type == NODE_UNARY) {
        Node* new_child;
        Node_unary* unary_oper = node_unwrap_unary(operator);
        insert_load(env, &new_child, block_children, idx_to_insert_before, node_wrap_expr(unary_oper->child));
        unary_oper->child = node_unwrap_expr(new_child);
    } else if (operator->type == NODE_BINARY) {
        log_tree(LOG_DEBUG, (Node*)operator);
        Node* new_lhs;
        Node* new_rhs;
        Node_binary* bin_oper = node_unwrap_binary(operator);
        insert_load(env, &new_lhs, block_children, idx_to_insert_before, node_wrap_expr(bin_oper->lhs));
        insert_load(env, &new_rhs, block_children, idx_to_insert_before, node_wrap_expr(bin_oper->rhs));
        bin_oper->lhs = node_unwrap_expr(new_lhs);
        bin_oper->rhs = node_unwrap_expr(new_rhs);
        log_tree(LOG_DEBUG, (Node*)operator);
    } else {
        unreachable("");
    }
}

static void add_load_foreach_arg(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_function_call* function_call
) {
    for (size_t idx = 0; idx < function_call->args.info.count; idx++) {
        Node_expr** arg = vec_at_ref(&function_call->args, idx);
        Node* new_arg;
        insert_load(env, &new_arg, block_children, idx_to_insert_before, node_wrap_expr(*arg));
        *arg = node_unwrap_expr(new_arg);
    }
}

static Llvm_register_sym move_operator_back(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operator
) {
    Llvm_register_sym operator_sym = llvm_register_sym_new_from_expr(node_wrap_operator(operator));
    operator_sym.node = node_wrap_expr(node_wrap_operator(operator));
    operator_sym.lang_type = get_operator_lang_type(operator);
    assert(operator_sym.lang_type.str.count > 0);
    assert(operator);
    insert_into_node_ptr_vec(
        block_children,
        idx_to_insert_before,
        *idx_to_insert_before,
        node_wrap_expr(node_wrap_operator(operator))
    );
    return operator_sym;
}

static Node* get_store_assignment(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
) {
    Llvm_register_sym store_dest = {0};
    Llvm_register_sym store_src = {0};
    Lang_type lhs_load_lang_type = {0};
    Lang_type rhs_load_lang_type = {0};

    if (assignment->lhs->type == NODE_EXPR) {
        Node_expr* lhs_expr = node_unwrap_expr(assignment->lhs);
        switch (lhs_expr->type) {
            case NODE_SYMBOL_TYPED:
                store_dest = get_storage_location(env, get_node_name(assignment->lhs));
                lhs_load_lang_type = node_unwrap_symbol_typed(lhs_expr)->lang_type;
                break;
            case NODE_MEMBER_SYM_TYPED: {
                Llvm_register_sym llvm_reg = do_load_struct_element_ptr(
                    env,
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_member_sym_typed(lhs_expr)
                );
                store_dest = llvm_reg;
                lhs_load_lang_type = get_member_sym_piece_final_lang_type(
                    node_unwrap_member_sym_typed(lhs_expr)
                );
                break;
            }
            case NODE_OPERATOR: {
                Node_unary* unary = node_unwrap_unary(node_unwrap_operator(lhs_expr));
                if (unary->token_type != TOKEN_DEREF) {
                    todo();
                }
                if (is_corresponding_to_a_struct(env, node_wrap_expr(unary->child))) {
                    if (get_lang_type_expr(unary->child).pointer_depth > 0) {
                        store_dest = get_storage_location(env, get_node_name_expr(unary->child));
                    } else {
                        todo();
                    }
                } else {
                    Node* new_child;
                    store_dest = insert_load(env, &new_child, block_children, idx_to_insert_before, node_wrap_expr(unary->child));
                    unary->child = node_unwrap_expr(new_child);
                }
                lhs_load_lang_type = unary->lang_type;
                break;
            }
            default:
                unreachable("");
        }
    } else {
        switch (assignment->lhs->type) {
            case NODE_VARIABLE_DEF:
                store_dest = get_storage_location(env, get_node_name(assignment->lhs));
                lhs_load_lang_type = node_unwrap_variable_def(assignment->lhs)->lang_type;
                break;
            default:
                unreachable(NODE_FMT, node_print(assignment->lhs));
        }
    }
   
    switch (assignment->rhs->type) {
        case NODE_STRUCT_LITERAL: {
            Node* new_rhs;
            store_src = insert_load(env, &new_rhs, block_children, idx_to_insert_before, node_wrap_expr(assignment->rhs));
            assignment->rhs = node_unwrap_expr(new_rhs);
            rhs_load_lang_type = node_unwrap_struct_literal(assignment->rhs)->lang_type;
            assert(rhs_load_lang_type.str.count > 0);
            break;
        }
        case NODE_SYMBOL_TYPED: {
            Node* new_rhs;
            Lang_type lang_type = node_unwrap_symbol_typed(assignment->rhs)->lang_type;
            store_src = insert_load(
                env, &new_rhs, block_children, idx_to_insert_before, node_wrap_expr(assignment->rhs)
            );
            assignment->rhs = node_unwrap_expr(new_rhs);
            rhs_load_lang_type = lang_type;
            break;
        }
        case NODE_LITERAL: {
            Node* new_rhs;
            log_tree(LOG_DEBUG, node_wrap_expr(assignment->rhs));
            store_src = insert_load(env, &new_rhs, block_children, idx_to_insert_before, node_wrap_expr(assignment->rhs));
            assignment->rhs = node_unwrap_expr(new_rhs);
            rhs_load_lang_type = node_unwrap_literal(node_unwrap_expr(new_rhs))->lang_type;
            break;
        }
        case NODE_LLVM_PLACEHOLDER:
            store_src = node_unwrap_llvm_placeholder(assignment->rhs)->llvm_reg;
            rhs_load_lang_type = node_unwrap_llvm_placeholder(assignment->rhs)->lang_type;
            break;
        case NODE_MEMBER_SYM_TYPED: {
            Node* new_rhs;
            store_src = insert_load(env, &new_rhs, block_children, idx_to_insert_before, node_wrap_expr(assignment->rhs));
            assignment->rhs = node_unwrap_expr(new_rhs);
            rhs_load_lang_type = node_unwrap_llvm_placeholder(assignment->rhs)->lang_type;
            break;
        }
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operator(assignment->rhs);
            if (operator->type == NODE_UNARY) {
                Node_unary* unary = node_unwrap_unary(operator);
                if (unary->token_type == TOKEN_DEREF) {
                    Node* store_src_imm;
                    if (unary->child->type == NODE_MEMBER_SYM_TYPED) {
                        todo();
                    } else {
                        Node* new_child;
                        store_src_imm = insert_load(env, &new_child, block_children, idx_to_insert_before, node_wrap_expr(unary->child)).node;
                        unary->child = node_unwrap_expr(new_child);
                    }
                    Node* new_store_imm;
                    Node_load_another_node* store_src_ = node_unwrap_load_another_node(insert_load(
                        env, &new_store_imm, block_children, idx_to_insert_before, store_src_imm).node
                    );
                    store_src_imm = new_store_imm;

                    store_src_->lang_type.pointer_depth--;
                    store_src = llvm_register_sym_new(node_wrap_load_another_node(store_src_));
                    rhs_load_lang_type = unary->lang_type;
                    break;
                } else if (unary->token_type == TOKEN_REFER) {
                    store_src = get_storage_location(env, get_node_name_expr(unary->child));
                    rhs_load_lang_type = unary->lang_type;
                    break;
                }
            }
            Llvm_register_sym store_src_ = move_operator_back(block_children, idx_to_insert_before, operator);
            store_src = store_src_;
            rhs_load_lang_type = store_src_.lang_type;
            break;
        }
        case NODE_FUNCTION_CALL:
            store_src = llvm_register_sym_new(node_wrap_expr(assignment->rhs));
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                node_wrap_expr(assignment->rhs)
            );
            Node_function_decl* fun_decl;
            {
                Node* fun_def;
                try(symbol_lookup(&fun_def, env, get_node_name_expr(assignment->rhs)));
                if (fun_def->type == NODE_FUNCTION_DEF) {
                    fun_decl = node_unwrap_function_def(fun_def)->declaration;
                } else {
                    fun_decl = node_unwrap_function_decl(fun_def);
                }
            }
            node_unwrap_function_call(node_unwrap_expr(store_src.node))->lang_type = fun_decl->return_type->lang_type;
            rhs_load_lang_type = node_unwrap_function_call(assignment->rhs)->lang_type;
            break;
        default:
            assert(false);
            todo();
    }

    if (assignment->rhs->type == NODE_LITERAL) {
        Node_llvm_store_literal* store = node_llvm_store_literal_new(node_wrap_expr(assignment->rhs)->pos);
        store->node_dest = llvm_register_sym_new(store_dest.node);
        store->lang_type = lhs_load_lang_type;
        assert(store->lang_type.str.count > 0);
        store->child = node_unwrap_literal(assignment->rhs);
        return node_wrap_llvm_store_literal(store);
    } else if (assignment->rhs->type == NODE_STRUCT_LITERAL) {
        Node_llvm_store_struct_literal* store = node_llvm_store_struct_literal_new(assignment->lhs->pos);
        store->child = node_unwrap_struct_literal(assignment->rhs);
        store->node_dest = llvm_register_sym_new(store_dest.node);
        return node_wrap_llvm_store_struct_literal(store);
    } else {
        Node_store_another_node* store = node_store_another_node_new(assignment->lhs->pos);
        store->node_dest = store_dest;
        store->node_src = store_src;
        if (assignment->rhs->type == NODE_STRUCT_LITERAL) {
            unreachable("");
        }
        store->lang_type = rhs_load_lang_type;
        assert(store->lang_type.str.count > 0);
        return node_wrap_store_another_node(store);
    }
}

static void add_load_return(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_return* return_statement
) {
    switch (return_statement->child->type) {
        case NODE_MEMBER_SYM_TYPED:
            // fallthrough
        case NODE_SYMBOL_TYPED: {
            Node* new_child;
            insert_load(env, &new_child, block_children, idx_to_insert_before, node_wrap_expr(return_statement->child));
            return_statement->child = node_unwrap_expr(new_child);
            return;
        }
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_LLVM_PLACEHOLDER:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_OPERATOR:
            if (return_statement->child->type == NODE_OPERATOR) {
                Llvm_register_sym llvm_reg = move_operator_back(
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_operator(return_statement->child)
                );
                return_statement->child = node_wrap_llvm_placeholder(node_e_llvm_placeholder_new_from_reg(llvm_reg, llvm_reg.lang_type));
                assert(return_statement->child);
            }
            break;
        default:
            unreachable("");
    }
}

static size_t get_idx_node_after_last_alloca(Node_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Node* node = vec_at(&block->children, idx);
        if (node->type != NODE_ALLOCA) {
            return idx;
        }
    }
    unreachable("");
}

static void load_function_parameters(Env* env, Node_function_def* fun_def) {
    Node_ptr_vec* params = &fun_def->declaration->parameters->params;
    for (size_t idx = 0; idx < params->info.count; idx++) {
        Node* param = vec_at(params, idx);
        if (is_corresponding_to_a_struct(env, param)) {
            continue;
        }
        Llvm_register_sym fun_param_call = llvm_register_sym_new(param);
        size_t idx_insert_before = get_idx_node_after_last_alloca(fun_def->body);
        insert_store(env, &fun_def->body->children, &idx_insert_before, fun_param_call.node);
    }
}

static void load_function_arguments(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_function_call* fun_call
) {
    add_load_foreach_arg(env, block_children, idx_to_insert_before, fun_call);
}

// returns operand or operand symbol
static Node* flatten_operator_operand(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_expr* operand
) {
    if (operand->type == NODE_OPERATOR) {
        Node_operator* child = node_unwrap_operator(operand);
        if (child->type == NODE_UNARY) {
            Node_unary* unary = node_unwrap_unary(child);
            if (unary->token_type == TOKEN_DEREF) {
                Node* new_child;
                Node* store_src_imm = insert_load(env, &new_child, block_children, idx_to_insert_before, node_wrap_expr(unary->child)).node;
                unary->child = node_unwrap_expr(new_child);

                Node* new_store_src_imm;
                Node_load_another_node* store_src_ = node_unwrap_load_another_node(insert_load(env, &new_store_src_imm, block_children, idx_to_insert_before, store_src_imm).node);
                store_src_imm = new_store_src_imm;

                store_src_->lang_type.pointer_depth--;
                Llvm_register_sym reg_sym = llvm_register_sym_new(node_wrap_load_another_node(store_src_));
                
                return node_wrap_expr(node_wrap_llvm_placeholder(node_e_llvm_placeholder_new_from_reg(reg_sym, store_src_->lang_type)));
            } else if (unary->token_type == TOKEN_UNSAFE_CAST) {
                Node_llvm_placeholder* operator_sym = node_llvm_placeholder_new(node_wrap_expr(operand)->pos);

                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    node_wrap_expr(operand)
                );
                operator_sym->llvm_reg = llvm_register_sym_new_from_expr(operand);
                operator_sym->lang_type = get_lang_type_expr(operand);
                return node_wrap_expr(node_wrap_llvm_placeholder(operator_sym));
            } else {
                todo();
            }
        } else if (child->type == NODE_BINARY) {
            Node_llvm_placeholder* operator_sym = node_llvm_placeholder_new(node_wrap_expr(operand)->pos);

            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                node_wrap_expr(operand)
            );
            operator_sym->lang_type = get_lang_type_expr(operand);
            operator_sym->llvm_reg = llvm_register_sym_new_from_expr(operand);
            return node_wrap_expr(node_wrap_llvm_placeholder(operator_sym));
        } else {
            unreachable("");
        }
        unreachable("");
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Llvm_register_sym fun_sym = llvm_register_sym_new_from_expr(operand);
        fun_sym.node = node_wrap_expr(operand);
        fun_sym.lang_type = get_lang_type_expr(operand);
        assert(fun_sym.node);
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            node_wrap_expr(operand)
        );
        return node_wrap_expr(node_wrap_llvm_placeholder(node_e_llvm_placeholder_new_from_reg(fun_sym, get_lang_type_expr(operand))));
    } else {
        assert(operand);
        return node_wrap_expr(operand);
    }
}

static void flatten_operator_if_nessessary(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* old_operator
) {
    if (old_operator->type == NODE_UNARY) {
        Node_unary* old_unary_op = node_unwrap_unary(old_operator);
        old_unary_op->child = node_unwrap_expr(flatten_operator_operand(env, block_children, idx_to_insert_before, old_unary_op->child));
    } else if (old_operator->type == NODE_BINARY) {
        Node_binary* old_bin_op = node_unwrap_binary(old_operator);
        old_bin_op->lhs = node_unwrap_expr(flatten_operator_operand(env, block_children, idx_to_insert_before, old_bin_op->lhs));
        old_bin_op->rhs = node_unwrap_expr(flatten_operator_operand(env, block_children, idx_to_insert_before, old_bin_op->rhs));
    } else {
        unreachable("");
    }
}

void add_load_and_store(Env* env) {
    Node* start_start_node = vec_top(&env->ancesters);
    if (start_start_node->type != NODE_BLOCK) {
        return;
    }
    Node_block* block = node_unwrap_block(start_start_node);

    if (block->children.info.count < 1) {
        return;
    }
    for (size_t idx = block->children.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&block->children, idx);

        if (curr_node->type == NODE_EXPR) {
            Node_expr* expr = node_unwrap_expr(curr_node);
            switch (expr->type) {
                case NODE_STRUCT_LITERAL:
                    do_struct_literal(env, &block->children, &idx, node_unwrap_struct_literal(expr));
                    break;
                case NODE_LITERAL:
                    break;
                case NODE_FUNCTION_CALL:
                    load_function_arguments(env, &block->children, &idx, node_unwrap_function_call(expr));
                    break;
                case NODE_SYMBOL_UNTYPED:
                    unreachable("untyped symbol should not still be present");
                case NODE_SYMBOL_TYPED:
                    break;
                case NODE_OPERATOR:
                    flatten_operator_if_nessessary(env, &block->children, &idx, node_unwrap_operator(expr));
                    load_operator_operands(env, &block->children, &idx, node_unwrap_operator(expr));
                    break;
                case NODE_MEMBER_SYM_TYPED:
                    break;
                case NODE_LLVM_PLACEHOLDER:
                    break;
                default:
                    unreachable("");
            }
        } else {
            switch (curr_node->type) {
                case NODE_FUNCTION_DEF: {
                    Node_block* fun_block = node_unwrap_function_def(curr_node)->body;
                    vec_append(&a_main, &env->ancesters, node_wrap_block(fun_block));
                    load_function_parameters(env, node_unwrap_function_def(curr_node));
                    vec_rem_last(&env->ancesters);
                    break;
                }
                case NODE_FUNCTION_PARAMS:
                    break;
                case NODE_LANG_TYPE:
                    break;
                case NODE_BLOCK:
                    break;
                case NODE_RETURN:
                    add_load_return(env, &block->children, &idx, node_unwrap_return(curr_node));
                    break;
                case NODE_VARIABLE_DEF:
                    break;
                case NODE_FUNCTION_DECL:
                    break;
                case NODE_ASSIGNMENT: {
                    Node* thing = get_store_assignment(
                        env,
                        &block->children,
                        &idx,
                        node_unwrap_assignment(curr_node)
                    );
                    *vec_at_ref(&block->children, idx) = thing;
                    break;
                }
                case NODE_FOR_RANGE:
                    unreachable("for loop node should not still exist at this point\n");
                    todo();
                case NODE_FOR_WITH_COND:
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
                case NODE_LABEL:
                    break;
                case NODE_ALLOCA:
                    break;
                case NODE_LOAD_ANOTHER_NODE:
                    break;
                case NODE_STORE_ANOTHER_NODE:
                    break;
                case NODE_LOAD_ELEMENT_PTR:
                    break;
                case NODE_IF:
                    unreachable("if statement node should not still exist at this point\n");
                case NODE_STRUCT_DEF:
                    break;
                case NODE_RAW_UNION_DEF:
                    break;
                case NODE_LLVM_STORE_LITERAL:
                    break;
                case NODE_LLVM_STORE_STRUCT_LITERAL:
                    break;
                case NODE_ENUM_DEF:
                    break;
                default:
                    unreachable(NODE_FMT"\n", node_print(curr_node));
            }
        }

        if (idx < 1) {
            break;
        }
    }

    return;
}
