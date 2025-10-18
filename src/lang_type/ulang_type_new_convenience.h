#ifndef ULANG_TYPE_NEW_CONVENIENCE_H
#define ULANG_TYPE_NEW_CONVENIENCE_H

#include <ulang_type.h>
#include <env.h>
#include <util.h>
#include <uast.h>
#include <lang_type_new_convenience.h>
#include <lang_type_from_ulang_type.h>

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

static inline Ulang_type ulang_type_new_char(void) {
    return ulang_type_new_int_x(sv("u8"));
}

static inline Ulang_type_vec ulang_type_gen_args_char_new(void) {
    return env.gen_args_char;
}

// TODO: remove this forward decl?
Ulang_type lang_type_to_ulang_type(Lang_type lang_type);

static inline Ulang_type ulang_type_new_slice(Pos pos, Ulang_type item_type, int16_t pointer_depth) {
    return lang_type_to_ulang_type(lang_type_new_slice(pos, item_type, pointer_depth));
}

#endif // ULANG_TYPE_NEW_CONVENIENCE_H
