#ifndef TAST_UTIL_H
#define TAST_UTIL_H

#include <tast.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

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
            return lang_type_atom_is_equal(lang_type_unwrap_primitive_const(a).atom, lang_type_unwrap_primitive_const(b).atom);
        case LANG_TYPE_SUM:
            return lang_type_atom_is_equal(lang_type_unwrap_sum_const(a).atom, lang_type_unwrap_sum_const(b).atom);
        case LANG_TYPE_STRUCT:
            return lang_type_atom_is_equal(lang_type_unwrap_struct_const(a).atom, lang_type_unwrap_struct_const(b).atom);
        case LANG_TYPE_ENUM:
            return lang_type_atom_is_equal(lang_type_unwrap_enum_const(a).atom, lang_type_unwrap_enum_const(b).atom);
        case LANG_TYPE_RAW_UNION:
            return lang_type_atom_is_equal(lang_type_unwrap_raw_union_const(a).atom, lang_type_unwrap_raw_union_const(b).atom);
        case LANG_TYPE_TUPLE:
            return lang_type_tuple_is_equal(lang_type_unwrap_tuple_const(a), lang_type_unwrap_tuple_const(b));
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

Str_view lang_type_print_internal(Arena* arena, Lang_type lang_type, bool surround_in_lt_gt);

#define lang_type_print(lang_type) str_view_print(lang_type_print_internal(&print_arena, (lang_type), false))

Str_view tast_print_internal(const Tast* tast, int recursion_depth);

#define tast_print(root) str_view_print(tast_print_internal(root, 0))

#define tast_printf(tast) \
    do { \
        log(LOG_NOTE, TAST_FMT"\n", tast_print(tast)); \
    } while (0);

