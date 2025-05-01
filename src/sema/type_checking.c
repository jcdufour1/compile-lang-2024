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
#include <uast_clone.h>
#include <lang_type_hand_written.h>
#include <lang_type_print.h>
#include <resolve_generics.h>
#include <ulang_type_get_atom.h>
#include <symbol_log.h>
#include <parent_of_print.h>
#include <lang_type_get_pos.h>
#include <lang_type_is.h>
#include <symbol_iter.h>
#include <expand_lang_def.h>
#include <ulang_type_serialize.h>
#include <symbol_table.h>


// TODO: expected failure test for too few elems in struct init (non designated args)

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

static Tast_expr* auto_deref_to_0(Tast_expr* expr) {
    int16_t prev_pointer_depth = lang_type_get_pointer_depth(tast_expr_get_lang_type(expr));
    while (lang_type_get_pointer_depth(tast_expr_get_lang_type(expr)) > 0) {
        unwrap(try_set_unary_types_finish(&expr, expr, tast_expr_get_pos(expr), UNARY_DEREF, (Lang_type) {0}));
        assert(lang_type_get_pointer_depth(tast_expr_get_lang_type(expr)) + 1 == prev_pointer_depth);
        prev_pointer_depth = lang_type_get_pointer_depth(tast_expr_get_lang_type(expr));
    }
    return expr;
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
     
    const Tast_expr* argument,
    const Uast_variable_def* corres_param
) {
    msg_internal(
        file, line,
        LOG_ERROR, EXPECT_FAIL_INVALID_FUN_ARG, env.file_path_to_text, tast_expr_get_pos(argument), 
        "argument is of type `"LANG_TYPE_FMT"`, "
        "but the corresponding parameter `"STR_VIEW_FMT"` is of type `"LANG_TYPE_FMT"`\n",
        lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(argument)), 
        name_print(corres_param->name),
        ulang_type_print(LANG_TYPE_MODE_MSG, corres_param->lang_type)
    );
    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, corres_param->pos,
        "corresponding parameter `"STR_VIEW_FMT"` defined here\n",
        name_print(corres_param->name)
    );
}

static void msg_invalid_count_function_args_internal(
    const char* file,
    int line,
     
    const Uast_function_call* fun_call,
    const Uast_function_decl* fun_decl,
    size_t min_args,
    size_t max_args
) {
    String message = {0};
    string_extend_size_t(&print_arena, &message, fun_call->args.info.count);
    string_extend_cstr(&print_arena, &message, " arguments are passed to function `");
    extend_name(false, &message, fun_decl->name);
    string_extend_cstr(&print_arena, &message, "`, but ");
    string_extend_size_t(&print_arena, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&print_arena, &message, " or more");
    }
    string_extend_cstr(&print_arena, &message, " arguments expected\n");
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env.file_path_to_text, fun_call->pos,
        STR_VIEW_FMT, str_view_print(string_to_strv(message))
    );

    msg_internal(
        file, line, LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, uast_function_decl_get_pos(fun_decl),
        "function `"STR_VIEW_FMT"` defined here\n", name_print(fun_decl->name)
    );
}

#define msg_invalid_function_arg(argument, corres_param) \
    msg_invalid_function_arg_internal(__FILE__, __LINE__, argument, corres_param)

#define msg_invalid_count_function_args(fun_call, fun_decl, min_args, max_args) \
    msg_invalid_count_function_args_internal(__FILE__, __LINE__, fun_call, fun_decl, min_args, max_args)

static void msg_invalid_yield_type_internal(const char* file, int line, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_YIELD_STATEMENT, env.file_path_to_text, pos,
            "no break statement in case that breaks `"LANG_TYPE_FMT"`\n",
            lang_type_print(LANG_TYPE_MODE_MSG, env.break_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_YIELD_TYPE, env.file_path_to_text, pos,
            "breaking `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(child)),
            lang_type_print(LANG_TYPE_MODE_MSG, env.break_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, (Pos) {0} /* TODO */,
        "case break type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(LANG_TYPE_MODE_MSG, env.break_type) 
    );
}

static void msg_invalid_return_type_internal(const char* file, int line, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISSING_RETURN, env.file_path_to_text, pos,
            "no return statement in function that returns `"LANG_TYPE_FMT"`\n",
            ulang_type_print(LANG_TYPE_MODE_MSG, env.parent_fn_rtn_type)
        );
    } else {
        msg_internal(
            file, line,
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, env.file_path_to_text, pos,
            "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(child)), 
            ulang_type_print(LANG_TYPE_MODE_MSG, env.parent_fn_rtn_type)
        );
    }

    msg_internal(
        file, line,
        LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, ulang_type_get_pos(env.parent_fn_rtn_type),
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        ulang_type_print(LANG_TYPE_MODE_MSG, env.parent_fn_rtn_type)
    );
}

#define msg_invalid_yield_type(pos, child, is_auto_inserted) \
    msg_invalid_yield_type_internal(__FILE__, __LINE__, pos, child, is_auto_inserted)

#define msg_invalid_return_type(pos, child, is_auto_inserted) \
    msg_invalid_return_type_internal(__FILE__, __LINE__, pos, child, is_auto_inserted)

typedef enum {
    CHECK_ASSIGN_OK,
    CHECK_ASSIGN_INVALID, // error was not printed to the user (caller should print error),
                          // and new_src is valid for printing purposes
    CHECK_ASSIGN_ERROR, // error was printed, and new_src is not valid for printing purposes
} CHECK_ASSIGN_STATUS;

CHECK_ASSIGN_STATUS check_generic_assignment_finish(
     
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    bool src_is_zero,
    Tast_expr* src
) {
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
        msg_todo("non literal implicit conversion", tast_expr_get_pos(src));
        return CHECK_ASSIGN_ERROR;
    } else {
        return CHECK_ASSIGN_INVALID;
    }
    unreachable("");
}

