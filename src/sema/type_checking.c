#include <type_checking.h>
#include <parser_utils.h>
#include <uast_utils.h>
#include <uast.h>
#include <tast.h>
#include <tast_utils.h>
#include <ir_utils.h>
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
#include <lang_type_get_pos.h>
#include <lang_type_is.h>
#include <symbol_iter.h>
#include <expand_lang_def.h>
#include <ulang_type_serialize.h>
#include <symbol_table.h>
#include <msg_undefined_symbol.h>
#include <pos_vec.h>
#include <check_struct_recursion.h>

typedef enum {
    PARENT_OF_NONE = 0,
    PARENT_OF_CASE,
    PARENT_OF_ASSIGN_RHS,
    PARENT_OF_RETURN,
    PARENT_OF_BREAK,
    PARENT_OF_IF,
} PARENT_OF;

typedef enum {
    PARENT_OF_DEFER_NONE = 0,
    PARENT_OF_DEFER_DEFER,
    PARENT_OF_DEFER_FOR,
} PARENT_OF_DEFER;

static Strv parent_of_print_internal(PARENT_OF parent_of) {
    switch (parent_of) {
        case PARENT_OF_NONE:
            return sv("PARENT_OF_NONE");
        case PARENT_OF_CASE:
            return sv("PARENT_OF_CASE");
        case PARENT_OF_ASSIGN_RHS:
            return sv("PARENT_OF_ASSIGN_RHS");
        case PARENT_OF_RETURN:
            return sv("PARENT_OF_RETURN");
        case PARENT_OF_BREAK:
            return sv("PARENT_OF_BREAK");
        case PARENT_OF_IF:
            return sv("PARENT_OF_IF");
    }
    unreachable("");
}

#define parent_of_print(parent_of) strv_print(parent_of_print_internal(parent_of))

static int dummy_int;

static Lang_type break_type;

static bool break_in_case;

static Uast_stmt_vec switch_case_defer_add_if_true;
static Uast_stmt_vec switch_case_defer_add_enum_case_part;

static Lang_type lhs_lang_type;

static void try_set_msg_redefinition_of_symbol(const Uast_def* new_sym_def);

static PARENT_OF parent_of;
static Uast_expr* parent_of_operand;

static PARENT_OF_DEFER parent_of_defer;

static bool is_in_struct_base_def;

static bool is_in_defer;
static Pos parent_defer_pos;

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

// TODO: actually implement this, or require user to specify double in literal suffix, etc.
static int64_t bit_width_needed_float(double num) {
    (void) num;
    return 32;
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

static bool can_be_implicitly_converted_lang_type_primitive(Lang_type_primitive dest, Lang_type_primitive src, bool src_is_zero, bool implicit_pointer_depth) {
    if (!implicit_pointer_depth) {
        if (lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, src) != lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, dest)) {
            return false;
        }
    }

    if (!lang_type_primitive_is_number(dest) || !lang_type_primitive_is_number(src)) {
        return false;
    }

    int32_t dest_bit_width = lang_type_primitive_get_bit_width(dest);
    int32_t src_bit_width = lang_type_primitive_get_bit_width(src);

    // both or none of types must be float
    if (dest.type == LANG_TYPE_FLOAT && src.type != LANG_TYPE_FLOAT) {
        return false;
    }
    if (dest.type != LANG_TYPE_FLOAT && src.type == LANG_TYPE_FLOAT) {
        return false;
    }

    if (dest.type == LANG_TYPE_UNSIGNED_INT && src.type != LANG_TYPE_UNSIGNED_INT) {
        return false;
    }

    if (src_is_zero) {
        return true;
    }

    if (dest.type == LANG_TYPE_SIGNED_INT) {
        unwrap(dest_bit_width > 0);
        dest_bit_width--;
    }
    if (src.type == LANG_TYPE_SIGNED_INT) {
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
            return can_be_implicitly_converted_lang_type_primitive(
                lang_type_primitive_const_unwrap(dest),
                lang_type_primitive_const_unwrap(src),
                src_is_zero,
                implicit_pointer_depth
            );
        case LANG_TYPE_ENUM:
            return false; // TODO
        case LANG_TYPE_STRUCT:
            return false; // TODO
        case LANG_TYPE_RAW_UNION:
            return false; // TODO
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
    const Uast_variable_def* corres_param,
    bool is_fun_callback
) {
    if (is_fun_callback) {
        msg_internal(
            file, line,
            DIAG_INVALID_FUN_ARG, tast_expr_get_pos(argument), 
            "argument is of type `"FMT"`, "
            "but the corresponding parameter is of type `"FMT"`\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(argument)), 
            lang_type_print(LANG_TYPE_MODE_MSG, lang_type_from_ulang_type(corres_param->lang_type))
        );
        msg_internal(
            file, line,
            DIAG_NOTE, corres_param->pos,
            "function callback type defined here\n"
        );
    } else {
        msg_internal(
            file, line,
            DIAG_INVALID_FUN_ARG, tast_expr_get_pos(argument), 
            "argument is of type `"FMT"`, "
            "but the corresponding parameter `"FMT"` is of type `"FMT"`\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(argument)), 
            name_print(NAME_MSG, corres_param->name),
            lang_type_print(LANG_TYPE_MODE_MSG, lang_type_from_ulang_type(corres_param->lang_type))
        );
        msg_internal(
            file, line,
            DIAG_NOTE, corres_param->pos,
            "corresponding parameter `"FMT"` defined here\n",
            name_print(NAME_MSG, corres_param->name)
        );
    }
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
    string_extend_size_t(&a_print, &message, fun_call->args.info.count);
    string_extend_cstr(&a_print, &message, " arguments are passed to function `");
    extend_name(NAME_MSG, &message, fun_decl->name);
    string_extend_cstr(&a_print, &message, "`, but ");
    string_extend_size_t(&a_print, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&a_print, &message, " or more");
    }
    string_extend_cstr(&a_print, &message, " arguments expected\n");
    msg_internal(
        file, line, DIAG_INVALID_COUNT_FUN_ARGS, fun_call->pos,
        FMT, strv_print(string_to_strv(message))
    );

    msg_internal(
        file, line, DIAG_NOTE, uast_function_decl_get_pos(fun_decl),
        "function `"FMT"` defined here\n", name_print(NAME_MSG, fun_decl->name)
    );
}

#define msg_invalid_function_arg(argument, corres_param, is_fun_callback) \
    msg_invalid_function_arg_internal(__FILE__, __LINE__, argument, corres_param, is_fun_callback)

#define msg_invalid_count_function_args(fun_call, fun_decl, min_args, max_args) \
    msg_invalid_count_function_args_internal(__FILE__, __LINE__, fun_call, fun_decl, min_args, max_args)

static void msg_invalid_count_struct_literal_args_internal(
    const char* file,
    int line,
    Uast_expr_vec membs,
    size_t min_args,
    size_t max_args,
    Pos pos
) {
    String message = {0};
    string_extend_size_t(&a_print, &message, membs.info.count);
    string_extend_cstr(&a_print, &message, " arguments are passed to struct literal, but ");
    string_extend_size_t(&a_print, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&a_print, &message, " or more");
    }
    string_extend_cstr(&a_print, &message, " arguments expected\n");
    msg_internal(
        file, line, DIAG_INVALID_COUNT_STRUCT_LIT_ARGS, pos,
        FMT, strv_print(string_to_strv(message))
    );
}

#define msg_invalid_count_struct_literal_args(membs, min_args, max_args, pos) \
    msg_invalid_count_struct_literal_args_internal(__FILE__, __LINE__, membs, min_args, max_args, pos)

static void msg_invalid_yield_type_internal(const char* file, int line, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            DIAG_MISSING_YIELD_STATEMENT, pos,
            "no break statement in case that breaks `"FMT"`\n",
            lang_type_print(LANG_TYPE_MODE_MSG, break_type)
        );
    } else {
        msg_internal(
            file, line,
            DIAG_MISMATCHED_YIELD_TYPE, pos,
            "breaking `"FMT"`, but type `"FMT"` expected\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(child)),
            lang_type_print(LANG_TYPE_MODE_MSG, break_type)
        );
    }

    msg_internal(
        file, line,
        DIAG_NOTE, lang_type_get_pos(break_type),
        "case break type `"FMT"` defined here\n",
        lang_type_print(LANG_TYPE_MODE_MSG, break_type) 
    );
}

