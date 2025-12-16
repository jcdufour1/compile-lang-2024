#include <ir_lang_type_after.h>
#include <str_and_num_utils.h>
#include <ast_msg.h>
#include <name.h>

static Ir_name ir_lang_type_primitive_get_name_normal(Ir_lang_type_primitive ir_lang_type) {
    Strv new_base = {0};
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "i%"PRIu32, ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width);
            break;
        }
        case IR_LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "f%"PRIu32, ir_lang_type_float_const_unwrap(ir_lang_type).bit_width);
            break;
        }
        case IR_LANG_TYPE_UNSIGNED_INT:
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "u%"PRIu32, ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width);
            break;
        case IR_LANG_TYPE_OPAQUE: {
            new_base = sv("opaque");
            break;
        }
        default:
            unreachable("");
    }

    assert(new_base.count > 0);
    Ir_name new_name = name_to_ir_name(name_new(
        MOD_PATH_BUILTIN,
        new_base,
        (Ulang_type_vec) {0},
        SCOPE_TOP_LEVEL,
        (Attrs) {0}
    ));
    assert(!strv_is_equal(new_name.base, sv("void")));
    assert(new_name.base.count > 0);
    return new_name;
}

static Ir_name ir_lang_type_primitive_get_name_c(Ir_lang_type_primitive ir_lang_type) {
    Strv new_base = {0};
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_FLOAT: {
            uint32_t bit_width = ir_lang_type_float_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 32) {
                new_base = sv("float");
            } else if (bit_width == 64) {
                new_base = sv("double");
            } else if (bit_width == 128) {
                new_base = sv("long double");
            } else {
                msg_todo("bit widths other than 32, 64, or 128 (for floating point numbers) with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
            }
            break;
        }
        case IR_LANG_TYPE_SIGNED_INT: {
            uint32_t bit_width = ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 1) {
                new_base = sv("bool");
            } else {
                if (bit_width == 1) {
                    todo();
                    // TODO: overflow may not happen correctly; maybe remove i1/u1 in earlier passes
                    new_base = sv("bool");
                } else if (bit_width == 8 || bit_width == 16 || bit_width == 32 || bit_width == 64) {
                    new_base = strv_from_f(&a_main, "int%d_t", bit_width);
                } else {
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 (for integers) with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
                }
            }
            break;
        }
        case IR_LANG_TYPE_UNSIGNED_INT: {
            // TODO: deduplicate this and above case?
            // TODO: bit width of 1 here?
            uint32_t bit_width = ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width;
            if (bit_width == 1) {
                new_base = sv("bool");
            } else if (bit_width == 8 || bit_width == 16 || bit_width == 32 || bit_width == 64) {
                new_base = strv_from_f(&a_main, "uint%d_t", bit_width);
            } else {
                msg_todo("bit widths other than 1, 8, 16, 32, or 64 with the c backend", ir_lang_type_primitive_get_pos(ir_lang_type));
            }
            break;
        }
        case IR_LANG_TYPE_OPAQUE:
            new_base = sv("void");
            break;
    }

    assert(new_base.count > 0);
    Ir_name new_name = name_to_ir_name(name_new(
        MOD_PATH_EXTERN_C,
        new_base,
        (Ulang_type_vec) {0},
        SCOPE_TOP_LEVEL,
        (Attrs) {0}
    ));
    assert(!strv_is_equal(new_name.base, sv("void")));
    assert(new_name.base.count > 0);
    return new_name;
}

Ir_name ir_lang_type_primitive_get_name(LANG_TYPE_MODE mode, Ir_lang_type_primitive ir_lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return ir_lang_type_primitive_get_name_normal(ir_lang_type);
        case LANG_TYPE_MODE_MSG:
            return ir_lang_type_primitive_get_name_normal(ir_lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            todo();
        case LANG_TYPE_MODE_EMIT_C:
            return ir_lang_type_primitive_get_name_c(ir_lang_type);
    }
    unreachable("");

}

bool ir_lang_type_get_name(Ir_name* result, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            *result = ir_lang_type_primitive_get_name(mode, ir_lang_type_primitive_const_unwrap(ir_lang_type));
            return true;
        case IR_LANG_TYPE_STRUCT:
            *result = ir_lang_type_struct_const_unwrap(ir_lang_type).name;
            return true;
        case IR_LANG_TYPE_TUPLE: {
            return false;
        }
        case IR_LANG_TYPE_FN: {
            return false;
        }
        case IR_LANG_TYPE_VOID: {
            *result = name_to_ir_name(name_new(
                MOD_PATH_BUILTIN,
                sv("void"),
                (Ulang_type_vec) {0},
                SCOPE_TOP_LEVEL,
                (Attrs) {0}
            ));
            return true;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

