#ifndef UAST_UTIL_H
#define UAST_UTIL_H

#include <uast.h>
#include <util.h>
#include <tast_utils.h>
#include <str_view.h>
#include <str_view_struct.h>
#include <lang_type_from_ulang_type.h>
#include <lang_type_print.h>
#include <ulang_type_get_pos.h>

// TODO: figure out where to put these things
Str_view ustruct_def_base_print_internal(Ustruct_def_base base, int indent);
#define ustruct_def_base_print(base) str_view_print(ustruct_def_base_print_internal(base, 0))

#define LANG_TYPE_FMT STR_VIEW_FMT

#ifndef UAST_FMT
#define UAST_FMT STR_VIEW_FMT
#endif // UAST_FMT

Str_view uast_print_internal(const Uast* uast, int recursion_depth);

Ulang_type uast_get_ulang_type_def(const Uast_def* def);

static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def);
    
static inline bool ustruct_def_base_get_lang_type_(Ulang_type* result, Env* env, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

#define uast_print(root) str_view_print(uast_print_internal(root, 0))

#define uast_printf(uast) \
    do { \
        log(LOG_NOTE, UAST_FMT"\n", uast_print(uast)); \
    } while (0);

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
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_LABEL:
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
        case UAST_PARAM:
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
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
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
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

bool uast_def_get_lang_type(Lang_type* result, Env* env, const Uast_def* def, Ulang_type_vec generics);

static inline bool uast_stmt_get_lang_type(Lang_type* result, Env* env, const Uast_stmt* stmt, Ulang_type_vec generics) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_def_get_lang_type(result, env, uast_def_const_unwrap(stmt), generics);
        case UAST_RETURN:
            unreachable("");
        case UAST_BREAK:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline bool uast_get_lang_type(Lang_type* result, Env* env, const Uast* uast, Ulang_type_vec generics) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_get_lang_type(result, env, uast_stmt_const_unwrap(uast), generics);
        case UAST_FUNCTION_PARAMS:
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
        case UAST_PARAM:
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
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
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
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_LABEL:
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

static inline Name uast_literal_def_get_name(const Uast_literal_def* lit_def) {
    switch (lit_def->type) {
        case UAST_STRUCT_LIT_DEF:
            return uast_struct_lit_def_const_unwrap(lit_def)->name;
        case UAST_STRING_DEF:
            return uast_string_def_const_unwrap(lit_def)->name;
        case UAST_VOID_DEF:
            return (Name) {.mod_path = (Str_view) {0}, .base = str_view_from_cstr("void")};
    }
    unreachable("");
}

static inline Name uast_def_get_name(const Uast_def* def) {
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
        case UAST_GENERIC_PARAM:
            todo();
            //return uast_generic_param_const_unwrap(def)->child->name;
        case UAST_POISON_DEF:
            return uast_poison_def_const_unwrap(def)->name;
        case UAST_IMPORT_PATH:
            return uast_import_path_const_unwrap(def)->mod_path;
        case UAST_MOD_ALIAS:
            return uast_mod_alias_const_unwrap(def)->name;
    }
    unreachable("");
}

static inline Str_view uast_get_name(const Uast* uast) {
    (void) uast;
    todo();
    //switch (uast->type) {
    //    case UAST_STMT:
    //        return uast_stmt_get_name(uast_stmt_const_unwrap(uast));
    //    case UAST_FUNCTION_PARAMS:
    //        unreachable("");
    //    case UAST_FOR_LOWER_BOUND:
    //        unreachable("");
    //    case UAST_FOR_UPPER_BOUND:
    //        unreachable("");
    //    case UAST_IF:
    //        unreachable("");
    //    case UAST_CONDITION:
    //        unreachable("");
    //    case UAST_CASE:
    //        unreachable("");
    //    case UAST_PARAM:
    //        return uast_param_const_unwrap(uast)->base->name;
    //}
    unreachable("");
}

static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def) {
    switch (def->type) {
        case UAST_SUM_DEF:
            return uast_sum_def_const_unwrap(def)->base;
        case UAST_ENUM_DEF:
            return uast_enum_def_const_unwrap(def)->base;
        case UAST_STRUCT_DEF:
            return uast_struct_def_const_unwrap(def)->base;
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_const_unwrap(def)->base;
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
    }
    unreachable("");
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Env* env, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

#endif // UAST_UTIL_H
