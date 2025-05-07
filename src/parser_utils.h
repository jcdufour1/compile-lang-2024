#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "symbol_table.h"
#include "tast_utils.h"
#include "uast_utils.h"
#include "llvm_utils.h"
#include "ctype.h"

bool lang_type_atom_is_unsigned(Lang_type_atom atom);

bool lang_type_atom_is_signed(Lang_type_atom atom);

bool lang_type_atom_is_number(Lang_type_atom atom);

bool lang_type_is_number_like(Lang_type lang_type);

bool lang_type_is_number(Lang_type lang_type);

bool lang_type_is_unsigned(Lang_type lang_type);

bool lang_type_is_signed(Lang_type lang_type);

bool lang_type_atom_is_signed(Lang_type_atom atom);

bool lang_type_atom_is_unsigned(Lang_type_atom atom);

Lang_type_atom lang_type_atom_unsigned_to_signed(Lang_type_atom atom);

int64_t i_lang_type_atom_to_bit_width(const Lang_type_atom atom);

int64_t str_view_to_int64_t(const Pos pos, Str_view str_view);

bool try_str_view_to_int64_t(int64_t* result, const Pos pos, Str_view str_view);

bool try_str_view_to_size_t(size_t* result, Str_view str_view);

bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

Str_view util_literal_str_view_new_internal(const char* file, int line, const char* debug_prefix);

#define util_literal_str_view_new() \
    util_literal_str_view_new_internal(__FILE__, __LINE__, "")

Str_view util_literal_name_new_prefix_internal(const char* file, int line, const char* debug_prefix);

Name util_literal_name_new_prefix_internal_2(const char* file, int line, const char* debug_prefix, Str_view mod_path);

#define util_literal_name_new_prefix(debug_prefix) \
    util_literal_name_new_prefix_internal(__FILE__, __LINE__, debug_prefix)

#define util_literal_name_new_prefix2(debug_prefix) \
    util_literal_name_new_prefix_internal_2(__FILE__, __LINE__, debug_prefix, (Str_view) {0})

// TODO: make util_literal_name_new function/macro return Name and Uname, etc.
#define util_literal_name_new2(mod_path) \
    util_literal_name_new_prefix_internal_2(__FILE__, __LINE__, "", (Str_view) {0})

// TODO: make util_literal_name_new function/macro return Name and Uname, etc.
#define util_literal_name_new_mod_path2(mod_path) \
    util_literal_name_new_prefix_internal_2(__FILE__, __LINE__, "", mod_path)

Name get_storage_location(Name sym_name);

bool try_str_view_hex_after_0x_to_int64_t(int64_t* result, const Pos pos, Str_view str_view);

static inline bool ishex(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// lhs and rhs should not be used for other tasks after this
Tast_assignment* util_assignment_new(Uast_expr* lhs, Uast_expr* rhs);

Tast_literal* util_tast_literal_new_from_strv(const Str_view value, TOKEN_TYPE token_type, Pos pos);

bool util_try_uast_literal_new_from_strv(Uast_literal** new_lit, const Str_view value, TOKEN_TYPE token_type, Pos pos);

Uast_literal* util_uast_literal_new_from_strv(const Str_view value, TOKEN_TYPE token_type, Pos pos);

Uast_literal* util_uast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos);

Tast_operator* util_binary_typed_new(Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operation_type);

//Tast_expr* util_unary_new(Tast_expr* child, TOKEN_TYPE operation_type, Lang_type init_lang_type);

const Tast* from_sym_definition_get_lang_type(const Tast* sym_def);

size_t struct_def_base_get_idx_largest_member(Struct_def_base base);

// TODO: move to another file
static inline size_t uast_get_member_index(const Ustruct_def_base* struct_def, Str_view member_name) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Uast_variable_def* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(curr_member->name.base, member_name)) {
            return idx;
        }
    }
    unreachable("member not found");
}

// TODO: move to another file
static inline size_t tast_get_member_index(const Struct_def_base* struct_def, Str_view member_name) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Tast_variable_def* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(curr_member->name.base, member_name)) {
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
        Uast_variable_def* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(curr_member->name.base, member_name)) {
            *member_def = curr_member;
            return true;
        }
    }
    return false;
}

static inline bool tast_try_get_member_def(
    Tast_variable_def** member_def,
    const Struct_def_base* struct_def,
    Name member_name
) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        Tast_variable_def* curr_member = vec_at(&struct_def->members, idx);
        if (name_is_equal(curr_member->name, member_name)) {
            *member_def = curr_member;
            return true;
        }
    }
    return false;
}

bool try_set_variable_def_types(
     
    Tast_variable_def** new_tast,
    Uast_variable_def* uast,
    bool add_to_sym_tbl,
    bool is_variadic
);

bool try_get_struct_def(Tast_struct_def** struct_def, Tast_stmt* stmt);

bool llvm_try_get_struct_def(Tast_struct_def** struct_def, Llvm* tast);
    
Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child);

Uast_operator* uast_condition_get_default_child(Uast_expr* if_cond_child);

static inline Tast_struct_def* llvm_get_struct_def(Llvm* tast) {
    Tast_struct_def* struct_def;
    if (!llvm_try_get_struct_def( &struct_def, tast)) {
        unreachable("could not find struct definition for "TAST_FMT"\n", llvm_print(tast));
    }
    return struct_def;
}

static inline const Tast_struct_def* llvm_get_struct_def_const(const Llvm* tast) {
    return llvm_get_struct_def( (Llvm*)tast);
}

static inline bool is_struct_like(LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_STRUCT:
            return true;
        case LANG_TYPE_ENUM:
            return false;
        case LANG_TYPE_PRIMITIVE:
            return false;
        case LANG_TYPE_RAW_UNION:
            return true;
        case LANG_TYPE_VOID:
            return false;
        case LANG_TYPE_SUM:
            return true;
        case LANG_TYPE_TUPLE:
            return true;
        case LANG_TYPE_FN:
            return false;
    }
    unreachable("");
}

#endif // PARSER_UTIL_H
