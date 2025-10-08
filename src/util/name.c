#include <name.h>
#include <util.h>
#include <ulang_type_serialize.h>
#include <ulang_type_clone.h>
#include <str_and_num_utils.h>
#include <uast.h>

Name name_new(Strv mod_path, Strv base, Ulang_type_vec gen_args, Scope_id scope_id) {
    return (Name) {.mod_path = mod_path, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

// this function will convert `io.i32` to `i32`, etc.
static Uname uname_normalize(Uname name) {
    Uast_def* dummy = NULL;
    Name possible_new = name_new(MOD_PATH_BUILTIN, name.base, name.gen_args, name.scope_id);
    if (usymbol_lookup(&dummy, possible_new)) {
        return name_to_uname(possible_new);
    }
    return name;
}

static Uname uname_new_internal(Name mod_alias, Strv base, Ulang_type_vec gen_args, Scope_id scope_id) {
    assert(mod_alias.base.count > 0);
    return (Uname) {.mod_alias = mod_alias, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

Uname name_to_uname(Name name) {
    Uast_mod_alias* new_alias = uast_mod_alias_new(
        POS_BUILTIN,
        util_literal_name_new(),
        name.mod_path,
        name.scope_id
    );
    unwrap(usymbol_add(uast_mod_alias_wrap(new_alias)));
    return uname_new_internal(new_alias->name, name.base, name.gen_args, name.scope_id);
}

Uname uname_new(Name mod_alias, Strv base, Ulang_type_vec gen_args, Scope_id scope_id) {
    assert(mod_alias.base.count > 0);
    return uname_normalize(uname_new_internal(mod_alias, base, gen_args, scope_id));
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
            // NOTE: even though ulang_types are used for generic arguments, mod_aliases are not actually used,
            //   so there is no need to switch to using Lang_type for generic arguents
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

    if (!is_msg) {
        // TODO: even when is_msg is true, show prefix sometimes to avoid confusion?
        //   (maybe only show prefix when mod_path of name is different than mod_path of file that 
        //   name appears in)
        string_extend_strv(&a_print, buf, name.mod_path);
        if (name.mod_path.count > 0) {
            string_extend_cstr(&a_print, buf, "::");
        }
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

void extend_uname(UNAME_MODE mode, String* buf, Uname name) {
    log(LOG_DEBUG, FMT"\n", strv_print(name.mod_alias.mod_path));
    if (
        mode != UNAME_MSG || !(
            strv_is_equal(name.mod_alias.mod_path, MOD_PATH_BUILTIN /* TODO */) ||
            name_is_equal(name.mod_alias, MOD_ALIAS_BUILTIN) ||
            name_is_equal(name.mod_alias, MOD_ALIAS_TOP_LEVEL)
        )
    ) {
        extend_name(mode == UNAME_MSG ? NAME_MSG : NAME_LOG, buf, name.mod_alias);
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

Name name_clone(Name name, bool use_new_scope, Scope_id new_scope) {
    Scope_id scope = use_new_scope ? new_scope : name.scope_id;
    return name_new(name.mod_path, name.base, ulang_type_vec_clone(name.gen_args, use_new_scope, new_scope), scope);
}

Uname uname_clone(Uname name, bool use_new_scope, Scope_id new_scope) {
    Scope_id scope = use_new_scope ? new_scope : name.scope_id;
    return uname_new(name.mod_alias, name.base, ulang_type_vec_clone(name.gen_args, use_new_scope, new_scope), scope);
}

bool name_is_equal(Name a, Name b) {
    if (!strv_is_equal(a.mod_path, b.mod_path) || !strv_is_equal(a.base, b.base)) {
        return false;
    }

    if (a.gen_args.info.count != b.gen_args.info.count) {
        return false;
    }
    for (size_t idx = 0; idx < a.gen_args.info.count; idx++) {
        if (!ulang_type_is_equal(vec_at(&a.gen_args, idx), vec_at(&b.gen_args, idx))) {
            return false;
        }
    }

    return true;
}

bool uname_is_equal(Uname a, Uname b) {
    Name new_a = {0};
    if (!name_from_uname(&new_a, a, POS_BUILTIN /* TODO */)) {
        return false;
    }
    Name new_b = {0};
    if (!name_from_uname(&new_b, b, POS_BUILTIN /* TODO */)) {
        return false;
    }
    return name_is_equal(new_a, new_b);
}
