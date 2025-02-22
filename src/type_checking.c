#include <type_checking.h>
#include <parser_utils.h>
#include <uast_utils.h>
#include <uast.h>
#include <tast.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <uast_hand_written.h>
#include <lang_type_from_ulang_type.h>
#include <bool_vec.h>
#include <ulang_type.h>

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
    while (lang_type_get_pointer_depth(tast_expr_get_lang_type(expr)) > 0) {
        try(try_set_unary_types_finish(env, &expr, expr, tast_expr_get_pos(expr), TOKEN_DEREF, (Lang_type) {0}));
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
    return uast_function_decl_unwrap(def);
}

const Uast_lang_type* get_parent_function_return_type(const Env* env) {
    return get_parent_function_decl_const(env)->return_type;
}

static bool can_be_implicitly_converted(Lang_type dest, Lang_type src, bool implicit_pointer_depth);

static bool can_be_implicitly_converted_lang_type_atom(Lang_type_atom dest, Lang_type_atom src, bool implicit_pointer_depth) {
    if (!implicit_pointer_depth) {
        if (src.pointer_depth != dest.pointer_depth) {
            return false;
        }
    }

    if (!lang_type_atom_is_signed(dest) || !lang_type_atom_is_signed(src)) {
        return lang_type_atom_is_equal(dest, src);
    }
    return i_lang_type_atom_to_bit_width(dest) >= i_lang_type_atom_to_bit_width(src);
}

static bool can_be_implicitly_converted_tuple(Lang_type_tuple dest, Lang_type_tuple src, bool implicit_pointer_depth) {
    (void) implicit_pointer_depth;
    if (dest.lang_types.info.count != src.lang_types.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < dest.lang_types.info.count; idx++) {
        if (!can_be_implicitly_converted(
            vec_at(&dest.lang_types, idx), vec_at(&src.lang_types, idx), implicit_pointer_depth
        )) {
            return false;
        }
    }

    return true;
}

