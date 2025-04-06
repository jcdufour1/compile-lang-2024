#ifndef SERIALIZE_MODULE_SYMBOL_NAME_H
#define SERIALIZE_MODULE_SYMBOL_NAME_H
// TODO: rename this header file

#include <util.h>
#include <parser_utils.h>

// TODO: remove this
bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

static inline Str_view serialize_name(Name name) {
    String buf = {0};

    string_extend_size_t(&a_main, &buf, name.mod_path.count);
    string_extend_cstr(&a_main, &buf, "_");
    string_extend_strv(&a_main, &buf, name.mod_path);
    string_extend_cstr(&a_main, &buf, "_");
    string_extend_strv(&a_main, &buf, name.base);

    string_extend_cstr(&a_main, &buf, "____");
    string_extend_size_t(&a_main, &buf, name.gen_args.info.count);
    string_extend_cstr(&a_main, &buf, "_");
    for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
        string_extend_strv(&a_main, &buf, serialize_name(serialize_ulang_type(env, env->curr_mod_path, vec_at(&gen_args, idx))));
    }

    return string_to_strv(buf);
}

static inline Name deserialize_name(Str_view serialized) {
    size_t mod_len = 0;
    unwrap(try_str_view_consume_size_t(&mod_len, &serialized, false));
    unwrap(str_view_try_consume(&serialized, '_'));
    Str_view mod_path = str_view_consume_count(&serialized, mod_len);

    unwrap(str_view_try_consume(&serialized, '_'));
    Str_view base_name = serialized;

    return (Name) {.mod_path = mod_path, .base = base_name};
}

#endif // SERIALIZE_MODULE_SYMBOL_NAME_H
