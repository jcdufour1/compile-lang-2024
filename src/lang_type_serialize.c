#include <lang_type_serialize.h>

Str_view serialize_struct_def_base(Env* env, Struct_def_base base) {
    String name = {0};
    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Lang_type curr = vec_at(&base.members, idx)->lang_type;
        string_extend_strv(&a_main, &name, serialize_lang_type(env, curr));
        //string_extend_size_t(&a_main, &name, lang_type_get_pointer_depth(curr));
        //string_extend_cstr(&a_main, &name, "_");
        //string_extend_size_t(&a_main, &name, lang_type_get_str(curr).count);
        //string_extend_cstr(&a_main, &name, "_");
        //string_extend_strv(&a_main, &name, lang_type_get_str(curr));
    }
    return string_to_strv(name);
}

Str_view serialize_lang_type_struct_thing_get_prefix(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_RAW_UNION:
            return str_view_from_cstr("RAW_UNION");
        case LANG_TYPE_STRUCT:
            return str_view_from_cstr("STRUCT");
        case LANG_TYPE_ENUM:
            return str_view_from_cstr("ENUM");
        case LANG_TYPE_SUM:
            return str_view_from_cstr("SUM");
        default:
            todo();
    }
    unreachable("");
}

Str_view serialize_lang_type_struct_thing(Env* env, Lang_type lang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_lang_type_struct_thing_get_prefix(lang_type));

    Tast_def* def = NULL;
    try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
    string_extend_strv(&a_main, &name, serialize_struct_def_base(env, tast_def_get_struct_def_base(def)));

    return string_to_strv(name);
}

Str_view serialize_lang_type_tuple(Env* env, Lang_type_tuple lang_type) {
    String name = {0};
    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        Lang_type curr = vec_at_const(lang_type.lang_types, idx);
        string_extend_strv(&a_main, &name, serialize_lang_type(env, curr));
    }
    return string_to_strv(name);
}

Str_view serialize_lang_type_fn(Env* env, Lang_type_fn lang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_lang_type_tuple(env, lang_type.params));
    string_extend_strv(&a_main, &name, serialize_lang_type(env, *lang_type.return_type));
    return string_to_strv(name);
}

// TODO: make separate function for Tast_lang_type and Lang_type
Str_view serialize_lang_type(Env* env, Lang_type lang_type) {
    // TODO: think about memory allocation strategy here
    // TODO: make function to serialize lang_type, and call it from here
    //
    //
    // TODO: factor in lang_type.type into serialization
    switch (lang_type.type) {
        case LANG_TYPE_FN: {
            return serialize_lang_type_fn(env, lang_type_fn_const_unwrap(lang_type));
        }
        case LANG_TYPE_TUPLE:
            return serialize_lang_type_tuple(env, lang_type_tuple_const_unwrap(lang_type));
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_ENUM: {
            String name = {0};
            string_extend_size_t(&a_main, &name, lang_type_get_pointer_depth(lang_type));
            string_extend_cstr(&a_main, &name, "_");
            string_extend_size_t(&a_main, &name, lang_type_get_str(lang_type).count);
            string_extend_cstr(&a_main, &name, "_");
            string_extend_strv(&a_main, &name, lang_type_get_str(lang_type));
            string_extend_cstr(&a_main, &name, "__");
            return string_to_strv(name);
        }
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            return serialize_lang_type_struct_thing(env, lang_type);
        case LANG_TYPE_VOID: {
            String name = {0};
            string_extend_cstr(&a_main, &name, "void");
            return string_to_strv(name);
        }
    }
    unreachable("");
}
