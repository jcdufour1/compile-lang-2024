#include <name.h>
#include <util.h>
#include <parser_utils.h>
#include <ulang_type_serialize.h>
#include <ulang_type_clone.h>

Name name_new(Strv mod_path, Strv base, Ulang_type_vec gen_args, Scope_id scope_id) {
    return (Name) {.mod_path = mod_path, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

Uname uname_new(Name mod_alias, Strv base, Ulang_type_vec gen_args, Scope_id scope_id) {
    assert(mod_alias.base.count > 0);
    return (Uname) {.mod_alias = mod_alias, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

void extend_name_ir(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name(name));
}

void extend_name_c(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name(name));
}

void serialize_strv(String* buf, Strv strv) {
    string_extend_cstr(&a_main, buf, "_");
    string_extend_size_t(&a_main, buf, strv.count);
    string_extend_cstr(&a_main, buf, "_");
    string_extend_strv(&a_main, buf, strv);
    string_extend_cstr(&a_main, buf, "_");
}

// TODO: merge serialize_name_symbol_table and serialize_name to be consistant with ulang_type?
Strv serialize_name_symbol_table(Name name) {
    String buf = {0};

    String new_mod_path = {0};
    for (size_t idx = 0; idx < name.mod_path.count; idx++) {
        if (strv_at(name.mod_path, idx) == '.') {
            vec_append(&a_main, &new_mod_path, '_');
        } else {
            vec_append(&a_main, &new_mod_path, strv_at(name.mod_path, idx));
        }
    }
    name.mod_path = string_to_strv(new_mod_path);

    if (name.mod_path.count > 0) {
        size_t path_count = 1;
        {
            Strv mod_path = name.mod_path;
            Strv dummy = {0};
            while (strv_try_consume_until(&dummy, &mod_path, PATH_SEPARATOR)) {
                unwrap(strv_try_consume(&mod_path, PATH_SEPARATOR));
                path_count++;
            }
        }

        string_extend_cstr(&a_main, &buf, "_");
        string_extend_size_t(&a_main, &buf, path_count);
        string_extend_cstr(&a_main, &buf, "_");

        {
            Strv mod_path = name.mod_path;
            Strv dir_name = {0};
            while (strv_try_consume_until(&dir_name, &mod_path, PATH_SEPARATOR)) {
                unwrap(strv_try_consume(&mod_path, PATH_SEPARATOR));
                serialize_strv(&buf, dir_name);
            }
            serialize_strv(&buf, mod_path);
        }
    }

    string_extend_strv(&a_main, &buf, name.base);

    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_main, &buf, "____");
        string_extend_size_t(&a_main, &buf, name.gen_args.info.count);
        string_extend_cstr(&a_main, &buf, "_");
        for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
            string_extend_strv(&a_main, &buf, serialize_name_symbol_table(serialize_ulang_type(name.mod_path, vec_at(&name.gen_args, idx), false)));
        }
    }

    return string_to_strv(buf);
}

Strv serialize_name(Name name) {
    String buf = {0};

    if (name.scope_id > 0) {
        string_extend_cstr(&a_main, &buf, "s");
        string_extend_size_t(&a_main, &buf, name.scope_id);
        string_extend_cstr(&a_main, &buf, "_");
    }

    string_extend_strv(&a_main, &buf, serialize_name_symbol_table(name));

    return string_to_strv(buf);
}

// TODO: move this macro
Strv name_print_internal(NAME_MODE mode, bool serialize, Name name) {
    if (serialize) {
        return serialize_name(name);
    }
        
    String buf = {0};
    extend_name(mode, &buf, name);
    return string_to_strv(buf);
}

Strv uname_print_internal(UNAME_MODE mode, Uname name) {
    String buf = {0};
    extend_uname(mode, &buf, name);
    return string_to_strv(buf);
}

void extend_name_log_internal(bool is_msg, String* buf, Name name) {
    if (!is_msg) {
        string_extend_cstr(&a_print, buf, "s");
        string_extend_size_t(&a_print, buf, name.scope_id);
        string_extend_cstr(&a_print, buf, "_");
    }

    string_extend_strv(&a_print, buf, name.mod_path);
    if (name.mod_path.count > 0) {
        string_extend_cstr(&a_print, buf, "::");
    }
    string_extend_strv(&a_print, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_print, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_print, buf, ", ");
        }
        string_extend_strv(&a_print, buf, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_print, buf, ">)");
    }
}

void extend_name_msg(String* buf, Name name) {
    extend_name_log_internal(true, buf, name);
}

// TODO: move this function elsewhere
// TODO: move this function elsewhere
void extend_uname(UNAME_MODE mode, String* buf, Uname name) {
    extend_name(mode == UNAME_MSG ? NAME_MSG : NAME_LOG, buf, name.mod_alias);
    if (name.mod_alias.base.count > 0 || (mode != UNAME_MSG && name.mod_alias.scope_id > 0)) {
        string_extend_cstr(&a_print, buf, ".");
    }
    string_extend_strv(&a_print, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_print, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_print, buf, ", ");
        }
        string_extend_strv(&a_print, buf, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_print, buf, ">)");
    }
}

void extend_name(NAME_MODE mode, String* buf, Name name) {
    switch (mode) {
        case NAME_MSG:
            extend_name_log_internal(true, buf, name);
            return;
        case NAME_LOG:
            extend_name_log_internal(false, buf, name);
            return;
        case NAME_EMIT_C:
            extend_name_ir(buf, name);
            return;
        case NAME_EMIT_IR:
            extend_name_ir(buf, name);
            return;
    }
    unreachable("");
}

Name name_clone(Name name, Scope_id new_scope) {
    return name_new(name.mod_path, name.base, ulang_type_vec_clone(name.gen_args, new_scope), new_scope);
}

Uname uname_clone(Uname name, Scope_id new_scope) {
    return uname_new(name.mod_alias, name.base, ulang_type_vec_clone(name.gen_args, new_scope), new_scope);
}
