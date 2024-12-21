#include <type_checking.h>
#include <parser_utils.h>
#include <node.h>

// result is rounded up
static int64_t log2_int64_t(int64_t num) {
    int64_t reference = 1;
    for (unsigned int power = 0; power < 64; power++) {
        if (num <= reference) {
            return power;
        }

        reference *= 2;
    }
    unreachable("");
}

static int64_t bit_width_needed_signed(int64_t num) {
    return 1 + log2_int64_t(num + 1);
}

//static int64_t bit_width_needed_unsigned(int64_t num) {
//    return MAX(1, log2(num + 1));
//}

static Node_expr* auto_deref_to_0(const Env* env, Node_expr* expr) {
    while (get_lang_type_expr(expr).pointer_depth > 0) {
        Lang_type init_lang_type = {0};
        expr = util_unary_new(env, expr, TOKEN_DEREF, init_lang_type);
    }
    return expr;
}

const Node_function_def* get_parent_function_def_const(const Env* env) {
    if (env->ancesters.info.count < 1) {
        unreachable("");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&env->ancesters, idx);
        if (curr_node->type == NODE_DEF) {
            Node_def* curr_def = node_unwrap_def(curr_node);

            if (curr_def->type == NODE_FUNCTION_DEF) {
                return node_unwrap_function_def(curr_def);
            }
        }

        if (idx < 1) {
            unreachable("");
        }
    }
}

Lang_type get_parent_function_return_type(const Env* env) {
    return get_parent_function_def_const(env)->declaration->return_type->lang_type;
}

static bool can_be_implicitly_converted(const Env* env, Lang_type dest, Lang_type src, bool implicit_pointer_depth) {
    (void) env;
    //Node* result = NULL;
    //if (symbol_lookup(&result, env, dest.str)) {
    //    log_tree(LOG_DEBUG, result);
    //    unreachable("");
    //}

    if (!implicit_pointer_depth) {
        if (src.pointer_depth != dest.pointer_depth) {
            return false;
        }
    }

    if (!lang_type_is_signed(dest) || !lang_type_is_signed(src)) {
        return lang_type_is_equal(dest, src);
    }
    return i_lang_type_to_bit_width(dest) >= i_lang_type_to_bit_width(src);
}

typedef enum {
    IMPLICIT_CONV_INVALID_TYPES,
    IMPLICIT_CONV_CONVERTED,
    IMPLICIT_CONV_OK,
} IMPLICIT_CONV_STATUS;

static IMPLICIT_CONV_STATUS do_implicit_conversion_if_needed(
    const Env* env,
    Lang_type dest_lang_type,
    Node_expr* src,
    bool inplicit_pointer_depth
) {
    Lang_type src_lang_type = get_lang_type_expr(src);
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );

    if (lang_type_is_equal(dest_lang_type, src_lang_type)) {
        return IMPLICIT_CONV_OK;
    }
    if (!can_be_implicitly_converted(env, dest_lang_type, src_lang_type, inplicit_pointer_depth)) {
        return IMPLICIT_CONV_INVALID_TYPES;
    }

    *get_lang_type_expr_ref(src) = dest_lang_type;
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );
    return IMPLICIT_CONV_CONVERTED;
}

bool check_generic_assignment_finish_symbol_untyped(
    const Env* env,
    Node_expr** new_src,
    Node_symbol_untyped* src
) {
    (void) env;
    (void) new_src;
    (void) src;
    todo();
    //switch (src->type) {
    //    case NODE_STRING
}

static void msg_invalid_function_arg_internal(
    const char* file,
    int line,
    const Env* env,
    const Node_expr* argument,
    const Node_variable_def* corres_param
) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env->file_text, node_get_pos_expr(argument), 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(get_lang_type_expr(argument)), 
        str_view_print(corres_param->name),
        lang_type_print(corres_param->lang_type)
    );
    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, corres_param->pos,
        "corresponding parameter `"STR_VIEW_FMT"` defined here\n",
        str_view_print(corres_param->name)
    );
}

#define msg_invalid_function_arg(env, argument, corres_param) \
    msg_invalid_function_arg_internal(__FILE__, __LINE__, env, argument, corres_param)

