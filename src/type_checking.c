#include <type_checking.h>
#include <parser_utils.h>
#include <uast_utils.h>
#include <uast.h>
#include <tast.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <uast_hand_written.h>

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
        try(try_set_unary_types_finish(env, &expr, expr, tast_get_pos_expr(expr), TOKEN_DEREF, (Lang_type) {0}));
    }
    return expr;
}

const Uast_function_decl* get_parent_function_decl_const(const Env* env) {
    Uast_def* def = NULL;
    try(env->name_parent_function.count > 0 && "no parent function here");
    try(usymbol_lookup(&def, env, env->name_parent_function));
    if (def->type != UAST_FUNCTION_DECL) {
        unreachable(TAST_FMT, uast_def_print(def));
    }
    return uast_unwrap_function_decl(def);
}

const Uast_lang_type* get_parent_function_return_type(const Env* env) {
    return get_parent_function_decl_const(env)->return_type;
}

static bool can_be_implicitly_converted_lang_type(Lang_type dest, Lang_type src, bool implicit_pointer_depth) {
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

static bool can_be_implicitly_converted(Lang_type_vec dest, Lang_type_vec src, bool implicit_pointer_depth) {
    if (dest.info.count != src.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < dest.info.count; idx++) {
        if (!can_be_implicitly_converted_lang_type(
            vec_at(&dest, idx), vec_at(&src, idx), implicit_pointer_depth
        )) {
            return false;
        }
    }

    return true;
}

typedef enum {
    IMPLICIT_CONV_INVALID_TYPES,
    IMPLICIT_CONV_CONVERTED,
    IMPLICIT_CONV_OK,
} IMPLICIT_CONV_STATUS;

static void msg_invalid_function_arg_internal(
    const char* file,
    int line,
    const Env* env,
    const Tast_expr* argument,
    const Uast_variable_def* corres_param
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

static void msg_invalid_return_type_internal(const char* file, int line, const Env* env, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    const Uast_function_decl* fun_decl = get_parent_function_decl_const(env);
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env->file_text, pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            lang_type_vec_print(fun_decl->return_type->lang_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(tast_get_lang_type_expr(child)), 
            lang_type_vec_print(fun_decl->return_type->lang_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, fun_decl->return_type->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_vec_print(fun_decl->return_type->lang_type)
    );
}

#define msg_invalid_return_type(env, pos, child, is_auto_inserted) \
    msg_invalid_return_type_internal(__FILE__, __LINE__, env, pos, child, is_auto_inserted)

typedef enum {
    CHECK_ASSIGN_OK,
    CHECK_ASSIGN_INVALID, // error was not printed to the user (caller should print error),
                          // and new_src is valid for printing purposes
    CHECK_ASSIGN_ERROR, // error was printed, and new_src is not valid for printing purposes
} CHECK_ASSIGN_STATUS;

CHECK_ASSIGN_STATUS check_generic_assignment_finish(
    Tast_expr** new_src,
    Lang_type_vec dest_lang_type,
    Tast_expr* src
) {
    if (lang_type_vec_is_equal(dest_lang_type, tast_get_lang_types_expr(src))) {
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(dest_lang_type, tast_get_lang_types_expr(src), false)) {
        if (src->type == TAST_LITERAL) {
            *new_src = src;
            tast_set_lang_types_expr(*new_src, dest_lang_type);
            return CHECK_ASSIGN_OK;
        }
        log(LOG_DEBUG, LANG_TYPE_FMT "   "TAST_FMT"\n", lang_type_vec_print(dest_lang_type), tast_expr_print(src));
        todo();
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

CHECK_ASSIGN_STATUS check_generic_assignment(
    Env* env,
    Tast_expr** new_src,
    Lang_type_vec dest_lang_type,
    Uast_expr* src,
    Pos pos
) {
    if (src->type == UAST_STRUCT_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        if (dest_lang_type.info.count != 1) {
            todo();
        }
        // TODO: tests for using struct literal as function argument (and later as an operand)
        if (!try_set_struct_literal_assignment_types(
            env, &new_src_, vec_at(&dest_lang_type, 0), uast_unwrap_struct_literal(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_unwrap_expr(new_src_);
    } else if (src->type == UAST_TUPLE) {
        Tast_tuple* new_src_ = NULL;
        // TODO: tests for using struct literal as function argument (and later as an operand)
        if (!try_set_tuple_assignment_types(
            env, &new_src_, dest_lang_type, uast_unwrap_tuple(src)
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_wrap_tuple(new_src_);
    } else {
        if (!try_set_expr_types(env, new_src, src)) {
            return CHECK_ASSIGN_ERROR;
        }
    }

    return check_generic_assignment_finish(new_src, dest_lang_type, *new_src);
}

Tast_literal* try_set_literal_types(Uast_literal* literal) {
    switch (literal->type) {
        case UAST_STRING: {
            Uast_string* old_string = uast_unwrap_string(literal);
            return tast_wrap_string(tast_string_new(
                old_string->pos,
                old_string->data,
                lang_type_new_from_cstr("u8", 1),
                old_string->name
            ));
        }
        case UAST_NUMBER: {
            Uast_number* old_number = uast_unwrap_number(literal);
            int64_t bit_width = bit_width_needed_signed(old_number->data);
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            return tast_wrap_number(tast_number_new(
                old_number->pos,
                old_number->data,
                lang_type_new_from_strv(string_to_strv(lang_type_str), 0)
            ));
        }
        case UAST_VOID: {
            Uast_void* old_void = uast_unwrap_void(literal);
            return tast_wrap_void(tast_void_new(
                old_void->pos
            ));
        }
        case UAST_CHAR: {
            Uast_char* old_char = uast_unwrap_char(literal);
            return tast_wrap_char(tast_char_new(
                old_char->pos,
                old_char->data,
                lang_type_new_from_cstr("u8", 0)
            ));
        }
        default:
            unreachable("");
    }
}

static void msg_undefined_symbol_internal(const char* file, int line, Str_view file_text, const Uast_stmt* sym_call) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, uast_get_pos_stmt(sym_call),
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_uast_name_stmt(sym_call))
    );
}

#define msg_undefined_symbol(file_text, sym_call) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, file_text, sym_call)

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Tast_expr** new_tast, Uast_symbol_untyped* sym_untyped) {
    Uast_def* sym_def;
    if (!usymbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, uast_wrap_expr(uast_wrap_symbol_untyped(sym_untyped)));
        return false;
    }

    Lang_type lang_type = uast_get_lang_type_def(sym_def);

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
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static Tast_literal* precalulate_char(
    const Tast_char* lhs,
    const Tast_char* rhs,
    TOKEN_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

bool try_set_binary_types_finish(Env* env, Tast_expr** new_tast, Tast_expr* new_lhs, Tast_expr* new_rhs, Pos oper_pos, TOKEN_TYPE oper_token_type) {
    if (!lang_type_is_equal(tast_get_lang_type_expr(new_lhs), tast_get_lang_type_expr(new_rhs))) {
        if (can_be_implicitly_converted(tast_get_lang_types_expr(new_lhs), tast_get_lang_types_expr(new_rhs), true)) {
            if (new_rhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                *tast_get_lang_type_literal_ref(tast_unwrap_literal(new_rhs)) = tast_get_lang_type_expr(new_lhs);
            } else {
                try(try_set_unary_types_finish(env, &new_rhs, new_rhs, tast_get_pos_expr(new_rhs), TOKEN_UNSAFE_CAST, tast_get_lang_type_expr(new_lhs)));
            }
        } else if (can_be_implicitly_converted(tast_get_lang_types_expr(new_rhs), tast_get_lang_types_expr(new_lhs), true)) {
            if (new_lhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                *tast_get_lang_type_literal_ref(tast_unwrap_literal(new_lhs)) = tast_get_lang_type_expr(new_rhs);
            } else {
                try(try_set_unary_types_finish(env, &new_lhs, new_lhs, tast_get_pos_expr(new_lhs), TOKEN_UNSAFE_CAST, tast_get_lang_type_expr(new_rhs)));
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, oper_pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(tast_get_lang_type_expr(new_lhs)), lang_type_print(tast_get_lang_type_expr(new_rhs))
            );
            return false;
        }
    }
            
    assert(tast_get_lang_type_expr(new_lhs).str.count > 0);

    // precalcuate binary in some situations
    if (new_lhs->type == TAST_LITERAL && new_rhs->type == TAST_LITERAL) {
        Tast_literal* lhs_lit = tast_unwrap_literal(new_lhs);
        Tast_literal* rhs_lit = tast_unwrap_literal(new_rhs);

        if (lhs_lit->type != rhs_lit->type) {
            unreachable("this error should have been caught earlier\n");
        }

        Tast_literal* literal = NULL;

        switch (lhs_lit->type) {
            case TAST_NUMBER:
                literal = precalulate_number(
                    tast_unwrap_number_const(lhs_lit),
                    tast_unwrap_number_const(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            case TAST_CHAR:
                literal = precalulate_char(
                    tast_unwrap_char_const(lhs_lit),
                    tast_unwrap_char_const(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            default:
                unreachable("");
        }

        *new_tast = tast_wrap_literal(literal);
    } else {
        *new_tast = tast_wrap_operator(tast_wrap_binary(tast_binary_new(
            oper_pos,
            new_lhs,
            new_rhs,
            oper_token_type,
            tast_get_lang_type_expr(new_lhs)
        )));
    }

    assert(tast_get_lang_type_expr(*new_tast).str.count > 0);
    assert(*new_tast);
    return true;
}

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Tast_expr** new_tast, Uast_binary* operator) {
    Tast_expr* new_lhs;
    if (!try_set_expr_types(env, &new_lhs, operator->lhs)) {
        return false;
    }
    assert(new_lhs);

    Tast_expr* new_rhs;
    if (!try_set_expr_types(env, &new_rhs, operator->rhs)) {
        return false;
    }
    assert(new_lhs);
    assert(new_rhs);

    return try_set_binary_types_finish(env, new_tast, new_lhs, new_rhs, operator->pos, operator->token_type);
}

bool try_set_unary_types_finish(
    Env* env,
    Tast_expr** new_tast,
    Tast_expr* new_child,
    Pos unary_pos,
    TOKEN_TYPE unary_token_type,
    Lang_type cast_to
) {
    Lang_type new_lang_type = {0};

    switch (unary_token_type) {
        case TOKEN_NOT:
            new_lang_type = tast_get_lang_type_expr(new_child);
            break;
        case TOKEN_DEREF:
            new_lang_type = tast_get_lang_type_expr(new_child);
            if (new_lang_type.pointer_depth <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, unary_pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            new_lang_type.pointer_depth--;
            break;
        case TOKEN_REFER:
            new_lang_type = tast_get_lang_type_expr(new_child);
            new_lang_type.pointer_depth++;
            assert(new_lang_type.pointer_depth > 0);
            break;
        case TOKEN_UNSAFE_CAST:
            new_lang_type = cast_to;
            assert(cast_to.str.count > 0);
            if (tast_get_lang_type_expr(new_child).pointer_depth > 0 && lang_type_is_number(tast_get_lang_type_expr(new_child))) {
            } else if (lang_type_is_number(tast_get_lang_type_expr(new_child)) && tast_get_lang_type_expr(new_child).pointer_depth > 0) {
            } else if (lang_type_is_number(tast_get_lang_type_expr(new_child)) && lang_type_is_number(tast_get_lang_type_expr(new_child))) {
            } else if (tast_get_lang_type_expr(new_child).pointer_depth > 0 && tast_get_lang_type_expr(new_child).pointer_depth > 0) {
            } else {
                todo();
            }
            break;
        default:
            unreachable("");
    }

    *new_tast = tast_wrap_operator(tast_wrap_unary(tast_unary_new(
        unary_pos,
        new_child,
        unary_token_type,
        new_lang_type
    )));
    return true;
}

bool try_set_unary_types(Env* env, Tast_expr** new_tast, Uast_unary* unary) {
    Tast_expr* new_child;
    if (!try_set_expr_types(env, &new_child, unary->child)) {
        return false;
    }

    return try_set_unary_types_finish(env, new_tast, new_child, uast_get_pos_unary(unary), unary->token_type, unary->lang_type);
}

// returns false if unsuccessful
bool try_set_operator_types(Env* env, Tast_expr** new_tast, Uast_operator* operator) {
    if (operator->type == UAST_UNARY) {
        return try_set_unary_types(env, new_tast, uast_unwrap_unary(operator));
    } else if (operator->type == UAST_BINARY) {
        return try_set_binary_types(env, new_tast, uast_unwrap_binary(operator));
    } else {
        unreachable("");
    }
}

bool try_set_tuple_assignment_types(
    Env* env,
    Tast_tuple** new_tast,
    Lang_type_vec dest_lang_type,
    Uast_tuple* tuple
) {
    if (dest_lang_type.info.count != tuple->members.info.count) {
        // TODO: clean up these error messages
        msg(
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_TUPLE_COUNT, env->file_text,
            uast_get_pos_tuple(tuple),
            "tuple `"UAST_FMT"` cannot be assigned to `"LANG_TYPE_FMT"`\n",
            uast_tuple_print(tuple), lang_type_vec_print(dest_lang_type)
        );
        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text,
            uast_get_pos_tuple(tuple),
            "tuple `"UAST_FMT"` has %zu elements, but type `"LANG_TYPE_FMT" has %zu elements`\n",
            uast_tuple_print(tuple), tuple->members.info.count,
            lang_type_vec_print(dest_lang_type), dest_lang_type.info.count
        );
        return false;
    }

    Tast_expr_vec new_members = {0};
    Lang_type_vec new_lang_type = {0};

    for (size_t idx = 0; idx < dest_lang_type.info.count; idx++) {
        Uast_expr* curr_src = vec_at(&tuple->members, idx);
        Lang_type curr_dest = vec_at(&dest_lang_type, idx);

        Tast_expr* new_memb = NULL;
        switch (check_generic_assignment(
            env,
            &new_memb,
            lang_type_vec_from_lang_type(curr_dest),
            curr_src,
            uast_get_pos_expr(curr_src)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                    tast_get_pos_expr(new_memb),
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(tast_get_lang_type_expr(new_memb)), lang_type_print(curr_dest)
                );
                todo();
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                todo();
        }

        vec_append(&a_main, &new_members, new_memb);
        vec_append(&a_main, &new_lang_type, tast_get_lang_type_expr(new_memb));
    }

    *new_tast = tast_tuple_new(tuple->pos, new_members, new_lang_type);
    return true;
}

bool try_set_struct_literal_assignment_types(
    Env* env,
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_struct_literal* struct_literal,
    Pos assign_pos
) {
    Uast_def* struct_def_;
    try(usymbol_lookup(&struct_def_, env, dest_lang_type.str));
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
            log(LOG_DEBUG, TAST_FMT"\n", uast_stmt_print(uast_wrap_def(struct_def_)));
            unreachable("");
    }
    Uast_struct_def* struct_def = uast_unwrap_struct_def(struct_def_);
    
    Tast_expr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        //log(LOG_DEBUG, "%zu\n", idx);
        Uast_stmt* memb_sym_def_ = vec_at(&struct_def->base.members, idx);
        Uast_variable_def* memb_sym_def = uast_unwrap_variable_def(uast_unwrap_def(memb_sym_def_));
        log(LOG_DEBUG, STR_VIEW_FMT, uast_stmt_print(uast_wrap_expr_const(uast_wrap_struct_literal_const(struct_literal))));
        Uast_assignment* assign_memb_sym = uast_unwrap_assignment(vec_at(&struct_literal->members, idx));
        Uast_symbol_untyped* memb_sym_piece_untyped = uast_unwrap_symbol_untyped(uast_unwrap_expr(assign_memb_sym->lhs));
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
            // TODO: consider how to handle this
            //msg(
            //    LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, lhs_var_def->pos,
            //    "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
            //    str_view_print(lhs_var_def->name), lang_type_print(lhs_var_def->lang_type)
            //);
            //msg(
            //    LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, memb_sym_def->pos,
            //    "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
            //    str_view_print(memb_sym_def->name), lang_type_print(lhs_var_def->lang_type)
            //);
            return false;
        }

        vec_append(&a_main, &new_literal_members, tast_wrap_literal(assign_memb_sym_rhs));
    }

    Tast_struct_literal* new_lit = tast_struct_literal_new(
        struct_literal->pos,
        new_literal_members,
        struct_literal->name,
        dest_lang_type
    );
    *new_tast = tast_wrap_expr(tast_wrap_struct_literal(new_lit));

    Tast_struct_lit_def* new_def = tast_struct_lit_def_new(
        new_lit->pos,
        new_lit->members,
        new_lit->name,
        new_lit->lang_type
    );

    try(symbol_add(env, tast_wrap_literal_def(tast_wrap_struct_lit_def(new_def))));
    return true;
}

bool try_set_expr_types(Env* env, Tast_expr** new_tast, Uast_expr* uast) {
    switch (uast->type) {
        case UAST_LITERAL: {
            *new_tast = tast_wrap_literal(try_set_literal_types(uast_unwrap_literal(uast)));
            return true;
        }
        case UAST_SYMBOL_UNTYPED:
            if (!try_set_symbol_type(env, new_tast, uast_unwrap_symbol_untyped(uast))) {
                return false;
            } else {
                assert(*new_tast);
            }
            return true;
        case UAST_MEMBER_ACCESS_UNTYPED: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_member_access_types(env, &new_tast_, uast_unwrap_member_access_untyped(uast))) {
                return false;
            }
            *new_tast = tast_unwrap_expr(new_tast_);
            return true;
        }
        case UAST_INDEX_UNTYPED: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_index_untyped_types(env, &new_tast_, uast_unwrap_index_untyped(uast))) {
                return false;
            }
            *new_tast = tast_unwrap_expr(new_tast_);
            return true;
        }
        case UAST_OPERATOR:
            if (!try_set_operator_types(env, new_tast, uast_unwrap_operator(uast))) {
                return false;
            }
            assert(*new_tast);
            return true;
        case UAST_FUNCTION_CALL: {
            Tast_function_call* new_call = NULL;
            if (!try_set_function_call_types(env, &new_call, uast_unwrap_function_call(uast))) {
                return false;
            }
            *new_tast = tast_wrap_function_call(new_call);
            return true;
        }
        case UAST_TUPLE: {
            Tast_tuple* new_call = NULL;
            if (!try_set_tuple_types(env, &new_call, uast_unwrap_tuple(uast))) {
                return false;
            }
            *new_tast = tast_wrap_tuple(new_call);
            return true;
        }
        case UAST_STRUCT_LITERAL:
            unreachable("");
    }
    unreachable("");
}

bool try_set_def_types(Env* env, Tast_def** new_tast, Uast_def* uast) {
    switch (uast->type) {
        case UAST_VARIABLE_DEF: {
            Tast_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, uast_unwrap_variable_def(uast), true)) {
                return false;
            }
            *new_tast = tast_wrap_variable_def(new_def);
            return true;
        }
        case UAST_FUNCTION_DECL: {
            Tast_function_decl* new_decl = NULL;
            if (!try_set_function_decl_types(env, &new_decl, uast_unwrap_function_decl(uast), false)) {
                return false;
            }
            *new_tast = tast_wrap_function_decl(new_decl);
            return true;
        }
        case UAST_FUNCTION_DEF: {
            Tast_function_def* new_def = NULL;
            if (!try_set_function_def_types(env, &new_def, uast_unwrap_function_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_function_def(new_def);
            return true;
        }
        case UAST_STRUCT_DEF: {
            Tast_struct_def* new_def = NULL;
            if (!try_set_struct_def_types(env, &new_def, uast_unwrap_struct_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_struct_def(new_def);
            return true;
        }
        case UAST_RAW_UNION_DEF: {
            Tast_raw_union_def* new_def = NULL;
            if (!try_set_raw_union_def_types(env, &new_def, uast_unwrap_raw_union_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_raw_union_def(new_def);
            return true;
        }
        case UAST_ENUM_DEF: {
            Tast_enum_def* new_def = NULL;
            if (!try_set_enum_def_types(env, &new_def, uast_unwrap_enum_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_enum_def(new_def);
            return true;
        }
        case UAST_PRIMITIVE_DEF: {
            Tast_primitive_def* new_def = NULL;
            if (!try_set_primitive_def_types(env, &new_def, uast_unwrap_primitive_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_primitive_def(new_def);
            return true;
        }
        case UAST_LITERAL_DEF: {
            Tast_literal_def* new_def = NULL;
            if (!try_set_literal_def_types(env, &new_def, uast_unwrap_literal_def(uast))) {
                return false;
            }
            *new_tast = tast_wrap_literal_def(new_def);
            return true;
        }
    }
    unreachable("");
}

bool try_set_assignment_types(Env* env, Tast_assignment** new_assign, Uast_assignment* assignment) {
    Tast_stmt* new_lhs = NULL;
    if (!try_set_stmt_types(env, &new_lhs, assignment->lhs)) { 
        return false;
    }

    Tast_expr* new_rhs = NULL;
    switch (check_generic_assignment(
        env, &new_rhs, tast_get_lang_types_stmt(new_lhs), assignment->rhs, assignment->pos
    )) {
        case CHECK_ASSIGN_OK:
            break;
        case CHECK_ASSIGN_INVALID:
            msg(
                LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                assignment->pos,
                "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                lang_type_vec_print(tast_get_lang_types_expr(new_rhs)), lang_type_vec_print(tast_get_lang_types_stmt(new_lhs))
            );
            return false;
        case CHECK_ASSIGN_ERROR:
            return false;
        default:
            unreachable("");
    }

    *new_assign = tast_assignment_new(assignment->pos, new_lhs, new_rhs);
    return true;
}

bool try_set_function_call_types(Env* env, Tast_function_call** new_call, Uast_function_call* fun_call) {
    bool status = true;

    Uast_def* fun_def;
    if (!usymbol_lookup(&fun_def, env, fun_call->name)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env->file_text, fun_call->pos,
            "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
        );
        status = false;
        goto error;
    }
    Uast_function_decl* fun_decl;
    if (fun_def->type == UAST_FUNCTION_DEF) {
        fun_decl = uast_unwrap_function_def(fun_def)->decl;
    } else {
        fun_decl = uast_unwrap_function_decl(fun_def);
    }
    Tast_lang_type* fun_rtn_type = NULL;
    if (!try_set_lang_type_types(env, &fun_rtn_type, fun_decl->return_type)) {
        return false;
    }

    Uast_function_params* params = fun_decl->params;
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
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, uast_get_pos_def(fun_def),
            "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
        );
        status = false;
        goto error;
    }

    Tast_expr_vec new_args = {0};
    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Uast_variable_def* corres_param = vec_at(&params->params, params_idx);
        Uast_expr* arg = vec_at(&fun_call->args, arg_idx);
        Tast_expr* new_arg = NULL;

        if (!corres_param->is_variadic) {
            params_idx++;
        }

        if (lang_type_is_equal(corres_param->lang_type, lang_type_new_from_cstr("any", 0))) {
            if (corres_param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                for (size_t idx = arg_idx; idx < fun_call->args.info.count; idx++) {
                    Tast_expr* new_sub_arg = NULL;
                    if (!try_set_expr_types(env, &new_sub_arg, vec_at(&fun_call->args, idx))) {
                        status = false;
                        continue;
                    }
                    vec_append(&a_main, &new_args, new_sub_arg);
                }
                break;
            } else {
                todo();
            }
        } else {
            switch (check_generic_assignment(
                env,
                &new_arg,
                lang_type_vec_from_lang_type(corres_param->lang_type),
                arg,
                uast_get_pos_expr(arg)
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg_invalid_function_arg(env, new_arg, corres_param);
                    status = false;
                    goto error;
                case CHECK_ASSIGN_ERROR:
                    status = false;
                    goto error;
                default:
                    unreachable("");
            }
        }

        vec_append(&a_main, &new_args, new_arg);
    }

    assert(!status || new_args.info.count == fun_call->args.info.count);

    *new_call = tast_function_call_new(
        fun_call->pos,
        new_args,
        fun_call->name,
        fun_rtn_type->lang_type
    );

error:
    return status;
}

bool try_set_tuple_types(Env* env, Tast_tuple** new_tuple, Uast_tuple* tuple) {
    Tast_expr_vec new_members = {0};
    Lang_type_vec new_lang_type = {0};

    for (size_t idx = 0; idx < tuple->members.info.count; idx++) {
        Tast_expr* new_memb = NULL;
        if (!try_set_expr_types(env, &new_memb, vec_at(&tuple->members, idx))) {
            return false;
        }
        vec_append(&a_main, &new_members, new_memb);
        vec_append(&a_main, &new_lang_type, tast_get_lang_type_expr(new_memb));
    }

    *new_tuple = tast_tuple_new(tuple->pos, new_members, new_lang_type);
    return true;
}

static void msg_invalid_member(
    const Env* env,
    Ustruct_def_base base,
    const Uast_member_access_untyped* access
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
    Ustruct_def_base base,
    const Uast_member_access_untyped* access
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
    Tast_stmt** new_tast,
    Uast_member_access_untyped* access,
    Ustruct_def_base def_base,
    Tast_expr* new_callee
) {
    Uast_variable_def* member_def = NULL;
    if (!uast_try_get_member_def(&member_def, &def_base, access->member_name)) {
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
    Tast_stmt** new_tast,
    Uast_def* lang_type_def,
    Uast_member_access_untyped* access,
    Tast_expr* new_callee
) {
    switch (lang_type_def->type) {
        case UAST_STRUCT_DEF: {
            Uast_struct_def* struct_def = uast_unwrap_struct_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, struct_def->base, new_callee
            );
        }
        case UAST_RAW_UNION_DEF: {
            Uast_raw_union_def* raw_union_def = uast_unwrap_raw_union_def(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, raw_union_def->base, new_callee
            );
        }
        case UAST_ENUM_DEF: {
            Uast_enum_def* enum_def = uast_unwrap_enum_def(lang_type_def);
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
                msg_invalid_enum_member(env, enum_def->base, access);
                return false;
            }

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&enum_def->base, access->member_name),
                member_def->lang_type
            );

            *new_tast = tast_wrap_expr(tast_wrap_literal(tast_wrap_enum_lit(new_lit)));
            assert(member_def->lang_type.str.count > 0);
            return true;
        }
        default:
            unreachable(UAST_FMT"\n", uast_stmt_print(uast_wrap_def(lang_type_def)));
    }

    unreachable("");
}

bool try_set_member_access_types(
    Env* env,
    Tast_stmt** new_tast,
    Uast_member_access_untyped* access
) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case TAST_SYMBOL_TYPED: {
            Tast_symbol_typed* sym = tast_unwrap_symbol_typed(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, tast_get_lang_type_symbol_typed(sym).str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MEMBER_ACCESS_TYPED: {
            Tast_member_access_typed* sym = tast_unwrap_member_access_typed(new_callee);

            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, sym->lang_type.str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);
        }
        case TAST_OPERATOR: {
            Tast_operator* sym = tast_unwrap_operator(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, tast_get_lang_type_operator(sym).str)) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);

        }
        default:
            unreachable(TAST_FMT, tast_expr_print(new_callee));
    }
    unreachable("");
}

bool try_set_index_untyped_types(Env* env, Tast_stmt** new_tast, Uast_index_untyped* index) {
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

static bool try_set_condition_types(Env* env, Tast_condition** new_cond, Uast_condition* cond) {
    Tast_expr* new_child_ = NULL;
    if (!try_set_operator_types(env, &new_child_, cond->child)) {
        return false;
    }

    Tast_operator* new_child = NULL;
    switch (new_child_->type) {
        case TAST_OPERATOR:
            new_child = tast_unwrap_operator(new_child_);
            break;
        case TAST_LITERAL:
            new_child = tast_condition_get_default_child(new_child_);
            break;
        case TAST_FUNCTION_CALL:
            new_child = tast_condition_get_default_child(new_child_);
            break;
        default:
            unreachable("");
    }

    *new_cond = tast_condition_new(
        cond->pos,
        new_child
    );
    return true;
}

bool try_set_struct_base_types(Env* env, Struct_def_base* new_base, Ustruct_def_base* base) {
    env->type_checking_is_in_struct_base_def = true;
    bool success = true;
    Tast_variable_def_vec new_members = {0};

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_stmt* curr = vec_at(&base->members, idx);

        Tast_stmt* new_memb_ = NULL;
        if (try_set_stmt_types(env, &new_memb_, curr)) {
            Tast_variable_def* new_memb = tast_unwrap_variable_def(tast_unwrap_def(new_memb_));
            vec_append(&a_main, &new_members, new_memb);
        } else {
            success = false;
        }
    }

    *new_base = (Struct_def_base) {
        .members = new_members,
        .llvm_id = 0,
        .name = base->name
    };

    env->type_checking_is_in_struct_base_def = false;
    return success;
}

bool try_set_enum_def_types(Env* env, Tast_enum_def** new_tast, Uast_enum_def* tast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &tast->base);
    *new_tast = tast_enum_def_new(tast->pos, new_base);
    try(symbol_add(env, tast_wrap_enum_def(*new_tast)));
    return success;
}

bool try_set_primitive_def_types(Env* env, Tast_primitive_def** new_tast, Uast_primitive_def* tast) {
    (void) env;
    *new_tast = tast_primitive_def_new(tast->pos, tast->lang_type);
    try(symbol_add(env, tast_wrap_primitive_def(*new_tast)));
    return true;
}

bool try_set_literal_def_types(Env* env, Tast_literal_def** new_tast, Uast_literal_def* tast) {
    (void) env;
    (void) new_tast;
    (void) tast;
    unreachable("");
}

bool try_set_raw_union_def_types(Env* env, Tast_raw_union_def** new_tast, Uast_raw_union_def* uast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &uast->base);
    *new_tast = tast_raw_union_def_new(uast->pos, new_base);
    try(symbol_add(env, tast_wrap_raw_union_def(*new_tast)));
    return success;
}

