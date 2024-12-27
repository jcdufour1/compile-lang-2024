#ifndef LLVM_UTIL_H
#define LLVM_UTIL_H

#include <llvm.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

static inline Lang_type llvm_lang_type_new_from_strv(Str_view str_view, int16_t pointer_depth) {
    Lang_type Lang_type = {.str = str_view, .pointer_depth = pointer_depth};
    assert(str_view.count < 1e9);
    return Lang_type;
}

// only literals can be used here
static inline Lang_type llvm_lang_type_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return lang_type_new_from_strv(str_view_from_cstr(cstr), pointer_depth);
}

#define lang_type_print(lang_type) str_view_print(lang_type_print_internal(&print_arena, (lang_type), false))

#define LLVM_FMT STR_VIEW_FMT

#define llvm_printf(llvm) \
    do { \
        log(LOG_NOTE, LLVM_FMT"\n", llvm_print(llvm)); \
    } while (0);

static inline Lang_type llvm_get_lang_type_operator(const Llvm_operator* operator) {
    if (operator->type == LLVM_UNARY) {
        return llvm_unwrap_unary_const(operator)->lang_type;
    } else if (operator->type == LLVM_BINARY) {
        return llvm_unwrap_binary_const(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type* llvm_get_operator_lang_type_ref(Llvm_operator* operator) {
    if (operator->type == LLVM_UNARY) {
        return &llvm_unwrap_unary(operator)->lang_type;
    } else if (operator->type == LLVM_BINARY) {
        return &llvm_unwrap_binary(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Sym_typed_base* llvm_symbol_typed_get_base_ref(Llvm_symbol_typed* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return &llvm_unwrap_primitive_sym(sym)->base;
        case LLVM_STRUCT_SYM:
            return &llvm_unwrap_struct_sym(sym)->base;
        case LLVM_ENUM_SYM:
            return &llvm_unwrap_enum_sym(sym)->base;
        case LLVM_RAW_UNION_SYM:
            return &llvm_unwrap_raw_union_sym(sym)->base;
    }
    unreachable("");
}

static inline Sym_typed_base llvm_symbol_typed_get_base_const(const Llvm_symbol_typed* sym) {
    return *llvm_symbol_typed_get_base_ref((Llvm_symbol_typed*)sym);
}

static inline Str_view llvm_get_symbol_typed_name(const Llvm_symbol_typed* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_unwrap_primitive_sym_const(sym)->base.name;
        case LLVM_STRUCT_SYM:
            return llvm_unwrap_struct_sym_const(sym)->base.name;
        case LLVM_ENUM_SYM:
            return llvm_unwrap_enum_sym_const(sym)->base.name;
        case LLVM_RAW_UNION_SYM:
            return llvm_unwrap_raw_union_sym_const(sym)->base.name;
    }
    unreachable("");
}

static inline Llvm_id llvm_get_llvm_id_expr(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_MEMBER_ACCESS_TYPED:
            unreachable("");
        case LLVM_INDEX_TYPED:
            unreachable("");
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_OPERATOR: {
            const Llvm_operator* operator = llvm_unwrap_operator_const(expr);
            if (operator->type == LLVM_UNARY) {
                return llvm_unwrap_unary_const(operator)->llvm_id;
            } else if (operator->type == LLVM_BINARY) {
                return llvm_unwrap_binary_const(operator)->llvm_id;
            } else {
                unreachable("");
            }
        }
        case LLVM_SYMBOL_TYPED:
            unreachable("");
        case LLVM_FUNCTION_CALL:
            return llvm_unwrap_function_call_const(expr)->llvm_id;
        case LLVM_STRUCT_LITERAL:
            unreachable("");
        case LLVM_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm_id llvm_get_llvm_id_def(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_VARIABLE_DEF:
            return llvm_unwrap_variable_def_const(def)->llvm_id;
        case LLVM_STRUCT_DEF:
            unreachable("");
        case LLVM_RAW_UNION_DEF:
            unreachable("");
        case LLVM_ENUM_DEF:
            unreachable("");
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm_id get_llvm_id_def(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_VARIABLE_DEF:
            return llvm_unwrap_variable_def_const(def)->llvm_id;
        case LLVM_STRUCT_DEF:
            unreachable("");
        case LLVM_RAW_UNION_DEF:
            unreachable("");
        case LLVM_ENUM_DEF:
            unreachable("");
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm_id llvm_get_llvm_id(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_EXPR:
            return llvm_get_llvm_id_expr(llvm_unwrap_expr_const(llvm));
        case LLVM_DEF:
            return llvm_get_llvm_id_def(llvm_unwrap_def_const(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return llvm_unwrap_alloca_const(llvm)->llvm_id;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_unwrap_load_another_llvm_const(llvm)->llvm_id;
        case LLVM_STORE_ANOTHER_LLVM:
            unreachable("");
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_unwrap_load_element_ptr_const(llvm)->llvm_id;
        default:
            unreachable(""); // we cannot print llvm_type because it will cause infinite recursion
    }
}

static inline Lang_type llvm_get_lang_type_symbol_typed(const Llvm_symbol_typed* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_unwrap_primitive_sym_const(sym)->base.lang_type;
        case LLVM_STRUCT_SYM:
            return llvm_unwrap_struct_sym_const(sym)->base.lang_type;
        case LLVM_ENUM_SYM:
            return llvm_unwrap_enum_sym_const(sym)->base.lang_type;
        case LLVM_RAW_UNION_SYM:
            return llvm_unwrap_raw_union_sym_const(sym)->base.lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_get_lang_type_literal(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_unwrap_number_const(lit)->lang_type;
        case LLVM_STRING:
            return llvm_unwrap_string_const(lit)->lang_type;
        case LLVM_VOID:
            return lang_type_new_from_cstr("void", 0);
        case LLVM_ENUM_LIT:
            return llvm_unwrap_enum_lit_const(lit)->lang_type;
        case LLVM_CHAR:
            return llvm_unwrap_char_const(lit)->lang_type;
    }
    unreachable("");
}

static inline Lang_type* llvm_get_lang_type_literal_ref(Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return &llvm_unwrap_number(lit)->lang_type;
        case LLVM_STRING:
            return &llvm_unwrap_string(lit)->lang_type;
        case LLVM_VOID:
            unreachable("");
        case LLVM_ENUM_LIT:
            return &llvm_unwrap_enum_lit(lit)->lang_type;
        case LLVM_CHAR:
            return &llvm_unwrap_char(lit)->lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_get_lang_type_expr(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_STRUCT_LITERAL:
            return llvm_unwrap_struct_literal_const(expr)->lang_type;
        case LLVM_FUNCTION_CALL:
            return llvm_unwrap_function_call_const(expr)->lang_type;
        case LLVM_MEMBER_ACCESS_TYPED:
            return llvm_unwrap_member_access_typed_const(expr)->lang_type;
        case LLVM_INDEX_TYPED:
            return llvm_unwrap_index_typed_const(expr)->lang_type;
        case LLVM_LITERAL:
            return llvm_get_lang_type_literal(llvm_unwrap_literal_const(expr));
        case LLVM_OPERATOR:
            return llvm_get_lang_type_operator(llvm_unwrap_operator_const(expr));
        case LLVM_SYMBOL_TYPED:
            return llvm_symbol_typed_get_base_const(llvm_unwrap_symbol_typed_const(expr)).lang_type;
        case LLVM_LLVM_PLACEHOLDER:
            return llvm_unwrap_llvm_placeholder_const(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_get_lang_type_def(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_RAW_UNION_DEF:
            unreachable("");
        case LLVM_ENUM_DEF:
            return lang_type_new_from_strv(llvm_unwrap_enum_def_const(def)->base.name, 0);
        case LLVM_VARIABLE_DEF:
            return llvm_unwrap_variable_def_const(def)->lang_type;
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_STRUCT_DEF:
            return lang_type_new_from_strv(llvm_unwrap_struct_def_const(def)->base.name, 0);
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* llvm_get_lang_type_expr_ref(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_STRUCT_LITERAL:
            return &llvm_unwrap_struct_literal(expr)->lang_type;
        case LLVM_FUNCTION_CALL:
            return &llvm_unwrap_function_call(expr)->lang_type;
        case LLVM_MEMBER_ACCESS_TYPED:
            unreachable("");
        case LLVM_INDEX_TYPED:
            unreachable("");
        case LLVM_LITERAL:
            return llvm_get_lang_type_literal_ref(llvm_unwrap_literal(expr));
        case LLVM_SYMBOL_TYPED:
            return &llvm_symbol_typed_get_base_ref(llvm_unwrap_symbol_typed(expr))->lang_type;
        case LLVM_OPERATOR:
            return llvm_get_operator_lang_type_ref(llvm_unwrap_operator(expr));
        case LLVM_LLVM_PLACEHOLDER:
            return &llvm_unwrap_llvm_placeholder(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_get_lang_type(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_get_lang_type_def(llvm_unwrap_def_const(llvm));
        case LLVM_EXPR:
            return llvm_get_lang_type_expr(llvm_unwrap_expr_const(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            return llvm_unwrap_lang_type_const(llvm)->lang_type;
        case LLVM_RETURN:
            return llvm_get_lang_type_expr(llvm_unwrap_return_const(llvm)->child);
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return llvm_unwrap_alloca_const(llvm)->lang_type;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_unwrap_load_another_llvm_const(llvm)->lang_type;
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_unwrap_store_another_llvm_const(llvm)->lang_type;
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_unwrap_load_element_ptr_const(llvm)->lang_type;
    }
    unreachable("");
}

static inline Lang_type* llvm_get_lang_type_def_ref(Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_RAW_UNION_DEF:
            unreachable("");
        case LLVM_ENUM_DEF:
            unreachable("");
        case LLVM_VARIABLE_DEF:
            unreachable("");
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_STRUCT_DEF:
            unreachable("");
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* llvm_get_lang_type_ref(Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_get_lang_type_def_ref(llvm_unwrap_def(llvm));
        case LLVM_EXPR:
            return llvm_get_lang_type_expr_ref(llvm_unwrap_expr(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            return &llvm_unwrap_lang_type(llvm)->lang_type;
        case LLVM_RETURN:
            return llvm_get_lang_type_expr_ref(llvm_unwrap_return(llvm)->child);
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return &llvm_unwrap_alloca(llvm)->lang_type;
        case LLVM_LOAD_ANOTHER_LLVM:
            return &llvm_unwrap_load_another_llvm(llvm)->lang_type;
        case LLVM_STORE_ANOTHER_LLVM:
            return &llvm_unwrap_store_another_llvm(llvm)->lang_type;
        case LLVM_LOAD_ELEMENT_PTR:
            return &llvm_unwrap_load_element_ptr(llvm)->lang_type;
    }
    unreachable("");
}

static inline Llvm* llvm_get_expr_src(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_STRUCT_LITERAL:
            unreachable("");
        case LLVM_MEMBER_ACCESS_TYPED:
            unreachable("");
        case LLVM_INDEX_TYPED:
            unreachable("");
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_SYMBOL_TYPED:
            unreachable("");
        case LLVM_FUNCTION_CALL:
            unreachable("");
        case LLVM_OPERATOR:
            unreachable("");
        case LLVM_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm* get_llvm_src(Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            unreachable("");
        case LLVM_EXPR:
            return llvm_get_expr_src(llvm_unwrap_expr(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
            //return llvm_wrap_expr(llvm_wrap_operator(llvm_unwrap_cond_goto(llvm)->llvm_src.llvm));
        case LLVM_ALLOCA:
            unreachable("");
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_unwrap_load_another_llvm(llvm)->llvm_src.llvm;
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_unwrap_store_another_llvm(llvm)->llvm_src.llvm;
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_unwrap_load_element_ptr(llvm)->llvm_src.llvm;
    }
    unreachable("");
}

static inline Llvm* llvm_get_expr_dest(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_STRUCT_LITERAL:
            unreachable("");
        case LLVM_MEMBER_ACCESS_TYPED:
            unreachable("");
        case LLVM_INDEX_TYPED:
            unreachable("");
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_SYMBOL_TYPED:
            unreachable("");
        case LLVM_FUNCTION_CALL:
            unreachable("");
        case LLVM_OPERATOR:
            unreachable("");
        case LLVM_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm* get_llvm_dest(Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            unreachable("");
        case LLVM_EXPR:
            return llvm_get_expr_dest(llvm_unwrap_expr(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            unreachable("");
        case LLVM_LOAD_ANOTHER_LLVM:
            unreachable("");
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_unwrap_store_another_llvm_const(llvm)->llvm_dest.llvm;
        case LLVM_LOAD_ELEMENT_PTR:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view llvm_get_literal_name(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            unreachable("");
        case LLVM_STRING:
            return llvm_unwrap_string_const(lit)->name;
        case LLVM_VOID:
            unreachable("");
        case LLVM_ENUM_LIT:
            unreachable("");
        case LLVM_CHAR:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view llvm_get_operator_name(const Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_unwrap_binary_const(operator)->name;
        case LLVM_UNARY:
            return llvm_unwrap_unary_const(operator)->name;
    }
    unreachable("");
}

static inline Str_view llvm_get_expr_name(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_get_operator_name(llvm_unwrap_operator_const(expr));
        case LLVM_STRUCT_LITERAL:
            return llvm_unwrap_struct_literal_const(expr)->name;
        case LLVM_MEMBER_ACCESS_TYPED:
            return llvm_unwrap_member_access_typed_const(expr)->member_name;
        case LLVM_INDEX_TYPED:
            unreachable("");
        case LLVM_SYMBOL_TYPED:
            return llvm_get_symbol_typed_name(llvm_unwrap_symbol_typed_const(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_unwrap_function_call_const(expr)->name;
        case LLVM_LITERAL:
            return llvm_get_literal_name(llvm_unwrap_literal_const(expr));
        case LLVM_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view llvm_get_literal_def_name(const Llvm_literal_def* lit_def) {
    switch (lit_def->type) {
        case LLVM_STRUCT_LIT_DEF:
            return llvm_unwrap_struct_lit_def_const(lit_def)->name;
        case LLVM_STRING_DEF:
            return llvm_unwrap_string_def_const(lit_def)->name;
    }
    unreachable("");
}

static inline Str_view llvm_get_def_name(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_PRIMITIVE_DEF:
            return llvm_unwrap_primitive_def_const(def)->lang_type.str;
        case LLVM_VARIABLE_DEF:
            return llvm_unwrap_variable_def_const(def)->name;
        case LLVM_STRUCT_DEF:
            return llvm_unwrap_struct_def_const(def)->base.name;
        case LLVM_RAW_UNION_DEF:
            return llvm_unwrap_raw_union_def_const(def)->base.name;
        case LLVM_ENUM_DEF:
            return llvm_unwrap_enum_def_const(def)->base.name;
        case LLVM_FUNCTION_DECL:
            return llvm_unwrap_function_decl_const(def)->name;
        case LLVM_FUNCTION_DEF:
            return llvm_unwrap_function_def_const(def)->decl->name;
        case LLVM_LABEL:
            return llvm_unwrap_label_const(def)->name;
        case LLVM_LITERAL_DEF:
            return llvm_get_literal_def_name(llvm_unwrap_literal_def_const(def));
    }
    unreachable("");
}

static inline Str_view llvm_get_llvm_name_expr(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_SYMBOL_TYPED:
            return llvm_get_symbol_typed_name(llvm_unwrap_symbol_typed_const(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_unwrap_function_call_const(expr)->name;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(llvm_wrap_expr_const(expr)));
    }
    unreachable("");
}

static inline Str_view llvm_get_node_name(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_get_def_name(llvm_unwrap_def_const(llvm));
        case LLVM_EXPR:
            return llvm_get_expr_name(llvm_unwrap_expr_const(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            return llvm_unwrap_goto_const(llvm)->name;
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return llvm_unwrap_alloca_const(llvm)->name;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_unwrap_load_another_llvm_const(llvm)->name;
        case LLVM_STORE_ANOTHER_LLVM:
            unreachable("");
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_unwrap_load_element_ptr_const(llvm)->name;
    }
    unreachable("");
}

static inline const Llvm* get_llvm_src_const(const Llvm* llvm) {
    return get_llvm_src((Llvm*)llvm);
}

static inline const Llvm* get_llvm_dest_const(const Llvm* llvm) {
    return get_llvm_dest((Llvm*)llvm);
}

#endif // LLVM_UTIL_H
