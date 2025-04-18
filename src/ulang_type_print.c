#include <ulang_type_print.h>
#include <extend_name.h>

Str_view ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type) {
    String buf = {0};
    extend_ulang_type_to_string(&buf, mode, lang_type);
    if (mode == LANG_TYPE_MODE_LOG) {
        string_extend_cstr(&print_arena, &buf, "\n");
    }
    return string_to_strv(buf);
}

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&print_arena, buf, "(( line:");
    string_extend_int64_t(&print_arena, buf, pos.line);
    string_extend_cstr(&print_arena, buf, " ))");
}

void extend_ulang_type_atom_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_atom atom) {
    // TODO: remove?
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '<');
    }

    if (atom.str.base.count > 1) {
        extend_uname(string, atom.str);
    } else {
        string_extend_cstr(&print_arena, string, "<null>");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&print_arena, string, '*');
    }

    // TODO: remove?
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '>');
    }
    return;
}

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            extend_ulang_type_atom_to_string(string, mode, ulang_type_regular_const_unwrap(lang_type).atom);
            return;
        case ULANG_TYPE_TUPLE: {
            vec_append(&print_arena, string, '(');
            Ulang_type_tuple tuple = ulang_type_tuple_const_unwrap(lang_type);
            for (size_t idx = 0; idx < tuple.ulang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&print_arena, string, ", ");
                }
                extend_ulang_type_to_string(string, mode, vec_at(&tuple.ulang_types, idx));
            }
            vec_append(&print_arena, string, ')');
            return;
        }
        case ULANG_TYPE_FN: {
            string_extend_cstr(&a_main, string, "fn");
            Ulang_type_fn fn = ulang_type_fn_const_unwrap(lang_type);
            extend_ulang_type_to_string(string, mode, ulang_type_tuple_const_wrap(fn.params));
            extend_ulang_type_to_string(string, mode, *fn.return_type);
            return;
        }
    }
    unreachable("");
}