static bool can_be_implicitly_converted(Lang_type dest, Lang_type src, bool implicit_pointer_depth) {
    if (dest.type != src.type) {
        return false;
    }

    switch (dest.type) {
        case LANG_TYPE_TUPLE:
            return can_be_implicitly_converted_tuple(lang_type_tuple_const_unwrap(dest), lang_type_tuple_const_unwrap(src), implicit_pointer_depth);
        case LANG_TYPE_PRIMITIVE:
            return can_be_implicitly_converted_lang_type_atom(lang_type_primitive_const_unwrap(dest).atom, lang_type_primitive_const_unwrap(src).atom, implicit_pointer_depth);
        case LANG_TYPE_SUM:
            return can_be_implicitly_converted_lang_type_atom(lang_type_sum_const_unwrap(dest).atom, lang_type_sum_const_unwrap(src).atom, implicit_pointer_depth);
        case LANG_TYPE_STRUCT:
            return can_be_implicitly_converted_lang_type_atom(lang_type_struct_const_unwrap(dest).atom, lang_type_struct_const_unwrap(src).atom, implicit_pointer_depth);
        case LANG_TYPE_RAW_UNION:
            return can_be_implicitly_converted_lang_type_atom(lang_type_raw_union_const_unwrap(dest).atom, lang_type_raw_union_const_unwrap(src).atom, implicit_pointer_depth);
        case LANG_TYPE_ENUM:
            return can_be_implicitly_converted_lang_type_atom(lang_type_enum_const_unwrap(dest).atom, lang_type_enum_const_unwrap(src).atom, implicit_pointer_depth);
        case LANG_TYPE_VOID:
            return true;
    }
    unreachable("");
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
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env->file_text, tast_expr_get_pos(argument), 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(tast_expr_get_lang_type(argument)), 
        str_view_print(corres_param->name),
        ulang_type_print(corres_param->lang_type)
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
            ulang_type_print(fun_decl->return_type->lang_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(tast_expr_get_lang_type(child)), 
            ulang_type_print(fun_decl->return_type->lang_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, fun_decl->return_type->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        ulang_type_print(fun_decl->return_type->lang_type)
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
    Lang_type dest_lang_type,
    Tast_expr* src
) {
    if (lang_type_is_equal(dest_lang_type, tast_expr_get_lang_type(src))) {
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(dest_lang_type, tast_expr_get_lang_type(src), false)) {
        if (src->type == TAST_LITERAL) {
            *new_src = src;
            tast_expr_set_lang_type(*new_src, dest_lang_type);
            return CHECK_ASSIGN_OK;
        }
        log(LOG_DEBUG, LANG_TYPE_FMT "   "TAST_FMT"\n", lang_type_print(dest_lang_type), tast_expr_print(src));
        todo();
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

CHECK_ASSIGN_STATUS check_generic_assignment(
    Env* env,
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    Uast_expr* src,
    Pos pos
) {
    if (src->type == UAST_STRUCT_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        // TODO: tests for using struct literal as function argument (and later as an operand)
        if (!try_set_struct_literal_assignment_types(
            env, &new_src_, dest_lang_type, uast_struct_literal_unwrap(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_expr_unwrap(new_src_);
    } else if (src->type == UAST_TUPLE) {
        Tast_tuple* new_src_ = NULL;
        // TODO: tests for using struct literal as function argument (and later as an operand)
        if (!try_set_tuple_assignment_types(
            env, &new_src_, dest_lang_type, uast_tuple_unwrap(src)
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_tuple_wrap(new_src_);
    } else {
        env->parent_of = PARENT_OF_ASSIGN_RHS;
        if (!try_set_expr_types(env, new_src, src)) {
            return CHECK_ASSIGN_ERROR;
        }
        env->parent_of = PARENT_OF_NONE;
    }

    return check_generic_assignment_finish(new_src, dest_lang_type, *new_src);
}

Tast_literal* try_set_literal_types(Uast_literal* literal) {
    switch (literal->type) {
        case UAST_STRING: {
            Uast_string* old_string = uast_string_unwrap(literal);
            return tast_string_wrap(tast_string_new(
                old_string->pos,
                old_string->data,
                lang_type_primitive_new(lang_type_atom_new_from_cstr("u8", 1)),
                old_string->name
            ));
        }
        case UAST_NUMBER: {
            Uast_number* old_number = uast_number_unwrap(literal);
            int64_t bit_width = bit_width_needed_signed(old_number->data);
            String lang_type_str = {0};
            string_extend_cstr(&a_main, &lang_type_str, "i");
            string_extend_int64_t(&a_main, &lang_type_str, bit_width);
            return tast_number_wrap(tast_number_new(
                old_number->pos,
                old_number->data,
                lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new(string_to_strv(lang_type_str), 0))
            )));
        }
        case UAST_VOID: {
            Uast_void* old_void = uast_void_unwrap(literal);
            return tast_void_wrap(tast_void_new(
                old_void->pos
            ));
        }
        case UAST_CHAR: {
            Uast_char* old_char = uast_char_unwrap(literal);
            return tast_char_wrap(tast_char_new(
                old_char->pos,
                old_char->data,
                lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new_from_cstr("u8", 0)))

            ));
        }
        default:
            unreachable("");
    }
}

static void msg_undefined_symbol_internal(const char* file, int line, Str_view file_text, const Uast_stmt* sym_call) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, file_text, uast_stmt_get_pos(sym_call),
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(uast_stmt_get_name(sym_call))
    );
}

#define msg_undefined_symbol(file_text, sym_call) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, file_text, sym_call)

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_type(const Env* env, Tast_expr** new_tast, Uast_symbol* sym_untyped) {
    Uast_def* sym_def;
    if (!usymbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, uast_expr_wrap(uast_symbol_wrap(sym_untyped)));
        return false;
    }

    Lang_type lang_type = uast_def_get_lang_type(env, sym_def);
    Sym_typed_base new_base = {.lang_type = lang_type, .name = sym_untyped->name};
    switch (lang_type.type) {
        case LANG_TYPE_VOID:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_TUPLE:
            // fallthrough
        case LANG_TYPE_STRUCT: {
            Tast_symbol* sym_typed = tast_symbol_new(sym_untyped->pos, new_base);
            *new_tast = tast_symbol_wrap(sym_typed);
            return true;
        }
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
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_lhs));
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_rhs));
    if (!lang_type_is_equal(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs))) {
        if (can_be_implicitly_converted(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs), true)) {
            if (new_rhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_rhs), tast_expr_get_lang_type(new_lhs));
            } else {
                try(try_set_unary_types_finish(env, &new_rhs, new_rhs, tast_expr_get_pos(new_rhs), TOKEN_UNSAFE_CAST, tast_expr_get_lang_type(new_lhs)));
            }
        } else if (can_be_implicitly_converted(tast_expr_get_lang_type(new_rhs), tast_expr_get_lang_type(new_lhs), true)) {
            if (new_lhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_lhs), tast_expr_get_lang_type(new_rhs));
            } else {
                try(try_set_unary_types_finish(env, &new_lhs, new_lhs, tast_expr_get_pos(new_lhs), TOKEN_UNSAFE_CAST, tast_expr_get_lang_type(new_rhs)));
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, oper_pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(tast_expr_get_lang_type(new_lhs)), lang_type_print(tast_expr_get_lang_type(new_rhs))
            );
            return false;
        }
    }
            
    assert(lang_type_get_str(tast_expr_get_lang_type(new_lhs)).count > 0);

    // precalcuate binary in some situations
    if (new_lhs->type == TAST_LITERAL && new_rhs->type == TAST_LITERAL) {
        Tast_literal* lhs_lit = tast_literal_unwrap(new_lhs);
        Tast_literal* rhs_lit = tast_literal_unwrap(new_rhs);

        if (lhs_lit->type != rhs_lit->type) {
            unreachable("this error should have been caught earlier\n");
        }

        Tast_literal* literal = NULL;

        switch (lhs_lit->type) {
            case TAST_NUMBER:
                literal = precalulate_number(
                    tast_number_const_unwrap(lhs_lit),
                    tast_number_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            case TAST_CHAR:
                literal = precalulate_char(
                    tast_char_const_unwrap(lhs_lit),
                    tast_char_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            default:
                unreachable("");
        }

        *new_tast = tast_literal_wrap(literal);
    } else {
        *new_tast = tast_operator_wrap(tast_binary_wrap(tast_binary_new(
            oper_pos,
            new_lhs,
            new_rhs,
            oper_token_type,
            tast_expr_get_lang_type(new_lhs)
        )));
    }

    assert(lang_type_get_str(tast_expr_get_lang_type(*new_tast)).count > 0);
    assert(*new_tast);
    return true;
}

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Tast_expr** new_tast, Uast_binary* operator) {
    log(LOG_DEBUG, UAST_FMT, uast_binary_print(operator));
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
            new_lang_type = tast_expr_get_lang_type(new_child);
            if (new_lang_type.type != LANG_TYPE_PRIMITIVE) {
                // TODO: check if this primitive type can actually be valid operand to logical not
                // TODO: make subtypes for LANG_TYPE_PRIMITIVE to make above easier
                msg(
                    LOG_ERROR, EXPECT_FAIL_UNARY_MISMATCHED_TYPES, env->file_text, tast_expr_get_pos(new_child),
                    "type `"LANG_TYPE_FMT"` is not a valid operand to logical not operation\n",
                    lang_type_print(new_lang_type)
                );
                return false;
            }
            break;
        case TOKEN_DEREF:
            new_lang_type = tast_expr_get_lang_type(new_child);
            if (lang_type_get_pointer_depth(new_lang_type) <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, unary_pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            lang_type_set_pointer_depth(&new_lang_type, lang_type_get_pointer_depth(new_lang_type) - 1);
            break;
        case TOKEN_REFER:
            new_lang_type = tast_expr_get_lang_type(new_child);
            lang_type_set_pointer_depth(&new_lang_type, lang_type_get_pointer_depth(new_lang_type) + 1);
            break;
        case TOKEN_UNSAFE_CAST:
            new_lang_type = cast_to;
            assert(lang_type_get_str(cast_to).count > 0);
            if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number(tast_expr_get_lang_type(new_child)) && lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0) {
            } else if (lang_type_is_number(tast_expr_get_lang_type(new_child)) && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0) {
            } else {
                todo();
            }
            break;
        default:
            unreachable("");
    }

    *new_tast = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
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

    return try_set_unary_types_finish(env, new_tast, new_child, uast_unary_get_pos(unary), unary->token_type, lang_type_from_ulang_type(env, unary->lang_type));
}

// returns false if unsuccessful
bool try_set_operator_types(Env* env, Tast_expr** new_tast, Uast_operator* operator) {
    if (operator->type == UAST_UNARY) {
        return try_set_unary_types(env, new_tast, uast_unary_unwrap(operator));
    } else if (operator->type == UAST_BINARY) {
        return try_set_binary_types(env, new_tast, uast_binary_unwrap(operator));
    } else {
        unreachable("");
    }
}