CHECK_ASSIGN_STATUS check_generic_assignment(
    Tast_expr** new_src,
    Lang_type dest_lang_type,
    Uast_expr* src,
    Pos pos
) {
    if (src->type == UAST_STRUCT_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        if (!try_set_struct_literal_types(
             &new_src_, dest_lang_type, uast_struct_literal_unwrap(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_expr_unwrap(new_src_);
    } else if (src->type == UAST_ARRAY_LITERAL) {
        Tast_stmt* new_src_ = NULL;
        if (!try_set_array_literal_types(
             &new_src_, dest_lang_type, uast_array_literal_unwrap(src), pos
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_expr_unwrap(new_src_);
    } else if (src->type == UAST_TUPLE) {
        Tast_tuple* new_src_ = NULL;
        if (!try_set_tuple_assignment_types(
             &new_src_, dest_lang_type, uast_tuple_unwrap(src)
        )) {
            return CHECK_ASSIGN_ERROR;
        }
        *new_src = tast_tuple_wrap(new_src_);
    } else {
        log(LOG_DEBUG, TAST_FMT"\n", lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type));
        log(LOG_DEBUG, TAST_FMT, uast_expr_print(src));
        Lang_type old_lhs_lang_type = env.lhs_lang_type;
        PARENT_OF old_parent_of = env.parent_of;
        env.parent_of = PARENT_OF_ASSIGN_RHS;
        env.lhs_lang_type = dest_lang_type;
        if (!try_set_expr_types(new_src, src)) {
            env.lhs_lang_type = old_lhs_lang_type;
            env.parent_of = old_parent_of;
            return CHECK_ASSIGN_ERROR;
        }
        env.lhs_lang_type = old_lhs_lang_type;
        env.parent_of = old_parent_of;
    }

    bool src_is_zero = false;
    if (src->type == UAST_LITERAL && uast_literal_unwrap(src)->type == UAST_NUMBER && uast_number_unwrap(uast_literal_unwrap(src))->data == 0) {
        src_is_zero = true;
    }

    return check_generic_assignment_finish(new_src, dest_lang_type, src_is_zero, *new_src);
}

Tast_literal* try_set_literal_types(Uast_literal* literal) {
    switch (literal->type) {
        case UAST_STRING: {
            Uast_string* old_string = uast_string_unwrap(literal);
            return tast_string_wrap(tast_string_new(
                old_string->pos,
                old_string->data,
                name_new(env.curr_mod_path, old_string->name, (Ulang_type_vec) {0}, 0)
            ));
        }
        case UAST_NUMBER: {
            Uast_number* old_number = uast_number_unwrap(literal);
            if (old_number->data < 0) {
                int64_t bit_width = bit_width_needed_signed(old_number->data);
                return tast_number_wrap(tast_number_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(old_number->pos, bit_width, 0))
                )));
            } else {
                int64_t bit_width = bit_width_needed_unsigned(old_number->data);
                return tast_number_wrap(tast_number_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(old_number->pos, bit_width, 0))
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

// set symbol lang_type, and report error if symbol is undefined
bool try_set_symbol_types(Tast_expr** new_tast, Uast_symbol* sym_untyped) {
    Uast_expr* new_expr = NULL;
    switch (expand_def_symbol(&new_expr, sym_untyped)) {
        case EXPAND_NAME_ERROR:
            return false;
        case EXPAND_NAME_NORMAL:
            break;
        case EXPAND_NAME_NEW_EXPR:
            return try_set_expr_types(new_tast, new_expr);
        default:
            unreachable("");
    }

    Uast_def* sym_def = NULL;
    if (!usymbol_lookup(&sym_def, sym_untyped->name)) {
        Name base_name = sym_untyped->name;
        memset(&base_name.gen_args, 0, sizeof(base_name.gen_args));
        if (!usymbol_lookup(&sym_def, base_name)) {
            msg_undefined_symbol(env.file_path_to_text, sym_untyped);
            return false;
        }
    }

    switch (sym_def->type) {
        case UAST_FUNCTION_DECL: {
            Uast_function_decl* new_decl = uast_function_decl_unwrap(sym_def);
            *new_tast = tast_literal_wrap(tast_function_lit_wrap(tast_function_lit_new(
                sym_untyped->pos,
                new_decl->name,
                lang_type_from_ulang_type(ulang_type_from_uast_function_decl(new_decl))
            )));
            return true;
        }
        case UAST_FUNCTION_DEF: {
            Uast_function_def* new_def = NULL;
            if (!resolve_generics_function_def(&new_def, uast_function_def_unwrap(sym_def), sym_untyped->name.gen_args, sym_untyped->pos)) {
                return false;
            }
            *new_tast = tast_literal_wrap(tast_function_lit_wrap(tast_function_lit_new(
                sym_untyped->pos,
                new_def->decl->name,
                lang_type_from_ulang_type(ulang_type_from_uast_function_decl(new_def->decl))
            )));
            return true;
        }
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
            Lang_type lang_type = {0};
            if (!uast_def_get_lang_type(&lang_type, sym_def, sym_untyped->name.gen_args)) {
                return false;
            }
            Sym_typed_base new_base = {.lang_type = lang_type, sym_untyped->name};
            Tast_symbol* sym_typed = tast_symbol_new(sym_untyped->pos, new_base);
            *new_tast = tast_symbol_wrap(sym_typed);
            return true;
        }
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS: {
            assert(uast_mod_alias_unwrap(sym_def)->mod_path.gen_args.info.count < 1);
            Tast_module_alias* sym_typed = tast_module_alias_new(sym_untyped->pos, uast_mod_alias_unwrap(sym_def)->name, uast_mod_alias_unwrap(sym_def)->mod_path.base);
            *new_tast = tast_module_alias_wrap(sym_typed);
            return true;
        }
        case UAST_GENERIC_PARAM:
            unreachable("cannot set symbol of template parameter here");
        case UAST_POISON_DEF:
            return false;
        case UAST_LANG_DEF:
            unreachable("lang def alias should have been expanded already");
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

bool try_set_binary_types_finish(Tast_expr** new_tast, Tast_expr* new_lhs, Tast_expr* new_rhs, Pos oper_pos, BINARY_TYPE oper_token_type) {
    if (!lang_type_is_equal(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs))) {
        if (can_be_implicitly_converted(
            
            tast_expr_get_lang_type(new_lhs),
            tast_expr_get_lang_type(new_rhs),
            (
                new_rhs->type == TAST_LITERAL &&
                tast_literal_unwrap(new_rhs)->type == TAST_NUMBER &&
                tast_number_unwrap(tast_literal_unwrap(new_rhs))->data == 0
            ),
            true
        )) {
            if (new_rhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(new_lhs);
                new_rhs = auto_deref_to_0(new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_rhs), tast_expr_get_lang_type(new_lhs));
            } else {
                unwrap(try_set_unary_types_finish(&new_rhs, new_rhs, tast_expr_get_pos(new_rhs), UNARY_UNSAFE_CAST, tast_expr_get_lang_type(new_lhs)));
            }
        } else if (can_be_implicitly_converted(
            
            tast_expr_get_lang_type(new_rhs),
            tast_expr_get_lang_type(new_lhs),
            (
                new_rhs->type == TAST_LITERAL &&
                tast_literal_unwrap(new_rhs)->type == TAST_NUMBER &&
                tast_number_unwrap(tast_literal_unwrap(new_rhs))->data == 0
            ),
            true
        )) {
            if (new_lhs->type == TAST_LITERAL) {
                new_lhs = auto_deref_to_0(new_lhs);
                new_rhs = auto_deref_to_0(new_rhs);
                tast_literal_set_lang_type(tast_literal_unwrap(new_lhs), tast_expr_get_lang_type(new_rhs));
            } else {
                unwrap(try_set_unary_types_finish(&new_lhs, new_lhs, tast_expr_get_pos(new_lhs), UNARY_UNSAFE_CAST, tast_expr_get_lang_type(new_rhs)));
            }
        } else {
            msg(
                LOG_ERROR, EXPECT_FAIL_BINARY_MISMATCHED_TYPES, env.file_path_to_text, oper_pos,
                "types `"LANG_TYPE_FMT"` and `"LANG_TYPE_FMT"` are not valid operands to binary expression\n",
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_lhs)),
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs))
            );
            return false;
        }
    }
            
    assert(lang_type_get_str(tast_expr_get_lang_type(new_lhs)).base.count > 0);

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
            lang_type_unsigned_int_new(POS_BUILTIN, 1, 0)
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

    assert(lang_type_get_str(tast_expr_get_lang_type(*new_tast)).base.count > 0);
    assert(*new_tast);
    return true;
}

// returns false if unsuccessful
bool try_set_binary_types(Tast_expr** new_tast, Uast_binary* operator) {
    Tast_expr* new_lhs;
    if (!try_set_expr_types(&new_lhs, operator->lhs)) {
        return false;
    }
    assert(new_lhs);

    Tast_expr* new_rhs = NULL;
    if (operator->token_type == BINARY_SINGLE_EQUAL) {
        switch (check_generic_assignment(&new_rhs, tast_expr_get_lang_type(new_lhs), operator->rhs, operator->pos)) {
            case CHECK_ASSIGN_OK:
                *new_tast = tast_assignment_wrap(tast_assignment_new(operator->pos, new_lhs, new_rhs));
                return true;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env.file_path_to_text,
                    operator->pos,
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_lhs))
                );
                return false;
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                unreachable("");
        }
    }

    env.lhs_lang_type = tast_expr_get_lang_type(new_lhs);
    if (!try_set_expr_types(&new_rhs, operator->rhs)) {
        // TODO: replace or remove this memset
        memset(&env.lhs_lang_type, 0, sizeof(env.lhs_lang_type));
        return false;
    }
    // TODO: replace or remove this memset
    memset(&env.lhs_lang_type, 0, sizeof(env.lhs_lang_type));

    return try_set_binary_types_finish(
        new_tast,
        new_lhs,
        new_rhs,
        operator->pos,
        operator->token_type
    );
}

bool try_set_unary_types_finish(
     
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
                    LOG_ERROR, EXPECT_FAIL_DEREF_NON_POINTER, env.file_path_to_text, unary_pos,
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
            assert(lang_type_get_str(cast_to).base.count > 0);
            if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number_like(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number(tast_expr_get_lang_type(new_child)) && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0) {
            } else {
                log(LOG_NOTE, TAST_FMT, lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_child)));
                log(LOG_NOTE, BOOL_FMT, bool_print(lang_type_is_number(tast_expr_get_lang_type(new_child))));
                log(LOG_NOTE, "%d\n", lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)));
                log(LOG_NOTE, TAST_FMT, tast_expr_print(new_child));
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
                    LOG_ERROR, EXPECT_FAIL_UNARY_MISMATCHED_TYPES, env.file_path_to_text, unary_pos,
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

bool try_set_unary_types(Tast_expr** new_tast, Uast_unary* unary) {
    Tast_expr* new_child;
    if (!try_set_expr_types(&new_child, unary->child)) {
        return false;
    }

    return try_set_unary_types_finish(new_tast, new_child, uast_unary_get_pos(unary), unary->token_type, lang_type_from_ulang_type(unary->lang_type));
}

