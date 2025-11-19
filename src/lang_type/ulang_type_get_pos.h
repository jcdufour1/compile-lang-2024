#ifndef ULANG_TYPE_GET_POS_H
#define ULANG_TYPE_GET_POS_H

#include <ulang_type.h>

// TODO: include header file and remove this forward declaration
bool name_from_uname(Name* new_name, Uname name, Pos name_pos);

static inline Pos ulang_type_get_pos(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REMOVED:
            return ulang_type_removed_const_unwrap(lang_type).pos;
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).pos;
        case ULANG_TYPE_ARRAY:
            return ulang_type_array_const_unwrap(lang_type).pos;
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_const_unwrap(lang_type).pos;
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_unwrap(lang_type).pos;
        case ULANG_TYPE_EXPR:
            return ulang_type_expr_const_unwrap(lang_type).pos;
        case ULANG_TYPE_INT:
            return ulang_type_int_const_unwrap(lang_type).pos;
    }
    unreachable("");
}


#endif // ULANG_TYPE_GET_POS_H