static void msg_invalid_return_type_internal(const char* file, int line, Pos pos, const Tast_expr* child, bool is_auto_inserted) {
    if (is_auto_inserted) {
        msg_internal(
            file, line,
            DIAG_MISSING_RETURN, pos,
            "no return statement in function that returns `"FMT"`\n",
            ulang_type_print(LANG_TYPE_MODE_MSG, env.parent_fn_rtn_type)
        );
    } else {
        msg_internal(
            file, line,
            DIAG_MISMATCHED_RETURN_TYPE, pos,
            "returning `"FMT"`, but type `"FMT"` expected\n",
            lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(child)), 
            ulang_type_print(LANG_TYPE_MODE_MSG, env.parent_fn_rtn_type)
        );
    }

    msg_internal(
        file, line,
        DIAG_NOTE, ulang_type_get_pos(env.parent_fn_rtn_type),
        "function return type `"FMT"` defined here\n",
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
        Lang_type old_lhs_lang_type = lhs_lang_type;
        PARENT_OF old_parent_of = parent_of;
        parent_of = PARENT_OF_ASSIGN_RHS;
        lhs_lang_type = dest_lang_type;
        if (!try_set_expr_types(new_src, src)) {
            lhs_lang_type = old_lhs_lang_type;
            parent_of = old_parent_of;
            return CHECK_ASSIGN_ERROR;
        }
        lhs_lang_type = old_lhs_lang_type;
        parent_of = old_parent_of;
    }

    bool src_is_zero = false;
    if (src->type == UAST_LITERAL && uast_literal_unwrap(src)->type == UAST_INT && uast_int_unwrap(uast_literal_unwrap(src))->data == 0) {
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
                old_string->data
            ));
        }
        case UAST_INT: {
            Uast_int* old_number = uast_int_unwrap(literal);
            if (old_number->data < 0) {
                int64_t bit_width = bit_width_needed_signed(old_number->data);
                return tast_int_wrap(tast_int_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(old_number->pos, bit_width, 0)))
                ));
            } else {
                int64_t bit_width = bit_width_needed_unsigned(old_number->data);
                return tast_int_wrap(tast_int_new(
                    old_number->pos,
                    old_number->data,
                    lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(old_number->pos, bit_width, 0)))
                ));
            }
        }
        case UAST_FLOAT: {
            Uast_float* old_number = uast_float_unwrap(literal);
            int64_t bit_width = bit_width_needed_float(old_number->data);
            return tast_float_wrap(tast_float_new(
                old_number->pos,
                old_number->data,
                lang_type_primitive_const_wrap(lang_type_float_const_wrap(lang_type_float_new(old_number->pos, bit_width, 0)))
            ));
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
    }
    unreachable("");
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
            msg_undefined_symbol(sym_untyped);
            return false;
        }
    }

    switch (sym_def->type) {
        case UAST_FUNCTION_DECL: {
            function_decl_tbl_add(uast_function_decl_unwrap(sym_def));
            Uast_function_decl* new_decl = uast_function_decl_unwrap(sym_def);
            *new_tast = tast_literal_wrap(tast_function_lit_wrap(tast_function_lit_new(
                sym_untyped->pos,
                new_decl->name,
                lang_type_from_ulang_type(ulang_type_from_uast_function_decl(new_decl))
            )));
            return true;
        }
        case UAST_FUNCTION_DEF: {
            Lang_type_fn new_lang_type = {0};
            Name new_name = {0};
            if (!resolve_generics_function_def_call(&new_lang_type, &new_name, uast_function_def_unwrap(sym_def), sym_untyped->name.gen_args, sym_untyped->pos)) {
                return false;
            }
            *new_tast = tast_literal_wrap(tast_function_lit_wrap(tast_function_lit_new(
                sym_untyped->pos,
                new_name,
                lang_type_fn_const_wrap(new_lang_type)
            )));
            return true;
        }
        case UAST_STRUCT_DEF:
            // fallthrough
        case UAST_ENUM_DEF:
            // fallthrough
        case UAST_RAW_UNION_DEF:
            // fallthrough
        case UAST_PRIMITIVE_DEF:
            // fallthrough
        case UAST_VOID_DEF:
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
        case UAST_LABEL:
            // TODO
            todo();
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

static bool precalulate_float_internal(double* result, double lhs_val, double rhs_val, BINARY_TYPE token_type, Pos pos) {
    switch (token_type) {
        case BINARY_SINGLE_EQUAL:
            unreachable("");
        case BINARY_ADD:
            *result = lhs_val + rhs_val;
            return true;
        case BINARY_SUB:
            *result = lhs_val - rhs_val;
            return true;
        case BINARY_MULTIPLY:
            *result = lhs_val*rhs_val;
            return true;
        case BINARY_DIVIDE:
            *result = lhs_val/rhs_val;
            return true;
        case BINARY_LESS_THAN:
            *result = lhs_val < rhs_val ? 1 : 0;
            return true;
        case BINARY_GREATER_THAN:
            *result = lhs_val > rhs_val ? 1 : 0;
            return true;
        case BINARY_LESS_OR_EQUAL:
            *result = lhs_val <= rhs_val ? 1 : 0;
            return true;
        case BINARY_GREATER_OR_EQUAL:
            *result = lhs_val >= rhs_val ? 1 : 0;
            return true;
        case BINARY_MODULO:
            // fallthrough
        case BINARY_NOT_EQUAL:
            // fallthrough
        case BINARY_DOUBLE_EQUAL:
            // fallthrough
        case BINARY_BITWISE_XOR:
            // fallthrough
        case BINARY_BITWISE_AND:
            // fallthrough
        case BINARY_BITWISE_OR:
            // fallthrough
        case BINARY_LOGICAL_AND:
            // fallthrough
        case BINARY_LOGICAL_OR:
            // fallthrough
        case BINARY_SHIFT_LEFT:
            // fallthrough
        case BINARY_SHIFT_RIGHT:
            msg(DIAG_BINARY_MISMATCHED_TYPES, pos, "floating point operand for operation `"FMT"` is not supported\n", binary_type_print(token_type));
            return false;
    }
    unreachable("");
}

static Tast_literal* precalulate_number(
    const Tast_int* lhs,
    const Tast_int* rhs,
    BINARY_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->data, rhs->data, token_type);
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_INT_LITERAL, pos);
}

