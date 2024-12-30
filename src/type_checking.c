#include <type_checking.h>
#include <parser_utils.h>
#include <tast.h>

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

static Tast_expr* auto_deref_to_0(Env* env, Tast_expr* expr) {
    while (tast_get_lang_type_expr(expr).pointer_depth > 0) {
        Lang_type init_lang_type = {0};
        expr = util_unary_new(env, expr, TOKEN_DEREF, init_lang_type);
    }
    return expr;
}

const Tast_function_decl* get_parent_function_decl_const(const Env* env) {
    Tast_def* def = NULL;
    try(env->name_parent_function.count > 0 && "no parent function here");
    try(symbol_lookup(&def, env, env->name_parent_function));
    if (def->type != TAST_FUNCTION_DECL) {
        unreachable(TAST_FMT, tast_def_print(def));
    }
    return tast_unwrap_function_decl(def);
}

Lang_type get_parent_function_return_type(const Env* env) {
    return get_parent_function_decl_const(env)->return_type->lang_type;
}

static bool can_be_implicitly_converted(const Env* env, Lang_type dest, Lang_type src, bool implicit_pointer_depth) {
    assert(dest.str.count > 0);
    assert(src.str.count > 0);
    (void) env;

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
    Tast_expr* src,
    bool inplicit_pointer_depth
) {
    Lang_type src_lang_type = tast_get_lang_type_expr(src);
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

    *tast_get_lang_type_expr_ref(src) = dest_lang_type;
    log(
        LOG_DEBUG, LANG_TYPE_FMT" to "LANG_TYPE_FMT"\n", lang_type_print(src_lang_type),
        lang_type_print(dest_lang_type)
    );
    return IMPLICIT_CONV_CONVERTED;
}

bool check_generic_assignment_finish_symbol_untyped(
    const Env* env,
    Tast_expr** new_src,
    Tast_symbol_untyped* src
) {
    (void) env;
    (void) new_src;
    (void) src;
    todo();
    //switch (src->type) {
    //    case TAST_STRING
}

static void msg_invalid_function_arg_internal(
    const char* file,
    int line,
    const Env* env,
    const Tast_expr* argument,
    const Tast_variable_def* corres_param
) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env->file_text, tast_get_pos_expr(argument), 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(tast_get_lang_type_expr(argument)), 
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

static void msg_invalid_return_type_internal(const char* file, int line, const Env* env, const Tast_return* rtn) {
    const Tast_function_decl* fun_decl = get_parent_function_decl_const(env);
    if (rtn->is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env->file_text, rtn->pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            lang_type_print(fun_decl->return_type->lang_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, rtn->pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(tast_get_lang_type_expr(rtn->child)), 
            lang_type_print(fun_decl->return_type->lang_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, fun_decl->return_type->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(fun_decl->return_type->lang_type)
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
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    Tast_expr* src
) {
    if (lang_type_is_equal(dest_lang_type, tast_get_lang_type_expr(src))) {
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(env, dest_lang_type, tast_get_lang_type_expr(src), false)) {
        if (src->type == TAST_LITERAL) {
            *new_src = src;
            *tast_get_lang_type_literal_ref(tast_unwrap_literal(*new_src)) = dest_lang_type;
            return CHECK_ASSIGN_OK;
        } else if (src->type == TAST_SYMBOL_UNTYPED) {
            unreachable(TAST_FMT"\n", tast_expr_print(src));
        }
        log(LOG_DEBUG, LANG_TYPE_FMT "   "TAST_FMT"\n", lang_type_print(dest_lang_type), tast_print((Tast*)src));
        todo();
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

void try_set_literal_types(Tast_literal* literal, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            tast_unwrap_string(literal)->lang_type = lang_type_new_from_cstr("u8", 1);
            break;
        case TOKEN_INT_LITERAL: {
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            int64_t bit_width = bit_width_needed_signed(tast_unwrap_number(literal)->data);
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            tast_unwrap_number(literal)->lang_type = lang_type_new_from_strv(string_to_strv(lang_type_str), 0);
            break;
        }
        case TOKEN_VOID:
            break;
        case TOKEN_CHAR_LITERAL:
            tast_unwrap_char(literal)->lang_type = lang_type_new_from_cstr("u8", 0);
            break;
        default:
            unreachable("");
    }
}

static void msg_undefined_symbol(Str_view file_text, const Tast* sym_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, tast_get_pos(sym_call),
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_tast_name(sym_call))
    );
}

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Tast_expr** new_tast, Tast_symbol_untyped* sym_untyped) {
    Tast_def* sym_def;
    if (!symbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, tast_wrap_expr(tast_wrap_symbol_untyped(sym_untyped)));
        return false;
    }

    Lang_type lang_type = tast_get_lang_type_def(sym_def);

    Sym_typed_base new_base = {.lang_type = lang_type, .name = sym_untyped->name};
    if (lang_type_is_struct(env, lang_type)) {
        Tast_struct_sym* sym_typed = tast_struct_sym_new(sym_untyped->pos, new_base);
        *new_tast = tast_wrap_symbol_typed(tast_wrap_struct_sym(sym_typed));
        return true;
    } else if (lang_type_is_raw_union(env, lang_type)) {
        Tast_raw_union_sym* sym_typed = tast_raw_union_sym_new(sym_untyped->pos, new_base);
        *new_tast = tast_wrap_symbol_typed(tast_wrap_raw_union_sym(sym_typed));
        return true;
    } else if (lang_type_is_enum(env, lang_type)) {
        Tast_enum_sym* sym_typed = tast_enum_sym_new(sym_untyped->pos, new_base);
        *new_tast = tast_wrap_symbol_typed(tast_wrap_enum_sym(sym_typed));
        return true;
    } else if (lang_type_is_primitive(env, lang_type)) {
        Tast_primitive_sym* sym_typed = tast_primitive_sym_new(sym_untyped->pos, new_base);
        *new_tast = tast_wrap_symbol_typed(tast_wrap_primitive_sym(sym_typed));
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
        case TOKEN_LESS_OR_EQUAL:
            return lhs_val <= rhs_val ? 1 : 0;
        case TOKEN_GREATER_OR_EQUAL:
            return lhs_val >= rhs_val ? 1 : 0;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(token_type));
    }
    unreachable("");
}

