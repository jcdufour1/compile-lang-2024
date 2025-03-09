#include <resolve_generics.h>
#include <type_checking.h>
#include <lang_type_serialize.h>
#include <uast_serialize.h>
#include <ulang_type.h>
#include <uast_clone.h>

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

// we need to get the index of the original definition so that we can put resolved versions in the correct spot
#define try_set_def_types_internal(env, after_res, before_res, new_def) \
    do { \
        size_t idx = 0; \
        for (; idx < (env)->ancesters.info.count; idx++) { \
            Uast_def* result = NULL; \
            if (usym_tbl_lookup(&result, &vec_at(&(env)->ancesters, idx)->usymbol_table, (before_res)->base.name)) { \
                break; \
            } \
        } \
        usym_tbl_add(&vec_at(&(env)->ancesters, idx)->usymbol_table, (after_res)); \
        sym_tbl_add(&vec_at(&(env)->ancesters, idx)->symbol_table, (new_def)); \
        log(LOG_DEBUG, "thing fdlksaf: %zu\n", idx); \
    } while (0)

static bool try_set_struct_def_types(Env* env, Uast_struct_def* before_res, Uast_struct_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &after_res->base);
    try_set_def_types_internal(
        env,
        uast_struct_def_wrap(after_res),
        before_res,
        tast_struct_def_wrap(tast_struct_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_raw_union_def_types(Env* env, Uast_raw_union_def* before_res, Uast_raw_union_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &after_res->base);
    try_set_def_types_internal(
        env,
        uast_raw_union_def_wrap(after_res),
        before_res,
        tast_raw_union_def_wrap(tast_raw_union_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_enum_def_types(Env* env, Uast_enum_def* before_res, Uast_enum_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &after_res->base);
    try_set_def_types_internal(
        env,
        uast_enum_def_wrap(after_res),
        before_res,
        tast_enum_def_wrap(tast_enum_def_new(after_res->pos, new_base))
    );
    return success;
}

// TODO: inline this function?
static bool try_set_sum_def_types(Env* env, Uast_sum_def* before_res, Uast_sum_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &after_res->base);
    try_set_def_types_internal(
        env,
        uast_sum_def_wrap(after_res),
        before_res,
        tast_sum_def_wrap(tast_sum_def_new(after_res->pos, new_base))
    );
    return success;
}

static Str_view resolve_generics_serialize_struct_def_base(
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_vec gen_args,
    Pos pos
) {
    (void) pos;
    // TODO: figure out way to avoid making new Ustruct_def_base every time
    memset(new_base, 0, sizeof(*new_base));

    if (gen_args.info.count < 1) {
        *new_base = old_base;
        return new_base->name;
    }

    for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
        vec_append(&a_main, &new_base->members, uast_variable_def_clone(vec_at(&old_base.members, idx_memb)));
    }

    String name = {0};
    string_extend_strv(&a_main, &name, serialize_ulang_type(ustruct_def_base_get_lang_type(old_base)));
    for (size_t idx_gen = 0; idx_gen < gen_args.info.count; idx_gen++) {
        Str_view gen_def = vec_at(&old_base.generics, idx_gen)->child->name;
        for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
            Str_view memb = ulang_type_regular_const_unwrap(vec_at(&old_base.members, idx_memb)->lang_type).atom.str;
            if (str_view_is_equal(gen_def, memb)) {
                vec_at(&new_base->members, idx_memb)->lang_type = vec_at(&gen_args, idx_gen);
            }
        }
    }
    new_base->name = string_to_strv(name);
    return string_to_strv(name);
}

// TODO: deduplicate this and above function
static void resolve_generics_struct_def(
    Env* env,
    Uast_struct_def** new_def,
    const Uast_struct_def* old_def,
    Ulang_type_vec gen_args
) {
    Ustruct_def_base new_base = {0};
    Str_view name = resolve_generics_serialize_struct_def_base(&new_base, old_def->base, gen_args, old_def->pos);
    Uast_def* new_def_ = NULL;
    if (usymbol_lookup(&new_def_, env, name)) {
        todo();
        *new_def = uast_struct_def_unwrap(new_def_);
        return;
    }

    *new_def = uast_struct_def_new(old_def->pos, new_base);
    return;
    //log(LOG_DEBUG, TAST_FMT"\n", uast_struct_def_print(*new_def));
    //todo();
}

