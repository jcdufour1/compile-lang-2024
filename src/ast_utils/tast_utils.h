#ifndef TAST_UTIL_H
#define TAST_UTIL_H

#include <tast.h>
#include <lang_type_after.h>
#include <ir_lang_type_after.h>
#include <ulang_type.h>
#include <lang_type_print.h>
#include <ulang_type_new_convenience.h>
#include <ulang_type_is_equal.h>
#include <ast_msg.h>

// TODO: remove this forward declaration
static inline Ulang_type ulang_type_new_int_x(Pos pos, Strv base);

static inline bool lang_type_is_equal(Lang_type a, Lang_type b);

static inline bool ir_lang_type_is_equal(Ir_lang_type a, Ir_lang_type b);

static inline Lang_type tast_expr_get_lang_type(const Tast_expr* expr);

static inline bool ir_lang_type_vec_is_equal(Ir_lang_type_vec a, Ir_lang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!ir_lang_type_is_equal(vec_at(a, idx), vec_at(b, idx))) {
            return false;
        }
    }

    return true;
}

static inline bool ir_lang_type_tuple_is_equal(Ir_lang_type_tuple a, Ir_lang_type_tuple b) {
    return ir_lang_type_vec_is_equal(a.ir_lang_types, b.ir_lang_types);
}

static inline bool ir_lang_type_fn_is_equal(Ir_lang_type_fn a, Ir_lang_type_fn b) {
    return ir_lang_type_tuple_is_equal(a.params, b.params) && ir_lang_type_is_equal(*a.return_type, *b.return_type);
}

static inline bool ir_lang_type_primitive_is_equal(Ir_lang_type_primitive a, Ir_lang_type_primitive b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case IR_LANG_TYPE_SIGNED_INT: {
            Ir_lang_type_signed_int a_int = ir_lang_type_signed_int_const_unwrap(a);
            Ir_lang_type_signed_int b_int = ir_lang_type_signed_int_const_unwrap(b);
            return a_int.bit_width == b_int.bit_width && a_int.pointer_depth == b_int.pointer_depth;
        }
        case IR_LANG_TYPE_UNSIGNED_INT: {
            Ir_lang_type_unsigned_int a_int = ir_lang_type_unsigned_int_const_unwrap(a);
            Ir_lang_type_unsigned_int b_int = ir_lang_type_unsigned_int_const_unwrap(b);
            return a_int.bit_width == b_int.bit_width && a_int.pointer_depth == b_int.pointer_depth;
        }
        case IR_LANG_TYPE_FLOAT: {
            Ir_lang_type_float a_int = ir_lang_type_float_const_unwrap(a);
            Ir_lang_type_float b_int = ir_lang_type_float_const_unwrap(b);
            return a_int.bit_width == b_int.bit_width && a_int.pointer_depth == b_int.pointer_depth;
        }
        case IR_LANG_TYPE_OPAQUE:
            return ir_lang_type_opaque_const_unwrap(a).pointer_depth == ir_lang_type_opaque_const_unwrap(b).pointer_depth;
        default:
            unreachable("");
    }
    unreachable("");
}

static inline bool ir_lang_type_is_equal(Ir_lang_type a, Ir_lang_type b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            return ir_lang_type_primitive_is_equal(
                ir_lang_type_primitive_const_unwrap(a),
                ir_lang_type_primitive_const_unwrap(b)
            );
        case IR_LANG_TYPE_STRUCT: {
            Ir_lang_type_struct a_struct = ir_lang_type_struct_const_unwrap(a);
            Ir_lang_type_struct b_struct = ir_lang_type_struct_const_unwrap(b);
            return a_struct.pointer_depth == b_struct.pointer_depth && ir_name_is_equal(a_struct.name, b_struct.name);
        }
        case IR_LANG_TYPE_VOID:
            assert(ir_lang_type_void_const_unwrap(a).pointer_depth == 0);
            assert(ir_lang_type_void_const_unwrap(b).pointer_depth == 0);
            return true;
        case IR_LANG_TYPE_TUPLE:
            return ir_lang_type_tuple_is_equal(ir_lang_type_tuple_const_unwrap(a), ir_lang_type_tuple_const_unwrap(b));
        case IR_LANG_TYPE_FN:
            return ir_lang_type_fn_is_equal(ir_lang_type_fn_const_unwrap(a), ir_lang_type_fn_const_unwrap(b));
    }
    unreachable("");
}