bool try_set_struct_def_types(Env* env, Tast_struct_def** new_tast, Uast_struct_def* uast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &uast->base);
    *new_tast = tast_struct_def_new(uast->pos, new_base);
    try(symbol_add(env, tast_wrap_struct_def(*new_tast)));
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
    Env* env,
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl
) {
    Uast_def* dummy = NULL;
    if (!usymbol_lookup(&dummy, env, uast->lang_type.str)) {
        msg_undefined_type(env, uast->pos, uast->lang_type);
        return false;
    }

    *new_tast = tast_variable_def_new(uast->pos, uast->lang_type, uast->is_variadic, uast->name);
    log(LOG_DEBUG, "adding:"STR_VIEW_FMT, tast_variable_def_print(*new_tast));
    symbol_log(LOG_DEBUG, env);
    if (add_to_sym_tbl && !env->type_checking_is_in_struct_base_def) {
        try(symbol_add(env, tast_wrap_variable_def(*new_tast)));
    }
    return true;
}

bool try_set_function_decl_types(
    Env* env,
    Tast_function_decl** new_tast,
    Uast_function_decl* decl,
    bool add_to_sym_tbl
) {
    Tast_function_params* new_params = NULL;
    if (!try_set_function_params_types(env, &new_params, decl->params, add_to_sym_tbl)) {
        return false;
    }

    Tast_lang_type* new_rtn_type = NULL;
    if (!try_set_lang_type_types(env, &new_rtn_type, decl->return_type)) {
        return false;
    }

    *new_tast = tast_function_decl_new(decl->pos, new_params, new_rtn_type, decl->name);
    try(symbol_add(env, tast_wrap_function_decl(*new_tast)));
    return true;
}

