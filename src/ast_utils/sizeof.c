#include <sizeof.h>
#include <lang_type.h>
#include <lang_type_after.h>
#include <ir_lang_type_after.h>
#include <tast_utils.h>
#include <ir_utils.h>
#include <tast.h>
#include <ir.h>
#include <lang_type_print.h>
#include <ir_lang_type_print.h>

static uint64_t bit_width_to_bytes(uint64_t bit_width) {
    return (bit_width + 7)/8;
}

uint64_t sizeof_primitive(Lang_type_primitive primitive) {
    if (lang_type_primitive_get_pointer_depth(primitive) > 0) {
        return params.sizeof_ptr_non_fn;
    }

    switch (primitive.type) {
        case LANG_TYPE_SIGNED_INT:
            return bit_width_to_bytes(lang_type_signed_int_const_unwrap(primitive).bit_width);
        case LANG_TYPE_UNSIGNED_INT:
            return bit_width_to_bytes(lang_type_unsigned_int_const_unwrap(primitive).bit_width);
        case LANG_TYPE_FLOAT:
            return bit_width_to_bytes(lang_type_float_const_unwrap(primitive).bit_width);
        case LANG_TYPE_OPAQUE:
            unreachable("");
    }
    unreachable("");
}

uint64_t sizeof_llvm_primitive(Ir_lang_type_primitive primitive) {
    // TODO: platform specific pointer size, etc.
    if (ir_lang_type_primitive_get_pointer_depth(primitive) > 0) {
        return params.sizeof_ptr_non_fn;
    }

    switch (primitive.type) {
        case IR_LANG_TYPE_SIGNED_INT:
            return bit_width_to_bytes(ir_lang_type_signed_int_const_unwrap(primitive).bit_width);
        case IR_LANG_TYPE_UNSIGNED_INT:
            return bit_width_to_bytes(ir_lang_type_unsigned_int_const_unwrap(primitive).bit_width);
        case IR_LANG_TYPE_FLOAT:
            return bit_width_to_bytes(ir_lang_type_float_const_unwrap(primitive).bit_width);
        case IR_LANG_TYPE_OPAQUE:
            unreachable("");
    }
    unreachable("");
}

uint64_t sizeof_lang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return sizeof_primitive(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT:
            fallthrough;
        case LANG_TYPE_ENUM:
            fallthrough;
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            Name name = {0};
            if (!lang_type_get_name(&name, lang_type)) {
                msg_todo("", lang_type_get_pos(lang_type));
                return 0;
            }
            unwrap(symbol_lookup(&def, name));
            return sizeof_def(def);
        }
        case LANG_TYPE_ARRAY: {
            Tast_def* def = NULL;
            Name name = {0};
            if (!lang_type_get_name(&name, lang_type)) {
                msg_todo("", tast_def_get_pos(def));
                return 0;
            }
            unwrap(symbol_lookup(&def, name));
            return sizeof_def(def);
        }
        case LANG_TYPE_VOID:
            return 0;
        case LANG_TYPE_LIT:
            // TODO
            return 0;
        case LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case LANG_TYPE_FN:
            return params.sizeof_ptr_non_fn; // TODO: make separate params member "sizeof_ptr_fn",
                                             //   and use it here
        case LANG_TYPE_REMOVED:
            unreachable("");
    }
    unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t alignof_lang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            // TODO: this may not work correctly with big ints if they use Lang_type_primitive
            return sizeof_primitive(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT:
            fallthrough;
        case LANG_TYPE_ENUM:
            fallthrough;
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            Name name = {0};
            if (!lang_type_get_name(&name, lang_type)) {
                msg_todo("", tast_def_get_pos(def));
                return 0;
            }
            unwrap(symbol_lookup(&def, name));
            return alignof_def(def);
        }
        case LANG_TYPE_ARRAY: {
            Tast_def* def = NULL;
            Name name = {0};
            if (!lang_type_get_name(&name, lang_type)) {
                msg_todo("", tast_def_get_pos(def));
                return 0;
            }
            unwrap(symbol_lookup(&def, name));
            return alignof_def(def);
        }
        case LANG_TYPE_VOID:
            return 0;
        case LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case LANG_TYPE_FN:
            msg_todo("", lang_type_get_pos(lang_type));
            return 0;
        case LANG_TYPE_LIT:
            msg_todo("", lang_type_get_pos(lang_type));
            return 0;
        case LANG_TYPE_REMOVED:
            unreachable("");
    }
    unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t sizeof_ir_lang_type(Ir_lang_type lang_type) {
    switch (lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            return sizeof_llvm_primitive(ir_lang_type_primitive_const_unwrap(lang_type));
        case IR_LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            Ir_name name = {0};
            unwrap(ir_lang_type_get_name(&name, LANG_TYPE_MODE_LOG, lang_type));
            unwrap(symbol_lookup(&def, ir_name_to_name(name)));
            return sizeof_def(def);
        }
        case IR_LANG_TYPE_VOID:
            return 0;
        case IR_LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case IR_LANG_TYPE_FN:
            // TODO
            todo();
    }
    unreachable(FMT, ir_lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t sizeof_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_ARRAY_DEF:
            return sizeof_array_def(tast_array_def_const_unwrap(def));
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return sizeof_struct_def_base(&tast_struct_def_const_unwrap(def)->base, false);
        case TAST_ENUM_DEF:
            return sizeof_struct_def_base(&tast_enum_def_const_unwrap(def)->base, true);
        case TAST_RAW_UNION_DEF:
            return sizeof_struct_def_base(&tast_raw_union_def_const_unwrap(def)->base, true);
        case TAST_LABEL:
            fallthrough;
        case TAST_IMPORT_PATH:
            fallthrough;
        case TAST_FUNCTION_DEF:
            fallthrough;
        case TAST_PRIMITIVE_DEF:
            fallthrough;
        case TAST_FUNCTION_DECL:
            msg_todo("", tast_def_get_pos(def));
            return 0;
    }
    unreachable("");
}

