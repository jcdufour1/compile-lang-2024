#ifndef LLVM_UTIL_H
#define LLVM_UTIL_H

#include <llvm.h>
#include <tast_utils.h>
#include <lang_type.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type);

void extend_serialize_lang_type_to_string(
    Env* env,
    String* string,
    Lang_type lang_type,
    bool do_tag
);

void extend_lang_type_atom_to_string(
    String* string,
    Lang_type_atom atom,
    bool surround_in_lt_gt
);

Str_view lang_type_vec_print_internal(Lang_type_vec types);

#define lang_type_vec_print(types) str_view_print(lang_type_vec_print_internal((types), false))

#define LLVM_FMT STR_VIEW_FMT

#define llvm_printf(llvm) \
    do { \
        log(LOG_NOTE, LLVM_FMT"\n", llvm_print(llvm)); \
    } while (0);

static inline Lang_type llvm_operator_get_lang_type(const Llvm_operator* operator) {
    if (operator->type == LLVM_UNARY) {
        return llvm_unary_const_unwrap(operator)->lang_type;
    } else if (operator->type == LLVM_BINARY) {
        return llvm_binary_const_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type* llvm_get_operator_lang_type_ref(Llvm_operator* operator) {
    if (operator->type == LLVM_UNARY) {
        return &llvm_unary_unwrap(operator)->lang_type;
    } else if (operator->type == LLVM_BINARY) {
        return &llvm_binary_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Sym_typed_base* llvm_symbol_typed_get_base_ref(Llvm_symbol* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return &llvm_primitive_sym_unwrap(sym)->base;
        case LLVM_STRUCT_SYM:
            return &llvm_struct_sym_unwrap(sym)->base;
        case LLVM_ENUM_SYM:
            return &llvm_enum_sym_unwrap(sym)->base;
        case LLVM_RAW_UNION_SYM:
            return &llvm_raw_union_sym_unwrap(sym)->base;
    }
    unreachable("");
}

static inline Sym_typed_base llvm_symbol_typed_get_base_const(const Llvm_symbol* sym) {
    return *llvm_symbol_typed_get_base_ref((Llvm_symbol*)sym);
}

static inline Str_view llvm_symbol_typed_get_name(const Llvm_symbol* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_const_unwrap(sym)->base.name;
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_const_unwrap(sym)->base.name;
        case LLVM_ENUM_SYM:
            return llvm_enum_sym_const_unwrap(sym)->base.name;
        case LLVM_RAW_UNION_SYM:
            return llvm_raw_union_sym_const_unwrap(sym)->base.name;
    }
    unreachable("");
}

static inline Llvm_id llvm_get_llvm_id_expr(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_OPERATOR: {
            const Llvm_operator* operator = llvm_operator_const_unwrap(expr);
            if (operator->type == LLVM_UNARY) {
                return llvm_unary_const_unwrap(operator)->llvm_id;
            } else if (operator->type == LLVM_BINARY) {
                return llvm_binary_const_unwrap(operator)->llvm_id;
            } else {
                unreachable("");
            }
        }
        case LLVM_SYMBOL:
            unreachable("");
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->llvm_id;
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
            return llvm_variable_def_const_unwrap(def)->llvm_id;
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
            return llvm_get_llvm_id_expr(llvm_expr_const_unwrap(llvm));
        case LLVM_DEF:
            return llvm_get_llvm_id_def(llvm_def_const_unwrap(llvm));
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
            return llvm_alloca_const_unwrap(llvm)->llvm_id;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_const_unwrap(llvm)->llvm_id;
        case LLVM_STORE_ANOTHER_LLVM:
            unreachable("");
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_const_unwrap(llvm)->llvm_id;
        default:
            unreachable(""); // we cannot print llvm_type because it will cause infinite recursion
    }
}

static inline Llvm_id llvm_id_from_get_name(Env* env, Str_view llvm) {
    Llvm* result = NULL;
    try(alloca_lookup(&result, env, llvm));
    return llvm_get_llvm_id(result);
}

static inline Lang_type llvm_symbol_typed_get_lang_type(const Llvm_symbol* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_const_unwrap(sym)->base.lang_type;
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_const_unwrap(sym)->base.lang_type;
        case LLVM_ENUM_SYM:
            return llvm_enum_sym_const_unwrap(sym)->base.lang_type;
        case LLVM_RAW_UNION_SYM:
            return llvm_raw_union_sym_const_unwrap(sym)->base.lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_literal_get_lang_type(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_const_unwrap(lit)->lang_type;
        case LLVM_STRING:
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type_atom_new_from_cstr("u8", 1))));
        case LLVM_VOID:
            return lang_type_void_const_wrap(lang_type_void_new(0));
        case LLVM_ENUM_LIT:
            return llvm_enum_lit_const_unwrap(lit)->lang_type;
        case LLVM_CHAR:
            // TODO: remove lang_type_atom from lang_type_char and lang_type_string
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type_atom_new_from_cstr("u8", 0))));
    }
    unreachable("");
}

