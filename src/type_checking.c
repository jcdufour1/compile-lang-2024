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
#include <msg_todo.h>
#include <token_type_to_operator_type.h>
#include <tast_clone.h>
#include <uast_clone.h>
#include <lang_type_hand_written.h>
#include <ulang_type_serialize.h>
#include <lang_type_print.h>
#include <ulang_type_print.h>
#include <ulang_type_from_uast_function_decl.h>
#include <resolve_generics.h>

// result is rounded up
static int64_t log2_int64_t(int64_t num) {
    if (num <= 0) {
        todo();
    }

    int64_t reference = 1;
    for (unsigned int power = 0; power < 64; power++) {
        if (num <= reference) {
            return power;
        }

        reference *= 2;
    }
    unreachable("");
}

static int64_t bit_width_needed_unsigned(int64_t num) {
    if (num == 0) {
        return 1;
    }
    if (num < 0) {
        return log2_int64_t(-num);
    }
    return log2_int64_t(num + 1);
}

static int64_t bit_width_needed_signed(int64_t num) {
    if (num < 0) {
        return log2_int64_t(-num) + 1;
    }
    return log2_int64_t(num + 1) + 1;
}

static Tast_expr* auto_deref_to_0(Env* env, Tast_expr* expr) {
    int16_t prev_pointer_depth = lang_type_get_pointer_depth(tast_expr_get_lang_type(expr));
    while (lang_type_get_pointer_depth(tast_expr_get_lang_type(expr)) > 0) {
        unwrap(try_set_unary_types_finish(env, &expr, expr, tast_expr_get_pos(expr), UNARY_DEREF, (Lang_type) {0}));
        assert(lang_type_get_pointer_depth(tast_expr_get_lang_type(expr)) + 1 == prev_pointer_depth);
        prev_pointer_depth = lang_type_get_pointer_depth(tast_expr_get_lang_type(expr));
    }
    return expr;
}

const Uast_function_decl* get_parent_function_decl_const(Env* env) {
    Uast_def* def = NULL;
    unwrap(env->name_parent_function.count > 0 && "no parent function here");
    unwrap(usymbol_lookup(&def, env, env->name_parent_function));
    if (def->type != UAST_FUNCTION_DECL) {
        unreachable(TAST_FMT, uast_def_print(def));
    }
    return uast_function_decl_unwrap(def);
}

const Uast_lang_type* get_parent_function_return_type(Env* env) {
    return get_parent_function_decl_const(env)->return_type;
}

static bool can_be_implicitly_converted(Lang_type dest, Lang_type src, bool src_is_zero, bool implicit_pointer_depth);

static bool can_be_implicitly_converted_lang_type_atom(Lang_type_atom dest, Lang_type_atom src, bool src_is_zero, bool implicit_pointer_depth) {
    if (!implicit_pointer_depth) {
        if (src.pointer_depth != dest.pointer_depth) {
            return false;
        }
    }

    if (!lang_type_atom_is_number(dest) || !lang_type_atom_is_number(src)) {
        return lang_type_atom_is_equal(dest, src);
    }

    if (lang_type_atom_is_unsigned(dest) && lang_type_atom_is_signed(src)) {
        return false;
    }

    if (src_is_zero) {
        return true;
    }

    int32_t dest_bit_width = i_lang_type_atom_to_bit_width(dest);
    int32_t src_bit_width = i_lang_type_atom_to_bit_width(src);

    if (lang_type_atom_is_signed(dest)) {
        unwrap(dest_bit_width > 0);
        dest_bit_width--;
    }
    if (lang_type_atom_is_signed(src)) {
        unwrap(src_bit_width > 0);
        src_bit_width--;
    }

    return dest_bit_width >= src_bit_width;
}

static bool can_be_implicitly_converted_tuple(Lang_type_tuple dest, Lang_type_tuple src, bool implicit_pointer_depth) {
    (void) implicit_pointer_depth;
    if (dest.lang_types.info.count != src.lang_types.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < dest.lang_types.info.count; idx++) {
        if (!can_be_implicitly_converted(
            vec_at(&dest.lang_types, idx), vec_at(&src.lang_types, idx), false, implicit_pointer_depth
        )) {
            return false;
        }
    }

    return true;
}

static bool can_be_implicitly_converted_fn(Lang_type_fn dest, Lang_type_fn src, bool implicit_pointer_depth) {
    if (!can_be_implicitly_converted_tuple(dest.params, src.params, implicit_pointer_depth)) {
        return false;
    }
    return can_be_implicitly_converted(*dest.return_type, *src.return_type, false, implicit_pointer_depth);
}

static bool can_be_implicitly_converted(Lang_type dest, Lang_type src, bool src_is_zero, bool implicit_pointer_depth) {
    if (dest.type != src.type) {
        return false;
    }

    switch (dest.type) {
        case LANG_TYPE_FN:
            return can_be_implicitly_converted_fn(lang_type_fn_const_unwrap(dest), lang_type_fn_const_unwrap(src), implicit_pointer_depth);
        case LANG_TYPE_TUPLE:
            return can_be_implicitly_converted_tuple(lang_type_tuple_const_unwrap(dest), lang_type_tuple_const_unwrap(src), implicit_pointer_depth);
        case LANG_TYPE_PRIMITIVE:
            return can_be_implicitly_converted_lang_type_atom(
                lang_type_primitive_get_atom(lang_type_primitive_const_unwrap(dest)),
                lang_type_primitive_get_atom(lang_type_primitive_const_unwrap(src)),
                src_is_zero,
                implicit_pointer_depth
            );
        case LANG_TYPE_SUM:
            return can_be_implicitly_converted_lang_type_atom(lang_type_sum_const_unwrap(dest).atom, lang_type_sum_const_unwrap(src).atom, false, implicit_pointer_depth);
        case LANG_TYPE_STRUCT:
            return can_be_implicitly_converted_lang_type_atom(lang_type_struct_const_unwrap(dest).atom, lang_type_struct_const_unwrap(src).atom, false, implicit_pointer_depth);
        case LANG_TYPE_RAW_UNION:
            return can_be_implicitly_converted_lang_type_atom(lang_type_raw_union_const_unwrap(dest).atom, lang_type_raw_union_const_unwrap(src).atom, false, implicit_pointer_depth);
        case LANG_TYPE_ENUM:
            return can_be_implicitly_converted_lang_type_atom(lang_type_enum_const_unwrap(dest).atom, lang_type_enum_const_unwrap(src).atom, false, implicit_pointer_depth);
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
    Env* env,
    const Tast_expr* argument,
    const Uast_variable_def* corres_param
) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env->file_text, tast_expr_get_pos(argument), 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(argument)), 
        str_view_print(corres_param->name),
        ulang_type_print(LANG_TYPE_MODE_MSG, corres_param->lang_type)
    );
    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, corres_param->pos,
        "corresponding parameter `"STR_VIEW_FMT"` defined here\n",
        str_view_print(corres_param->name)
    );
}

