#ifndef ULANG_TYPE_GET_POS_H
#define ULANG_TYPE_GET_POS_H

static inline Pos ulang_type_get_pos(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).pos;
        case ULANG_TYPE_REG_GENERIC:
            return ulang_type_generic_const_unwrap(lang_type).pos;
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_const_unwrap(lang_type).pos;
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_unwrap(lang_type).pos;
        case ULANG_TYPE_RESOL:
            return ulang_type_resol_const_unwrap(lang_type).pos;
    }
    unreachable("");
}

#endif // ULANG_TYPE_GET_POS_H
