#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

#include <str_and_num_utils.h>
#include <parameters.h>
#include <lang_type_mode.h>
#include <lang_type_hand_written.h>
#include <lang_type.h>

Name lang_type_primitive_get_name(const Lang_type_primitive lang_type);

// TODO: remove mode parameter?
bool lang_type_literal_get_name(Name* result, LANG_TYPE_MODE mode, Lang_type_lit lang_type);

// TODO: remove mode parameter?
bool lang_type_get_name(Name* result, LANG_TYPE_MODE mode, Lang_type lang_type);

bool lang_type_name_base_is_signed(Strv name);

bool lang_type_name_base_is_unsigned(Strv name);

bool lang_type_name_base_is_float(Strv name);

bool lang_type_name_base_is_number(Strv name);

static inline int16_t lang_type_get_pointer_depth(Lang_type lang_type);

static inline uint32_t lang_type_primitive_get_bit_width(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_FLOAT:
            return lang_type_float_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_OPAQUE:
            unreachable("");
    }
    unreachable("");
}

static inline uint32_t lang_type_get_bit_width(Lang_type lang_type) {
    return lang_type_primitive_get_bit_width(lang_type_primitive_const_unwrap(lang_type));
}

// only for unsafe_cast and similar cases
static inline bool lang_type_is_number_like(Lang_type lang_type) {
    if (lang_type_get_pointer_depth(lang_type) > 0) {
        return true;
    }
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    switch (lang_type_primitive_const_unwrap(lang_type).type) {
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_FLOAT:
            return true;
        case LANG_TYPE_OPAQUE:
            return false;
    }
    unreachable("");
}

// for general use
static inline bool lang_type_primitive_is_number(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_SIGNED_INT:
            return true;
        case LANG_TYPE_UNSIGNED_INT:
            return true;
        case LANG_TYPE_FLOAT:
            return true;
        case LANG_TYPE_OPAQUE:
            return false;
    }
    unreachable("");
}

// for general use
static inline bool lang_type_is_number(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_is_number(lang_type_primitive_const_unwrap(lang_type));
}

static inline bool lang_type_is_signed(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_SIGNED_INT;
}

static inline bool lang_type_is_unsigned(Lang_type lang_type) {
    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return false;
    }
    return lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_UNSIGNED_INT;
}

static inline bool is_struct_like(LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_STRUCT:
            return true;
        case LANG_TYPE_PRIMITIVE:
            return false;
        case LANG_TYPE_RAW_UNION:
            return true;
        case LANG_TYPE_ARRAY:
            return true;
        case LANG_TYPE_VOID:
            return false;
        case LANG_TYPE_ENUM:
            return true;
        case LANG_TYPE_TUPLE:
            return true;
        case LANG_TYPE_FN:
            return false;
        case LANG_TYPE_REMOVED:
            return false;
        case LANG_TYPE_LIT:
            return false;
    }
    unreachable("");
}

// convert literal i2 to i32, etc. to avoid making variable definitions small integers by default
Lang_type lang_type_standardize(Lang_type lang_type, bool lang_type_is_lit, Pos pos);

#endif // LANG_TYPE_AFTER_H