bool try_set_function_def_types(
    Env* env,
    Tast_function_def** new_tast,
    Uast_function_def* def
) {
    Str_view prev_par_fun = env->name_parent_function;
    env->name_parent_function = def->decl->name;
    assert(env->name_parent_function.count > 0);
    bool status = true;

    Tast_function_decl* new_decl = NULL;
    vec_append(&a_main, &env->ancesters, &def->body->symbol_collection);
    if (!try_set_function_decl_types(env, &new_decl, def->decl, true)) {
        status = false;
    }
    vec_rem_last(&env->ancesters);

    size_t prev_ancesters_count = env->ancesters.info.count;
    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, def->body, true)) {
        status = false;
    }
    assert(prev_ancesters_count == env->ancesters.info.count);

    *new_tast = tast_function_def_new(def->pos, new_decl, new_body);
    env->name_parent_function = prev_par_fun;
    return status;
}

bool try_set_function_params_types(
    Env* env,
    Tast_function_params** new_tast,
    Uast_function_params* params,
    bool add_to_sym_tbl
) {
    bool status = true;

    Tast_variable_def_vec new_params = {0};
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Uast_variable_def* def = vec_at(&params->params, idx);

        Tast_variable_def* new_def = NULL;
        if (!try_set_variable_def_types(env, &new_def, def, add_to_sym_tbl)) {
            status = false;
        }

        vec_append(&a_main, &new_params, new_def);
    }

    *new_tast = tast_function_params_new(params->pos, new_params);
    return status;
}