static void msg_invalid_count_function_args_internal(
    const char* file,
    int line,
    Env* env,
    const Uast_function_call* fun_call,
    const Uast_function_decl* fun_decl,
    size_t min_args,
    size_t max_args
) {
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
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env->file_text, fun_call->pos,
        STR_VIEW_FMT, str_view_print(string_to_strv(message))
    );

    msg_internal(
        file, line, LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, uast_function_decl_get_pos(fun_decl),
        "function `"STR_VIEW_FMT"` defined here\n", str_view_print(fun_decl->name)
    );
}

#define msg_invalid_function_arg(env, argument, corres_param) \
    msg_invalid_function_arg_internal(__FILE__, __LINE__, env, argument, corres_param)

#define msg_invalid_count_function_args(env, fun_call, fun_decl, min_args, max_args) \
    msg_invalid_count_function_args_internal(__FILE__, __LINE__, env, fun_call, fun_decl, min_args, max_args)

static void msg_invalid_return_type_internal(const char* file, int line, Env* env, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    const Uast_function_decl* fun_decl = get_parent_function_decl_const(env);
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env->file_text, pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            ulang_type_print(LANG_TYPE_MODE_MSG, fun_decl->return_type->lang_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env->file_text, pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(child)), 
            ulang_type_print(LANG_TYPE_MODE_MSG, fun_decl->return_type->lang_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, fun_decl->return_type->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        ulang_type_print(LANG_TYPE_MODE_MSG, fun_decl->return_type->lang_type)
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
    const Env* env,
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    bool src_is_zero,
    Tast_expr* src
) {
    log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, dest_lang_type));
    log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, tast_expr_get_lang_type(src)));
    if (lang_type_is_equal(dest_lang_type, tast_expr_get_lang_type(src))) {
        *new_src = src;
        return CHECK_ASSIGN_OK;
    }

    if (can_be_implicitly_converted(dest_lang_type, tast_expr_get_lang_type(src), src_is_zero, false)) {
        if (src->type == TAST_LITERAL) {
            *new_src = src;
            tast_expr_set_lang_type(*new_src, dest_lang_type);
            return CHECK_ASSIGN_OK;
        }
        msg_todo(env, "non literal implicit conversion", tast_expr_get_pos(src));
        return CHECK_ASSIGN_ERROR;
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
    log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, dest_lang_type));
    if (src->type == UAST_STRUCT_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        if (!try_set_struct_literal_assignment_types(
            env, &new_src_, dest_lang_type, uast_struct_literal_unwrap(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_expr_unwrap(new_src_);
    } else if (src->type == UAST_TUPLE) {
        Tast_tuple* new_src_ = NULL;
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

    bool src_is_zero = false;
    if (src->type == UAST_LITERAL && uast_literal_unwrap(src)->type == UAST_NUMBER && uast_number_unwrap(uast_literal_unwrap(src))->data == 0) {
        src_is_zero = true;
    }

    return check_generic_assignment_finish(env, new_src, dest_lang_type, src_is_zero, *new_src);
}

Tast_literal* try_set_literal_types(Uast_literal* literal) {
    switch (literal->type) {
        case UAST_STRING: {
            Uast_string* old_string = uast_string_unwrap(literal);
            return tast_string_wrap(tast_string_new(
                old_string->pos,
                old_string->data,
                old_string->name
            ));
        }
        case UAST_NUMBER: {
            Uast_number* old_number = uast_number_unwrap(literal);
            if (old_number->data < 0) {
                int64_t bit_width = bit_width_needed_signed(old_number->data);
                return tast_number_wrap(tast_number_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(bit_width, 0))
                )));
            } else {
                int64_t bit_width = bit_width_needed_unsigned(old_number->data);
                return tast_number_wrap(tast_number_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(bit_width, 0))
                )));
            }
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
                old_char->data
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
bool try_set_symbol_types(Env* env, Tast_expr** new_tast, Uast_symbol* sym_untyped) {
    Uast_def* sym_def = NULL;
    if (!usymbol_lookup(&sym_def, env, sym_untyped->name)) {
        msg_undefined_symbol(env->file_text, uast_expr_wrap(uast_symbol_wrap(sym_untyped)));
        return false;
    }

    switch (sym_def->type) {
        case UAST_FUNCTION_DECL:
            *new_tast = tast_literal_wrap(tast_function_lit_wrap(tast_function_lit_new(
                sym_untyped->pos,
                sym_untyped->name,
                lang_type_from_ulang_type(env, ulang_type_from_uast_function_decl(uast_function_decl_unwrap(sym_def)))
            )));
            return true;
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_STRUCT_DEF:
            // fallthrough
        case UAST_SUM_DEF:
            // fallthrough
        case UAST_ENUM_DEF:
            // fallthrough
        case UAST_RAW_UNION_DEF:
            // fallthrough
        case UAST_PRIMITIVE_DEF:
            // fallthrough
        case UAST_LITERAL_DEF:
            // fallthrough
        case UAST_VARIABLE_DEF: {
            log(LOG_DEBUG, TAST_FMT, uast_symbol_print(sym_untyped));
            Lang_type lang_type = uast_def_get_lang_type(env, sym_def, sym_untyped->generic_args);
            Sym_typed_base new_base = {.lang_type = lang_type, .name = sym_untyped->name};
            Tast_symbol* sym_typed = tast_symbol_new(sym_untyped->pos, new_base);
            *new_tast = tast_symbol_wrap(sym_typed);
            return true;
        }
        case UAST_GENERIC_PARAM:
            unreachable("cannot set symbol of template parameter here");
    }
    unreachable("");
}

static int64_t precalulate_number_internal(int64_t lhs_val, int64_t rhs_val, BINARY_TYPE token_type) {
    switch (token_type) {
        case BINARY_SINGLE_EQUAL:
            unreachable("");
        case BINARY_ADD:
            return lhs_val + rhs_val;
        case BINARY_SUB:
            return lhs_val - rhs_val;
        case BINARY_MULTIPLY:
            return lhs_val*rhs_val;
        case BINARY_DIVIDE:
            return lhs_val/rhs_val;
        case BINARY_LESS_THAN:
            return lhs_val < rhs_val ? 1 : 0;
        case BINARY_GREATER_THAN:
            return lhs_val > rhs_val ? 1 : 0;
        case BINARY_DOUBLE_EQUAL:
            return lhs_val == rhs_val ? 1 : 0;
        case BINARY_NOT_EQUAL:
            return lhs_val != rhs_val ? 1 : 0;
        case BINARY_LESS_OR_EQUAL:
            return lhs_val <= rhs_val ? 1 : 0;
        case BINARY_GREATER_OR_EQUAL:
            return lhs_val >= rhs_val ? 1 : 0;
        case BINARY_MODULO:
            return lhs_val%rhs_val;
        case BINARY_BITWISE_XOR:
            return lhs_val^rhs_val;
        case BINARY_BITWISE_AND:
            return lhs_val&rhs_val;
        case BINARY_BITWISE_OR:
            return lhs_val|rhs_val;
        case BINARY_LOGICAL_AND:
            return lhs_val && rhs_val;
        case BINARY_LOGICAL_OR:
            return lhs_val || rhs_val;
        case BINARY_SHIFT_LEFT:
            return lhs_val<<rhs_val;
        case BINARY_SHIFT_RIGHT:
            return lhs_val>>rhs_val;
    }
    unreachable("");
}

