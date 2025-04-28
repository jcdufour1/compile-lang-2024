#include <llvm_utils.h>
#include <lang_type.h>
#include <lang_type_after.h>

Lang_type llvm_operator_get_lang_type(const Llvm_operator* operator) {
    if (operator->type == LLVM_UNARY) {
        return llvm_unary_const_unwrap(operator)->lang_type;
    } else if (operator->type == LLVM_BINARY) {
        return llvm_binary_const_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

Llvm_id llvm_get_llvm_id_expr(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_PRIMITIVE_SYM:
            unreachable("");
        case LLVM_STRUCT_SYM:
            unreachable("");
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
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->llvm_id;
    }
    unreachable("");
}

Llvm_id llvm_get_llvm_id_def(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_const_unwrap(def)->llvm_id;
        case LLVM_STRUCT_DEF:
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

Llvm_id llvm_get_llvm_id(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_EXPR:
            return llvm_get_llvm_id_expr(llvm_expr_const_unwrap(llvm));
        case LLVM_DEF:
            return llvm_get_llvm_id_def(llvm_def_const_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
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

Llvm_id llvm_id_from_get_name(Name llvm) {
    Llvm* result = NULL;
    unwrap(alloca_lookup(&result,  llvm));
    return llvm_get_llvm_id(result);
}

Lang_type llvm_literal_get_lang_type(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_const_unwrap(lit)->lang_type;
        case LLVM_STRING:
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(llvm_literal_get_pos(lit), lang_type_atom_new_from_cstr("u8", 1))));
        case LLVM_VOID:
            return lang_type_void_const_wrap(lang_type_void_new(llvm_literal_get_pos(lit)));
        case LLVM_FUNCTION_NAME:
            // TODO: remove lang_type_atom from lang_type_char and lang_type_string
            return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(llvm_literal_get_pos(lit), lang_type_atom_new_from_cstr("ptr", 1))));
    }
    unreachable("");
}

Lang_type llvm_expr_get_lang_type(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->lang_type;
        case LLVM_LITERAL:
            return llvm_literal_get_lang_type(llvm_literal_const_unwrap(expr));
        case LLVM_OPERATOR:
            return llvm_operator_get_lang_type(llvm_operator_const_unwrap(expr));
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_const_unwrap(expr)->base.lang_type;
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_const_unwrap(expr)->base.lang_type;
    }
    unreachable("");
}

Lang_type llvm_def_get_lang_type(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            unreachable("");
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_const_unwrap(def)->lang_type;
        case LLVM_FUNCTION_DECL:
            unreachable("");
        case LLVM_STRUCT_DEF:
            return lang_type_struct_const_wrap(lang_type_struct_new(llvm_def_get_pos(def), lang_type_atom_new(llvm_struct_def_const_unwrap(def)->base.name, 0)));
        case LLVM_PRIMITIVE_DEF:
            unreachable("");
        case LLVM_LABEL:
            unreachable("");
        case LLVM_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

Lang_type llvm_get_lang_type(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_def_get_lang_type(llvm_def_const_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_get_lang_type(llvm_expr_const_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
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

Name llvm_literal_get_name(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_const_unwrap(lit)->name;
        case LLVM_STRING:
            return llvm_string_const_unwrap(lit)->name;
        case LLVM_VOID:
            return llvm_void_const_unwrap(lit)->name;
        case LLVM_FUNCTION_NAME:
            return llvm_function_name_const_unwrap(lit)->name_self;
    }
    unreachable("");
}

Name llvm_operator_get_name(const Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_binary_const_unwrap(operator)->name;
        case LLVM_UNARY:
            return llvm_unary_const_unwrap(operator)->name;
    }
    unreachable("");
}

Name llvm_expr_get_name(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_operator_get_name(llvm_operator_const_unwrap(expr));
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_const_unwrap(expr)->base.name;
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_const_unwrap(expr)->base.name;
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_const_unwrap(expr)->name_self;
        case LLVM_LITERAL:
            return llvm_literal_get_name(llvm_literal_const_unwrap(expr));
    }
    unreachable("");
}

Name llvm_literal_def_get_name(const Llvm_literal_def* lit_def) {
    switch (lit_def->type) {
        case LLVM_STRUCT_LIT_DEF:
            return llvm_struct_lit_def_const_unwrap(lit_def)->name;
        case LLVM_STRING_DEF:
            return llvm_string_def_const_unwrap(lit_def)->name;
    }
    unreachable("");
}

Name llvm_def_get_name(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_PRIMITIVE_DEF:
            return lang_type_get_str(llvm_primitive_def_const_unwrap(def)->lang_type);
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_const_unwrap(def)->name_self;
        case LLVM_STRUCT_DEF:
            return llvm_struct_def_const_unwrap(def)->base.name;
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

Name llvm_tast_get_name(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            return llvm_def_get_name(llvm_def_const_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_get_name(llvm_expr_const_unwrap(llvm));
        case LLVM_BLOCK:
            unreachable("");
        case LLVM_FUNCTION_PARAMS:
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

// TODO: rename this function
Lang_type lang_type_from_get_name(Name name) {
    Llvm* result = NULL;
    unwrap(alloca_lookup(&result,  name));
    return llvm_get_lang_type(result);
}