// returns false if unsuccessful
bool try_set_operator_types(Tast_expr** new_tast, Uast_operator* operator) {
    if (operator->type == UAST_UNARY) {
        return try_set_unary_types(new_tast, uast_unary_unwrap(operator));
    } else if (operator->type == UAST_BINARY) {
        return try_set_binary_types(new_tast, uast_binary_unwrap(operator));
    } else {
        unreachable("");
    }
}

bool try_set_tuple_assignment_types(
    Tast_tuple** new_tast,
    Lang_type dest_lang_type,
    Uast_tuple* tuple
) {
    if (lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count != tuple->members.info.count) {
        msg(
            LOG_ERROR, EXPECT_FAIL_MISMATCHED_TUPLE_COUNT, env.file_path_to_text, uast_tuple_get_pos(tuple),
            "tuple `"UAST_FMT"` cannot be assigned to `"LANG_TYPE_FMT"`; "
            "tuple `"UAST_FMT"` has %zu elements, but type `"LANG_TYPE_FMT"` has %zu elements\n",
            uast_tuple_print(tuple), lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type),
            uast_tuple_print(tuple), tuple->members.info.count,
            lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type),
            lang_type_tuple_const_unwrap(dest_lang_type).lang_types.info.count
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
            &new_memb,
            curr_dest,
            curr_src,
            uast_expr_get_pos(curr_src)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env.file_path_to_text,
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

    *new_tast = tast_tuple_new(tuple->pos, new_members, lang_type_tuple_new(lang_type_tuple_const_unwrap(dest_lang_type).pos, new_lang_type));
    return true;
}

static bool uast_expr_is_assignment(const Uast_expr* expr) {
    if (expr->type != UAST_OPERATOR) {
        return false;
    }
    const Uast_operator* oper = uast_operator_const_unwrap(expr);

    if (oper->type != UAST_BINARY) {
        return false;
    }
    const Uast_binary* bin = uast_binary_const_unwrap(oper);

    return bin->token_type == BINARY_SINGLE_EQUAL;
}

static bool uast_expr_is_designator(const Uast_expr* expr) {
    if (!uast_expr_is_assignment(expr)) {
        return false;
    }
    const Uast_binary* assign = uast_binary_const_unwrap(uast_operator_const_unwrap(expr));
    if (assign->lhs->type != UAST_MEMBER_ACCESS) {
        return false;
    }
    const Uast_member_access* access = uast_member_access_const_unwrap(assign->lhs);

    return access->callee->type == UAST_UNKNOWN;
}

static bool try_set_struct_literal_member_types(Tast_expr_vec* new_membs, Uast_expr_vec membs, Uast_variable_def_vec memb_defs) {
    for (size_t idx = 0; idx < membs.info.count; idx++) {
        Uast_variable_def* memb_def = vec_at(&memb_defs, idx);
        Uast_expr* memb = vec_at(&membs, idx);
        Uast_expr* rhs = NULL;
        if (uast_expr_is_designator(memb)) {
            // TODO: expected failure case for invalid thing on lhs of designated initializer
            Uast_member_access* lhs = uast_member_access_unwrap(
                uast_binary_unwrap(uast_operator_unwrap(memb))->lhs // parser should catch invalid assignment
            );
            rhs = uast_binary_unwrap(uast_operator_unwrap(memb))->rhs;
            if (!name_is_equal(memb_def->name, lhs->member_name->name)) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_INVALID_MEMBER_IN_LITERAL, env.file_path_to_text, lhs->pos,
                    "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
                    str_view_print(memb_def->name.base), name_print(lhs->member_name->name)
                );
                return false;
            }
        } else {
            rhs = memb;
        }

        log(LOG_DEBUG, TAST_FMT, uast_expr_print(rhs));
        Tast_expr* new_rhs = NULL;
        switch (check_generic_assignment(
             &new_rhs, lang_type_from_ulang_type(memb_def->lang_type), rhs, uast_expr_get_pos(memb)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env.file_path_to_text,
                    uast_expr_get_pos(memb),
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                    ulang_type_print(LANG_TYPE_MODE_MSG, memb_def->lang_type)
                );
                return false;
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                unreachable("");
        }

        vec_append(&a_main, new_membs, new_rhs);
    }

    return true;
}

bool try_set_struct_literal_types(
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_struct_literal* lit,
    Pos assign_pos
) {
    switch (dest_lang_type.type) {
        case LANG_TYPE_STRUCT:
            break;
        case LANG_TYPE_RAW_UNION:
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_RAW_UNION, env.file_path_to_text,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        case LANG_TYPE_SUM:
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_SUM, env.file_path_to_text,
                assign_pos, "struct literal cannot be assigned to sum\n"
            );
            return false;
        case LANG_TYPE_PRIMITIVE:
            msg(
                LOG_ERROR, EXPECT_FAIL_STRUCT_INIT_ON_PRIMITIVE, env.file_path_to_text,
                assign_pos, "struct literal cannot be assigned to primitive type `"LANG_TYPE_FMT"`\n",
                lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type)
            );
            return false;
        default:
            unreachable(TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, dest_lang_type));
    }
    Uast_def* struct_def_ = NULL;
    unwrap(usymbol_lookup(&struct_def_, lang_type_struct_const_unwrap(dest_lang_type).atom.str));
    Uast_struct_def* struct_def = uast_struct_def_unwrap(struct_def_);
    
    Tast_expr_vec new_membs = {0};
    if (!try_set_struct_literal_member_types(&new_membs, lit->members, struct_def->base.members)) {
        return false;
    }

    Tast_struct_literal* new_lit = tast_struct_literal_new(
        lit->pos,
        new_membs,
        name_new(env.curr_mod_path, lit->name, (Ulang_type_vec) {0}, 0),
        dest_lang_type
    );
    *new_tast = tast_expr_wrap(tast_struct_literal_wrap(new_lit));

    Tast_struct_lit_def* new_def = tast_struct_lit_def_new(
        new_lit->pos,
        new_lit->members,
        new_lit->name,
        new_lit->lang_type
    );

    unwrap(symbol_add(tast_literal_def_wrap(tast_struct_lit_def_wrap(new_def))));
    return true;
}

bool try_set_array_literal_types(
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_array_literal* lit,
    Pos assign_pos
) {
    (void) assign_pos;
    log(LOG_DEBUG, TAST_FMT"\n", lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type));
    Ulang_type gen_arg_ = {0};
    Lang_type gen_arg = {0};
    if (lang_type_is_slice(&gen_arg_, dest_lang_type)) {
        if (!try_lang_type_from_ulang_type(&gen_arg, gen_arg_, lit->pos)) {
            // TODO: expected failure test
            todo();
        }
        log(LOG_DEBUG, TAST_FMT"\n", lang_type_print(LANG_TYPE_MODE_MSG, gen_arg));
    } else {
        todo();
    }

    Uast_def* struct_def_ = NULL;
    unwrap(usymbol_lookup(&struct_def_, lang_type_struct_const_unwrap(dest_lang_type).atom.str));
    
    Tast_expr_vec new_membs = {0};
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Uast_expr* rhs = vec_at(&lit->members, idx);
        Tast_expr* new_rhs = NULL;
        switch (check_generic_assignment(
             &new_rhs, gen_arg, rhs, uast_expr_get_pos(rhs)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env.file_path_to_text,
                    uast_expr_get_pos(rhs),
                    "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                    lang_type_print(LANG_TYPE_MODE_MSG, gen_arg)
                );
                return false;
            case CHECK_ASSIGN_ERROR:
                return false;
            default:
                unreachable("");
        }

        vec_append(&a_main, &new_membs, new_rhs);
    }

    Tast_variable_def_vec inner_def_membs = {0};
    for (size_t idx = 0; idx < new_membs.info.count; idx++) {
        vec_append(&a_main, &inner_def_membs, tast_variable_def_new(
            lit->pos,
            gen_arg,
            false,
            name_new(env.curr_mod_path, util_literal_name_new(), (Ulang_type_vec) {0}, 0)
        ));
    }
    Tast_struct_def* inner_def = tast_struct_def_new(
        lit->pos,
        (Struct_def_base) {.members = inner_def_membs, .name = name_new(env.curr_mod_path, util_literal_name_new(), (Ulang_type_vec) {0}, 0)}
    );
    sym_tbl_add(tast_struct_def_wrap(inner_def));

    Tast_struct_literal* new_inner_lit = tast_struct_literal_new(
        lit->pos,
        new_membs,
        name_new(env.curr_mod_path, lit->name, (Ulang_type_vec) {0}, 0),
        lang_type_struct_const_wrap(lang_type_struct_new(lit->pos, lang_type_atom_new(inner_def->base.name, 0)))
    );
    Ulang_type dummy = {0};
    if (!lang_type_is_slice(&dummy, dest_lang_type)) {
        todo();
    }

    Lang_type unary_lang_type = gen_arg;
    lang_type_set_pointer_depth(&unary_lang_type, lang_type_get_pointer_depth(unary_lang_type) + 1);
    Tast_expr* ptr = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
        new_inner_lit->pos,
        tast_struct_literal_wrap(new_inner_lit),
        UNARY_REFER,
        unary_lang_type
    )));

    Tast_struct_lit_def* new_def = tast_struct_lit_def_new(
        new_inner_lit->pos,
        new_inner_lit->members,
        new_inner_lit->name,
        new_inner_lit->lang_type
    );
    unwrap(symbol_add(tast_literal_def_wrap(tast_struct_lit_def_wrap(new_def))));

    Tast_expr_vec new_lit_membs = {0};
    vec_append(&a_main, &new_lit_membs, ptr);
    vec_append(&a_main, &new_lit_membs, tast_literal_wrap(tast_number_wrap(tast_number_new(
        new_inner_lit->pos,
        new_membs.info.count,
        lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(new_inner_lit->pos, 8, 0)))
    ))));
    Tast_struct_literal* new_lit = tast_struct_literal_new(
        new_inner_lit->pos,
        new_lit_membs,
        name_new(env.curr_mod_path, util_literal_name_new(), (Ulang_type_vec) {0}, 0),
        dest_lang_type
    );
    *new_tast = tast_expr_wrap(tast_struct_literal_wrap(new_lit));
    return true;
}