static Tast_literal* precalulate_number(
    const Tast_number* lhs,
    const Tast_number* rhs,
    BINARY_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static Tast_literal* precalulate_char(
    const Tast_char* lhs,
    const Tast_char* rhs,
    BINARY_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

bool try_set_binary_types_finish(Env* env, Tast_expr** new_tast, Tast_expr* new_lhs, Tast_expr* new_rhs, Pos oper_pos, BINARY_TYPE oper_token_type) {
    if (!lang_type_is_equal(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs))) {
        if (can_be_implicitly_converted(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs), new_rhs->type == TAST_LITERAL && tast_literal_unwrap(new_rhs)->type == TAST_NUMBER && tast_number_unwrap(tast_literal_unwrap(new_rhs))->data == 0, true)) {
            if (new_rhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_rhs), tast_expr_get_lang_type(new_lhs));
            } else {
                unwrap(try_set_unary_types_finish(env, &new_rhs, new_rhs, tast_expr_get_pos(new_rhs), UNARY_UNSAFE_CAST, tast_expr_get_lang_type(new_lhs)));
            }
        } else if (can_be_implicitly_converted(tast_expr_get_lang_type(new_rhs), tast_expr_get_lang_type(new_lhs), new_rhs->type == TAST_LITERAL && tast_literal_unwrap(new_rhs)->type == TAST_NUMBER && tast_number_unwrap(tast_literal_unwrap(new_rhs))->data == 0, true)) {
            if (new_lhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(env, new_lhs);
                new_rhs = auto_deref_to_0(env, new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_lhs), tast_expr_get_lang_type(new_rhs));
            } else {
                unwrap(try_set_unary_types_finish(env, &new_lhs, new_lhs, tast_expr_get_pos(new_lhs), UNARY_UNSAFE_CAST, tast_expr_get_lang_type(new_rhs)));
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env->file_text, oper_pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_lhs)),
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs))
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
        Lang_type u1_lang_type = lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
            lang_type_unsigned_int_new(1, 0)
        ));

        switch (oper_token_type) {
            case BINARY_SHIFT_LEFT:
                // fallthrough
            case BINARY_SHIFT_RIGHT:
                // fallthrough
            case BINARY_BITWISE_XOR:
                // fallthrough
            case BINARY_BITWISE_AND:
                // fallthrough
            case BINARY_BITWISE_OR:
                // fallthrough
            case BINARY_MODULO:
                // fallthrough
            case BINARY_DIVIDE:
                // fallthrough
            case BINARY_MULTIPLY:
                // fallthrough
            case BINARY_SUB:
                // fallthrough
            case BINARY_ADD:
                *new_tast = tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                    oper_pos,
                    new_lhs,
                    new_rhs,
                    oper_token_type,
                    tast_expr_get_lang_type(new_lhs)
                )));
                break;
            case BINARY_LESS_THAN:
                // fallthrough
            case BINARY_LESS_OR_EQUAL:
                // fallthrough
            case BINARY_GREATER_OR_EQUAL:
                // fallthrough
            case BINARY_GREATER_THAN:
                // fallthrough
            case BINARY_NOT_EQUAL:
                // fallthrough
            case BINARY_DOUBLE_EQUAL:
                *new_tast = tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                    oper_pos,
                    new_lhs,
                    new_rhs,
                    oper_token_type,
                    u1_lang_type
                )));
                break;
            case BINARY_LOGICAL_OR:
                // fallthrough
            case BINARY_LOGICAL_AND: {
                Tast_literal* new_lit_lhs = util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, tast_expr_get_pos(new_lhs));
                tast_literal_set_lang_type(new_lit_lhs, tast_expr_get_lang_type(new_lhs));

                Tast_literal* new_lit_rhs = util_tast_literal_new_from_int64_t(0, TOKEN_INT_LITERAL, tast_expr_get_pos(new_rhs));
                tast_literal_set_lang_type(new_lit_rhs, tast_expr_get_lang_type(new_rhs));

                *new_tast = tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                    oper_pos,
                    tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                        tast_expr_get_pos(new_lhs),
                        tast_literal_wrap(new_lit_lhs),
                        new_lhs,
                        BINARY_NOT_EQUAL,
                        u1_lang_type
                    ))),
                    tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                        tast_expr_get_pos(new_rhs),
                        tast_literal_wrap(new_lit_rhs),
                        new_rhs,
                        BINARY_NOT_EQUAL,
                        u1_lang_type
                    ))),
                    oper_token_type,
                    u1_lang_type
                )));
                break;
            }
            default:
                unreachable(STR_VIEW_FMT, binary_type_print(oper_token_type));
        }

    }

    assert(lang_type_get_str(tast_expr_get_lang_type(*new_tast)).count > 0);
    assert(*new_tast);
    return true;
}

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Tast_expr** new_tast, Uast_binary* operator) {
    Tast_stmt* new_lhs;
    switch (try_set_stmt_types(env, &new_lhs, operator->lhs)) {
        case STMT_OK:
            break;
        case STMT_NO_STMT:
            unreachable("");
        case STMT_ERROR:
            return false;
    }
    assert(new_lhs);

    Tast_expr* new_rhs = NULL;
    if (operator->token_type == BINARY_SINGLE_EQUAL) {
        switch (check_generic_assignment(env, &new_rhs, tast_stmt_get_lang_type(new_lhs), operator->rhs, operator->pos)) {
            case CHECK_ASSIGN_OK:
                *new_tast = tast_assignment_wrap(tast_assignment_new(operator->pos, new_lhs, new_rhs));
                return true;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env->file_text,
                    operator->pos,
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_stmt_get_lang_type(new_lhs))
                );
                return false;
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                unreachable("");
        }
    }

    if (!try_set_expr_types(env, &new_rhs, operator->rhs)) {
        return false;
    }

    if (operator->lhs->type != UAST_EXPR) {
        // TODO: expected failure case
        unreachable("not-expr cannot be lhs of this operator");
    }

    log(LOG_DEBUG, TAST_FMT"\n", binary_type_print(operator->token_type));
    log(LOG_DEBUG, TAST_FMT, uast_binary_print(operator));
    return try_set_binary_types_finish(
        env,
        new_tast,
        tast_expr_unwrap(new_lhs),
        new_rhs,
        operator->pos,
        operator->token_type
    );
}