static inline Ir_lang_type_vec ir_lang_type_vec_from_ir_lang_type(Ir_lang_type ir_lang_type) {
    Ir_lang_type_vec vec = {0};
    vec_append(&a_main, &vec, ir_lang_type);
    return vec;
}

static inline bool lang_type_vec_is_equal(Lang_type_vec a, Lang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!lang_type_is_equal(vec_at(a, idx), vec_at(b, idx))) {
            return false;
        }
    }

    return true;
}

static inline bool lang_type_tuple_is_equal(Lang_type_tuple a, Lang_type_tuple b) {
    return lang_type_vec_is_equal(a.lang_types, b.lang_types);
}

static inline bool lang_type_fn_is_equal(Lang_type_fn a, Lang_type_fn b) {
    return lang_type_tuple_is_equal(a.params, b.params) && lang_type_is_equal(*a.return_type, *b.return_type);
}

static inline bool lang_type_array_is_equal(Lang_type_array a, Lang_type_array b) {
    return lang_type_is_equal(*a.item_type, *b.item_type);
}

static inline bool lang_type_lit_is_equal(Lang_type_lit a, Lang_type_lit b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case LANG_TYPE_INT_LIT:
            return lang_type_int_lit_const_unwrap(a).data == lang_type_int_lit_const_unwrap(b).data;
        case LANG_TYPE_STRING_LIT:
            return strv_is_equal(lang_type_string_lit_const_unwrap(a).data, lang_type_string_lit_const_unwrap(b).data);
        case LANG_TYPE_STRUCT_LIT:
            return uast_expr_is_equal(lang_type_struct_lit_const_unwrap(a).data, lang_type_struct_lit_const_unwrap(b).data);
        case LANG_TYPE_FN_LIT:
            return name_is_equal(lang_type_fn_lit_const_unwrap(a).name, lang_type_fn_lit_const_unwrap(b).name);
        case LANG_TYPE_FLOAT_LIT:
            return lang_type_float_lit_const_unwrap(a).data == lang_type_float_lit_const_unwrap(b).data;
    }
    unreachable("");
}

