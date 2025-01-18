#ifndef SERIALIZE_H
#define SERIALIZE_H

static inline Str_view serialize_struct_def(const Tast_struct_def* struct_def) {
    String name = {0};
    for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
        Lang_type curr = vec_at(&struct_def->base.members, idx)->lang_type;
        string_extend_size_t(&a_main, &name, lang_type_get_pointer_depth(curr));
        string_extend_cstr(&a_main, &name, "_");
        string_extend_size_t(&a_main, &name, lang_type_get_str(curr).count);
        string_extend_cstr(&a_main, &name, "_");
        string_extend_strv(&a_main, &name, lang_type_get_str(curr));
        string_extend_cstr(&a_main, &name, "__");
    }
    return string_to_strv(name);
}

static inline Str_view serialize_lang_type(Lang_type lang_type) {
    // TODO: think about memory allocation strategy here
    // TODO: make function to serialize lang_type, and call it from here
    // TODO: separate file for serialization functions
    //
    //
    // TODO: factor in lang_type.type into serialization
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE: {
            String name = {0};
            for (size_t idx = 0; idx < lang_type_unwrap_tuple_const(lang_type).lang_types.info.count; idx++) {
                Lang_type curr = vec_at_const(lang_type_unwrap_tuple_const(lang_type).lang_types, idx);
                string_extend_strv(&a_main, &name, serialize_lang_type(curr));
            }
            return string_to_strv(name);
        }
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_RAW_UNION: {
            String name = {0};
            string_extend_size_t(&a_main, &name, lang_type_get_pointer_depth(lang_type));
            string_extend_cstr(&a_main, &name, "_");
            string_extend_size_t(&a_main, &name, lang_type_get_str(lang_type).count);
            string_extend_cstr(&a_main, &name, "_");
            string_extend_strv(&a_main, &name, lang_type_get_str(lang_type));
            string_extend_cstr(&a_main, &name, "__");
            return string_to_strv(name);
        }
        case LANG_TYPE_VOID:
            todo();
    }
    unreachable("");
}

static inline Str_view serialize_def(const Tast_def* def) {
    switch (def->type) {
        case TAST_STRUCT_DEF:
            return serialize_struct_def(tast_unwrap_struct_def_const(def));
        case TAST_FUNCTION_DEF:
            unreachable("");
        case TAST_FUNCTION_DECL:
            unreachable("");
        case TAST_VARIABLE_DEF:
            unreachable("");
        case TAST_RAW_UNION_DEF:
            unreachable("");
        case TAST_ENUM_DEF:
            unreachable("");
        case TAST_SUM_DEF:
            unreachable("");
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}


#endif // SERIALIZE_H