bool try_set_lang_type_types(
    Env* env,
    Tast_lang_type** new_tast,
    Uast_lang_type* uast
) {
    for (size_t idx = 0; idx < uast->lang_type.info.count; idx++) {
        Uast_def* dummy = NULL;
        if (!usymbol_lookup(&dummy, env, vec_at(&uast->lang_type, idx).str)) {
            msg_undefined_type(env, uast->pos, vec_at(&uast->lang_type, idx));
            return false;
        }
    }

    *new_tast = tast_lang_type_new(uast->pos, uast->lang_type);
    return true;
}

bool try_set_return_types(Env* env, Tast_return** new_tast, Uast_return* rtn) {
    *new_tast = NULL;

    const Lang_type_vec fun_rtn_type = get_parent_function_return_type(env)->lang_type;

    Tast_expr* new_child = NULL;
    switch (check_generic_assignment(env, &new_child, fun_rtn_type, rtn->child, rtn->pos)) {
        case CHECK_ASSIGN_OK:
            break;
        case CHECK_ASSIGN_INVALID:
            msg_invalid_return_type(env, rtn->pos, new_child, rtn->is_auto_inserted);
            return false;
        case CHECK_ASSIGN_ERROR:
            return false;
        default:
            unreachable("");
    }

    *new_tast = tast_return_new(rtn->pos, new_child, rtn->is_auto_inserted);
    return true;
}

