#include <ir_lang_type_after.h>
#include <str_and_num_utils.h>

Ir_lang_type_atom ir_lang_type_primitive_get_atom_normal(Ir_lang_type_primitive ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "i");
            string_extend_int64_t(&a_main, &string, ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width);
            Ir_lang_type_atom atom = ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_signed_int_const_unwrap(ir_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case IR_LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "f");
            string_extend_int64_t(&a_main, &string, ir_lang_type_float_const_unwrap(ir_lang_type).bit_width);
            Ir_lang_type_atom atom = ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_float_const_unwrap(ir_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case IR_LANG_TYPE_UNSIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "u");
            string_extend_int64_t(&a_main, &string, ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width);
            Ir_lang_type_atom atom = ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case IR_LANG_TYPE_OPAQUE: {
            // TODO: remove atom from IR_LANG_TYPE_OPAQUE
            Ir_lang_type_atom atom = ir_lang_type_opaque_const_unwrap(ir_lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
    }
    unreachable("");
}

Ir_lang_type_atom ir_lang_type_primitive_get_atom_c(Ir_lang_type_primitive ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_FLOAT: {
            String string = {0};
            uint32_t bit_width = ir_lang_type_float_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 32) {
                string_extend_cstr(&a_main, &string, "float");
            } else if (bit_width == 64) {
                string_extend_cstr(&a_main, &string, "double");
            } else {
                    msg_todo("bit widths other than 32 or 64 (for floating point numbers) with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
            }
            return ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_float_const_unwrap(ir_lang_type).pointer_depth
            );
        }
        case IR_LANG_TYPE_SIGNED_INT: {
            String string = {0};
            uint32_t bit_width = ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 1) {
                string_extend_cstr(&a_main, &string, "bool");
            } else {
                string_extend_cstr(&a_main, &string, "int");
                if (bit_width == 1) {
                    // TODO: overflow may not happen correctly; maybe remove i1/u1 in earlier passes
                    string_extend_cstr(&a_main, &string, "bool");
                } else if (bit_width == 8) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 16) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 32) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 64) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else {
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 (for integers) with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_signed_int_const_unwrap(ir_lang_type).pointer_depth
            );
        }
        case IR_LANG_TYPE_UNSIGNED_INT: {
            // TODO: deduplicate this and above case?
            // TODO: bit width of 1 here?
            String string = {0};
            uint32_t bit_width = ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 1) {
                string_extend_cstr(&a_main, &string, "bool");
            } else {
                string_extend_cstr(&a_main, &string, "uint");
                if (bit_width == 8) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 16) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 32) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 64) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else {
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return ir_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).pointer_depth
            );
        }
        case IR_LANG_TYPE_OPAQUE:
            return ir_lang_type_atom_new(
                name_new((Strv) {0}, sv("void"), (Ulang_type_vec) {0}, 0),
                ir_lang_type_opaque_const_unwrap(ir_lang_type).atom.pointer_depth
            );
    }
    unreachable("");
}

Ir_lang_type_atom ir_lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Ir_lang_type_primitive ir_lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return ir_lang_type_primitive_get_atom_normal(ir_lang_type);
        case LANG_TYPE_MODE_MSG:
            return ir_lang_type_primitive_get_atom_normal(ir_lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            return ir_lang_type_primitive_get_atom_normal(ir_lang_type);
        case LANG_TYPE_MODE_EMIT_C:
            return ir_lang_type_primitive_get_atom_c(ir_lang_type);
    }
    unreachable("");
}

Ir_lang_type_atom ir_lang_type_get_atom(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE: {
            Ir_lang_type_atom atom = ir_lang_type_primitive_get_atom(mode, ir_lang_type_primitive_const_unwrap(ir_lang_type));
            return atom;
        }
        case IR_LANG_TYPE_STRUCT: {
            Ir_lang_type_atom atom = ir_lang_type_struct_const_unwrap(ir_lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case IR_LANG_TYPE_TUPLE: {
            unreachable("");
        }
        case IR_LANG_TYPE_FN: {
            Ir_lang_type_atom atom = ir_lang_type_atom_new_from_cstr("", 1, 0);
            assert(!strv_is_equal(atom.str.base, sv("void")));
            return atom;
        }
        case IR_LANG_TYPE_VOID: {
            Ir_lang_type_atom atom = ir_lang_type_atom_new_from_cstr("void", 0, SCOPE_BUILTIN);
            return atom;
        }
        case IR_LANG_TYPE_ARRAY:
            unreachable("");
    }
    unreachable("");
}

// TODO: remove this function?
void ir_lang_type_primitive_set_atom(Ir_lang_type_primitive* ir_lang_type, Ir_lang_type_atom atom) {
    switch (ir_lang_type->type) {
        case IR_LANG_TYPE_SIGNED_INT:
            ir_lang_type_signed_int_unwrap(ir_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            ir_lang_type_signed_int_unwrap(ir_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case IR_LANG_TYPE_UNSIGNED_INT:
            ir_lang_type_unsigned_int_unwrap(ir_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            ir_lang_type_unsigned_int_unwrap(ir_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case IR_LANG_TYPE_FLOAT:
            ir_lang_type_float_unwrap(ir_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            ir_lang_type_float_unwrap(ir_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case IR_LANG_TYPE_OPAQUE:
            ir_lang_type_opaque_unwrap(ir_lang_type)->atom = atom;
            return;
    }
    unreachable("");
}

void ir_lang_type_set_atom(Ir_lang_type* ir_lang_type, Ir_lang_type_atom atom) {
    switch (ir_lang_type->type) {
        case IR_LANG_TYPE_PRIMITIVE:
            ir_lang_type_primitive_set_atom(ir_lang_type_primitive_unwrap(ir_lang_type), atom);
            return;
        case IR_LANG_TYPE_STRUCT:
            ir_lang_type_struct_unwrap(ir_lang_type)->atom = atom;
            return;
        case IR_LANG_TYPE_TUPLE:
            unreachable("");
        case IR_LANG_TYPE_FN:
            // TODO
            return;
        case IR_LANG_TYPE_VOID:
            // TODO
            return;
        case IR_LANG_TYPE_ARRAY:
            unreachable("");
    }
    unreachable("");
}

