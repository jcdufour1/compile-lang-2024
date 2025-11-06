#include <lang_type_print.h>
#include <lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>

void extend_lang_type_tag_to_string(String* buf, LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&a_temp, buf, "primitive");
            return;
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&a_temp, buf, "struct");
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&a_temp, buf, "raw_union");
            return;
        case LANG_TYPE_ENUM:
            string_extend_cstr(&a_temp, buf, "enum");
            return;
        case LANG_TYPE_TUPLE:
            string_extend_cstr(&a_temp, buf, "tuple");
            return;
        case LANG_TYPE_VOID:
            string_extend_cstr(&a_temp, buf, "void");
            return;
        case LANG_TYPE_FN:
            string_extend_cstr(&a_temp, buf, "fn");
            return;
        case LANG_TYPE_ARRAY:
            string_extend_cstr(&a_temp, buf, "array");
            return;
        case LANG_TYPE_REMOVED:
            string_extend_cstr(&a_temp, buf, "removed");
            return;
    }
    unreachable("");
}

Strv lang_type_vec_print_internal(Lang_type_vec types) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_lang_type_to_string(&buf, LANG_TYPE_MODE_MSG, vec_at(types, idx));
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

void extend_lang_type_atom(String* string, LANG_TYPE_MODE mode, Lang_type_atom atom) {
    Name temp = atom.str;

    if (atom.str.base.count > 1) {
        switch (mode) {
            case LANG_TYPE_MODE_LOG:
                extend_name(NAME_LOG, string, atom.str);
                break;
            case LANG_TYPE_MODE_MSG:
                extend_name(NAME_MSG, string, atom.str);
                break;
            case LANG_TYPE_MODE_EMIT_C:
                extend_name(NAME_EMIT_C, string, atom.str);
                break;
            case LANG_TYPE_MODE_EMIT_LLVM:
                extend_name(NAME_EMIT_IR, string, atom.str);
                break;
            default:
                unreachable("");
        }
    } else {
        unreachable("");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&a_temp, string, '*');
    }

    if (mode == LANG_TYPE_MODE_EMIT_LLVM) {
        if (temp.gen_args.info.count > 0) {
            todo();
        }
    }
}

Strv lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type) {
    String buf = {0};
    extend_lang_type_to_string(&buf, mode, lang_type);
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

Strv lang_type_atom_print_internal(Lang_type_atom atom, LANG_TYPE_MODE mode) {
    String buf = {0};
    extend_lang_type_atom(&buf, mode, atom);
    return string_to_strv(buf);
}

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '<');
    }

    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            extend_lang_type_tag_to_string(string, lang_type.type);
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

    if (lang_type.type == LANG_TYPE_PRIMITIVE) {
        unwrap(!strv_is_equal(lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type).str.base, sv("void")));
    }

    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, "(");
            }
            Lang_type_vec lang_types = lang_type_tuple_const_unwrap(lang_type).lang_types;
            for (size_t idx = 0; idx < lang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&a_main, string, ", ");
                }
                extend_lang_type_to_string(string, mode, vec_at(lang_types, idx));
            }
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, ")");
            }
            goto end;
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            string_extend_cstr(&a_main, string, "fn");
            extend_lang_type_to_string(string, mode, lang_type_tuple_const_wrap(fn.params));
            extend_lang_type_to_string(string, mode, *fn.return_type);
            goto end;
        }
        case LANG_TYPE_ARRAY: {
            Lang_type_array array = lang_type_array_const_unwrap(lang_type);
            extend_lang_type_to_string(string, mode, *array.item_type);
            string_extend_cstr(&a_temp, string, "[");
            string_extend_size_t(&a_temp, string, array.count);
            string_extend_cstr(&a_temp, string, "]");
            for (int16_t idx = 0; idx < array.pointer_depth; idx++) {
                vec_append(&a_temp, string, '*');
            }
            goto end;
        }
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_STRUCT:
            assert(!strv_is_equal(lang_type_get_atom(mode, lang_type).str.base, sv("void")));
        case LANG_TYPE_VOID:
            extend_lang_type_atom(string, mode, lang_type_get_atom(mode, lang_type));
            goto end;
        case LANG_TYPE_PRIMITIVE: {
            Strv base = lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type).base;
            //unwrap(base.count >= 4 && strv_slice(base, base.count - 4, 4));
            //log(LOG_DEBUG, FMT"\n", string_print(*string));
            extend_lang_type_atom(string, mode, lang_type_get_atom(mode, lang_type));
            //log(LOG_DEBUG, FMT"\n", string_print(*string));
            unwrap(base.count >= 1);
            goto end;
        }
        case LANG_TYPE_REMOVED:
            goto end;
    }
    unreachable("");

end:
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '>');
    }
}