static void msg_invalid_return_type_internal(const char* file, int line, const Env* env, const Node_return* rtn) {
    const Node_function_def* fun_def = get_parent_function_def_const(env);
    if (rtn->is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env->file_text, rtn->pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            lang_type_print(fun_def->declaration->return_type->lang_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, rtn->pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(get_lang_type_expr(rtn->child)), 
            lang_type_print(fun_def->declaration->return_type->lang_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, fun_def->declaration->return_type->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(fun_def->declaration->return_type->lang_type)
    );
}

#define msg_invalid_return_type(env, rtn) \
    msg_invalid_return_type_internal(__FILE__, __LINE__, env, rtn)

typedef enum {
    CHECK_ASSIGN_OK,
    CHECK_ASSIGN_INVALID,
} CHECK_ASSIGN_STATUS;

CHECK_ASSIGN_STATUS check_generic_assignment(
    const Env* env,
    Node_expr** new_src,
    Lang_type dest_lang_type,
    Node_expr* src
) {
    if (lang_type_is_equal(dest_lang_type, get_lang_type_expr(src))) {
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(env, dest_lang_type, get_lang_type_expr(src), false)) {
        if (src->type == NODE_LITERAL) {
            *new_src = src;
            *get_lang_type_literal_ref(node_unwrap_literal(*new_src)) = dest_lang_type;
            return CHECK_ASSIGN_OK;
        } else {
            todo();
        }
        log(LOG_DEBUG, LANG_TYPE_FMT "   "NODE_FMT"\n", lang_type_print(dest_lang_type), node_print((Node*)src));
        todo();
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

void try_set_literal_types(Lang_type* lang_type, Node_literal* literal, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            node_unwrap_string(literal)->lang_type = lang_type_new_from_cstr("u8", 1);
            break;
        case TOKEN_INT_LITERAL: {
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            int64_t bit_width = bit_width_needed_signed(node_unwrap_number(literal)->data);
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            node_unwrap_number(literal)->lang_type = lang_type_new_from_strv(string_to_strv(lang_type_str), 0);
            break;
        }
        case TOKEN_VOID:
            break;
        case TOKEN_CHAR_LITERAL:
            node_unwrap_char(literal)->lang_type = lang_type_new_from_cstr("u8", 0);
            break;
        default:
            unreachable("");
    }

    *lang_type = get_lang_type_literal(literal);
}

static void msg_undefined_symbol(Str_view file_text, const Node* sym_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, node_get_pos(sym_call),
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_node_name(sym_call))
    );
}

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_symbol_untyped* sym_untyped) {
    Node_def* sym_def;
    if (!symbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, node_wrap_expr(node_wrap_symbol_untyped(sym_untyped)));
        return false;
    }

    *lang_type = get_lang_type_def(sym_def);

    Sym_typed_base new_base = {.lang_type = *lang_type, .name = sym_untyped->name};
    if (lang_type_is_struct(env, *lang_type)) {
        Node_struct_sym* sym_typed = node_struct_sym_new(sym_untyped->pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_struct_sym(sym_typed));
        return true;
    } else if (lang_type_is_raw_union(env, *lang_type)) {
        Node_raw_union_sym* sym_typed = node_raw_union_sym_new(sym_untyped->pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_raw_union_sym(sym_typed));
        return true;
    } else if (lang_type_is_enum(env, *lang_type)) {
        Node_enum_sym* sym_typed = node_enum_sym_new(sym_untyped->pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_enum_sym(sym_typed));
        return true;
    } else if (lang_type_is_primitive(env, *lang_type)) {
        Node_primitive_sym* sym_typed = node_primitive_sym_new(sym_untyped->pos);
        sym_typed->base = new_base;
        *new_node = node_wrap_symbol_typed(node_wrap_primitive_sym(sym_typed));
        return true;
    } else {
        unreachable("uncaught undefined lang_type");
    }

    unreachable("");
}

static int64_t precalulate_number_internal(int64_t lhs_val, int64_t rhs_val, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_SINGLE_PLUS:
            return lhs_val + rhs_val;
        case TOKEN_SINGLE_MINUS:
            return lhs_val - rhs_val;
        case TOKEN_ASTERISK:
            return lhs_val*rhs_val;
        case TOKEN_SLASH:
            return lhs_val/rhs_val;
        case TOKEN_LESS_THAN:
            return lhs_val < rhs_val ? 1 : 0;
        case TOKEN_GREATER_THAN:
            return lhs_val > rhs_val ? 1 : 0;
        case TOKEN_DOUBLE_EQUAL:
            return lhs_val == rhs_val ? 1 : 0;
        case TOKEN_NOT_EQUAL:
            return lhs_val != rhs_val ? 1 : 0;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(token_type));
    }
    unreachable("");
}

static Node_literal* precalulate_number(
    const Node_number* lhs,
    const Node_number* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static Node_literal* precalulate_char(
    const Node_char* lhs,
    const Node_char* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

// returns false if unsuccessful
bool try_set_binary_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_binary* operator) {
    Lang_type dummy;
    log_tree(LOG_DEBUG, (Node*)operator->lhs);
    Node_expr* new_lhs;
    if (!try_set_expr_types(env, &new_lhs, &dummy, operator->lhs)) {
        return false;
    }
    assert(new_lhs);
    operator->lhs = new_lhs;

    Node_expr* new_rhs;
    if (!try_set_expr_types(env, &new_rhs, &dummy, operator->rhs)) {
        return false;
    }
    operator->rhs = new_rhs;
    assert(operator->lhs);
    assert(operator->rhs);
    log_tree(LOG_DEBUG, (Node*)operator);

    if (!lang_type_is_equal(get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs))) {
        if (can_be_implicitly_converted(env, get_lang_type_expr(operator->lhs), get_lang_type_expr(operator->rhs), true)) {
            if (operator->rhs->type == NODE_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                *get_lang_type_literal_ref(node_unwrap_literal(operator->rhs)) = get_lang_type_expr(operator->lhs);
            } else {
                Node_expr* unary = util_unary_new(env, operator->rhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->lhs));
                operator->rhs = unary;
            }
        } else if (can_be_implicitly_converted(env, get_lang_type_expr(operator->rhs), get_lang_type_expr(operator->lhs), true)) {
            if (operator->lhs->type == NODE_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                *get_lang_type_literal_ref(node_unwrap_literal(operator->lhs)) = get_lang_type_expr(operator->rhs);
            } else {
                Node_expr* unary = util_unary_new(env, operator->lhs, TOKEN_UNSAFE_CAST, get_lang_type_expr(operator->rhs));
                operator->lhs = unary;
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, operator->pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(get_lang_type_expr(operator->lhs)), lang_type_print(get_lang_type_expr(operator->rhs))
            );
            return false;
        }
    }
            
    assert(get_lang_type_expr(operator->lhs).str.count > 0);
    *lang_type = get_lang_type_expr(operator->lhs);
    operator->lang_type = get_lang_type_expr(operator->lhs);

    // precalcuate binary in some situations
    if (operator->lhs->type == NODE_LITERAL && operator->rhs->type == NODE_LITERAL) {
        Node_literal* lhs_lit = node_unwrap_literal(operator->lhs);
        Node_literal* rhs_lit = node_unwrap_literal(operator->rhs);

        if (lhs_lit->type != rhs_lit->type) {
            // TODO: make expected failure test for this
            unreachable("mismatched types");
        }

        Node_literal* literal = NULL;

        switch (lhs_lit->type) {
            case NODE_NUMBER:
                literal = precalulate_number(
                    node_unwrap_number_const(lhs_lit),
                    node_unwrap_number_const(rhs_lit),
                    operator->token_type,
                    operator->pos
                );
                break;
            case NODE_CHAR:
                literal = precalulate_char(
                    node_unwrap_char_const(lhs_lit),
                    node_unwrap_char_const(rhs_lit),
                    operator->token_type,
                    operator->pos
                );
                break;
            default:
                unreachable("");
        }

        *new_node = node_wrap_literal(literal);
        log_tree(LOG_DEBUG, node_wrap_expr(*new_node));
    } else {
        *new_node = node_wrap_operator(node_wrap_binary(operator));
    }

    assert(get_lang_type_expr(*new_node).str.count > 0);
    assert(*new_node);
    return true;
}