static Tast_literal* precalulate_number(
    const Tast_number* lhs,
    const Tast_number* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static Tast_literal* precalulate_char(
    const Tast_char* lhs,
    const Tast_char* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Tast_expr** new_tast, Tast_binary* operator) {
    log_tree(LOG_DEBUG, (Tast*)operator->lhs);
    Tast_expr* new_lhs;
    if (!try_set_expr_types(env, &new_lhs, operator->lhs)) {
        return false;
    }
    assert(new_lhs);
    operator->lhs = new_lhs;

    Tast_expr* new_rhs;
    if (!try_set_expr_types(env, &new_rhs, operator->rhs)) {
        return false;
    }
    operator->rhs = new_rhs;
    assert(operator->lhs);
    assert(operator->rhs);
    log_tree(LOG_DEBUG, (Tast*)operator);

    if (!lang_type_is_equal(tast_get_lang_type_expr(operator->lhs), tast_get_lang_type_expr(operator->rhs))) {
        if (can_be_implicitly_converted(env, tast_get_lang_type_expr(operator->lhs), tast_get_lang_type_expr(operator->rhs), true)) {
            if (operator->rhs->type == TAST_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                *tast_get_lang_type_literal_ref(tast_unwrap_literal(operator->rhs)) = tast_get_lang_type_expr(operator->lhs);
            } else {
                Tast_expr* unary = util_unary_new(env, operator->rhs, TOKEN_UNSAFE_CAST, tast_get_lang_type_expr(operator->lhs));
                operator->rhs = unary;
            }
        } else if (can_be_implicitly_converted(env, tast_get_lang_type_expr(operator->rhs), tast_get_lang_type_expr(operator->lhs), true)) {
            if (operator->lhs->type == TAST_LITERAL) {
                operator->lhs = auto_deref_to_0(env, operator->lhs);
                operator->rhs = auto_deref_to_0(env, operator->rhs);
                *tast_get_lang_type_literal_ref(tast_unwrap_literal(operator->lhs)) = tast_get_lang_type_expr(operator->rhs);
            } else {
                Tast_expr* unary = util_unary_new(env, operator->lhs, TOKEN_UNSAFE_CAST, tast_get_lang_type_expr(operator->rhs));
                operator->lhs = unary;
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, operator->pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(tast_get_lang_type_expr(operator->lhs)), lang_type_print(tast_get_lang_type_expr(operator->rhs))
            );
            return false;
        }
    }
            
    assert(tast_get_lang_type_expr(operator->lhs).str.count > 0);
    operator->lang_type = tast_get_lang_type_expr(operator->lhs);

    // precalcuate binary in some situations
    if (operator->lhs->type == TAST_LITERAL && operator->rhs->type == TAST_LITERAL) {
        Tast_literal* lhs_lit = tast_unwrap_literal(operator->lhs);
        Tast_literal* rhs_lit = tast_unwrap_literal(operator->rhs);

        if (lhs_lit->type != rhs_lit->type) {
            // TODO: make expected failure test for this
            unreachable("mismatched types");
        }

        Tast_literal* literal = NULL;

        switch (lhs_lit->type) {
            case TAST_NUMBER:
                literal = precalulate_number(
                    tast_unwrap_number_const(lhs_lit),
                    tast_unwrap_number_const(rhs_lit),
                    operator->token_type,
                    operator->pos
                );
                break;
            case TAST_CHAR:
                literal = precalulate_char(
                    tast_unwrap_char_const(lhs_lit),
                    tast_unwrap_char_const(rhs_lit),
                    operator->token_type,
                    operator->pos
                );
                break;
            default:
                unreachable("");
        }

        *new_tast = tast_wrap_literal(literal);
        log_tree(LOG_DEBUG, tast_wrap_expr(*new_tast));
    } else {
        *new_tast = tast_wrap_operator(tast_wrap_binary(operator));
    }

    assert(tast_get_lang_type_expr(*new_tast).str.count > 0);
    assert(*new_tast);
    return true;
}

bool try_set_unary_types(Env* env, Tast_expr** new_tast, Tast_unary* unary) {
    Tast_expr* new_child;
    if (!try_set_expr_types(env, &new_child, unary->child)) {
        return false;
    }
    unary->child = new_child;

    switch (unary->token_type) {
        case TOKEN_NOT:
            unary->lang_type = tast_get_lang_type_expr(new_child);
            break;
        case TOKEN_DEREF:
            unary->lang_type = tast_get_lang_type_expr(new_child);
            if (unary->lang_type.pointer_depth <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, unary->pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            unary->lang_type.pointer_depth--;
            break;
        case TOKEN_REFER:
            unary->lang_type = tast_get_lang_type_expr(new_child);
            unary->lang_type.pointer_depth++;
            assert(unary->lang_type.pointer_depth > 0);
            break;
        case TOKEN_UNSAFE_CAST:
            assert(unary->lang_type.str.count > 0);
            if (unary->lang_type.pointer_depth > 0 && lang_type_is_number(tast_get_lang_type_expr(unary->child))) {
            } else if (lang_type_is_number(unary->lang_type) && tast_get_lang_type_expr(unary->child).pointer_depth > 0) {
            } else if (lang_type_is_number(unary->lang_type) && lang_type_is_number(tast_get_lang_type_expr(unary->child))) {
            } else if (unary->lang_type.pointer_depth > 0 && tast_get_lang_type_expr(unary->child).pointer_depth > 0) {
            } else {
                todo();
            }
            break;
        default:
            unreachable("");
    }

    *new_tast = tast_wrap_operator(tast_wrap_unary(unary));
    return true;
}

// returns false if unsuccessful
bool try_set_operator_types(Env* env, Tast_expr** new_tast, Tast_operator* operator) {
    if (operator->type == TAST_UNARY) {
        return try_set_unary_types(env, new_tast, tast_unwrap_unary(operator));
    } else if (operator->type == TAST_BINARY) {
        return try_set_binary_types(env, new_tast, tast_unwrap_binary(operator));
    } else {
        unreachable("");
    }
}

bool try_set_struct_literal_assignment_types(Env* env, Tast** new_tast, const Tast* lhs, Tast_struct_literal* struct_literal, Pos assign_pos) {
    //log(LOG_DEBUG, "------------------------------\n");
    //if (!is_corresponding_to_a_struct(env, lhs)) {
    //    todo(); // non_struct assigned struct literal
    //}
    Tast_def* lhs_var_def_;
    try(symbol_lookup(&lhs_var_def_, env, get_tast_name(lhs)));
    Tast_variable_def* lhs_var_def = tast_unwrap_variable_def(lhs_var_def_);
    Tast_def* struct_def_;
    try(symbol_lookup(&struct_def_, env, lhs_var_def->lang_type.str));
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF:
            break;
        case TAST_RAW_UNION_DEF:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env->file_text,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        default:
            log(LOG_DEBUG, TAST_FMT"\n", tast_print(tast_wrap_def(struct_def_)));
            unreachable("");
    }
    Tast_struct_def* struct_def = tast_unwrap_struct_def(struct_def_);
    Lang_type new_lang_type = {.str = struct_def->base.name, .pointer_depth = 0};
    struct_literal->lang_type = new_lang_type;
    
    Tast_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        //log(LOG_DEBUG, "%zu\n", idx);
        Tast* memb_sym_def_ = vec_at(&struct_def->base.members, idx);
        Tast_variable_def* memb_sym_def = tast_unwrap_variable_def(tast_unwrap_def(memb_sym_def_));
        log_tree(LOG_DEBUG, tast_wrap_expr_const(tast_wrap_struct_literal_const(struct_literal)));
        Tast_assignment* assign_memb_sym = tast_unwrap_assignment(vec_at(&struct_literal->members, idx));
        Tast_symbol_untyped* memb_sym_piece_untyped = tast_unwrap_symbol_untyped(tast_unwrap_expr(assign_memb_sym->lhs));
        Tast_expr* new_rhs = NULL;
        if (!try_set_expr_types(env, &new_rhs, assign_memb_sym->rhs)) {
            unreachable("");
        }
        Tast_literal* assign_memb_sym_rhs = tast_unwrap_literal(new_rhs);

        *tast_get_lang_type_literal_ref(assign_memb_sym_rhs) = memb_sym_def->lang_type;
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

        vec_append_safe(&a_main, &new_literal_members, tast_wrap_expr(tast_wrap_literal(assign_memb_sym_rhs)));
    }

    struct_literal->members = new_literal_members;

    assert(struct_literal->lang_type.str.count > 0);

    *new_tast = tast_wrap_expr(tast_wrap_struct_literal(struct_literal));
    return true;
}