static bool precalulate_float(
    Tast_literal** result,
    const Tast_float* lhs,
    const Tast_float* rhs,
    BINARY_TYPE token_type,
    Pos pos
) {
    double result_val = 0;
    if (!precalulate_float_internal(&result_val, lhs->data, rhs->data, token_type, pos)) {
        return false;
    }
    *result = util_tast_literal_new_from_double(result_val, pos);
    return true;
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

static Tast_literal* precalulate_enum_lit(
    const Tast_enum_lit* lhs,
    const Tast_enum_lit* rhs,
    BINARY_TYPE token_type,
    Pos pos
) {
    int64_t result_val = precalulate_number_internal(lhs->tag->data, rhs->tag->data, token_type);
    return util_tast_literal_new_from_int64_t(result_val, TOKEN_CHAR_LITERAL, pos);
}

bool try_set_binary_types_finish(Tast_expr** new_tast, Tast_expr* new_lhs, Tast_expr* new_rhs, Pos oper_pos, BINARY_TYPE oper_token_type) {
    bool src_is_zero = 
        new_rhs->type == TAST_LITERAL &&
        tast_literal_unwrap(new_rhs)->type == TAST_INT &&
        tast_int_unwrap(tast_literal_unwrap(new_rhs))->data == 0;

    if (!lang_type_is_equal(tast_expr_get_lang_type(new_lhs), tast_expr_get_lang_type(new_rhs))) {
        if (can_be_implicitly_converted(
            tast_expr_get_lang_type(new_lhs),
            tast_expr_get_lang_type(new_rhs),
            src_is_zero,
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
            src_is_zero,
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
                DIAG_BINARY_MISMATCHED_TYPES, oper_pos,
                "types `"FMT"` and `"FMT"` are not valid operands to binary expression\n",
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_lhs)),
                lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_rhs))
            );
            return false;
        }
    }
            
    // precalcuate binary in some situations
    if (new_lhs->type == TAST_LITERAL && new_rhs->type == TAST_LITERAL) {
        Tast_literal* lhs_lit = tast_literal_unwrap(new_lhs);
        Tast_literal* rhs_lit = tast_literal_unwrap(new_rhs);

        if (lhs_lit->type != rhs_lit->type) {
            unreachable("this error should have been caught earlier\n");
        }

        Tast_literal* literal = NULL;

        switch (lhs_lit->type) {
            case TAST_INT:
                literal = precalulate_number(
                    tast_int_const_unwrap(lhs_lit),
                    tast_int_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            case TAST_FLOAT:
                if (!precalulate_float(
                    &literal,
                    tast_float_const_unwrap(lhs_lit),
                    tast_float_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                )) {
                    return false;
                }
                break;
            case TAST_CHAR:
                literal = precalulate_char(
                    tast_char_const_unwrap(lhs_lit),
                    tast_char_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            case TAST_ENUM_LIT: {
                Tast_enum_lit* lhs = tast_enum_lit_unwrap(lhs_lit);
                Tast_enum_lit* rhs = tast_enum_lit_unwrap(rhs_lit);
                if (!lang_type_is_equal(lhs->enum_lang_type, rhs->enum_lang_type)) {
                    // binary operators with mismatched enum types
                    return false;
                }

                Uast_def* enum_def_ = NULL;
                unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, lhs->enum_lang_type)));
                if (!ulang_type_is_equal(
                    vec_at(&uast_enum_def_unwrap(enum_def_)->base.members, (size_t)lhs->tag->data)->lang_type,
                    lang_type_to_ulang_type(lang_type_void_const_wrap(lang_type_void_new(lhs->pos)))
                )) {
                    // overloaded binary operators not defined for non-void inner types of enum
                    msg_todo("overloaded binary operators for non-void inner types of enum", lhs->pos);
                    todo();
                }
                literal = precalulate_enum_lit(
                    tast_enum_lit_const_unwrap(lhs_lit),
                    tast_enum_lit_const_unwrap(rhs_lit),
                    oper_token_type,
                    oper_pos
                );
                break;
            }
            default:
                unreachable("");
        }

        *new_tast = tast_literal_wrap(literal);
    } else {
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
                    lang_type_new_u1()
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
                        lang_type_new_u1()
                    ))),
                    tast_operator_wrap(tast_binary_wrap(tast_binary_new(
                        tast_expr_get_pos(new_rhs),
                        tast_literal_wrap(new_lit_rhs),
                        new_rhs,
                        BINARY_NOT_EQUAL,
                        lang_type_new_u1()
                    ))),
                    oper_token_type,
                    lang_type_new_u1()
                )));
                break;
            }
            default:
                unreachable(FMT, binary_type_print(oper_token_type));
        }

    }

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
                    DIAG_ASSIGNMENT_MISMATCHED_TYPES, 
                    operator->pos,
                    "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
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

    lhs_lang_type = tast_expr_get_lang_type(new_lhs);
    if (!try_set_expr_types(&new_rhs, operator->rhs)) {
        return false;
    }

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
                    DIAG_DEREF_NON_POINTER, unary_pos,
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
        case UNARY_SIZEOF:
            *new_tast = tast_operator_wrap(tast_unary_wrap(tast_unary_new(
                unary_pos,
                new_child,
                unary_token_type,
                lang_type_new_usize()
            )));
            return true;
        case UNARY_COUNTOF: {
            Lang_type_atom atom = {0};
            //if (lang_type.type != ULANG_TYPE_REGULAR) {
            //    msg(
            //        DIAG_INVALID_COUNTOF, unary_pos,
            //        "type `"FMT"` is not a valid operand to `countof`\n",
            //        ulang_type_print(LANG_TYPE_MODE_LOG, lang_type)
            //    );
            //    return false;
            //}
            if (!try_lang_type_get_atom(&atom, LANG_TYPE_MODE_LOG, tast_expr_get_lang_type(new_child))) {
                msg(
                    DIAG_INVALID_COUNTOF, unary_pos,
                    "type `"FMT"` is not a valid operand to `countof`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_child))
                );
                return false;
            }
            Uast_def* def = {0};
            unwrap(usymbol_lookup(&def, atom.str));

            Ustruct_def_base base = {0};
            if (!try_uast_def_get_struct_def_base(&base, def)) {
                msg(
                    DIAG_INVALID_COUNTOF, unary_pos,
                    "type `"FMT"` is not a valid operand to `countof`\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_child))
                );
                return false;
            }
            *new_tast = tast_literal_wrap(tast_int_wrap(tast_int_new(
                unary_pos,
                base.members.info.count,
                lang_type_new_usize()
            )));
            return true;
        }
        case UNARY_UNSAFE_CAST:
            new_lang_type = cast_to;
            assert(lang_type_get_str(LANG_TYPE_MODE_LOG, cast_to).base.count > 0);
            if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number_like(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_is_number(tast_expr_get_lang_type(new_child)) && lang_type_is_number(tast_expr_get_lang_type(new_child))) {
            } else if (lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0 && lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)) > 0) {
            } else {
                log(LOG_NOTE, FMT, lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_child)));
                log(LOG_NOTE, "%d\n", lang_type_get_pointer_depth(tast_expr_get_lang_type(new_child)));
                log(LOG_NOTE, FMT, tast_expr_print(new_child));
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
                    DIAG_UNARY_MISMATCHED_TYPES, unary_pos,
                    "`"FMT"` is not valid operand to logical not operation\n",
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
            DIAG_MISMATCHED_TUPLE_COUNT, uast_tuple_get_pos(tuple),
            "tuple `"FMT"` cannot be assigned to `"FMT"`; "
            "tuple `"FMT"` has %zu elements, but type `"FMT"` has %zu elements\n",
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
                    DIAG_ASSIGNMENT_MISMATCHED_TYPES, 
                    tast_expr_get_pos(new_memb),
                    "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
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

static bool try_set_struct_literal_member_types(Tast_expr_vec* new_membs, Uast_expr_vec membs, Uast_variable_def_vec memb_defs, Pos pos) {
    if (membs.info.count != memb_defs.info.count) {
        msg_invalid_count_struct_literal_args(membs, memb_defs.info.count, memb_defs.info.count, pos);
        return false;
    }

    for (size_t idx = 0; idx < membs.info.count; idx++) {
        Uast_variable_def* memb_def = vec_at(&memb_defs, idx);
        Uast_expr* memb = vec_at(&membs, idx);
        Uast_expr* rhs = NULL;
        if (uast_expr_is_designator(memb)) {
            // TODO: expected failure case for invalid thing (not identifier) on lhs of designated initializer
            Uast_member_access* lhs = uast_member_access_unwrap(
                uast_binary_unwrap(uast_operator_unwrap(memb))->lhs // parser should catch invalid assignment
            );
            rhs = uast_binary_unwrap(uast_operator_unwrap(memb))->rhs;
            if (!name_is_equal(memb_def->name, lhs->member_name->name)) {
                msg(
                    DIAG_INVALID_MEMBER_IN_LITERAL, lhs->pos,
                    "expected `."FMT" =`, got `."FMT" =`\n", 
                    strv_print(memb_def->name.base), name_print(NAME_MSG, lhs->member_name->name)
                );
                return false;
            }
        } else {
            rhs = memb;
        }

        Tast_expr* new_rhs = NULL;
        switch (check_generic_assignment(
             &new_rhs, lang_type_from_ulang_type(memb_def->lang_type), rhs, uast_expr_get_pos(memb)
        )) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg(
                    DIAG_ASSIGNMENT_MISMATCHED_TYPES,
                    uast_expr_get_pos(memb),
                    "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
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
                DIAG_STRUCT_INIT_ON_RAW_UNION,
                assign_pos, "struct literal cannot be assigned to raw_union\n"
            );
            return false;
        case LANG_TYPE_ENUM:
            msg(
                DIAG_STRUCT_INIT_ON_ENUM,
                assign_pos, "struct literal cannot be assigned to enum\n"
            );
            return false;
        case LANG_TYPE_PRIMITIVE:
            msg(
                DIAG_STRUCT_INIT_ON_PRIMITIVE,
                assign_pos, "struct literal cannot be assigned to primitive type `"FMT"`\n",
                lang_type_print(LANG_TYPE_MODE_MSG, dest_lang_type)
            );
            return false;
        default:
            unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, dest_lang_type));
    }
    Uast_def* struct_def_ = NULL;
    unwrap(usymbol_lookup(&struct_def_, lang_type_struct_const_unwrap(dest_lang_type).atom.str));
    Uast_struct_def* struct_def = uast_struct_def_unwrap(struct_def_);
    
    Tast_expr_vec new_membs = {0};
    if (!try_set_struct_literal_member_types(&new_membs, lit->members, struct_def->base.members, lit->pos)) {
        return false;
    }

    Tast_struct_literal* new_lit = tast_struct_literal_new(
        lit->pos,
        new_membs,
        util_literal_name_new(),
        dest_lang_type
    );
    *new_tast = tast_expr_wrap(tast_struct_literal_wrap(new_lit));

    return true;
}

