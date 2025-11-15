#ifndef IR_LANG_TYPE_AFTER_H
#define IR_LANG_TYPE_AFTER_H

#include <msg_todo.h>
#include <ir_lang_type_get_pos.h>

Ir_lang_type_atom ir_lang_type_get_atom(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type);

Ir_lang_type_atom ir_lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Ir_lang_type_primitive ir_lang_type);

void ir_lang_type_set_atom(Ir_lang_type* ir_lang_type, Ir_lang_type_atom atom);

static inline Ir_name ir_lang_type_get_str(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    return ir_lang_type_get_atom(mode, ir_lang_type).str;
}

static inline int16_t ir_lang_type_get_pointer_depth(Ir_lang_type ir_lang_type) {
    return ir_lang_type_get_atom(LANG_TYPE_MODE_LOG, ir_lang_type).pointer_depth;
}

static inline int16_t ir_lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE mode, Ir_lang_type_primitive ir_lang_type) {
    return ir_lang_type_primitive_get_atom(mode, ir_lang_type).pointer_depth;
}

static inline uint32_t ir_lang_type_primitive_get_bit_width(Ir_lang_type_primitive ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_UNSIGNED_INT:
            return ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width;
        case IR_LANG_TYPE_SIGNED_INT:
            return ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width;
        case IR_LANG_TYPE_FLOAT:
            return ir_lang_type_float_const_unwrap(ir_lang_type).bit_width;
        case IR_LANG_TYPE_OPAQUE:
            unreachable("");
    }
    unreachable("");
}

static inline uint32_t ir_lang_type_get_bit_width(Ir_lang_type ir_lang_type) {
    return ir_lang_type_primitive_get_bit_width(ir_lang_type_primitive_const_unwrap(ir_lang_type));
}

static inline void ir_lang_type_set_pointer_depth(Ir_lang_type* ir_lang_type, int16_t pointer_depth) {
    Ir_lang_type_atom atom = ir_lang_type_get_atom(LANG_TYPE_MODE_LOG, *ir_lang_type);
    atom.pointer_depth = pointer_depth;
    ir_lang_type_set_atom(ir_lang_type, atom);
}

static inline Ir_lang_type ir_lang_type_new_ux(uint32_t bit_width) {
    return ir_lang_type_primitive_const_wrap(ir_lang_type_unsigned_int_const_wrap(
        ir_lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)
    ));
}

static inline Ir_lang_type ir_lang_type_new_u8(void) {
    return ir_lang_type_new_ux(8);
}

static inline Ir_lang_type ir_lang_type_new_usize(void) {
    return ir_lang_type_new_ux(64 /* TODO: change based on target */);
}

// TODO: rename to ir_lang_type_is_struct_like
static inline bool llvm_is_struct_like(IR_LANG_TYPE_TYPE type) {
    switch (type) {
        case IR_LANG_TYPE_STRUCT:
            return true;
        case IR_LANG_TYPE_PRIMITIVE:
            return false;
        case IR_LANG_TYPE_VOID:
            return false;
        case IR_LANG_TYPE_TUPLE:
            return true;
        case IR_LANG_TYPE_FN:
            return false;
    }
    unreachable("");
}

static inline Ir_lang_type ir_lang_type_pointer_depth_inc(Ir_lang_type lang_type) {
    ir_lang_type_set_pointer_depth(&lang_type, ir_lang_type_get_pointer_depth(lang_type) + 1);
    return lang_type;
}

static inline Ir_lang_type ir_lang_type_pointer_depth_dec(Ir_lang_type lang_type) {
    ir_lang_type_set_pointer_depth(&lang_type, ir_lang_type_get_pointer_depth(lang_type) - 1);
    return lang_type;
}

#endif // IR_LANG_TYPE_AFTER_H
