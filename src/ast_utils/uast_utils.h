#ifndef UAST_UTIL_H
#define UAST_UTIL_H

#include <uast.h>
#include <util.h>
#include <tast_utils.h>
#include <strv.h>
#include <strv_struct.h>
#include <lang_type_from_ulang_type.h>
#include <lang_type_print.h>
#include <uast_clone.h>
#include <ast_msg.h>

// TODO: figure out where to put these things
Strv ustruct_def_base_print_internal(Ustruct_def_base base, Indent indent);
#define ustruct_def_base_print(base) strv_print(ustruct_def_base_print_internal(base, 0))

static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def);
    
static inline bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

#define uast_printf(uast) \
    do { \
        log(LOG_NOTE, FMT"\n", uast_print(uast)); \
    } while (0);

bool uast_def_get_lang_type(Lang_type* result, const Uast_def* def, Ulang_type_vec generics, Pos dest_pos);

static inline bool uast_stmt_get_lang_type(Lang_type* result, const Uast_stmt* stmt, Ulang_type_vec generics, Pos dest_pos) {
    switch (stmt->type) {
        case UAST_EXPR:
            unreachable("");
        case UAST_DEF:
            return uast_def_get_lang_type(result,  uast_def_const_unwrap(stmt), generics, dest_pos);
        case UAST_RETURN:
            unreachable("");
        case UAST_FOR_WITH_COND:
            unreachable("");
        case UAST_ASSIGNMENT:
            unreachable("");
        case UAST_DEFER:
            unreachable("");
        case UAST_YIELD:
            unreachable("");
        case UAST_CONTINUE:
            unreachable("");
        case UAST_USING:
            unreachable("");
        case UAST_STMT_REMOVED:
            unreachable("");
    }
    unreachable("");
}

static inline bool uast_get_lang_type(Lang_type* result, const Uast* uast, Ulang_type_vec generics, Pos dest_pos) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_get_lang_type(result,  uast_stmt_const_unwrap(uast), generics, dest_pos);
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

static inline Name uast_primitive_def_get_name(const Uast_primitive_def* def) {
    // TODO: store Lang_type_primitive in Uast_prmitive_def instead of Lang_type?
    if (def->lang_type.type != LANG_TYPE_PRIMITIVE) {
        msg_todo("", def->pos);
        return util_literal_name_new_poison();
    }
    Lang_type_primitive lang_type = lang_type_primitive_const_unwrap(def->lang_type);

    return lang_type_primitive_get_name(lang_type);
}

static inline Name uast_def_get_name(const Uast_def* def) {
    switch (def->type) {
        case UAST_PRIMITIVE_DEF:
            return uast_primitive_def_get_name(uast_primitive_def_const_unwrap(def));
        case UAST_VOID_DEF:
            return name_new(MOD_PATH_BUILTIN, sv("void"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
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
            return uast_generic_param_const_unwrap(def)->name;
        case UAST_POISON_DEF:
            return uast_poison_def_const_unwrap(def)->name;
        case UAST_IMPORT_PATH:
            return name_new(MOD_PATH_OF_MOD_PATHS, uast_import_path_const_unwrap(def)->mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0});
        case UAST_MOD_ALIAS:
            return uast_mod_alias_const_unwrap(def)->name;
        case UAST_LANG_DEF:
            return uast_lang_def_const_unwrap(def)->alias_name;
        case UAST_LABEL:
            return uast_label_const_unwrap(def)->name;
        case UAST_BUILTIN_DEF:
            return uast_builtin_def_const_unwrap(def)->name;
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
            return false;
        case UAST_BUILTIN_DEF:
            return false;
        case UAST_LABEL:
            return false;
    }
    unreachable("");
}

static inline Ustruct_def_base uast_def_get_struct_def_base(const Uast_def* def) {
    Ustruct_def_base result = {0};
    unwrap(try_uast_def_get_struct_def_base(&result, def));
    return result;
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec generics, Pos pos);

Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl);

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child);