bool try_set_unary_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_unary* unary) {
    assert(lang_type);
    Lang_type init_lang_type;
    Node_expr* new_child;
    if (!try_set_expr_types(env, &new_child, &init_lang_type, unary->child)) {
        return false;
    }
    unary->child = new_child;

    switch (unary->token_type) {
        case TOKEN_NOT:
            unary->lang_type = init_lang_type;
            *lang_type = unary->lang_type;
            break;
        case TOKEN_DEREF:
            unary->lang_type = init_lang_type;
            if (unary->lang_type.pointer_depth <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, unary->pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            unary->lang_type.pointer_depth--;
            *lang_type = unary->lang_type;
            break;
        case TOKEN_REFER:
            unary->lang_type = init_lang_type;
            unary->lang_type.pointer_depth++;
            assert(unary->lang_type.pointer_depth > 0);
            *lang_type = unary->lang_type;
            break;
        case TOKEN_UNSAFE_CAST:
            assert(unary->lang_type.str.count > 0);
            if (unary->lang_type.pointer_depth > 0 && lang_type_is_number(get_lang_type_expr(unary->child))) {
                *lang_type = init_lang_type;
            } else if (lang_type_is_number(unary->lang_type) && get_lang_type_expr(unary->child).pointer_depth > 0) {
                *lang_type = init_lang_type;
            } else if (lang_type_is_number(unary->lang_type) && lang_type_is_number(get_lang_type_expr(unary->child))) {
                *lang_type = init_lang_type;
            } else if (unary->lang_type.pointer_depth > 0 && get_lang_type_expr(unary->child).pointer_depth > 0) {
                *lang_type = init_lang_type;
            } else {
                todo();
            }
            break;
        default:
            unreachable("");
    }

    *new_node = node_wrap_operator(node_wrap_unary(unary));
    return true;
}

// returns false if unsuccessful
bool try_set_operator_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_operator* operator) {
    if (operator->type == NODE_UNARY) {
        return try_set_unary_types(env, new_node, lang_type, node_unwrap_unary(operator));
    } else if (operator->type == NODE_BINARY) {
        return try_set_binary_types(env, new_node, lang_type, node_unwrap_binary(operator));
    } else {
        unreachable("");
    }
}

