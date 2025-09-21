#ifndef TAST_UTIL_H
#define TAST_UTIL_H

#include <tast.h>
#include <lang_type_after.h>
#include <llvm_lang_type_after.h>
#include <ulang_type.h>
#include <ulang_type_get_pos.h>

static inline bool lang_type_is_equal(Lang_type a, Lang_type b);

static inline bool llvm_lang_type_is_equal(Llvm_lang_type a, Llvm_lang_type b);

static inline Lang_type tast_expr_get_lang_type(const Tast_expr* expr);

static inline void tast_expr_set_lang_type(Tast_expr* expr, Lang_type lang_type);
    
static inline bool llvm_lang_type_atom_is_equal(Llvm_lang_type_atom a, Llvm_lang_type_atom b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return name_is_equal(a.str, b.str);
}

static inline bool llvm_lang_type_vec_is_equal(Llvm_lang_type_vec a, Llvm_lang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!llvm_lang_type_is_equal(vec_at(&a, idx), vec_at(&b, idx))) {
            return false;
        }
    }

    return true;
}

static inline bool llvm_lang_type_tuple_is_equal(Llvm_lang_type_tuple a, Llvm_lang_type_tuple b) {
    return llvm_lang_type_vec_is_equal(a.llvm_lang_types, b.llvm_lang_types);
}

static inline bool llvm_lang_type_fn_is_equal(Llvm_lang_type_fn a, Llvm_lang_type_fn b) {
    return llvm_lang_type_tuple_is_equal(a.params, b.params) && llvm_lang_type_is_equal(*a.return_type, *b.return_type);
}

static inline bool llvm_lang_type_is_equal(Llvm_lang_type a, Llvm_lang_type b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case LLVM_LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LLVM_LANG_TYPE_STRUCT:
            // fallthrough
        case LLVM_LANG_TYPE_VOID:
            return llvm_lang_type_atom_is_equal(llvm_lang_type_get_atom(LANG_TYPE_MODE_LOG, a), llvm_lang_type_get_atom(LANG_TYPE_MODE_LOG, b));
        case LLVM_LANG_TYPE_TUPLE:
            return llvm_lang_type_tuple_is_equal(llvm_lang_type_tuple_const_unwrap(a), llvm_lang_type_tuple_const_unwrap(b));
        case LLVM_LANG_TYPE_FN:
            return llvm_lang_type_fn_is_equal(llvm_lang_type_fn_const_unwrap(a), llvm_lang_type_fn_const_unwrap(b));
    }
    unreachable("");
}

static inline Llvm_lang_type_vec llvm_lang_type_vec_from_llvm_lang_type(Llvm_lang_type llvm_lang_type) {
    Llvm_lang_type_vec vec = {0};
    vec_append(&a_main, &vec, llvm_lang_type);
    return vec;
}

static inline bool lang_type_atom_is_equal(Lang_type_atom a, Lang_type_atom b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return name_is_equal(a.str, b.str);
}

static inline bool lang_type_vec_is_equal(Lang_type_vec a, Lang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!lang_type_is_equal(vec_at(&a, idx), vec_at(&b, idx))) {
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

static inline bool lang_type_is_equal(Lang_type a, Lang_type b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_VOID:
            return lang_type_atom_is_equal(lang_type_get_atom(LANG_TYPE_MODE_LOG, a), lang_type_get_atom(LANG_TYPE_MODE_LOG, b));
        case LANG_TYPE_TUPLE:
            return lang_type_tuple_is_equal(lang_type_tuple_const_unwrap(a), lang_type_tuple_const_unwrap(b));
        case LANG_TYPE_FN:
            return lang_type_fn_is_equal(lang_type_fn_const_unwrap(a), lang_type_fn_const_unwrap(b));
    }
    unreachable("");
}

static inline Lang_type_vec lang_type_vec_from_lang_type(Lang_type lang_type) {
    Lang_type_vec vec = {0};
    vec_append(&a_main, &vec, lang_type);
    return vec;
}

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type);

Strv tast_print_internal(const Tast* tast, int recursion_depth);

#define tast_print(root) strv_print(tast_print_internal(root, 0))

#define tast_printf(tast) \
    do { \
        log(LOG_NOTE, FMT"\n", tast_print(tast)); \
    } while (0);

static inline Lang_type tast_operator_get_lang_type(const Tast_operator* operator) {
    if (operator->type == TAST_UNARY) {
        return tast_unary_const_unwrap(operator)->lang_type;
    } else if (operator->type == TAST_BINARY) {
        return tast_binary_const_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline void tast_operator_set_lang_type(Tast_operator* operator, Lang_type lang_type) {
    if (operator->type == TAST_UNARY) {
        tast_unary_unwrap(operator)->lang_type = lang_type;
    } else if (operator->type == TAST_BINARY) {
        tast_binary_unwrap(operator)->lang_type = lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type tast_literal_get_lang_type(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_INT:
            return tast_int_const_unwrap(lit)->lang_type;
        case TAST_FLOAT:
            return tast_float_const_unwrap(lit)->lang_type;
        case TAST_STRING:
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(tast_literal_get_pos(lit), lang_type_atom_new_from_cstr("u8", 1, 0))));
        case TAST_VOID:
            return lang_type_void_const_wrap(lang_type_void_new(tast_literal_get_pos(lit)));
        case TAST_ENUM_TAG_LIT:
            return tast_enum_tag_lit_const_unwrap(lit)->lang_type;
        case TAST_CHAR:
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(tast_literal_get_pos(lit), lang_type_atom_new_from_cstr("u8", 0, 0))));
        case TAST_ENUM_LIT:
            return tast_enum_lit_const_unwrap(lit)->enum_lang_type;
        case TAST_RAW_UNION_LIT:
            return tast_expr_get_lang_type(tast_enum_lit_const_unwrap(lit)->item);
        case TAST_FUNCTION_LIT:
            return tast_function_lit_const_unwrap(lit)->lang_type;
    }
    unreachable("");
}