static inline bool lang_type_primitive_is_equal(Lang_type_primitive a, Lang_type_primitive b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case LANG_TYPE_SIGNED_INT: {
            Lang_type_signed_int a_signed = lang_type_signed_int_const_unwrap(a);
            Lang_type_signed_int b_signed = lang_type_signed_int_const_unwrap(b);
            return a_signed.bit_width == b_signed.bit_width && a_signed.pointer_depth == b_signed.pointer_depth;
        }
        case LANG_TYPE_UNSIGNED_INT: {
            Lang_type_unsigned_int a_unsigned = lang_type_unsigned_int_const_unwrap(a);
            Lang_type_unsigned_int b_unsigned = lang_type_unsigned_int_const_unwrap(b);
            return a_unsigned.bit_width == b_unsigned.bit_width && a_unsigned.pointer_depth == b_unsigned.pointer_depth;
        }
        case LANG_TYPE_FLOAT: {
            Lang_type_float a_float = lang_type_float_const_unwrap(a);
            Lang_type_float b_float = lang_type_float_const_unwrap(b);
            return a_float.bit_width == b_float.bit_width && a_float.pointer_depth == b_float.pointer_depth;
        }
        case LANG_TYPE_OPAQUE: {
            Lang_type_opaque a_opaque = lang_type_opaque_const_unwrap(a);
            Lang_type_opaque b_opaque = lang_type_opaque_const_unwrap(b);
            return a_opaque.pointer_depth == b_opaque.pointer_depth;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

// TOOD: move these lang_type functions
static inline bool lang_type_is_equal(Lang_type a, Lang_type b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case LANG_TYPE_PRIMITIVE:
            return lang_type_primitive_is_equal(lang_type_primitive_const_unwrap(a), lang_type_primitive_const_unwrap(b));
        case LANG_TYPE_STRUCT: {
            Lang_type_struct a_struct = lang_type_struct_const_unwrap(a);
            Lang_type_struct b_struct = lang_type_struct_const_unwrap(b);
            return name_is_equal(a_struct.name, b_struct.name) && a_struct.pointer_depth == b_struct.pointer_depth;
        }
        case LANG_TYPE_RAW_UNION: {
            Lang_type_raw_union a_raw_union = lang_type_raw_union_const_unwrap(a);
            Lang_type_raw_union b_raw_union = lang_type_raw_union_const_unwrap(b);
            return name_is_equal(a_raw_union.name, b_raw_union.name) && a_raw_union.pointer_depth == b_raw_union.pointer_depth;
        }
        case LANG_TYPE_ENUM: {
            Lang_type_enum a_enum = lang_type_enum_const_unwrap(a);
            Lang_type_enum b_enum = lang_type_enum_const_unwrap(b);
            return name_is_equal(a_enum.name, b_enum.name) && a_enum.pointer_depth == b_enum.pointer_depth;
        }
        case LANG_TYPE_VOID:
            return true;
        case LANG_TYPE_TUPLE:
            return lang_type_tuple_is_equal(lang_type_tuple_const_unwrap(a), lang_type_tuple_const_unwrap(b));
        case LANG_TYPE_FN:
            return lang_type_fn_is_equal(lang_type_fn_const_unwrap(a), lang_type_fn_const_unwrap(b));
        case LANG_TYPE_ARRAY:
            return lang_type_array_is_equal(lang_type_array_const_unwrap(a), lang_type_array_const_unwrap(b));
        case LANG_TYPE_LIT:
            return lang_type_lit_is_equal(
                lang_type_lit_const_unwrap(a),
                lang_type_lit_const_unwrap(b)
            );
        case LANG_TYPE_REMOVED:
            return true;
        default:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type_vec lang_type_vec_from_lang_type(Lang_type lang_type) {
    Lang_type_vec vec = {0};
    vec_append(&a_main, &vec, lang_type);
    return vec;
}

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

Strv tast_print_internal(const Tast* tast, Indent indent);

static inline Lang_type tast_operator_get_lang_type(const Tast_operator* operator) {
    if (operator->type == TAST_UNARY) {
        return tast_unary_const_unwrap(operator)->lang_type;
    } else if (operator->type == TAST_BINARY) {
        return tast_binary_const_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type tast_string_get_lang_type(const Tast_string* str) {
    if (str->is_cstr) {
        return lang_type_struct_const_wrap(lang_type_struct_new(
            str->pos,
            name_new(MOD_PATH_BUILTIN, sv("u8"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0}),
            0
        ));
    }

    return lang_type_struct_const_wrap(lang_type_struct_new(
        str->pos,
        name_new(MOD_PATH_RUNTIME, sv("Slice"), ulang_type_gen_args_char_new(), SCOPE_TOP_LEVEL, (Attrs) {0}),
        0
    ));
}

static inline Lang_type tast_literal_get_lang_type(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_INT:
            return tast_int_const_unwrap(lit)->lang_type;
        case TAST_FLOAT:
            return tast_float_const_unwrap(lit)->lang_type;
        case TAST_STRING:
            return tast_string_get_lang_type(tast_string_const_unwrap(lit));
        case TAST_VOID:
            return lang_type_void_const_wrap(lang_type_void_new(tast_literal_get_pos(lit), 0));
        case TAST_ENUM_TAG_LIT:
            return tast_enum_tag_lit_const_unwrap(lit)->lang_type;
        case TAST_ENUM_LIT:
            return tast_enum_lit_const_unwrap(lit)->enum_lang_type;
        case TAST_RAW_UNION_LIT:
            return tast_expr_get_lang_type(tast_enum_lit_const_unwrap(lit)->item);
        case TAST_FUNCTION_LIT:
            return tast_function_lit_const_unwrap(lit)->lang_type;
    }
    unreachable("");
}

static inline Lang_type tast_if_else_chain_get_lang_type(const Tast_if_else_chain* if_else) {
    if (if_else->tasts.info.count < 1) {
        return lang_type_void_const_wrap(lang_type_void_new(if_else->pos, 0));
    }
    return vec_at(if_else->tasts, 0)->yield_type;
}

static inline Lang_type tast_expr_get_lang_type(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_BLOCK:
            return tast_block_const_unwrap(expr)->lang_type;
        case TAST_STRUCT_LITERAL:
            return tast_struct_literal_const_unwrap(expr)->lang_type;
        case TAST_FUNCTION_CALL:
            return tast_function_call_const_unwrap(expr)->lang_type;
        case TAST_MEMBER_ACCESS:
            return tast_member_access_const_unwrap(expr)->lang_type;
        case TAST_INDEX:
            return tast_index_const_unwrap(expr)->lang_type;
        case TAST_LITERAL:
            return tast_literal_get_lang_type(tast_literal_const_unwrap(expr));
        case TAST_OPERATOR:
            return tast_operator_get_lang_type(tast_operator_const_unwrap(expr));
        case TAST_SYMBOL:
            return tast_symbol_const_unwrap(expr)->base.lang_type;
        case TAST_TUPLE:
            return lang_type_tuple_const_wrap(tast_tuple_const_unwrap(expr)->lang_type);
        case TAST_ENUM_CALLEE:
            return tast_enum_callee_const_unwrap(expr)->enum_lang_type;
        case TAST_ENUM_CASE:
            return tast_enum_case_const_unwrap(expr)->enum_lang_type;
        case TAST_ENUM_ACCESS:
            return tast_enum_access_const_unwrap(expr)->lang_type;
        case TAST_ENUM_GET_TAG:
            return tast_expr_get_lang_type(tast_enum_get_tag_const_unwrap(expr)->callee);
        case TAST_ASSIGNMENT:
            return tast_expr_get_lang_type(tast_assignment_const_unwrap(expr)->lhs);
        case TAST_IF_ELSE_CHAIN:
            return tast_if_else_chain_get_lang_type(tast_if_else_chain_const_unwrap(expr));
        case TAST_MODULE_ALIAS:
            return lang_type_void_const_wrap(lang_type_void_new(tast_expr_get_pos(expr), 0));
    }
    unreachable("");
}

static inline Lang_type tast_raw_union_def_get_lang_type(const Tast_raw_union_def* def) {
    return lang_type_raw_union_const_wrap(lang_type_raw_union_new(def->pos, def->base.name, 0));
}

static inline Lang_type tast_struct_def_get_lang_type(const Tast_struct_def* def) {
    return lang_type_struct_const_wrap(lang_type_struct_new(def->pos, def->base.name, 0));
}

static inline Lang_type tast_enum_def_get_lang_type(const Tast_enum_def* def) {
    return lang_type_enum_const_wrap(lang_type_enum_new(def->pos, def->base.name, 0));
}

static inline Lang_type tast_def_get_lang_type(const Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            return tast_raw_union_def_get_lang_type(tast_raw_union_def_const_unwrap(def));
        case TAST_VARIABLE_DEF:
            return tast_variable_def_const_unwrap(def)->lang_type;
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            return tast_struct_def_get_lang_type(tast_struct_def_const_unwrap(def));
        case TAST_ENUM_DEF:
            return tast_enum_def_get_lang_type(tast_enum_def_const_unwrap(def));
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_IMPORT_PATH:
            unreachable("");
        case TAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_stmt_get_lang_type(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_def_get_lang_type(tast_def_const_unwrap(stmt));
        case TAST_EXPR:
            return tast_expr_get_lang_type(tast_expr_const_unwrap(stmt));
        case TAST_DEFER:
            return tast_stmt_get_lang_type(tast_defer_const_unwrap(stmt)->child);
        case TAST_RETURN:
            return tast_expr_get_lang_type(tast_return_const_unwrap(stmt)->child);
        case TAST_ACTUAL_BREAK:
            if (tast_actual_break_const_unwrap(stmt)->do_break_expr) {
                return tast_expr_get_lang_type(tast_actual_break_const_unwrap(stmt)->break_expr);
            }
            return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN, 0));
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_YIELD:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type(const Tast* tast) {
    (void) tast;
    unreachable("");
}

static inline Name tast_expr_get_name(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_BLOCK:
            unreachable("");
        case TAST_OPERATOR:
            unreachable("");
        case TAST_STRUCT_LITERAL:
            return tast_struct_literal_const_unwrap(expr)->name;
        case TAST_MEMBER_ACCESS:
            todo();
            //return tast_member_access_const_unwrap(expr)->member_name;
        case TAST_INDEX:
            unreachable("");
        case TAST_SYMBOL:
            return tast_symbol_const_unwrap(expr)->base.name;
        case TAST_FUNCTION_CALL:
            return tast_expr_get_name(tast_function_call_const_unwrap(expr)->callee);
        case TAST_LITERAL:
            todo();
        case TAST_TUPLE:
            unreachable("");
        case TAST_ENUM_CALLEE:
            unreachable("");
        case TAST_ENUM_CASE:
            unreachable("");
        case TAST_ENUM_ACCESS:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_ENUM_GET_TAG:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_MODULE_ALIAS:
            unreachable("");
    }
    unreachable("");
}

static inline Name tast_def_get_name(const Tast_def* def) {
    switch (def->type) {
        case TAST_PRIMITIVE_DEF: {
            Name name = {0};
            if (!lang_type_get_name(&name, LANG_TYPE_MODE_LOG, tast_primitive_def_const_unwrap(def)->lang_type)) {
                msg_todo("", tast_def_get_pos(def));
                return util_literal_name_new_poison();
            }
            return name;
        }
        case TAST_VARIABLE_DEF:
            return tast_variable_def_const_unwrap(def)->name;
        case TAST_STRUCT_DEF:
            return tast_struct_def_const_unwrap(def)->base.name;
        case TAST_RAW_UNION_DEF:
            return tast_raw_union_def_const_unwrap(def)->base.name;
        case TAST_FUNCTION_DECL:
            return tast_function_decl_const_unwrap(def)->name;
        case TAST_FUNCTION_DEF:
            return tast_function_def_const_unwrap(def)->decl->name;
        case TAST_ENUM_DEF:
            return tast_enum_def_const_unwrap(def)->base.name;
        case TAST_IMPORT_PATH:
            return name_new(MOD_PATH_OF_MOD_PATHS, tast_import_path_const_unwrap(def)->mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
        case TAST_LABEL:
            return tast_label_const_unwrap(def)->name;
    }
    unreachable("");
}

static inline Name tast_stmt_get_name(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_def_get_name(tast_def_const_unwrap(stmt));
        case TAST_EXPR:
            return tast_expr_get_name(tast_expr_const_unwrap(stmt));
        case TAST_DEFER:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_ACTUAL_BREAK:
            unreachable("");
        case TAST_YIELD:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
    unreachable("");
}

static inline Struct_def_base tast_def_get_struct_def_base(const Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            return tast_raw_union_def_const_unwrap(def)->base;
        case TAST_VARIABLE_DEF:
            unreachable("");
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            return tast_struct_def_const_unwrap(def)->base;
        case TAST_ENUM_DEF:
            return tast_enum_def_const_unwrap(def)->base;
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_IMPORT_PATH:
            todo();
        case TAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline Tast_def* tast_def_from_name(Name name) {
    Tast_def* def = NULL;
    // TODO: assert that name.base.count > 0
    unwrap(symbol_lookup(&def, name));
    return def;
}

static inline Lang_type tast_lang_type_from_name(Name name) {
    return tast_def_get_lang_type(tast_def_from_name(name));
}

Tast_literal* util_tast_literal_new_from_double(double value, Pos pos);

Tast_expr* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Tast_operator* util_binary_typed_new(Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operator_type, bool is_actually_used_as_expr);

Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child);

size_t struct_def_base_get_idx_largest_member(Struct_def_base base);

static inline size_t tast_get_member_index(const Struct_def_base* struct_def, Strv member_name) {
    log(LOG_TRACE, "tast_get_member_index: start\n");
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Tast_variable_def* curr_member = vec_at(struct_def->members, idx);
        if (strv_is_equal(curr_member->name.base, member_name)) {
            return idx;
        }
        log(LOG_TRACE, FMT" "FMT"\n", tast_print(curr_member), strv_print(member_name));
    }
    unreachable("member "FMT" of "FMT" not found", strv_print(member_name), name_print(NAME_LOG, struct_def->name));
}

static inline bool tast_operator_is_lvalue(const Tast_operator* oper) {
    switch (oper->type) {
        case TAST_BINARY:
            return false;
        case TAST_UNARY:
            return tast_unary_const_unwrap(oper)->token_type == UNARY_DEREF;
    }
    unreachable("");
}

static inline bool tast_expr_is_lvalue(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_BLOCK:
            return false;
        case TAST_MODULE_ALIAS:
            return false;
        case TAST_IF_ELSE_CHAIN:
            return false;
        case TAST_ASSIGNMENT:
            return false;
        case TAST_OPERATOR:
            return tast_operator_is_lvalue(tast_operator_const_unwrap(expr));
        case TAST_SYMBOL:
            return true;
        case TAST_MEMBER_ACCESS:
            return true;
        case TAST_INDEX:
            return true;
        case TAST_LITERAL:
            return false;
        case TAST_FUNCTION_CALL:
            return false;
        case TAST_STRUCT_LITERAL:
            return false;
        case TAST_TUPLE:
            return false;
        case TAST_ENUM_CALLEE:
            unreachable("this should not be in lhs");
        case TAST_ENUM_CASE:
            unreachable("this should not be in lhs");
        case TAST_ENUM_GET_TAG:
            return false;
        case TAST_ENUM_ACCESS:
            return false;
    }
    unreachable("");
}

#endif // TAST_UTIL_H
