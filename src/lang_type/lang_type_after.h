#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

#include <msg_todo.h>
#include <str_and_num_utils.h>
#include <parameters.h>
#include <lang_type_mode.h>
#include <lang_type_hand_written.h>
#include <lang_type.h>

Lang_type_atom lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Lang_type_primitive lang_type);

void lang_type_set_atom(Lang_type* lang_type, Lang_type_atom atom);

bool try_lang_type_get_atom(Lang_type_atom* result, LANG_TYPE_MODE mode, Lang_type lang_type);

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

// TODO: make separate file for lang_type_atom functions?
static inline int64_t i_lang_type_atom_to_bit_width(const Lang_type_atom atom) {
    //assert(lang_type_atom_is_signed(lang_type));
    return strv_to_int64_t(POS_BUILTIN, strv_slice(atom.str.base, 1, atom.str.base.count - 1));
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

static inline Name lang_type_get_str(LANG_TYPE_MODE mode, Lang_type lang_type) {
    Lang_type_atom atom = {0};
    if (!try_lang_type_get_atom(&atom, mode, lang_type)) {
        return util_literal_name_new();
    }
    return atom.str;
}

// convert literal i2 to i32, etc. to avoid making variable definitions small integers by default
Lang_type lang_type_standardize(Lang_type lang_type, bool lang_type_is_lit, Pos pos);

#endif // LANG_TYPE_AFTER_H