bool try_set_array_literal_types(
    Tast_stmt** new_tast,
    Lang_type dest_lang_type,
    Uast_array_literal* lit,
    Pos assign_pos
) {
    (void) assign_pos;
    Ulang_type gen_arg_ = {0};
    Lang_type gen_arg = {0};
    if (lang_type_is_slice(&gen_arg_, dest_lang_type)) {
        if (!try_lang_type_from_ulang_type(&gen_arg, gen_arg_, lit->pos)) {
            return false;
        }
    } else {
        msg_todo("array literal assigned to non-slice type", assign_pos);
        return false;
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
                    DIAG_ASSIGNMENT_MISMATCHED_TYPES,
                    uast_expr_get_pos(rhs),
                    "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
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
            util_literal_name_new()
        ));
    }
    Tast_struct_def* inner_def = tast_struct_def_new(
        lit->pos,
        (Struct_def_base) {.members = inner_def_membs, .name = util_literal_name_new()}
    );
    sym_tbl_add(tast_struct_def_wrap(inner_def));

    Tast_struct_literal* new_inner_lit = tast_struct_literal_new(
        lit->pos,
        new_membs,
        util_literal_name_new(),
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

    Tast_expr_vec new_lit_membs = {0};
    vec_append(&a_main, &new_lit_membs, ptr);
    vec_append(&a_main, &new_lit_membs, tast_literal_wrap(tast_int_wrap(tast_int_new(
        new_inner_lit->pos,
        new_membs.info.count,
        lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(new_inner_lit->pos, 8/*TODO*/, 0)))
    ))));
    Tast_struct_literal* new_lit = tast_struct_literal_new(
        new_inner_lit->pos,
        new_lit_membs,
        util_literal_name_new(),
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
            }
            assert(*new_tast);
            return true;
        case UAST_UNKNOWN:
            if (lhs_lang_type.type != LANG_TYPE_ENUM) {
                msg(
                    DIAG_UNKNOWN_ON_NON_ENUM_TYPE,
                    uast_expr_get_pos(uast),
                    "infered callee is non-enum type `"FMT"`; only enum types can be infered here\n",
                    lang_type_print(LANG_TYPE_MODE_MSG, lhs_lang_type)
                );
                return false;
            }

            return try_set_symbol_types(new_tast, uast_symbol_new(
                uast_expr_get_pos(uast),
                lang_type_get_str(LANG_TYPE_MODE_LOG, lhs_lang_type)
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
        case UAST_MACRO:
            return try_set_macro_types(new_tast, uast_macro_unwrap(uast));
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
        case UAST_ENUM_ACCESS: {
            Tast_enum_access* new_access = NULL;
            if (!try_set_enum_access_types(&new_access, uast_enum_access_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_enum_access_wrap(new_access);
            return true;
        }
        case UAST_ENUM_GET_TAG: {
            Tast_enum_get_tag* new_access = NULL;
            if (!try_set_enum_get_tag_types(&new_access, uast_enum_get_tag_unwrap(uast))) {
                return false;
            }
            *new_tast = tast_enum_get_tag_wrap(new_access);
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
        case UAST_PRIMITIVE_DEF: {
            if (!try_set_primitive_def_types(uast_primitive_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        }
        case UAST_VOID_DEF:
            if (!try_set_void_def_types(uast_void_def_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        case UAST_ENUM_DEF: {
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
        case UAST_LABEL:
            if (!try_set_label_def_types(uast_label_unwrap(uast))) {
                return STMT_ERROR;
            }
            return STMT_NO_STMT;
        case UAST_LANG_DEF:
            return STMT_NO_STMT;
    }
    unreachable("");
}

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
                DIAG_ASSIGNMENT_MISMATCHED_TYPES,
                assignment->pos,
                "type `"FMT"` cannot be implicitly converted to `"FMT"`\n",
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

bool try_set_function_call_types_enum_case(Tast_enum_case** new_case, Uast_expr_vec args, Tast_enum_case* enum_case) {
    switch (enum_case->tag->lang_type.type) {
        case LANG_TYPE_VOID:
            unreachable("this error should have already been caught");
        default: {
            // tast_enum_case->tag->lang_type is of selected varient of enum (maybe)
            Uast_variable_def* new_def = uast_variable_def_new(
                enum_case->pos,
                lang_type_to_ulang_type(enum_case->tag->lang_type),
                name_new((Strv) {0}, uast_symbol_unwrap(vec_at(&args, 0))->name.base, (Ulang_type_vec) {0}, uast_symbol_unwrap(vec_at(&args, 0))->name.scope_id)
            );
            if (!usymbol_add(uast_variable_def_wrap(new_def))) {
                // TODO: in error message, specify that the new variable definition is in the enum case () (and print accurate position)
                try_set_msg_redefinition_of_symbol(uast_variable_def_wrap(new_def));
                return false;
            }

            Uast_assignment* new_assign = uast_assignment_new(
                new_def->pos,
                uast_symbol_wrap(uast_symbol_new(new_def->pos, new_def->name)),
                uast_enum_access_wrap(uast_enum_access_new(
                    new_def->pos,
                    enum_case->tag,
                    lang_type_from_ulang_type(new_def->lang_type),
                    uast_expr_clone(parent_of_operand, uast_symbol_unwrap(vec_at(&args, 0))->name.scope_id, enum_case->pos)
                ))
            );

            vec_append(&a_main, &switch_case_defer_add_enum_case_part, uast_assignment_wrap(new_assign));

            *new_case = enum_case;
            return true;
        }
    }
}

static Uast_function_decl* uast_function_decl_from_ulang_type_fn(Name sym_name, Ulang_type_fn lang_type, Pos pos) {
    Name name = sym_name;

    Uast_param_vec params = {0};
    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        vec_append(&a_main, &params, uast_param_new(
            lang_type.pos,
            uast_variable_def_new(lang_type.pos, vec_at(&lang_type.params.ulang_types, idx), util_literal_name_new()),
            false, // TODO: test case for optional in function callback
            false, // TODO: test case for variadic in function callback
            NULL
        ));
    }

    Uast_function_decl* fun_decl = uast_function_decl_new(
        lang_type.pos,
        (Uast_generic_param_vec) {0},
        uast_function_params_new(pos, params),
        *lang_type.return_type,
        name
    );
    return fun_decl;
}


bool try_set_function_call_types(Tast_expr** new_call, Uast_function_call* fun_call) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, fun_call->callee)) {
        return false;
    }

    bool status = true;

    Name fun_name = {0};
    Uast_function_decl* fun_decl = NULL;
    bool is_fun_callback = false;
    switch (new_callee->type) {
        case TAST_ENUM_CALLEE: {
            // TAST_ENUM_CALLEE is for right hand side of assignments that have non-void inner type
            if (fun_call->args.info.count < 1) {
                msg(
                    DIAG_MISSING_ENUM_ARG, tast_enum_callee_unwrap(new_callee)->pos,
                    "() in enum case has no argument; add argument in () or remove ()\n"
                );
                return false;
            }
            if (fun_call->args.info.count > 1) {
                msg(
                    DIAG_ENUM_CASE_TOO_MOPAQUE_ARGS, tast_enum_callee_unwrap(new_callee)->pos,
                    "() in enum case must contain exactly one argument, but %zu arguments found\n",
                    fun_call->args.info.count
                );
                return false;
            }
            if (tast_enum_callee_unwrap(new_callee)->tag->lang_type.type == LANG_TYPE_VOID) {
                unreachable("enum symbol with void callee should have been converted to TAST_ENUM_LIT instead of TAST_ENUM_CALLEE in try_set_symbol_types");
            }

            Tast_enum_callee* enum_callee = tast_enum_callee_unwrap(new_callee);

            Uast_def* enum_def_ = NULL;
            unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, enum_callee->enum_lang_type)));
            Uast_enum_def* enum_def = uast_enum_def_unwrap(enum_def_);

            Tast_expr* new_item = NULL;
            switch (check_generic_assignment(
                &new_item,
                lang_type_from_ulang_type(vec_at(&enum_def->base.members, (size_t)enum_callee->tag->data)->lang_type),
                vec_at(&fun_call->args, 0),
                uast_expr_get_pos(vec_at(&fun_call->args, 0))
            )) {
                case CHECK_ASSIGN_OK:
                    break;
                case CHECK_ASSIGN_INVALID:
                    msg(
                        DIAG_ENUM_LIT_INVALID_ARG, tast_expr_get_pos(new_item),
                        "cannot assign "FMT" of type `"FMT"` to '"FMT"`\n", 
                        tast_expr_print(new_item),
                        lang_type_print(LANG_TYPE_MODE_MSG, tast_expr_get_lang_type(new_item)), 
                        lang_type_print(LANG_TYPE_MODE_MSG, lang_type_from_ulang_type(
                             vec_at(&enum_def->base.members, (size_t)enum_callee->tag->data)->lang_type
                        ))
                   );
                   status = false;
                   break;
                case CHECK_ASSIGN_ERROR:
                    todo();
                default:
                    unreachable("");
            }

            enum_callee->tag->lang_type = lang_type_new_usize();

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                enum_callee->pos,
                enum_callee->tag,
                new_item,
                enum_callee->enum_lang_type
            );
            *new_call = tast_literal_wrap(tast_enum_lit_wrap(new_lit));
            return status;
        }
        case TAST_ENUM_CASE: {
            // TAST_ENUM_CASE is for switch cases
            // TODO: can these checks be shared with TAST_ENUM_CALLEE?
            if (tast_enum_case_unwrap(new_callee)->tag->lang_type.type == LANG_TYPE_VOID) {
                msg(DIAG_INVALID_COUNT_FUN_ARGS, fun_call->pos, "inner type is void; remove ()\n");
                return false;
            }
            if (fun_call->args.info.count < 1) {
                msg(
                    DIAG_MISSING_ENUM_ARG, tast_enum_case_unwrap(new_callee)->pos,
                    "() in enum case has no argument; add argument in () or remove ()\n"
                );
                return false;
            }
            if (fun_call->args.info.count > 1) {
                msg(
                    DIAG_ENUM_CASE_TOO_MOPAQUE_ARGS, tast_enum_case_unwrap(new_callee)->pos,
                    "() in enum case must contain exactly one argument, but %zu arguments found\n",
                    fun_call->args.info.count
                );
                return false;
            }
            Tast_enum_case* new_case = NULL;
            if (!try_set_function_call_types_enum_case(&new_case, fun_call->args, tast_enum_case_unwrap(new_callee))) {
                return false;
            }
            *new_call = tast_enum_case_wrap(new_case);
            return status;
        }
        case TAST_LITERAL: {
            if (tast_literal_unwrap(new_callee)->type == TAST_ENUM_LIT) {
                // TAST_ENUM_LIT in function callee means that the user used () with void enum varient in right hand side of assignment
                msg(DIAG_INVALID_COUNT_FUN_ARGS, fun_call->pos, "inner type is void; remove ()\n");
                return false;
            } else if (tast_literal_unwrap(new_callee)->type == TAST_FUNCTION_LIT) {
                fun_name = tast_function_lit_unwrap(tast_literal_unwrap(new_callee))->name;
                unwrap(function_decl_tbl_lookup(&fun_decl, fun_name));
                break;
            } else {
                msg(
                    DIAG_INVALID_FUNCTION_CALLEE, tast_expr_get_pos(new_callee),
                    "callee is not callable\n"
                );
                return false;
            }
        }
        case TAST_SYMBOL: {
            fun_name = tast_symbol_unwrap(new_callee)->base.name;
            fun_decl = uast_function_decl_from_ulang_type_fn(
                fun_name,
                ulang_type_fn_const_unwrap(lang_type_to_ulang_type(tast_symbol_unwrap(new_callee)->base.lang_type)),
                tast_symbol_unwrap(new_callee)->pos
            );
            is_fun_callback = true;
            break;
        }
        case TAST_MEMBER_ACCESS:
            if (tast_expr_get_lang_type(new_callee).type != LANG_TYPE_FN) {
                todo();
            }
            fun_decl = uast_function_decl_from_ulang_type_fn(
                fun_name,
                ulang_type_fn_const_unwrap(lang_type_to_ulang_type(tast_expr_get_lang_type(new_callee))),
                tast_expr_get_pos(new_callee)
            );
            is_fun_callback = true;
            break;
        default:
            unreachable(FMT, tast_expr_print(new_callee));
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
            unwrap(!param->is_optional && "this should have been caught in the parser");
        } else if (param->is_optional) {
            unwrap(!is_variadic);
            // TODO: expected failure case for invalid optional_default
            corres_arg = uast_expr_clone(param->optional_default, fun_name.scope_id/* TODO */, fun_call->pos);
        } else {
            // TODO: print max count correctly for variadic functions
            msg_invalid_count_function_args(fun_call, fun_decl, param_idx + 1, param_idx + 1);
            status = false;
            goto error;
        }

        Tast_expr* new_arg = NULL;

        if (lang_type_is_equal(lang_type_from_ulang_type(param->base->lang_type), lang_type_primitive_const_wrap(lang_type_opaque_const_wrap(lang_type_opaque_new(POS_BUILTIN, lang_type_atom_new_from_cstr("opaque", 0, 0)))))) {
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
                    msg_invalid_function_arg(new_arg, param->base, is_fun_callback);
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

bool try_set_macro_types(Tast_expr** new_call, Uast_macro* macro) {
    if (strv_is_equal(macro->name, sv("file"))) {
        *new_call = tast_literal_wrap(tast_string_wrap(tast_string_new(macro->pos, macro->value.file_path)));
        return true;
    } else if (strv_is_equal(macro->name, sv("line"))) {
        *new_call = tast_literal_wrap(util_tast_literal_new_from_int64_t(macro->value.line, TOKEN_INT_LITERAL, macro->pos));
        return true;
    } else if (strv_is_equal(macro->name, sv("column"))) {
        *new_call = tast_literal_wrap(util_tast_literal_new_from_int64_t(macro->value.column, TOKEN_INT_LITERAL, macro->pos));
        return true;
    } else {
        msg_todo("language feature macro (other than `#file`, `#line`, `#column`)", macro->pos);
        return false;
    }
    unreachable("");
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

bool try_set_enum_access_types(Tast_enum_access** new_access, Uast_enum_access* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
        return false;
    }

    *new_access = tast_enum_access_new(access->pos, access->tag, access->lang_type, new_callee);
    return true;
}

bool try_set_enum_get_tag_types(Tast_enum_get_tag** new_access, Uast_enum_get_tag* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
        return false;
    }

    *new_access = tast_enum_get_tag_new(access->pos, new_callee);
    return true;
}

static void msg_invalid_member_internal(
    const char* file,
    int line,
    Name base_name,
    const Uast_member_access* access
) {
    msg_internal(
        file, line, DIAG_INVALID_MEMBER_ACCESS,
        access->pos,
        "`"FMT"` is not a member of `"FMT"`\n", 
        strv_print(access->member_name->name.base), name_print(NAME_MSG, base_name)
    );
}

#define msg_invalid_member(base_name, access) \
    msg_invalid_member_internal(__FILE__, __LINE__, base_name, access)

bool try_set_member_access_types_finish_generic_struct(
    Tast_stmt** new_tast, // TODO: change to tast_expr
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
        // TODO
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

bool try_set_member_access_types_finish_enum_def(
    Tast_stmt** new_tast,
    Uast_enum_def* enum_def,
    Uast_member_access* access,
    Tast_expr* new_callee
) {
    (void) new_callee;

    switch (parent_of) {
        case PARENT_OF_CASE: {
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name->name.base)) {
                msg_invalid_member(enum_def->base.name, access);
                return false;
            }

            Tast_enum_tag_lit* new_tag = tast_enum_tag_lit_new(
                access->pos,
                uast_get_member_index(&enum_def->base, access->member_name->name.base),
                lang_type_from_ulang_type(member_def->lang_type)
            );

            *new_tast = tast_expr_wrap(tast_enum_case_wrap(tast_enum_case_new(
                access->pos,
                new_tag,
                lang_type_enum_const_wrap(lang_type_enum_new(enum_def->pos, lang_type_atom_new(enum_def->base.name, 0)))
            )));

            return true;
        }
        case PARENT_OF_RETURN:
            todo();
        case PARENT_OF_BREAK:
            todo();
            // fallthrough
        case PARENT_OF_NONE:
            // fallthrough
        case PARENT_OF_ASSIGN_RHS: {
            Uast_variable_def* member_def = NULL;
            if (!uast_try_get_member_def(&member_def, &enum_def->base, access->member_name->name.base)) {
                msg_invalid_member(enum_def->base.name, access);
                return false;
            }
            
            Tast_enum_tag_lit* new_tag = tast_enum_tag_lit_new(
                access->pos,
                uast_get_member_index(&enum_def->base, access->member_name->name.base),
                lang_type_from_ulang_type(member_def->lang_type)
            );

            Tast_enum_callee* new_callee = tast_enum_callee_new(
                access->pos,
                new_tag,
                lang_type_enum_const_wrap(lang_type_enum_new(enum_def->pos, lang_type_atom_new(enum_def->base.name, 0)))
            );

            if (new_tag->lang_type.type != LANG_TYPE_VOID) {
                *new_tast = tast_expr_wrap(tast_enum_callee_wrap(new_callee));
                return true;
            }

            new_callee->tag->lang_type = lang_type_new_usize();

            Tast_enum_lit* new_lit = tast_enum_lit_new(
                new_callee->pos,
                new_callee->tag,
                tast_literal_wrap(tast_void_wrap(tast_void_new(new_callee->pos))),
                new_callee->enum_lang_type
            );
            *new_tast = tast_expr_wrap(tast_literal_wrap(tast_enum_lit_wrap(new_lit)));
            return true;
        }
        case PARENT_OF_IF:
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
        case UAST_ENUM_DEF:
            return try_set_member_access_types_finish_enum_def(new_tast, uast_enum_def_unwrap(lang_type_def), access, new_callee);
        case UAST_PRIMITIVE_DEF:
            msg_invalid_member(lang_type_get_str(LANG_TYPE_MODE_LOG, uast_primitive_def_unwrap(lang_type_def)->lang_type), access);
            return false;
        case UAST_LABEL: {
            todo();
        }
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
        case UAST_VOID_DEF:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("lang def should have been eliminated by now");
    }
    unreachable("");
}

