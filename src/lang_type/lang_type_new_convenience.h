#ifndef LANG_TYPE_NEW_CONVENIENCE_H
#define LANG_TYPE_NEW_CONVENIENCE_H

#include <lang_type.h>
#include <util.h>

static inline Lang_type lang_type_new_ux(uint32_t bit_width) {
    // TODO: don't use POS_BUILTIN?
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)
    ));
}

static inline Lang_type lang_type_new_usize(void) {
    // TODO: don't use POS_BUILTIN?
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, params.sizeof_usize, 0)
    ));
}

static inline Lang_type lang_type_new_u1(void) {
    // TODO: don't use POS_BUILTIN?
    return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(
        lang_type_unsigned_int_new(POS_BUILTIN, 1, 0)
    ));
}

static inline Lang_type lang_type_new_char(void) {
    return lang_type_new_ux(8);
}

static inline Lang_type lang_type_new_void(void) {
    return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
}

static inline Lang_type lang_type_new_slice(Pos pos, Ulang_type item_type, int16_t pointer_depth) {
    Ulang_type_vec gen_args = {0};
    vec_append(&a_main, &gen_args, item_type);
    return lang_type_struct_const_wrap(lang_type_struct_new(pos, lang_type_atom_new(
        name_new(MOD_PATH_RUNTIME, sv("Slice"), gen_args, SCOPE_TOP_LEVEL, (Attrs) {0}),
        pointer_depth
    )));
}

#endif // LANG_TYPE_NEW_CONVENIENCE_H
