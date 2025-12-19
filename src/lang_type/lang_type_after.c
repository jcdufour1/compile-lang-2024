#include <lang_type_after.h>
#include <lang_type_print.h>

Name lang_type_primitive_get_name(const Lang_type_primitive lang_type) {
    Strv new_base = {0};
    switch (lang_type.type) {
        case LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "i%"PRIu32, lang_type_signed_int_const_unwrap(lang_type).bit_width);
            break;
        }
        case LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "f%"PRIu32, lang_type_float_const_unwrap(lang_type).bit_width);
            break;
        }
        case LANG_TYPE_UNSIGNED_INT:
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "u%"PRIu32, lang_type_unsigned_int_const_unwrap(lang_type).bit_width);
            break;
        case LANG_TYPE_OPAQUE: {
            new_base = sv("opaque");
            break;
        }
        default:
            unreachable("");
    }

    assert(new_base.count > 0);
    Name new_name = name_new(MOD_PATH_BUILTIN, new_base, (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
    assert(!strv_is_equal(new_name.base, sv("void")));
    assert(new_name.base.count > 0);
    return new_name;
}

bool lang_type_literal_get_name(Name* result, Lang_type_lit lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_INT_LIT:
            return false;
        case LANG_TYPE_FLOAT_LIT:
            return false;
        case LANG_TYPE_STRING_LIT:
            return false;
        case LANG_TYPE_STRUCT_LIT:
            return false;
        case LANG_TYPE_FN_LIT:
            *result = lang_type_fn_lit_const_unwrap(lang_type).name;
            return true;
        default:
            unreachable("");
    }
    unreachable("");
}

bool lang_type_get_name(Name* result, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            *result = lang_type_primitive_get_name(lang_type_primitive_const_unwrap(lang_type));
            return true;
        case LANG_TYPE_STRUCT:
            *result = lang_type_struct_const_unwrap(lang_type).name;
            return true;
        case LANG_TYPE_RAW_UNION:
            *result = lang_type_raw_union_const_unwrap(lang_type).name;
            return true;
        case LANG_TYPE_ENUM:
            *result = lang_type_enum_const_unwrap(lang_type).name;
            return true;
        case LANG_TYPE_TUPLE:
            return false;
        case LANG_TYPE_VOID:
            *result = name_new(MOD_PATH_BUILTIN, sv("void"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL);
            return true;
        case LANG_TYPE_FN:
            return false;
        case LANG_TYPE_ARRAY:
            return false;
        case LANG_TYPE_LIT:
            return lang_type_literal_get_name(result, lang_type_lit_const_unwrap(lang_type));
        case LANG_TYPE_REMOVED:
            return false;
    }
    unreachable("");
}

static bool lang_type_name_base_is_number_finish(Strv name_base, bool allow_decimal) {
    size_t idx = 0;
    bool decimal_enc = false;
    for (idx = 1; idx < name_base.count; idx++) {
        if (strv_at(name_base, idx) == '.') {
            if (!allow_decimal || decimal_enc) {
                return false;
            }
            decimal_enc = true;
        } else if (!isdigit(strv_at(name_base, idx))) {
            return false;
        }
    }

    return idx > 1;
}

bool lang_type_name_base_is_signed(Strv name_base) {
    if (name_base.count < 1) {
        return false;
    }
    if (strv_at(name_base, 0) != 'i') {
        return false;
    }
    return lang_type_name_base_is_number_finish(name_base, false);
}

bool lang_type_name_base_is_unsigned(Strv name_base) {
    if (name_base.count < 1) {
        return false;
    }
    if (strv_at(name_base, 0) != 'u') {
        return false;
    }
    return lang_type_name_base_is_number_finish(name_base, false);
}

bool lang_type_name_base_is_float(Strv name_base) {
    if (name_base.count < 1) {
        return false;
    }
    if (strv_at(name_base, 0) != 'f') {
        return false;
    }
    return lang_type_name_base_is_number_finish(name_base, true);
}

bool lang_type_name_base_is_int(Strv name_base) {
    return lang_type_name_base_is_unsigned(name_base) || lang_type_name_base_is_signed(name_base);
}

bool lang_type_name_base_is_number(Strv name_base) {
    return lang_type_name_base_is_int(name_base) || lang_type_name_base_is_float(name_base);
}

static bool bit_width_calculation(uint32_t* new_width, uint32_t old_width, Pos pos_arg) {
    if (old_width <= 32) {
        *new_width = 32;
        return true;
    }

    if (old_width <= 64) {
        *new_width = 64;
        return true;
    }

    // TODO
    msg(
        DIAG_GEN_INFER_MORE_THAN_64_WIDE,
        pos_arg,
        "bit widths larger than 64 for type inference in generics is not yet implemented\n"
    );
    return false;
}

// convert literal i2 to i32, etc. to avoid making variable definitions small integers by default
Lang_type lang_type_standardize(Lang_type lang_type, bool lang_type_is_lit, Pos pos) {
    if (!lang_type_is_lit) {
        return lang_type;
    }

    if (lang_type.type != LANG_TYPE_PRIMITIVE) {
        return lang_type;
    }

    Lang_type_primitive primitive = lang_type_primitive_const_unwrap(lang_type);
    switch (primitive.type) {
        case LANG_TYPE_SIGNED_INT: {
            Lang_type_signed_int sign = lang_type_signed_int_const_unwrap(primitive);
            if (!bit_width_calculation(&sign.bit_width, sign.bit_width, pos)) {
                return lang_type;
            }
            return lang_type_primitive_const_wrap(
                lang_type_signed_int_const_wrap(sign)
            );
        } break;
        case LANG_TYPE_UNSIGNED_INT: {
            Lang_type_unsigned_int unsign = lang_type_unsigned_int_const_unwrap(primitive);
            Lang_type_signed_int sign = lang_type_signed_int_new(
                pos,
                unsign.bit_width,
                unsign.pointer_depth
            );
            if (!bit_width_calculation(&sign.bit_width, sign.bit_width, pos)) {
                return lang_type;
            }
            return lang_type_primitive_const_wrap(
                lang_type_signed_int_const_wrap(sign)
            );
        } break;
        case LANG_TYPE_FLOAT: {
            Lang_type_float lang_float = lang_type_float_const_unwrap(primitive);
            if (!bit_width_calculation(&lang_float.bit_width, lang_float.bit_width, pos)) {
                return lang_type;
            }
            return lang_type_primitive_const_wrap(
                lang_type_float_const_wrap(lang_float)
            );
        } break;
        case LANG_TYPE_OPAQUE:
            break;
    }
    unreachable("");
}

