#ifndef LANG_TYPE_NEW_CONVENIENCE_H
#define LANG_TYPE_NEW_CONVENIENCE_H

#include <lang_type.h>
#include <util.h>

static inline Lang_type lang_type_new_ux(Pos pos, uint32_t bit_width) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(pos, bit_width, 0)
    ));
}

static inline Lang_type lang_type_new_usize(Pos pos) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(pos, params.sizeof_usize, 0)
    ));
}

static inline Lang_type lang_type_new_u1(Pos pos) {
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(pos, 1, 0)
    ));
}

static inline Lang_type lang_type_new_char(Pos pos) {
    return lang_type_new_ux(pos, 8);
}

static inline Lang_type lang_type_new_void(Pos pos) {
    return lang_type_void_const_wrap(lang_type_void_new(pos, 0));
}

static inline Lang_type lang_type_new_slice(Pos pos, Ulang_type item_type, int16_t pointer_depth) {
    Ulang_type_darr gen_args = {0};
    darr_append(&a_main, &gen_args, item_type);
    return lang_type_struct_const_wrap(lang_type_struct_new(
        pos,
        name_new(MOD_PATH_RUNTIME, sv("Slice"), gen_args, SCOPE_TOP_LEVEL),
        pointer_depth
    ));
}

static inline Lang_type lang_type_new_optional(Pos pos, Ulang_type inner_type) {
    Ulang_type_darr gen_args = {0};
    darr_append(&a_main, &gen_args, inner_type);
    return lang_type_struct_const_wrap(lang_type_struct_new(
        pos,
        name_new(MOD_PATH_RUNTIME, sv("Optional"), gen_args, SCOPE_TOP_LEVEL),
        0
    ));
}

#endif // LANG_TYPE_NEW_CONVENIENCE_H