bool try_set_unary_types_finish(
    Env* env,
    Tast_expr** new_tast,
    Tast_expr* new_child,
    Pos unary_pos,
    UNARY_TYPE unary_token_type,
    Lang_type cast_to
) {
    Lang_type new_lang_type = {0};
    switch (unary_token_type) {
        case UNARY_DEREF:
            new_lang_type = tast_expr_get_lang_type(new_child);
            if (lang_type_get_pointer_depth(new_lang_type) <= 0) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env->file_text, unary_pos,
                    "derefencing a type that is not a pointer\n"
                );
                return false;
            }
            lang_type_set_pointer_depth(&new_lang_type, lang_type_get_pointer_depth(new_lang_type) - 1);
            *new_tast = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                unary_pos,
                new_child,
                unary_token_type,
                new_lang_type
            )));
            return true;
        case UNARY_REFER:
            new_lang_type = tast_expr_get_lang_type(new_child);
            lang_type_set_pointer_depth(&new_lang_type, lang_type_get_pointer_depth(new_lang_type) + 1);
            *new_tast = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                unary_pos,
                new_child,
                unary_token_type,
                new_lang_type
            )));
            return true;
        case UNARY_UNSAFE_CAST:
            new_lang_type = cast_to;
            assert(lang_type_get_str(cast_to).count > 0);
            if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number_like(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number(tast_expr_get_lang_type(new_child)) && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0) {
            } else {
                log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_child)));
                log(LOG_DEBUG, BOOL_FMT, bool_print(lang_type_is_number(tast_expr_get_lang_type(new_child))));
                log(LOG_DEBUG, "%d\n", lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)));
                log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_child));
                todo();
            }
            *new_tast = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                unary_pos,
                new_child,
                unary_token_type,
                new_lang_type
            )));
            return true;
        case UNARY_NOT:
            new_lang_type = tast_expr_get_lang_type(new_child);
            if (!lang_type_is_number(new_lang_type)) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_UNARY_MISMATCHED_TYPES, env->file_text, unary_pos,
                    "`"LANG_TYPE_FMT"` is not valid operand to logical not operation\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, new_lang_type)
                );
                return false;
            }
            *new_tast = tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                unary_pos,
                new_child,
                tast_literal_wrap(util_tast_literal_new_from_int64_t(
                    0,
                    TOKEN_INT_LITERAL,
                    unary_pos
                )),
                BINARY_DOUBLE_EQUAL,
                new_lang_type // TODO: make this u1?
            )));
            return true;
    }
    unreachable("");

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
            uast_tuple_print(tuple), lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type)
        );
        msg(
            LOG_NOTE, EXPECT_FAIL_NONE, env->file_text,
            uast_tuple_get_pos(tuple),
            "tuple `"UAST_FMT"` has %zu elements, but type `"LANG_TYPE_FMT" has %zu elements`\n",
            uast_tuple_print(tuple), tuple->members.info.count,
            lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type), lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count
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
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_memb)),
                    lang_type_print(LANG_TYPE_MODE_MSG, curr_dest)
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
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env->file_text,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        case LANG_TYPE_SUM:
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_SUM, env->file_text,
                assign_pos, "struct literal cannot be assigned to sum\n"
            );
            return false;
        case LANG_TYPE_PRIMITIVE:
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_PRIMITIVE, env->file_text,
                assign_pos, "struct literal cannot be assigned to primitive type `"LANG_TYPE_FMT"`\n",
                lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type)
            );
            return false;
        default:
            unreachable("");
    }
    Uast_def* struct_def_ = NULL;
    unwrap(usymbol_lookup(&struct_def_, env, lang_type_struct_const_unwrap(dest_lang_type).atom.str));
    Uast_struct_def* struct_def = uast_struct_def_unwrap(struct_def_);
    
    Tast_expr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        Uast_variable_def* memb_sym_def = vec_at(&struct_def->base.members, idx);
        log(LOG_DEBUG, TAST_FMT, uast_stmt_print(vec_at(&struct_literal->members, idx)));
        Uast_binary* assign_memb_sym = uast_binary_unwrap(uast_operator_unwrap(uast_expr_unwrap(vec_at(&struct_literal->members, idx))));
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
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                    ulang_type_print(LANG_TYPE_MODE_MSG, memb_sym_def->lang_type)
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
            //    LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, lhs_var_def->pos,
            //    "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
            //    str_view_print(lhs_var_def->name), lang_type_print(lhs_var_def->lang_type)
            //);
            //msg(
            //    LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, memb_sym_def->pos,
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

    unwrap(symbol_add(env, tast_literal_def_wrap(tast_struct_lit_def_wrap(new_def))));
    return true;
}