bool try_set_for_lower_bound_types(Env* env, Tast_for_lower_bound** new_tast, Uast_for_lower_bound* uast) {
    Tast_expr* new_lower = NULL;
    if (!try_set_expr_types(env, &new_lower, uast->child)) {
        return false;
    }
    *new_tast = tast_for_lower_bound_new(uast->pos, new_lower);
    return true;
}

bool try_set_for_upper_bound_types(Env* env, Tast_for_upper_bound** new_tast, Uast_for_upper_bound* uast) {
    Tast_expr* new_upper = NULL;
    if (!try_set_expr_types(env, &new_upper, uast->child)) {
        return false;
    }
    *new_tast = tast_for_upper_bound_new(uast->pos, new_upper);
    return true;
}

bool try_set_for_range_types(Env* env, Tast_for_range** new_tast, Uast_for_range* uast) {
    bool status = true;

    Tast_variable_def* new_var_def = NULL;
    vec_append(&a_main, &env->ancesters, &uast->body->symbol_collection);
    if (!try_set_variable_def_types(env, &new_var_def, uast->var_def, true)) {
        status = false;
    }
    vec_rem_last(&env->ancesters);

    Tast_for_lower_bound* new_lower = NULL;
    if (!try_set_for_lower_bound_types(env, &new_lower, uast->lower_bound)) {
        status = false;
    }

    Tast_for_upper_bound* new_upper = NULL;
    if (!try_set_for_upper_bound_types(env, &new_upper, uast->upper_bound)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, uast->body, false)) {
        status = false;
    }

    *new_tast = tast_for_range_new(uast->pos, new_var_def, new_lower, new_upper, new_body);
    return status;
}

