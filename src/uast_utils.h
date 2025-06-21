#ifndef UAST_UTIL_H
#define UAST_UTIL_H

#include <uast.h>
#include <util.h>
#include <tast_utils.h>
#include <strv.h>
#include <strv_struct.h>
#include <lang_type_from_ulang_type.h>
#include <lang_type_print.h>
#include <ulang_type_get_pos.h>

// TODO: figure out where to put these things
Strv ustruct_def_base_print_internal(Ustruct_def_base base, int indent);
#define ustruct_def_base_print(base) strv_print(ustruct_def_base_print_internal(base, 0))

Strv uast_print_internal(const Uast* uast, int recursion_depth);

Ulang_type uast_get_ulang_type_def(const Uast_def* def);

static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def);
    
static inline bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

#define uast_print(root) strv_print(uast_print_internal(root, 0))

#define uast_printf(uast) \
    do { \
        log(LOG_NOTE, FMT"\n", uast_print(uast)); \
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
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_DEFER:
            unreachable("");
        case UAST_YIELD:
            unreachable("");
        case UAST_CONTINUE2:
            unreachable("");
    }
    unreachable("");
}

bool uast_def_get_lang_type(Lang_type* result, const Uast_def* def, Ulang_type_vec generics);

static inline bool uast_stmt_get_lang_type(Lang_type* result, const Uast_stmt* stmt, Ulang_type_vec generics) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_BLOCK:
            unreachable("");
        case UAST_DEF:
            return uast_def_get_lang_type(result,  uast_def_const_unwrap(stmt), generics);
        case UAST_RETURN:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_DEFER:
            unreachable("");
        case UAST_YIELD:
            unreachable("");
        case UAST_CONTINUE2:
            unreachable("");
    }
    unreachable("");
}

static inline bool uast_get_lang_type(Lang_type* result, const Uast* uast, Ulang_type_vec generics) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_get_lang_type(result,  uast_stmt_const_unwrap(uast), generics);
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

static inline Name uast_def_get_name(const Uast_def* def) {
    switch (def->type) {
        case UAST_PRIMITIVE_DEF:
            return lang_type_get_str(LANG_TYPE_MODE_LOG, uast_primitive_def_const_unwrap(def)->lang_type);
        case UAST_VOID_DEF:
            return lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN)));
        case UAST_VARIABLE_DEF:
            return uast_variable_def_const_unwrap(def)->name;
        case UAST_STRUCT_DEF:
            return uast_struct_def_const_unwrap(def)->base.name;
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_const_unwrap(def)->base.name;
        case UAST_FUNCTION_DECL:
            return uast_function_decl_const_unwrap(def)->name;
        case UAST_FUNCTION_DEF:
            return uast_function_def_const_unwrap(def)->decl->name;
        case UAST_ENUM_DEF:
            return uast_enum_def_const_unwrap(def)->base.name;
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            return uast_poison_def_const_unwrap(def)->name;
        case UAST_IMPORT_PATH:
            return uast_import_path_const_unwrap(def)->mod_path;
        case UAST_MOD_ALIAS:
            return uast_mod_alias_const_unwrap(def)->name;
        case UAST_LANG_DEF:
            return uast_lang_def_const_unwrap(def)->alias_name;
        case UAST_LABEL:
            return uast_label_const_unwrap(def)->name;
    }
    unreachable("");
}

// TODO: this function should call try_uast_def_get_struct_def_base
static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def) {
    switch (def->type) {
        case UAST_ENUM_DEF:
            return uast_enum_def_const_unwrap(def)->base;
        case UAST_STRUCT_DEF:
            return uast_struct_def_const_unwrap(def)->base;
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_const_unwrap(def)->base;
        case UAST_VOID_DEF:
            unreachable("");
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_LANG_DEF:
            todo();
        case UAST_LABEL:
            unreachable("");
    }
    unreachable("");
}

static inline bool try_uast_def_get_struct_def_base(Ustruct_def_base* result, const Uast_def* def) {
    switch (def->type) {
        case UAST_ENUM_DEF:
            *result = uast_enum_def_const_unwrap(def)->base;
            return true;
        case UAST_STRUCT_DEF:
            *result = uast_struct_def_const_unwrap(def)->base;
            return true;
        case UAST_RAW_UNION_DEF:
            *result = uast_raw_union_def_const_unwrap(def)->base;
            return true;
        case UAST_VOID_DEF:
            return false;
        case UAST_FUNCTION_DEF:
            return false;
        case UAST_VARIABLE_DEF:
            return false;
        case UAST_GENERIC_PARAM:
            return false;
        case UAST_PRIMITIVE_DEF:
            return false;
        case UAST_FUNCTION_DECL:
            return false;
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            return false;
        case UAST_MOD_ALIAS:
            return false;
        case UAST_LANG_DEF:
            todo();
        case UAST_LABEL:
            return false;
    }
    unreachable("");
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

static inline bool ulang_type_vec_is_equal(Ulang_type_vec a, Ulang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!ulang_type_is_equal(vec_at(&a, idx), vec_at(&b, idx))) {
            return false;
        }
    }

    return true;
}

Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl);

#endif // UAST_UTIL_H