bool try_set_member_access_types(Tast_stmt** new_tast, Uast_member_access* access) {
    Tast_expr* new_callee = NULL;
    if (!try_set_expr_types(&new_callee, access->callee)) {
        return false;
    }
    new_callee = auto_deref_to_0(new_callee);

    switch (new_callee->type) {
        case TAST_SYMBOL: {
            Tast_symbol* sym = tast_symbol_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(LANG_TYPE_MODE_LOG, sym->base.lang_type))) {
                log(LOG_DEBUG, FMT, lang_type_print(LANG_TYPE_MODE_LOG, sym->base.lang_type));
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);

        }
        case TAST_MEMBER_ACCESS: {
            Tast_member_access* sym = tast_member_access_unwrap(new_callee);

            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(LANG_TYPE_MODE_LOG, sym->lang_type))) {
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);
        }
        case TAST_OPERATOR: {
            Tast_operator* sym = tast_operator_unwrap(new_callee);
            Uast_def* lang_type_def = NULL;
            if (!usymbol_lookup(&lang_type_def, lang_type_get_str(LANG_TYPE_MODE_LOG, tast_operator_get_lang_type(sym)))) {
                todo();
            }

            return try_set_member_access_types_finish(new_tast, lang_type_def, access, new_callee);
        }
        case TAST_FUNCTION_CALL: {
            Tast_function_call* call = tast_function_call_unwrap(new_callee);
            Lang_type lang_type = *lang_type_fn_const_unwrap(tast_expr_get_lang_type(call->callee)).return_type;
            Uast_def* lang_type_def = NULL;
            unwrap(usymbol_lookup(&lang_type_def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));

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
            unreachable(FMT, tast_expr_print(new_callee));
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
            lang_type_new_usize()
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
    unwrap(symbol_add(tast_primitive_def_wrap(tast_primitive_def_new(tast->pos, tast->lang_type))));
    return true;
}

