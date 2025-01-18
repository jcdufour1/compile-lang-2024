#ifndef UAST_UTIL_H
#define UAST_UTIL_H

#include <uast.h>
#include <util.h>
#include <tast_utils.h>
#include <str_view.h>
#include <str_view_struct.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

#ifndef UAST_FMT
#define UAST_FMT STR_VIEW_FMT
#endif // UAST_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

Str_view uast_print_internal(const Uast* uast, int recursion_depth);

#define uast_print(root) str_view_print(uast_print_internal(root, 0))

#define uast_printf(uast) \
    do { \
        log(LOG_NOTE, UAST_FMT"\n", uast_print(uast)); \
    } while (0);

static inline Lang_type uast_get_lang_type_def(const Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            return lang_type_new_from_strv(uast_unwrap_enum_def_const(def)->base.name, 0);
        case UAST_VARIABLE_DEF:
            return uast_unwrap_variable_def_const(def)->lang_type;
        case UAST_FUNCTION_DECL:
            try(uast_unwrap_function_decl_const(def)->return_type->lang_type.info.count == 1);
            return vec_at(&uast_unwrap_function_decl_const(def)->return_type->lang_type, 0);
        case UAST_STRUCT_DEF:
            return lang_type_new_from_strv(uast_unwrap_struct_def_const(def)->base.name, 0);
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_SUM_DEF:
            return lang_type_new_from_strv(uast_unwrap_sum_def_const(def)->base.name, 0);
    }
    unreachable("");
}

static inline Lang_type uast_get_lang_type_stmt(const Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_get_lang_type_def(uast_unwrap_def_const(stmt));
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

static inline Lang_type uast_get_lang_type(const Uast* uast) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_get_lang_type_stmt(uast_unwrap_stmt_const(uast));
        case UAST_FUNCTION_PARAMS:
            unreachable("");
        case UAST_LANG_TYPE:
            todo();
            //return uast_unwrap_lang_type_const(uast)->lang_type;
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

static inline Lang_type* uast_get_lang_type_def_ref(Uast_def* def) {
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

static inline Lang_type* uast_get_lang_type_ref_stmt(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_get_lang_type_def_ref(uast_unwrap_def(stmt));
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

static inline Str_view uast_get_literal_name(const Uast_literal* lit) {
    switch (lit->type) {
        case UAST_NUMBER:
            unreachable("");
        case UAST_STRING:
            return uast_unwrap_string_const(lit)->name;
        case UAST_VOID:
            unreachable("");
        case UAST_ENUM_LIT:
            unreachable("");
        case UAST_CHAR:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view get_uast_expr_name(const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            unreachable("");
        case UAST_STRUCT_LITERAL:
            return uast_unwrap_struct_literal_const(expr)->name;
        case UAST_MEMBER_ACCESS_UNTYPED:
            return uast_unwrap_member_access_untyped_const(expr)->member_name;
        case UAST_INDEX_UNTYPED:
            unreachable("");
        case UAST_SYMBOL_UNTYPED:
            return uast_unwrap_symbol_untyped_const(expr)->name;
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_LITERAL:
            return uast_get_literal_name(uast_unwrap_literal_const(expr));
        case UAST_TUPLE:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view uast_get_literal_def_name(const Uast_literal_def* lit_def) {
    switch (lit_def->type) {
        case UAST_STRUCT_LIT_DEF:
            return uast_unwrap_struct_lit_def_const(lit_def)->name;
        case UAST_STRING_DEF:
            return uast_unwrap_string_def_const(lit_def)->name;
    }
    unreachable("");
}

static inline Str_view get_uast_name_def(const Uast_def* def) {
    switch (def->type) {
        case UAST_PRIMITIVE_DEF:
            return uast_unwrap_primitive_def_const(def)->lang_type.str;
        case UAST_VARIABLE_DEF:
            return uast_unwrap_variable_def_const(def)->name;
        case UAST_STRUCT_DEF:
            return uast_unwrap_struct_def_const(def)->base.name;
        case UAST_RAW_UNION_DEF:
            return uast_unwrap_raw_union_def_const(def)->base.name;
        case UAST_ENUM_DEF:
            return uast_unwrap_enum_def_const(def)->base.name;
        case UAST_FUNCTION_DECL:
            return uast_unwrap_function_decl_const(def)->name;
        case UAST_FUNCTION_DEF:
            return uast_unwrap_function_def_const(def)->decl->name;
        case UAST_LITERAL_DEF:
            return uast_get_literal_def_name(uast_unwrap_literal_def_const(def));
        case UAST_SUM_DEF:
            return uast_unwrap_sum_def_const(def)->base.name;
    }
    unreachable("");
}

static inline Str_view get_uast_name_expr(const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_FUNCTION_CALL:
            todo();
        default:
            unreachable(UAST_FMT"\n", uast_expr_print(expr));
    }
    unreachable("");
}

static inline Str_view get_uast_name_stmt(const Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_DEF:
            return get_uast_name_def(uast_unwrap_def_const(stmt));
        case UAST_BLOCK:
            unreachable("");
        case UAST_EXPR:
            return get_uast_expr_name(uast_unwrap_expr_const(stmt));
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

static inline Str_view get_uast_name(const Uast* uast) {
    switch (uast->type) {
        case UAST_STMT:
            return get_uast_name_stmt(uast_unwrap_stmt_const(uast));
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