Uast_literal* util_uast_literal_new_from_double(double value, Pos pos);

Uast_expr* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Uast_expr* util_uast_literal_new_from_strv(const Strv value, TOKEN_TYPE token_type, Pos pos);

// will print error on failure
bool util_try_uast_literal_new_from_strv(Uast_expr** new_lit, const Strv value, TOKEN_TYPE token_type, Pos pos);

typedef enum {
    UAST_GET_MEMB_DEF_NONE,
    UAST_GET_MEMB_DEF_NORMAL,
    UAST_GET_MEMB_DEF_EXPR,

    // for static asserts
    UAST_GET_MEMB_DEF_COUNT,
} UAST_GET_MEMB_DEF;

// TODO: move this function to uast_utils.c file
static inline UAST_GET_MEMB_DEF uast_try_get_member_def(
    Uast_expr** new_expr,
    Uast_variable_def** member_def,
    const Ustruct_def_base* base,
    Strv member_name,
    Pos dest_pos
) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_variable_def* curr = vec_at(base->members, idx);
        if (strv_is_equal(curr->name.base, member_name)) {
            *member_def = curr;
            return UAST_GET_MEMB_DEF_NORMAL;
        }
    }

    vec_foreach(idx, Uast_generic_param*, gen_param, base->generics) {
        if (gen_param->is_expr && strv_is_equal(member_name, gen_param->name.base)) {
            if (vec_at(base->name.gen_args, idx).type != ULANG_TYPE_LIT) {
                msg_todo("non const expression here", dest_pos);
                return UAST_GET_MEMB_DEF_NONE;
            }

            Ulang_type_lit const_expr = ulang_type_lit_const_unwrap(vec_at(base->name.gen_args, idx));
            switch (const_expr.type) {
                case ULANG_TYPE_INT_LIT: {
                    Ulang_type_int_lit lit = ulang_type_int_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_int_wrap(uast_int_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_FLOAT_LIT: {
                    Ulang_type_float_lit lit = ulang_type_float_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_float_wrap(uast_float_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_STRING_LIT: {
                    Ulang_type_string_lit lit = ulang_type_string_lit_const_unwrap(const_expr);
                    *new_expr = uast_literal_wrap(uast_string_wrap(uast_string_new(dest_pos, lit.data)));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_STRUCT_LIT: {
                    Ulang_type_struct_lit lit = ulang_type_struct_lit_const_unwrap(const_expr);
                    Uast_expr* inner = uast_expr_clone(lit.expr, false, 0, lit.pos); // clone
                    unwrap(gen_param->is_expr);
                    *new_expr = uast_operator_wrap(uast_unary_wrap(uast_unary_new(
                        lit.pos,
                        inner,
                        UNARY_UNSAFE_CAST,
                        gen_param->expr_lang_type
                    )));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
                case ULANG_TYPE_FN_LIT: {
                    Ulang_type_fn_lit lit = ulang_type_fn_lit_const_unwrap(const_expr);
                    *new_expr = uast_symbol_wrap(uast_symbol_new(lit.pos, lit.name));
                    return UAST_GET_MEMB_DEF_EXPR;
                }
            }

            msg_todo("this type of expression in this situation", ulang_type_lit_get_pos(const_expr));
            return UAST_GET_MEMB_DEF_NONE;
        }
    }

    return UAST_GET_MEMB_DEF_NONE;
}

static inline size_t uast_get_member_index(const Ustruct_def_base* struct_def, Strv member_name) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Uast_variable_def* curr_member = vec_at(struct_def->members, idx);
        if (strv_is_equal(curr_member->name.base, member_name)) {
            return idx;
        }
    }
    unreachable("member not found");
}

Strv print_enum_def_member_internal(Lang_type enum_def_lang_type, size_t memb_idx);

#define print_enum_def_member(enum_def_lang_type, memb_idx) \
    strv_print(print_enum_def_member_internal(enum_def_lang_type, memb_idx))

#endif // UAST_UTIL_H
