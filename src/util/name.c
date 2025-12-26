#include <name.h>
#include <util.h>
#include <ulang_type_serialize.h>
#include <ulang_type_clone.h>
#include <str_and_num_utils.h>
#include <uast.h>
#include <ulang_type_is_equal.h>
#include <symbol_iter.h>

Name name_new_quick(Strv mod_path, Strv base, Scope_id scope_id) {
    return (Name) {.mod_path = mod_path, .base = base, .gen_args = (Ulang_type_darr) {0}, .scope_id = scope_id};
}

// TODO: replace genrgs in src with gen_args
Name name_new(Strv mod_path, Strv base, Ulang_type_darr gen_args, Scope_id scope_id) {
    return (Name) {.mod_path = mod_path, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

Ir_name ir_name_new(Strv mod_path, Strv base, Ulang_type_darr gen_args, Scope_id scope_id) {
    return name_to_ir_name(name_new(mod_path, base, gen_args, scope_id));
}

// this function will convert `io.i32` to `i32`, etc.
static Uname uname_normalize(Uname name) {
    if (lang_type_name_base_is_number(name.base)) {
        name.mod_alias = MOD_ALIAS_BUILTIN;
    }
    return name;
}

static Uname uname_new_internal(Name mod_alias, Strv base, Ulang_type_darr gen_args, Scope_id scope_id) {
    unwrap(mod_alias.base.count > 0);
    assert(base.count > 0);
    return (Uname) {.mod_alias = mod_alias, .base = base, .gen_args = gen_args, .scope_id = scope_id};
}

Uname name_to_uname(Name name) {
    if (lang_type_name_base_is_number(name.base), 0) {
        return uname_new_internal(MOD_ALIAS_BUILTIN, name.base, name.gen_args, name.scope_id);
    }

    Name alias_name = name_new(MOD_PATH_AUX_ALIASES, name.mod_path, (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL);
#   ifndef NDEBUG
        // TODO: uncomment this code (before uncommenting code, it may be nessessary to fix issue of runtime not being autoimported when prelude is disabled)
        //Uast_def* dummy = NULL;
        //if (!usymbol_lookup(&dummy, alias_name)) {
        //    log(LOG_FATAL, FMT"\n", name_print(NAME_LOG, alias_name));
        //    unreachable("");
        //}
#   endif // NDEBUG

    return uname_new_internal(alias_name, name.base, name.gen_args, name.scope_id);
}

Name ir_name_to_name(Ir_name name) {
    if (name.scope_id == SCOPE_NOT) {
        static_assert(sizeof(name) == sizeof(Ir_name), "the type punning below will probably not work anymore");
        return *(Name*)&name;
    }

    Ir_name_to_name_table_node* result = NULL;
    unwrap(
        ir_name_to_name_lookup(&result, name) &&
        "\"ir_name_new\" function should not be used directly; use \"name_to_ir_name(name_new(\" instead"
    );
    return result->name_regular;
}

// TODO: Attrs should be stored in hash tables instead of in Name and Ir_name?
Ir_name name_to_ir_name(Name name) {
    if (name.scope_id == SCOPE_NOT) {
        static_assert(sizeof(name) == sizeof(Ir_name), "the type punning below will probably not work anymore");
        return *(Ir_name*)&name;
    }

    Name_to_ir_name_table_node* result = NULL;
    if (name_to_ir_name_lookup(&result, name)) {
        return result->ir_name;
    }

    if (name.scope_id == SCOPE_TOP_LEVEL) {
        Ir_name ir_name = *(Ir_name*)&name;
        static_assert(sizeof(name) == sizeof(ir_name), "the type punning above will probably not work anymore");
        unwrap(ir_name_to_name_add((Ir_name_to_name_table_node) {.name_self = ir_name, .name_regular = name}));
        unwrap(name_to_ir_name_add((Name_to_ir_name_table_node) {.name_self = name, .ir_name = ir_name}));
        return ir_name;
    }

    Scope_id curr = name.scope_id;
    Scope_id prev = SCOPE_NOT;
    while (curr != SCOPE_TOP_LEVEL) {
        prev = curr;
        curr = scope_get_parent_tbl_lookup(curr);
    }
    assert(prev != SCOPE_TOP_LEVEL && prev != SCOPE_NOT);

    Name ir_name_ = util_literal_name_new();
    Ir_name ir_name = *(Ir_name*)&ir_name_;
    static_assert(sizeof(ir_name_) == sizeof(ir_name), "the type punning above will probably not work anymore");
    ir_name.scope_id = prev;
    assert(ir_name.scope_id != SCOPE_TOP_LEVEL);
    assert(name.scope_id != SCOPE_TOP_LEVEL);
    unwrap(ir_name_to_name_add((Ir_name_to_name_table_node) {.name_self = ir_name, .name_regular = name}));
    unwrap(name_to_ir_name_add((Name_to_ir_name_table_node) {.name_self = name, .ir_name = ir_name}));
    return ir_name;
}

Uname uname_new(Name mod_alias, Strv base, Ulang_type_darr gen_args, Scope_id scope_id) {
    unwrap(mod_alias.base.count > 0);
    return uname_normalize(uname_new_internal(mod_alias, base, gen_args, scope_id));
}

void extend_name_ir(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name(name));
}

void extend_name_c(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name(name));
}

// TODO: improve naming of serialize_strv_actual and serialize_strv
void serialize_strv_actual(String* buf, Strv strv) {
    string_extend_cstr(&a_main, buf, "_");
    string_extend_size_t(&a_main, buf, strv.count);
    string_extend_cstr(&a_main, buf, "_");
    // TODO: refactor this for loop into a separate function
    for (size_t idx = 0; idx < strv.count; idx++) {
        char curr = strv.str[idx];

        if (isalnum(curr)) {
            darr_append(&a_main, buf, curr);
        } else if (curr == '_') {
            string_extend_cstr(&a_main, buf, "__");
        } else {
            string_extend_f(&a_main, buf, "_%02x", curr);
        }
    }
    string_extend_cstr(&a_main, buf, "_");
}

void serialize_strv(String* buf, Strv strv) {
    string_extend_cstr(&a_main, buf, "_");
    string_extend_size_t(&a_main, buf, strv.count);
    string_extend_cstr(&a_main, buf, "_");
    string_extend_strv(&a_main, buf, strv);
    string_extend_cstr(&a_main, buf, "_");
}

// TODO: merge serialize_name_symbol_table and serialize_name to be consistant with ulang_type?
// TODO: deduplicate serialize_name_symbol_table and serialize_ir_name_symbol_table?
// TODO: this function seems confusing and needlessly complex
Strv serialize_name_symbol_table(Arena* arena, Name name) {
    // TODO: remove this if body, and instead assert that a and b mod_path always == MOD_PATH_BUILTIN
    //   if it is required to?
    if (lang_type_name_base_is_number(name.base)) {
        name.mod_path = MOD_PATH_BUILTIN;
    }

    String buf = {0};

    String new_mod_path = {0};
    for (size_t idx = 0; idx < name.mod_path.count; idx++) {
        if (strv_at(name.mod_path, idx) == '.') {
            darr_append(arena, &new_mod_path, '_');
        } else {
            darr_append(arena, &new_mod_path, strv_at(name.mod_path, idx));
        }
    }
    name.mod_path = string_to_strv(new_mod_path);

    if (name.mod_path.count > 0) {
        size_t path_count = 1;
        {
            Strv mod_path = name.mod_path;
            Strv dummy = {0};
            while (strv_try_consume_until(&dummy, &mod_path, PATH_SEP)) {
                unwrap(strv_try_consume(&mod_path, PATH_SEP));
                path_count++;
            }
        }

        string_extend_cstr(arena, &buf, "_");
        string_extend_size_t(arena, &buf, path_count);
        string_extend_cstr(arena, &buf, "_");

        {
            Strv mod_path = name.mod_path;
            Strv dir_name = {0};
            while (strv_try_consume_until(&dir_name, &mod_path, PATH_SEP)) {
                unwrap(strv_try_consume(&mod_path, PATH_SEP));
                serialize_strv(&buf, dir_name);
            }
            serialize_strv(&buf, mod_path);
        }
    }

    string_extend_strv(arena, &buf, name.base);

    if (name.gen_args.info.count > 0) {
        string_extend_cstr(arena, &buf, "____");
        string_extend_size_t(arena, &buf, name.gen_args.info.count);
        string_extend_cstr(arena, &buf, "_");
        for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
            // NOTE: even though ulang_types are used for generic arguments, mod_aliases are not actually used,
            //   so there is no need to switch to using Lang_type for generic arguents
            string_extend_strv(
                arena,
                &buf,
                serialize_name_symbol_table(
                    &a_main,
                    serialize_ulang_type(name.mod_path, darr_at(name.gen_args, idx), false)
                )
            );
        }
    }

    return string_to_strv(buf);
}

