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
    if (lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, primitive) > 0) {
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
    if (ir_lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE_LOG, primitive) > 0) {
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
        case LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case LANG_TYPE_ENUM: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case LANG_TYPE_ARRAY: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case LANG_TYPE_VOID:
            return 0;
        case LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case LANG_TYPE_FN:
            todo();
    }
    unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t alignof_lang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            // TODO: this may not work correctly with big ints if they use Lang_type_primitive
            return sizeof_primitive(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return alignof_def(def);
        }
        case LANG_TYPE_ENUM: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return alignof_def(def);
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return alignof_def(def);
        }
        case LANG_TYPE_ARRAY: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return alignof_def(def);
        }
        case LANG_TYPE_VOID:
            return 0;
        case LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case LANG_TYPE_FN:
            todo();
    }
    unreachable(FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t sizeof_ir_lang_type(Ir_lang_type lang_type) {
    switch (lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            return sizeof_llvm_primitive(ir_lang_type_primitive_const_unwrap(lang_type));
        case IR_LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            unwrap(symbol_lookup(&def, ir_lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
            return sizeof_def(def);
        }
        case IR_LANG_TYPE_VOID:
            return 0;
        case IR_LANG_TYPE_TUPLE:
            unreachable("tuple should not be here");
        case IR_LANG_TYPE_FN:
            todo();
    }
    unreachable(FMT, ir_lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
}

uint64_t sizeof_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_lang_type(tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return sizeof_struct_def_base(&tast_struct_def_const_unwrap(def)->base, false);
        case TAST_ENUM_DEF:
            return sizeof_struct_def_base(&tast_enum_def_const_unwrap(def)->base, true);
        case TAST_RAW_UNION_DEF:
            return sizeof_struct_def_base(&tast_raw_union_def_const_unwrap(def)->base, true);
        default:
            unreachable("");
    }
}

uint64_t alignof_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return alignof_lang_type(tast_variable_def_const_unwrap(def)->lang_type);
        case TAST_STRUCT_DEF:
            return alignof_struct_def_base(&tast_struct_def_const_unwrap(def)->base);
        case TAST_ENUM_DEF:
            return alignof_struct_def_base(&tast_enum_def_const_unwrap(def)->base);
        case TAST_RAW_UNION_DEF:
            return alignof_struct_def_base(&tast_raw_union_def_const_unwrap(def)->base);
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Tast_struct_literal* struct_literal) {
    Tast_def* def_ = NULL;
    unwrap(symbol_lookup(&def_, lang_type_get_str(LANG_TYPE_MODE_LOG, struct_literal->lang_type)));
    return sizeof_struct_def_base(&tast_struct_def_unwrap(def_)->base, false);
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

uint64_t sizeof_struct_expr(const Tast_expr* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case TAST_STRUCT_LITERAL:
            return sizeof_struct_literal(tast_struct_literal_const_unwrap(struct_literal_or_def));
        default:
            unreachable("");
    }
    unreachable("");
}

static uint64_t ir_sizeof_expr(const Ir_expr* expr) {
    (void) env;
    switch (expr->type) {
        case IR_LITERAL:
            return sizeof_ir_lang_type(ir_literal_get_lang_type(ir_literal_const_unwrap(expr)));
        default:
            unreachable("");
    }
}

static uint64_t ir_sizeof_def(const Ir_def* def) {
    (void) env;
    switch (def->type) {
        case TAST_VARIABLE_DEF:
            return sizeof_ir_lang_type(ir_variable_def_const_unwrap(def)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t ir_sizeof_item(const Ir* item) {
    (void) env;
    switch (item->type) {
        case TAST_EXPR:
            return ir_sizeof_expr(ir_expr_const_unwrap(item));
        case TAST_DEF:
            return ir_sizeof_def(ir_def_const_unwrap(item));
        default:
            unreachable("");
    }
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

