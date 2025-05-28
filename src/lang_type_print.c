#include <lang_type_print.h>
#include <lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>

void extend_lang_type_tag_to_string(String* buf, LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&a_print, buf, "primitive");
            return;
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&a_print, buf, "struct");
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&a_print, buf, "raw_union");
            return;
        case LANG_TYPE_ENUM:
            string_extend_cstr(&a_print, buf, "enum");
            return;
        case LANG_TYPE_TUPLE:
            string_extend_cstr(&a_print, buf, "tuple");
            return;
        case LANG_TYPE_VOID:
            string_extend_cstr(&a_print, buf, "void");
            return;
        case LANG_TYPE_FN:
            string_extend_cstr(&a_print, buf, "fn");
            return;
    }
    unreachable("");
}

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
                extend_name(NAME_EMIT_LLVM, string, atom.str);
                break;
            default:
                unreachable("");
        }
    } else {
        string_extend_cstr(&a_print, string, "void");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&a_print, string, '*');
    }

    if (mode == LANG_TYPE_MODE_EMIT_LLVM) {
        if (temp.gen_args.info.count > 0) {
            todo();
        }
    }
}

Str_view lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type) {
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
            string_extend_cstr(&a_print, &buf, "\n");
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

Str_view lang_type_atom_print_internal(Lang_type_atom atom, LANG_TYPE_MODE mode) {
    String buf = {0};
    extend_lang_type_atom(&buf, mode, atom);
    return string_to_strv(buf);
}

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_print, string, '<');
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

    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            log(LOG_DEBUG, "thing thing tuple\n");
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
            log(LOG_DEBUG, "thing thing fn\n");
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            string_extend_cstr(&a_main, string, "fn");
            extend_lang_type_to_string(string, mode, lang_type_tuple_const_wrap(fn.params));
            extend_lang_type_to_string(string, mode, *fn.return_type);
            goto end;
        }
        case LANG_TYPE_ENUM:
            log(LOG_DEBUG, "thing 2.2 other\n");
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            log(LOG_DEBUG, "thing 2.3 other\n");
            // fallthrough
        case LANG_TYPE_STRUCT:
            log(LOG_DEBUG, "thing 2.4 other\n");
            // fallthrough
            assert(!str_view_cstr_is_equal(lang_type_get_atom(mode, lang_type).str.base, "void"));
        case LANG_TYPE_VOID:
            log(LOG_DEBUG, "thing 2.1 other\n");
            // fallthrough
        case LANG_TYPE_PRIMITIVE:
            log(LOG_DEBUG, "thing 2.5 other\n");
            log(LOG_DEBUG, "thing thing other\n");
            log(LOG_DEBUG, TAST_FMT"\n", lang_type_atom_print(LANG_TYPE_MODE_LOG, lang_type_get_atom(mode, lang_type)));
            extend_lang_type_atom(string, mode, lang_type_get_atom(mode, lang_type));
            goto end;
    }
    unreachable("");

end:
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_print, string, '>');
    }
}