bool try_set_expr_types(Env* env, Tast_expr** new_tast, Tast_expr* tast) {
    switch (tast->type) {
        case TAST_LITERAL:
            *new_tast = tast;
            return true;
        case TAST_SYMBOL_UNTYPED:
            if (!try_set_symbol_type(env, new_tast, tast_unwrap_symbol_untyped(tast))) {
                return false;
            } else {
                assert(*new_tast);
            }
            return true;
        case TAST_MEMBER_ACCESS_UNTYPED: {
            Tast* new_tast_ = NULL;
            if (!try_set_member_access_types(env, &new_tast_, tast_unwrap_member_access_untyped(tast))) {
                return false;
            }
            *new_tast = tast_unwrap_expr(new_tast_);
            return true;
        }
        case TAST_MEMBER_ACCESS_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_types(tast_unwrap_member_sym_typed(tast));
            //*new_tast = tast;
            //return true;
        case TAST_INDEX_UNTYPED: {
            Tast* new_tast_ = NULL;
            if (!try_set_index_untyped_types(env, &new_tast_, tast_unwrap_index_untyped(tast))) {
                return false;
            }
            *new_tast = tast_unwrap_expr(new_tast_);
            return true;
        }
        case TAST_INDEX_TYPED:
            todo();
            //*lang_type = get_member_sym_piece_final_types(tast_unwrap_member_sym_typed(tast));
            //*new_tast = tast;
            //return true;
        case TAST_SYMBOL_TYPED:
            *new_tast = tast;
            return true;
        case TAST_OPERATOR:
            if (!try_set_operator_types(env, new_tast, tast_unwrap_operator(tast))) {
                return false;
            }
            assert(*new_tast);
            return true;
        case TAST_FUNCTION_CALL:
            return try_set_function_call_types(env, new_tast, tast_unwrap_function_call(tast));
        case TAST_STRUCT_LITERAL:
            unreachable("cannot set struct literal type here");
    }
    unreachable("");
}

