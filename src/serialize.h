#ifndef SERIALIZE_H
#define SERIALIZE_H

static inline Str_view serialize_lang_type(const Env* env, Lang_type lang_type);

static inline Str_view serialize_struct_def_base(const Env* env, Struct_def_base base) {
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

static inline Str_view serialize_struct_def(const Env* env, const Tast_struct_def* struct_def) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_struct_def_base(env, struct_def->base));
    return string_to_strv(name);
}

static inline Str_view serialize_sum_def(const Env* env, const Tast_sum_def* sum_def) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_struct_def_base(env, sum_def->base));
    return string_to_strv(name);
}

static inline Str_view serialize_lang_type_struct_thing(const Env* env, Lang_type lang_type) {
    String name = {0};

    Struct_def_base base = {0};
    switch (lang_type.type) {
        case LANG_TYPE_STRUCT: {
            Tast_def* def = NULL;
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            base = tast_struct_def_unwrap(def)->base;
            break;
        }
        case LANG_TYPE_SUM: {
            Tast_def* def = NULL;
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            base = tast_sum_def_unwrap(def)->base;
            break;
        }
        case LANG_TYPE_RAW_UNION: {
            Tast_def* def = NULL;
            log(LOG_DEBUG, "%zu\n", env->ancesters.info.count);
            log(LOG_DEBUG, TAST_FMT, lang_type_print(lang_type));
            log(LOG_DEBUG, TAST_FMT"\n", str_view_print(lang_type_get_str(lang_type)));
            try(symbol_lookup(&def, env, lang_type_get_str(lang_type)));
            log(LOG_DEBUG, TAST_FMT, tast_def_print(def));
            base = tast_raw_union_def_unwrap(def)->base;
            break;
        }
        default:
            unreachable("");
    }

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Tast_variable_def* curr = vec_at(&base.members, idx);
        string_extend_strv(&a_main, &name, serialize_lang_type(env, curr->lang_type));
    }

    return string_to_strv(name);
}

// TODO: make separate function for Tast_lang_type and Lang_type
// TODO: call serialize_struct_def serialize_tast_struct_def, etc.
static inline Str_view serialize_lang_type(const Env* env, Lang_type lang_type) {
    // TODO: think about memory allocation strategy here
    // TODO: make function to serialize lang_type, and call it from here
    // TODO: separate file for serialization functions
    //
    //
    // TODO: factor in lang_type.type into serialization
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE: {
            String name = {0};
            for (size_t idx = 0; idx < lang_type_tuple_const_unwrap(lang_type).lang_types.info.count; idx++) {
                Lang_type curr = vec_at_const(lang_type_tuple_const_unwrap(lang_type).lang_types, idx);
                string_extend_strv(&a_main, &name, serialize_lang_type(env, curr));
            }
            return string_to_strv(name);
        }
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
        case LANG_TYPE_VOID:
            todo();
    }
    unreachable("");
}

static inline Str_view serialize_def(const Env* env, const Tast_def* def) {
    switch (def->type) {
        case TAST_STRUCT_DEF:
            return serialize_struct_def(env, tast_struct_def_const_unwrap(def));
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
            return serialize_sum_def(env, tast_sum_def_const_unwrap(def));
        case TAST_PRIMITIVE_DEF:
            unreachable("");
        case TAST_LITERAL_DEF:
            unreachable("");
    }
    unreachable("");
}


#endif // SERIALIZE_H