bool try_set_struct_literal_assignment_types(const Env* env, Node** new_node, Lang_type* lang_type, const Node* lhs, Node_struct_literal* struct_literal, Pos assign_pos) {
    //log(LOG_DEBUG, "------------------------------\n");
    //if (!is_corresponding_to_a_struct(env, lhs)) {
    //    todo(); // non_struct assigned struct literal
    //}
    Node_def* lhs_var_def_;
    try(symbol_lookup(&lhs_var_def_, env, get_node_name(lhs)));
    Node_variable_def* lhs_var_def = node_unwrap_variable_def(lhs_var_def_);
    Node_def* struct_def_;
    try(symbol_lookup(&struct_def_, env, lhs_var_def->lang_type.str));
    switch (struct_def_->type) {
        case NODE_STRUCT_DEF:
            break;
        case NODE_RAW_UNION_DEF:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env->file_text,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        default:
            log(LOG_DEBUG, NODE_FMT"\n", node_print(node_wrap_def(struct_def_)));
            unreachable("");
    }
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    Lang_type new_lang_type = {.str = struct_def->base.name, .pointer_depth = 0};
    struct_literal->lang_type = new_lang_type;
    
    Node_ptr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        //log(LOG_DEBUG, "%zu\n", idx);
        Node* memb_sym_def_ = vec_at(&struct_def->base.members, idx);
        Node_variable_def* memb_sym_def = node_unwrap_variable_def(node_unwrap_def(memb_sym_def_));
        log_tree(LOG_DEBUG, node_wrap_expr_const(node_wrap_struct_literal_const(struct_literal)));
        Node_assignment* assign_memb_sym = node_unwrap_assignment(vec_at(&struct_literal->members, idx));
        Node_symbol_untyped* memb_sym_piece_untyped = node_unwrap_symbol_untyped(node_unwrap_expr(assign_memb_sym->lhs));
        Node_expr* new_rhs = NULL;
        Lang_type dummy;
        if (!try_set_expr_types(env, &new_rhs, &dummy, assign_memb_sym->rhs)) {
            unreachable("");
        }
        Node_literal* assign_memb_sym_rhs = node_unwrap_literal(new_rhs);

        *get_lang_type_literal_ref(assign_memb_sym_rhs) = memb_sym_def->lang_type;
        if (!str_view_is_equal(memb_sym_def->name, memb_sym_piece_untyped->name)) {
            msg(
                LOG_ERROR, EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL, env->file_text,
                memb_sym_piece_untyped->pos,
                "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
                str_view_print(memb_sym_def->name), str_view_print(memb_sym_piece_untyped->name)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, lhs_var_def->pos,
                "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
                str_view_print(lhs_var_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            msg(
                LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, memb_sym_def->pos,
                "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
                str_view_print(memb_sym_def->name), lang_type_print(lhs_var_def->lang_type)
            );
            return false;
        }

        vec_append_safe(&a_main, &new_literal_members, node_wrap_expr(node_wrap_literal(assign_memb_sym_rhs)));
    }

    struct_literal->members = new_literal_members;

    assert(struct_literal->lang_type.str.count > 0);

    *lang_type = new_lang_type;
    *new_node = node_wrap_expr(node_wrap_struct_literal(struct_literal));
    return true;
}

bool try_set_expr_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_expr* node) {
    switch (node->type) {
        case NODE_LITERAL:
            *lang_type = get_lang_type_literal(node_unwrap_literal(node));
            *new_node = node;
            return true;
        case NODE_SYMBOL_UNTYPED:
            if (!try_set_symbol_type(env, new_node, lang_type, node_unwrap_symbol_untyped(node))) {
                return false;
            } else {
                assert(*new_node);
            }
            return true;
        case NODE_MEMBER_ACCESS_UNTYPED: {
            Node* new_node_ = NULL;
            if (!try_set_member_access_types(env, &new_node_, lang_type, node_unwrap_member_access_untyped(node))) {
                return false;
            }
            *new_node = node_unwrap_expr(new_node_);
            return true;
        }
        case NODE_MEMBER_ACCESS_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_types(node_unwrap_member_sym_typed(node));
            //*new_node = node;
            //return true;
        case NODE_INDEX_UNTYPED: {
            Node* new_node_ = NULL;
            if (!try_set_index_untyped_types(env, &new_node_, lang_type, node_unwrap_index_untyped(node))) {
                return false;
            }
            *new_node = node_unwrap_expr(new_node_);
            return true;
        }
        case NODE_INDEX_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_types(node_unwrap_member_sym_typed(node));
            //*new_node = node;
            //return true;
        case NODE_SYMBOL_TYPED:
            *lang_type = get_lang_type_symbol_typed(node_unwrap_symbol_typed(node));
            *new_node = node;
            return true;
        case NODE_OPERATOR:
            if (!try_set_operator_types(env, new_node, lang_type, node_unwrap_operator(node))) {
                return false;
            }
            assert(*new_node);
            return true;
        case NODE_FUNCTION_CALL:
            return try_set_function_call_types(env, new_node, lang_type, node_unwrap_function_call(node));
        case NODE_STRUCT_LITERAL:
            unreachable("cannot set struct literal type here");
        case NODE_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

bool try_set_def_types(Env* env, Node_def** new_node, Lang_type* lang_type, Node_def* node) {
    switch (node->type) {
        case NODE_VARIABLE_DEF: {
            Node_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, lang_type, node_unwrap_variable_def(node))) {
                return false;
            }
            *new_node = node_wrap_variable_def(new_def);
            return true;
        }
        case NODE_FUNCTION_DECL:
            // TODO: do this?
            *new_node = node;
            return true;
        case NODE_FUNCTION_DEF: {
            Node_function_def* new_def = NULL;
            if (!try_set_function_def_types(env, &new_def, lang_type, node_unwrap_function_def(node))) {
                return false;
            }
            *new_node = node_wrap_function_def(new_def);
            return true;
        }
        case NODE_STRUCT_DEF: {
            Node_struct_def* new_def = NULL;
            if (!try_set_struct_def_types(env, &new_def, lang_type, node_unwrap_struct_def(node))) {
                return false;
            }
            *new_node = node_wrap_struct_def(new_def);
            return true;
        }
        case NODE_RAW_UNION_DEF: {
            Node_raw_union_def* new_def = NULL;
            if (!try_set_raw_union_def_types(env, &new_def, lang_type, node_unwrap_raw_union_def(node))) {
                return false;
            }
            *new_node = node_wrap_raw_union_def(new_def);
            return true;
        }
        case NODE_ENUM_DEF: {
            Node_enum_def* new_def = NULL;
            if (!try_set_enum_def_types(env, &new_def, lang_type, node_unwrap_enum_def(node))) {
                return false;
            }
            *new_node = node_wrap_enum_def(new_def);
            return true;
        }
        case NODE_PRIMITIVE_DEF:
            *new_node = node;
            return true;
        case NODE_LABEL:
            *new_node = node;
            return true;
        case NODE_LITERAL_DEF:
            *new_node = node;
            return true;
    }
    unreachable("");
}

