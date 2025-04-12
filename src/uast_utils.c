#include <uast_utils.h>
#include <extend_name.h>

bool uast_def_get_lang_type(Lang_type* result, Env* env, const Uast_def* def, Ulang_type_vec generics) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            return try_lang_type_from_ulang_type(result, env, uast_variable_def_const_unwrap(def)->lang_type, ulang_type_get_pos(uast_variable_def_const_unwrap(def)->lang_type));
        case UAST_FUNCTION_DECL:
            *result = lang_type_from_ulang_type(env, uast_function_decl_const_unwrap(def)->return_type);
            return true;
        case UAST_PRIMITIVE_DEF:
            *result = uast_primitive_def_const_unwrap(def)->lang_type;
            return true;
        case UAST_LITERAL_DEF:
            unreachable("");
        case UAST_STRUCT_DEF:
            // fallthrough
        case UAST_RAW_UNION_DEF:
            // fallthrough
        case UAST_ENUM_DEF:
            // fallthrough
        case UAST_SUM_DEF: {
            Ulang_type ulang_type = {0};
            if (!ustruct_def_base_get_lang_type_(&ulang_type, env, uast_def_get_struct_def_base(def), generics, uast_def_get_pos(def))) {
                return false;
            }
            *result = lang_type_from_ulang_type(env, ulang_type);
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
    }
    unreachable("");
}

bool ustruct_def_base_get_lang_type_(Ulang_type* result, Env* env, Ustruct_def_base base, Ulang_type_vec gen_args, Pos pos) {
    Name base_name = base.name;
    base_name.gen_args = gen_args;
    return resolve_generics_ulang_type_regular(result, env, ulang_type_regular_new(ulang_type_atom_new(base_name, 0), pos));
}
