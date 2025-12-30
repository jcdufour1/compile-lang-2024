#ifndef ULANG_TYPE_AFTER_H
#define ULANG_TYPE_AFTER_H

#include <name.h>
#include <ulang_type.h>

static inline bool ulang_type_lit_get_uname(Uname* result, Ulang_type_lit ulang_type) {
    switch (ulang_type.type) {
        case ULANG_TYPE_INT_LIT:
            return false;
        case ULANG_TYPE_FLOAT_LIT:
            return false;
        case ULANG_TYPE_STRING_LIT:
            return false;
        case ULANG_TYPE_STRUCT_LIT:
            return false;
        case ULANG_TYPE_FN_LIT:
            *result = name_to_uname(ulang_type_fn_lit_const_unwrap(ulang_type).name);
            return true;
        default:
            unreachable("");
    }
    unreachable("");
}

static inline bool ulang_type_get_uname(Uname* result, Ulang_type ulang_type) {
    switch (ulang_type.type) {
        case ULANG_TYPE_TUPLE:
            return false;
        case ULANG_TYPE_FN:
            return false;
        case ULANG_TYPE_REGULAR:
            *result = ulang_type_regular_const_unwrap(ulang_type).name;
            return true;
        case ULANG_TYPE_ARRAY:
            return false;
        case ULANG_TYPE_EXPR:
            return false;
        case ULANG_TYPE_LIT:
            return ulang_type_lit_get_uname(result, ulang_type_lit_const_unwrap(ulang_type));
        case ULANG_TYPE_REMOVED:
            return false;
        default:
            unreachable("");
    }
    unreachable("");
}

#endif // ULANG_TYPE_AFTER_H
