#include <resolve_generics.h>
#include <type_checking.h>
#include <lang_type_serialize.h>
#include <ulang_type_serialize.h>
#include <uast_serialize.h>
#include <ulang_type.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <ulang_type_get_pos.h>
#include <ulang_type_get_atom.h>
#include <generic_sub.h>
#include <symbol_log.h>

static bool ulang_type_generics_are_present(Ulang_type lang_type);

Name serialize_generic(Env* env, Name old_name, Ulang_type_vec gen_args) {
    String name = {0};
    string_extend_cstr(&a_main, &name, "____");
    string_extend_size_t(&a_main, &name, gen_args.info.count);
    string_extend_cstr(&a_main, &name, "_");
    // TODO: serialize str_view, here, not atom
    string_extend_strv(&a_main, &name, serialize_ulang_type_atom(ulang_type_atom_new(old_name, 0)));
    for (size_t idx = 0; idx < gen_args.info.count; idx++) {
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(env, env->curr_mod_path, vec_at(&gen_args, idx))));
    }
    return (Name) {.mod_path = old_name.mod_path, .base = string_to_strv(name)};
}

bool deserialize_generic(Ulang_type_generic* deserialized, int16_t pointer_depth, Name* serialized) {
    (void) deserialized;
    (void) pointer_depth;
    (void) serialized;
    todo();
    ////log(LOG_DEBUG, TAST_FMT"\n", str_view_print(*serialized));
    //if (!str_view_try_consume_count(serialized, '_', 4)) { // for now, ____ means generic
    //    // not a generic lang_type
    //    return false;
    //}

    ////log(LOG_DEBUG, TAST_FMT"\n", str_view_print(*serialized));

    //size_t count_gen = 0;
    //unwrap(try_str_view_consume_size_t(&count_gen, serialized, false));
    //unwrap(str_view_try_consume(serialized, '_'));

    //Ulang_type_atom atom = deserialize_ulang_type_atom(serialized);
    //atom.pointer_depth = pointer_depth;

    //Ulang_type_vec gen_args = {0};
    //for (size_t idx = 0; idx < count_gen; idx++) {
    //    Ulang_type gen_arg = deserialize_ulang_type(serialized, pointer_depth);
    //    vec_append(&a_main, &gen_args, gen_arg);
    //}

    //*deserialized = ulang_type_generic_new(atom, gen_args, POS_BUILTIN);
    //return true;
}

#define msg_invalid_count_generic_args(env, pos_def, pos_gen_args, gen_args, min_args, max_args) \
    msg_invalid_count_generic_args_internal(__FILE__, __LINE__, env, pos_def, pos_gen_args, gen_args, min_args, max_args)

// TODO: move this function and macro to better place
static void msg_undefined_type_internal(
    const char* file,
    int line,
    Env* env,
    Pos pos,
    Ulang_type lang_type
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_path_to_text, pos,
        "type `"LANG_TYPE_FMT"` is not defined\n", ulang_type_print(LANG_TYPE_MODE_MSG, lang_type)
    );
}

#define msg_undefined_type(env, pos, lang_type) \
    msg_undefined_type_internal(__FILE__, __LINE__, env, pos, lang_type)

static void msg_invalid_count_generic_args_internal(
    const char* file,
    int line,
    Env* env,
    Pos pos_def,
    Pos pos_gen_args,
    Ulang_type_vec gen_args,
    size_t min_args,
    size_t max_args
) {
    String message = {0};
    string_extend_size_t(&print_arena, &message, gen_args.info.count);
    string_extend_cstr(&print_arena, &message, " generic arguments are passed");
    string_extend_cstr(&print_arena, &message, ", but ");
    string_extend_size_t(&print_arena, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&print_arena, &message, " or more");
    }
    string_extend_cstr(&print_arena, &message, " generic arguments expected\n");
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_GENERIC_ARGS, env->file_path_to_text, pos_gen_args,
        STR_VIEW_FMT, str_view_print(string_to_strv(message))
    );

    msg_internal(
        file, line, LOG_NOTE, EXPECT_FAIL_NONE, env->file_path_to_text, pos_def,
        "generic parameters defined here\n" 
    );
}