Strv serialize_ir_name_symbol_table(Arena* arena, Ir_name name) {
    static_assert(sizeof(name) == sizeof(Name), "type punning below might not work anymore");
    return serialize_name_symbol_table(arena, *(Name*)&name);
}

Strv serialize_name(Name name) {
    String buf = {0};

    if (name.scope_id > 0) {
        string_extend_cstr(&a_main, &buf, "s");
        string_extend_size_t(&a_main, &buf, name.scope_id);
        string_extend_cstr(&a_main, &buf, "_");
    }

    string_extend_strv(&a_main, &buf, serialize_name_symbol_table(&a_main, name));

    return string_to_strv(buf);
}

Strv name_print_internal(NAME_MODE mode, bool serialize, Name name) {
    if (serialize) {
        return serialize_name(name);
    }
        
    String buf = {0};
    extend_name(mode, &buf, name);
    return string_to_strv(buf);
}

Strv ir_name_print_internal(NAME_MODE mode, bool serialize, Ir_name name) {
    if (serialize) {
        todo();
        static_assert(sizeof(Name) == sizeof(Ir_name), "this type punning below will no longer work");
        return serialize_name(*(Name*)&name);
    }
        
    String buf = {0};
    extend_ir_name(mode, &buf, name);
    return string_to_strv(buf);
}