bool try_set_expr_types(Tast_expr** new_tast, Uast_expr* uast) {
    switch (uast->type) {
        case UAST_LITERAL: {
            *new_tast = tast_literal_wrap(try_set_literal_types(uast_literal_unwrap(uast)));
            return true;
        }
        case UAST_SYMBOL:
            if (!try_set_symbol_types(new_tast, uast_symbol_unwrap(uast))) {
                return false;
            } else {
                assert(*new_tast);
            }
            return true;
        case UAST_UNKNOWN:
            return try_set_symbol_types(new_tast, uast_symbol_new(
                uast_expr_get_pos(uast),
                lang_type_get_str(env.lhs_lang_type)
            ));
        case UAST_MEMBER_ACCESS: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_member_access_types(&new_tast_, uast_member_access_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_expr_unwrap(new_tast_);
            return true;
        }
        case UAST_INDEX: {
            Tast_stmt* new_tast_ = NULL;
            if (!try_set_index_untyped_types(&new_tast_, uast_index_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_expr_unwrap(new_tast_);
            return true;
        }
        case UAST_OPERATOR:
            if (!try_set_operator_types(new_tast, uast_operator_unwrap(uast))) {
                return false;
            }
            assert(*new_tast);
            return true;
        case UAST_FUNCTION_CALL:
            return try_set_function_call_types(new_tast, uast_function_call_unwrap(uast));
        case UAST_TUPLE: {
            Tast_tuple* new_call = NULL;
            if (!try_set_tuple_types(&new_call, uast_tuple_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_tuple_wrap(new_call);
            return true;
        }
        case UAST_STRUCT_LITERAL:
            unreachable("");
        case UAST_SUM_ACCESS: {
            Tast_sum_access* new_access = NULL;
            if (!try_set_sum_access_types(&new_access, uast_sum_access_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_sum_access_wrap(new_access);
            return true;
        }
        case UAST_SUM_GET_TAG: {
            Tast_sum_get_tag* new_access = NULL;
            if (!try_set_sum_get_tag_types(&new_access, uast_sum_get_tag_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_sum_get_tag_wrap(new_access);
            return true;
        }
        case UAST_SWITCH: {
            Tast_if_else_chain* new_if_else = NULL;
            if (!try_set_switch_types(&new_if_else, uast_switch_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_if_else_chain_wrap(new_if_else);
            return true;
        }
        case UAST_IF_ELSE_CHAIN: {
            Tast_if_else_chain* new_for = NULL;
            if (!try_set_if_else_chain(&new_for, uast_if_else_chain_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_if_else_chain_wrap(new_for);
            return true;
        }
        case UAST_ARRAY_LITERAL:
            unreachable("");
    }
    unreachable("");
}

STMT_STATUS try_set_def_types(Tast_stmt** new_stmt, Uast_def* uast) {
    switch (uast->type) {
        case UAST_VARIABLE_DEF: {
            Tast_variable_def* new_def = NULL;
            if (!try_set_variable_def_types(&new_def, uast_variable_def_unwrap(uast), true, false)) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_FUNCTION_DECL: {
            Tast_function_decl* dummy = NULL;
            if (!try_set_function_decl_types(&dummy, uast_function_decl_unwrap(uast), false)) {
                assert(error_count > 0);
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_FUNCTION_DEF: {
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
            if (!try_set_primitive_def_types(uast_primitive_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_LITERAL_DEF: {
            if (!try_set_literal_def_types(uast_literal_def_unwrap(uast))) {
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
        case UAST_POISON_DEF: {
            todo();
        }
        case UAST_IMPORT_PATH: {
            Tast_block* new_block = NULL;
            if (!try_set_import_path_types(&new_block, uast_import_path_unwrap(uast))) {
                return STMT_ERROR;
            }
            *new_stmt = tast_block_wrap(new_block);
            return STMT_OK;
        }
        case UAST_MOD_ALIAS:
            return STMT_NO_STMT;
        case UAST_LANG_DEF:
            return STMT_NO_STMT;
    }
    unreachable("");
}

// TODO: remove this function and Uast_assignment
bool try_set_assignment_types(Tast_assignment** new_assign, Uast_assignment* assignment) {
    Tast_expr* new_lhs = NULL;
    if (!try_set_expr_types(&new_lhs, assignment->lhs)) {
        return false;
    }

    Tast_expr* new_rhs = NULL;
    switch (check_generic_assignment(
         &new_rhs, tast_expr_get_lang_type(new_lhs), assignment->rhs, assignment->pos
    )) {
        case CHECK_ASSIGN_OK:
            break;
        case CHECK_ASSIGN_INVALID:
            msg(
                LOG_ERROR, EXPECT_FAIL_ASSIGNMENT_MISMATCHED_TYPES, env.file_path_to_text,
                assignment->pos,
                "type `"LANG_TYPE_FMT"` cannot be implicitly converted to `"LANG_TYPE_FMT"`\n",
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs)),
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_lhs))
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

bool try_set_function_call_types_sum_case(Tast_sum_case** new_case, Uast_expr_vec args, Tast_sum_case* sum_case) {
    switch (sum_case->tag->lang_type.type) {
        case LANG_TYPE_VOID: {
            if (args.info.count > 0) {
                // TODO: expected failure case
                msg(LOG_ERROR, EXPECT_FAIL_NONE, env.file_path_to_text, uast_expr_get_pos(vec_at(&args, 0)), "no arguments expected here\n");
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
                name_new(env.curr_mod_path, uast_symbol_unwrap(vec_at(&args, 0))->name.base, (Ulang_type_vec) {0}, uast_symbol_unwrap(vec_at(&args, 0))->name.scope_id)
            );
            todo();
            //usymbol_add(uast_variable_def_wrap(new_def));

            Uast_assignment* new_assign = uast_assignment_new(
                new_def->pos,
                uast_symbol_wrap(uast_symbol_new(new_def->pos, new_def->name)),
                uast_sum_access_wrap(uast_sum_access_new(
                    new_def->pos,
                    sum_case->tag,
                    lang_type_from_ulang_type(new_def->lang_type),
                    uast_expr_clone(env.parent_of_operand, uast_symbol_unwrap(vec_at(&args, 0))->name.scope_id)
                ))
            );

            vec_append(&a_main, &env.switch_case_defer_add_sum_case_part, uast_assignment_wrap(new_assign));

            *new_case = sum_case;
            return true;
        }
    }
}

static Uast_function_decl* uast_function_decl_from_ulang_type_fn(Ulang_type_fn lang_type, Pos pos) {
    Name name = serialize_ulang_type(env.curr_mod_path, ulang_type_fn_const_wrap(lang_type));
    Uast_def* fun_decl_ = NULL;
    if (usym_tbl_lookup(&fun_decl_, name)) {
        return uast_function_decl_unwrap(fun_decl_);
    }

    Uast_param_vec params = {0};
    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        vec_append(&a_main, &params, uast_param_new(
            pos,
            uast_variable_def_new(pos, vec_at(&lang_type.params.ulang_types, idx), name_new(env.curr_mod_path, util_literal_name_new(), (Ulang_type_vec) {0}, 0)),
            false, // TODO: test case for optional in function callback
            false, // TODO: test case for variadic in function callback
            NULL
        ));
    }

    Uast_function_decl* fun_decl = uast_function_decl_new(
        pos,
        (Uast_generic_param_vec) {0},
        uast_function_params_new(pos, params),
        *lang_type.return_type,
        name
    );
    usym_tbl_add(uast_function_decl_wrap(fun_decl));
    return fun_decl;
}


bool try_set_function_call_types(Tast_expr** new_call, Uast_function_call* fun_call) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, fun_call->callee)) {
        return false;
    }

    bool status = true;

    Uast_def* fun_def = NULL;
    switch (new_callee->type) {
        case TAST_SYMBOL: {
            if (!usymbol_lookup(&fun_def, tast_symbol_unwrap(new_callee)->base.name)) {
                msg(
                    LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, env.file_path_to_text, fun_call->pos,
                    "function `"STR_VIEW_FMT"` is not defined\n", name_print(tast_symbol_unwrap(new_callee)->base.name)
                );
                status = false;
                goto error;
            }
            break;
        }
        case TAST_MEMBER_ACCESS:
            todo();
            //if (tast_member_access_is_sum(tast_member_access_unwrap(new_callee))) {
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
            unwrap(usymbol_lookup(&sum_def_, lang_type_get_str(sum_callee->sum_lang_type)));
            Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);

            Tast_expr* new_item = NULL;
            switch (check_generic_assignment(
                &new_item,
                lang_type_from_ulang_type(vec_at(&sum_def->base.members, (size_t)sum_callee->tag->data)->lang_type),
                vec_at(&fun_call->args, 0),
                uast_expr_get_pos(vec_at(&fun_call->args, 0))
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg(
                        LOG_ERROR, EXPECT_FAIL_SUM_LIT_INVALID_ARG, env.file_path_to_text, tast_expr_get_pos(new_item),
                        "cannot assign "TAST_FMT" of type `"LANG_TYPE_FMT"` to '"LANG_TYPE_FMT"`\n", 
                        tast_expr_print(new_item),
                        lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_item)), 
                        lang_type_print(LANG_TYPE_MODE_MSG, lang_type_from_ulang_type(
                             vec_at(&sum_def->base.members, (size_t)sum_callee->tag->data)->lang_type
                        ))
                   );
                   status = false;
                   break;
                case CHECK_ASSIGN_ERROR:
                    todo();
                default:
                    unreachable("");
            }

            // TODO: set tag size based on target platform
            sum_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)));

            Tast_sum_lit* new_lit = tast_sum_lit_new(
                sum_callee->pos,
                sum_callee->tag,
                new_item,
                sum_callee->sum_lang_type
            );
            *new_call = tast_literal_wrap(tast_sum_lit_wrap(new_lit));
            return status;
        }
        case TAST_SUM_CASE: {
            if (fun_call->args.info.count != 1) {
                // TODO: expected failure case
                todo();
            }
            Tast_sum_case* new_case = NULL;
            if (!try_set_function_call_types_sum_case(&new_case, fun_call->args, tast_sum_case_unwrap(new_callee))) {
                return false;
            }
            *new_call = tast_sum_case_wrap(new_case);
            return status;
        }
        case TAST_LITERAL: {
            if (tast_literal_unwrap(new_callee)->type == TAST_SUM_LIT) {
                Tast_sum_lit* sum_lit = tast_sum_lit_unwrap(tast_literal_unwrap(new_callee));
                if (fun_call->args.info.count != 0) {
                    Uast_def* sum_def_ = NULL;
                    unwrap(usymbol_lookup(&sum_def_, lang_type_get_str(sum_lit->sum_lang_type)));
                    Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);
                    msg(
                        LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_FUN_ARGS, env.file_path_to_text, fun_call->pos,
                        "cannot assign argument to varient `"LANG_TYPE_FMT"."LANG_TYPE_FMT"`, because inner type is void\n",
                        lang_type_print(LANG_TYPE_MODE_MSG, sum_lit->sum_lang_type), name_print(vec_at(&sum_def->base.members, (size_t)sum_lit->tag->data)->name)
                    );
                    return false;
                }
                *new_call = new_callee;
                return status;
            } else {
                usymbol_log_level(LOG_DEBUG, 1);
                log(LOG_DEBUG, "before problem\n");
                log(LOG_DEBUG, "%zu\n", tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name.scope_id);
                log(LOG_DEBUG, "right before problem\n");
                log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_name_symbol_table(tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name)));
                log(LOG_DEBUG, "%zu\n", serialize_name_symbol_table(tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name).count);
                unwrap(usym_tbl_lookup(&fun_def, tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name));
                unwrap(usymbol_lookup(&fun_def, tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name));
                break;
            }
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
        case UAST_VARIABLE_DEF: {
            if (uast_variable_def_unwrap(fun_def)->lang_type.type != ULANG_TYPE_FN) {
                todo();
            }
            fun_decl = uast_function_decl_from_ulang_type_fn(
                
                ulang_type_fn_const_unwrap(uast_variable_def_unwrap(fun_def)->lang_type),
                uast_variable_def_unwrap(fun_def)->pos
            );
            break;
        }
        default:
            unreachable(TAST_FMT, uast_def_print(fun_def));
    }

    Lang_type fun_rtn_type = lang_type_from_ulang_type(fun_decl->return_type);
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
            corres_arg = uast_expr_clone(param->optional_default, 0 /* TODO */);
        } else {
            // TODO: print max count correctly for variadic fucntions
            msg_invalid_count_function_args(fun_call, fun_decl, param_idx + 1, param_idx + 1);
            status = false;
            goto error;
        }

        Tast_expr* new_arg = NULL;

        if (lang_type_is_equal(lang_type_from_ulang_type(param->base->lang_type), lang_type_primitive_const_wrap(lang_type_any_const_wrap(lang_type_any_new(POS_BUILTIN, lang_type_atom_new_from_cstr("any", 0, 0)))))) {
            if (param->is_variadic) {
                // TODO: do type checking here if this function is not an extern "c" function
                for (size_t arg_idx = param_idx; arg_idx < fun_call->args.info.count; arg_idx++) {
                    Tast_expr* new_sub_arg = NULL;
                    if (!try_set_expr_types(&new_sub_arg, vec_at(&fun_call->args, arg_idx))) {
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
                
                &new_arg,
                lang_type_from_ulang_type(param->base->lang_type),
                corres_arg,
                uast_expr_get_pos(corres_arg)
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg_invalid_function_arg(new_arg, param->base);
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
        msg_invalid_count_function_args(fun_call, fun_decl, params->params.info.count, params->params.info.count);
        status = false;
        goto error;
    }

    *new_call = tast_function_call_wrap(tast_function_call_new(
        fun_call->pos,
        new_args,
        new_callee,
        fun_rtn_type
    ));

error:
    return status;
}

bool try_set_tuple_types(Tast_tuple** new_tuple, Uast_tuple* tuple) {
    Tast_expr_vec new_members = {0};
    Lang_type_vec new_lang_type = {0};

    for (size_t idx = 0; idx < tuple->members.info.count; idx++) {
        Tast_expr* new_memb = NULL;
        if (!try_set_expr_types(&new_memb, vec_at(&tuple->members, idx))) {
            return false;
        }
        vec_append(&a_main, &new_members, new_memb);
        vec_append(&a_main, &new_lang_type, tast_expr_get_lang_type(new_memb));
    }

    Pos pos = POS_BUILTIN;
    if (tuple->members.info.count > 0) {
        pos = lang_type_get_pos(vec_at(&new_lang_type, 0));
    }
    *new_tuple = tast_tuple_new(tuple->pos, new_members, lang_type_tuple_new(pos, new_lang_type));
    return true;
}

bool try_set_sum_access_types(Tast_sum_access** new_access, Uast_sum_access* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
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

bool try_set_sum_get_tag_types(Tast_sum_get_tag** new_access, Uast_sum_get_tag* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
        return false;
    }

    *new_access = tast_sum_get_tag_new(
        access->pos,
        new_callee
    );

    return true;
}

static void msg_invalid_member_internal(
    const char* file,
    int line,
    Name base_name,
    const Uast_member_access* access
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_INVALID_MEMBER_ACCESS, env.file_path_to_text,
        access->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(access->member_name->name.base), name_print(base_name)
    );
}

#define msg_invalid_member(base_name, access) \
    msg_invalid_member_internal(__FILE__, __LINE__, base_name, access)

bool try_set_member_access_types_finish_generic_struct(
    Tast_stmt** new_tast,
    Uast_member_access* access,
    Ustruct_def_base def_base,
    Tast_expr* new_callee
) {
    Uast_variable_def* member_def = NULL;
    if (!uast_try_get_member_def(&member_def, &def_base, access->member_name->name.base)) {
        msg_invalid_member(def_base.name, access);
        return false;
    }

    if (access->member_name->name.gen_args.info.count > 0) {
        todo();
    }

    Tast_member_access* new_access = tast_member_access_new(
        access->pos,
        lang_type_from_ulang_type(member_def->lang_type),
        access->member_name->name.base,
        new_callee
    );

    *new_tast = tast_expr_wrap(tast_member_access_wrap(new_access));

    return true;
}

bool try_set_member_access_types_finish_sum_def(
     
    Tast_stmt** new_tast,
    Uast_sum_def* sum_def,
    Uast_member_access* access,
    Tast_expr* new_callee
) {
    (void) new_callee;

    switch (env.parent_of) {
        case PARENT_OF_CASE: {
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &sum_def->base, access->member_name->name.base)) {
                msg_invalid_member(sum_def->base.name, access);
                return false;
            }

            Tast_enum_lit* new_tag = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&sum_def->base, access->member_name->name.base),
                lang_type_from_ulang_type(member_def->lang_type)
            );

            *new_tast = tast_expr_wrap(tast_sum_case_wrap(tast_sum_case_new(
                access->pos,
                new_tag,
                lang_type_sum_const_wrap(lang_type_sum_new(sum_def->pos, lang_type_atom_new(sum_def->base.name, 0)))
            )));

            return true;
        }
        case PARENT_OF_RETURN:
            todo();
        case PARENT_OF_BREAK:
            todo();
            // fallthrough
        case PARENT_OF_ASSIGN_RHS: {
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &sum_def->base, access->member_name->name.base)) {
                todo();
                //msg_invalid_member(enum_def->base, access);
                //return false;
            }
            
            Tast_enum_lit* new_tag = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&sum_def->base, access->member_name->name.base),
                lang_type_from_ulang_type(member_def->lang_type)
            );

            Tast_sum_callee* new_callee = tast_sum_callee_new(
                access->pos,
                new_tag,
                lang_type_sum_const_wrap(lang_type_sum_new(sum_def->pos, lang_type_atom_new(sum_def->base.name, 0)))
            );

            if (new_tag->lang_type.type != LANG_TYPE_VOID) {
                *new_tast = tast_expr_wrap(tast_sum_callee_wrap(new_callee));
                return true;
            }

            new_callee->tag->lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, 64, 0)));

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
        case PARENT_OF_IF:
            unreachable("");
        case PARENT_OF_NONE:
            unreachable("");
    }
    unreachable("");
}

