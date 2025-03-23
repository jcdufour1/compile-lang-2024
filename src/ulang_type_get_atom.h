#ifndef ULANG_TYPE_GET_ATOM_H
#define ULANG_TYPE_GET_ATOM_H

#include <ulang_type.h>

static inline Ulang_type_atom ulang_type_get_atom(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).atom;
        case ULANG_TYPE_REG_GENERIC:
            return ulang_type_generic_const_unwrap(lang_type).atom;
        case ULANG_TYPE_TUPLE:
            unreachable("");
        case ULANG_TYPE_FN:
            unreachable("");
    }
}

#endif // ULANG_TYPE_GET_ATOM_H