bool try_set_for_with_cond_types(Env* env, Tast_for_with_cond** new_tast, Uast_for_with_cond* uast) {
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(env, &new_cond, uast->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, uast->body, false)) {
        status = false;
    }

    *new_tast = tast_for_with_cond_new(uast->pos, new_cond, new_body);
    return status;
}

bool try_set_if_types(Env* env, Tast_if** new_tast, Uast_if* uast) {
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(env, &new_cond, uast->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(env, &new_body, uast->body, false)) {
        status = false;
    }

    *new_tast = tast_if_new(uast->pos, new_cond, new_body);
    return status;
}

bool try_set_if_else_chain(Env* env, Tast_if_else_chain** new_tast, Uast_if_else_chain* if_else) {
    bool status = true;

    Tast_if_vec new_ifs = {0};
    for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
        Uast_if* old_if = vec_at(&if_else->uasts, idx);
                
        Tast_if* new_if = NULL;
        if (!try_set_if_types(env, &new_if, old_if)) {
            status = false;
        }

        vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(if_else->pos, new_ifs);
    return status;
}

bool try_set_block_types(Env* env, Tast_block** new_tast, Uast_block* block, bool is_directly_in_fun_def) {
    bool status = true;

    Symbol_collection new_sym_coll = block->symbol_collection;

    vec_append(&a_main, &env->ancesters, &new_sym_coll);

    Tast_stmt_vec new_tasts = {0};
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Uast_stmt* curr_tast = vec_at(&block->children, idx);
        Tast_stmt* new_tast = NULL;
        if (!try_set_stmt_types(env, &new_tast, curr_tast)) {
            status = false;
        }

        assert(curr_tast);
        vec_append(&a_main, &new_tasts, new_tast);
    }

    if (is_directly_in_fun_def && (
        block->children.info.count < 1 ||
        vec_at(&block->children, block->children.info.count - 1)->type != UAST_RETURN
    )) {
        Uast_return* rtn_statement = uast_return_new(
            block->pos_end,
            uast_wrap_literal(util_uast_literal_new_from_strv(
                str_view_from_cstr(""), TOKEN_VOID, block->pos_end
            )),
            true
        );
        if (rtn_statement->pos.line == 0) {
            symbol_log(LOG_DEBUG, env);
            unreachable("");
        }
        Tast_stmt* new_rtn_statement = NULL;
        if (!try_set_stmt_types(env, &new_rtn_statement, uast_wrap_return(rtn_statement))) {
            goto error;
        }
        assert(rtn_statement);
        assert(new_rtn_statement);
        vec_append(&a_main, &new_tasts, new_rtn_statement);
    }