bool try_set_member_access_types_finish(
    Tast_stmt** new_tast,
    Uast_def* lang_type_def,
    Uast_member_access* access,
    Tast_expr* new_callee
) {
    switch (lang_type_def->type) {
        case UAST_STRUCT_DEF: {
            Uast_struct_def* struct_def = uast_struct_def_unwrap(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                 new_tast, access, struct_def->base, new_callee
            );
        }
        case UAST_RAW_UNION_DEF: {
            Uast_raw_union_def* raw_union_def = uast_raw_union_def_unwrap(lang_type_def);
            return try_set_member_access_types_finish_generic_struct(
                 new_tast, access, raw_union_def->base, new_callee
            );
        }
        case UAST_ENUM_DEF: {
            Uast_enum_def* enum_def = uast_enum_def_unwrap(lang_type_def);
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name->name.base)) {
                msg_invalid_member(enum_def->base.name, access);
                return false;
            }

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                access->pos,
                uast_get_member_index(&enum_def->base, access->member_name->name.base),
                lang_type_from_ulang_type(member_def->lang_type)
            );

            *new_tast = tast_expr_wrap(tast_literal_wrap(tast_enum_lit_wrap(new_lit)));
            return true;
        }
        case UAST_SUM_DEF:
            return try_set_member_access_types_finish_sum_def(new_tast, uast_sum_def_unwrap(lang_type_def), access, new_callee);
        case UAST_PRIMITIVE_DEF:
            msg_invalid_member(lang_type_get_str(uast_primitive_def_unwrap(lang_type_def)->lang_type), access);
            return false;
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("lang def should have been eliminated by now");
    }
    unreachable("");
}