bool try_set_expr_types(Env* env, Tast_expr** new_tast, Uast_expr* uast) {
    switch (uast->type) {
        case UAST_LITERAL: {
            *new_tast = tast_literal_wrap(try_set_literal_types(uast_literal_unwrap(uast)));
            return true;
        }
        case UAST_SYMBOL:
            log(LOG_DEBUG, TAST_FMT, uast_expr_print(uast));
            if (!try_set_symbol_types(env, new_tast, uast_symbol_unwrap(uast))) {
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

STMT_STATUS try_set_def_types(Env* env, Tast_def** new_tast, Uast_def* uast) {
    switch (uast->type) {
        case UAST_VARIABLE_DEF: {
            Tast_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(env, &new_def, uast_variable_def_unwrap(uast), true, false)) {
                return STMT_ERROR;
            }
            *new_tast = tast_variable_def_wrap(new_def);
            return STMT_OK;
        }
        case UAST_FUNCTION_DECL: {
            Tast_function_decl* dummy = NULL;
            if (!try_set_function_decl_types(env, &dummy, uast_function_decl_unwrap(uast), false)) {
                assert(error_count > 0);
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_FUNCTION_DEF: {
            if (!try_set_function_def_types(env, uast_function_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_STRUCT_DEF: {
            return STMT_NO_STMT;
        }
        case UAST_RAW_UNION_DEF: {
            return STMT_NO_STMT;
        }
        case UAST_ENUM_DEF: {
            return STMT_NO_STMT;
        }
        case UAST_PRIMITIVE_DEF: {
            if (!try_set_primitive_def_types(env, uast_primitive_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_LITERAL_DEF: {
            if (!try_set_literal_def_types(env, uast_literal_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_SUM_DEF: {
            return STMT_NO_STMT;
        }
        case UAST_GENERIC_PARAM: {
            todo();
        }
    }
    unreachable("");
}

// TODO: remove this function and Uast_assignment
bool try_set_assignment_types(Env* env, Tast_assignment** new_assign, Uast_assignment* assignment) {
    Tast_stmt* new_lhs = NULL;
    switch (try_set_stmt_types(env, &new_lhs, assignment->lhs)) { 
        case STMT_OK:
            break;
        case STMT_NO_STMT:
            unreachable(TAST_FMT, uast_assignment_print(assignment));
        case STMT_ERROR:
            return false;
        default:
            unreachable("");
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
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                lang_type_print(LANG_TYPE_MODE_MSG, tast_stmt_get_lang_type(new_lhs))
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
                msg(LOG_DEBUG, EXPECT_FAIL_NONE, env->file_text, uast_expr_get_pos(vec_at(&args, 0)), "no arguments expected here\n");
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
                uast_expr_get_name(vec_at(&args, 0))
            );
            usymbol_add_defer(env, uast_variable_def_wrap(new_def));

            Uast_assignment* new_assign = uast_assignment_new(
                new_def->pos,
                uast_def_wrap(uast_variable_def_wrap(new_def)),
                uast_sum_access_wrap(uast_sum_access_new(
                    new_def->pos,
                    sum_case->tag,
                    lang_type_from_ulang_type(env, new_def->lang_type),
                    uast_symbol_wrap(uast_symbol_new(
                        new_def->pos,
                        uast_expr_get_name(env->parent_of_operand),
                        (Ulang_type_vec) {0}
                    ))
                ))
            );

            vec_append(&a_main, &env->switch_case_defer_add_sum_case_part, uast_assignment_wrap(new_assign));

            *new_case = sum_case;
            return true;
        }
    }
}

static Uast_function_decl* uast_function_decl_from_ulang_type_fn(Env* env, Ulang_type_fn lang_type, Pos pos) {
    Str_view name = serialize_ulang_type(ulang_type_fn_const_wrap(lang_type));
    Uast_def* fun_decl_ = NULL;
    if (usym_tbl_lookup(&fun_decl_, &vec_at(&env->ancesters, 0)->usymbol_table, name)) {
        return uast_function_decl_unwrap(fun_decl_);
    }

    Uast_param_vec params = {0};
    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        vec_append(&a_main, &params, uast_param_new(
            pos,
            uast_variable_def_new(pos, vec_at(&lang_type.params.ulang_types, idx), util_literal_name_new()),
            false, // TODO: test case for optional in function callback
            false, // TODO: test case for variadic in function callback
            NULL
        ));
    }

    Uast_function_decl* fun_decl = uast_function_decl_new(
        pos,
        uast_function_params_new(pos, params),
        uast_lang_type_new(pos, *lang_type.return_type),
        name
    );
    usym_tbl_add(&vec_at(&env->ancesters, 0)->usymbol_table, uast_function_decl_wrap(fun_decl));
    log(LOG_DEBUG, TAST_FMT"\n", uast_function_decl_print(fun_decl));
    return fun_decl;

    todo();
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
                // TODO: expected failure case
                todo();
            }
            Tast_sum_callee* sum_callee = tast_sum_callee_unwrap(new_callee);

            Uast_def* sum_def_ = NULL;
            unwrap(usymbol_lookup(&sum_def_, env, lang_type_get_str(sum_callee->sum_lang_type)));
            Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);

            Tast_expr* new_item = NULL;
            switch (check_generic_assignment(
                env,
                &new_item,
                lang_type_from_ulang_type(env, vec_at(&sum_def->base.members, (size_t)sum_callee->tag->data)->lang_type),
                vec_at(&fun_call->args, 0),
                uast_expr_get_pos(vec_at(&fun_call->args, 0))
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg(
                        LOG_ERROR, EXPECT_FAIL_SUM_LIT_INVALID_ARG, env->file_text, tast_expr_get_pos(new_item),
                        "cannot assign "TAST_FMT" of type `"LANG_TYPE_FMT"` to '"LANG_TYPE_FMT"`\n", 
                        tast_expr_print(new_item),
                        lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_item)), 
                        lang_type_print(LANG_TYPE_MODE_MSG, lang_type_from_ulang_type(
                            env, vec_at(&sum_def->base.members, (size_t)sum_callee->tag->data)->lang_type
                        ))
                   );
                   break;
                case CHECK_ASSIGN_ERROR:
                    todo();
                default:
                    unreachable("");
            }

            // TODO: set tag size based on target platform
            sum_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0)));

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
                // TODO: expected failure case
                todo();
            }
            Tast_sum_case* new_case = NULL;
            if (!try_set_function_call_types_sum_case(env, &new_case, fun_call->args, tast_sum_case_unwrap(new_callee))) {
                return false;
            }
            *new_call = tast_sum_case_wrap(new_case);
            return true;
        }
        case TAST_LITERAL: {
            if (tast_literal_unwrap(new_callee)->type == TAST_SUM_LIT) {
                Tast_sum_lit* sum_lit = tast_sum_lit_unwrap(tast_literal_unwrap(new_callee));
                if (fun_call->args.info.count != 0) {
                    Uast_def* sum_def_ = NULL;
                    unwrap(usymbol_lookup(&sum_def_, env, lang_type_get_str(sum_lit->sum_lang_type)));
                    Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);
                    msg(
                        LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env->file_text, fun_call->pos,
                        "cannot assign argument to varient `"LANG_TYPE_FMT"."LANG_TYPE_FMT"`, because inner type is void\n",
                        lang_type_print(LANG_TYPE_MODE_MSG, sum_lit->sum_lang_type), str_view_print(vec_at(&sum_def->base.members, (size_t)sum_lit->tag->data)->name)
                    );
                    return false;
                }
                *new_call = new_callee;
                return true;
            } else {
                unwrap(usymbol_lookup(&fun_def, env, tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name));
                break;
            }
        }
        default:
            unreachable(TAST_FMT, tast_expr_print(new_callee));
    }

    Uast_function_decl* fun_decl;
    switch (fun_def->type) {
        case UAST_FUNCTION_DEF:
            todo();
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
        case UAST_VARIABLE_DEF: {
            if (uast_variable_def_unwrap(fun_def)->lang_type.type != ULANG_TYPE_FN) {
                todo();
            }
            fun_decl = uast_function_decl_from_ulang_type_fn(
                env,
                ulang_type_fn_const_unwrap(uast_variable_def_unwrap(fun_def)->lang_type),
                uast_variable_def_unwrap(fun_def)->pos
            );
            log(LOG_DEBUG, TAST_FMT, uast_function_decl_print(fun_decl));
            break;
        }
        default:
            unreachable(TAST_FMT, uast_def_print(fun_def));
    }
    Tast_lang_type* fun_rtn_type = NULL;
    if (!try_types_set_lang_type(env, &fun_rtn_type, fun_decl->return_type)) {
        return false;
    }

    Uast_function_params* params = fun_decl->params;

    Tast_expr_vec new_args = {0};
    bool is_variadic = false;
    // TODO: consider case of optional arguments and variadic arguments being used in same function
    for (size_t param_idx = 0; param_idx < params->params.info.count; param_idx++) {
        Uast_param* param = vec_at(&params->params, param_idx);
        Uast_expr* corres_arg = NULL;
        if (param->is_variadic) {
            is_variadic = true;
        }

        if (fun_call->args.info.count > param_idx) {
            corres_arg = vec_at(&fun_call->args, param_idx);
        } else if (is_variadic) {
        } else if (param->is_optional) {
            unwrap(!is_variadic && "cannot mix variadic args and optional args right now");
            // TODO: expected failure case for invalid optional_default
            corres_arg = uast_expr_clone(param->optional_default);
        } else {
            msg_invalid_count_function_args(env, fun_call, fun_decl, param_idx + 1, param_idx + 1);
            goto error;
        }

        Tast_expr* new_arg = NULL;

        if (lang_type_is_equal(lang_type_from_ulang_type(env, param->base->lang_type), lang_type_primitive_const_wrap(lang_type_any_const_wrap(lang_type_any_new(lang_type_atom_new_from_cstr("any", 0)))))) {
            if (param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                for (size_t arg_idx = param_idx; arg_idx < fun_call->args.info.count; arg_idx++) {
                    Tast_expr* new_sub_arg = NULL;
                    if (!try_set_expr_types(env, &new_sub_arg, vec_at(&fun_call->args, arg_idx))) {
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
                lang_type_from_ulang_type(env, param->base->lang_type),
                corres_arg,
                uast_expr_get_pos(corres_arg)
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg_invalid_function_arg(env, new_arg, param->base);
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

    if (!is_variadic && fun_call->args.info.count > params->params.info.count) {
        msg_invalid_count_function_args(env, fun_call, fun_decl, params->params.info.count, params->params.info.count);
        goto error;
    }

    *new_call = tast_function_call_wrap(tast_function_call_new(
        fun_call->pos,
        new_args,
        new_callee,
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

static void msg_invalid_struct_member(
    Env* env,
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
    Env* env,
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
    Env* env,
    Tast_stmt** new_tast,
    Uast_member_access* access,
    Ustruct_def_base def_base,
    Tast_expr* new_callee
) {
    Uast_variable_def* member_def = NULL;
    if (!uast_try_get_member_def(&member_def, &def_base, access->member_name)) {
        msg_invalid_struct_member(env, def_base, access);
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
        case PARENT_OF_CASE: {
            Uast_variable_def* member_def = NULL;
            log(LOG_DEBUG, TAST_FMT, uast_sum_def_print(sum_def));
            if (!uast_try_get_member_def(&member_def, &sum_def->base, access->member_name)) {
                // TODO: expected failure case
                log(LOG_DEBUG, TAST_FMT, uast_member_access_print(access));
                todo();
                //msg_invalid_enum_member(env, enum_def->base, access);
                //return false;
            }

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
        }
        case PARENT_OF_ASSIGN_RHS: {
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

            Tast_sum_callee* new_callee = tast_sum_callee_new(
                access->pos,
                new_tag,
                lang_type_sum_const_wrap(lang_type_sum_new(lang_type_atom_new(sum_def->base.name, 0)))
            );

            if (new_tag->lang_type.type != LANG_TYPE_VOID) {
                *new_tast = tast_expr_wrap(tast_sum_callee_wrap(new_callee));
                return true;
            }

            new_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0)));

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
        case UAST_PRIMITIVE_DEF:
            // TODO: expected failure case
            unreachable("trying to access primitive defintion member");
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
    if (lang_type_get_bit_width(tast_expr_get_lang_type(new_inner_index)) < 64) {
        unwrap(try_set_unary_types_finish(
            env,
            &new_inner_index,
            new_inner_index,
            tast_expr_get_pos(new_inner_index),
            UNARY_UNSAFE_CAST,
            lang_type_primitive_const_wrap(
                lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(64, 0))
            )
        ));
    } else {
        unreachable("");
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

bool try_set_primitive_def_types(Env* env, Uast_primitive_def* tast) {
    (void) env;
    unwrap(symbol_add(env, tast_primitive_def_wrap(tast_primitive_def_new(tast->pos, tast->lang_type))));
    return true;
}

bool try_set_literal_def_types(Env* env, Uast_literal_def* tast) {
    (void) env;
    (void) tast;
    unreachable("");
}

static void msg_undefined_type_internal(
    const char* file,
    int line,
    Env* env,
    Pos pos,
    Ulang_type lang_type
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_text, pos,
        "type `"LANG_TYPE_FMT"` is not defined\n", ulang_type_print(LANG_TYPE_MODE_MSG, lang_type)
    );
}

#define msg_undefined_type(env, pos, lang_type) \
    msg_undefined_type_internal(__FILE__, __LINE__, env, pos, lang_type)

bool try_set_variable_def_types(
    Env* env,
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl,
    bool is_variadic
) {
    Lang_type new_lang_type = {0};
    log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, uast->lang_type));
    log(LOG_DEBUG, "%d\n", uast->lang_type.type);
    if (!try_lang_type_from_ulang_type(&new_lang_type, env, uast->lang_type, uast->pos)) {
        return false;
    }
    *new_tast = tast_variable_def_new(uast->pos, new_lang_type, is_variadic, uast->name);
    if (add_to_sym_tbl && !env->type_checking_is_in_struct_base_def) {
        unwrap(symbol_add(env, tast_variable_def_wrap(*new_tast)));
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
        assert(error_count > 0);
        return false;
    }

    Tast_lang_type* new_rtn_type = NULL;
    if (!try_types_set_lang_type(env, &new_rtn_type, decl->return_type)) {
        assert(error_count > 0);
        return false;
    }

    *new_tast = tast_function_decl_new(decl->pos, new_params, new_rtn_type, decl->name);

    Symbol_collection* top = vec_top(&env->ancesters);
    if (add_to_sym_tbl) {
        vec_rem_last(&env->ancesters);
    }
    unwrap(symbol_add(env, tast_function_decl_wrap(*new_tast)));
    if (add_to_sym_tbl) {
        vec_append(&a_main, &env->ancesters, top);
    }

    return true;
}

bool try_set_function_def_types(
    Env* env,
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

    env->name_parent_function = prev_par_fun;
    Tast_def* result = NULL;
    unwrap(symbol_lookup(&result, env, new_decl->name));
    symbol_update(env, tast_function_def_wrap(tast_function_def_new(def->pos, new_decl, new_body)));
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
        Uast_param* def = vec_at(&params->params, idx);

        Tast_variable_def* new_def = NULL;
        if (try_set_variable_def_types(env, &new_def, def->base, add_to_sym_tbl, def->is_variadic)) {
            vec_append(&a_main, &new_params, new_def);
        } else {
            status = false;
        }
    }

    *new_tast = tast_function_params_new(params->pos, new_params);
    return status;
}

// TODO: remove this function
static bool try_types_internal_set_lang_type(
    Env* env,
    Ulang_type lang_type,
    Pos pos
) {
    switch (lang_type.type) {
        case ULANG_TYPE_TUPLE: {
            for (size_t idx = 0; idx < ulang_type_tuple_const_unwrap(lang_type).ulang_types.info.count; idx++) {
                if (!try_types_internal_set_lang_type(env, vec_at_const(ulang_type_tuple_const_unwrap(lang_type).ulang_types, idx), pos)) {
                    assert(error_count > 0);
                    return false;
                }
            }
        }
        case ULANG_TYPE_REGULAR:
            lang_type_from_ulang_type(env, lang_type);
            return true;
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_REG_GENERIC:
            todo();
    }
    unreachable("");
}

bool try_types_set_lang_type(
    Env* env,
    Tast_lang_type** new_tast,
    Uast_lang_type* uast
) {
    if (!try_types_internal_set_lang_type(env, uast->lang_type, uast->pos)) {
        assert(error_count > 0);
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
    if (!try_set_variable_def_types(env, &new_var_def, uast->var_def, true, false)) {
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

static Exhaustive_data check_for_exhaustiveness_start(Env* env, Lang_type oper_lang_type) {
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
    unwrap(enum_def.members.info.count > 0);
    exhaustive_data.max_data = enum_def.members.info.count - 1;

    vec_reserve(&print_arena, &exhaustive_data.covered, exhaustive_data.max_data + 1);
    for (size_t idx = 0; idx < exhaustive_data.max_data + 1; idx++) {
        vec_append(&print_arena, &exhaustive_data.covered, false);
    }
    unwrap(exhaustive_data.covered.info.count == exhaustive_data.max_data + 1);

    return exhaustive_data;
}

static bool check_for_exhaustiveness_inner(
    Env* env,
    Exhaustive_data* exhaustive_data,
    const Tast_if* curr_if,
    bool is_default
) {
    if (is_default) {
        if (exhaustive_data->default_is_pre) {
            msg(
                LOG_ERROR, EXPECT_FAIL_DUPLICATE_DEFAULT, env->file_text, curr_if->pos,
                "duplicate default in switch statement\n"
            );
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
            if (vec_at(&exhaustive_data->covered, (size_t)curr_lit->data)) {
                Uast_def* enum_def_ = NULL;
                unwrap(usymbol_lookup(&enum_def_, env, lang_type_get_str(exhaustive_data->oper_lang_type)));
                Uast_enum_def* enum_def = uast_enum_def_unwrap(enum_def_);
                msg(
                    LOG_ERROR, EXPECT_FAIL_DUPLICATE_CASE, env->file_text, curr_if->pos,
                    "duplicate case `"STR_VIEW_FMT"."STR_VIEW_FMT"` in switch statement\n",
                    str_view_print(enum_def->base.name), str_view_print(vec_at(&enum_def->base.members, (size_t)curr_lit->data)->name)
                );
                // TODO: print where original case is
                return false;
            }
            *vec_at_ref(&exhaustive_data->covered, (size_t)curr_lit->data) = true;
            return true;
        }
        case LANG_TYPE_SUM: {
            const Tast_enum_lit* curr_lit = tast_sum_case_unwrap(
                tast_binary_unwrap(curr_if->condition->child)->rhs
            )->tag;

            if (curr_lit->data > (int64_t)exhaustive_data->max_data) {
                unreachable("invalid enum value\n");
            }
            if (vec_at(&exhaustive_data->covered, (size_t)curr_lit->data)) {
                Uast_def* sum_def_ = NULL;
                unwrap(usymbol_lookup(&sum_def_, env, lang_type_get_str(exhaustive_data->oper_lang_type)));
                Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);
                msg(
                    LOG_ERROR, EXPECT_FAIL_DUPLICATE_CASE, env->file_text, curr_if->pos,
                    "duplicate case `"STR_VIEW_FMT"."STR_VIEW_FMT"` in switch statement\n",
                    str_view_print(sum_def->base.name), str_view_print(vec_at(&sum_def->base.members, (size_t)curr_lit->data)->name)
                );
                // TODO: print where original case is
                return false;
            }
            *vec_at_ref(&exhaustive_data->covered, (size_t)curr_lit->data) = true;
            return true;
        }
        default:
            todo();
    }
    unreachable("");
}

static bool check_for_exhaustiveness_finish(Env* env, Exhaustive_data exhaustive_data, Pos pos_switch) {
        unwrap(exhaustive_data.covered.info.count == exhaustive_data.max_data + 1);

        if (exhaustive_data.default_is_pre) {
            return true;
        }

        bool status = true;
        String string = {0};

        for (size_t idx = 0; idx < exhaustive_data.covered.info.count; idx++) {
            if (!vec_at(&exhaustive_data.covered, idx)) {
                Uast_def* enum_def_ = NULL;
                unwrap(usymbol_lookup(&enum_def_, env, lang_type_get_str(exhaustive_data.oper_lang_type)));
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

                if (status == true) {
                    string_extend_cstr(&a_main, &string, "some cases are not covered: ");
                    status = false;
                } else {
                    string_extend_cstr(&a_main, &string, ", ");
                }

                string_extend_strv(&a_main, &string, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&enum_def.members, idx)->lang_type));
                string_extend_cstr(&a_main, &string, ".");
                string_extend_strv(&a_main, &string, vec_at(&enum_def.members, idx)->name);
            }
        }

        if (!status) {
            msg(
                LOG_ERROR, EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH, env->file_text, pos_switch,
                STR_VIEW_FMT"\n", string_print(string)
            );
        }
        return true;
}

bool try_set_switch_types(Env* env, Tast_if_else_chain** new_tast, const Uast_switch* lang_switch) {
    Tast_if_vec new_ifs = {0};

    Tast_expr* new_operand = NULL;
    unwrap(try_set_expr_types(env, &new_operand, lang_switch->operand));

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
                old_case->pos, uast_expr_wrap(lang_switch->operand), old_case->expr, BINARY_DOUBLE_EQUAL
            )));
        }

        vec_append(&a_main, &env->switch_case_defer_add_if_true, old_case->if_true);
        Uast_block* if_true = uast_block_new(
            old_case->pos,
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

        if (!check_for_exhaustiveness_inner(env, &exhaustive_data, new_if, old_case->is_default)) {
            return false;
        }

        vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(lang_switch->pos, new_ifs);
    return check_for_exhaustiveness_finish(env, exhaustive_data, lang_switch->pos);
}

// TODO: merge this with msg_redefinition_of_symbol?
static void try_set_msg_redefinition_of_symbol(Env* env, const Uast_def* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, env->file_text, uast_def_get_pos(new_sym_def),
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, env, uast_def_get_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, env->file_text, uast_def_get_pos(original_def),
        STR_VIEW_FMT " originally defined here\n", str_view_print(uast_def_get_name(original_def))
    );
}

