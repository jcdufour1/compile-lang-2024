#ifndef IR_LANG_TYPE_GET_POS_H
#define IR_LANG_TYPE_GET_POS_H

#include <ir_lang_type.h>

static inline Pos ir_lang_type_primitive_get_pos(Ir_lang_type_primitive ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_SIGNED_INT:
            return ir_lang_type_signed_int_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_UNSIGNED_INT:
            return ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_FLOAT:
            return ir_lang_type_float_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_OPAQUE:
            return ir_lang_type_opaque_const_unwrap(ir_lang_type).pos;
    }
    unreachable("");
}

static inline Pos ir_lang_type_get_pos(Ir_lang_type ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            return ir_lang_type_primitive_get_pos(ir_lang_type_primitive_const_unwrap(ir_lang_type));
        case IR_LANG_TYPE_STRUCT:
            return ir_lang_type_struct_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_TUPLE:
            return ir_lang_type_tuple_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_VOID:
            return ir_lang_type_void_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_FN:
            return ir_lang_type_fn_const_unwrap(ir_lang_type).pos;
        case IR_LANG_TYPE_ARRAY:
            return ir_lang_type_array_const_unwrap(ir_lang_type).pos;
    }
    unreachable("");
}

#endif // IR_LANG_TYPE_GET_POS_H