bool try_set_def_types(Env* env, Tast_def** new_tast, Tast_def* tast) {
    switch (tast->type) {
        case TAST_VARIABLE_DEF: {
            Tast_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, tast_unwrap_variable_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_variable_def(new_def);
            return true;
        }
        case TAST_FUNCTION_DECL:
            // TODO: do this?
            *new_tast = tast;
            return true;
        case TAST_FUNCTION_DEF: {
            Tast_function_def* new_def = NULL;
            if (!try_set_function_def_types(env, &new_def, tast_unwrap_function_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_function_def(new_def);
            return true;
        }
        case TAST_STRUCT_DEF: {
            Tast_struct_def* new_def = NULL;
            if (!try_set_struct_def_types(env, &new_def, tast_unwrap_struct_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_struct_def(new_def);
            return true;
        }
        case TAST_RAW_UNION_DEF: {
            Tast_raw_union_def* new_def = NULL;
            if (!try_set_raw_union_def_types(env, &new_def, tast_unwrap_raw_union_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_raw_union_def(new_def);
            return true;
        }
        case TAST_ENUM_DEF: {
            Tast_enum_def* new_def = NULL;
            if (!try_set_enum_def_types(env, &new_def, tast_unwrap_enum_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_enum_def(new_def);
            return true;
        }
        case TAST_PRIMITIVE_DEF:
            *new_tast = tast;
            return true;
        case TAST_LITERAL_DEF:
            *new_tast = tast;
            return true;
    }
    unreachable("");
}

bool try_set_assignment_types(Env* env, Tast_assignment* assignment) {
    Tast* new_lhs = NULL;
    if (!try_set_tast_types(env, &new_lhs, assignment->lhs)) { 
        return false;
    }
    assignment->lhs = new_lhs;

    Tast_expr* new_rhs;
    if (assignment->rhs->type == TAST_STRUCT_LITERAL) {
        Tast* new_rhs_ = NULL;
        // TODO: do this in check_generic_assignment
        if (!try_set_struct_literal_assignment_types(
            env, &new_rhs_, assignment->lhs, tast_unwrap_struct_literal(assignment->rhs), assignment->pos
        )) {
            return false;
        }
        new_rhs = tast_unwrap_expr(new_rhs_);
    } else {
        if (!try_set_expr_types(env, &new_rhs, assignment->rhs)) {
            return false;
        }
    }
    assignment->rhs = new_rhs;

    if (CHECK_ASSIGN_OK != check_generic_assignment(env, &assignment->rhs, tast_get_lang_type(assignment->lhs), new_rhs)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
            assignment->pos,
            "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
            lang_type_print(tast_get_lang_type_expr(new_rhs)), lang_type_print(tast_get_lang_type(assignment->lhs))
        );
        return false;
    }

    return true;
}

bool try_set_function_call_types(Env* env, Tast_expr** new_tast, Tast_function_call* fun_call) {
    bool status = true;

    Tast_def* fun_def;
    if (!symbol_lookup(&fun_def, env, fun_call->name)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env->file_text, fun_call->pos,
            "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
        );
        status = false;
        goto error;
    }
    Tast_function_decl* fun_decl;
    if (fun_def->type == TAST_FUNCTION_DEF) {
        fun_decl = tast_unwrap_function_def(fun_def)->decl;
    } else {
        fun_decl = tast_unwrap_function_decl(fun_def);
    }
    Tast_lang_type* fun_rtn_type = fun_decl->return_type;
    fun_call->lang_type = fun_rtn_type->lang_type;
    assert(fun_call->lang_type.str.count > 0);
    Tast_function_params* params = fun_decl->params;
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
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, tast_get_pos_def(fun_def),
            "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
        );
        status = false;
        goto error;
    }

    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Tast_variable_def* corres_param = vec_at(&params->params, params_idx);
        Tast_expr** argument = vec_at_ref(&fun_call->args, arg_idx);
        Tast_expr* new_arg = NULL;

        if (!corres_param->is_variadic) {
            params_idx++;
        }

        Tast* new_arg_ = NULL;
        if ((*argument)->type == TAST_STRUCT_LITERAL) {
            todo();
            try(try_set_struct_literal_assignment_types(
                env,
                &new_arg_,
                tast_wrap_def(tast_wrap_variable_def(corres_param)),
                tast_unwrap_struct_literal(*argument),
                fun_call->pos
            ));
            new_arg = tast_unwrap_expr(new_arg_);
        } else {
            if (!try_set_expr_types(env, &new_arg, *argument)) {
                status = false;
                continue;
            }
        }
        log_tree(LOG_DEBUG, tast_wrap_expr(*argument));
        log_tree(LOG_DEBUG, tast_wrap_expr(new_arg));
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

        Tast_expr* new_new_arg = NULL;
        if (CHECK_ASSIGN_OK != check_generic_assignment(env, &new_new_arg, corres_param->lang_type, new_arg)) {
            msg_invalid_function_arg(env, new_arg, corres_param);
            status = false;
            goto error;
        }

        *argument = new_new_arg;
        log_tree(LOG_DEBUG, tast_wrap_expr(*argument));
    }

error:
    *new_tast = tast_wrap_function_call(fun_call);
    return status;
}

static void msg_invalid_member(
    const Env* env,
    Struct_def_base base,
    const Tast_member_access_untyped* access
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER, env->file_text,
        access->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(access->member_name), str_view_print(base.name)
    );

    // TODO: add notes for where struct def of callee is defined, etc.
}

static void msg_invalid_enum_member(
    const Env* env,
    Struct_def_base base,
    const Tast_member_access_untyped* access
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_ENUM_MEMBER, env->file_text,
        access->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(access->member_name), str_view_print(base.name)
    );

    // TODO: add notes for where struct def of callee is defined, etc.
}

bool try_set_member_access_types_finish_generic_struct(
    const Env* env,
    Tast** new_tast,
    Tast_member_access_untyped* access,
    Struct_def_base def_base,
    Tast_expr* new_callee
) {
    Tast_variable_def* member_def = NULL;
    if (!try_get_member_def(&member_def, &def_base, access->member_name)) {
        msg_invalid_member(env, def_base, access);
        return false;
    }

    Tast_member_access_typed* new_access = tast_member_access_typed_new(
        access->pos,
        member_def->lang_type,
        access->member_name,
        new_callee
    );

    *new_tast = tast_wrap_expr(tast_wrap_member_access_typed(new_access));

    assert(new_access->lang_type.str.count > 0);
    return true;
}

bool try_set_member_access_types_finish(
    const Env* env,
    Tast** new_tast,
    Tast_def* lang_type_def,
    Tast_member_access_untyped* access,
    Tast_expr* new_callee
) {
    switch (lang_type_def->type) {
        case TAST_STRUCT_DEF: {
            Tast_struct_def* struct_def = tast_unwrap_struct_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, struct_def->base, new_callee
            );
        }
        case TAST_RAW_UNION_DEF: {
            Tast_raw_union_def* raw_union_def = tast_unwrap_raw_union_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, raw_union_def->base, new_callee
            );
        }
        case TAST_ENUM_DEF: {
            Tast_enum_def* enum_def = tast_unwrap_enum_def(lang_type_def);
            Tast_variable_def* member_def = NULL;
            if (!try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
                msg_invalid_enum_member(env, enum_def->base, access);
                return false;
            }

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                access->pos,
                get_member_index(&enum_def->base, access->member_name),
                member_def->lang_type
            );

            *new_tast = tast_wrap_expr(tast_wrap_literal(tast_wrap_enum_lit(new_lit)));
            assert(member_def->lang_type.str.count > 0);
            return true;
        }
        default:
            unreachable(TAST_FMT"\n", tast_print(tast_wrap_def(lang_type_def)));
    }

    unreachable("");
}

