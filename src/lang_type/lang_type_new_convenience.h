#ifndef LANG_TYPE_NEW_CONVENIENCE_H
#define LANG_TYPE_NEW_CONVENIENCE_H

#include <lang_type.h>
#include <util.h>

static inline Lang_type lang_type_new_ux(int32_t bit_width) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)
    ));
}

static inline Lang_type lang_type_new_usize(void) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, params.sizeof_usize, 0)
    ));
}

static inline Lang_type lang_type_new_u1(void) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, 1, 0)
    ));
}

static inline Lang_type lang_type_new_char(void) {
    return lang_type_struct_const_wrap(lang_type_struct_new(
        POS_BUILTIN,
        lang_type_atom_new(
            name_new(MOD_PATH_RUNTIME, sv("char"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL),
            0
        )
    ));
}

static inline Tast_expr_vec lang_type_struct_literal_membs_new_char(Pos pos, char data) {
    Tast_expr_vec membs = {0};
    vec_append(&a_main, &membs, tast_literal_wrap(tast_int_wrap(tast_int_new(pos, data, lang_type_new_ux(8)))));
    return membs;
}

static inline Tast_expr_vec lang_type_struct_literal_new_char(Pos pos, char data) {
    Tast_expr_vec membs = {0};
    vec_append(&a_main, &membs, tast_literal_wrap(tast_int_wrap(tast_int_new(pos, data, lang_type_new_ux(8)))));
    return membs;
}

static inline Lang_type lang_type_new_void(void) {
    return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
}

#endif // LANG_TYPE_NEW_CONVENIENCE_H