bool try_set_void_def_types(Uast_void_def* tast) {
    (void) tast;
    return true;
}

bool try_set_label_def_types(Uast_label* tast) {
    symbol_add(tast_label_wrap(tast_label_new(tast->pos, tast->name, tast->block_scope)));
    return true;
}

bool try_set_import_path_types(Tast_block** new_tast, Uast_import_path* tast) {
    return try_set_block_types(new_tast, tast->block, false);
}

bool try_set_variable_def_types(
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl,
    bool is_variadic
) {
    Uast_def* result = NULL;
    if (usymbol_lookup(&result, uast->name) && result->type == UAST_POISON_DEF) {
        unwrap(error_count > 0);
        return false;
    }

    Lang_type new_lang_type = {0};
    if (!try_lang_type_from_ulang_type(&new_lang_type, uast->lang_type, uast->pos)) {
        Uast_poison_def* new_poison = uast_poison_def_new(uast->pos, uast->name);
        usymbol_update(uast_poison_def_wrap(new_poison));
        return false;
    }

    *new_tast = tast_variable_def_new(uast->pos, new_lang_type, is_variadic, uast->name);
    if (add_to_sym_tbl && !is_in_struct_base_def) {
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
    PARENT_OF old_parent_of = parent_of;
    parent_of = PARENT_OF_RETURN;

    if (is_in_defer) {
        msg(DIAG_RETURN_IN_DEFER, rtn->pos, "return statement cannot be located in defer\n");
        msg(DIAG_NOTE, parent_defer_pos, "defer statement defined here\n");
        status = false;
        goto error;
    }

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
    parent_of = old_parent_of;
    return status;
}

bool try_set_continue_types(Tast_continue** new_tast, Uast_continue* cont) {
    switch (parent_of_defer) {
        case PARENT_OF_DEFER_FOR:
            *new_tast = tast_continue_new(cont->pos);
            return true;
        case PARENT_OF_DEFER_NONE:
            *new_tast = tast_continue_new(cont->pos);
            return true;
        case PARENT_OF_DEFER_DEFER:
            msg(DIAG_CONTINUE_OUT_OF_DEFER, cont->pos, "cannot continue out of defer\n");
            return false;
    }
    unreachable("");
}

bool try_set_break_types(Tast_break** new_tast, Uast_break* lang_break) {
    bool status = true;
    PARENT_OF old_parent_of = parent_of;
    parent_of = PARENT_OF_BREAK;

    switch (parent_of_defer) {
        case PARENT_OF_DEFER_FOR:
            break;
        case PARENT_OF_DEFER_NONE:
            break;
        case PARENT_OF_DEFER_DEFER:
            msg(DIAG_BREAK_OUT_OF_DEFER, lang_break->pos, "cannot break out of defer\n");
            status = false;
            goto error;
        default:
            unreachable("");
    }

    Tast_expr* new_child = NULL;
    if (lang_break->do_break_expr) {
        switch (check_generic_assignment(&new_child, break_type, lang_break->break_expr, lang_break->pos)) {
            case CHECK_ASSIGN_OK:
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

    break_in_case = true;
error:
    parent_of = old_parent_of;
    return status;
}

bool try_set_yield_types(Tast_yield** new_tast, Uast_yield* yield) {
    bool status = true;
    PARENT_OF old_parent_of = parent_of;
    parent_of = PARENT_OF_BREAK; // TODO

    Uast_def* dummy = NULL;
    if (!usymbol_lookup(&dummy, yield->break_out_of)) {
        msg_undefined_symbol(uast_symbol_new(yield->pos, yield->break_out_of));
        status = false;
        goto error;
    }

    switch (parent_of_defer) {
        case PARENT_OF_DEFER_FOR:
            break;
        case PARENT_OF_DEFER_NONE:
            break;
        case PARENT_OF_DEFER_DEFER:
            msg(DIAG_BREAK_OUT_OF_DEFER/*TODO*/, yield->pos, "cannot yield out of defer\n");
            status = false;
            goto error;
        default:
            unreachable("");
    }

    Tast_expr* new_child = NULL;
    if (yield->do_yield_expr) {
        switch (check_generic_assignment(&new_child, break_type/* TODO: this will not work in all situations*/, yield->yield_expr, yield->pos)) {
            case CHECK_ASSIGN_OK:
                break;
            case CHECK_ASSIGN_INVALID:
                msg_invalid_yield_type(yield->pos, new_child, false);
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

    *new_tast = tast_yield_new(yield->pos, yield->do_yield_expr, new_child, yield->break_out_of);

    break_in_case = true;
error:
    parent_of = old_parent_of;
    return status;
}

bool try_set_continue2_types(Tast_continue2** new_tast, Uast_continue2* cont) {
    bool status = true;
    PARENT_OF old_parent_of = parent_of;
    parent_of = PARENT_OF_BREAK; // TODO

    Uast_def* dummy = NULL;
    if (!usymbol_lookup(&dummy, cont->break_out_of)) {
        msg_undefined_symbol(uast_symbol_new(cont->pos, cont->break_out_of));
        status = false;
        goto error;
    }

    switch (parent_of_defer) {
        case PARENT_OF_DEFER_FOR:
            break;
        case PARENT_OF_DEFER_NONE:
            break;
        case PARENT_OF_DEFER_DEFER:
            msg(DIAG_BREAK_OUT_OF_DEFER/*TODO*/, cont->pos, "cannot continue out of defer\n");
            status = false;
            goto error;
        default:
            unreachable("");
    }

    *new_tast = tast_continue2_new(cont->pos, cont->break_out_of);

    break_in_case = true;
error:
    parent_of = old_parent_of;
    return status;
}

bool try_set_for_with_cond_types(Tast_for_with_cond** new_tast, Uast_for_with_cond* uast) {
    PARENT_OF_DEFER old_parent_of_defer = parent_of_defer;
    parent_of_defer = PARENT_OF_DEFER_FOR;
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(&new_cond, uast->condition)) {
        status = false;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(&new_body, uast->body, false)) {
        status = false;
    }

    *new_tast = tast_for_with_cond_new(uast->pos, new_cond, new_body, uast->continue_label, uast->do_cont_label);
    parent_of_defer = old_parent_of_defer;
    return status;
}

bool try_set_if_types(Tast_if** new_tast, Uast_if* uast) {
    bool status = true;

    Tast_condition* new_cond = NULL;
    if (!try_set_condition_types(&new_cond, uast->condition)) {
        status = false;
    }

    Uast_stmt_vec new_if_children = {0};
    vec_extend(&a_main, &new_if_children, &switch_case_defer_add_enum_case_part);
    vec_extend(&a_main, &new_if_children, &switch_case_defer_add_if_true);
    vec_extend(&a_main, &new_if_children, &uast->body->children);
    uast->body->children = new_if_children;
    vec_reset(&switch_case_defer_add_enum_case_part);
    vec_reset(&switch_case_defer_add_if_true);

    Tast_block* new_body = NULL;
    if (!(status && try_set_block_types(&new_body, uast->body, false))) {
        status = false;
    }

    if (status) {
        *new_tast = tast_if_new(uast->pos, new_cond, new_body, new_body->lang_type);
        if (parent_of == PARENT_OF_CASE) {
            if (new_body->lang_type.type != LANG_TYPE_VOID && !break_in_case) {
                // TODO: this will not work if there is nested switch or if-else
                msg_invalid_yield_type(new_body->pos_end, NULL, true);
                status = false;
            }
        } else if (parent_of == PARENT_OF_IF) {
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
    Pos_vec covered_pos;
    size_t max_data;
} Exhaustive_data;

static Exhaustive_data check_for_exhaustiveness_start(Lang_type oper_lang_type) {
    Exhaustive_data exhaustive_data = {0};

    exhaustive_data.oper_lang_type = oper_lang_type;

    Uast_def* enum_def_ = NULL;
    if (!usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, exhaustive_data.oper_lang_type))) {
        todo();
    }
    Ustruct_def_base enum_def = {0};
    switch (enum_def_->type) {
        case UAST_ENUM_DEF:
            enum_def = uast_enum_def_unwrap(enum_def_)->base;
            break;
        default:
            todo();
    }
    unwrap(enum_def.members.info.count > 0);
    exhaustive_data.max_data = enum_def.members.info.count - 1;

    vec_reserve(&a_print, &exhaustive_data.covered, exhaustive_data.max_data + 1);
    vec_reserve(&a_print, &exhaustive_data.covered_pos, exhaustive_data.max_data + 1);
    for (size_t idx = 0; idx < exhaustive_data.max_data + 1; idx++) {
        vec_append(&a_print, &exhaustive_data.covered, false);
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
                DIAG_DUPLICATE_DEFAULT, curr_if->pos,
                "duplicate default in switch statement\n"
            );
            return false;
        }
        exhaustive_data->default_is_pre = true;
        return true;
    }

    switch (exhaustive_data->oper_lang_type.type) {
        case LANG_TYPE_ENUM: {
            const Tast_enum_tag_lit* curr_lit = tast_enum_case_unwrap(
                tast_binary_unwrap(curr_if->condition->child)->rhs
            )->tag;

            if (curr_lit->data > (int64_t)exhaustive_data->max_data) {
                unreachable("invalid enum value\n");
            }
            if (vec_at(&exhaustive_data->covered, (size_t)curr_lit->data)) {
                Uast_def* enum_def_ = NULL;
                unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, exhaustive_data->oper_lang_type)));
                Uast_enum_def* enum_def = uast_enum_def_unwrap(enum_def_);
                msg(
                    DIAG_DUPLICATE_CASE, curr_if->pos,
                    "duplicate case `"FMT"."FMT"` in switch statement\n",
                    name_print(NAME_MSG, enum_def->base.name), name_print(NAME_MSG, vec_at(&enum_def->base.members, (size_t)curr_lit->data)->name)
                );
                msg(
                    DIAG_NOTE, vec_at(&exhaustive_data->covered_pos,
                    (size_t)curr_lit->data), "case originally covered here\n"
                );
                return false;
            }
            *vec_at_ref(&exhaustive_data->covered, (size_t)curr_lit->data) = true;
            vec_append(&a_print, &exhaustive_data->covered_pos, curr_lit->pos);
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
                unwrap(usymbol_lookup(&enum_def_, lang_type_get_str(LANG_TYPE_MODE_LOG, exhaustive_data.oper_lang_type)));
                Ustruct_def_base enum_def = {0};
                switch (enum_def_->type) {
                    case UAST_ENUM_DEF:
                        enum_def = uast_enum_def_unwrap(enum_def_)->base;
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

                extend_name(NAME_MSG, &string, enum_def.name);
                string_extend_cstr(&a_main, &string, ".");
                extend_name(NAME_MSG, &string, vec_at(&enum_def.members, idx)->name);
            }
        }

        if (!status) {
            msg(
                DIAG_NON_EXHAUSTIVE_SWITCH, pos_switch,
                FMT"\n", string_print(string)
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
    PARENT_OF old_parent_of = parent_of;
    break_in_case = false;
    if (parent_of == PARENT_OF_ASSIGN_RHS) {
        break_type = lhs_lang_type;
    } else {
        break_type = lang_type_void_const_wrap(lang_type_void_new(lang_switch->pos));
    }

    switch (tast_expr_get_lang_type(new_operand).type) {
        case LANG_TYPE_ENUM:
            break;
        default:
            msg_todo("switch on type that is not enum", tast_expr_get_pos(new_operand));
            status = false;
            goto error;
    }

    Exhaustive_data exhaustive_data = check_for_exhaustiveness_start(
         tast_expr_get_lang_type(new_operand)
    );

    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        Uast_case* old_case = vec_at(&lang_switch->cases, idx);
        Uast_condition* cond = NULL;

        Uast_expr* operand = NULL;

        switch (tast_expr_get_lang_type(new_operand).type) {
            case LANG_TYPE_ENUM:
                operand = uast_enum_get_tag_wrap(uast_enum_get_tag_new(
                    uast_expr_get_pos(lang_switch->operand), lang_switch->operand
                ));
                break;
            default:
                unreachable("this should have been caught earlier");
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

        vec_append(&a_main, &switch_case_defer_add_if_true, old_case->if_true);
        Uast_block* if_true = uast_block_new(
            old_case->pos,
            (Uast_stmt_vec) {0},
            old_case->pos,
            old_case->scope_id
        );
                
        parent_of = PARENT_OF_CASE;
        parent_of_operand = lang_switch->operand;
        Tast_if* new_if = NULL;
        if (!try_set_if_types(&new_if, uast_if_new(old_case->pos, cond, if_true))) {
            status = false;
            goto error_inner;
        }

error_inner:
        parent_of_operand = NULL;
        parent_of = PARENT_OF_NONE;
        break_in_case = false;
        if (!status) {
            goto error;
        }

        if (!check_for_exhaustiveness_inner(&exhaustive_data, new_if, old_case->is_default)) {
            status = false;
            goto error;
        }

        vec_append(&a_main, &new_ifs, new_if);
    }

    *new_tast = tast_if_else_chain_new(lang_switch->pos, new_ifs, true);
    if (!check_for_exhaustiveness_finish(exhaustive_data, lang_switch->pos)) {
        status = false;
        goto error;
    }

error:
    parent_of = old_parent_of;
    break_in_case = false;
    return status;
}

bool try_set_defer_types(Tast_defer** new_tast, const Uast_defer* defer) {
    bool status = true;
    bool old_is_in_defer = is_in_defer;
    is_in_defer = true;
    Pos old_defer_pos = parent_defer_pos;
    parent_defer_pos = defer->pos;
    PARENT_OF_DEFER old_parent_of_defer = parent_of_defer;
    parent_of_defer = PARENT_OF_DEFER_DEFER;

    Tast_stmt* new_child = NULL;
    switch (try_set_stmt_types(&new_child, defer->child, false)) {
        case STMT_OK:
            break;
        case STMT_ERROR:
            status = false;
            goto error;
        case STMT_NO_STMT:
            todo();
        default:
            unreachable("");
    }
    *new_tast = tast_defer_new(defer->pos, new_child);

error:
    is_in_defer = old_is_in_defer;
    parent_defer_pos = old_defer_pos;
    parent_of_defer = old_parent_of_defer;
    return status;
}

// TODO: merge this with msg_redefinition_of_symbol?
static void try_set_msg_redefinition_of_symbol(const Uast_def* new_sym_def) {
    msg(
        DIAG_REDEFINITION_SYMBOL, uast_def_get_pos(new_sym_def),
        "redefinition of symbol "FMT"\n", name_print(NAME_MSG, uast_def_get_name(new_sym_def))
    );

    Uast_def* original_def;
    unwrap(usymbol_lookup(&original_def, uast_def_get_name(new_sym_def)));
    msg(
        DIAG_NOTE, uast_def_get_pos(original_def),
        FMT " originally defined here\n", name_print(NAME_MSG, uast_def_get_name(original_def))
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

bool try_set_block_types(Tast_block** new_tast, Uast_block* block, bool is_directly_in_fun_def) {
    do_test_bit_width();

    bool status = true;

    Tast_stmt_vec new_tasts = {0};

    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        if (curr->type != UAST_VARIABLE_DEF && curr->type != UAST_IMPORT_PATH && curr->type != UAST_LABEL) {
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
                break;
            default:
                unreachable("");
        }
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Uast_stmt* curr_tast = vec_at(&block->children, idx);
        Tast_stmt* new_tast = NULL;
        switch (try_set_stmt_types(&new_tast, curr_tast, block->scope_id == SCOPE_TOP_LEVEL)) {
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
                 sv(""), TOKEN_VOID, block->pos_end
            )),
            true
        );
        if (rtn_statement->pos.line == 0) {
            unreachable("");
        }
        Tast_stmt* new_rtn_statement = NULL;
        switch (try_set_stmt_types(&new_rtn_statement, uast_return_wrap(rtn_statement), block->scope_id == SCOPE_TOP_LEVEL)) {
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
        if (!usymbol_lookup(&main_fn_, name_new((Strv) {0}, sv("main"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL))) {
            msg(DIAG_NO_MAIN, POS_BUILTIN, "no main function\n");
            goto error;
        }
        if (main_fn_->type != UAST_FUNCTION_DEF) {
            todo();
        }
        Lang_type_fn new_lang_type = {0};
        Name new_name = {0};
        if (!resolve_generics_function_def_call(&new_lang_type, &new_name, uast_function_def_unwrap(main_fn_), (Ulang_type_vec) {0}, (Pos) {0})) {
            status = false;
        }
    }

error:
    dummy_int = 0; // allow pre-c23 compilers
    Lang_type yield_type = lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
    assert(yield_type.type == LANG_TYPE_VOID);
    if (parent_of == PARENT_OF_CASE) {
        yield_type = break_type;
    } else if (parent_of == PARENT_OF_IF) {
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

static bool stmt_type_allowed_in_top_level(UAST_STMT_TYPE type) {
    switch (type) {
        case UAST_BLOCK:
            return false;
        case UAST_EXPR:
            return false;
        case UAST_DEF:
            return true;
        case UAST_FOR_WITH_COND:
            return false;
        case UAST_BREAK:
            return false;
        case UAST_YIELD:
            return false;
        case UAST_CONTINUE:
            return false;
        case UAST_CONTINUE2:
            return false;
        case UAST_ASSIGNMENT:
            return false;
        case UAST_RETURN:
            return false;
        case UAST_DEFER:
            return false;
    }
    unreachable("");
}

STMT_STATUS try_set_stmt_types(Tast_stmt** new_tast, Uast_stmt* stmt, bool is_top_level) {
    if (is_top_level && !stmt_type_allowed_in_top_level(stmt->type)) {
        // TODO: actually print the types of statements that are allowed?
        msg(
            DIAG_INVALID_STMT_TOP_LEVEL, uast_stmt_get_pos(stmt),
            "this statement is not permitted in the top level\n"
        );
        return STMT_ERROR;
    }

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
        case UAST_CONTINUE: {
            Tast_continue* new_cont = NULL;
            if (!try_set_continue_types(&new_cont, uast_continue_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_continue_wrap(new_cont);
            return STMT_OK;
        }
        case UAST_BLOCK: {
            Tast_block* new_for = NULL;
            if (!try_set_block_types(&new_for, uast_block_unwrap(stmt), false)) {
                return STMT_ERROR;
            }
            *new_tast = tast_block_wrap(new_for);
            return STMT_OK;
        }
        case UAST_DEFER: {
            Tast_defer* new_defer = NULL;
            if (!try_set_defer_types(&new_defer, uast_defer_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_defer_wrap(new_defer);
            return STMT_OK;
        }
        case UAST_YIELD: {
            Tast_yield* new_yield = NULL;
            if (!try_set_yield_types(&new_yield, uast_yield_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_yield_wrap(new_yield);
            return STMT_OK;
        }
        case UAST_CONTINUE2: {
            Tast_continue2* new_cont = NULL;
            if (!try_set_continue2_types(&new_cont, uast_continue2_unwrap(stmt))) {
                return STMT_ERROR;
            }
            *new_tast = tast_continue2_wrap(new_cont);
            return STMT_OK;
        }
    }
    unreachable("");
}

bool try_set_types(Tast_block** new_tast, Uast_block* block) {
    bool status = true;

    // TODO: this def iteration should be abstracted to a separate function (try_set_block_types has similar)
    Usymbol_iter iter = usym_tbl_iter_new(0);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        // TODO: make switch for this if for exhausive checking
        if (curr->type != UAST_VARIABLE_DEF && curr->type != UAST_IMPORT_PATH && curr->type != UAST_LABEL) {
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
                break;
            default:
                unreachable("");
        }
    }

    if (!try_set_block_types(new_tast, block, false)) {
        status = false;
    }

    while (env.fun_implementations_waiting_to_resolve.info.count > 0) {
        Name curr_name = {0};
        vec_pop(curr_name, &env.fun_implementations_waiting_to_resolve);
        if (!resolve_generics_function_def_implementation(curr_name)) {
            status = false;
        }
    }

    while (env.struct_like_waiting_to_resolve.info.count > 0) {
        Name curr_name = {0};
        vec_pop(curr_name, &env.struct_like_waiting_to_resolve);
        if (!resolve_generics_struct_like_def_implementation(curr_name)) {
            status = false;
        }
    }

    Usymbol_iter rec_iter = usym_tbl_iter_new_table(env.struct_like_tbl);
    Uast_def* curr_def = NULL;
    while (usym_tbl_iter_next(&curr_def, &rec_iter)) {
        if (!check_struct_for_rec(curr_def)) {
            status = false;
        }
    }

    return status;
}
