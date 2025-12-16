#include <ulang_type_after.h>

bool ulang_type_get_uname(Uname* result, Ulang_type ulang_type) {
    switch (ulang_type.type) {
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_REGULAR:
            *result = ulang_type_regular_const_unwrap(ulang_type).name;
            return true;
        case ULANG_TYPE_ARRAY:
            todo();
        case ULANG_TYPE_EXPR:
            todo();
        case ULANG_TYPE_LIT:
            todo();
        case ULANG_TYPE_REMOVED:
            todo();
        default:
            unreachable("");
    }
    unreachable("");
}