bool try_set_member_access_types(
    Env* env,
    Tast** new_tast,
    Tast_member_access_untyped* access
) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case TAST_SYMBOL_TYPED: {
            Tast_symbol_typed* sym = tast_unwrap_symbol_typed(new_callee);
            Tast_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, tast_get_lang_type_symbol_typed(sym).str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MEMBER_ACCESS_TYPED: {
            Tast_member_access_typed* sym = tast_unwrap_member_access_typed(new_callee);

            Tast_def* lang_type_def = NULL;
            if (!symbol_lookup(&lang_type_def, env, sym->lang_type.str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);
        }
        default:
            unreachable("");
    }
    unreachable("");
}

bool try_set_index_untyped_types(Env* env, Tast** new_tast, Tast_index_untyped* index) {
    Tast_expr* new_callee = NULL;
    Tast_expr* new_inner_index = NULL;
    if (!try_set_expr_types(env, &new_callee, index->callee)) {
        return false;
    }
    if (!try_set_expr_types(env, &new_inner_index, index->index)) {
        return false;
    }

    Lang_type new_lang_type = tast_get_lang_type_expr(new_callee);
    if (new_lang_type.pointer_depth < 1) {
        todo();
    }
    new_lang_type.pointer_depth--;

    Tast_index_typed* new_index = tast_index_typed_new(
        index->pos,
        new_lang_type,
        new_inner_index,
        new_callee
    );

    *new_tast = tast_wrap_expr(tast_wrap_index_typed(new_index));
    return true;
}

