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
    return lang_type_new_ux(8);
}

static inline Lang_type lang_type_new_void(void) {
    return lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN));
}

#endif // LANG_TYPE_NEW_CONVENIENCE_H