static bool try_set_struct_base_types(Env* env, Struct_def_base* new_base, Ustruct_def_base* base, bool is_enum) {
    env->type_checking_is_in_struct_base_def = true;
    bool success = true;
    Tast_variable_def_vec new_members = {0};

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_variable_def* curr = vec_at(&base->members, idx);

        if (is_enum) {
            Tast_variable_def* new_memb = tast_variable_def_new(
                curr->pos,
                lang_type_enum_const_wrap(lang_type_enum_new(curr->pos, lang_type_atom_new(base->name, 0))),
                false,
                curr->name
            );
            vec_append(&a_main, &new_members, new_memb);
        } else {
            Tast_variable_def* new_memb = NULL;
            if (try_set_variable_def_types(env, &new_memb, curr, false, false)) {
                vec_append(&a_main, &new_members, new_memb);
            } else {
                success = false;
            }
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
    } while (0)

static bool try_set_struct_def_types(Env* env, Uast_struct_def* before_res, Uast_struct_def* after_res) {
    (void) env;
    (void) before_res;
    (void) after_res;
    todo();
    //// TODO: consider nested thing:
    //// type struct Token {
    ////      token Token
    //// }
    //Struct_def_base new_base = {0};
    //bool success = try_set_struct_base_types(env, &new_base, &after_res->base, false);
    //try_set_def_types_internal(
    //    env,
    //    uast_struct_def_wrap(after_res),
    //    before_res,
    //    tast_struct_def_wrap(tast_struct_def_new(after_res->pos, new_base))
    //);
    //return success;
}

static bool try_set_raw_union_def_types(Env* env, Uast_raw_union_def* before_res, Uast_raw_union_def* after_res) {
    // TODO: consider nested thing:
    // type raw_union Token {
    //      token Token
    // }
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(env, &new_base, &after_res->base, false);
    try_set_def_types_internal(
        env,
        uast_raw_union_def_wrap(after_res),
        before_res,
        tast_raw_union_def_wrap(tast_raw_union_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_enum_def_types(Env* env, Uast_enum_def* before_res, Uast_enum_def* after_res) {
    (void) env;
    (void) before_res;
    (void) after_res;
    todo();
    //Struct_def_base new_base = {0};
    //bool success = try_set_struct_base_types(env, &new_base, &after_res->base, true);
    //try_set_def_types_internal(
    //    env,
    //    uast_enum_def_wrap(after_res),
    //    before_res,
    //    tast_enum_def_wrap(tast_enum_def_new(after_res->pos, new_base))
    //);
    //return success;
}

// TODO: inline this function?
static bool try_set_sum_def_types(Env* env, Uast_sum_def* before_res, Uast_sum_def* after_res) {
    (void) env;
    (void) before_res;
    (void) after_res;
    todo();
    //// TODO: consider nested thing:
    //// type sum Token {
    ////      token Token
    //// }
    //Struct_def_base new_base = {0};
    //bool success = try_set_struct_base_types(env, &new_base, &after_res->base, false);
    //try_set_def_types_internal(
    //    env,
    //    uast_sum_def_wrap(after_res),
    //    before_res,
    //    tast_sum_def_wrap(tast_sum_def_new(after_res->pos, new_base))
    //);
    //return success;
}

static bool resolve_generics_serialize_struct_def_base(
    Env* env,
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_vec gen_args,
    Name new_name
) {
    (void) env;
    (void) new_base;
    (void) old_base;
    (void) gen_args;
    (void) new_name;
    //// TODO: figure out way to avoid making new Ustruct_def_base every time (do name thing here, and check if varient with name already exists)
    //memset(new_base, 0, sizeof(*new_base));

    if (gen_args.info.count < 1) {
        *new_base = old_base;
        return true;
    }
    todo();

    //for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
    //    // TODO: gen thign
    //    vec_append(&a_main, &new_base->members, uast_variable_def_clone(vec_at(&old_base.members, idx_memb)));
    //}

    //for (size_t idx_gen = 0; idx_gen < gen_args.info.count; idx_gen++) {
    //    Str_view gen_def = vec_at(&old_base.generics, idx_gen)->child->name;
    //    generic_sub_struct_def_base(env, new_base, gen_def, vec_at(&gen_args, idx_gen));
    //}

    //assert(old_base.members.info.count == new_base->members.info.count);

    //new_base->name = new_name;
    //return true;
}

typedef bool(*Set_obj_types)(Env*, void*, void*);
typedef Uast_def*(*Obj_unwrap)(void*);

static Uast_def* local_raw_union_new(Pos pos, Ustruct_def_base base) {
    return uast_raw_union_def_wrap(uast_raw_union_def_new(pos, base));
}

static Uast_def* local_enum_new(Pos pos, Ustruct_def_base base) {
    return uast_enum_def_wrap(uast_enum_def_new(pos, base));
}

static Uast_def* local_sum_new(Pos pos, Ustruct_def_base base) {
    return uast_sum_def_wrap(uast_sum_def_new(pos, base));
}

static Uast_def* local_struct_new(Pos pos, Ustruct_def_base base) {
    return uast_struct_def_wrap(uast_struct_def_new(pos, base));
}

static bool resolve_generics_ulang_type_internal_struct_like(
    Uast_def** after_res,
    Ulang_type* result,
    Env* env,
    Uast_def* before_res,
    Ulang_type lang_type,
    Ulang_type_vec gen_args,
    Uast_def*(*obj_new)(Pos, Ustruct_def_base),
    Set_obj_types set_obj_types,
    Obj_unwrap obj_unwrap
) {
    Ustruct_def_base old_base = uast_def_get_struct_def_base(before_res);
    Name new_name = serialize_generic(env, ulang_type_get_atom(lang_type).str, gen_args);

    if (old_base.generics.info.count != gen_args.info.count) {
        msg_invalid_count_generic_args(
            env,
            uast_def_get_pos(before_res),
            ulang_type_get_pos(lang_type),
            gen_args,
            old_base.generics.info.count,
            old_base.generics.info.count
        );
        return false;
    }

    Uast_def* new_def_ = NULL;
    if (usymbol_lookup(&new_def_, env, new_name)) {
        *after_res = new_def_;
    } else {
        Ustruct_def_base new_base = {0};
        if (!resolve_generics_serialize_struct_def_base(env, &new_base, old_base, gen_args, new_name)) {
            todo();
            return false;
        }
        *after_res = obj_new(uast_def_get_pos(before_res), new_base);
    }

    *result = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(
        uast_def_get_struct_def_base(*after_res).name, ulang_type_get_atom(lang_type).pointer_depth
    ), ulang_type_get_pos(lang_type)));

    Tast_def* dummy = NULL;
    if (symbol_lookup(&dummy, env, new_name)) {
        return true;
    }
    return set_obj_types(env, obj_unwrap(before_res), obj_unwrap(*after_res));
}

static bool resolve_generics_ulang_type_internal(Ulang_type* result, Env* env, Uast_def* before_res, Ulang_type lang_type, Ulang_type_vec gen_args) {
    Uast_def* after_res = NULL;
    switch (before_res->type) {
        case UAST_RAW_UNION_DEF:
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                env,
                before_res,
                lang_type,
                gen_args,
                local_raw_union_new,
                (Set_obj_types)try_set_raw_union_def_types,
                (Obj_unwrap)uast_raw_union_def_unwrap
            );
        case UAST_ENUM_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                env,
                before_res,
                lang_type,
                gen_args,
                local_enum_new,
                (Set_obj_types)try_set_enum_def_types,
                (Obj_unwrap)uast_enum_def_unwrap
            );
        }
        case UAST_SUM_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                env,
                before_res,
                lang_type,
                gen_args,
                local_sum_new,
                (Set_obj_types)try_set_sum_def_types,
                (Obj_unwrap)uast_sum_def_unwrap
            );
        }
        case UAST_STRUCT_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                env,
                before_res,
                lang_type,
                gen_args,
                local_struct_new,
                (Set_obj_types)try_set_struct_def_types,
                (Obj_unwrap)uast_struct_def_unwrap
            );
        }
        case UAST_PRIMITIVE_DEF:
            *result = lang_type;
            return true;
        case UAST_LITERAL_DEF:
            *result = lang_type;
            return true;
        default:
            unreachable(TAST_FMT, uast_def_print(before_res));
    }
    unreachable("");
}