bool try_set_tuple_assignment_types(
    Env* env,
    Tast_tuple** new_tast,
    Lang_type dest_lang_type,
    Uast_tuple* tuple
) {
    if (dest_lang_type.type != LANG_TYPE_TUPLE || lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count != tuple->members.info.count) {
        // TODO: clean up these error messages
        msg(
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_TUPLE_COUNT, env->file_text,
            uast_tuple_get_pos(tuple),
            "tuple `"UAST_FMT"` cannot be assigned to `"LANG_TYPE_FMT"`\n",
            uast_tuple_print(tuple), lang_type_print(dest_lang_type)
        );
        msg(
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text,
            uast_tuple_get_pos(tuple),
            "tuple `"UAST_FMT"` has %zu elements, but type `"LANG_TYPE_FMT" has %zu elements`\n",
            uast_tuple_print(tuple), tuple->members.info.count,
            lang_type_print(dest_lang_type), lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count
        );
        return false;
    }

    Tast_expr_vec new_members = {0};
    Lang_type_vec new_lang_type = {0};

    for (size_t idx = 0; idx < lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count; idx++) {
        Uast_expr* curr_src = vec_at(&tuple->members, idx);
        Lang_type curr_dest = vec_at_const(lang_type_tuple_const_unwrap(dest_lang_type).lang_types, idx);

        Tast_expr* new_memb = NULL;
        switch (check_generic_assignment(
            env,
            &new_memb,
            curr_dest,
            curr_src,
            uast_expr_get_pos(curr_src)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                    tast_expr_get_pos(new_memb),
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(tast_expr_get_lang_type(new_memb)), lang_type_print(curr_dest)
                );
                todo();
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                todo();
        }

        vec_append(&a_main, &new_members, new_memb);
        vec_append(&a_main, &new_lang_type, tast_expr_get_lang_type(new_memb));
    }

    *new_tast = tast_tuple_new(tuple->pos, new_members, lang_type_tuple_new(new_lang_type));
    return true;
}

bool try_set_struct_literal_assignment_types(
    Env* env,
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_struct_literal* struct_literal,
    Pos assign_pos
) {
    switch (dest_lang_type.type) {
        case LANG_TYPE_STRUCT:
            break;
        case LANG_TYPE_RAW_UNION:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env->file_text,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        case LANG_TYPE_SUM:
            // TODO: improve this
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_SUM, env->file_text,
                assign_pos, "struct literal cannot be assigned to sum\n"
            );
            return false;
        case LANG_TYPE_PRIMITIVE:
            // TODO: expected failure case for this
            unreachable("returning value in non void function");
        default:
            log(LOG_DEBUG, TAST_FMT"\n", lang_type_print(dest_lang_type));
            unreachable("");
    }
    Uast_def* struct_def_ = NULL;
    try(usymbol_lookup(&struct_def_, env, lang_type_struct_const_unwrap(dest_lang_type).atom.str));
    Uast_struct_def* struct_def = uast_struct_def_unwrap(struct_def_);
    
    Tast_expr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        //log(LOG_DEBUG, "%zu\n", idx);
        Uast_variable_def* memb_sym_def = vec_at(&struct_def->base.members, idx);
        log(LOG_DEBUG, STR_VIEW_FMT, uast_stmt_print(uast_expr_const_wrap(uast_struct_literal_const_wrap(struct_literal))));
        Uast_assignment* assign_memb_sym = uast_assignment_unwrap(vec_at(&struct_literal->members, idx));
        Uast_symbol* memb_sym_piece_untyped = uast_symbol_unwrap(uast_expr_unwrap(assign_memb_sym->lhs));
        Tast_expr* new_rhs = NULL;

        switch (check_generic_assignment(
            env, &new_rhs, lang_type_from_ulang_type(env, memb_sym_def->lang_type), assign_memb_sym->rhs, assign_memb_sym->pos
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                    assign_memb_sym->pos,
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(tast_expr_get_lang_type(new_rhs)), ulang_type_print(memb_sym_def->lang_type)
                );
                return false;
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                unreachable("");
        }


        //*tast_expr_set_lang_type(new_rhs) = memb_sym_def->lang_type;
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

        vec_append(&a_main, &new_literal_members, new_rhs);
    }

    Tast_struct_literal* new_lit = tast_struct_literal_new(
        struct_literal->pos,
        new_literal_members,
        struct_literal->name,
        dest_lang_type
    );
    *new_tast = tast_expr_wrap(tast_struct_literal_wrap(new_lit));

    Tast_struct_lit_def* new_def = tast_struct_lit_def_new(
        new_lit->pos,
        new_lit->members,
        new_lit->name,
        new_lit->lang_type
    );

    try(symbol_add(env, tast_literal_def_wrap(tast_struct_lit_def_wrap(new_def))));
    return true;
}