bool try_set_assignment_types(Env* env, Lang_type* lang_type, Node_assignment* assignment) {
    Lang_type lhs_lang_type = {0};
    Lang_type rhs_lang_type = {0};
    Node* new_lhs = NULL;
    if (!try_set_node_types(env, &new_lhs, &lhs_lang_type, assignment->lhs)) { 
        return false;
    }
    assignment->lhs = new_lhs;

    Node_expr* new_rhs;
    if (assignment->rhs->type == NODE_STRUCT_LITERAL) {
        Node* new_rhs_ = NULL;
        // TODO: do this in check_generic_assignment
        if (!try_set_struct_literal_assignment_types(
            env, &new_rhs_, &rhs_lang_type, assignment->lhs, node_unwrap_struct_literal(assignment->rhs), assignment->pos
        )) {
            return false;
        }
        new_rhs = node_unwrap_expr(new_rhs_);
    } else {
        if (!try_set_expr_types(env, &new_rhs, &rhs_lang_type, assignment->rhs)) {
            return false;
        }
    }
    assignment->rhs = new_rhs;

    *lang_type = lhs_lang_type;

    if (CHECK_ASSIGN_OK != check_generic_assignment(env, &assignment->rhs, lhs_lang_type, new_rhs)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
            assignment->pos,
            "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
            lang_type_print(rhs_lang_type), lang_type_print(lhs_lang_type)
        );
        return false;
    }

    return true;
}

bool try_set_function_call_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_function_call* fun_call) {
    bool status = true;

    Node_def* fun_def;
    if (!symbol_lookup(&fun_def, env, fun_call->name)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env->file_text, fun_call->pos,
            "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
        );
        return false;
    }
    Node_function_decl* fun_decl;
    if (fun_def->type == NODE_FUNCTION_DEF) {
        fun_decl = node_unwrap_function_def(fun_def)->declaration;
    } else {
        fun_decl = node_unwrap_function_decl(fun_def);
    }
    Node_lang_type* fun_rtn_type = fun_decl->return_type;
    fun_call->lang_type = fun_rtn_type->lang_type;
    assert(fun_call->lang_type.str.count > 0);
    Node_function_params* params = fun_decl->parameters;
    size_t params_idx = 0;

    size_t min_args;
    size_t max_args;
    if (params->params.info.count < 1) {
        min_args = 0;
        max_args = 0;
    } else if (vec_top(&params->params)->is_variadic) {
        min_args = params->params.info.count - 1;
        max_args = SIZE_MAX;
    } else {
        min_args = params->params.info.count;
        max_args = params->params.info.count;
    }
    if (fun_call->args.info.count < min_args || fun_call->args.info.count > max_args) {
        String message = {0};
        string_extend_size_t(&print_arena, &message, fun_call->args.info.count);
        string_extend_cstr(&print_arena, &message, " arguments are passed to function `");
        string_extend_strv(&print_arena, &message, fun_call->name);
        string_extend_cstr(&print_arena, &message, "`, but ");
        string_extend_size_t(&print_arena, &message, min_args);
        if (max_args > min_args) {
            string_extend_cstr(&print_arena, &message, " or more");
        }
        string_extend_cstr(&print_arena, &message, " arguments expected\n");
        msg(
            LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env->file_text, fun_call->pos,
            STR_VIEW_FMT, str_view_print(string_to_strv(message))
        );

        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, node_get_pos_def(fun_def),
            "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
        );
        return false;
    }

    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Node_variable_def* corres_param = vec_at(&params->params, params_idx);
        Node_expr** argument = vec_at_ref(&fun_call->args, arg_idx);
        Node_expr* new_arg = NULL;

        if (!corres_param->is_variadic) {
            params_idx++;
        }

        Lang_type dummy = {0};
        Node* new_arg_ = NULL;
        if ((*argument)->type == NODE_STRUCT_LITERAL) {
            todo();
            try(try_set_struct_literal_assignment_types(
                env,
                &new_arg_,
                &dummy,
                node_wrap_def(node_wrap_variable_def(corres_param)),
                node_unwrap_struct_literal(*argument),
                fun_call->pos
            ));
            new_arg = node_unwrap_expr(new_arg_);
        } else {
            if (!try_set_expr_types(env, &new_arg, &dummy, *argument)) {
                status = false;
                continue;
            }
        }
        log_tree(LOG_DEBUG, node_wrap_expr(*argument));
        log_tree(LOG_DEBUG, node_wrap_expr(new_arg));
        assert(new_arg);

        *argument = new_arg;

        if (lang_type_is_equal(corres_param->lang_type, lang_type_new_from_cstr("any", 0))) {
            if (corres_param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                continue;
            } else {
                todo();
            }
        }

        Node_expr* new_new_arg = NULL;
        if (CHECK_ASSIGN_OK != check_generic_assignment(env, &new_new_arg, corres_param->lang_type, new_arg)) {
            msg_invalid_function_arg(env, new_arg, corres_param);
            return false;
        }

        *argument = new_new_arg;
        log_tree(LOG_DEBUG, node_wrap_expr(*argument));
    }

    *lang_type = fun_rtn_type->lang_type;
    *new_node = node_wrap_function_call(fun_call);
    return status;
}

static void msg_invalid_member(
    const Env* env,
    Struct_def_base base,
    const Node_member_access_untyped* access
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER, env->file_text,
        access->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(access->member_name), str_view_print(base.name)
    );

    // TODO: add notes for where struct def of callee is defined, etc.
}