bool resolve_generics_ulang_type_generic(Ulang_type* result, Env* env, Ulang_type_generic lang_type) {
    Uast_def* before_res = NULL;
    if (!usymbol_lookup(&before_res, env, lang_type.atom.str)) {
        msg_undefined_type(env, lang_type.pos, ulang_type_generic_const_wrap(lang_type));
        return false;
    }

    return resolve_generics_ulang_type_internal(
        result,
        env,
        before_res,
        ulang_type_generic_const_wrap(lang_type),
        lang_type.generic_args
    );
}

bool resolve_generics_ulang_type_regular(Ulang_type* result, Env* env, Ulang_type_regular lang_type) {
    Uast_def* before_res = NULL;
    log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, ulang_type_regular_const_wrap(lang_type)));
    if (!usymbol_lookup(&before_res, env, lang_type.atom.str)) {
        msg_undefined_type(env, lang_type.pos, ulang_type_regular_const_wrap(lang_type));
        return false;
    }

    return resolve_generics_ulang_type_internal(
        result,
        env,
        before_res,
        ulang_type_regular_const_wrap(lang_type), (Ulang_type_vec) {0}
    );
}

bool resolve_generics_ulang_type(Ulang_type* result, Env* env, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return resolve_generics_ulang_type_regular(result, env, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_REG_GENERIC:
            return resolve_generics_ulang_type_generic(result, env, ulang_type_generic_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
    }
    unreachable("");
}

static bool resolve_generics_serialize_function_decl(
    Env* env,
    Uast_function_decl** new_decl,
    const Uast_function_decl* old_decl,
    Uast_block* new_block,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
) {
    todo();
    //// TODO: figure out way to avoid making new Uast_function_decl every time
    //memset(new_decl, 0, sizeof(*new_decl));

    //Uast_param_vec params = {0};
    //for (size_t idx = 0; idx < old_decl->params->params.info.count; idx++) {
    //    vec_append(&a_main, &params, uast_param_clone(vec_at(&old_decl->params->params, idx)));
    //}

    //Ulang_type new_rtn_type = old_decl->return_type;

    //size_t idx_arg = 0;
    //for (; idx_arg < gen_args.info.count; idx_arg++) {
    //    if (idx_arg >= old_decl->generics.info.count) {
    //        msg_invalid_count_generic_args(
    //            env,
    //            old_decl->pos,
    //            pos_gen_args,
    //            gen_args,
    //            old_decl->generics.info.count,
    //            old_decl->generics.info.count
    //        );
    //        return false;
    //    }

    //    for (size_t idx_fun_param = 0; idx_fun_param < params.info.count; idx_fun_param++) {
    //        Str_view curr_arg = vec_at(&old_decl->generics, idx_arg)->child->name;
    //        generic_sub_param(env, vec_at(&params, idx_fun_param), curr_arg, vec_at(&gen_args, idx_arg));
    //    }
    //    Str_view curr_gen = vec_at(&old_decl->generics, idx_arg)->child->name;
    //    generic_sub_lang_type(env, &new_rtn_type, new_rtn_type, curr_gen, vec_at(&gen_args, idx_arg));
    //    generic_sub_block(env, new_block, curr_gen, vec_at(&gen_args, idx_arg));
    //}

    //if (idx_arg < old_decl->generics.info.count) {
    //    msg_invalid_count_generic_args(
    //        env,
    //        old_decl->pos,
    //        pos_gen_args,
    //        gen_args,
    //        old_decl->generics.info.count,
    //        old_decl->generics.info.count
    //    );
    //    return false;
    //}

    //String name = {0};
    //string_extend_cstr(&a_main, &name, "_");
    //string_extend_size_t(&a_main, &name, old_decl->name.count);
    //string_extend_strv(&a_main, &name, old_decl->name);
    //for (size_t idx = 0; idx < gen_args.info.count; idx++) {
    //    string_extend_strv(&a_main, &name, serialize_ulang_type(vec_at(&gen_args, idx)));
    //}

    //*new_decl = uast_function_decl_new(
    //    old_decl->pos,
    //    (Uast_generic_param_vec) {0},
    //    uast_function_params_new(old_decl->params->pos, params),
    //    new_rtn_type,
    //    string_to_strv(name)
    //);

    //return true;
}

// only generic function decls can be passed in here
bool resolve_generics_function_def(
    Uast_function_def** new_def,
    Env* env,
    Uast_function_def* def,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
) {
    todo();
    //bool status = true;

    //Uast_function_decl* new_decl = NULL;
    //if (!function_decl_generics_are_present(def->decl)) {
    //    unreachable("non generic function decls should not be passed here");
    //}

    //// TODO: try to avoid cloning block if resolve_generics_serialize_function_decl fails
    //Uast_block* new_block = uast_block_clone(def->body);
    //assert(new_block != def->body);
    //assert(new_block->symbol_collection.usymbol_table.table_tasts != def->body->symbol_collection.usymbol_table.table_tasts);

    //if (!resolve_generics_serialize_function_decl(env, &new_decl, def->decl, new_block, gen_args, pos_gen_args)) {
    //    return false;
    //}
    //*new_def = uast_function_def_new(new_decl->pos, new_decl, new_block);
    ////vec_rem_last(&env->ancesters);
    ////vec_append(&a_main, &env->ancesters, &new_block->symbol_collection);

    //// TODO: think about scopes for symbol_table if non-top-level functions are implemented

    //{
    //    size_t idx = 0;
    //    for (; idx < env->ancesters.info.count; idx++) {
    //        Uast_def* result = NULL;
    //        if (usym_tbl_lookup(&result, &vec_at(&env->ancesters, idx)->usymbol_table, def->decl->name)) {
    //            break;
    //        }
    //    }
    //    usym_tbl_add(&vec_at(&env->ancesters, idx)->usymbol_table, uast_function_decl_wrap(new_decl));
    //}

    //Tast_def* dummy = NULL;
    //if (!symbol_lookup(&dummy, env, (*new_def)->decl->name)) {
    //    // TODO: see if there is less hacky way to do this
    //    Sym_coll_vec tbls = env->ancesters;
    //    memset(&env->ancesters, 0, sizeof(env->ancesters));
    //    vec_append(&a_main, &env->ancesters, vec_at(&tbls, 0));
    //    if (!try_set_function_def_types(env, *new_def, true)) {
    //        status = false;
    //    }
    //    env->ancesters = tbls;
    //}

    //unwrap(symbol_lookup(&dummy, env, (*new_def)->decl->name));
    //return status;
}

static bool ulang_type_generics_are_present_tuple(Ulang_type_tuple lang_type) {
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        if (ulang_type_generics_are_present(vec_at(&lang_type.ulang_types, idx))) {
            return true;
        }
    }
    return false;
}

static bool ulang_type_generics_are_present_fn(Ulang_type_fn lang_type) {
    if (ulang_type_generics_are_present_tuple(lang_type.params)) {
        return true;
    }
    return ulang_type_generics_are_present(*lang_type.return_type);
}

static bool ulang_type_generics_are_present(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return false;
        case ULANG_TYPE_REG_GENERIC:
            return true;
        case ULANG_TYPE_TUPLE:
            return ulang_type_generics_are_present_tuple(ulang_type_tuple_const_unwrap(lang_type));
        case ULANG_TYPE_FN:
            return ulang_type_generics_are_present_fn(ulang_type_fn_const_unwrap(lang_type));
    }
    unreachable("");
}

bool function_decl_generics_are_present(const Uast_function_decl* decl) {
    return decl->generics.info.count > 0;
}

bool variable_def_generics_are_present(const Uast_variable_def* def) {
    return ulang_type_generics_are_present(def->lang_type);
}