// TODO: consider how to do this
static void do_test_bit_width(void) {
    assert(0 == log2_int64_t(1));
    assert(1 == log2_int64_t(2));
    assert(2 == log2_int64_t(3));
    assert(2 == log2_int64_t(4));
    assert(3 == log2_int64_t(5));
    assert(3 == log2_int64_t(6));
    assert(3 == log2_int64_t(7));
    assert(3 == log2_int64_t(8));

    assert(1 == bit_width_needed_signed(-1));
    assert(2 == bit_width_needed_signed(-2));
    assert(3 == bit_width_needed_signed(-3));
    assert(3 == bit_width_needed_signed(-4));
    assert(4 == bit_width_needed_signed(-5));
    assert(4 == bit_width_needed_signed(-6));
    assert(4 == bit_width_needed_signed(-7));
    assert(4 == bit_width_needed_signed(-8));
    assert(5 == bit_width_needed_signed(-9));

    assert(1 == bit_width_needed_signed(0));
    assert(2 == bit_width_needed_signed(1));
    assert(3 == bit_width_needed_signed(2));
    assert(3 == bit_width_needed_signed(3));
    assert(4 == bit_width_needed_signed(4));
    assert(4 == bit_width_needed_signed(5));
    assert(4 == bit_width_needed_signed(6));
    assert(4 == bit_width_needed_signed(7));
    assert(5 == bit_width_needed_signed(8));

    assert(1 == bit_width_needed_unsigned(0));
    assert(1 == bit_width_needed_unsigned(1));
    assert(2 == bit_width_needed_unsigned(2));
    assert(2 == bit_width_needed_unsigned(3));
    assert(3 == bit_width_needed_unsigned(4));
    assert(3 == bit_width_needed_unsigned(5));
    assert(3 == bit_width_needed_unsigned(6));
    assert(3 == bit_width_needed_unsigned(7));
    assert(4 == bit_width_needed_unsigned(8));
}

