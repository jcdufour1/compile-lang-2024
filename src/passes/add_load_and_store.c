#include "../node.h"
#include "../nodes.h"
#include "../symbol_table.h"
#include "../parser_utils.h"

#include "passes.h"

static Node* get_store_assignment(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
);

static void do_struct_literal(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_struct_literal* struct_literal
) {
    try(sym_tbl_add(&env->global_literals, node_wrap_struct_literal(struct_literal)));

    for (size_t idx = 0; idx < struct_literal->members.info.count; idx++) {
        Node** curr_memb = vec_at_ref(&struct_literal->members, idx);

        switch ((*curr_memb)->type) {
            case NODE_ASSIGNMENT:
                *curr_memb = get_store_assignment(
                    env,
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_assignment(*curr_memb)
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
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_member_sym_typed* symbol_call
) {
    Node* prev_struct_sym = node_wrap_member_sym_typed(symbol_call);
    Node* sym_def_;
    if (!symbol_lookup(&sym_def_, env, symbol_call->name)) {
        unreachable("symbol definition not found");
    }
    Node* node_element_ptr_to_load = get_storage_location(env, symbol_call->name);
    Node_load_element_ptr* load_element_ptr = NULL;
    for (size_t idx = 0; idx < symbol_call->children.info.count; idx++) {
        Node* element_sym_ = vec_at(&symbol_call->children, idx);
        Node_member_sym_piece_typed* element_sym = node_unwrap_member_sym_piece_typed(element_sym_);

        load_element_ptr = node_unwrap_load_element_ptr(node_new(node_wrap_member_sym_piece_typed(element_sym)->pos, NODE_LOAD_ELEMENT_PTR));
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

        prev_struct_sym = node_wrap_member_sym_piece_typed(element_sym);
        node_element_ptr_to_load = node_wrap_load_element_ptr(load_element_ptr);
    }
    assert(load_element_ptr);
    return load_element_ptr;
}

static void sym_typed_to_load_sym_rtn_value_sym(Node_symbol_typed* symbol, Node_load_another_node* node_src) {
    Node_symbol_typed temp = *symbol;
    node_wrap_symbol_typed(symbol)->type = NODE_LLVM_REGISTER_SYM;
    Node_llvm_register_sym* load_ref = node_unwrap_llvm_register_sym(node_wrap_symbol_typed(symbol));
    load_ref->lang_type = temp.lang_type;
    load_ref->node_src = node_wrap_load_another_node(node_src);
}

static void struct_memb_to_load_memb_rtn_val_sym(Node_member_sym_typed* struct_memb, Node_load_another_node* node_src) {
    //Node_member_sym_typed temp = *struct_memb;
    Lang_type lang_type_to_load = get_member_sym_piece_final_lang_type(struct_memb);
    node_wrap_member_sym_typed(struct_memb)->type = NODE_LLVM_REGISTER_SYM;
    Node_llvm_register_sym* load_ref = node_unwrap_llvm_register_sym(node_wrap_member_sym_typed(struct_memb));
    load_ref->lang_type = lang_type_to_load;
    load_ref->node_src = node_wrap_load_another_node(node_src);
}

static Node_llvm_register_sym* refer_to_llvm_register_symbol(const Env* env, const Node_op_unary* refer) {
    assert(refer->token_type == TOKEN_REFER);

    //Node_member_sym_typed temp = *struct_memb;
    Lang_type lang_type_to_load = refer->lang_type;
    Node_llvm_register_sym* load_ref = node_unwrap_llvm_register_sym(node_new(node_wrap_operator(node_wrap_operator_generic(refer))->pos, NODE_LLVM_REGISTER_SYM));
    load_ref->lang_type = lang_type_to_load;
    load_ref->node_src = get_storage_location(env, get_node_name(refer->child));
    return load_ref;
}

static Node_load_another_node* insert_load(
    Env* env,
    Node** new_symbol_call,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node* symbol_call
) {
    switch (symbol_call->type) {
        case NODE_STRUCT_LITERAL:
            do_struct_literal(env, block_children, idx_to_insert_before, node_unwrap_struct_literal(symbol_call));
            *new_symbol_call = symbol_call;
            return NULL;
        case NODE_LITERAL:
            try(sym_tbl_add(&env->global_literals, symbol_call));
            log(LOG_DEBUG, "env->global_literals: %p\n", (void*)&env->global_literals);
            *new_symbol_call = symbol_call;
            return NULL;
        case NODE_LLVM_REGISTER_SYM:
            *new_symbol_call = symbol_call;
            return NULL;
        case NODE_MEMBER_SYM_TYPED: {
            Node_load_another_node* load = node_unwrap_load_another_node(node_new(symbol_call->pos, NODE_LOAD_ANOTHER_NODE));
            Node_load_element_ptr* load_element_ptr = do_load_struct_element_ptr(
                env,
                block_children,
                idx_to_insert_before,
                node_unwrap_member_sym_typed(symbol_call)
            );
            load->node_src = node_wrap_load_element_ptr(load_element_ptr);
            load->lang_type = load_element_ptr->lang_type;
            assert(load->lang_type.str.count > 0);
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                node_wrap_load_another_node(load)
            );
            struct_memb_to_load_memb_rtn_val_sym(node_unwrap_member_sym_typed(symbol_call), load);
            *new_symbol_call = symbol_call;
            return load;
        }
        case NODE_LOAD_ANOTHER_NODE: {
            Node_load_another_node* load = node_unwrap_load_another_node(node_new(symbol_call->pos, NODE_LOAD_ANOTHER_NODE));
            load->node_src = symbol_call;
            load->lang_type = node_unwrap_load_another_node(symbol_call)->lang_type;
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                node_wrap_load_another_node(load)
            );
            *new_symbol_call = symbol_call;
            return load;
        }
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_VARIABLE_DEF: {
            Node* sym_def;
            try(symbol_lookup(&sym_def, env, get_node_name(symbol_call)));
            Node_load_another_node* load = node_unwrap_load_another_node(node_new(sym_def->pos, NODE_LOAD_ANOTHER_NODE));
            load->node_src = get_storage_location(env, get_node_name(symbol_call));
            load->lang_type = node_unwrap_variable_def(sym_def)->lang_type;
            assert(load->lang_type.str.count > 0);
            switch (symbol_call->type) {
                case NODE_SYMBOL_TYPED:
                    log_tree(LOG_DEBUG, symbol_call);
                    sym_typed_to_load_sym_rtn_value_sym(node_unwrap_symbol_typed(symbol_call), load);
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
            return load;
        }
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operator(symbol_call);
            Node_op_unary* unary = node_unwrap_op_unary(operator);
            if (unary->token_type != TOKEN_REFER) {
                todo();
            }
            *new_symbol_call = node_wrap_llvm_register_sym(refer_to_llvm_register_symbol(env, unary));
            return NULL;
        }
        default:
            node_printf(symbol_call);
            todo();
    }
}

static void insert_store(
    Env* env,
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
        case NODE_VARIABLE_DEF:
            dest_name = get_node_name(symbol_call);
            break;
        case NODE_LLVM_REGISTER_SYM:
            dest_name = get_node_name(node_unwrap_llvm_register_sym(symbol_call)->node_src);
            break;
        default:
            node_printf(symbol_call);
            todo();
    }

    if (symbol_call->type == NODE_MEMBER_SYM_TYPED) {
        todo();
    } else {
        Node_store_another_node* store = node_unwrap_store_another_node(node_new(symbol_call->pos, NODE_STORE_ANOTHER_NODE));
        store->node_src = get_node_src(symbol_call);
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
    if (operator->type == NODE_OP_UNARY) {
        Node* new_child;
        Node_op_unary* unary_oper = node_unwrap_op_unary(operator);
        insert_load(env, &new_child, block_children, idx_to_insert_before, unary_oper->child);
        unary_oper->child = new_child;
    } else if (operator->type == NODE_OP_BINARY) {
        Node* new_lhs;
        Node* new_rhs;
        Node_op_binary* bin_oper = node_unwrap_op_binary(operator);
        insert_load(env, &new_lhs, block_children, idx_to_insert_before, bin_oper->lhs);
        insert_load(env, &new_rhs, block_children, idx_to_insert_before, bin_oper->rhs);
        bin_oper->lhs = new_lhs;
        bin_oper->rhs = new_rhs;
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
        Node** arg = vec_at_ref(&function_call->args, idx);
        Node* new_arg;
        insert_load(env, &new_arg, block_children, idx_to_insert_before, *arg);
        *arg = new_arg;
    }
}

static Node_llvm_register_sym* move_operator_back(
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* operator
) {
    Node_llvm_register_sym* operator_sym = node_unwrap_llvm_register_sym(
        node_new(node_wrap_operator(node_wrap_operator_generic(operator))->pos, NODE_LLVM_REGISTER_SYM)
    );
    operator_sym->node_src = node_wrap_operator(node_wrap_operator_generic(operator));
    operator_sym->lang_type = get_operator_lang_type(operator);
    assert(operator_sym->lang_type.str.count > 0);
    assert(operator);
    insert_into_node_ptr_vec(
        block_children,
        idx_to_insert_before,
        *idx_to_insert_before,
        node_wrap_operator(node_wrap_operator_generic(operator))
    );
    assert(operator_sym);
    return operator_sym;
}

static Node* get_store_assignment(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_assignment* assignment
) {
    Node* store_dest = NULL;
    Node* store_src = NULL;
    Lang_type lhs_load_lang_type = {0};
    Lang_type rhs_load_lang_type = {0};

    switch (assignment->lhs->type) {
        case NODE_VARIABLE_DEF:
            store_dest = get_storage_location(env, get_node_name(assignment->lhs));
            lhs_load_lang_type = node_unwrap_variable_def(assignment->lhs)->lang_type;
            break;
        case NODE_SYMBOL_TYPED:
            store_dest = get_storage_location(env, get_node_name(assignment->lhs));
            lhs_load_lang_type = node_unwrap_symbol_typed(assignment->lhs)->lang_type;
            break;
        case NODE_MEMBER_SYM_TYPED: {
            store_dest = node_wrap_load_element_ptr(do_load_struct_element_ptr(
                env,
                block_children,
                idx_to_insert_before,
                node_unwrap_member_sym_typed(assignment->lhs)
            ));
            lhs_load_lang_type = get_member_sym_piece_final_lang_type(
                node_unwrap_member_sym_typed(assignment->lhs)
            );
            break;
        }
        case NODE_OPERATOR: {
            Node_op_unary* unary = node_unwrap_op_unary(node_unwrap_operator(assignment->lhs));
            if (unary->token_type != TOKEN_DEREF) {
                todo();
            }
            if (is_corresponding_to_a_struct(env, unary->child)) {
                if (get_lang_type(unary->child).pointer_depth > 0) {
                    store_dest = get_storage_location(env, get_node_name(unary->child));
                } else {
                    todo();
                }
            } else {
                Node* new_child;
                store_dest = node_wrap_load_another_node(insert_load(env, &new_child, block_children, idx_to_insert_before, unary->child));
                unary->child = new_child;
            }
            lhs_load_lang_type = unary->lang_type;
            break;
        }
        default:
            unreachable(NODE_FMT, node_print(assignment->lhs));
    }
   
    switch (assignment->rhs->type) {
        case NODE_VARIABLE_DEF:
            todo();
            break;
        case NODE_STRUCT_LITERAL: {
            Node* new_rhs;
            store_src = node_wrap_load_another_node(insert_load(env, &new_rhs, block_children, idx_to_insert_before, assignment->rhs));
            assignment->rhs = new_rhs;
            rhs_load_lang_type = node_unwrap_struct_literal(assignment->rhs)->lang_type;
            assert(rhs_load_lang_type.str.count > 0);
            break;
        }
        case NODE_SYMBOL_TYPED: {
            Node* new_rhs;
            store_src = node_wrap_load_another_node(insert_load(env, &new_rhs, block_children, idx_to_insert_before, assignment->rhs));
            assignment->rhs = new_rhs;
            assert(store_src);
            rhs_load_lang_type = node_unwrap_llvm_register_sym(assignment->rhs)->lang_type;
            break;
        }
        case NODE_LITERAL: {
            Node* new_rhs;
            store_src = node_wrap_load_another_node(insert_load(env, &new_rhs, block_children, idx_to_insert_before, assignment->rhs));
            assignment->rhs = new_rhs;
            rhs_load_lang_type = node_unwrap_literal(assignment->rhs)->lang_type;
            break;
        }
        case NODE_LLVM_REGISTER_SYM:
            store_src = node_unwrap_llvm_register_sym(assignment->rhs)->node_src;
            rhs_load_lang_type = node_unwrap_llvm_register_sym(assignment->rhs)->lang_type;
            break;
        case NODE_MEMBER_SYM_TYPED: {
            Node* new_rhs;
            store_src = node_wrap_load_another_node(insert_load(env, &new_rhs, block_children, idx_to_insert_before, assignment->rhs));
            assignment->rhs = new_rhs;
            rhs_load_lang_type = node_unwrap_llvm_register_sym(assignment->rhs)->lang_type;
            break;
        }
        case NODE_OPERATOR: {
            Node_operator* operator = node_unwrap_operator(assignment->rhs);
            if (operator->type == NODE_OP_UNARY) {
                Node_op_unary* unary = node_unwrap_op_unary(operator);
                if (unary->token_type == TOKEN_DEREF) {
                    Node* store_src_imm;
                    if (unary->child->type == NODE_MEMBER_SYM_TYPED) {
                        todo();
                    } else {
                        Node* new_child;
                        store_src_imm = node_wrap_load_another_node(insert_load(env, &new_child, block_children, idx_to_insert_before, unary->child));
                        unary->child = new_child;
                    }
                    Node* new_store_imm;
                    Node_load_another_node* store_src_ = insert_load(env, &new_store_imm, block_children, idx_to_insert_before, store_src_imm);
                    store_src_imm = new_store_imm;

                    store_src_->lang_type.pointer_depth--;
                    store_src = node_wrap_load_another_node(store_src_);
                    rhs_load_lang_type = unary->lang_type;
                    break;
                } else if (unary->token_type == TOKEN_REFER) {
                    store_src = get_storage_location(env, get_node_name(unary->child));
                    rhs_load_lang_type = unary->lang_type;
                    break;
                }
            }
            Node_llvm_register_sym* store_src_ = move_operator_back(block_children, idx_to_insert_before, operator);
            store_src = store_src_->node_src;
            rhs_load_lang_type = store_src_->lang_type;
            break;
        }
        case NODE_FUNCTION_CALL:
            store_src = assignment->rhs;
            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                assignment->rhs
            );
            Node_function_decl* fun_decl;
            {
                Node* fun_def;
                try(symbol_lookup(&fun_def, env, get_node_name(assignment->rhs)));
                if (fun_def->type == NODE_FUNCTION_DEF) {
                    fun_decl = node_unwrap_function_def(fun_def)->declaration;
                } else {
                    fun_decl = node_unwrap_function_decl(fun_def);
                }
            }
            node_unwrap_function_call(store_src)->lang_type = fun_decl->return_type->lang_type;
            assert(store_src);
            rhs_load_lang_type = node_unwrap_function_call(assignment->rhs)->lang_type;
            break;
        default:
            assert(false);
            todo();
    }

    if (assignment->rhs->type == NODE_LITERAL) {
        Node_llvm_store_literal* store = node_unwrap_llvm_store_literal(node_new(assignment->rhs->pos, NODE_LLVM_STORE_LITERAL));
        store->node_dest = store_dest;
        store->lang_type = lhs_load_lang_type;
        assert(store->lang_type.str.count > 0);
        store->child = node_unwrap_literal(assignment->rhs);
        return node_wrap_llvm_store_literal(store);
    } else if (assignment->rhs->type == NODE_STRUCT_LITERAL) {
        Node_llvm_store_struct_literal* store = node_unwrap_llvm_store_struct_literal(node_new(assignment->lhs->pos, NODE_LLVM_STORE_STRUCT_LITERAL));
        store->child = node_unwrap_struct_literal(assignment->rhs);
        store->node_dest = store_dest;
        return node_wrap_llvm_store_struct_literal(store);
    } else {
        Node_store_another_node* store = node_unwrap_store_another_node(node_new(assignment->lhs->pos, NODE_STORE_ANOTHER_NODE));
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
            insert_load(env, &new_child, block_children, idx_to_insert_before, return_statement->child);
            return_statement->child = new_child;
            return;
        }
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_LLVM_REGISTER_SYM:
            // fallthrough
        case NODE_LITERAL:
            return;
        case NODE_OPERATOR:
            if (return_statement->child->type == NODE_OPERATOR) {
                return_statement->child = node_wrap_llvm_register_sym(move_operator_back(
                    block_children,
                    idx_to_insert_before,
                    node_unwrap_operator(return_statement->child)
                ));
                assert(return_statement->child);
            }
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(return_statement->child));
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
        Node_llvm_register_sym* fun_param_call = node_unwrap_llvm_register_sym(node_new(param->pos, NODE_LLVM_REGISTER_SYM));
        fun_param_call->node_src = param;
        fun_param_call->lang_type = get_lang_type(param);
        size_t idx_insert_before = get_idx_node_after_last_alloca(fun_def->body);
        insert_store(env, &fun_def->body->children, &idx_insert_before, node_wrap_llvm_register_sym(fun_param_call));
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
    Node* operand
) {
    if (operand->type == NODE_OPERATOR) {
        Node_operator* child = node_unwrap_operator(operand);
        if (child->type == NODE_OP_UNARY) {
            Node_op_unary* unary = node_unwrap_op_unary(child);
            if (unary->token_type == TOKEN_DEREF) {
                Node* new_child;
                Node* store_src_imm = node_wrap_load_another_node(insert_load(env, &new_child, block_children, idx_to_insert_before, unary->child));
                unary->child = new_child;

                Node* new_store_src_imm;
                Node_load_another_node* store_src_ = insert_load(env, &new_store_src_imm, block_children, idx_to_insert_before, store_src_imm);
                store_src_imm = new_store_src_imm;

                store_src_->lang_type.pointer_depth--;
                Node_llvm_register_sym* reg_sym = node_unwrap_llvm_register_sym(node_new(operand->pos, NODE_LLVM_REGISTER_SYM));
                reg_sym->node_src = node_wrap_load_another_node(store_src_);
                return node_wrap_llvm_register_sym(reg_sym);
            } else if (unary->token_type == TOKEN_UNSAFE_CAST) {
                Node* operator_sym = node_new(operand->pos, NODE_LLVM_REGISTER_SYM);

                insert_into_node_ptr_vec(
                    block_children,
                    idx_to_insert_before,
                    *idx_to_insert_before,
                    operand
                );
                node_unwrap_llvm_register_sym(operator_sym)->node_src = operand;
                node_unwrap_llvm_register_sym(operator_sym)->lang_type = get_lang_type(operand);
                assert(node_unwrap_llvm_register_sym(operator_sym)->node_src);
                return operator_sym;
            } else {
                todo();
            }
        } else if (child->type == NODE_OP_BINARY) {
            Node* operator_sym = node_new(operand->pos, NODE_LLVM_REGISTER_SYM);

            insert_into_node_ptr_vec(
                block_children,
                idx_to_insert_before,
                *idx_to_insert_before,
                operand
            );
            node_unwrap_llvm_register_sym(operator_sym)->node_src = operand;
            node_unwrap_llvm_register_sym(operator_sym)->lang_type = get_lang_type(operand);
            assert(node_unwrap_llvm_register_sym(operator_sym)->node_src);
            return operator_sym;
        } else {
            unreachable("");
        }
        unreachable("");
    } else if (operand->type == NODE_FUNCTION_CALL) {
        Node_llvm_register_sym* fun_sym = node_unwrap_llvm_register_sym(node_new(operand->pos, NODE_LLVM_REGISTER_SYM));
        fun_sym->node_src = operand;
        fun_sym->lang_type = get_lang_type(operand);
        assert(fun_sym->node_src);
        insert_into_node_ptr_vec(
            block_children,
            idx_to_insert_before,
            *idx_to_insert_before,
            operand
        );
        return node_wrap_llvm_register_sym(fun_sym);
    } else {
        assert(operand);
        return operand;
    }
}

