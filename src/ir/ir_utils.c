#include <ir_utils.h>
#include <lang_type.h>
#include <ir_lang_type.h>
#include <lang_type_after.h>
#include <ir_lang_type_after.h>
#include <name.h>

Ir_lang_type ir_operator_get_lang_type(const Ir_operator* operator) {
    if (operator->type == IR_UNARY) {
        return ir_unary_const_unwrap(operator)->lang_type;
    } else if (operator->type == IR_BINARY) {
        return ir_binary_const_unwrap(operator)->lang_type;
    } else {
        unreachable("");
    }
}

Ir_lang_type ir_literal_get_lang_type(const Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            return ir_int_const_unwrap(lit)->lang_type;
        case IR_FLOAT:
            return ir_float_const_unwrap(lit)->lang_type;
        case IR_STRING:
            return ir_lang_type_primitive_const_wrap(ir_lang_type_unsigned_int_const_wrap(ir_lang_type_unsigned_int_new(ir_literal_get_pos(lit), 8, 1)));
        case IR_VOID:
            return ir_lang_type_void_const_wrap(ir_lang_type_void_new(ir_literal_get_pos(lit), 0));
        case IR_FUNCTION_NAME:
            return ir_lang_type_primitive_const_wrap(ir_lang_type_unsigned_int_const_wrap(ir_lang_type_unsigned_int_new(ir_literal_get_pos(lit), params.sizeof_usize, 1)));
    }
    unreachable("");
}

Ir_lang_type ir_expr_get_lang_type(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_FUNCTION_CALL:
            return ir_function_call_const_unwrap(expr)->lang_type;
        case IR_LITERAL:
            return ir_literal_get_lang_type(ir_literal_const_unwrap(expr));
        case IR_OPERATOR:
            return ir_operator_get_lang_type(ir_operator_const_unwrap(expr));
    }
    unreachable("");
}

Ir_lang_type ir_def_get_lang_type(const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            unreachable("");
        case IR_VARIABLE_DEF:
            return ir_variable_def_const_unwrap(def)->lang_type;
        case IR_FUNCTION_DECL:
            unreachable("");
        case IR_STRUCT_DEF: {
            const Ir_struct_def* struct_def = ir_struct_def_const_unwrap(def);
            return ir_lang_type_struct_const_wrap(ir_lang_type_struct_new(
                struct_def->pos,
                struct_def->base.name,
                0
            ));
        }
        case IR_PRIMITIVE_DEF:
            unreachable("");
        case IR_LABEL:
            unreachable("");
        case IR_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}

Ir_lang_type ir_get_lang_type(const Ir* ir) {
    switch (ir->type) {
        case IR_DEF:
            return ir_def_get_lang_type(ir_def_const_unwrap(ir));
        case IR_EXPR:
            return ir_expr_get_lang_type(ir_expr_const_unwrap(ir));
        case IR_BLOCK:
            unreachable("");
        case IR_FUNCTION_PARAMS:
            unreachable("");
        case IR_RETURN:
            unreachable("");
        case IR_GOTO:
            unreachable("");
        case IR_COND_GOTO:
            unreachable("");
        case IR_ALLOCA:
            return ir_alloca_const_unwrap(ir)->lang_type;
        case IR_LOAD_ANOTHER_IR:
            return ir_load_another_ir_const_unwrap(ir)->lang_type;
        case IR_STORE_ANOTHER_IR:
            return ir_store_another_ir_const_unwrap(ir)->lang_type;
        case IR_LOAD_ELEMENT_PTR:
            return ir_load_element_ptr_const_unwrap(ir)->lang_type;
        case IR_ARRAY_ACCESS:
            return ir_array_access_const_unwrap(ir)->lang_type;
        case IR_IMPORT_PATH:
            unreachable("");
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
        case IR_REMOVED:
            unreachable("");
    }
    unreachable("");
}

Ir_name ir_literal_get_name(const Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            return ir_int_const_unwrap(lit)->name;
        case IR_FLOAT:
            return ir_float_const_unwrap(lit)->name;
        case IR_STRING:
            return ir_string_const_unwrap(lit)->name;
        case IR_VOID:
            return ir_void_const_unwrap(lit)->name;
        case IR_FUNCTION_NAME:
            return ir_function_name_const_unwrap(lit)->name_self;
    }
    unreachable("");
}

