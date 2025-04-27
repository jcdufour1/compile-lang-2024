#include <name.h>
#include <util.h>
#include <parser_utils.h>
#include <ulang_type_serialize.h>
#include <extend_name.h>
#include <ulang_type_clone.h>

void extend_name_llvm(String* buf, Name name) {
    string_extend_strv(&a_main, buf, serialize_name(name));
}

void serialize_str_view(String* buf, Str_view str_view) {
    string_extend_cstr(&a_main, buf, "_");
    string_extend_size_t(&a_main, buf, str_view.count);
    string_extend_cstr(&a_main, buf, "_");
    string_extend_strv(&a_main, buf, str_view);
    string_extend_cstr(&a_main, buf, "_");
}

// TODO: remove this
bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

Str_view serialize_name(Name name) {
    String buf = {0};

    if (name.mod_path.count > 0) {
        size_t path_count = 1;
        {
            Str_view mod_path = name.mod_path;
            Str_view dummy = {0};
            while (str_view_try_consume_until(&dummy, &mod_path, PATH_SEPARATOR)) {
                unwrap(str_view_try_consume(&mod_path, PATH_SEPARATOR));
                path_count++;
            }
        }

        string_extend_cstr(&a_main, &buf, "_");
        string_extend_size_t(&a_main, &buf, path_count);
        string_extend_cstr(&a_main, &buf, "_");

        {
            Str_view mod_path = name.mod_path;
            Str_view dir_name = {0};
            while (str_view_try_consume_until(&dir_name, &mod_path, PATH_SEPARATOR)) {
                unwrap(str_view_try_consume(&mod_path, PATH_SEPARATOR));
                serialize_str_view(&buf, dir_name);
            }
            serialize_str_view(&buf, mod_path);
        }
    }

    string_extend_strv(&a_main, &buf, name.base);

    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_main, &buf, "____");
        string_extend_size_t(&a_main, &buf, name.gen_args.info.count);
        string_extend_cstr(&a_main, &buf, "_");
        for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
            string_extend_strv(&a_main, &buf, serialize_name( serialize_ulang_type( name.mod_path, vec_at(&name.gen_args, idx))));
        }
    }

    return string_to_strv(buf);
}

// TODO: move this macro
Str_view name_print_internal(bool serialize, Name name) {
    if (serialize) {
        return serialize_name( name);
    }
        
    String buf = {0};
    extend_name( false, &buf, name);
    return string_to_strv(buf);
}

Str_view uname_print_internal(Uname name) {
    String buf = {0};
    extend_uname(&buf, name);
    return string_to_strv(buf);
}

void extend_name_msg(String* buf, Name name) {
    string_extend_strv(&print_arena, buf, name.mod_path);
    if (name.mod_path.count > 0) {
        string_extend_cstr(&print_arena, buf, "::");
    }
    string_extend_strv(&print_arena, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&print_arena, buf, ", ");
        }
        string_extend_strv(&print_arena, buf, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, ">)");
    }
}

void extend_uname_msg(String* buf, Uname name) {
    extend_name(false, buf, name.mod_alias);
    if (name.mod_alias.base.count > 0) {
        string_extend_cstr(&print_arena, buf, ".");
    }
    string_extend_strv(&print_arena, buf, name.base);
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, "(<");
    }
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&print_arena, buf, ", ");
        }
        string_extend_strv(&print_arena, buf, ulang_type_print_internal(LANG_TYPE_MODE_MSG, vec_at(&name.gen_args, idx)));
    }
    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&print_arena, buf, ">)");
    }
}

// TODO: move this function elsewhere
// TODO: move this function elsewhere
void extend_uname(String* buf, Uname name) {
    extend_uname_msg(buf, name);
}

void extend_name(bool is_llvm, String* buf, Name name) {
    if (is_llvm) {
        extend_name_llvm( buf, name);
        return;
    }
    extend_name_msg(buf, name);
    return;
}

Name name_clone(Name name) {
    return (Name) {.mod_path = name.mod_path, .base = name.base, .gen_args = ulang_type_vec_clone(name.gen_args)};
}
