// TODO: get rid of all functions in this file if they will not be used and bug fixed
#include <lang_type_serialize.h>

Strv serialize_struct_def_base(Struct_def_base base) {
    String name = {0};
    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Lang_type curr = vec_at(base.members, idx)->lang_type;
        string_extend_strv(&a_main, &name, serialize_lang_type(curr));
        //string_extend_size_t(&a_main, &name, lang_type_get_pointer_depth(curr));
        //string_extend_cstr(&a_main, &name, "_");
        //string_extend_size_t(&a_main, &name, lang_type_get_str(curr).count);
        //string_extend_cstr(&a_main, &name, "_");
        //string_extend_strv(&a_main, &name, lang_type_get_str(curr));
    }
    return string_to_strv(name);
}

Strv serialize_lang_type_get_prefix(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_RAW_UNION:
            return sv("RAW_UNION");
        case LANG_TYPE_STRUCT:
            return sv("STRUCT");
        case LANG_TYPE_ENUM:
            return sv("ENUM");
        case LANG_TYPE_PRIMITIVE:
            return sv("PRI");
        case LANG_TYPE_TUPLE:
            todo();
        case LANG_TYPE_VOID:
            return sv("VOID");
        case LANG_TYPE_FN:
            return sv("FN");
        case LANG_TYPE_ARRAY:
            return sv("ARRAY");
        case LANG_TYPE_REMOVED:
            return sv("REMOVED");
    }
    unreachable("");
}

Strv serialize_lang_type_struct_thing(Lang_type lang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_lang_type_get_prefix(lang_type));

    Tast_def* def = NULL;
    unwrap(symbol_lookup(&def,  lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)));
    string_extend_strv(&a_main, &name, serialize_struct_def_base(tast_def_get_struct_def_base(def)));

    return string_to_strv(name);
}

Strv serialize_lang_type_tuple(Lang_type_tuple lang_type) {
    String name = {0};
    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        Lang_type curr = vec_at(lang_type.lang_types, idx);
        string_extend_strv(&a_main, &name, serialize_lang_type(curr));
    }
    return string_to_strv(name);
}

Strv serialize_lang_type_fn(Lang_type_fn lang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_lang_type_tuple(lang_type.params));
    string_extend_strv(&a_main, &name, serialize_lang_type(*lang_type.return_type));
    return string_to_strv(name);
}

Strv serialize_lang_type_array(Lang_type_array lang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_lang_type(*lang_type.item_type));
    string_extend_size_t(&a_main, &name, lang_type.count);
    return string_to_strv(name);
}

// TODO: make separate function for Tast_lang_type and Lang_type
Strv serialize_lang_type(Lang_type lang_type) {
    // TODO: think about memory allocation strategy here
    // TODO: make function to serialize lang_type, and call it from here
    //
    //
    // TODO: factor in lang_type.type into serialization
    switch (lang_type.type) {
        case LANG_TYPE_FN:
            return serialize_lang_type_fn(lang_type_fn_const_unwrap(lang_type));
        case LANG_TYPE_TUPLE:
            return serialize_lang_type_tuple(lang_type_tuple_const_unwrap(lang_type));
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            return serialize_lang_type_struct_thing(lang_type);
        case LANG_TYPE_VOID: {
            String name = {0};
            string_extend_cstr(&a_main, &name, "void");
            return string_to_strv(name);
        }
        case LANG_TYPE_ARRAY:
            return serialize_lang_type_array(lang_type_array_const_unwrap(lang_type));
        case LANG_TYPE_REMOVED:
            unreachable("lang_type_removed should not be serialized");
    }
    unreachable("");
}

// TODO: remove this function?
Lang_type deserialize_lang_type(Strv* serialized) {
    log(LOG_DEBUG, FMT"\n", strv_print(*serialized));
    todo();
}
