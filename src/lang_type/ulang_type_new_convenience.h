#ifndef ULANG_TYPE_NEW_CONVENIENCE_H
#define ULANG_TYPE_NEW_CONVENIENCE_H

#include <ulang_type.h>
#include <env.h>
#include <util.h>

static inline Ulang_type ulang_type_new_int_x(Strv base) {
    return ulang_type_regular_const_wrap(ulang_type_regular_new(
        (Ulang_type_atom) {
            .str = uname_new(MOD_ALIAS_BUILTIN, base, (Ulang_type_vec) {0}, SCOPE_BUILTIN),
            .pointer_depth = 0
        },
        POS_BUILTIN
    ));
}

static inline Ulang_type ulang_type_new_usize(void) {
    return ulang_type_new_int_x(sv(params.usize_size_ux));
}

static inline Ulang_type_vec ulang_type_gen_args_char_new(void) {
    return env.gen_args_char;
}

#endif // ULANG_TYPE_NEW_CONVENIENCE_H