bool try_set_member_access_types_finish_generic_struct(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_member_access_untyped* access,
    Struct_def_base def_base,
    Node_expr* new_callee
) {
    Node_variable_def* member_def = NULL;
    if (!try_get_member_def(&member_def, &def_base, access->member_name)) {
        msg_invalid_member(env, def_base, access);
        return false;
    }
    *lang_type = member_def->lang_type;

    Node_member_access_typed* new_access = node_member_access_typed_new(access->pos);
    new_access->lang_type = *lang_type;
    new_access->member_name = access->member_name;
    new_access->callee = new_callee;

    *new_node = node_wrap_expr(node_wrap_member_access_typed(new_access));

    assert(lang_type->str.count > 0);
    return true;
}

bool try_set_member_access_types_finish(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_def* lang_type_def,
    Node_member_access_untyped* access,
    Node_expr* new_callee
) {
    switch (lang_type_def->type) {
        case NODE_STRUCT_DEF: {
            Node_struct_def* struct_def = node_unwrap_struct_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_node, lang_type, access, struct_def->base, new_callee
            );
        }
        case NODE_RAW_UNION_DEF: {
            Node_raw_union_def* raw_union_def = node_unwrap_raw_union_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_node, lang_type, access, raw_union_def->base, new_callee
            );
        }
        case NODE_ENUM_DEF: {
            Node_enum_def* enum_def = node_unwrap_enum_def(lang_type_def);
            Node_variable_def* member_def = NULL;
            if (!try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
                todo();
            }
            *lang_type = member_def->lang_type;
            Node_enum_lit* new_lit = node_enum_lit_new(access->pos);
            new_lit->data = get_member_index(&enum_def->base, access->member_name);
            new_lit->lang_type = *lang_type;

            *new_node = node_wrap_expr(node_wrap_literal(node_wrap_enum_lit(new_lit)));
            assert(lang_type->str.count > 0);
            return true;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_def(lang_type_def)));
    }

    unreachable("");
}

bool try_set_member_access_types(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_member_access_untyped* access
) {
    Node_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, lang_type, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case NODE_SYMBOL_TYPED: {
            Node_symbol_typed* sym = node_unwrap_symbol_typed(new_callee);
            Node_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, get_lang_type_symbol_typed(sym).str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_node, lang_type, lang_type_def, access, new_callee);

        }
        case NODE_MEMBER_ACCESS_TYPED: {
            Node_member_access_typed* sym = node_unwrap_member_access_typed(new_callee);

            Node_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, sym->lang_type.str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_node, lang_type, lang_type_def, access, new_callee);
        }
        default:
            unreachable("");
    }
    unreachable("");
}

bool try_set_index_untyped_types(
    const Env* env,
    Node** new_node,
    Lang_type* lang_type,
    Node_index_untyped* index
) {
    Node_expr* new_callee = NULL;
    Node_expr* new_inner_index = NULL;
    if (!try_set_expr_types(env, &new_callee, lang_type, index->callee)) {
        return false;
    }
    if (!try_set_expr_types(env, &new_inner_index, lang_type, index->index)) {
        return false;
    }

    Lang_type new_lang_type = get_lang_type_expr(new_callee);
    if (new_lang_type.pointer_depth < 1) {
        todo();
    }
    new_lang_type.pointer_depth--;

    Node_index_typed* new_index = node_index_typed_new(index->pos);
    new_index->lang_type = new_lang_type;
    new_index->index = new_inner_index;
    new_index->callee = new_callee;

    *new_node = node_wrap_expr(node_wrap_index_typed(new_index));
    return true;
}

static bool try_set_condition_types(const Env* env, Lang_type* lang_type, Node_condition* if_cond) {
    Node_expr* new_if_cond_child;
    if (!try_set_operator_types(env, &new_if_cond_child, lang_type, if_cond->child)) {
        return false;
    }

    switch (new_if_cond_child->type) {
        case NODE_OPERATOR:
            if_cond->child = node_unwrap_operator(new_if_cond_child);
            break;
        case NODE_LITERAL:
            if_cond->child = condition_get_default_child(new_if_cond_child);
            break;
        case NODE_FUNCTION_CALL:
            if_cond->child = condition_get_default_child(new_if_cond_child);
            break;
        default:
            unreachable("");
    }

    return true;
}

bool try_set_struct_base_types(Env* env, Struct_def_base* base) {
    bool success = true;

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Node* curr = vec_at(&base->members, idx);

        Node* result_dummy = NULL;
        Lang_type lang_type_dummy = {0};
        if (!try_set_node_types(env, &result_dummy, &lang_type_dummy, curr)) {
            success = false;
        }
    }

    return success;
}

bool try_set_enum_def_types(Env* env, Node_enum_def** new_node, Lang_type* lang_type, Node_enum_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

bool try_set_raw_union_def_types(Env* env, Node_raw_union_def** new_node, Lang_type* lang_type, Node_raw_union_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

bool try_set_struct_def_types(Env* env, Node_struct_def** new_node, Lang_type* lang_type, Node_struct_def* node) {
    bool success = try_set_struct_base_types(env, &node->base);
    *lang_type = lang_type_new_from_strv(node->base.name, 0);
    *new_node = node;
    return success;
}

static void msg_undefined_type_internal(
    const char* file,
    int line,
    const Env* env,
    Pos pos,
    Lang_type lang_type
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_text, pos,
        "type `"LANG_TYPE_FMT"` is not defined\n", lang_type_print(lang_type)
    );
}

#define msg_undefined_type(env, pos, lang_type) \
    msg_undefined_type_internal(__FILE__, __LINE__, env, pos, lang_type)

bool try_set_variable_def_types(
    const Env* env,
    Node_variable_def** new_node,
    Lang_type* lang_type,
    Node_variable_def* node
) {
    Node_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, node->lang_type.str)) {
        msg_undefined_type(env, node->pos, node->lang_type);
        return false;
    }

    *lang_type = node->lang_type;
    *new_node = node;
    return true;
}

