#ifndef ULANG_TYPE_NEW_CONVENIENCE_H
#define ULANG_TYPE_NEW_CONVENIENCE_H

#include <ulang_type.h>
#include <env.h>
#include <util.h>
#include <uast.h>
#include <lang_type_new_convenience.h>
#include <lang_type_from_ulang_type.h>
#include <strv.h>

static inline Ulang_type ulang_type_new_int_x(Pos pos, Strv base) {
    assert(strv_at(base, 0) == 'u' || strv_at(base, 0) == 'i');
    for (size_t idx = 1; idx < base.count; idx++) {
        assert(isdigit(strv_at(base, idx)));
    }

    return ulang_type_regular_const_wrap(ulang_type_regular_new(
        pos,
        uname_new(MOD_ALIAS_BUILTIN, base, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
        0
    ));
}

static inline Ulang_type ulang_type_new_usize(Pos pos) {
    return ulang_type_new_int_x(pos, sv(params.usize_size_ux));
}

static inline Ulang_type ulang_type_new_char(Pos pos) {
    return ulang_type_new_int_x(pos, sv("u8"));
}

static inline Ulang_type_vec ulang_type_gen_args_char_new(void) {
    return env.gen_args_char;
}

// TODO: remove this forward decl?
Ulang_type lang_type_to_ulang_type(Lang_type lang_type);

static inline Ulang_type ulang_type_new_slice(Pos pos, Ulang_type item_type, int16_t pointer_depth) {
    return lang_type_to_ulang_type(lang_type_new_slice(pos, item_type, pointer_depth));
}

static inline Ulang_type ulang_type_new_void(Pos pos) {
    return lang_type_to_ulang_type(lang_type_void_const_wrap(lang_type_void_new(pos, 0)));
}

static inline Ulang_type ulang_type_new_optional(Pos pos, Ulang_type inner_type) {
    return lang_type_to_ulang_type(lang_type_new_optional(pos, inner_type));
}

#endif // ULANG_TYPE_NEW_CONVENIENCE_H