static bool try_set_condition_types(Env* env, Tast_condition* if_cond) {
    Tast_expr* new_if_cond_child;
    if (!try_set_operator_types(env, &new_if_cond_child, if_cond->child)) {
        return false;
    }

    switch (new_if_cond_child->type) {
        case TAST_OPERATOR:
            if_cond->child = tast_unwrap_operator(new_if_cond_child);
            break;
        case TAST_LITERAL:
            if_cond->child = condition_get_default_child(new_if_cond_child);
            break;
        case TAST_FUNCTION_CALL:
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
        Tast* curr = vec_at(&base->members, idx);

        Tast* result_dummy = NULL;
        if (!try_set_tast_types(env, &result_dummy, curr)) {
            success = false;
        }
    }

    return success;
}

bool try_set_enum_def_types(Env* env, Tast_enum_def** new_tast, Tast_enum_def* tast) {
    bool success = try_set_struct_base_types(env, &tast->base);
    *new_tast = tast;
    return success;
}

bool try_set_raw_union_def_types(Env* env, Tast_raw_union_def** new_tast, Tast_raw_union_def* tast) {
    bool success = try_set_struct_base_types(env, &tast->base);
    *new_tast = tast;
    return success;
}

bool try_set_struct_def_types(Env* env, Tast_struct_def** new_tast, Tast_struct_def* tast) {
    bool success = try_set_struct_base_types(env, &tast->base);
    *new_tast = tast;
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
    Tast_variable_def** new_tast,
    Tast_variable_def* tast
) {
    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, tast->lang_type.str)) {
        msg_undefined_type(env, tast->pos, tast->lang_type);
        return false;
    }

    *new_tast = tast;
    return true;
}

