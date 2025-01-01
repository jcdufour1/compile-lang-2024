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

static inline Lang_type lang_type_new_from_strv(Str_view str_view, int16_t pointer_depth) {
    Lang_type Lang_type = {.str = str_view, .pointer_depth = pointer_depth};
    assert(str_view.count < 1e9);
    return Lang_type;
}

// only literals can be used here
static inline Lang_type lang_type_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return lang_type_new_from_strv(str_view_from_cstr(cstr), pointer_depth);
}

static inline bool lang_type_is_equal(Lang_type a, Lang_type b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return str_view_is_equal(a.str, b.str);
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
    }
    unreachable("");
}

static inline Sym_typed_base tast_symbol_typed_get_base_const(const Tast_symbol_typed* sym) {
    return *tast_symbol_typed_get_base_ref((Tast_symbol_typed*)sym);
}

static inline Str_view tast_get_symbol_typed_name(const Tast_symbol_typed* sym) {
    switch (sym->type) {
        case TAST_PRIMITIVE_SYM:
            return tast_unwrap_primitive_sym_const(sym)->base.name;
        case TAST_STRUCT_SYM:
            return tast_unwrap_struct_sym_const(sym)->base.name;
        case TAST_ENUM_SYM:
            return tast_unwrap_enum_sym_const(sym)->base.name;
        case TAST_RAW_UNION_SYM:
            return tast_unwrap_raw_union_sym_const(sym)->base.name;
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_symbol_typed(const Tast_symbol_typed* sym) {
    switch (sym->type) {
        case TAST_PRIMITIVE_SYM:
            return tast_unwrap_primitive_sym_const(sym)->base.lang_type;
        case TAST_STRUCT_SYM:
            return tast_unwrap_struct_sym_const(sym)->base.lang_type;
        case TAST_ENUM_SYM:
            return tast_unwrap_enum_sym_const(sym)->base.lang_type;
        case TAST_RAW_UNION_SYM:
            return tast_unwrap_raw_union_sym_const(sym)->base.lang_type;
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_literal(const Tast_literal* lit) {
    switch (lit->type) {
        case TAST_NUMBER:
            return tast_unwrap_number_const(lit)->lang_type;
        case TAST_STRING:
            return tast_unwrap_string_const(lit)->lang_type;
        case TAST_VOID:
            return lang_type_new_from_cstr("void", 0);
        case TAST_ENUM_LIT:
            return tast_unwrap_enum_lit_const(lit)->lang_type;
        case TAST_CHAR:
            return tast_unwrap_char_const(lit)->lang_type;
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
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type_expr(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            return tast_unwrap_struct_literal_const(expr)->lang_type;
        case TAST_FUNCTION_CALL:
            return tast_unwrap_function_call_const(expr)->lang_type;
        case TAST_MEMBER_ACCESS_TYPED:
            return tast_unwrap_member_access_typed_const(expr)->lang_type;
        case TAST_INDEX_TYPED:
            return tast_unwrap_index_typed_const(expr)->lang_type;
        case TAST_LITERAL:
            return tast_get_lang_type_literal(tast_unwrap_literal_const(expr));
        case TAST_OPERATOR:
            return tast_get_lang_type_operator(tast_unwrap_operator_const(expr));
        case TAST_SYMBOL_TYPED:
            return tast_symbol_typed_get_base_const(tast_unwrap_symbol_typed_const(expr)).lang_type;
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
            return &tast_unwrap_function_call(expr)->lang_type;
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
    }
    unreachable("");
}

static inline Lang_type tast_get_lang_type(const Tast* tast) {
    switch (tast->type) {
        case TAST_DEF:
            return tast_get_lang_type_def(tast_unwrap_def_const(tast));
        case TAST_EXPR:
            return tast_get_lang_type_expr(tast_unwrap_expr_const(tast));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            return tast_unwrap_lang_type_const(tast)->lang_type;
        case TAST_RETURN:
            return tast_get_lang_type_expr(tast_unwrap_return_const(tast)->child);
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
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
    }
    unreachable("");
}

static inline Lang_type* tast_get_lang_type_ref(Tast* tast) {
    switch (tast->type) {
        case TAST_DEF:
            return tast_get_lang_type_def_ref(tast_unwrap_def(tast));
        case TAST_EXPR:
            return tast_get_lang_type_expr_ref(tast_unwrap_expr(tast));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            return &tast_unwrap_lang_type(tast)->lang_type;
        case TAST_RETURN:
            return tast_get_lang_type_expr_ref(tast_unwrap_return(tast)->child);
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
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

static inline Tast* get_expr_src(Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            unreachable("");
        case TAST_MEMBER_ACCESS_TYPED:
            unreachable("");
        case TAST_INDEX_TYPED:
            unreachable("");
        case TAST_LITERAL:
            unreachable("");
        case TAST_SYMBOL_TYPED:
            unreachable("");
        case TAST_FUNCTION_CALL:
            unreachable("");
        case TAST_OPERATOR:
            unreachable("");
    }
    unreachable("");
}

static inline Tast* get_tast_src(Tast* tast) {
    switch (tast->type) {
        case TAST_DEF:
            unreachable("");
        case TAST_EXPR:
            return get_expr_src(tast_unwrap_expr(tast));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
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

static inline Tast* get_expr_dest(Tast_expr* expr) {
    switch (expr->type) {
        case TAST_STRUCT_LITERAL:
            unreachable("");
        case TAST_MEMBER_ACCESS_TYPED:
            unreachable("");
        case TAST_INDEX_TYPED:
            unreachable("");
        case TAST_LITERAL:
            unreachable("");
        case TAST_SYMBOL_TYPED:
            unreachable("");
        case TAST_FUNCTION_CALL:
            unreachable("");
        case TAST_OPERATOR:
            unreachable("");
    }
    unreachable("");
}

static inline Tast* get_tast_dest(Tast* tast) {
    switch (tast->type) {
        case TAST_DEF:
            unreachable("");
        case TAST_EXPR:
            return get_expr_dest(tast_unwrap_expr(tast));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
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
            unreachable(TAST_FMT"\n", tast_print(tast_wrap_expr_const(expr)));
    }
    unreachable("");
}

static inline Str_view get_tast_name(const Tast* tast) {
    switch (tast->type) {
        case TAST_DEF:
            return get_def_name(tast_unwrap_def_const(tast));
        case TAST_EXPR:
            return get_expr_name(tast_unwrap_expr_const(tast));
        case TAST_BLOCK:
            unreachable("");
        case TAST_FUNCTION_PARAMS:
            unreachable("");
        case TAST_LANG_TYPE:
            unreachable("");
        case TAST_RETURN:
            unreachable("");
        case TAST_FOR_RANGE:
            unreachable("");
        case TAST_FOR_WITH_COND:
            unreachable("");
        case TAST_FOR_LOWER_BOUND:
            unreachable("");
        case TAST_FOR_UPPER_BOUND:
            unreachable("");
        case TAST_BREAK:
            unreachable("");
        case TAST_IF:
            unreachable("");
        case TAST_CONDITION:
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

static inline const Tast* get_tast_src_const(const Tast* tast) {
    return get_tast_src((Tast*)tast);
}

static inline const Tast* get_tast_dest_const(const Tast* tast) {
    return get_tast_dest((Tast*)tast);
}

#endif // TAST_UTIL_H