uint64_t alignof_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_ARRAY_DEF:
            return alignof_array_def(tast_array_def_const_unwrap(def));
        case TAST_VARIABLE_DEF:
            return alignof_lang_type(tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return alignof_struct_def_base(&tast_struct_def_const_unwrap(def)->base);
        case TAST_ENUM_DEF:
            return alignof_struct_def_base(&tast_enum_def_const_unwrap(def)->base);
        case TAST_RAW_UNION_DEF:
            return alignof_struct_def_base(&tast_raw_union_def_const_unwrap(def)->base);
        case TAST_LABEL:
            fallthrough;
        case TAST_IMPORT_PATH:
            fallthrough;
        case TAST_FUNCTION_DEF:
            fallthrough;
        case TAST_PRIMITIVE_DEF:
            fallthrough;
        case TAST_FUNCTION_DECL:
            msg_todo("", tast_def_get_pos(def));
            return 0;
    }
    unreachable("");
}

uint64_t sizeof_struct_literal(const Tast_struct_literal* lit) {
    Tast_def* def_ = NULL;
    Name name = {0};
    if (!lang_type_get_name(&name, lit->lang_type)) {
        msg_todo("", lit->pos);
        return 0;
    }
    unwrap(symbol_lookup(&def_, name));
    return sizeof_struct_def_base(&tast_struct_def_unwrap(def_)->base, false);
}

uint64_t sizeof_array_def(const Tast_array_def* def) {
    uint64_t end_alignment = alignof_lang_type(def->item_type);
    uint64_t total = sizeof_lang_type(def->item_type)*def->count;

    // TODO: use get_next_multiple function or similar function
    total += (end_alignment - total%end_alignment)%end_alignment;
    return total;
}

uint64_t sizeof_struct_def_base(const Struct_def_base* base, bool is_sum_type) {
    uint64_t end_alignment = 0;

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(memb_def->lang_type);
        end_alignment = max(end_alignment, alignof_lang_type(memb_def->lang_type));
        if (is_sum_type) {
            total = max(total, sizeof_curr_item);
        } else {
            total += sizeof_curr_item;
        }
    }

    // TODO: use get_next_multiple function or similar function
    total += (end_alignment - total%end_alignment)%end_alignment;
    return total;
}

uint64_t alignof_struct_def_base(const Struct_def_base* base) {
    uint64_t max_align = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        max_align = max(max_align, alignof_lang_type(vec_at(base->members, idx)->lang_type));
    }
    return max_align;
}

uint64_t alignof_array_def(const Tast_array_def* def) {
    return alignof_lang_type(def->item_type);
}

static uint64_t ir_sizeof_expr(const Ir_expr* expr) {
    (void) env;
    switch (expr->type) {
        case IR_LITERAL:
            return sizeof_ir_lang_type(ir_literal_get_lang_type(ir_literal_const_unwrap(expr)));
        case IR_OPERATOR:
            fallthrough;
        case IR_FUNCTION_CALL:
            msg_todo("", ir_expr_get_pos(expr));
            return 0;
    }
    unreachable("");
}

static uint64_t ir_sizeof_def(const Ir_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_ir_lang_type(ir_variable_def_const_unwrap(def)->lang_type);
        case IR_FUNCTION_DEF:
            fallthrough;
        case IR_VARIABLE_DEF:
            fallthrough;
        case IR_STRUCT_DEF:
            fallthrough;
        case IR_FUNCTION_DECL:
            fallthrough;
        case IR_LABEL:
            fallthrough;
        case IR_LITERAL_DEF:
            msg_todo("", ir_def_get_pos(def));
            return 0;
    }
    unreachable("");
}

uint64_t ir_sizeof_item(const Ir* item) {
    (void) env;
    switch (item->type) {
        case TAST_EXPR:
            return ir_sizeof_expr(ir_expr_const_unwrap(item));
        case TAST_DEF:
            return ir_sizeof_def(ir_def_const_unwrap(item));
        case IR_BLOCK:
            fallthrough;
        case IR_LOAD_ELEMENT_PTR:
            fallthrough;
        case IR_ARRAY_ACCESS:
            fallthrough;
        case IR_FUNCTION_PARAMS:
            fallthrough;
        case IR_DEF:
            fallthrough;
        case IR_RETURN:
            fallthrough;
        case IR_COND_GOTO:
            fallthrough;
        case IR_ALLOCA:
            fallthrough;
        case IR_LOAD_ANOTHER_IR:
            fallthrough;
        case IR_STORE_ANOTHER_IR:
            fallthrough;
        case IR_IMPORT_PATH:
            fallthrough;
        case IR_STRUCT_MEMB_DEF:
            fallthrough;
        case IR_REMOVED:
            msg_todo("", ir_get_pos(item));
            return 0;
    }
    unreachable("");
}

// TODO: update or remove this function
// TODO: if ir_sizeof* functions are kept, make a single set of unit tests that work for both so that
//   differences between tast_sizeof* and ir_sizeof* can be caught
uint64_t ir_sizeof_struct_def_base(const Struct_def_base* base) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        const Tast_variable_def* memb_def = vec_at(base->members, idx);
        uint64_t sizeof_curr_item = sizeof_lang_type(memb_def->lang_type);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

