#include <lang_type_print.h>
#include <lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>
#include <ulang_type_print.h>

void extend_lang_type_tag_to_string(String* buf, LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&print_arena, buf, "primitive");
            return;
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&print_arena, buf, "struct");
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&print_arena, buf, "raw_union");
            return;
        case LANG_TYPE_ENUM:
            string_extend_cstr(&print_arena, buf, "enum");
            return;
        case LANG_TYPE_SUM:
            string_extend_cstr(&print_arena, buf, "sum");
            return;
        case LANG_TYPE_TUPLE:
            string_extend_cstr(&print_arena, buf, "tuple");
            return;
        case LANG_TYPE_VOID:
            string_extend_cstr(&print_arena, buf, "void");
            return;
        case LANG_TYPE_FN:
            string_extend_cstr(&print_arena, buf, "fn");
            return;
    }
    unreachable("");
}

// TODO: make separate .c file for these lang_type functions
Str_view lang_type_vec_print_internal(Lang_type_vec types) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, vec_at(&types, idx));
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

// TODO: remove this function entirely
//void extend_serialize_lang_type_to_string(Env* env, String* string, Lang_type lang_type, bool do_tag) {
//    if (do_tag) {
//        extend_lang_type_tag_to_string(string, lang_type.type);
//    }
//
//    if (lang_type.type == LANG_TYPE_TUPLE) {
//        Lang_type_vec lang_types = lang_type_tuple_const_unwrap(lang_type).lang_types;
//        for (size_t idx = 0; idx < lang_types.info.count; idx++) {
//            extend_serialize_lang_type_to_string(env, string, vec_at(&lang_types, idx), do_tag);
//        }
//    } else {
//        if (lang_type_get_str(lang_type).count > 1) {
//            string_extend_strv(&print_arena, string, serialize_lang_type(env, lang_type));
//        } else {
//            string_extend_cstr(&print_arena, string, "<null>");
//        }
//        if (lang_type_get_pointer_depth(lang_type) < 0) {
//            todo();
//        }
//        for (int16_t idx = 0; idx < lang_type_get_pointer_depth(lang_type); idx++) {
//            vec_append(&print_arena, string, '*');
//        }
//    }
//}

void extend_lang_type_atom(String* string, LANG_TYPE_MODE mode, Lang_type_atom atom) {
    Ulang_type_generic deserialized = {0};
    if (deserialize_generic(&deserialized, atom.str)) {
        extend_ulang_type_to_string(string, mode, ulang_type_generic_const_wrap(deserialized));
        return;
    }

    if (atom.str.count > 1) {
        string_extend_strv(&print_arena, string, atom.str);
    } else {
        string_extend_cstr(&print_arena, string, "void");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&print_arena, string, '*');
    }
}

Str_view lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type) {
    String buf = {0};
    extend_lang_type_to_string(&buf, mode, lang_type);
    switch (mode) {
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_LOG:
            string_extend_cstr(&print_arena, &buf, "\n");
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

Str_view lang_type_atom_print_internal(Lang_type_atom atom, LANG_TYPE_MODE mode) {
    String buf = {0};
    // TODO: do not use `lang_type_primitive_new` here
    extend_lang_type_atom(&buf, mode, atom);
    return string_to_strv(buf);
}

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '<');
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
        default:
            unreachable("");
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
                extend_lang_type_to_string(string, mode, vec_at(&lang_types, idx));
            }
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, ")");
            }
            goto end;
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            extend_lang_type_to_string(string, mode, lang_type_tuple_const_wrap(fn.params));
            extend_lang_type_to_string(string, mode, *fn.return_type);
            goto end;
        }
        case LANG_TYPE_VOID:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_PRIMITIVE:
            extend_lang_type_atom(string, mode, lang_type_get_atom(lang_type));
            goto end;
    }
    unreachable("");

end:
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '>');
    }
}

