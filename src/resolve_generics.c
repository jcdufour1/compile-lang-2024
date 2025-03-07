#include <resolve_generics.h>
#include <type_checking.h>
#include <lang_type_serialize.h>
#include <uast_serialize.h>

//static Str_view resolve_generics_serialize_struct_def_base(Struct_def_base base) {
//    String name = {0};
//
//    for (size_t idx = 0; idx < base.members.info.count; idx++) {
//        string_extend_strv(&name, serialize_ulang_type(env, vec_at(&base.members, idx)->lang_type));
//    }
//
//    return name;
//}

static bool try_set_struct_base_types(Env* env, Struct_def_base* new_base, Ustruct_def_base* base) {
    env->type_checking_is_in_struct_base_def = true;
    bool success = true;
    Tast_variable_def_vec new_members = {0};

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_variable_def* curr = vec_at(&base->members, idx);

        Tast_variable_def* new_memb = NULL;
        if (try_set_variable_def_types(env, &new_memb, curr, false, false)) {
            vec_append(&a_main, &new_members, new_memb);
        } else {
            success = false;
        }
    }

    *new_base = (Struct_def_base) {
        .members = new_members,
        .name = base->name
    };

    env->type_checking_is_in_struct_base_def = false;
    return success;
}

static bool try_set_raw_union_def_types(Env* env, Uast_raw_union_def* uast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &uast->base);
    unwrap(symbol_add(env, tast_raw_union_def_wrap(tast_raw_union_def_new(uast->pos, new_base))));
    return success;
}

static bool try_set_struct_def_types(Env* env, Uast_struct_def* uast) {
    Uast_def* dummy = NULL;
    assert(usymbol_lookup(&dummy, env, uast->base.name));
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &uast->base);
    unwrap(symbol_add(env, tast_struct_def_wrap(tast_struct_def_new(uast->pos, new_base))));
    return success;
}

static bool try_set_enum_def_types(Env* env, Uast_enum_def* tast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &tast->base);
    Tast_enum_def* new_def = tast_enum_def_new(tast->pos, new_base);
    unwrap(symbol_add(env, tast_enum_def_wrap(new_def)));
    return success;
}

static bool try_set_sum_def_types(Env* env, Uast_sum_def* tast) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &tast->base);
    Tast_sum_def* new_def = tast_sum_def_new(tast->pos, new_base);
    unwrap(symbol_add(env, tast_sum_def_wrap(new_def)));
    return success;
}

static void resolve_generics_struct_def_base(
    Env* env,
    //Ulang_type lang_type_surface_level,
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_vec gen_args
) {
    (void) env;
    (void) new_base;
    (void) old_base;

    String name = {0};
    //string_extend_strv(&a_main, &name, serialize_ulang_type(lang_type_surface_level));

    string_extend_strv(&a_main, &name, serialize_ulang_type(ustruct_def_base_get_lang_type(old_base)));

    for (size_t idx = 0; idx < gen_args.info.count; idx++) {
        string_extend_strv(&a_main, &name, serialize_ulang_type(vec_at(&gen_args, idx)));
    }
    log(LOG_DEBUG, TAST_FMT"\n", string_print(name));
    todo();
}

static void resolve_generics_sum_def(
    Env* env,
    Uast_sum_def** new_def,
    const Uast_sum_def* old_def,
    Ulang_type_vec gen_args
) {
    (void) new_def;
    Ustruct_def_base new_base = {0};
    resolve_generics_struct_def_base(env, &new_base, old_def->base, gen_args);
    todo();
}

Ulang_type resolve_generics_ulang_type_reg_generic(Env* env, Ulang_type_reg_generic lang_type) {
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_reg_generic_const_wrap(lang_type)));
    Uast_def* result = NULL;
    unwrap(usymbol_lookup(&result, env, lang_type.atom.str));

    Uast_sum_def* new_def = NULL;
    switch (result->type) {
        case UAST_SUM_DEF:
            resolve_generics_sum_def(env, &new_def, uast_sum_def_unwrap(result), lang_type.generic_args);
            break;
        default:
            unreachable(TAST_FMT, uast_def_print(result));
    }

    if (lang_type.generic_args.info.count != new_def->base.members.info.count) {
        // TODO: expected failure case
        unreachable("invalid count template args or parameters");
    }
    todo();
}