Strv uname_print_internal(UNAME_MODE mode, Uname name) {
    String buf = {0};
    extend_uname(mode, &buf, name);
    return string_to_strv(buf);
}

void extend_name_log_internal(bool is_msg, String* buf, Name name) {
    if (!is_msg) {
        string_extend_cstr(&a_temp, buf, "s");
        string_extend_size_t(&a_temp, buf, name.scope_id);
        string_extend_cstr(&a_temp, buf, "_");
    }

    assert(!is_msg || env.mod_path_curr_file.count > 0);
    if (!is_msg || !strv_is_equal(name.mod_path, env.mod_path_curr_file)) {
        if (!is_msg || !strv_is_equal(name.mod_path, MOD_PATH_BUILTIN)) {
            if (is_msg) {
                Scope_id curr_scope = name.scope_id;
                while (1) {
                    // TODO: consider if mod_aliases should be cached to prevent this nested while loop on every name_print(NAME_MSG, ...)
                    Usymbol_iter iter = usym_tbl_iter_new(curr_scope);
                    Uast_def* curr = NULL;
                    while (usym_tbl_iter_next(&curr, &iter)) {
                        if (curr->type != UAST_MOD_ALIAS) {
                            continue;
                        }
                        const Uast_mod_alias* mod_alias = uast_mod_alias_const_unwrap(curr);
                        if (!mod_alias->is_actually_mod_alias) {
                            continue;
                        }
                        if (!strv_is_equal(mod_alias->name.mod_path, env.mod_path_curr_file)) {
                            continue;
                        }
                        if (strv_is_equal(mod_alias->mod_path, name.mod_path)) {
                            string_extend_f(&a_temp, buf, FMT".", strv_print(mod_alias->name.base));
                            goto after_print_path;
                        }
                    }

                    if (curr_scope == SCOPE_TOP_LEVEL) {
                        break;
                    }
                    curr_scope = scope_get_parent_tbl_lookup(curr_scope);
                }
            }

            string_extend_strv(&a_temp, buf, name.mod_path);
            if (name.mod_path.count > 0) {
                string_extend_cstr(&a_temp, buf, "::");
            }
        }
    }
after_print_path:

    string_extend_strv(&a_temp, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_temp, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, buf, ", ");
        }
        string_extend_strv(&a_temp, buf, ulang_type_print_internal(is_msg ? LANG_TYPE_MODE_MSG : LANG_TYPE_MODE_LOG, darr_at(name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_temp, buf, ">)");
    }
}