Ir_name ir_operator_get_name(const Ir_operator* operator) {
    switch (operator->type) {
        case IR_BINARY:
            return ir_binary_const_unwrap(operator)->name;
        case IR_UNARY:
            return ir_unary_const_unwrap(operator)->name;
    }
    unreachable("");
}

Ir_name ir_expr_get_name(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            return ir_operator_get_name(ir_operator_const_unwrap(expr));
        case IR_FUNCTION_CALL:
            return ir_function_call_const_unwrap(expr)->name_self;
        case IR_LITERAL:
            return ir_literal_get_name(ir_literal_const_unwrap(expr));
    }
    unreachable("");
}

Ir_name ir_literal_def_get_name(const Ir_literal_def* lit_def) {
    switch (lit_def->type) {
        case IR_STRUCT_LIT_DEF:
            return ir_struct_lit_def_const_unwrap(lit_def)->name;
        case IR_STRING_DEF:
            return ir_string_def_const_unwrap(lit_def)->name;
    }
    unreachable("");
}

Ir_name ir_def_get_name(LANG_TYPE_MODE mode, const Ir_def* def) {
    switch (def->type) {
        case IR_PRIMITIVE_DEF: {
            Ir_name result = {0};
            unwrap(ir_lang_type_get_name(&result, mode, ir_primitive_def_const_unwrap(def)->lang_type));
            return result;
        }
        case IR_VARIABLE_DEF:
            return ir_variable_def_const_unwrap(def)->name_self;
        case IR_STRUCT_DEF:
            return ir_struct_def_const_unwrap(def)->base.name;
        case IR_FUNCTION_DECL:
            return ir_function_decl_const_unwrap(def)->name;
        case IR_FUNCTION_DEF:
            return ir_function_def_const_unwrap(def)->name_self;
        case IR_LABEL:
            return ir_label_const_unwrap(def)->name;
        case IR_LITERAL_DEF:
            return ir_literal_def_get_name(ir_literal_def_const_unwrap(def));
    }
    unreachable("");
}

Ir_name ir_get_name(LANG_TYPE_MODE mode, const Ir* ir) {
    switch (ir->type) {
        case IR_DEF:
            return ir_def_get_name(mode, ir_def_const_unwrap(ir));
        case IR_EXPR:
            return ir_expr_get_name(ir_expr_const_unwrap(ir));
        case IR_BLOCK:
            return ir_block_const_unwrap(ir)->name;
        case IR_FUNCTION_PARAMS:
            return ir_function_params_const_unwrap(ir)->name;
        case IR_RETURN:
            return ir_return_const_unwrap(ir)->name_self;
        case IR_GOTO:
            return ir_goto_const_unwrap(ir)->name_self;
        case IR_COND_GOTO:
            return ir_cond_goto_const_unwrap(ir)->name_self;
        case IR_ALLOCA:
            return ir_alloca_const_unwrap(ir)->name;
        case IR_LOAD_ANOTHER_IR:
            return ir_load_another_ir_const_unwrap(ir)->name;
        case IR_STORE_ANOTHER_IR:
            return ir_store_another_ir_const_unwrap(ir)->name;
        case IR_LOAD_ELEMENT_PTR:
            return ir_load_element_ptr_const_unwrap(ir)->name_self;
        case IR_ARRAY_ACCESS:
            return ir_array_access_const_unwrap(ir)->name_self;
        case IR_IMPORT_PATH:
            return name_to_ir_name(name_new(MOD_PATH_OF_MOD_PATHS, ir_import_path_const_unwrap(ir)->mod_path, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL));
        case IR_STRUCT_MEMB_DEF:
            return ir_struct_memb_def_const_unwrap(ir)->name_self;
        case IR_REMOVED:
            unreachable("");
    }
    unreachable("");
}

Ir_lang_type lang_type_from_ir_name(Ir_name name) {
    return ir_get_lang_type(ir_from_ir_name(name));
}

// TODO: use this instead of the verbose `ir_lookup` in more places?
Ir* ir_from_ir_name(Ir_name name) {
    Ir* result = NULL;
    unwrap(ir_lookup(&result,  name));
    return result;
}

size_t struct_def_get_idx_matching_member(Ir_struct_def* def, Ir_name memb_name) {
    for (size_t idx = 0; idx < def->base.members.info.count; idx++) {
        if (ir_name_is_equal(vec_at(def->base.members, idx)->name_self, memb_name)) {
            return idx;
        }
    }
    log(LOG_DEBUG, FMT"\n", ir_print(def));
    unreachable(FMT, ir_name_print(NAME_MSG, memb_name));
}

