#ifndef UAST_UTIL_H
#define UAST_UTIL_H

#include <uast.h>
#include <util.h>
#include <tast_utils.h>
#include <str_view.h>
#include <str_view_struct.h>
#include <lang_type_from_ulang_type.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

#ifndef UAST_FMT
#define UAST_FMT STR_VIEW_FMT
#endif // UAST_FMT

Str_view uast_print_internal(const Uast* uast, int recursion_depth);

#define uast_print(root) str_view_print(uast_print_internal(root, 0))

#define uast_printf(uast) \
    do { \
        log(LOG_NOTE, UAST_FMT"\n", uast_print(uast)); \
    } while (0);

static inline Ulang_type uast_get_ulang_type_def(const Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(uast_enum_def_const_unwrap(def)->base.name, 0)));
        case UAST_VARIABLE_DEF:
            return uast_variable_def_const_unwrap(def)->lang_type;
        case UAST_FUNCTION_DECL:
            return uast_function_decl_const_unwrap(def)->return_type->lang_type;
        case UAST_STRUCT_DEF:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(uast_struct_def_const_unwrap(def)->base.name, 0)));
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(uast_sum_def_const_unwrap(def)->base.name, 0)));
    }
    unreachable("");
}

static inline Ulang_type uast_get_ulang_type_stmt(const Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_get_ulang_type_def(uast_def_const_unwrap(stmt));
        case UAST_RETURN:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_RANGE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_IF_ELSE_CHAIN:
            unreachable("");
        case UAST_SWITCH:
            unreachable("");
    }
    unreachable("");
}

static inline Ulang_type uast_get_ulang_type(const Uast* uast) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_get_ulang_type_stmt(uast_stmt_const_unwrap(uast));
        case UAST_FUNCTION_PARAMS:
            unreachable("");
        case UAST_LANG_TYPE:
            todo();
            //return uast_lang_type_const_unwrap(uast)->lang_type;
        case UAST_FOR_LOWER_BOUND:
            unreachable("");
        case UAST_FOR_UPPER_BOUND:
            unreachable("");
        case UAST_IF:
            unreachable("");
        case UAST_CONDITION:
            unreachable("");
        case UAST_CASE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* uast_get_ulang_type_def_ref(Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_STRUCT_DEF:
            unreachable("");
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* uast_get_ulang_type_ref_stmt(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_get_ulang_type_def_ref(uast_def_unwrap(stmt));
        case UAST_RETURN:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_RANGE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_IF_ELSE_CHAIN:
            unreachable("");
        case UAST_SWITCH:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type uast_def_get_lang_type(const Env* env, const Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            return lang_type_enum_const_wrap(lang_type_enum_new(lang_type_atom_new(uast_enum_def_const_unwrap(def)->base.name, 0)));
        case UAST_VARIABLE_DEF:
            return lang_type_from_ulang_type(env, uast_variable_def_const_unwrap(def)->lang_type);
        case UAST_FUNCTION_DECL:
            return lang_type_from_ulang_type(env, uast_function_decl_const_unwrap(def)->return_type->lang_type);
        case UAST_STRUCT_DEF:
            return lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(uast_struct_def_const_unwrap(def)->base.name, 0)));
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            return lang_type_sum_const_wrap(lang_type_sum_new(lang_type_atom_new(uast_sum_def_const_unwrap(def)->base.name, 0)));
    }
    unreachable("");
}