static void flatten_operator_if_nessessary(
    Env* env,
    Node_ptr_vec* block_children,
    size_t* idx_to_insert_before,
    Node_operator* old_operator
) {
    if (old_operator->type == NODE_OP_UNARY) {
        Node_op_unary* old_unary_op = node_unwrap_op_unary(old_operator);
        old_unary_op->child = flatten_operator_operand(env, block_children, idx_to_insert_before, old_unary_op->child);
    } else if (old_operator->type == NODE_OP_BINARY) {
        Node_op_binary* old_bin_op = node_unwrap_op_binary(old_operator);
        old_bin_op->lhs = flatten_operator_operand(env, block_children, idx_to_insert_before, old_bin_op->lhs);
        old_bin_op->rhs = flatten_operator_operand(env, block_children, idx_to_insert_before, old_bin_op->rhs);
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

        switch (curr_node->type) {
            case NODE_STRUCT_LITERAL:
                do_struct_literal(env, &block->children, &idx, node_unwrap_struct_literal(curr_node));
                break;
            case NODE_LITERAL:
                break;
            case NODE_FUNCTION_CALL:
                load_function_arguments(env, &block->children, &idx, node_unwrap_function_call(curr_node));
                break;
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
            case NODE_OPERATOR:
                flatten_operator_if_nessessary(env, &block->children, &idx, node_unwrap_operator(curr_node));
                load_operator_operands(env, &block->children, &idx, node_unwrap_operator(curr_node));
                break;
            case NODE_BLOCK:
                break;
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbol should not still be present");
            case NODE_SYMBOL_TYPED:
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
            case NODE_MEMBER_SYM_TYPED:
                break;
            case NODE_LLVM_REGISTER_SYM:
                break;
            case NODE_STRUCT_DEF:
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

    return;
}