bool try_set_function_def_types(
    Env* env,
    Node_function_def** new_node,
    Lang_type* lang_type,
    Node_function_def* old_def
) {
    vec_append(&a_main, &env->ancesters, node_wrap_def(node_wrap_function_def(old_def)));

    bool status = true;

    Node_function_decl* new_decl = NULL;
    if (!try_set_function_decl_types(env, &new_decl, lang_type, old_def->declaration)) {
        status = false;
    }
    old_def->declaration = new_decl;

    size_t prev_ancesters_count = env->ancesters.info.count;
    Node_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, lang_type, old_def->body)) {
        status = false;
    }
    old_def->body = new_body;
    assert(prev_ancesters_count == env->ancesters.info.count);

    vec_rem_last(&env->ancesters);
    *new_node = old_def;
    return status;
}

bool try_set_function_decl_types(
    Env* env,
    Node_function_decl** new_node,
    Lang_type* lang_type,
    Node_function_decl* old_def
) {
    bool status = true;

    Node_function_params* new_params = NULL;
    if (!try_set_function_params_types(env, &new_params, lang_type, old_def->parameters)) {
        status = false;
    }
    old_def->parameters = new_params;

    Node_lang_type* new_rtn_type = NULL;
    if (!try_set_lang_type_types(env, &new_rtn_type, lang_type, old_def->return_type)) {
        status = false;
    }
    old_def->return_type = new_rtn_type;

    *new_node = old_def;
    return status;
}

bool try_set_function_params_types(
    Env* env,
    Node_function_params** new_node,
    Lang_type* lang_type,
    Node_function_params* old_params
) {
    memset(lang_type, 0, sizeof(*lang_type));
    bool status = true;

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Node_variable_def* old_def = vec_at(&old_params->params, idx);

        Node_variable_def* new_def = NULL;
        if (!try_set_variable_def_types(env, &new_def, lang_type, old_def)) {
            status = false;
        }

        *vec_at_ref(&old_params->params, idx) = new_def;
    }

    *new_node = old_params;
    return status;
}

bool try_set_lang_type_types(
    Env* env,
    Node_lang_type** new_node,
    Lang_type* lang_type,
    Node_lang_type* old_node
) {
    Node_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, old_node->lang_type.str)) {
        msg_undefined_type(env, old_node->pos, old_node->lang_type);
        return false;
    }

    *lang_type = old_node->lang_type;
    *new_node = old_node;
    return true;
}

bool try_set_return_types(const Env* env, Node_return** new_node, Lang_type* lang_type, Node_return* rtn) {
    *new_node = NULL;

    Node_expr* new_rtn_child;
    if (!try_set_expr_types(env, &new_rtn_child, lang_type, rtn->child)) {
        todo();
        return false;
    }
    rtn->child = new_rtn_child;

    //Lang_type src_lang_type = get_lang_type_expr(rtn->child);
    Lang_type dest_lang_type = get_parent_function_def_const(env)->declaration->return_type->lang_type;

    if (CHECK_ASSIGN_OK != check_generic_assignment(env, &new_rtn_child, dest_lang_type, rtn->child)) {
        msg_invalid_return_type(env, rtn);
        return false;
    }
    rtn->child = new_rtn_child;

    *new_node = rtn;
    return true;
}

bool try_set_for_range_types(Env* env, Node_for_range** new_node, Lang_type* lang_type, Node_for_range* old_for) {
    memset(lang_type, 0, sizeof(*lang_type));
    bool status = true;

    Lang_type dummy = {0};

    Node_variable_def* new_var_def = NULL;
    if (!try_set_variable_def_types(env, &new_var_def, &dummy, old_for->var_def)) {
        status = false;
    }
    old_for->var_def = new_var_def;

    Node_expr* new_lower = NULL;
    if (!try_set_expr_types(env, &new_lower, &dummy, old_for->lower_bound->child)) {
        status = false;
    }
    old_for->lower_bound->child = new_lower;

    Node_expr* new_upper = NULL;
    if (!try_set_expr_types(env, &new_upper, &dummy, old_for->upper_bound->child)) {
        status = false;
    }
    old_for->upper_bound->child = new_upper;

    Node_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, &dummy, old_for->body)) {
        status = false;
    }
    old_for->body = new_body;

    *new_node = old_for;
    return status;
}

bool try_set_for_with_cond_types(Env* env, Node_for_with_cond** new_node, Lang_type* lang_type, Node_for_with_cond* old_for) {
    memset(lang_type, 0, sizeof(*lang_type));
    bool status = true;

    Lang_type dummy = {0};

    if (!try_set_condition_types(env, &dummy, old_for->condition)) {
        status = false;
    }

    Node_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, &dummy, old_for->body)) {
        status = false;
    }
    old_for->body = new_body;

    *new_node = old_for;
    return status;
}

