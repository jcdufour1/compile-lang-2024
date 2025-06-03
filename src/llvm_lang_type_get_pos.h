#ifndef LLVM_LANG_TYPE_GET_POS_H
#define LLVM_LANG_TYPE_GET_POS_H

#include <llvm_lang_type.h>

static inline Pos llvm_lang_type_primitive_get_pos(Llvm_lang_type_primitive llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_CHAR:
            return llvm_lang_type_char_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_SIGNED_INT:
            return llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_UNSIGNED_INT:
            return llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_FLOAT:
            return llvm_lang_type_float_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_OPAQUE:
            return llvm_lang_type_opaque_const_unwrap(llvm_lang_type).pos;
    }
    unreachable("");
}

static inline Pos llvm_lang_type_get_pos(Llvm_lang_type llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_PRIMITIVE:
            return llvm_lang_type_primitive_get_pos(llvm_lang_type_primitive_const_unwrap(llvm_lang_type));
        case LLVM_LANG_TYPE_STRUCT:
            return llvm_lang_type_struct_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_RAW_UNION:
            return llvm_lang_type_raw_union_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_ENUM:
            return llvm_lang_type_enum_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_TUPLE:
            return llvm_lang_type_tuple_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_VOID:
            return llvm_lang_type_void_const_unwrap(llvm_lang_type).pos;
        case LLVM_LANG_TYPE_FN:
            return llvm_lang_type_fn_const_unwrap(llvm_lang_type).pos;
    }
    unreachable("");
}

#endif // LLVM_LANG_TYPE_GET_POS_H