static inline Lang_type uast_stmt_get_lang_type(const Env* env, const Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_def_get_lang_type(env, uast_def_const_unwrap(stmt));
        case UAST_RETURN:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_RANGE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_IF_ELSE_CHAIN:
            unreachable("");
        case UAST_SWITCH:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type uast_get_lang_type(const Env* env, const Uast* uast) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_get_lang_type(env, uast_stmt_const_unwrap(uast));
        case UAST_FUNCTION_PARAMS:
            unreachable("");
        case UAST_LANG_TYPE:
            todo();
            //return uast_lang_type_const_unwrap(uast)->lang_type;
        case UAST_FOR_LOWER_BOUND:
            unreachable("");
        case UAST_FOR_UPPER_BOUND:
            unreachable("");
        case UAST_IF:
            unreachable("");
        case UAST_CONDITION:
            unreachable("");
        case UAST_CASE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* uast_def_ref_get_lang_type(Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_STRUCT_DEF:
            unreachable("");
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* uast_ref_stmt_get_lang_type(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_def_ref_get_lang_type(uast_def_unwrap(stmt));
        case UAST_RETURN:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_RANGE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_IF_ELSE_CHAIN:
            unreachable("");
        case UAST_SWITCH:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view uast_literal_get_name(const Uast_literal* lit) {
    switch (lit->type) {
        case UAST_NUMBER:
            unreachable("");
        case UAST_STRING:
            return uast_string_const_unwrap(lit)->name;
        case UAST_VOID:
            unreachable("");
        case UAST_ENUM_LIT:
            unreachable("");
        case UAST_CHAR:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view uast_expr_get_name(const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            unreachable("");
        case UAST_STRUCT_LITERAL:
            return uast_struct_literal_const_unwrap(expr)->name;
        case UAST_MEMBER_ACCESS:
            return uast_member_access_const_unwrap(expr)->member_name;
        case UAST_INDEX:
            unreachable("");
        case UAST_SYMBOL:
            return uast_symbol_const_unwrap(expr)->name;
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_LITERAL:
            return uast_literal_get_name(uast_literal_const_unwrap(expr));
        case UAST_TUPLE:
            unreachable("");
        case UAST_SUM_ACCESS:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view uast_literal_def_get_name(const Uast_literal_def* lit_def) {
    switch (lit_def->type) {
        case UAST_STRUCT_LIT_DEF:
            return uast_struct_lit_def_const_unwrap(lit_def)->name;
        case UAST_STRING_DEF:
            return uast_string_def_const_unwrap(lit_def)->name;
        case UAST_VOID_DEF:
            return str_view_from_cstr("void");
    }
    unreachable("");
}

static inline Str_view uast_def_get_name(const Uast_def* def) {
    switch (def->type) {
        case UAST_PRIMITIVE_DEF:
            return lang_type_get_str(uast_primitive_def_const_unwrap(def)->lang_type);
        case UAST_VARIABLE_DEF:
            return uast_variable_def_const_unwrap(def)->name;
        case UAST_STRUCT_DEF:
            return uast_struct_def_const_unwrap(def)->base.name;
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_const_unwrap(def)->base.name;
        case UAST_ENUM_DEF:
            return uast_enum_def_const_unwrap(def)->base.name;
        case UAST_FUNCTION_DECL:
            return uast_function_decl_const_unwrap(def)->name;
        case UAST_FUNCTION_DEF:
            return uast_function_def_const_unwrap(def)->decl->name;
        case UAST_LITERAL_DEF:
            return uast_literal_def_get_name(uast_literal_def_const_unwrap(def));
        case UAST_SUM_DEF:
            return uast_sum_def_const_unwrap(def)->base.name;
    }
    unreachable("");
}

static inline Str_view uast_stmt_get_name(const Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_DEF:
            return uast_def_get_name(uast_def_const_unwrap(stmt));
        case UAST_BLOCK:
            unreachable("");
        case UAST_EXPR:
            return uast_expr_get_name(uast_expr_const_unwrap(stmt));
        case UAST_FOR_RANGE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_IF_ELSE_CHAIN:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_RETURN:
            unreachable("");
        case UAST_SWITCH:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view uast_get_name(const Uast* uast) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_get_name(uast_stmt_const_unwrap(uast));
        case UAST_FUNCTION_PARAMS:
            unreachable("");
        case UAST_LANG_TYPE:
            unreachable("");
        case UAST_FOR_LOWER_BOUND:
            unreachable("");
        case UAST_FOR_UPPER_BOUND:
            unreachable("");
        case UAST_IF:
            unreachable("");
        case UAST_CONDITION:
            unreachable("");
        case UAST_CASE:
            unreachable("");
    }
    unreachable("");
}

#endif // UAST_UTIL_H