static inline void tast_literal_set_lang_type(Tast_literal* lit, Lang_type lang_type) {
    switch (lit->type) {
        case TAST_INT:
            tast_int_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_FLOAT:
            tast_float_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_STRING:
            tast_string_unwrap(lit)->is_cstr = true;
            return;
        case TAST_VOID:
            unreachable("");
        case TAST_ENUM_TAG_LIT:
            tast_enum_tag_lit_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_CHAR:
            unreachable("");
        case TAST_ENUM_LIT:
            tast_enum_lit_unwrap(lit)->enum_lang_type = lang_type;
            return;
        case TAST_RAW_UNION_LIT:
            tast_expr_set_lang_type(tast_raw_union_lit_unwrap(lit)->item, lang_type);
            return;
        case TAST_FUNCTION_LIT:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_if_else_chain_get_lang_type(const Tast_if_else_chain* if_else) {
    if (if_else->tasts.info.count < 1) {
        return lang_type_void_const_wrap(lang_type_void_new(if_else->pos));
    }
    return vec_at(&if_else->tasts, 0)->yield_type;
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
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            return tast_if_else_chain_get_lang_type(tast_if_else_chain_const_unwrap(expr));
        case TAST_MODULE_ALIAS:
            return (Lang_type) {0} /* TODO */;
    }
    unreachable("");
}

static inline Lang_type tast_raw_union_def_get_lang_type(const Tast_raw_union_def* def) {
    return lang_type_raw_union_const_wrap(lang_type_raw_union_new(def->pos, lang_type_atom_new(def->base.name, 0)));
}

static inline Lang_type tast_struct_def_get_lang_type(const Tast_struct_def* def) {
    return lang_type_struct_const_wrap(lang_type_struct_new(def->pos, lang_type_atom_new(def->base.name, 0)));
}

static inline Lang_type tast_enum_def_get_lang_type(const Tast_enum_def* def) {
    return lang_type_enum_const_wrap(lang_type_enum_new(def->pos, lang_type_atom_new(def->base.name, 0)));
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
        case TAST_IMPORT:
            unreachable("");
        case TAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline void tast_expr_set_lang_type(Tast_expr* expr, Lang_type lang_type) {
    switch (expr->type) {
        case TAST_BLOCK:
            unreachable("");
        case TAST_OPERATOR:
            todo();
        case TAST_SYMBOL:
            todo();
        case TAST_MEMBER_ACCESS:
            todo();
        case TAST_INDEX:
            todo();
        case TAST_LITERAL:
            tast_literal_set_lang_type(tast_literal_unwrap(expr), lang_type);
            return;
        case TAST_FUNCTION_CALL:
            tast_function_call_unwrap(expr)->lang_type = lang_type;
            return;
        case TAST_STRUCT_LITERAL:
            todo();
        case TAST_TUPLE:
            todo();
        case TAST_ENUM_CALLEE:
            todo();
        case TAST_ENUM_CASE:
            unreachable("");
        case TAST_ENUM_GET_TAG:
            unreachable("");
        case TAST_ENUM_ACCESS:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_MODULE_ALIAS:
            unreachable("");
    }
    todo();
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
            return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
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

static inline Lang_type* tast_def_set_lang_type(Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            unreachable("");
        case TAST_VARIABLE_DEF:
            unreachable("");
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_ENUM_DEF:
            unreachable("");
        case TAST_IMPORT:
            unreachable("");
        case TAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline void tast_stmt_set_lang_type(Tast_stmt* stmt, Lang_type lang_type) {
    switch (stmt->type) {
        case TAST_DEF:
            tast_def_set_lang_type(tast_def_unwrap(stmt));
            return;
        case TAST_EXPR:
            tast_expr_set_lang_type(tast_expr_unwrap(stmt), lang_type);
            return;
        case TAST_RETURN:
            tast_expr_set_lang_type(tast_return_unwrap(stmt)->child, lang_type);
            return;
        case TAST_ACTUAL_BREAK:
            tast_expr_set_lang_type(tast_actual_break_unwrap(stmt)->break_expr, lang_type);
            return;
        case TAST_DEFER:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_YIELD:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
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
        case TAST_PRIMITIVE_DEF:
            return lang_type_get_str(LANG_TYPE_MODE_LOG, tast_primitive_def_const_unwrap(def)->lang_type);
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
        case TAST_IMPORT:
            todo();
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
        case TAST_IMPORT:
            todo();
        case TAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline Tast_def* tast_def_from_name(Name name) {
    Tast_def* def = NULL;
    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name));
    // TODO: assert that name.base.count > 0
    unwrap(symbol_lookup(&def, name));
    return def;
}

static inline Lang_type tast_lang_type_from_name(Name name) {
    return tast_def_get_lang_type(tast_def_from_name(name));
}

#endif // TAST_UTIL_H