static inline Lang_type* llvm_literal_ref_get_lang_type(Llvm_literal* lit) {
    (void) lit;
    todo();
    //switch (lit->type) {
    //    case LLVM_NUMBER:
    //        return &llvm_number_unwrap(lit)->lang_type;
    //    case LLVM_STRING:
    //        return &llvm_string_unwrap(lit)->lang_type;
    //    case LLVM_VOID:
    //        unreachable("");
    //    case LLVM_ENUM_LIT:
    //        return &llvm_enum_lit_unwrap(lit)->lang_type;
    //    case LLVM_CHAR:
    //        return &llvm_char_unwrap(lit)->lang_type;
    //}
    unreachable("");
}

static inline Lang_type llvm_expr_get_lang_type(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->lang_type;
        case LLVM_LITERAL:
            return llvm_literal_get_lang_type(llvm_literal_const_unwrap(expr));
        case LLVM_OPERATOR:
            return llvm_operator_get_lang_type(llvm_operator_const_unwrap(expr));
        case LLVM_SYMBOL:
            return llvm_symbol_typed_get_base_const(llvm_symbol_const_unwrap(expr)).lang_type;
        case LLVM_LLVM_PLACEHOLDER:
            return llvm_llvm_placeholder_const_unwrap(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_def_get_lang_type(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_RAW_UNION_DEF:
            unreachable("");
        case LLVM_ENUM_DEF:
            return lang_type_enum_const_wrap(lang_type_enum_new(lang_type_atom_new(llvm_enum_def_const_unwrap(def)->base.name, 0)));
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_const_unwrap(def)->lang_type;
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_STRUCT_DEF:
            return lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(llvm_struct_def_const_unwrap(def)->base.name, 0)));
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* llvm_expr_ref_get_lang_type(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_FUNCTION_CALL:
            return &llvm_function_call_unwrap(expr)->lang_type;
        case LLVM_LITERAL:
            return llvm_literal_ref_get_lang_type(llvm_literal_unwrap(expr));
        case LLVM_SYMBOL:
            return &llvm_symbol_typed_get_base_ref(llvm_symbol_unwrap(expr))->lang_type;
        case LLVM_OPERATOR:
            return llvm_get_operator_lang_type_ref(llvm_operator_unwrap(expr));
        case LLVM_LLVM_PLACEHOLDER:
            return &llvm_llvm_placeholder_unwrap(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type llvm_get_lang_type(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_def_get_lang_type(llvm_def_const_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_get_lang_type(llvm_expr_const_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            return llvm_lang_type_const_unwrap(llvm)->lang_type;
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return llvm_alloca_const_unwrap(llvm)->lang_type;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_const_unwrap(llvm)->lang_type;
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_const_unwrap(llvm)->lang_type;
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_const_unwrap(llvm)->lang_type;
    }
    unreachable("");
}

static inline Lang_type* llvm_def_ref_get_lang_type(Llvm_def* def) {
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

static inline Lang_type* llvm_ref_get_lang_type(Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_def_ref_get_lang_type(llvm_def_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_ref_get_lang_type(llvm_expr_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            return &llvm_lang_type_unwrap(llvm)->lang_type;
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            unreachable("");
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return &llvm_alloca_unwrap(llvm)->lang_type;
        case LLVM_LOAD_ANOTHER_LLVM:
            return &llvm_load_another_llvm_unwrap(llvm)->lang_type;
        case LLVM_STORE_ANOTHER_LLVM:
            return &llvm_store_another_llvm_unwrap(llvm)->lang_type;
        case LLVM_LOAD_ELEMENT_PTR:
            return &llvm_load_element_ptr_unwrap(llvm)->lang_type;
    }
    unreachable("");
}

static inline Llvm* llvm_get_expr_src(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_SYMBOL:
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
            return llvm_get_expr_src(llvm_expr_unwrap(llvm));
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
            //return llvm_expr_wrap(llvm_operator_wrap(llvm_cond_goto_unwrap(llvm)->llvm_src.llvm));
        case LLVM_ALLOCA:
            unreachable("");
        case LLVM_LOAD_ANOTHER_LLVM:
            unreachable("");
        case LLVM_STORE_ANOTHER_LLVM:
            unreachable("");
        case LLVM_LOAD_ELEMENT_PTR:
            unreachable("");
    }
    unreachable("");
}

static inline Llvm* llvm_get_expr_dest(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_LITERAL:
            unreachable("");
        case LLVM_SYMBOL:
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
            return llvm_get_expr_dest(llvm_expr_unwrap(llvm));
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
            unreachable("");
        case LLVM_LOAD_ELEMENT_PTR:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view llvm_literal_get_name(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_const_unwrap(lit)->name;
        case LLVM_STRING:
            return llvm_string_const_unwrap(lit)->name;
        case LLVM_VOID:
            return llvm_void_const_unwrap(lit)->name;
        case LLVM_ENUM_LIT:
            return llvm_enum_lit_const_unwrap(lit)->name;
        case LLVM_CHAR:
            return llvm_char_const_unwrap(lit)->name;
    }
    unreachable("");
}

static inline Str_view llvm_operator_get_name(const Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_binary_const_unwrap(operator)->name;
        case LLVM_UNARY:
            return llvm_unary_const_unwrap(operator)->name;
    }
    unreachable("");
}

static inline Str_view llvm_expr_get_name(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_operator_get_name(llvm_operator_const_unwrap(expr));
        case LLVM_SYMBOL:
            return llvm_symbol_typed_get_name(llvm_symbol_const_unwrap(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->name_self;
        case LLVM_LITERAL:
            return llvm_literal_get_name(llvm_literal_const_unwrap(expr));
        case LLVM_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view llvm_literal_def_get_name(const Llvm_literal_def* lit_def) {
    switch (lit_def->type) {
        case LLVM_STRUCT_LIT_DEF:
            return llvm_struct_lit_def_const_unwrap(lit_def)->name;
        case LLVM_STRING_DEF:
            return llvm_string_def_const_unwrap(lit_def)->name;
    }
    unreachable("");
}

static inline Str_view llvm_def_get_name(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_PRIMITIVE_DEF:
            return lang_type_get_str(llvm_primitive_def_const_unwrap(def)->lang_type);
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_const_unwrap(def)->name_self;
        case LLVM_STRUCT_DEF:
            return llvm_struct_def_const_unwrap(def)->base.name;
        case LLVM_RAW_UNION_DEF:
            return llvm_raw_union_def_const_unwrap(def)->base.name;
        case LLVM_ENUM_DEF:
            return llvm_enum_def_const_unwrap(def)->base.name;
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_const_unwrap(def)->name;
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_const_unwrap(def)->decl->name;
        case LLVM_LABEL:
            return llvm_label_const_unwrap(def)->name;
        case LLVM_LITERAL_DEF:
            return llvm_literal_def_get_name(llvm_literal_def_const_unwrap(def));
    }
    unreachable("");
}

static inline Str_view llvm_llvm_expr_get_name(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_SYMBOL:
            return llvm_symbol_typed_get_name(llvm_symbol_const_unwrap(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->name_self;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(expr)));
    }
    unreachable("");
}

static inline Str_view llvm_tast_get_name(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_def_get_name(llvm_def_const_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_get_name(llvm_expr_const_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_LANG_TYPE:
            unreachable("");
        case LLVM_RETURN:
            unreachable("");
        case LLVM_GOTO:
            return llvm_goto_const_unwrap(llvm)->name;
        case LLVM_COND_GOTO:
            unreachable("");
        case LLVM_ALLOCA:
            return llvm_alloca_const_unwrap(llvm)->name;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_const_unwrap(llvm)->name;
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_const_unwrap(llvm)->name;
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_const_unwrap(llvm)->name_self;
    }
    unreachable("");
}

static inline const Llvm* get_llvm_src_const(const Llvm* llvm) {
    return get_llvm_src((Llvm*)llvm);
}

static inline const Llvm* get_llvm_dest_const(const Llvm* llvm) {
    return get_llvm_dest((Llvm*)llvm);
}

static inline Lang_type lang_type_from_get_name(Env* env, Str_view name) {
    Llvm* result = NULL;
    try(alloca_lookup(&result, env, name));
    return llvm_get_lang_type(result);
}

#endif // LLVM_UTIL_H
