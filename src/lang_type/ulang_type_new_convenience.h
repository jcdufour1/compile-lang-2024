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

static inline Ulang_type ulang_type_new_char_pointer_depth_0(void) {
    return env.ulang_type_char_pointer_depth_0;
}

static inline Uast_expr_vec ulang_type_struct_literal_membs_new_char(Pos pos, char data) {
    Uast_expr_vec membs = {0};
    vec_append(&a_main, &membs, uast_literal_wrap(uast_int_wrap(uast_int_new(pos, data))));
    return membs;
}

static inline Uast_unary* ulang_type_struct_literal_new_char(Pos pos, char data) {
    return uast_unary_new(
        pos,
        uast_struct_literal_wrap(uast_struct_literal_new(pos, ulang_type_struct_literal_membs_new_char(pos, data))),
        UNARY_UNSAFE_CAST,
        ulang_type_new_char_pointer_depth_0()
    );
}

static inline Ulang_type_vec ulang_type_gen_args_char_new(void) {
    return env.gen_args_char;
}

#endif // ULANG_TYPE_NEW_CONVENIENCE_H