bool try_set_member_access_types(
    Tast_stmt** new_tast,
    Uast_member_access* access
) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
        return false;
    }

    switch (new_callee->type) {
        case TAST_SYMBOL: {
            Tast_symbol* sym = tast_symbol_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(sym->base.lang_type))) {
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MEMBER_ACCESS: {
            Tast_member_access* sym = tast_member_access_unwrap(new_callee);

            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(sym->lang_type))) {
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);
        }
        case TAST_OPERATOR: {
            Tast_operator* sym = tast_operator_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(tast_operator_get_lang_type(sym)))) {
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MODULE_ALIAS: {
            Uast_symbol* sym = uast_symbol_new(access->pos, name_new(
                tast_module_alias_unwrap(new_callee)->mod_path,
                access->member_name->name.base,
                access->member_name->name.gen_args,
                access->member_name->name.scope_id
            ));
            Tast_expr* new_expr = NULL;
            if (!try_set_symbol_types(&new_expr, sym)) {
                return false;
            }
            *new_tast = tast_expr_wrap(new_expr);
            return true;
        }
        default:
            unreachable(TAST_FMT, tast_expr_print(new_callee));
    }
    unreachable("");
}

bool try_set_index_untyped_types(Tast_stmt** new_tast, Uast_index* index) {
    Tast_expr* new_callee = NULL;
    Tast_expr* new_inner_index = NULL;
    if (!try_set_expr_types(&new_callee, index->callee)) {
        return false;
    }
    if (!try_set_expr_types(&new_inner_index, index->index)) {
        return false;
    }
    if (lang_type_get_bit_width(tast_expr_get_lang_type(new_inner_index)) <= 64) {
        unwrap(try_set_unary_types_finish(
            
            &new_inner_index,
            new_inner_index,
            tast_expr_get_pos(new_inner_index),
            UNARY_UNSAFE_CAST,
            lang_type_primitive_const_wrap(
                lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(POS_BUILTIN, 64, 0))
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

static bool try_set_condition_types(Tast_condition** new_cond, Uast_condition* cond) {
    Tast_expr* new_child_ = NULL;
    if (!try_set_operator_types(&new_child_, cond->child)) {
        vec_reset(&env.udefered_symbols_to_add);
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

bool try_set_primitive_def_types(Uast_primitive_def* tast) {
    (void) env;
    unwrap(symbol_add(tast_primitive_def_wrap(tast_primitive_def_new(tast->pos, tast->lang_type))));
    return true;
}

bool try_set_literal_def_types(Uast_literal_def* tast) {
    (void) env;
    (void) tast;
    unreachable("");
}

bool try_set_import_path_types(Tast_block** new_tast, Uast_import_path* tast) {
    return try_set_block_types(new_tast, tast->block, false, false);
}

bool try_set_variable_def_types(
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl,
    bool is_variadic
) {
    Uast_def* result = NULL;
    log(LOG_DEBUG, TAST_FMT, uast_variable_def_print(uast));
    if (usymbol_lookup(&result, uast->name) && result->type == UAST_POISON_DEF) {
        unwrap(error_count > 0);
        return false;
    }

    log(LOG_DEBUG, TAST_FMT"\n", name_print(uast->name));
    log(LOG_DEBUG, "%p\n", (void*)uast);
    Lang_type new_lang_type = {0};
    if (!try_lang_type_from_ulang_type(&new_lang_type, uast->lang_type, uast->pos)) {
        log(LOG_DEBUG, TAST_FMT"\n", lang_type_print(LANG_TYPE_MODE_MSG, new_lang_type));
        Uast_poison_def* new_poison = uast_poison_def_new(uast->pos, uast->name);
        usymbol_update(uast_poison_def_wrap(new_poison));
        return false;
    }

    *new_tast = tast_variable_def_new(uast->pos, new_lang_type, is_variadic, uast->name);
    if (add_to_sym_tbl && !env.type_checking_is_in_struct_base_def) {
        symbol_add(tast_variable_def_wrap(*new_tast));
    }
    return true;
}

bool try_set_function_decl_types(
    Tast_function_decl** new_tast,
    Uast_function_decl* decl,
    bool add_to_sym_tbl
) {
    Tast_function_params* new_params = NULL;
    if (!try_set_function_params_types(&new_params, decl->params, add_to_sym_tbl)) {
        Uast_poison_def* poison = uast_poison_def_new(decl->pos, decl->name);
        usymbol_update(uast_poison_def_wrap(poison));
        return false;
    }

    Lang_type fun_rtn_type = lang_type_from_ulang_type(decl->return_type);
    *new_tast = tast_function_decl_new(decl->pos, new_params, fun_rtn_type, decl->name);
    unwrap(sym_tbl_add(tast_function_decl_wrap(*new_tast)));

    return true;
}

bool try_set_function_params_types(
    Tast_function_params** new_tast,
    Uast_function_params* params,
    bool add_to_sym_tbl
) {
    bool status = true;

    Tast_variable_def_vec new_params = {0};
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Uast_param* def = vec_at(&params->params, idx);

        Tast_variable_def* new_def = NULL;
        if (try_set_variable_def_types(&new_def, def->base, add_to_sym_tbl, def->is_variadic)) {
            vec_append(&a_main, &new_params, new_def);
        } else {
            status = false;
        }
    }

    *new_tast = tast_function_params_new(params->pos, new_params);
    return status;
}

bool try_set_return_types(Tast_return** new_tast, Uast_return* rtn) {
    bool status = true;
    PARENT_OF old_parent_of = env.parent_of;
    env.parent_of = PARENT_OF_RETURN;

    Tast_expr* new_child = NULL;
    switch (check_generic_assignment(&new_child, lang_type_from_ulang_type(env.parent_fn_rtn_type), rtn->child, rtn->pos)) {
        case CHECK_ASSIGN_OK:
            break;
        case CHECK_ASSIGN_INVALID:
            msg_invalid_return_type(rtn->pos, new_child, rtn->is_auto_inserted);
            status = false;
            goto error;
        case CHECK_ASSIGN_ERROR:
            status = false;
            goto error;
        default:
            unreachable("");
    }

    *new_tast = tast_return_new(rtn->pos, new_child, rtn->is_auto_inserted);

error:
    env.parent_of = old_parent_of;
    return status;
}

bool try_set_break_types(Tast_break** new_tast, Uast_break* lang_break) {
    bool status = true;
    PARENT_OF old_parent_of = env.parent_of;
    env.parent_of = PARENT_OF_BREAK;

    Tast_expr* new_child = NULL;
    if (lang_break->do_break_expr) {
        switch (check_generic_assignment(&new_child, env.break_type, lang_break->break_expr, lang_break->pos)) {
            case CHECK_ASSIGN_OK:
                log(LOG_DEBUG, TAST_FMT, uast_expr_print(lang_break->break_expr));
                break;
            case CHECK_ASSIGN_INVALID:
                msg_invalid_yield_type(lang_break->pos, new_child, false);
                status = false;
                goto error;
            case CHECK_ASSIGN_ERROR:
                todo();
                status = false;
                goto error;
            default:
                unreachable("");
        }
    }

    *new_tast = tast_break_new(lang_break->pos, lang_break->do_break_expr, new_child);

    env.break_in_case = true;
error:
    env.parent_of = old_parent_of;
    return status;
}

bool try_set_for_with_cond_types(Tast_for_with_cond** new_tast, Uast_for_with_cond* uast) {
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(&new_cond, uast->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(&new_body, uast->body, false, true)) {
        status = false;
    }

    *new_tast = tast_for_with_cond_new(uast->pos, new_cond, new_body, uast->continue_label, uast->do_cont_label);
    return status;
}

bool try_set_if_types(Tast_if** new_tast, Uast_if* uast) {
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(&new_cond, uast->condition)) {
        status = false;
    }

    Uast_stmt_vec new_if_children = {0};
    vec_extend(&a_main, &new_if_children, &env.switch_case_defer_add_sum_case_part);
    vec_extend(&a_main, &new_if_children, &env.switch_case_defer_add_if_true);
    vec_extend(&a_main, &new_if_children, &uast->body->children);
    uast->body->children = new_if_children;
    vec_reset(&env.switch_case_defer_add_sum_case_part);
    vec_reset(&env.switch_case_defer_add_if_true);

    Tast_block* new_body = NULL;
    if (!(status && try_set_block_types(&new_body, uast->body, false, true))) {
        status = false;
    }

    //log(LOG_DEBUG, TAST_FMT, uast_if_print(uast));
    //todo();

    if (status) {
        *new_tast = tast_if_new(uast->pos, new_cond, new_body, new_body->lang_type);
        if (env.parent_of == PARENT_OF_CASE) {
            if (new_body->lang_type.type != LANG_TYPE_VOID && !env.break_in_case) {
                // TODO: this will not work if there is nested switch or if-else
                msg_invalid_yield_type(new_body->pos_end, NULL, true);
                status = false;
            }
        } else if (env.parent_of == PARENT_OF_IF) {
            todo();
        }
    }
    return status;
}

bool try_set_if_else_chain(Tast_if_else_chain** new_tast, Uast_if_else_chain* if_else) {
    bool status = true;

    Tast_if_vec new_ifs = {0};

    for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
        Uast_if* old_if = vec_at(&if_else->uasts, idx);
                
        Tast_if* new_if = NULL;
        if (!try_set_if_types(&new_if, old_if)) {
            status = false;
        }

        vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(if_else->pos, new_ifs, false);
    return status;
}

bool try_set_case_types(Tast_if** new_tast, const Uast_case* lang_case) {
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

static Exhaustive_data check_for_exhaustiveness_start(Lang_type oper_lang_type) {
    Exhaustive_data exhaustive_data = {0};

    exhaustive_data.oper_lang_type = oper_lang_type;

    Uast_def* enum_def_ = NULL;
    if (!usymbol_lookup(&enum_def_, lang_type_get_str(exhaustive_data.oper_lang_type))) {
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
    Exhaustive_data* exhaustive_data,
    const Tast_if* curr_if,
    bool is_default
) {
    if (is_default) {
        if (exhaustive_data->default_is_pre) {
            msg(
                LOG_ERROR, EXPECT_FAIL_DUPLICATE_DEFAULT, env.file_path_to_text, curr_if->pos,
                "duplicate default in switch statement\n"
            );
            return false;
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
                unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(exhaustive_data->oper_lang_type)));
                Uast_enum_def* enum_def = uast_enum_def_unwrap(enum_def_);
                msg(
                    LOG_ERROR, EXPECT_FAIL_DUPLICATE_CASE, env.file_path_to_text, curr_if->pos,
                    "duplicate case `"STR_VIEW_FMT"."STR_VIEW_FMT"` in switch statement\n",
                    name_print(enum_def->base.name), name_print(vec_at(&enum_def->base.members, (size_t)curr_lit->data)->name)
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
                unwrap(usymbol_lookup(&sum_def_, lang_type_get_str(exhaustive_data->oper_lang_type)));
                Uast_sum_def* sum_def = uast_sum_def_unwrap(sum_def_);
                msg(
                    LOG_ERROR, EXPECT_FAIL_DUPLICATE_CASE, env.file_path_to_text, curr_if->pos,
                    "duplicate case `"STR_VIEW_FMT"."STR_VIEW_FMT"` in switch statement\n",
                    name_print(sum_def->base.name), name_print(vec_at(&sum_def->base.members, (size_t)curr_lit->data)->name)
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

static bool check_for_exhaustiveness_finish(Exhaustive_data exhaustive_data, Pos pos_switch) {
        unwrap(exhaustive_data.covered.info.count == exhaustive_data.max_data + 1);

        if (exhaustive_data.default_is_pre) {
            return true;
        }

        bool status = true;
        String string = {0};

        for (size_t idx = 0; idx < exhaustive_data.covered.info.count; idx++) {
            if (!vec_at(&exhaustive_data.covered, idx)) {
                Uast_def* enum_def_ = NULL;
                unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(exhaustive_data.oper_lang_type)));
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

                extend_name(false, &string, enum_def.name);
                string_extend_cstr(&a_main, &string, ".");
                extend_name(false, &string, vec_at(&enum_def.members, idx)->name);
            }
        }

        if (!status) {
            msg(
                LOG_ERROR, EXPECT_FAIL_NON_EXHAUSTIVE_SWITCH, env.file_path_to_text, pos_switch,
                STR_VIEW_FMT"\n", string_print(string)
            );
        }
        return status;
}

bool try_set_switch_types(Tast_if_else_chain** new_tast, const Uast_switch* lang_switch) {
    Tast_if_vec new_ifs = {0};

    Tast_expr* new_operand = NULL;
    if (!try_set_expr_types(&new_operand, lang_switch->operand)) {
        return false;
    }

    bool status = true;
    PARENT_OF old_parent_of = env.parent_of;
    env.break_in_case = false;
    if (env.parent_of == PARENT_OF_ASSIGN_RHS) {
        env.break_type = env.lhs_lang_type;
    } else {
        env.break_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
    }

    Exhaustive_data exhaustive_data = check_for_exhaustiveness_start(
         tast_expr_get_lang_type(new_operand)
    );

    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        Uast_case* old_case = vec_at(&lang_switch->cases, idx);
        Uast_condition* cond = NULL;

        Uast_expr* operand = NULL;

        switch (tast_expr_get_lang_type(new_operand).type) {
            case LANG_TYPE_SUM:
                operand = uast_sum_get_tag_wrap(uast_sum_get_tag_new(
                    uast_expr_get_pos(lang_switch->operand), lang_switch->operand
                ));
                break;
            case LANG_TYPE_ENUM:
                operand = lang_switch->operand;
                break;
            default:
                unreachable(TAST_FMT, uast_expr_print(lang_switch->operand));
                todo();
        }

        if (old_case->is_default) {
            cond = uast_condition_new(
                old_case->pos,
                uast_condition_get_default_child(uast_literal_wrap(
                    util_uast_literal_new_from_int64_t(1, TOKEN_INT_LITERAL, old_case->pos)
                ))
            );
        } else {
            cond = uast_condition_new(old_case->pos, uast_binary_wrap(uast_binary_new(
                old_case->pos, operand, old_case->expr, BINARY_DOUBLE_EQUAL
            )));
        }

        todo();
        //vec_append(&a_main, &env.switch_case_defer_add_if_true, old_case->if_true);
        //Uast_block* if_true = uast_block_new(
        //    old_case->pos,
        //    (Uast_stmt_vec) {0},
        //    old_case->pos,
        //    scope_id_new()
        //);
                
        todo();
        //env.parent_of = PARENT_OF_CASE;
        //env.parent_of_operand = lang_switch->operand;
        //Tast_if* new_if = NULL;
        //if (!try_set_if_types(&new_if, uast_if_new(old_case->pos, cond, if_true))) {
        //    status = false;
        //    goto error_inner;
        //}

error_inner:
        env.parent_of_operand = NULL;
        env.parent_of = PARENT_OF_NONE;
        env.break_in_case = false;
        if (!status) {
            goto error;
        }

        todo();
        //if (!check_for_exhaustiveness_inner(&exhaustive_data, new_if, old_case->is_default)) {
        //    status = false;
        //    goto error;
        //}

        todo();
        //vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(lang_switch->pos, new_ifs, true);
    if (!check_for_exhaustiveness_finish(exhaustive_data, lang_switch->pos)) {
        status = false;
        goto error;
    }

error:
    env.parent_of = old_parent_of;
    env.break_in_case = false;
    return status;
}

bool try_set_label_types(Tast_label** new_tast, const Uast_label* lang_label) {
    *new_tast = tast_label_new(lang_label->pos, name_new(env.curr_mod_path, lang_label->name, (Ulang_type_vec) {0}, 0 /* TODO */));
    return lang_label;
}

// TODO: merge this with msg_redefinition_of_symbol?
static void try_set_msg_redefinition_of_symbol(const Uast_def* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, env.file_path_to_text, uast_def_get_pos(new_sym_def),
        "redefinition of symbol "STR_VIEW_FMT"\n", name_print(uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, env.file_path_to_text, uast_def_get_pos(original_def),
        STR_VIEW_FMT " originally defined here\n", name_print(uast_def_get_name(original_def))
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

bool try_set_block_types(Tast_block** new_tast, Uast_block* block, bool is_directly_in_fun_def, bool new_sym_tbl) {
    do_test_bit_width();

    bool status = true;

    Tast_stmt_vec new_tasts = {0};

    // TODO: remove this variable?
    Tast_stmt_vec aux_stmts = {0};

    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        if (curr->type != UAST_VARIABLE_DEF && curr->type != UAST_IMPORT_PATH) {
            // TODO: eventually, we should do also function defs, etc. in this for loop
            // (change parser to not put function defs, etc. in block)
            continue;
        }

        Tast_stmt* new_node = NULL;
        switch (try_set_def_types(&new_node, curr)) {
            case STMT_NO_STMT:
                break;
            case STMT_ERROR:
                status = false;
                break;
            case STMT_OK:
                vec_append(&a_main, &aux_stmts, new_node);
                break;
            default:
                unreachable("");
        }
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Uast_stmt* curr_tast = vec_at(&block->children, idx);
        Tast_stmt* new_tast = NULL;
        switch (try_set_stmt_types(&new_tast, curr_tast)) {
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
                unreachable("");
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
        switch (try_set_stmt_types(&new_rtn_statement, uast_return_wrap(rtn_statement))) {
            case STMT_ERROR:
                status = false;
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

    if (block->scope_id == SCOPE_TOP_LEVEL) {
        Uast_def* main_fn_ = NULL;
        if (!usymbol_lookup(&main_fn_, name_new((Str_view) {0}, str_view_from_cstr("main"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL))) {
            log(LOG_WARNING, "no main function\n");
            goto after_main;
        }
        if (main_fn_->type != UAST_FUNCTION_DEF) {
            todo();
        }
        log(LOG_DEBUG, "def->decl->name.scope_id: %zu\n", uast_function_def_unwrap(main_fn_)->decl->name.scope_id);
        Uast_function_def* new_def = NULL;
        if (!resolve_generics_function_def(&new_def, uast_function_def_unwrap(main_fn_), (Ulang_type_vec) {0}, (Pos) {0})) {
            status = false;
        }
    }
after_main:
    assert(true /* TODO: remove */);

error:
    assert(true /* TODO: remove */);
    Lang_type yield_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
    assert(yield_type.type == LANG_TYPE_VOID);
    if (env.parent_of == PARENT_OF_CASE) {
        yield_type = env.break_type;
    } else if (env.parent_of == PARENT_OF_IF) {
        todo();
    }
    *new_tast = tast_block_new(block->pos, new_tasts, block->pos_end, yield_type, block->scope_id);
    if (status) {
        assert(*new_tast);
    } else {
        assert(error_count > 0);
    }
    return status;
}

STMT_STATUS try_set_stmt_types(Tast_stmt** new_tast, Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Tast_expr* new_tast_ = NULL;
            if (!try_set_expr_types(&new_tast_, uast_expr_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_expr_wrap(new_tast_);
            return STMT_OK;
        }
        case UAST_DEF:
            return try_set_def_types(new_tast, uast_def_unwrap(stmt));
        case UAST_FOR_WITH_COND: {
            Tast_for_with_cond* new_tast_ = NULL;
            if (!try_set_for_with_cond_types(&new_tast_, uast_for_with_cond_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_for_with_cond_wrap(new_tast_);
            return STMT_OK;
        }
        case UAST_ASSIGNMENT: {
            Tast_assignment* new_tast_ = NULL;
            if (!try_set_assignment_types(&new_tast_, uast_assignment_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_expr_wrap(tast_assignment_wrap(new_tast_));
            return STMT_OK;
        }
        case UAST_RETURN: {
            Tast_return* new_rtn = NULL;
            if (!try_set_return_types(&new_rtn, uast_return_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_return_wrap(new_rtn);
            return STMT_OK;
        }
        case UAST_BREAK: {
            Tast_break* new_break = NULL;
            if (!try_set_break_types(&new_break, uast_break_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_break_wrap(new_break);
            return STMT_OK;
        }
        case UAST_CONTINUE:
            *new_tast = tast_continue_wrap(tast_continue_new(uast_continue_unwrap(stmt)->pos));
            return STMT_OK;
        case UAST_BLOCK: {
            assert(uast_block_unwrap(stmt)->pos_end.line > 0);
            Tast_block* new_for = NULL;
            if (!try_set_block_types(&new_for, uast_block_unwrap(stmt), false, true)) {
                return STMT_ERROR;
            }
            *new_tast = tast_block_wrap(new_for);
            return STMT_OK;
        }
        case UAST_LABEL: {
            Tast_label* new_label = NULL;
            if (!try_set_label_types(&new_label, uast_label_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_label_wrap(new_label);
            return STMT_OK;
        }
    }
    unreachable("");
}

bool try_set_types(Tast_block** new_tast, Uast_block* block) {
    return try_set_block_types(new_tast, block, false, true);
}