bool try_set_function_def_types(
    Env* env,
    Tast_function_def** new_tast,
    Tast_function_def* old_def
) {
    Str_view prev_par_fun = env->name_parent_function;
    env->name_parent_function = old_def->decl->name;
    assert(env->name_parent_function.count > 0);
    bool status = true;

    Tast_function_decl* new_decl = NULL;
    if (!try_set_function_decl_types(env, &new_decl, old_def->decl)) {
        status = false;
    }
    old_def->decl = new_decl;

    size_t prev_ancesters_count = env->ancesters.info.count;
    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, old_def->body, true)) {
        status = false;
    }
    old_def->body = new_body;
    assert(prev_ancesters_count == env->ancesters.info.count);

    *new_tast = old_def;
    env->name_parent_function = prev_par_fun;
    return status;
}

bool try_set_function_decl_types(
    Env* env,
    Tast_function_decl** new_tast,
    Tast_function_decl* old_def
) {
    bool status = true;

    Tast_function_params* new_params = NULL;
    if (!try_set_function_params_types(env, &new_params, old_def->params)) {
        status = false;
    }
    old_def->params = new_params;

    Tast_lang_type* new_rtn_type = NULL;
    if (!try_set_lang_type_types(env, &new_rtn_type, old_def->return_type)) {
        status = false;
    }
    old_def->return_type = new_rtn_type;

    *new_tast = old_def;
    return status;
}

bool try_set_function_params_types(
    Env* env,
    Tast_function_params** new_tast,
    Tast_function_params* old_params
) {
    bool status = true;

    for (size_t idx = 0; idx < old_params->params.info.count; idx++) {
        Tast_variable_def* old_def = vec_at(&old_params->params, idx);

        Tast_variable_def* new_def = NULL;
        if (!try_set_variable_def_types(env, &new_def, old_def)) {
            status = false;
        }

        *vec_at_ref(&old_params->params, idx) = new_def;
    }

    *new_tast = old_params;
    return status;
}

bool try_set_lang_type_types(
    Env* env,
    Tast_lang_type** new_tast,
    Tast_lang_type* old_tast
) {
    Tast_def* dummy = NULL;
    if (!symbol_lookup(&dummy, env, old_tast->lang_type.str)) {
        msg_undefined_type(env, old_tast->pos, old_tast->lang_type);
        return false;
    }

    *new_tast = old_tast;
    return true;
}

bool try_set_return_types(Env* env, Tast_return** new_tast, Tast_return* rtn) {
    *new_tast = NULL;

    Tast_expr* new_rtn_child;
    if (!try_set_expr_types(env, &new_rtn_child, rtn->child)) {
        todo();
        return false;
    }
    rtn->child = new_rtn_child;

    Lang_type dest_lang_type = get_parent_function_return_type(env);

    if (CHECK_ASSIGN_OK != check_generic_assignment(env, &new_rtn_child, dest_lang_type, rtn->child)) {
        msg_invalid_return_type(env, rtn);
        return false;
    }
    rtn->child = new_rtn_child;

    *new_tast = rtn;
    return true;
}

bool try_set_for_range_types(Env* env, Tast_for_range** new_tast, Tast_for_range* old_for) {
    bool status = true;

    Tast_variable_def* new_var_def = NULL;
    if (!try_set_variable_def_types(env, &new_var_def, old_for->var_def)) {
        status = false;
    }
    old_for->var_def = new_var_def;

    Tast_expr* new_lower = NULL;
    if (!try_set_expr_types(env, &new_lower, old_for->lower_bound->child)) {
        status = false;
    }
    old_for->lower_bound->child = new_lower;

    Tast_expr* new_upper = NULL;
    if (!try_set_expr_types(env, &new_upper, old_for->upper_bound->child)) {
        status = false;
    }
    old_for->upper_bound->child = new_upper;

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, old_for->body, false)) {
        status = false;
    }
    old_for->body = new_body;

    *new_tast = old_for;
    return status;
}

bool try_set_for_with_cond_types(Env* env, Tast_for_with_cond** new_tast, Tast_for_with_cond* old_for) {
    bool status = true;

    if (!try_set_condition_types(env, old_for->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, old_for->body, false)) {
        status = false;
    }
    old_for->body = new_body;

    *new_tast = old_for;
    return status;
}

bool try_set_if_types(Env* env, Tast_if** new_tast, Tast_if* old_if) {
    bool status = true;

    if (!try_set_condition_types(env, old_if->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, old_if->body, false)) {
        status = false;
    }
    old_if->body = new_body;

    *new_tast = old_if;
    return status;
}