bool try_set_expr_types(Env* env, Tast_expr** new_tast, Uast_expr* uast) {
    switch (uast->type) {
        case UAST_LITERAL: {
            *new_tast = tast_literal_wrap(try_set_literal_types(uast_literal_unwrap(uast)));
            return true;
        }
        case TAST_SYMBOL:
            if (!try_set_symbol_type(env, new_tast, uast_symbol_unwrap(uast))) {
                return false;
            } else {
                assert(*new_tast);
            }
            return true;
        case UAST_MEMBER_ACCESS: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_member_access_types(env, &new_tast_, uast_member_access_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_expr_unwrap(new_tast_);
            return true;
        }
        case UAST_INDEX: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_index_untyped_types(env, &new_tast_, uast_index_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_expr_unwrap(new_tast_);
            return true;
        }
        case UAST_OPERATOR:
            if (!try_set_operator_types(env, new_tast, uast_operator_unwrap(uast))) {
                return false;
            }
            assert(*new_tast);
            return true;
        case UAST_FUNCTION_CALL:
            return try_set_function_call_types(env, new_tast, uast_function_call_unwrap(uast));
        case UAST_TUPLE: {
            Tast_tuple* new_call = NULL;
            if (!try_set_tuple_types(env, &new_call, uast_tuple_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_tuple_wrap(new_call);
            return true;
        }
        case UAST_STRUCT_LITERAL:
            unreachable("");
        case UAST_SUM_ACCESS: {
            Tast_sum_access* new_access = NULL;
            if (!try_set_sum_access_types(env, &new_access, uast_sum_access_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_sum_access_wrap(new_access);
            return true;
        }
    }
    unreachable("");
}

bool try_set_def_types(Env* env, Tast_def** new_tast, Uast_def* uast) {
    switch (uast->type) {
        case UAST_VARIABLE_DEF: {
            Tast_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, uast_variable_def_unwrap(uast), true)) {
                return false;
            }
            *new_tast = tast_variable_def_wrap(new_def);
            return true;
        }
        case UAST_FUNCTION_DECL: {
            Tast_function_decl* new_decl = NULL;
            if (!try_set_function_decl_types(env, &new_decl, uast_function_decl_unwrap(uast), false)) {
                return false;
            }
            *new_tast = tast_function_decl_wrap(new_decl);
            return true;
        }
        case UAST_FUNCTION_DEF: {
            Tast_function_def* new_def = NULL;
            if (!try_set_function_def_types(env, &new_def, uast_function_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_function_def_wrap(new_def);
            return true;
        }
        case UAST_STRUCT_DEF: {
            Tast_struct_def* new_def = NULL;
            if (!try_set_struct_def_types(env, &new_def, uast_struct_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_struct_def_wrap(new_def);
            return true;
        }
        case UAST_RAW_UNION_DEF: {
            Tast_raw_union_def* new_def = NULL;
            if (!try_set_raw_union_def_types(env, &new_def, uast_raw_union_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_raw_union_def_wrap(new_def);
            return true;
        }
        case UAST_ENUM_DEF: {
            Tast_enum_def* new_def = NULL;
            if (!try_set_enum_def_types(env, &new_def, uast_enum_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_enum_def_wrap(new_def);
            return true;
        }
        case UAST_PRIMITIVE_DEF: {
            Tast_primitive_def* new_def = NULL;
            if (!try_set_primitive_def_types(env, &new_def, uast_primitive_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_primitive_def_wrap(new_def);
            return true;
        }
        case UAST_LITERAL_DEF: {
            Tast_literal_def* new_def = NULL;
            if (!try_set_literal_def_types(env, &new_def, uast_literal_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_literal_def_wrap(new_def);
            return true;
        }
        case UAST_SUM_DEF: {
            Tast_sum_def* new_def = NULL;
            if (!try_set_sum_def_types(env, &new_def, uast_sum_def_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_sum_def_wrap(new_def);
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
        env, &new_rhs, tast_stmt_get_lang_type(new_lhs), assignment->rhs, assignment->pos
    )) {
        case CHECK_ASSIGN_OK:
            break;
        case CHECK_ASSIGN_INVALID:
            msg(
                LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                assignment->pos,
                "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                lang_type_print(tast_expr_get_lang_type(new_rhs)), lang_type_print(tast_stmt_get_lang_type(new_lhs))
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

bool try_set_function_call_types_sum_case(Env* env, Tast_sum_case** new_case, Uast_expr_vec args, Tast_sum_case* sum_case) {
    switch (sum_case->tag->lang_type.type) {
        case LANG_TYPE_VOID: {
            if (args.info.count > 0) {
                // TODO: expected failure case
                msg(LOG_DEBUG, EXPECT_FAIL_TYPE_NONE, env->file_text, uast_expr_get_pos(vec_at(&args, 0)), "no arguments expected here\n");
                todo();
            }
            *new_case = sum_case;
            return true;
        }
        default: {
            // tast_sum_case->tag->lang_type is of selected varient of sum (maybe)
            Uast_variable_def* new_def = uast_variable_def_new(
                sum_case->pos,
                lang_type_to_ulang_type(sum_case->tag->lang_type),
                false,
                uast_expr_get_name(vec_at(&args, 0))
            );
            usymbol_add_defer(env, uast_variable_def_wrap(new_def));
            // TODO: add assignment here
            log(LOG_DEBUG, TAST_FMT, uast_variable_def_print(new_def));
            log(LOG_DEBUG, TAST_FMT, tast_sum_case_print(sum_case));

            Uast_assignment* new_assign = uast_assignment_new(
                new_def->pos,
                uast_def_wrap(uast_variable_def_wrap(new_def)),
                uast_sum_access_wrap(uast_sum_access_new(
                    new_def->pos,
                    sum_case->tag,
                    lang_type_from_ulang_type(env, new_def->lang_type),
                    uast_symbol_wrap(uast_symbol_new(
                        new_def->pos,
                        uast_expr_get_name(env->parent_of_operand)
                    ))
                ))
            );

            vec_append(&a_main, &env->switch_case_defer_add_sum_case_part, uast_assignment_wrap(new_assign));

            *new_case = sum_case;
            return true;
        }
    }
}

bool try_set_function_call_types(Env* env, Tast_expr** new_call, Uast_function_call* fun_call) {
    bool status = true;

    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, fun_call->callee)) {
        return false;
    }

    Uast_def* fun_def = NULL;
    switch (new_callee->type) {
        case TAST_SYMBOL: {
            if (!usymbol_lookup(&fun_def, env, tast_symbol_unwrap(new_callee)->base.name)) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env->file_text, fun_call->pos,
                    "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(tast_symbol_unwrap(new_callee)->base.name)
                );
                status = false;
                goto error;
            }
            break;
        }
        case TAST_MEMBER_ACCESS:
            todo();
            //if (tast_member_access_is_sum(env, tast_member_access_unwrap(new_callee))) {
            //    todo();
            //} else {
            //    unreachable("");
            //}
        case TAST_SUM_CALLEE: {
            if (fun_call->args.info.count != 1) {
                todo();
            }
            Tast_sum_callee* sum_callee = tast_sum_callee_unwrap(new_callee);
            log(LOG_DEBUG, TAST_FMT, tast_sum_callee_print(sum_callee));
            log(LOG_DEBUG, TAST_FMT, tast_enum_lit_print(sum_callee->tag));

            Uast_def* sum_def_ = NULL;
            try(usymbol_lookup(&sum_def_, env, lang_type_get_str(sum_callee->sum_lang_type)));
            Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);
            log(LOG_DEBUG, TAST_FMT, uast_sum_def_print(uast_sum_def_unwrap(sum_def_)));

            Tast_expr* new_item = NULL;
            switch (check_generic_assignment(
                env,
                &new_item,
                lang_type_from_ulang_type(env, vec_at(&sum_def->base.members, sum_callee->tag->data)->lang_type),
                vec_at(&fun_call->args, 0),
                uast_expr_get_pos(vec_at(&fun_call->args, 0))
            )) {
                case CHECK_ASSIGN_OK:
                    log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_item));
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg(
                        LOG_ERROR, EXPECT_FAIL_SUM_LIT_INVALID_ARG, env->file_text, tast_expr_get_pos(new_item),
                        "cannot assign "TAST_FMT" of type `"LANG_TYPE_FMT"` to '"LANG_TYPE_FMT"`\n", 
                        tast_expr_print(new_item),
                        lang_type_print(tast_expr_get_lang_type(new_item)), 
                        lang_type_print(lang_type_from_ulang_type(
                            env, vec_at(&sum_def->base.members, sum_callee->tag->data)->lang_type
                        ))
                   );
                   break;
                case CHECK_ASSIGN_ERROR:
                    todo();
                default:
                    unreachable("");
            }

            // TODO: is tag set to a type that makes sense?
            // (right now, it is set to i64)
            sum_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new_from_cstr("i64", 0)));

            Tast_sum_lit* new_lit = tast_sum_lit_new(
                sum_callee->pos,
                sum_callee->tag,
                new_item,
                sum_callee->sum_lang_type
            );
            *new_call = tast_literal_wrap(tast_sum_lit_wrap(new_lit));
            return true;
        }
        case TAST_SUM_CASE: {
            if (fun_call->args.info.count != 1) {
                todo();
            }
            Tast_sum_case* new_case = NULL;
            if (!try_set_function_call_types_sum_case(env, &new_case, fun_call->args, tast_sum_case_unwrap(new_callee))) {
                return false;
            }
            *new_call = tast_sum_case_wrap(new_case);
            return true;
        }
        default:
            unreachable(TAST_FMT, tast_expr_print(new_callee));
    }

    Uast_function_decl* fun_decl;
    switch (fun_def->type) {
        case UAST_FUNCTION_DEF:
            fun_decl = uast_function_def_unwrap(fun_def)->decl;
            break;
        case UAST_FUNCTION_DECL:
            fun_decl = uast_function_decl_unwrap(fun_def);
            break;
        case UAST_SUM_DEF: {
            Uast_sum_def* sum_def = uast_sum_def_unwrap(fun_def);
            (void) sum_def;
            //*new_call = 
            todo();
        }
        default:
            unreachable("");
    }
    Tast_lang_type* fun_rtn_type = NULL;
    if (!try_types_set_lang_type(env, &fun_rtn_type, fun_decl->return_type)) {
        return false;
    }
    log(LOG_DEBUG, TAST_FMT, uast_lang_type_print(fun_decl->return_type));
    log(LOG_DEBUG, TAST_FMT, tast_lang_type_print(fun_rtn_type));

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
        string_extend_strv(&print_arena, &message, fun_decl->name);
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
            LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, uast_def_get_pos(fun_def),
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

        if (lang_type_is_equal(lang_type_from_ulang_type(env, corres_param->lang_type), lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new_from_cstr("any", 0))))) {
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
            log(LOG_DEBUG, LANG_TYPE_FMT"\n", ulang_type_print(corres_param->lang_type));
            log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(lang_type_from_ulang_type(env, corres_param->lang_type)));
            switch (check_generic_assignment(
                env,
                &new_arg,
                lang_type_from_ulang_type(env, corres_param->lang_type),
                arg,
                uast_expr_get_pos(arg)
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

    *new_call = tast_function_call_wrap(tast_function_call_new(
        fun_call->pos,
        new_args,
        fun_decl->name,
        fun_rtn_type->lang_type
    ));

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
        vec_append(&a_main, &new_lang_type, tast_expr_get_lang_type(new_memb));
    }

    *new_tuple = tast_tuple_new(tuple->pos, new_members, lang_type_tuple_new(new_lang_type));
    return true;
}

bool try_set_sum_access_types(Env* env, Tast_sum_access** new_access, Uast_sum_access* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, access->callee)) {
        return false;
    }

    *new_access = tast_sum_access_new(
        access->pos,
        access->tag, 
        access->lang_type,
        new_callee
    );

    return true;
}

static void msg_invalid_member(
    const Env* env,
    Ustruct_def_base base,
    const Uast_member_access* access
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
    const Uast_member_access* access
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
    Uast_member_access* access,
    Ustruct_def_base def_base,
    Tast_expr* new_callee
) {
    Uast_variable_def* member_def = NULL;
    if (!uast_try_get_member_def(&member_def, &def_base, access->member_name)) {
        msg_invalid_member(env, def_base, access);
        return false;
    }

    Tast_member_access* new_access = tast_member_access_new(
        access->pos,
        lang_type_from_ulang_type(env, member_def->lang_type),
        access->member_name,
        new_callee
    );

    *new_tast = tast_expr_wrap(tast_member_access_wrap(new_access));

    assert(lang_type_get_str(new_access->lang_type).count > 0);
    return true;
}

bool try_set_member_access_types_finish_sum_def(
    Env* env,
    Tast_stmt** new_tast,
    Uast_sum_def* sum_def,
    Uast_member_access* access,
    Tast_expr* new_callee
) {
    (void) new_callee;

    switch (env->parent_of) {
        case PARENT_OF_CASE:
            log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_callee));
            log(LOG_DEBUG, TAST_FMT, uast_member_access_print(access));
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &sum_def->base, access->member_name)) {
                todo();
                //msg_invalid_enum_member(env, enum_def->base, access);
                //return false;
            }
            log(LOG_DEBUG, TAST_FMT, uast_variable_def_print(member_def));
            Tast_enum_lit* new_tag = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&sum_def->base, access->member_name),
                lang_type_from_ulang_type(env, member_def->lang_type)
            );

            *new_tast = tast_expr_wrap(tast_sum_case_wrap(tast_sum_case_new(
                access->pos,
                new_tag,
                lang_type_sum_const_wrap(lang_type_sum_new(lang_type_atom_new(sum_def->base.name, 0)))
            )));

            return true;
            //Uast_variable_def* member_def = NULL;
            //if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
            //    msg_invalid_enum_member(env, enum_def->base, access);
            //    return false;
            //}

            //Tast_enum_lit* new_lit = tast_enum_lit_new(
            //    access->pos,
            //    uast_get_member_index(&enum_def->base, access->member_name),
            //    member_def->lang_type
            //);

            //*new_tast = tast_expr_wrap(tast_literal_wrap(tast_enum_lit_wrap(new_lit)));
            //assert(member_def->lang_type.str.count > 0);
            //return true;
        case PARENT_OF_ASSIGN_RHS: {
            Uast_variable_def* member_def = NULL;
            log(LOG_DEBUG, TAST_FMT, uast_sum_def_print(sum_def));
            log(LOG_DEBUG, TAST_FMT, uast_member_access_print(access));
            if (!uast_try_get_member_def(&member_def, &sum_def->base, access->member_name)) {
                todo();
                //msg_invalid_enum_member(env, enum_def->base, access);
                //return false;
            }
            
            Tast_enum_lit* new_tag = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&sum_def->base, access->member_name),
                lang_type_from_ulang_type(env, member_def->lang_type)
            );

            Tast_sum_callee* new_callee = tast_sum_callee_new(
                access->pos,
                new_tag,
                lang_type_sum_const_wrap(lang_type_sum_new(lang_type_atom_new(sum_def->base.name, 0)))
            );

            if (new_tag->lang_type.type != LANG_TYPE_VOID) {
                *new_tast = tast_expr_wrap(tast_sum_callee_wrap(new_callee));
                return true;
            }

            new_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new_from_cstr("i64", 0)));

            Tast_sum_lit* new_lit = tast_sum_lit_new(
                new_callee->pos,
                new_callee->tag,
                tast_literal_wrap(tast_void_wrap(tast_void_new(new_callee->pos))),
                new_callee->sum_lang_type
            );
            *new_tast = tast_expr_wrap(tast_literal_wrap(tast_sum_lit_wrap(new_lit)));
            return true;

            todo();

            //Tast_sum_case* new_call = NULL;
            //*new_tast = tast_expr_wrap(tast_sum_callee_wrap(new_call));
            //return true;
        }
        case PARENT_OF_NONE:
            unreachable("");
    }
    unreachable("");
}

