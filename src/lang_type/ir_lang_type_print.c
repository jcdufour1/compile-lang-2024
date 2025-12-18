#include <ir_lang_type_print.h>
#include <ir_lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>
#include <ast_msg.h>

void extend_ir_lang_type_tag_to_string(String* buf, IR_LANG_TYPE_TYPE type) {
    switch (type) {
        case IR_LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&a_temp, buf, "primitive");
            return;
        case IR_LANG_TYPE_STRUCT:
            string_extend_cstr(&a_temp, buf, "struct");
            return;
        case IR_LANG_TYPE_TUPLE:
            string_extend_cstr(&a_temp, buf, "tuple");
            return;
        case IR_LANG_TYPE_VOID:
            string_extend_cstr(&a_temp, buf, "void");
            return;
        case IR_LANG_TYPE_FN:
            string_extend_cstr(&a_temp, buf, "fn");
            return;
    }
    unreachable("");
}

Strv ir_lang_type_vec_print_internal(Ir_lang_type_vec types) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, vec_at(types, idx));
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

Strv ir_lang_type_print_internal(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    String buf = {0};
    extend_ir_lang_type_to_string(&buf, mode, ir_lang_type);
    switch (mode) {
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_EMIT_C:
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_LOG:
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

// TODO: make this an extern function?
static NAME_MODE lang_type_mode_to_name_mode(LANG_TYPE_MODE mode) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return NAME_LOG;
        case LANG_TYPE_MODE_MSG:
            return NAME_MSG;
        case LANG_TYPE_MODE_EMIT_LLVM:
            return NAME_EMIT_C;
        case LANG_TYPE_MODE_EMIT_C:
            return NAME_EMIT_IR;
    }
    unreachable("");
}

// TODO: add arena argument?
void extend_ir_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '<');
    }

    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            extend_ir_lang_type_tag_to_string(string, ir_lang_type.type);
            string_extend_cstr(&a_main, string, " ");
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_EMIT_C:
            break;
        default:
            unreachable("");
    }

    // TODO: remove gotos in below switch
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_TUPLE:
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, "(");
            }
            Ir_lang_type_vec ir_lang_types = ir_lang_type_tuple_const_unwrap(ir_lang_type).ir_lang_types;
            for (size_t idx = 0; idx < ir_lang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&a_main, string, ", ");
                }
                extend_ir_lang_type_to_string(string, mode, vec_at(ir_lang_types, idx));
            }
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, ")");
            }
            goto end;
        case IR_LANG_TYPE_FN: {
            Ir_lang_type_fn fn = ir_lang_type_fn_const_unwrap(ir_lang_type);
            string_extend_f(&a_main, string, "%sfn", fn.pointer_depth > 1 ? "(" : "");
            extend_ir_lang_type_to_string(string, mode, ir_lang_type_tuple_const_wrap(fn.params));
            extend_ir_lang_type_to_string(string, mode, *fn.return_type);
            if (fn.pointer_depth > 1) {
                vec_append(&a_temp, string, ')');
            }
            assert(fn.pointer_depth > 0);
            for (int16_t idx = 1/*TODO*/; idx < fn.pointer_depth; idx++) {
                vec_append(&a_temp, string, '*');
            }
            goto end;
        }
        case IR_LANG_TYPE_STRUCT: {
            Ir_name name = {0};
            if (!ir_lang_type_get_name(&name, ir_lang_type)) {
                msg_todo("", ir_lang_type_get_pos(ir_lang_type));
                goto end;
            }
            extend_ir_name(lang_type_mode_to_name_mode(mode), string, name);
            for (int16_t idx = 0; idx < ir_lang_type_get_pointer_depth(ir_lang_type); idx++) {
                vec_append(&a_temp, string, '*');
            }
            goto end;
        }
        case IR_LANG_TYPE_VOID: {
            Ir_name name = {0};
            if (!ir_lang_type_get_name(&name, ir_lang_type)) {
                msg_todo("", ir_lang_type_get_pos(ir_lang_type));
                break;
            }

            extend_ir_name(lang_type_mode_to_name_mode(mode), string, name);
            goto end;
        }
        case IR_LANG_TYPE_PRIMITIVE: {
            Ir_name name = {0};
            if (!ir_lang_type_get_name(&name, ir_lang_type)) {
                msg_todo("", ir_lang_type_get_pos(ir_lang_type));
                break;
            }
            if (mode == LANG_TYPE_MODE_LOG) {
                assert(strv_is_equal(name.mod_path, MOD_PATH_BUILTIN));
            }
            assert(name.base.count >= 1);
            extend_ir_name(lang_type_mode_to_name_mode(mode), string, name);
            for (int16_t idx = 0; idx < ir_lang_type_get_pointer_depth(ir_lang_type); idx++) {
                vec_append(&a_temp, string, '*');
            }
        }
            goto end;
    }
    unreachable("");

end:
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '>');
    }
}

