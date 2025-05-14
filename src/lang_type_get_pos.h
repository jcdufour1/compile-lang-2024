#ifndef LANG_TYPE_GET_POS_H
#define LANG_TYPE_GET_POS_H

#include <lang_type.h>

static inline Pos lang_type_primitive_get_pos(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            return lang_type_char_const_unwrap(lang_type).pos;
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(lang_type).pos;
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(lang_type).pos;
        case LANG_TYPE_ANY:
            return lang_type_any_const_unwrap(lang_type).pos;
    }
    unreachable("");
}

static inline Pos lang_type_get_pos(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return lang_type_primitive_get_pos(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_STRUCT:
            return lang_type_struct_const_unwrap(lang_type).pos;
        case LANG_TYPE_RAW_UNION:
            return lang_type_raw_union_const_unwrap(lang_type).pos;
        case LANG_TYPE_SUM:
            return lang_type_sum_const_unwrap(lang_type).pos;
        case LANG_TYPE_TUPLE:
            return lang_type_tuple_const_unwrap(lang_type).pos;
        case LANG_TYPE_VOID:
            return lang_type_void_const_unwrap(lang_type).pos;
        case LANG_TYPE_FN:
            return lang_type_fn_const_unwrap(lang_type).pos;
    }
    unreachable("");
}

#endif // LANG_TYPE_GET_POS_H