bool try_set_member_access_types_finish(
    Env* env,
    Tast_stmt** new_tast,
    Uast_def* lang_type_def,
    Uast_member_access* access,
    Tast_expr* new_callee
) {
    switch (lang_type_def->type) {
        case UAST_STRUCT_DEF: {
            Uast_struct_def* struct_def = uast_struct_def_unwrap(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, struct_def->base, new_callee
            );
        }
        case UAST_RAW_UNION_DEF: {
            Uast_raw_union_def* raw_union_def = uast_raw_union_def_unwrap(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                env, new_tast, access, raw_union_def->base, new_callee
            );
        }
        case UAST_ENUM_DEF: {
            Uast_enum_def* enum_def = uast_enum_def_unwrap(lang_type_def);
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name)) {
                msg_invalid_enum_member(env, enum_def->base, access);
                return false;
            }

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&enum_def->base, access->member_name),
                lang_type_from_ulang_type(env, member_def->lang_type)
            );

            *new_tast = tast_expr_wrap(tast_literal_wrap(tast_enum_lit_wrap(new_lit)));
            return true;
        }
        case UAST_SUM_DEF:
            return try_set_member_access_types_finish_sum_def(env, new_tast, uast_sum_def_unwrap(lang_type_def), access, new_callee);
        default:
            unreachable(UAST_FMT"\n", uast_stmt_print(uast_def_wrap(lang_type_def)));
    }

    unreachable("");
}