bool try_set_if_types(Env* env, Node_if** new_node, Lang_type* lang_type, Node_if* old_if) {
    bool status = true;

    if (!try_set_condition_types(env, lang_type, old_if->condition)) {
        status = false;
    }

    Node_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, lang_type, old_if->body)) {
        status = false;
    }
    old_if->body = new_body;

    *new_node = old_if;
    return status;
}

bool try_set_if_else_chain(Env* env, Node_if_else_chain** new_node, Lang_type* lang_type, Node_if_else_chain* old_if_else) {
    bool status = true;

    for (size_t idx = 0; idx < old_if_else->nodes.info.count; idx++) {
        Node_if** old_if = vec_at_ref(&old_if_else->nodes, idx);
                
        Node_if* new_if = NULL;
        if (!try_set_if_types(env, &new_if, lang_type, *old_if)) {
            status = false;
        }
        *old_if = new_if;
    }

    *new_node = old_if_else;
    return status;
}

static bool is_directly_in_function_def(const Env* env) {
    return 
        env->ancesters.info.count > 1 &&
        vec_at(&env->ancesters, env->ancesters.info.count - 2)->type == NODE_DEF && 
        node_unwrap_def(vec_at(&env->ancesters, env->ancesters.info.count - 2))->type == NODE_FUNCTION_DEF;
}

// TODO: consider if lang_type result should be removed
bool try_set_block_types(Env* env, Node_block** new_node, Lang_type* lang_type, Node_block* block) {
    memset(lang_type, 0, sizeof(*lang_type));

    Node_ptr_vec* block_children = &block->children;

    vec_append(&a_main, &env->ancesters, node_wrap_block(block));

    bool need_add_return = is_directly_in_function_def(env) && block_children->info.count == 0;
    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Node** curr_node = vec_at_ref(block_children, idx);
        Lang_type dummy;
        Node* new_node;
        try_set_node_types(env, &new_node, &dummy, *curr_node);
        *curr_node = new_node;
        assert(*curr_node);

        if (idx == block_children->info.count - 1 
            && (*curr_node)->type != NODE_RETURN
            && is_directly_in_function_def(env)
        ) {
            need_add_return = true;
        }
    }

    if (need_add_return && 
        env->ancesters.info.count > 1 && vec_at(&env->ancesters, env->ancesters.info.count - 2)
    ) {
        Node_return* rtn_statement = node_return_new(block->pos_end);
        if (rtn_statement->pos.line == 0) {
            symbol_log(LOG_DEBUG, env);
            unreachable("");
        }
        rtn_statement->is_auto_inserted = true;
        rtn_statement->child = node_wrap_literal(util_literal_new_from_strv(str_view_from_cstr(""), TOKEN_VOID, block->pos_end));
        Lang_type dummy = {0};
        Node* new_rtn_statement = NULL;
        if (!try_set_node_types(env, &new_rtn_statement, &dummy, node_wrap_return(rtn_statement))) {
            goto error;
        }
        assert(rtn_statement);
        assert(new_rtn_statement);
        vec_append_safe(&a_main, block_children, new_rtn_statement);
    }

error:
    vec_rem_last(&env->ancesters);
    *new_node = block;
    return true;
}

bool try_set_node_types(Env* env, Node** new_node, Lang_type* lang_type, Node* node) {
    *new_node = node;

    switch (node->type) {
        case NODE_EXPR: {
            Node_expr* new_node_ = NULL;
            if (!try_set_expr_types(env, &new_node_, lang_type, node_unwrap_expr(node))) {
                return false;
            }
            *new_node = node_wrap_expr(new_node_);
            return true;
        }
        case NODE_DEF: {
            Node_def* new_node_ = NULL;
            if (!try_set_def_types(env, &new_node_, lang_type, node_unwrap_def(node))) {
                return false;
            }
            *new_node = node_wrap_def(new_node_);
            return true;
        }
        case NODE_IF:
            unreachable("");
        case NODE_FOR_WITH_COND: {
            Node_for_with_cond* new_node_ = NULL;
            if (!try_set_for_with_cond_types(env, &new_node_, lang_type, node_unwrap_for_with_cond(node))) {
                return false;
            }
            *new_node = node_wrap_for_with_cond(new_node_);
            return true;
        }
        case NODE_ASSIGNMENT:
            return try_set_assignment_types(env, lang_type, node_unwrap_assignment(node));
        case NODE_RETURN: {
            Node_return* new_rtn = NULL;
            if (!try_set_return_types(env, &new_rtn, lang_type, node_unwrap_return(node))) {
                return false;
            }
            *new_node = node_wrap_return(new_rtn);
            return true;
        }
        case NODE_FOR_RANGE: {
            Node_for_range* new_for = NULL;
            if (!try_set_for_range_types(env, &new_for, lang_type, node_unwrap_for_range(node))) {
                return false;
            }
            *new_node = node_wrap_for_range(new_for);
            return true;
        }
        case NODE_BREAK:
            *new_node = node;
            return true;
        case NODE_CONTINUE:
            *new_node = node;
            return true;
        case NODE_BLOCK: {
            assert(node_unwrap_block(node)->pos_end.line > 0);
            Node_block* new_for = NULL;
            if (!try_set_block_types(env, &new_for, lang_type, node_unwrap_block(node))) {
                return false;
            }
            *new_node = node_wrap_block(new_for);
            return true;
        }
        case NODE_IF_ELSE_CHAIN: {
            Node_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(env, &new_for, lang_type, node_unwrap_if_else_chain(node))) {
                return false;
            }
            *new_node = node_wrap_if_else_chain(new_for);
            return true;
        }
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}
