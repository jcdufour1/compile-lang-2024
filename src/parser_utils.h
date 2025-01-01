#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "tast.h"
#include "llvm.h"
#include "tasts.h"
#include "symbol_table.h"
#include "tast_utils.h"
#include "uast_utils.h"
#include "llvm_utils.h"

bool lang_type_is_unsigned(Lang_type lang_type);

bool lang_type_is_signed(Lang_type lang_type);

bool lang_type_is_number(Lang_type lang_type);

Lang_type lang_type_unsigned_to_signed(Lang_type lang_type);

int64_t i_lang_type_to_bit_width(Lang_type lang_type);

int64_t str_view_to_int64_t(Str_view str_view);

bool try_str_view_to_int64_t(int64_t* result, Str_view str_view);

bool try_str_view_to_size_t(size_t* result, Str_view str_view);

Str_view util_literal_name_new(void);

Str_view util_literal_name_new_prefix(const char* debug_prefix);

Llvm_id get_prev_load_id(const Tast* var_call);

Str_view get_storage_location(const Env* env, Str_view sym_name);

static inline bool Llvm_reg_is_some(Llvm_reg llvm_reg) {
    Llvm_reg reference = {0};
    return 0 != memcmp(&reference, &llvm_reg, sizeof(llvm_reg));
}

static inline Llvm_reg llvm_register_sym_new(Llvm* llvm) {
    if (llvm) {
        Llvm_reg llvm_reg = {.lang_type = llvm_get_lang_type(llvm), .llvm = llvm};
        return llvm_reg;
    } else {
        Llvm_reg llvm_reg = {0};
        return llvm_reg;
    }
    unreachable("");
}

static inline Llvm_llvm_placeholder* llvm_llvm_placeholder_new_from_reg(
    Llvm_reg llvm_reg, Lang_type lang_type
) {
    return llvm_llvm_placeholder_new(llvm_get_pos(llvm_reg.llvm), lang_type, llvm_reg);
;
}

static inline Llvm_reg llvm_register_sym_new_from_expr(Llvm_expr* expr) {
    return llvm_register_sym_new(llvm_wrap_expr(expr));
}

static inline Llvm_reg llvm_register_sym_new_from_operator(Llvm_operator* operator) {
    return llvm_register_sym_new_from_expr(llvm_wrap_operator(operator));
}

Llvm_id get_matching_label_id(const Env* env, Str_view name);

// lhs and rhs should not be used for other tasks after this
Tast_assignment* util_assignment_new(Env* env, Uast* lhs, Uast_expr* rhs);

Tast_literal* util_tast_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos);

Uast_literal* util_uast_literal_new_from_strv(Str_view value, TOKEN_TYPE token_type, Pos pos);

Uast_literal* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Tast_operator* util_binary_typed_new(Env* env, Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operation_type);

//Tast_expr* util_unary_new(Env* env, Tast_expr* child, TOKEN_TYPE operation_type, Lang_type init_lang_type);

Llvm_id get_matching_fun_param_load_id(const Tast* src);

const Tast* get_lang_type_from_sym_definition(const Tast* sym_def);

uint64_t sizeof_lang_type(const Env* env, Lang_type lang_type);

uint64_t sizeof_item(const Env* env, const Tast* item);

uint64_t sizeof_struct(const Env* env, const Tast* struct_literal);

uint64_t sizeof_struct_def_base(const Env* env, const Struct_def_base* base);

uint64_t sizeof_struct_literal(const Env* env, const Tast_struct_literal* struct_literal);

static uint64_t llvm_sizeof_expr(const Env* env, const Llvm_expr* expr);

static uint64_t llvm_sizeof_def(const Env* env, const Llvm_def* def);

uint64_t llvm_sizeof_item(const Env* env, const Llvm* item);

uint64_t llvm_sizeof_struct_literal(const Env* env, const Llvm_struct_literal* struct_literal);

uint64_t llvm_sizeof_struct_def_base(const Env* env, const Struct_def_base* base);

uint64_t llvm_sizeof_struct_expr(const Env* env, const Llvm_expr* struct_literal_or_def);

bool lang_type_is_struct(const Env* env, Lang_type lang_type);

bool lang_type_is_raw_union(const Env* env, Lang_type lang_type);

bool lang_type_is_enum(const Env* env, Lang_type lang_type);

bool lang_type_is_primitive(const Env* env, Lang_type lang_type);

static inline size_t uast_get_member_index(const Ustruct_def_base* struct_def, Str_view member_name) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Uast* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_uast_name(curr_member), member_name)) {
            return idx;
        }
    }
    unreachable("member not found");
}

static inline size_t tast_get_member_index(const Struct_def_base* struct_def, Str_view member_name) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Tast* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_tast_name(curr_member), member_name)) {
            return idx;
        }
    }
    unreachable("member not found");
}

static inline bool uast_try_get_member_def(
    Uast_variable_def** member_def,
    const Ustruct_def_base* struct_def,
    Str_view member_name
) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        Uast* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_uast_name(curr_member), member_name)) {
            assert(uast_get_lang_type(curr_member).str.count > 0);
            *member_def = uast_unwrap_variable_def(uast_unwrap_def(curr_member));
            return true;
        }
    }
    return false;
}

static inline bool tast_try_get_member_def(
    Tast_variable_def** member_def,
    const Struct_def_base* struct_def,
    Str_view member_name
) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        Tast* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_tast_name(curr_member), member_name)) {
            assert(tast_get_lang_type(curr_member).str.count > 0);
            *member_def = tast_unwrap_variable_def(tast_unwrap_def(curr_member));
            return true;
        }
    }
    return false;
}

bool try_set_variable_def_types(
    Env* env,
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl
);

bool try_set_struct_def_types(Env* env, Tast_struct_def** new_tast, Uast_struct_def* tast);

bool try_set_raw_union_def_types(Env* env, Tast_raw_union_def** new_tast, Uast_raw_union_def* tast);

bool try_set_enum_def_types(Env* env, Tast_enum_def** new_tast, Uast_enum_def* tast);

bool try_get_struct_def(const Env* env, Tast_struct_def** struct_def, Tast* tast);

bool llvm_try_get_struct_def(const Env* env, Tast_struct_def** struct_def, Llvm* tast);
    
Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child);

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child);

static inline Tast_struct_def* get_struct_def(const Env* env, Tast* tast) {
    Tast_struct_def* struct_def;
    if (!try_get_struct_def(env, &struct_def, tast)) {
        unreachable("could not find struct definition for "TAST_FMT"\n", tast_print(tast));
    }
    return struct_def;
}

static inline const Tast_struct_def* get_struct_def_const(const Env* env, const Tast* tast) {
    return get_struct_def(env, (Tast*)tast);
}

static inline Tast_struct_def* llvm_get_struct_def(const Env* env, Llvm* tast) {
    Tast_struct_def* struct_def;
    if (!llvm_try_get_struct_def(env, &struct_def, tast)) {
        unreachable("could not find struct definition for "TAST_FMT"\n", llvm_print(tast));
    }
    return struct_def;
}

static inline const Tast_struct_def* llvm_get_struct_def_const(const Env* env, const Llvm* tast) {
    return llvm_get_struct_def(env, (Llvm*)tast);
}

#endif // PARSER_UTIL_H