static inline Lang_type tast_get_lang_type_operator(const Tast_operator* operator) {
    if (operator->type == TAST_UNARY) {
        return tast_unwrap_unary_const(operator)->lang_type;
    } else if (operator->type == TAST_BINARY) {
        return tast_unwrap_binary_const(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type* get_operator_lang_type_ref(Tast_operator* operator) {
    if (operator->type == TAST_UNARY) {
        return &tast_unwrap_unary(operator)->lang_type;
    } else if (operator->type == TAST_BINARY) {
        return &tast_unwrap_binary(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Sym_typed_base* tast_symbol_typed_get_base_ref(Tast_symbol_typed* sym) {
    switch (sym->type) {
        case TAST_PRIMITIVE_SYM:
            return &tast_unwrap_primitive_sym(sym)->base;
        case TAST_STRUCT_SYM:
            return &tast_unwrap_struct_sym(sym)->base;
        case TAST_ENUM_SYM:
            return &tast_unwrap_enum_sym(sym)->base;
        case TAST_RAW_UNION_SYM:
            return &tast_unwrap_raw_union_sym(sym)->base;
        case TAST_SUM_SYM:
            return &tast_unwrap_sum_sym(sym)->base;
    }
    unreachable("");
}

static inline Sym_typed_base tast_symbol_typed_get_base_const(const Tast_symbol_typed* sym) {
    return *tast_symbol_typed_get_base_ref((Tast_symbol_typed*)sym);
}

static inline Str_view tast_get_symbol_typed_name(const Tast_symbol_typed* sym) {
    return tast_symbol_typed_get_base_const(sym).name;
}

static inline Lang_type_vec tast_get_lang_type_symbol_typed(const Tast_symbol_typed* sym) {
    return tast_symbol_typed_get_base_const(sym).lang_type;
}

static inline Lang_type tast_get_lang_type_literal(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_NUMBER:
            return tast_unwrap_number_const(lit)->lang_type;
        case TAST_STRING:
            return tast_unwrap_string_const(lit)->lang_type;
        case TAST_VOID:
            return lang_type_wrap_void_const(lang_type_void_new(0));
        case TAST_ENUM_LIT:
            return tast_unwrap_enum_lit_const(lit)->lang_type;
        case TAST_CHAR:
            return tast_unwrap_char_const(lit)->lang_type;
        case TAST_SUM_LIT:
            return tast_unwrap_sum_lit_const(lit)->lang_type;
    }
    unreachable("");
}

static inline Lang_type* tast_get_lang_type_literal_ref(Tast_literal* lit) {
    switch (lit->type) {
        case TAST_NUMBER:
            return &tast_unwrap_number(lit)->lang_type;
        case TAST_STRING:
            return &tast_unwrap_string(lit)->lang_type;
        case TAST_VOID:
            unreachable("");
        case TAST_ENUM_LIT:
            return &tast_unwrap_enum_lit(lit)->lang_type;
        case TAST_CHAR:
            return &tast_unwrap_char(lit)->lang_type;
        case TAST_SUM_LIT:
            return &tast_unwrap_sum_lit(lit)->lang_type;
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_expr(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            return tast_unwrap_struct_literal_const(expr)->lang_type;
        case TAST_FUNCTION_CALL:
            try(tast_unwrap_function_call_const(expr)->lang_type.info.count == 1);
            return vec_at(&tast_unwrap_function_call_const(expr)->lang_type, 0);
        case TAST_MEMBER_ACCESS_TYPED:
            return tast_unwrap_member_access_typed_const(expr)->lang_type;
        case TAST_INDEX_TYPED:
            return tast_unwrap_index_typed_const(expr)->lang_type;
        case TAST_LITERAL:
            return tast_get_lang_type_literal(tast_unwrap_literal_const(expr));
        case TAST_OPERATOR:
            return tast_get_lang_type_operator(tast_unwrap_operator_const(expr));
        case TAST_SYMBOL_TYPED:
            try(tast_symbol_typed_get_base_const(tast_unwrap_symbol_typed_const(expr)).lang_type.info.count == 1);
            return vec_at(&tast_symbol_typed_get_base_ref((Tast_symbol_typed*)tast_unwrap_symbol_typed_const(expr))->lang_type, 0);
        case TAST_TUPLE: {
            const Tast_tuple* tuple = tast_unwrap_tuple_const(expr);
            try(tuple->lang_type.info.count == tuple->members.info.count);
            try(tuple->lang_type.info.count == 1);
            return vec_at(&tuple->lang_type, 0);
        }
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

// TODO: reduce allocations
static inline Lang_type_vec tast_get_lang_types_expr(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            return lang_type_vec_from_lang_type(tast_unwrap_struct_literal_const(expr)->lang_type);
        case TAST_FUNCTION_CALL:
            return tast_unwrap_function_call_const(expr)->lang_type;
        case TAST_MEMBER_ACCESS_TYPED:
            return lang_type_vec_from_lang_type(tast_unwrap_member_access_typed_const(expr)->lang_type);
        case TAST_INDEX_TYPED:
            return lang_type_vec_from_lang_type(tast_unwrap_index_typed_const(expr)->lang_type);
        case TAST_LITERAL:
            return lang_type_vec_from_lang_type(tast_get_lang_type_literal(tast_unwrap_literal_const(expr)));
        case TAST_OPERATOR:
            return lang_type_vec_from_lang_type(tast_get_lang_type_operator(tast_unwrap_operator_const(expr)));
        case TAST_SYMBOL_TYPED:
            return tast_symbol_typed_get_base_const(tast_unwrap_symbol_typed_const(expr)).lang_type;
        case TAST_TUPLE:
            return tast_unwrap_tuple_const(expr)->lang_type;
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type_vec tast_get_lang_types_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            return tast_unwrap_function_def_const(def)->decl->return_type->lang_type;
        case TAST_VARIABLE_DEF:
            return lang_type_vec_from_lang_type(tast_unwrap_variable_def_const(def)->lang_type);
        default:
            unreachable(TAST_FMT, tast_def_print(def));
    }
}

static inline Lang_type_vec tast_get_lang_types_stmt(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_get_lang_types_def(tast_unwrap_def_const(stmt));
        case TAST_EXPR:
            return tast_get_lang_types_expr(tast_unwrap_expr_const(stmt));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_CONTINUE:
            unreachable("");
        case TAST_IF_ELSE_CHAIN:
            unreachable("");
        case TAST_ASSIGNMENT:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type_vec tast_get_lang_types(const Tast* tast) {
    switch (tast->type) {
        case TAST_STMT:
            return tast_get_lang_types_stmt(tast_unwrap_stmt_const(tast));
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            todo();
            //return tast_unwrap_lang_type_const(tast)->lang_type;
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
            unreachable("");
    }
    unreachable("");
}

static inline void tast_set_lang_types_literal(Tast_literal* lit, Lang_type_vec types) {
    switch (lit->type) {
        case TAST_NUMBER:
            tast_unwrap_number(lit)->lang_type = vec_at(&types, 0);
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

static inline void tast_set_lang_types_expr(Tast_expr* expr, Lang_type_vec types) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            todo();
        case TAST_FUNCTION_CALL:
            unreachable("");
            //return tast_unwrap_function_call_const(expr)->lang_type;
        case TAST_MEMBER_ACCESS_TYPED:
            todo();
        case TAST_INDEX_TYPED:
            todo();
        case TAST_LITERAL:
            tast_set_lang_types_literal(tast_unwrap_literal(expr), types);
            return;
        case TAST_OPERATOR:
            todo();
        case TAST_SYMBOL_TYPED:
            todo();
        case TAST_TUPLE: {
            Tast_tuple* tuple = tast_unwrap_tuple(expr);
            tuple->lang_type = types;
            try(tuple->lang_type.info.count == tuple->members.info.count);
            return;
        }
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            unreachable("");
        case TAST_ENUM_DEF:
            return lang_type_new_from_strv(tast_unwrap_enum_def_const(def)->base.name, 0);
        case TAST_VARIABLE_DEF:
            return tast_unwrap_variable_def_const(def)->lang_type;
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_STRUCT_DEF:
            return lang_type_new_from_strv(tast_unwrap_struct_def_const(def)->base.name, 0);
        case TAST_SUM_DEF:
            return lang_type_new_from_strv(tast_unwrap_sum_def_const(def)->base.name, 0);
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* tast_get_lang_type_expr_ref(Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            return &tast_unwrap_struct_literal(expr)->lang_type;
        case TAST_FUNCTION_CALL:
            try(tast_unwrap_function_call_const(expr)->lang_type.info.count == 1);
            return &vec_at(&tast_unwrap_function_call(expr)->lang_type, 0);
        case TAST_MEMBER_ACCESS_TYPED:
            unreachable("");
        case TAST_INDEX_TYPED:
            unreachable("");
        case TAST_LITERAL:
            return tast_get_lang_type_literal_ref(tast_unwrap_literal(expr));
        case TAST_SYMBOL_TYPED:
            return &tast_symbol_typed_get_base_ref(tast_unwrap_symbol_typed(expr))->lang_type;
        case TAST_OPERATOR:
            return get_operator_lang_type_ref(tast_unwrap_operator(expr));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_stmt(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_get_lang_type_def(tast_unwrap_def_const(stmt));
        case TAST_EXPR:
            return tast_get_lang_type_expr(tast_unwrap_expr_const(stmt));
        case TAST_BLOCK:
            unreachable("");
        case TAST_RETURN:
            return tast_get_lang_type_expr(tast_unwrap_return_const(stmt)->child);
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
            //return tast_unwrap_lang_type_const(tast)->lang_type;
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

static inline Lang_type* tast_get_lang_type_def_ref(Tast_def* def) {
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

static inline Lang_type* tast_get_lang_type_stmt_ref(Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return tast_get_lang_type_def_ref(tast_unwrap_def(stmt));
        case TAST_EXPR:
            return tast_get_lang_type_expr_ref(tast_unwrap_expr(stmt));
        case TAST_BLOCK:
            unreachable("");
        case TAST_RETURN:
            return tast_get_lang_type_expr_ref(tast_unwrap_return(stmt)->child);
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

static inline Lang_type* tast_get_lang_type_ref(Tast* tast) {
    switch (tast->type) {
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            todo();
            //return &tast_unwrap_lang_type(tast)->lang_type;
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view get_literal_name(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_NUMBER:
            unreachable("");
        case TAST_STRING:
            return tast_unwrap_string_const(lit)->name;
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

static inline Str_view get_expr_name(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_OPERATOR:
            unreachable("");
        case TAST_STRUCT_LITERAL:
            return tast_unwrap_struct_literal_const(expr)->name;
        case TAST_MEMBER_ACCESS_TYPED:
            return tast_unwrap_member_access_typed_const(expr)->member_name;
        case TAST_INDEX_TYPED:
            unreachable("");
        case TAST_SYMBOL_TYPED:
            return tast_get_symbol_typed_name(tast_unwrap_symbol_typed_const(expr));
        case TAST_FUNCTION_CALL:
            return tast_unwrap_function_call_const(expr)->name;
        case TAST_LITERAL:
            return get_literal_name(tast_unwrap_literal_const(expr));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view get_literal_def_name(const Tast_literal_def* lit_def) {
    switch (lit_def->type) {
        case TAST_STRUCT_LIT_DEF:
            return tast_unwrap_struct_lit_def_const(lit_def)->name;
        case TAST_STRING_DEF:
            return tast_unwrap_string_def_const(lit_def)->name;
    }
    unreachable("");
}

static inline Str_view get_def_name(const Tast_def* def) {
    switch (def->type) {
        case TAST_PRIMITIVE_DEF:
            return tast_unwrap_primitive_def_const(def)->lang_type.str;
        case TAST_VARIABLE_DEF:
            return tast_unwrap_variable_def_const(def)->name;
        case TAST_STRUCT_DEF:
            return tast_unwrap_struct_def_const(def)->base.name;
        case TAST_RAW_UNION_DEF:
            return tast_unwrap_raw_union_def_const(def)->base.name;
        case TAST_ENUM_DEF:
            return tast_unwrap_enum_def_const(def)->base.name;
        case TAST_FUNCTION_DECL:
            return tast_unwrap_function_decl_const(def)->name;
        case TAST_FUNCTION_DEF:
            return tast_unwrap_function_def_const(def)->decl->name;
        case TAST_LITERAL_DEF:
            return get_literal_def_name(tast_unwrap_literal_def_const(def));
        case TAST_SUM_DEF:
            return tast_unwrap_sum_def_const(def)->base.name;
    }
    unreachable("");
}

static inline Str_view get_tast_name_expr(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_SYMBOL_TYPED:
            return tast_get_symbol_typed_name(tast_unwrap_symbol_typed_const(expr));
        case TAST_FUNCTION_CALL:
            return tast_unwrap_function_call_const(expr)->name;
        default:
            unreachable(TAST_FMT"\n", tast_expr_print(expr));
    }
    unreachable("");
}

static inline Str_view get_tast_name_stmt(const Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_DEF:
            return get_def_name(tast_unwrap_def_const(stmt));
        case TAST_EXPR:
            return get_expr_name(tast_unwrap_expr_const(stmt));
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

static inline Str_view get_tast_name(const Tast* tast) {
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
