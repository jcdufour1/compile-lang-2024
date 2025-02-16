#ifndef TAST_UTIL_H
#define TAST_UTIL_H

#include <tast.h>
#include <lang_type_after.h>
#include <ulang_type.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

static inline bool lang_type_is_equal(Lang_type a, Lang_type b);

static inline bool lang_type_atom_is_equal(Lang_type_atom a, Lang_type_atom b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return str_view_is_equal(a.str, b.str);
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

static inline bool lang_type_is_equal(Lang_type a, Lang_type b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case LANG_TYPE_PRIMITIVE:
            return lang_type_atom_is_equal(lang_type_primitive_const_unwrap(a).atom, lang_type_primitive_const_unwrap(b).atom);
        case LANG_TYPE_SUM:
            return lang_type_atom_is_equal(lang_type_sum_const_unwrap(a).atom, lang_type_sum_const_unwrap(b).atom);
        case LANG_TYPE_STRUCT:
            return lang_type_atom_is_equal(lang_type_struct_const_unwrap(a).atom, lang_type_struct_const_unwrap(b).atom);
        case LANG_TYPE_ENUM:
            return lang_type_atom_is_equal(lang_type_enum_const_unwrap(a).atom, lang_type_enum_const_unwrap(b).atom);
        case LANG_TYPE_RAW_UNION:
            return lang_type_atom_is_equal(lang_type_raw_union_const_unwrap(a).atom, lang_type_raw_union_const_unwrap(b).atom);
        case LANG_TYPE_TUPLE:
            return lang_type_tuple_is_equal(lang_type_tuple_const_unwrap(a), lang_type_tuple_const_unwrap(b));
        case LANG_TYPE_VOID:
            return true;
    }
    unreachable("");
}

static inline Lang_type_vec lang_type_vec_from_lang_type(Lang_type lang_type) {
    Lang_type_vec vec = {0};
    vec_append(&a_main, &vec, lang_type);
    return vec;
}

Str_view lang_type_print_internal(Lang_type lang_type, bool surround_in_lt_gt, bool do_tag);

Str_view ulang_type_print_internal(Ulang_type lang_type, bool surround_in_lt_gt);

Str_view lang_type_atom_print_internal(Lang_type_atom atom, bool surround_in_lt_gt);

#define lang_type_print(lang_type) str_view_print(lang_type_print_internal((lang_type), false, true))

#define ulang_type_print(lang_type) str_view_print(ulang_type_print_internal((lang_type), false))

#define lang_type_atom_print(atom) str_view_print(lang_type_atom_print_internal((atom), false))

Str_view tast_print_internal(const Tast* tast, int recursion_depth);

#define tast_print(root) str_view_print(tast_print_internal(root, 0))

#define tast_printf(tast) \
    do { \
        log(LOG_NOTE, TAST_FMT"\n", tast_print(tast)); \
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
        case TAST_NUMBER:
            return tast_number_const_unwrap(lit)->lang_type;
        case TAST_STRING:
            return lang_type_primitive_const_wrap(tast_string_const_unwrap(lit)->lang_type);
        case TAST_VOID:
            return lang_type_void_const_wrap(lang_type_void_new(0));
        case TAST_ENUM_LIT:
            return tast_enum_lit_const_unwrap(lit)->lang_type;
        case TAST_CHAR:
            return tast_char_const_unwrap(lit)->lang_type;
        case TAST_SUM_LIT:
            return tast_sum_lit_const_unwrap(lit)->lang_type;
    }
    unreachable("");
}

static inline void tast_literal_set_lang_type(Tast_literal* lit, Lang_type lang_type) {
    switch (lit->type) {
        case TAST_NUMBER:
            tast_number_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_STRING:
            todo();
        case TAST_VOID:
            unreachable("");
        case TAST_ENUM_LIT:
            tast_enum_lit_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_CHAR:
            tast_char_unwrap(lit)->lang_type = lang_type;
            return;
        case TAST_SUM_LIT:
            tast_sum_lit_unwrap(lit)->lang_type = lang_type;
            return;
    }
    unreachable("");
}

static inline Lang_type tast_expr_get_lang_type(const Tast_expr* expr) {
    switch (expr->type) {
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
        case TAST_SUM_CALLEE:
            unreachable("");
        case TAST_SUM_CASE:
            return tast_sum_case_const_unwrap(expr)->sum_lang_type;
        case TAST_SUM_ACCESS:
            return tast_sum_access_const_unwrap(expr)->lang_type;
    }
    unreachable("");
}

// TODO: remove set_lang_types functions
// TODO: remove get_lang_types functions
static inline void tast_set_lang_types_literal(Tast_literal* lit, Lang_type_vec types) {
    switch (lit->type) {
        case TAST_NUMBER:
            tast_number_unwrap(lit)->lang_type = vec_at(&types, 0);
            return;
        case TAST_VOID:
            unreachable("");
        case TAST_ENUM_LIT:
            unreachable("");
        case TAST_STRING:
            unreachable("");
            return;
        case TAST_CHAR:
            unreachable("");
            return;
        case TAST_SUM_LIT:
            unreachable("");
            return;
    }
    unreachable("");
}

