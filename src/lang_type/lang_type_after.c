#include <lang_type_after.h>
#include <lang_type_print.h>

Lang_type_atom lang_type_primitive_get_atom_normal(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "i");
            string_extend_int64_t(&a_main, &string, lang_type_signed_int_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new(MOD_PATH_BUILTIN, string_to_strv(string), (Ulang_type_vec) {0}, 0, (Attrs) {0}),
                lang_type_signed_int_const_unwrap(lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "f");
            string_extend_int64_t(&a_main, &string, lang_type_float_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new(MOD_PATH_BUILTIN, string_to_strv(string), (Ulang_type_vec) {0}, 0, (Attrs) {0}),
                lang_type_float_const_unwrap(lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_UNSIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "u");
            string_extend_int64_t(&a_main, &string, lang_type_unsigned_int_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new(MOD_PATH_BUILTIN, string_to_strv(string), (Ulang_type_vec) {0}, 0, (Attrs) {0}),
                lang_type_unsigned_int_const_unwrap(lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_OPAQUE: {
            log(
                LOG_DEBUG,
                FMT"\n",
                lang_type_atom_print(
                    LANG_TYPE_MODE_LOG,
                    lang_type_atom_new(
                        name_new(MOD_PATH_BUILTIN, sv("opaque"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0}),
                        lang_type_opaque_const_unwrap(lang_type).pointer_depth
                    )
                )
            );
            return lang_type_atom_new(
                name_new(MOD_PATH_BUILTIN, sv("opaque"), (Ulang_type_vec) {0}, SCOPE_TOP_LEVEL, (Attrs) {0}),
                lang_type_opaque_const_unwrap(lang_type).pointer_depth
            );
        }
    }
    unreachable("");
}

static bool lang_type_atom_is_number_finish(Lang_type_atom atom, bool allow_decimal) {
    size_t idx = 0;
    bool decimal_enc = false;
    for (idx = 1; idx < atom.str.base.count; idx++) {
        if (strv_at(atom.str.base, idx) == '.') {
            if (!allow_decimal || decimal_enc) {
                return false;
            }
            decimal_enc = true;
        } else if (!isdigit(strv_at(atom.str.base, idx))) {
            return false;
        }
    }

    return idx > 1;
}

// TODO: get rid of this function?
bool lang_type_atom_is_signed(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (strv_at(atom.str.base, 0) != 'i') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, false);
}

// TODO: get rid of this function?
bool lang_type_atom_is_unsigned(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (strv_at(atom.str.base, 0) != 'u') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, false);
}

bool lang_type_atom_is_float(Lang_type_atom atom) {
    if (atom.str.base.count < 1) {
        return false;
    }
    if (strv_at(atom.str.base, 0) != 'f') {
        return false;
    }
    return lang_type_atom_is_number_finish(atom, true);
}

bool lang_type_atom_is_number(Lang_type_atom atom) {
    return lang_type_atom_is_unsigned(atom) || lang_type_atom_is_signed(atom);
}

// TODO: put strings in a hash table to avoid allocating duplicate types
Lang_type_atom lang_type_atom_unsigned_to_signed(Lang_type_atom lang_type) {
    // TODO: remove or change this function
    assert(lang_type_atom_is_unsigned(lang_type));

    if (lang_type.pointer_depth != 0) {
        todo();
    }

    String string = {0};
    string_extend_cstr(&a_main, &string, "i");
    string_extend_strv(&a_main, &string, strv_slice(lang_type.str.base, 1, lang_type.str.base.count - 1));
    return lang_type_atom_new(name_new(MOD_PATH_BUILTIN, string_to_strv(string), (Ulang_type_vec) {0}, 0, (Attrs) {0}), 0);
}

// TODO: remove this function and use try_lang_type_get_atom instead
Lang_type_atom lang_type_get_atom(LANG_TYPE_MODE mode, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE: {
            Lang_type_atom atom = lang_type_primitive_get_atom(mode, lang_type_primitive_const_unwrap(lang_type));
            return atom;
        }
        case LANG_TYPE_ENUM: {
            Lang_type_atom atom = lang_type_enum_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_STRUCT: {
            Lang_type_atom atom = lang_type_struct_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_RAW_UNION: {
            Lang_type_atom atom = lang_type_raw_union_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_TUPLE: {
            unreachable("");
        }
        case LANG_TYPE_FN: {
            todo();
        }
        case LANG_TYPE_ARRAY:
            unreachable("");
        case LANG_TYPE_REMOVED:
            unreachable("");
        case LANG_TYPE_VOID: {
            Lang_type_atom atom = lang_type_atom_new_from_cstr("void", 0, SCOPE_TOP_LEVEL);
            return atom;
        }
    }
    unreachable("");
}

// TODO: remove this function?
void lang_type_primitive_set_atom(Lang_type_primitive* lang_type, Lang_type_atom atom) {
    switch (lang_type->type) {
        case LANG_TYPE_SIGNED_INT:
            lang_type_signed_int_unwrap(lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_signed_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_UNSIGNED_INT:
            lang_type_unsigned_int_unwrap(lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_unsigned_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_FLOAT:
            lang_type_float_unwrap(lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_float_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_OPAQUE:
            assert(strv_is_equal(atom.str.base, sv("opaque")));
            lang_type_opaque_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
    }
    unreachable("");
}

bool try_lang_type_get_atom(Lang_type_atom* result, LANG_TYPE_MODE mode, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE: {
            Lang_type_atom atom = lang_type_primitive_get_atom(mode, lang_type_primitive_const_unwrap(lang_type));
            *result = atom;
            return true;
        }
        case LANG_TYPE_ENUM: {
            Lang_type_atom atom = lang_type_enum_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            *result = atom;
            return true;
        }
        case LANG_TYPE_STRUCT: {
            Lang_type_atom atom = lang_type_struct_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            *result = atom;
            return true;
        }
        case LANG_TYPE_RAW_UNION: {
            Lang_type_atom atom = lang_type_raw_union_const_unwrap(lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            *result = atom;
            return true;
        }
        case LANG_TYPE_TUPLE:
            return false;
        case LANG_TYPE_FN:
            return false;
        case LANG_TYPE_ARRAY:
            return false;
        case LANG_TYPE_REMOVED:
            return false;
        case LANG_TYPE_VOID:
            *result = lang_type_atom_new_from_cstr("void", 0, SCOPE_TOP_LEVEL);
            return true;
    }
    unreachable("");
}

void lang_type_set_atom(Lang_type* lang_type, Lang_type_atom atom) {
    switch (lang_type->type) {
        case LANG_TYPE_PRIMITIVE:
            lang_type_primitive_set_atom(lang_type_primitive_unwrap(lang_type), atom);
            return;
        case LANG_TYPE_ENUM:
            lang_type_enum_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_STRUCT:
            lang_type_struct_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_RAW_UNION:
            lang_type_raw_union_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_ARRAY:
            unreachable("");
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_FN:
            unreachable("");
        case LANG_TYPE_VOID:
            unreachable("");
        case LANG_TYPE_REMOVED:
            unreachable("");
    }
    unreachable("");
}

Lang_type_atom lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Lang_type_primitive lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return lang_type_primitive_get_atom_normal(lang_type);
        case LANG_TYPE_MODE_MSG:
            return lang_type_primitive_get_atom_normal(lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            unreachable("");
        case LANG_TYPE_MODE_EMIT_C:
            // TODO: make separate LANG_TYPE_MODE for IR?
            unreachable("");
    }
    unreachable("");
}