bool try_set_if_else_chain(Env* env, Tast_if_else_chain** new_tast, Tast_if_else_chain* old_if_else) {
    bool status = true;

    for (size_t idx = 0; idx < old_if_else->tasts.info.count; idx++) {
        Tast_if** old_if = vec_at_ref(&old_if_else->tasts, idx);
                
        Tast_if* new_if = NULL;
        if (!try_set_if_types(env, &new_if, *old_if)) {
            status = false;
        }
        *old_if = new_if;
    }

    *new_tast = old_if_else;
    return status;
}

// TODO: consider if lang_type result should be removed
bool try_set_block_types(Env* env, Tast_block** new_tast, Tast_block* block, bool is_directly_in_fun_def) {
    Tast_vec* block_children = &block->children;

    vec_append(&a_main, &env->ancesters, &block->symbol_collection);

    for (size_t idx = 0; idx < block_children->info.count; idx++) {
        Tast** curr_tast = vec_at_ref(block_children, idx);
        Tast* new_tast;
        try_set_tast_types(env, &new_tast, *curr_tast);
        *curr_tast = new_tast;
        assert(*curr_tast);

    }

    if (is_directly_in_fun_def && (
        block_children->info.count < 1 ||
        vec_at(block_children, block_children->info.count - 1)->type != TAST_RETURN
    )) {
        Tast_return* rtn_statement = tast_return_new(
            block->pos_end,
            tast_wrap_literal(util_literal_new_from_strv(
                str_view_from_cstr(""), TOKEN_VOID, block->pos_end
            )),
            true
        );
        if (rtn_statement->pos.line == 0) {
            symbol_log(LOG_DEBUG, env);
            unreachable("");
        }
        Tast* new_rtn_statement = NULL;
        if (!try_set_tast_types(env, &new_rtn_statement, tast_wrap_return(rtn_statement))) {
            goto error;
        }
        assert(rtn_statement);
        assert(new_rtn_statement);
        vec_append_safe(&a_main, block_children, new_rtn_statement);
    }

error:
    vec_rem_last(&env->ancesters);
    *new_tast = block;
    return true;
}

bool try_set_tast_types(Env* env, Tast** new_tast, Tast* tast) {
    *new_tast = tast;

    switch (tast->type) {
        case TAST_EXPR: {
            Tast_expr* new_tast_ = NULL;
            if (!try_set_expr_types(env, &new_tast_, tast_unwrap_expr(tast))) {
                return false;
            }
            *new_tast = tast_wrap_expr(new_tast_);
            return true;
        }
        case TAST_DEF: {
            Tast_def* new_tast_ = NULL;
            if (!try_set_def_types(env, &new_tast_, tast_unwrap_def(tast))) {
                return false;
            }
            *new_tast = tast_wrap_def(new_tast_);
            return true;
        }
        case TAST_IF:
            unreachable("");
        case TAST_FOR_WITH_COND: {
            Tast_for_with_cond* new_tast_ = NULL;
            if (!try_set_for_with_cond_types(env, &new_tast_, tast_unwrap_for_with_cond(tast))) {
                return false;
            }
            *new_tast = tast_wrap_for_with_cond(new_tast_);
            return true;
        }
        case TAST_ASSIGNMENT:
            return try_set_assignment_types(env, tast_unwrap_assignment(tast));
        case TAST_RETURN: {
            Tast_return* new_rtn = NULL;
            if (!try_set_return_types(env, &new_rtn, tast_unwrap_return(tast))) {
                return false;
            }
            *new_tast = tast_wrap_return(new_rtn);
            return true;
        }
        case TAST_FOR_RANGE: {
            Tast_for_range* new_for = NULL;
            if (!try_set_for_range_types(env, &new_for, tast_unwrap_for_range(tast))) {
                return false;
            }
            *new_tast = tast_wrap_for_range(new_for);
            return true;
        }
        case TAST_BREAK:
            *new_tast = tast;
            return true;
        case TAST_CONTINUE:
            *new_tast = tast;
            return true;
        case TAST_BLOCK: {
            assert(tast_unwrap_block(tast)->pos_end.line > 0);
            Tast_block* new_for = NULL;
            if (!try_set_block_types(env, &new_for, tast_unwrap_block(tast), false)) {
                return false;
            }
            *new_tast = tast_wrap_block(new_for);
            return true;
        }
        case TAST_IF_ELSE_CHAIN: {
            Tast_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(env, &new_for, tast_unwrap_if_else_chain(tast))) {
                return false;
            }
            *new_tast = tast_wrap_if_else_chain(new_for);
            return true;
        }
        default:
            unreachable(TAST_FMT, tast_print(tast));
    }
}