error:
    vec_rem_last(&env->ancesters);
    *new_tast = tast_block_new(block->pos, block->is_variadic, new_tasts, new_sym_coll, block->pos_end);
    return status;
}

bool try_set_stmt_types(Env* env, Tast_stmt** new_tast, Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Tast_expr* new_tast_ = NULL;
            if (!try_set_expr_types(env, &new_tast_, uast_unwrap_expr(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_expr(new_tast_);
            return true;
        }
        case UAST_DEF: {
            Tast_def* new_tast_ = NULL;
            if (!try_set_def_types(env, &new_tast_, uast_unwrap_def(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_def(new_tast_);
            return true;
        }
        case UAST_FOR_WITH_COND: {
            Tast_for_with_cond* new_tast_ = NULL;
            if (!try_set_for_with_cond_types(env, &new_tast_, uast_unwrap_for_with_cond(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_for_with_cond(new_tast_);
            return true;
        }
        case UAST_ASSIGNMENT: {
            Tast_assignment* new_tast_ = NULL;
            if (!try_set_assignment_types(env, &new_tast_, uast_unwrap_assignment(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_assignment(new_tast_);
            return true;
        }
        case UAST_RETURN: {
            Tast_return* new_rtn = NULL;
            if (!try_set_return_types(env, &new_rtn, uast_unwrap_return(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_return(new_rtn);
            return true;
        }
        case UAST_FOR_RANGE: {
            Tast_for_range* new_for = NULL;
            if (!try_set_for_range_types(env, &new_for, uast_unwrap_for_range(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_for_range(new_for);
            return true;
        }
        case UAST_BREAK:
            *new_tast = tast_wrap_break(tast_break_new(uast_unwrap_break(stmt)->pos));
            return true;
        case UAST_CONTINUE:
            *new_tast = tast_wrap_continue(tast_continue_new(uast_unwrap_continue(stmt)->pos));
            return true;
        case UAST_BLOCK: {
            assert(uast_unwrap_block(stmt)->pos_end.line > 0);
            Tast_block* new_for = NULL;
            if (!try_set_block_types(env, &new_for, uast_unwrap_block(stmt), false)) {
                return false;
            }
            *new_tast = tast_wrap_block(new_for);
            return true;
        }
        case UAST_IF_ELSE_CHAIN: {
            Tast_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(env, &new_for, uast_unwrap_if_else_chain(stmt))) {
                return false;
            }
            *new_tast = tast_wrap_if_else_chain(new_for);
            return true;
        }
    }
    unreachable("");
}

bool try_set_uast_types(Env* env, Tast** new_tast, Uast* uast) {
    (void) env;
    (void) new_tast;
    (void) uast;
    unreachable(TAST_FMT, uast_print(uast));
}