bool try_set_block_types(Env* env, Tast_block** new_tast, Uast_block* block, bool is_directly_in_fun_def) {
    do_test_bit_width();

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
        switch (try_set_stmt_types(env, &new_tast, curr_tast)) {
            case STMT_OK:
                assert(curr_tast);
                vec_append(&a_main, &new_tasts, new_tast);
                break;
            case STMT_NO_STMT:
                break;
            case STMT_ERROR:
                status = false;
                break;
            default:
                todo();
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
            unreachable("");
        }
        Tast_stmt* new_rtn_statement = NULL;
        switch (try_set_stmt_types(env, &new_rtn_statement, uast_return_wrap(rtn_statement))) {
            case STMT_ERROR:
                goto error;
            case STMT_OK:
                break;
            default:
                todo();
        }
        assert(rtn_statement);
        assert(new_rtn_statement);
        vec_append(&a_main, &new_tasts, new_rtn_statement);
    }

error:
    vec_rem_last(&env->ancesters);
    *new_tast = tast_block_new(block->pos, new_tasts, new_sym_coll, block->pos_end);
    if (status) {
        assert(*new_tast);
    } else {
        assert(error_count > 0);
    }
    return status;
}

STMT_STATUS try_set_stmt_types(Env* env, Tast_stmt** new_tast, Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Tast_expr* new_tast_ = NULL;
            if (!try_set_expr_types(env, &new_tast_, uast_expr_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_expr_wrap(new_tast_);
            return STMT_OK;
        }
        case UAST_DEF: {
            Tast_def* new_tast_ = NULL;
            STMT_STATUS status = try_set_def_types(env, &new_tast_, uast_def_unwrap(stmt));
            if (status != STMT_OK) {
                return status;
            }
            *new_tast = tast_def_wrap(new_tast_);
            return STMT_OK;
        }
        case UAST_FOR_WITH_COND: {
            Tast_for_with_cond* new_tast_ = NULL;
            if (!try_set_for_with_cond_types(env, &new_tast_, uast_for_with_cond_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_for_with_cond_wrap(new_tast_);
            return STMT_OK;
        }
        case UAST_ASSIGNMENT: {
            Tast_assignment* new_tast_ = NULL;
            if (!try_set_assignment_types(env, &new_tast_, uast_assignment_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_expr_wrap(tast_assignment_wrap(new_tast_));
            return STMT_OK;
        }
        case UAST_RETURN: {
            Tast_return* new_rtn = NULL;
            if (!try_set_return_types(env, &new_rtn, uast_return_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_return_wrap(new_rtn);
            return STMT_OK;
        }
        case UAST_FOR_RANGE: {
            Tast_for_range* new_for = NULL;
            if (!try_set_for_range_types(env, &new_for, uast_for_range_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_for_range_wrap(new_for);
            return STMT_OK;
        }
        case UAST_BREAK:
            *new_tast = tast_break_wrap(tast_break_new(uast_break_unwrap(stmt)->pos));
            return STMT_OK;
        case UAST_CONTINUE:
            *new_tast = tast_continue_wrap(tast_continue_new(uast_continue_unwrap(stmt)->pos));
            return STMT_OK;
        case UAST_BLOCK: {
            assert(uast_block_unwrap(stmt)->pos_end.line > 0);
            Tast_block* new_for = NULL;
            if (!try_set_block_types(env, &new_for, uast_block_unwrap(stmt), false)) {
                return STMT_ERROR;
            }
            *new_tast = tast_block_wrap(new_for);
            return STMT_OK;
        }
        case UAST_IF_ELSE_CHAIN: {
            Tast_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(env, &new_for, uast_if_else_chain_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_if_else_chain_wrap(new_for);
            return STMT_OK;
        }
        case UAST_SWITCH: {
            Tast_if_else_chain* new_if_else = NULL;
            if (!try_set_switch_types(env, &new_if_else, uast_switch_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_if_else_chain_wrap(new_if_else);
            return STMT_OK;
        }
    }
    unreachable("");
}