void extend_name_msg(String* buf, Name name) {
    extend_name_log_internal(true, buf, name);
}

void extend_uname(UNAME_MODE mode, String* buf, Uname name) {
    if (
        mode != UNAME_MSG || !(
            strv_is_equal(name.mod_alias.mod_path, MOD_PATH_BUILTIN /* TODO */) ||
            strv_is_equal(name.mod_alias.mod_path, MOD_PATH_AUX_ALIASES) ||
            name_is_equal(name.mod_alias, MOD_ALIAS_BUILTIN) ||
            name_is_equal(name.mod_alias, MOD_ALIAS_TOP_LEVEL)
        )
    ) {
        extend_name(mode == UNAME_MSG ? NAME_MSG : NAME_LOG, buf, name.mod_alias);
        string_extend_cstr(&a_temp, buf, ".");
    }
    if (mode == UNAME_LOG) {
        // TODO: uncomment below when possible to allow for better debugging?
        //string_extend_f(&a_temp, buf, "s"SIZE_T_FMT"_", name.scope_id);
    }
    string_extend_strv(&a_temp, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_temp, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_temp, buf, ", ");
        }
        string_extend_strv(&a_temp, buf, ulang_type_print_internal(mode == UNAME_MSG ? LANG_TYPE_MODE_MSG : LANG_TYPE_MODE_LOG, darr_at(name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_temp, buf, ">)");
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

void extend_ir_name(NAME_MODE mode, String* buf, Ir_name name) {
    static_assert(sizeof(name) == sizeof(Name), "type punning below might not work anymore");
    switch (mode) {
        case NAME_MSG:
            extend_name(mode, buf, ir_name_to_name(name));
            return;
        case NAME_LOG:
            extend_name(mode, buf, *(Name*)&name);
            string_extend_cstr(&a_temp, buf, "(");
            extend_name(mode, buf, ir_name_to_name(name));
            string_extend_cstr(&a_temp, buf, ")");
            return;
        case NAME_EMIT_C:
            fallthrough;
        case NAME_EMIT_IR:
            extend_name(mode, buf, *(Name*)&name);
            return;
    }
    unreachable("");
}

Name name_clone(Name name, bool use_new_scope, Scope_id new_scope) {
    Scope_id scope = use_new_scope ? new_scope : name.scope_id;
    return name_new(name.mod_path, name.base, ulang_type_darr_clone(name.gen_args, use_new_scope, new_scope), scope);
}

Uname uname_clone(Uname name, bool use_new_scope, Scope_id new_scope) {
    Scope_id scope = use_new_scope ? new_scope : name.scope_id;
    return uname_new(name.mod_alias, name.base, ulang_type_darr_clone(name.gen_args, use_new_scope, new_scope), scope);
}

bool ir_name_is_equal(Ir_name a, Ir_name b) {
    static_assert(sizeof(a) == sizeof(Name) && sizeof(b) == sizeof(Name), "type punning below might not work anymore");
    return name_is_equal(*(Name*)&a, *(Name*)&b);
}

bool name_is_equal(Name a, Name b) {
    if (!strv_is_equal(a.mod_path, b.mod_path) || !strv_is_equal(a.base, b.base)) {
        return false;
    }

    if (a.gen_args.info.count != b.gen_args.info.count) {
        return false;
    }
    for (size_t idx = 0; idx < a.gen_args.info.count; idx++) {
        if (!ulang_type_is_equal(darr_at(a.gen_args, idx), darr_at(b.gen_args, idx))) {
            return false;
        }
    }

    return true;
}

bool uname_is_equal(Uname a, Uname b) {
    Name new_a = {0};
    if (!name_from_uname(&new_a, a, POS_BUILTIN)) {
        todo();
        return false;
    }
    Name new_b = {0};
    if (!name_from_uname(&new_b, b, POS_BUILTIN)) {
        todo();
        return false;
    }

    // TODO: remove this if body, and instead assert that a and b mod_path always == MOD_PATH_BUILTIN
    //   if it is required to?
    if (lang_type_name_base_is_number(new_a.base)) {
        new_a.mod_path = MOD_PATH_BUILTIN;
    }
    if (lang_type_name_base_is_number(new_b.base)) {
        new_b.mod_path = MOD_PATH_BUILTIN;
    }

    return name_is_equal(new_a, new_b);
}
