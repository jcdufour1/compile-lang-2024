#include <uast_utils.h>

bool uast_def_get_lang_type(Lang_type* result, const Uast_def* def, Ulang_type_vec generics) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            return try_lang_type_from_ulang_type(result,  uast_variable_def_const_unwrap(def)->lang_type, ulang_type_get_pos(uast_variable_def_const_unwrap(def)->lang_type));
        case UAST_FUNCTION_DECL:
            *result = lang_type_from_ulang_type( uast_function_decl_const_unwrap(def)->return_type);
            return true;
        case UAST_PRIMITIVE_DEF:
            *result = uast_primitive_def_const_unwrap(def)->lang_type;
            return true;
        case UAST_STRUCT_DEF:
            // fallthrough
        case UAST_RAW_UNION_DEF:
            // fallthrough
        case UAST_ENUM_DEF: {
            Ulang_type ulang_type = {0};
            if (!ustruct_def_base_get_lang_type_(&ulang_type,  uast_def_get_struct_def_base(def), generics, uast_def_get_pos(def))) {
                return false;
            }
            *result = lang_type_from_ulang_type( ulang_type);
            return true;
        }
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("");
    }
    unreachable("");
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Ustruct_def_base base, Ulang_type_vec gen_args, Pos pos) {
    Uname base_name = name_to_uname(base.name);
    base_name.gen_args = gen_args;
    LANG_TYPE_TYPE type = {0};
    return resolve_generics_ulang_type_regular(&type, result, ulang_type_regular_new(ulang_type_atom_new(base_name, 0), pos));
}

// TODO: remove
Ulang_type uast_get_ulang_type_def(const Uast_def* def) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_RAW_UNION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            return uast_variable_def_const_unwrap(def)->lang_type;
        case UAST_FUNCTION_DECL:
            return uast_function_decl_const_unwrap(def)->return_type;
        case UAST_STRUCT_DEF:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(name_to_uname(uast_struct_def_const_unwrap(def)->base.name), 0), uast_def_get_pos(def)));
        case UAST_PRIMITIVE_DEF:
            unreachable("");
        case UAST_ENUM_DEF:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(name_to_uname(uast_enum_def_const_unwrap(def)->base.name), 0), uast_def_get_pos(def)));
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("");
    }
    unreachable("");
}

Ulang_type ulang_type_from_uast_function_decl(const Uast_function_decl* decl) {
    Ulang_type_vec params = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        vec_append(&a_main, &params, vec_at(&decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* return_type = arena_alloc(&a_main, sizeof(*return_type));
    *return_type = decl->return_type;
    Ulang_type_fn fn = ulang_type_fn_new(ulang_type_tuple_new(params, decl->pos), return_type, decl->pos);
    return ulang_type_fn_const_wrap(fn);
}