static inline Lang_type tast_def_get_lang_type(const Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            unreachable("");
        case TAST_ENUM_DEF:
            return lang_type_enum_const_wrap(lang_type_enum_new(lang_type_atom_new(tast_enum_def_const_unwrap(def)->base.name, 0)));
        case TAST_VARIABLE_DEF:
            return tast_variable_def_const_unwrap(def)->lang_type;
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            return lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(tast_struct_def_const_unwrap(def)->base.name, 0)));
        case TAST_SUM_DEF:
            return lang_type_sum_const_wrap(lang_type_sum_new(lang_type_atom_new(tast_sum_def_const_unwrap(def)->base.name, 0)));
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline void tast_expr_set_lang_type(Tast_expr* expr, Lang_type lang_type) {
    (void) expr;
    (void) lang_type;

    switch (expr->type) {
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
        case TAST_SUM_CALLEE:
            todo();
        case TAST_SUM_CASE:
            unreachable("");
        case TAST_SUM_ACCESS:
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
        case TAST_BLOCK:
            unreachable("");
        case TAST_RETURN:
            return tast_expr_get_lang_type(tast_return_const_unwrap(stmt)->child);
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type(const Tast* tast) {
    switch (tast->type) {
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            todo();
            //return tast_lang_type_const_unwrap(tast)->lang_type;
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_CONDITION:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* tast_def_set_lang_type(Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            unreachable("");
        case TAST_ENUM_DEF:
            unreachable("");
        case TAST_VARIABLE_DEF:
            unreachable("");
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
        case TAST_SUM_DEF:
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
        case TAST_BLOCK:
            unreachable("");
        case TAST_RETURN:
            tast_expr_set_lang_type(tast_return_unwrap(stmt)->child, lang_type);
            return;
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view tast_literal_get_name(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_NUMBER:
            unreachable("");
        case TAST_STRING:
            return tast_string_const_unwrap(lit)->name;
        case TAST_VOID:
            unreachable("");
        case TAST_ENUM_LIT:
            unreachable("");
        case TAST_CHAR:
            unreachable("");
        case TAST_SUM_LIT:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view tast_expr_get_name(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_OPERATOR:
            unreachable("");
        case TAST_STRUCT_LITERAL:
            return tast_struct_literal_const_unwrap(expr)->name;
        case TAST_MEMBER_ACCESS:
            return tast_member_access_const_unwrap(expr)->member_name;
        case TAST_INDEX:
            unreachable("");
        case TAST_SYMBOL:
            return tast_symbol_const_unwrap(expr)->base.name;
        case TAST_FUNCTION_CALL:
            return tast_function_call_const_unwrap(expr)->name;
        case TAST_LITERAL:
            return tast_literal_get_name(tast_literal_const_unwrap(expr));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            unreachable("");
        case TAST_SUM_CASE:
            unreachable("");
        case TAST_SUM_ACCESS:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view tast_literal_def_get_name(const Tast_literal_def* lit_def) {
    switch (lit_def->type) {
        case TAST_STRUCT_LIT_DEF:
            return tast_struct_lit_def_const_unwrap(lit_def)->name;
        case TAST_STRING_DEF:
            return tast_string_def_const_unwrap(lit_def)->name;
    }
    unreachable("");
}

static inline Str_view tast_def_get_name(const Tast_def* def) {
    switch (def->type) {
        case TAST_PRIMITIVE_DEF:
            return lang_type_get_str(tast_primitive_def_const_unwrap(def)->lang_type);
        case TAST_VARIABLE_DEF:
            return tast_variable_def_const_unwrap(def)->name;
        case TAST_STRUCT_DEF:
            return tast_struct_def_const_unwrap(def)->base.name;
        case TAST_RAW_UNION_DEF:
            return tast_raw_union_def_const_unwrap(def)->base.name;
        case TAST_ENUM_DEF:
            return tast_enum_def_const_unwrap(def)->base.name;
        case TAST_FUNCTION_DECL:
            return tast_function_decl_const_unwrap(def)->name;
        case TAST_FUNCTION_DEF:
            return tast_function_def_const_unwrap(def)->decl->name;
        case TAST_LITERAL_DEF:
            return tast_literal_def_get_name(tast_literal_def_const_unwrap(def));
        case TAST_SUM_DEF:
            return tast_sum_def_const_unwrap(def)->base.name;
    }
    unreachable("");
}

static inline Str_view tast_stmt_get_name(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_def_get_name(tast_def_const_unwrap(stmt));
        case TAST_EXPR:
            return tast_expr_get_name(tast_expr_const_unwrap(stmt));
        case TAST_BLOCK:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view tast_get_name(const Tast* tast) {
    switch (tast->type) {
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_IF:
            unreachable("");
    }
    unreachable("");
}

#endif // TAST_UTIL_H