bool try_set_member_access_types(
    Env* env,
    Tast_stmt** new_tast,
    Uast_member_access* access
) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(env, &new_callee, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case TAST_SYMBOL: {
            Tast_symbol* sym = tast_symbol_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, lang_type_get_str(sym->base.lang_type))) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MEMBER_ACCESS: {
            Tast_member_access* sym = tast_member_access_unwrap(new_callee);

            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, lang_type_get_str(sym->lang_type))) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);
        }
        case TAST_OPERATOR: {
            Tast_operator* sym = tast_operator_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, env, lang_type_get_str(tast_operator_get_lang_type(sym)))) {
                todo();
            }

            return try_set_member_access_types_finish(env, new_tast, lang_type_def, access, new_callee);

        }
        default:
            unreachable(TAST_FMT, tast_expr_print(new_callee));
    }
    unreachable("");
}

bool try_set_index_untyped_types(Env* env, Tast_stmt** new_tast, Uast_index* index) {
    Tast_expr* new_callee = NULL;
    Tast_expr* new_inner_index = NULL;
    if (!try_set_expr_types(env, &new_callee, index->callee)) {
        return false;
    }
    if (!try_set_expr_types(env, &new_inner_index, index->index)) {
        return false;
    }

    Lang_type new_lang_type = tast_expr_get_lang_type(new_callee);
    if (lang_type_get_pointer_depth(new_lang_type) < 1) {
        todo();
    }
    lang_type_set_pointer_depth(&new_lang_type, lang_type_get_pointer_depth(new_lang_type) - 1);

    Tast_index* new_index = tast_index_new(
        index->pos,
        new_lang_type,
        new_inner_index,
        new_callee
    );

    *new_tast = tast_expr_wrap(tast_index_wrap(new_index));
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
            new_child = tast_operator_unwrap(new_child_);
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
        Uast_variable_def* curr = vec_at(&base->members, idx);

        Tast_variable_def* new_memb = NULL;
        if (try_set_variable_def_types(env, &new_memb, curr, false)) {
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
    Tast_enum_def* new_def = tast_enum_def_new(tast->pos, new_base);
    try(symbol_add(env, tast_enum_def_wrap(new_def)));
    *new_tast = new_def;
    return success;
}

bool try_set_sum_def_types(Env* env, Tast_sum_def** new_tast, Uast_sum_def* tast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &tast->base);
    Tast_sum_def* new_def = tast_sum_def_new(tast->pos, new_base);
    try(symbol_add(env, tast_sum_def_wrap(new_def)));
    *new_tast = new_def;
    return success;
}

bool try_set_primitive_def_types(Env* env, Tast_primitive_def** new_tast, Uast_primitive_def* tast) {
    (void) env;
    *new_tast = tast_primitive_def_new(tast->pos, tast->lang_type);
    try(symbol_add(env, tast_primitive_def_wrap(*new_tast)));
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
    try(symbol_add(env, tast_raw_union_def_wrap(*new_tast)));
    return success;
}

bool try_set_struct_def_types(Env* env, Tast_struct_def** new_tast, Uast_struct_def* uast) {
    Uast_def* dummy = NULL;
    assert(usymbol_lookup(&dummy, env, uast->base.name));
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &uast->base);
    *new_tast = tast_struct_def_new(uast->pos, new_base);
    try(symbol_add(env, tast_struct_def_wrap(*new_tast)));
    return success;
}

