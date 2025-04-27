#ifndef SERIALIZE_MODULE_SYMBOL_NAME_H
#define SERIALIZE_MODULE_SYMBOL_NAME_H
// TODO: rename this header file

#include <util.h>
#include <parser_utils.h>
#include <ulang_type_serialize.h>
#include <extend_name.h>

static inline void serialize_str_view(String* buf, Str_view str_view) {
    string_extend_cstr(&a_main, buf, "_");
    string_extend_size_t(&a_main, buf, str_view.count);
    string_extend_cstr(&a_main, buf, "_");
    string_extend_strv(&a_main, buf, str_view);
    string_extend_cstr(&a_main, buf, "_");
}

// TODO: remove this
bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

static inline Str_view serialize_name(Name name) {
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
static inline Str_view name_print_internal(bool serialize, Name name) {
    if (serialize) {
        return serialize_name( name);
    }
        
    String buf = {0};
    extend_name( false, &buf, name);
    return string_to_strv(buf);
}

static inline Str_view uname_print_internal(Uname name) {
    String buf = {0};
    extend_uname(&buf, name);
    return string_to_strv(buf);
}

#endif // SERIALIZE_MODULE_SYMBOL_NAME_H