Ulang_type resolve_generics_ulang_type_reg_generic(Env* env, Ulang_type_reg_generic lang_type) {
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_reg_generic_const_wrap(lang_type)));
    Uast_def* result = NULL;
    unwrap(usymbol_lookup(&result, env, lang_type.atom.str));

    size_t def_count = uast_def_get_struct_def_base(result).generics.info.count;
    if (lang_type.generic_args.info.count != def_count) {
        log(
            LOG_DEBUG,
            "lang_type.generic_args.info.count: %zu; new_def->base.generics.info.count: %zu\n",
            lang_type.generic_args.info.count,
            def_count
        );
        // TODO: expected failure case
        unreachable("invalid count template args or parameters");
    }

    Uast_def* new_def = NULL;
    switch (result->type) {
        case UAST_SUM_DEF: {
            Ustruct_def_base new_base = {0};
            Str_view name = resolve_generics_serialize_struct_def_base(&new_base, uast_sum_def_unwrap(result)->base, lang_type.generic_args, uast_def_get_pos(result));
            Uast_def* new_def_ = NULL;
            if (usymbol_lookup(&new_def_, env, name)) {
                todo();
                //*new_def = uast_sum_def_unwrap(new_def_);
                //return;
            }

            new_def = uast_sum_def_wrap(uast_sum_def_new(uast_def_get_pos(result), new_base));
            break;
        }
        default:
            unreachable(TAST_FMT, uast_def_print(result));
    }

    return ustruct_def_base_get_lang_type(uast_def_get_struct_def_base(new_def));
}

Ulang_type resolve_generics_ulang_type_regular(Env* env, Ulang_type_regular lang_type) {
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(lang_type)));
    Uast_def* before_res = NULL;
    unwrap(usymbol_lookup(&before_res, env, lang_type.atom.str));
    assert(before_res);

    Uast_def* after_res = NULL;
    switch (before_res->type) {
        case UAST_SUM_DEF: {
            Ustruct_def_base new_base = {0};
            Str_view name = resolve_generics_serialize_struct_def_base(&new_base, uast_sum_def_unwrap(before_res)->base, (Ulang_type_vec) {0}, uast_def_get_pos(before_res));
            Uast_def* new_def_ = NULL;
            if (usymbol_lookup(&new_def_, env, name)) {
                after_res = new_def_;
            } else {
                after_res = uast_sum_def_wrap(uast_sum_def_new(uast_def_get_pos(before_res), new_base));
            }
            unwrap(try_set_sum_def_types(env, uast_sum_def_unwrap(before_res), uast_sum_def_unwrap(after_res)));
            break;
        }
        case UAST_STRUCT_DEF: {
            todo();
            Ustruct_def_base new_base = {0};
            Str_view name = resolve_generics_serialize_struct_def_base(&new_base, uast_struct_def_unwrap(before_res)->base, (Ulang_type_vec) {0}, uast_def_get_pos(before_res));
            Uast_def* new_def_ = NULL;
            if (usymbol_lookup(&new_def_, env, name)) {
                after_res = new_def_;
                break;
            }
            after_res = uast_struct_def_wrap(uast_struct_def_new(uast_def_get_pos(before_res), new_base));
            unwrap(try_set_struct_def_types(env, uast_struct_def_unwrap(before_res), uast_struct_def_unwrap(after_res)));
            break;
        }
        case UAST_PRIMITIVE_DEF:
            return ulang_type_regular_const_wrap(lang_type);
        case UAST_LITERAL_DEF:
            return ulang_type_regular_const_wrap(lang_type);
        default:
            unreachable(TAST_FMT, uast_def_print(before_res));
    }

    return ustruct_def_base_get_lang_type(uast_def_get_struct_def_base(after_res));
}

Ulang_type resolve_generics_ulang_type(Env* env, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return resolve_generics_ulang_type_regular(env, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_REG_GENERIC:
            return resolve_generics_ulang_type_reg_generic(env, ulang_type_reg_generic_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
    }
    todo();
}