static void msg_undefined_type_internal(
    const char* file,
    int line,
    const Env* env,
    Pos pos,
    Ulang_type lang_type
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_text, pos,
        "type `"LANG_TYPE_FMT"` is not defined\n", ulang_type_print(lang_type)
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
    if (!usymbol_lookup(&dummy, env, ulang_type_regular_const_unwrap(uast->lang_type).atom.str)) {
        msg_undefined_type(env, uast->pos, uast->lang_type);
        return false;
    }

    *new_tast = tast_variable_def_new(uast->pos, lang_type_from_ulang_type(env, uast->lang_type), uast->is_variadic, uast->name);
    log(LOG_DEBUG, "adding:"STR_VIEW_FMT, tast_variable_def_print(*new_tast));
    symbol_log(LOG_DEBUG, env);
    if (add_to_sym_tbl && !env->type_checking_is_in_struct_base_def) {
        try(symbol_add(env, tast_variable_def_wrap(*new_tast)));
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
    if (!try_types_set_lang_type(env, &new_rtn_type, decl->return_type)) {
        return false;
    }

    *new_tast = tast_function_decl_new(decl->pos, new_params, new_rtn_type, decl->name);
    try(symbol_add(env, tast_function_decl_wrap(*new_tast)));
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

static bool try_types_internal_set_lang_type(
    Env* env,
    Ulang_type lang_type,
    Pos pos
) {
    switch (lang_type.type) {
        case ULANG_TYPE_TUPLE: {
            for (size_t idx = 0; idx < ulang_type_tuple_const_unwrap(lang_type).ulang_types.info.count; idx++) {
                if (!try_types_internal_set_lang_type(env, vec_at_const(ulang_type_tuple_const_unwrap(lang_type).ulang_types, idx), pos)) {
                    return false;
                }
            }
        }
        case ULANG_TYPE_REGULAR:
            lang_type_from_ulang_type(env, lang_type);
            return true;
    }
    unreachable("");
}

bool try_types_set_lang_type(
    Env* env,
    Tast_lang_type** new_tast,
    Uast_lang_type* uast
) {
    if (!try_types_internal_set_lang_type(env, uast->lang_type, uast->pos)) {
        return false;
    }

    *new_tast = tast_lang_type_new(uast->pos, lang_type_from_ulang_type(env, uast->lang_type));
    return true;
}

bool try_set_return_types(Env* env, Tast_return** new_tast, Uast_return* rtn) {
    *new_tast = NULL;

    Ulang_type fun_rtn_type = get_parent_function_return_type(env)->lang_type;

    Tast_expr* new_child = NULL;
    switch (check_generic_assignment(env, &new_child, lang_type_from_ulang_type(env, fun_rtn_type), rtn->child, rtn->pos)) {
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
        return false;
    }

    Uast_stmt_vec new_if_children = {0};
    vec_extend(&a_main, &new_if_children, &env->switch_case_defer_add_sum_case_part);
    vec_extend(&a_main, &new_if_children, &env->switch_case_defer_add_if_true);
    vec_extend(&a_main, &new_if_children, &uast->body->children);
    uast->body->children = new_if_children;
    vec_reset(&env->switch_case_defer_add_sum_case_part);
    vec_reset(&env->switch_case_defer_add_if_true);

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

bool try_set_case_types(Env* env, Tast_if** new_tast, const Uast_case* lang_case) {
    (void) env;
    (void) new_tast;
    (void) lang_case;
    todo();
}

typedef struct {
    Lang_type oper_lang_type;
    bool default_is_pre;
    Bool_vec covered;
    size_t max_data;
} Exhaustive_data;

static Exhaustive_data check_for_exhaustiveness_start(const Env* env, Lang_type oper_lang_type) {
    Exhaustive_data exhaustive_data = {0};

    exhaustive_data.oper_lang_type = oper_lang_type;

    Uast_def* enum_def_ = NULL;
    if (!usymbol_lookup(&enum_def_, env, lang_type_get_str(exhaustive_data.oper_lang_type))) {
        todo();
    }
    Ustruct_def_base enum_def = {0};
    switch (enum_def_->type) {
        case UAST_ENUM_DEF:
            enum_def = uast_enum_def_unwrap(enum_def_)->base;
            break;
        case UAST_SUM_DEF:
            enum_def = uast_sum_def_unwrap(enum_def_)->base;
            break;
        default:
            todo();
    }
    try(enum_def.members.info.count > 0);
    exhaustive_data.max_data = enum_def.members.info.count - 1;

    vec_reserve(&print_arena, &exhaustive_data.covered, exhaustive_data.max_data + 1);
    for (size_t idx = 0; idx < exhaustive_data.max_data + 1; idx++) {
        vec_append(&print_arena, &exhaustive_data.covered, false);
    }
    try(exhaustive_data.covered.info.count == exhaustive_data.max_data + 1);

    return exhaustive_data;
}

static bool check_for_exhaustiveness_inner(
    Exhaustive_data* exhaustive_data,
    const Tast_if* curr_if,
    bool is_default
) {
    if (is_default) {
        if (exhaustive_data->default_is_pre) {
            // TODO: expected failure case
            unreachable("muliple default cases present");
        }
        exhaustive_data->default_is_pre = true;
        return true;
    }

    switch (exhaustive_data->oper_lang_type.type) {
        case LANG_TYPE_ENUM: {
            const Tast_enum_lit* curr_lit = tast_enum_lit_unwrap(
                tast_literal_unwrap(
                    tast_binary_unwrap(curr_if->condition->child)->rhs
                )
            );
            if (curr_lit->data > (int64_t)exhaustive_data->max_data) {
                unreachable("invalid enum value\n");
            }
            if (vec_at(&exhaustive_data->covered, curr_lit->data)) {
                // TODO: expected failure case
                unreachable("duplicate case");
            }
            *vec_at_ref(&exhaustive_data->covered, curr_lit->data) = true;
            return true;
        }
        case LANG_TYPE_SUM: {
            log(LOG_DEBUG, TAST_FMT, tast_enum_lit_print(tast_sum_case_unwrap(tast_binary_unwrap(curr_if->condition->child)->rhs)->tag));

            const Tast_enum_lit* curr_lit = tast_sum_case_unwrap(
                tast_binary_unwrap(curr_if->condition->child)->rhs
            )->tag;

            if (curr_lit->data > (int64_t)exhaustive_data->max_data) {
                unreachable("invalid enum value\n");
            }
            if (vec_at(&exhaustive_data->covered, curr_lit->data)) {
                // TODO: expected failure case
                unreachable("duplicate case");
            }
            *vec_at_ref(&exhaustive_data->covered, curr_lit->data) = true;
            return true;
        }
        default:
            todo();
    }
    unreachable("");
}

static bool check_for_exhaustiveness_finish(const Env* env, Exhaustive_data exhaustive_data, Pos pos_switch) {
        try(exhaustive_data.covered.info.count == exhaustive_data.max_data + 1);

        if (exhaustive_data.default_is_pre) {
            return true;
        }

        for (size_t idx = 0; idx < exhaustive_data.covered.info.count; idx++) {
            if (!vec_at(&exhaustive_data.covered, idx)) {
                Uast_def* enum_def_ = NULL;
                try(usymbol_lookup(&enum_def_, env, lang_type_get_str(exhaustive_data.oper_lang_type)));
                Ustruct_def_base enum_def = {0};
                switch (enum_def_->type) {
                    case UAST_ENUM_DEF:
                        enum_def = uast_enum_def_unwrap(enum_def_)->base;
                        break;
                    case UAST_SUM_DEF:
                        enum_def = uast_sum_def_unwrap(enum_def_)->base;
                        break;
                    default:
                        todo();
                }

                // TODO: list multiple uncovered cases
                msg(
                    LOG_ERROR, EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH, env->file_text, pos_switch,
                    "case `"LANG_TYPE_FMT"."STR_VIEW_FMT"` is not covered\n",
                    ulang_type_print(vec_at(&enum_def.members, idx)->lang_type),
                    str_view_print(vec_at(&enum_def.members, idx)->name)
                );
                return false;
            }
        }

        return true;
}

bool try_set_switch_types(Env* env, Tast_if_else_chain** new_tast, const Uast_switch* lang_switch) {
    Tast_if_vec new_ifs = {0};

    Tast_expr* new_operand = NULL;
    try(try_set_expr_types(env, &new_operand, lang_switch->operand));

    Exhaustive_data exhaustive_data = check_for_exhaustiveness_start(
        env, tast_expr_get_lang_type(new_operand)
    );

    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        Uast_case* old_case = vec_at(&lang_switch->cases, idx);
        Uast_condition* cond = NULL;
        if (old_case->is_default) {
            cond = uast_condition_new(
                old_case->pos,
                uast_condition_get_default_child(uast_literal_wrap(
                    util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, old_case->pos)
                ))
            );
        } else {
            cond = uast_condition_new(old_case->pos, uast_binary_wrap(uast_binary_new(
                old_case->pos, lang_switch->operand, old_case->expr, TOKEN_DOUBLE_EQUAL
            )));
        }

        vec_append(&a_main, &env->switch_case_defer_add_if_true, old_case->if_true);
        Uast_block* if_true = uast_block_new(
            old_case->pos,
            false,
            (Uast_stmt_vec) {0},
            (Symbol_collection) {0},
            old_case->pos
        );
                
        env->parent_of = PARENT_OF_CASE;
        env->parent_of_operand = lang_switch->operand;
        Tast_if* new_if = NULL;
        if (!try_set_if_types(env, &new_if, uast_if_new(old_case->pos, cond, if_true))) {
            env->parent_of_operand = NULL;
            env->parent_of = PARENT_OF_NONE;
            return false;
        }
        env->parent_of_operand = NULL;
        env->parent_of = PARENT_OF_NONE;

        if (!check_for_exhaustiveness_inner(&exhaustive_data, new_if, old_case->is_default)) {
            return false;
        }

        vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(lang_switch->pos, new_ifs);
    return check_for_exhaustiveness_finish(env, exhaustive_data, lang_switch->pos);
}

// TODO: merge this with msg_redefinition_of_symbol?
static void try_set_msg_redefinition_of_symbol(const Env* env, const Uast_def* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, env->file_text, uast_def_get_pos(new_sym_def),
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    try(usymbol_lookup(&original_def, env, uast_def_get_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, env->file_text, uast_def_get_pos(original_def),
        STR_VIEW_FMT " originally defined here\n", str_view_print(uast_def_get_name(original_def))
    );
}

bool try_set_block_types(Env* env, Tast_block** new_tast, Uast_block* block, bool is_directly_in_fun_def) {
    bool status = true;

    Symbol_collection new_sym_coll = block->symbol_collection;

    vec_append(&a_main, &env->ancesters, &new_sym_coll);

    Tast_stmt_vec new_tasts = {0};

    Uast_def* redef_sym = NULL;
    if (!usymbol_do_add_defered(&redef_sym, env)) {
        try_set_msg_redefinition_of_symbol(env, redef_sym);
        status = false;
        goto error;
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Uast_stmt* curr_tast = vec_at(&block->children, idx);
        Tast_stmt* new_tast = NULL;
        if (try_set_stmt_types(env, &new_tast, curr_tast)) {
            assert(curr_tast);
            vec_append(&a_main, &new_tasts, new_tast);
        } else {
            status = false;
        }
    }

    if (is_directly_in_fun_def && (
        block->children.info.count < 1 ||
        vec_at(&block->children, block->children.info.count - 1)->type != UAST_RETURN
    )) {
        Uast_return* rtn_statement = uast_return_new(
            block->pos_end,
            uast_literal_wrap(util_uast_literal_new_from_strv(
                str_view_from_cstr(""), TOKEN_VOID, block->pos_end
            )),
            true
        );
        if (rtn_statement->pos.line == 0) {
            symbol_log(LOG_DEBUG, env);
            unreachable("");
        }
        Tast_stmt* new_rtn_statement = NULL;
        if (!try_set_stmt_types(env, &new_rtn_statement, uast_return_wrap(rtn_statement))) {
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
            if (!try_set_expr_types(env, &new_tast_, uast_expr_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_expr_wrap(new_tast_);
            return true;
        }
        case UAST_DEF: {
            Tast_def* new_tast_ = NULL;
            if (!try_set_def_types(env, &new_tast_, uast_def_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_def_wrap(new_tast_);
            return true;
        }
        case UAST_FOR_WITH_COND: {
            Tast_for_with_cond* new_tast_ = NULL;
            if (!try_set_for_with_cond_types(env, &new_tast_, uast_for_with_cond_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_for_with_cond_wrap(new_tast_);
            return true;
        }
        case UAST_ASSIGNMENT: {
            Tast_assignment* new_tast_ = NULL;
            if (!try_set_assignment_types(env, &new_tast_, uast_assignment_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_assignment_wrap(new_tast_);
            return true;
        }
        case UAST_RETURN: {
            Tast_return* new_rtn = NULL;
            if (!try_set_return_types(env, &new_rtn, uast_return_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_return_wrap(new_rtn);
            return true;
        }
        case UAST_FOR_RANGE: {
            Tast_for_range* new_for = NULL;
            if (!try_set_for_range_types(env, &new_for, uast_for_range_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_for_range_wrap(new_for);
            return true;
        }
        case UAST_BREAK:
            *new_tast = tast_break_wrap(tast_break_new(uast_break_unwrap(stmt)->pos));
            return true;
        case UAST_CONTINUE:
            *new_tast = tast_continue_wrap(tast_continue_new(uast_continue_unwrap(stmt)->pos));
            return true;
        case UAST_BLOCK: {
            assert(uast_block_unwrap(stmt)->pos_end.line > 0);
            Tast_block* new_for = NULL;
            if (!try_set_block_types(env, &new_for, uast_block_unwrap(stmt), false)) {
                return false;
            }
            *new_tast = tast_block_wrap(new_for);
            return true;
        }
        case UAST_IF_ELSE_CHAIN: {
            Tast_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(env, &new_for, uast_if_else_chain_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_if_else_chain_wrap(new_for);
            return true;
        }
        case UAST_SWITCH: {
            Tast_if_else_chain* new_if_else = NULL;
            if (!try_set_switch_types(env, &new_if_else, uast_switch_unwrap(stmt))) {
                return false;
            }
            *new_tast = tast_if_else_chain_wrap(new_if_else);
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
