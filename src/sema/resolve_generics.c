#include <resolve_generics.h>
#include <type_checking.h>
#include <lang_type_serialize.h>
#include <uast_serialize.h>
#include <ulang_type.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <ulang_type_get_pos.h>
#include <ulang_type_get_atom.h>
#include <generic_sub.h>
#include <symbol_log.h>
#include <msg_todo.h>

#define msg_invalid_count_generic_args(pos_def, pos_gen_args, gen_args, min_args, max_args) \
    msg_invalid_count_generic_args_internal(__FILE__, __LINE__,  pos_def, pos_gen_args, gen_args, min_args, max_args)

// TODO: move this function and macro to better place
static void msg_undefined_type_internal(
    const char* file,
    int line,
     
    Pos pos,
    Ulang_type lang_type
) {
    msg_internal(
        file, line, LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, pos,
        "type `"LANG_TYPE_FMT"` is not defined\n", ulang_type_print(LANG_TYPE_MODE_MSG, lang_type)
    );
}

#define msg_undefined_type(pos, lang_type) \
    msg_undefined_type_internal(__FILE__, __LINE__,  pos, lang_type)

static void msg_invalid_count_generic_args_internal(
    const char* file,
    int line,
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
        file, line, LOG_ERROR, EXPECT_FAIL_INVALID_COUNT_GENERIC_ARGS, pos_gen_args,
        STR_VIEW_FMT, str_view_print(string_to_strv(message))
    );

    msg_internal(
        file, line, LOG_NOTE, EXPECT_FAIL_NONE, pos_def,
        "generic parameters defined here\n" 
    );
}

static bool try_set_struct_base_types(Struct_def_base* new_base, Ustruct_def_base* base, bool is_enum) {
    env.type_checking_is_in_struct_base_def = true;
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
            if (try_set_variable_def_types(&new_memb, curr, false, false)) {
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

    env.type_checking_is_in_struct_base_def = false;
    return success;
}

#define try_set_def_types_internal(after_res, before_res, new_def) \
    do { \
        usym_tbl_add(after_res); \
        sym_tbl_add(new_def); \
    } while (0)

static bool try_set_struct_def_types(Uast_struct_def* before_res, Uast_struct_def* after_res) {
    // TODO: consider nested thing:
    // type struct Token {
    //      token Token
    // }
    (void) before_res;
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base, false);
    try_set_def_types_internal(
        uast_struct_def_wrap(after_res),
        before_res,
        tast_struct_def_wrap(tast_struct_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_raw_union_def_types(Uast_raw_union_def* before_res, Uast_raw_union_def* after_res) {
    // TODO: consider nested thing:
    // type raw_union Token {
    //      token Token
    // }
    (void) before_res;
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base, false);
    try_set_def_types_internal(
        uast_raw_union_def_wrap(after_res),
        before_res,
        tast_raw_union_def_wrap(tast_raw_union_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_enum_def_types(Uast_enum_def* before_res, Uast_enum_def* after_res) {
    (void) before_res;
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base, true);
    try_set_def_types_internal(
        uast_enum_def_wrap(after_res),
        before_res,
        tast_enum_def_wrap(tast_enum_def_new(after_res->pos, new_base))
    );
    return success;
}

// TODO: inline this function?
static bool try_set_sum_def_types(Uast_sum_def* before_res, Uast_sum_def* after_res) {
    (void) before_res;
    // TODO: consider nested thing:
    // type sum Token {
    //      token Token
    // }
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base, false);
    try_set_def_types_internal(
        uast_sum_def_wrap(after_res),
        before_res,
        tast_sum_def_wrap(tast_sum_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool resolve_generics_serialize_struct_def_base(
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_vec gen_args,
    Name new_name
) {
    //// TODO: figure out way to avoid making new Ustruct_def_base every time (do name thing here, and check if varient with name already exists)
    //memset(new_base, 0, sizeof(*new_base));

    if (gen_args.info.count < 1) {
        *new_base = old_base;
        return true;
    }

    for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
        // TODO: gen thign
        vec_append(&a_main, &new_base->members, uast_variable_def_clone(vec_at(&old_base.members, idx_memb), 0));
    }

    for (size_t idx_gen = 0; idx_gen < gen_args.info.count; idx_gen++) {
        Name gen_def = vec_at(&old_base.generics, idx_gen)->child->name;
        generic_sub_struct_def_base(new_base, gen_def, vec_at(&gen_args, idx_gen));
    }

    assert(old_base.members.info.count == new_base->members.info.count);

    new_base->name = new_name;
    return true;
}

typedef bool(*Set_obj_types)(void*, void*);
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
    Uast_def* before_res,
    Ulang_type lang_type,
    Uast_def*(*obj_new)(Pos, Ustruct_def_base),
    Set_obj_types set_obj_types,
    Obj_unwrap obj_unwrap
) {
    Ustruct_def_base old_base = uast_def_get_struct_def_base(before_res);
    Name new_name = name_new(old_base.name.mod_path, old_base.name.base, ulang_type_regular_const_unwrap(lang_type).atom.str.gen_args, SCOPE_TOP_LEVEL /* TODO */);

    if (old_base.generics.info.count != new_name.gen_args.info.count) {
        msg_invalid_count_generic_args(
            uast_def_get_pos(before_res),
            ulang_type_get_pos(lang_type),
            new_name.gen_args,
            old_base.generics.info.count,
            old_base.generics.info.count
        );
        return false;
    }

    Uast_def* new_def_ = NULL;
    if (usymbol_lookup(&new_def_,  new_name)) {
        *after_res = new_def_;
    } else {
        Ustruct_def_base new_base = {0};
        if (!resolve_generics_serialize_struct_def_base(&new_base, old_base, new_name.gen_args, new_name)) {
            todo();
            return false;
        }
        *after_res = obj_new(uast_def_get_pos(before_res), new_base);
    }

    *result = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(
        name_to_uname(uast_def_get_struct_def_base(*after_res).name), ulang_type_get_atom(lang_type).pointer_depth
    ), ulang_type_get_pos(lang_type)));

    Tast_def* dummy = NULL;
    if (symbol_lookup(&dummy,  new_name)) {
        return true;
    }
    return set_obj_types(obj_unwrap(before_res), obj_unwrap(*after_res));
}

static bool resolve_generics_ulang_type_internal(Ulang_type* result, Uast_def* before_res, Ulang_type lang_type) {
    Uast_def* after_res = NULL;
    switch (before_res->type) {
        case UAST_RAW_UNION_DEF:
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                before_res,
                lang_type,
                local_raw_union_new,
                (Set_obj_types)try_set_raw_union_def_types,
                (Obj_unwrap)uast_raw_union_def_unwrap
            );
        case UAST_ENUM_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                before_res,
                lang_type,
                local_enum_new,
                (Set_obj_types)try_set_enum_def_types,
                (Obj_unwrap)uast_enum_def_unwrap
            );
        }
        case UAST_SUM_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                before_res,
                lang_type,
                local_sum_new,
                (Set_obj_types)try_set_sum_def_types,
                (Obj_unwrap)uast_sum_def_unwrap
            );
        }
        case UAST_STRUCT_DEF: {
            return resolve_generics_ulang_type_internal_struct_like(
                &after_res,
                result,
                before_res,
                lang_type,
                local_struct_new,
                (Set_obj_types)try_set_struct_def_types,
                (Obj_unwrap)uast_struct_def_unwrap
            );
        }
        case UAST_PRIMITIVE_DEF:
            *result = lang_type;
            return true;
        case UAST_LANG_DEF: {
            unreachable("def should have been eliminated by now");
            //if (lang_type.type != ULANG_TYPE_REGULAR) {
            //    msg_todo("def alias in tuple, function callback type, etc.", ulang_type_regular_const_unwrap(lang_type).pos);
            //    return false;
            //}

            //Uast_expr* new_expr = uast_lang_def_unwrap(before_res)->expr;
            //if (new_expr->type != UAST_SYMBOL) {
            //    msg_todo("def aliased to non-symbol used as type", ulang_type_regular_const_unwrap(lang_type).pos);
            //    return false;
            //}
            //Ulang_type new_lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(
            //    ulang_type_atom_new(
            //        name_to_uname(uast_symbol_unwrap(new_expr)->name),
            //        ulang_type_regular_const_unwrap(lang_type).atom.pointer_depth
            //    ),
            //    ulang_type_regular_const_unwrap(lang_type).pos
            //));
            //return resolve_generics_ulang_type(result, new_lang_type);
        }
        case UAST_POISON_DEF:
            todo();
        case UAST_IMPORT_PATH:
            todo();
        case UAST_MOD_ALIAS:
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_VARIABLE_DEF:
            todo();
    }
    unreachable("");
}

bool resolve_generics_ulang_type_regular(Ulang_type* result, Ulang_type_regular lang_type) {
    Uast_def* before_res = NULL;
    Name name_base = {0};
    if (!name_from_uname(&name_base, lang_type.atom.str)) {
        return false;
    }
    memset(&name_base.gen_args, 0, sizeof(name_base.gen_args));
    if (!usymbol_lookup(&before_res, name_base)) {
        msg_undefined_type(lang_type.pos, ulang_type_regular_const_wrap(lang_type));
        return false;
    }

    return resolve_generics_ulang_type_internal(
        result,
        before_res,
        ulang_type_regular_const_wrap(lang_type)
    );
}

bool resolve_generics_ulang_type(Ulang_type* result, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return resolve_generics_ulang_type_regular(result,  ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
    }
    unreachable("");
}

static bool resolve_generics_set_function_def_types(Uast_function_def* def) {
    Ulang_type prev_fn_rtn_type = env.parent_fn_rtn_type;
    env.parent_fn_rtn_type = def->decl->return_type;
    bool status = true;

    Tast_function_decl* new_decl = NULL;
    if (!try_set_function_decl_types(&new_decl, def->decl, true)) {
        status = false;
        goto error;
    }

    Tast_block* new_body = NULL;
    if (!try_set_block_types(&new_body, def->body, true, true)) {
        status = false;
        goto error;
    }

    Tast_def* result = NULL;
    unwrap(symbol_lookup(&result, new_decl->name));
    if (true /* TODO */) {
        sym_tbl_update(SCOPE_TOP_LEVEL, tast_function_def_wrap(tast_function_def_new(def->pos, new_decl, new_body)));
        unwrap(symbol_lookup(&result, new_decl->name));
    } else {
        symbol_update(tast_function_def_wrap(tast_function_def_new(def->pos, new_decl, new_body)));
    }

error:
    env.parent_fn_rtn_type = prev_fn_rtn_type;
    return status;
}

static bool resolve_generics_serialize_function_decl(
    Uast_function_decl** new_decl,
    const Uast_function_decl* old_decl,
    Uast_block* new_block,
    Ulang_type_vec gen_args, // TODO: remove this arg
    Pos pos_gen_args
) {
    // TODO: figure out way to avoid making new Uast_function_decl every time
    memset(new_decl, 0, sizeof(*new_decl));

    Uast_param_vec params = {0};
    for (size_t idx = 0; idx < old_decl->params->params.info.count; idx++) {
        vec_append(&a_main, &params, uast_param_clone(vec_at(&old_decl->params->params, idx), new_block->scope_id));
    }

    Ulang_type new_rtn_type = old_decl->return_type;

    size_t idx_arg = 0;
    for (; idx_arg < gen_args.info.count; idx_arg++) {
        if (idx_arg >= old_decl->generics.info.count) {
            msg_invalid_count_generic_args(
                old_decl->pos,
                pos_gen_args,
                gen_args,
                old_decl->generics.info.count,
                old_decl->generics.info.count
            );
            return false;
        }

        for (size_t idx_fun_param = 0; idx_fun_param < params.info.count; idx_fun_param++) {
            Name curr_arg = vec_at(&old_decl->generics, idx_arg)->child->name;
            // TODO: same params are being replaced both here and in generic_sub_block?
            generic_sub_param(vec_at(&params, idx_fun_param), curr_arg, vec_at(&gen_args, idx_arg));
        }
        Name curr_gen = vec_at(&old_decl->generics, idx_arg)->child->name;
        generic_sub_lang_type(&new_rtn_type, new_rtn_type, curr_gen, vec_at(&gen_args, idx_arg));
        generic_sub_block(new_block, curr_gen, vec_at(&gen_args, idx_arg));
    }

    if (idx_arg < old_decl->generics.info.count) {
        msg_invalid_count_generic_args(
            old_decl->pos,
            pos_gen_args,
            gen_args,
            old_decl->generics.info.count,
            old_decl->generics.info.count
        );
        return false;
    }

    *new_decl = uast_function_decl_new(
        old_decl->pos,
        (Uast_generic_param_vec) {0},
        uast_function_params_new(old_decl->params->pos, params),
        new_rtn_type,
        name_new(env.curr_mod_path, old_decl->name.base, gen_args, scope_get_parent_tbl_lookup(new_block->scope_id))
    );

    return true;
}

bool resolve_generics_function_def_call(
    Lang_type* rtn_type, // TODO: rename this parameter
    Name* new_name,
    Uast_function_def* def,
    Ulang_type_vec gen_args, // TODO: remove or refactor name?
    Pos pos_gen_args
) {
    (void) pos_gen_args;
    Name name = name_new(def->decl->name.mod_path, def->decl->name.base, gen_args, def->decl->name.scope_id);
    Name name_plain = name_new(def->decl->name.mod_path, def->decl->name.base, (Ulang_type_vec) {0}, def->decl->name.scope_id);

    // TODO: put pos_gen_args as value in resolved_already_tbl_add?
    Uast_function_decl* cached = NULL;
    if (function_decl_tbl_lookup(&cached, name)) {
        *rtn_type = lang_type_from_ulang_type(cached->return_type);
        *new_name = name;
        return true;
    }

    // TODO: expected failure case for recursive generic function with mismatched args
    if (def->decl->generics.info.count != gen_args.info.count) {
        msg_invalid_count_generic_args(def->pos, pos_gen_args, gen_args, def->decl->generics.info.count, def->decl->generics.info.count);
        return false;
    }

    //Uast_def* result = NULL;
    //if (!usymbol_lookup(&result, name_plain)) {
    //    todo();
    //}
    //Uast_function_def* fun_def = uast_function_def_unwrap(result);
    // TODO: avoid cloning everytime; use function_decl_tbl instead where possible
    Uast_function_decl* decl = uast_function_decl_clone(def->decl, def->decl->name.scope_id);
    decl->name = name_plain;
    if (def->decl->generics.info.count > 0) {
        for (size_t idx_gen_param = 0; idx_gen_param < gen_args.info.count; idx_gen_param++) {
            Name gen_param = vec_at(&decl->generics, idx_gen_param)->child->name;
            Ulang_type gen_arg = vec_at(&gen_args, idx_gen_param);
            generic_sub_lang_type(&decl->return_type, decl->return_type, gen_param, gen_arg);
            for (size_t idx_param = 0; idx_param < decl->params->params.info.count; idx_param++) {
                Uast_param* param = vec_at(&decl->params->params, idx_param);
                generic_sub_param(param, gen_param, gen_arg);
            }
        }
    }
    decl->name = name;
    Uast_function_decl* dummy = NULL;
    unwrap(function_decl_tbl_add(decl));
    unwrap(function_decl_tbl_lookup(&dummy, decl->name));

    Ulang_type_vec ulang_types = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        vec_append(&a_main, &ulang_types, vec_at(&decl->params->params, idx)->base->lang_type);
    }
    Ulang_type* ulang_type_rtn_type = arena_alloc(&a_main, sizeof(*ulang_type_rtn_type));
    *ulang_type_rtn_type = decl->return_type;
    Ulang_type_fn new_fn = ulang_type_fn_new(
        ulang_type_tuple_new(ulang_types, (Pos) {0} /* TODO */),
        ulang_type_rtn_type,
        (Pos) {0} /* TODO */
    );
    Lang_type_fn rtn_type_ = {0};
    if (!try_lang_type_from_ulang_type_fn(&rtn_type_, new_fn, (Pos) {0})) {
        return false;
    }
    *rtn_type = lang_type_fn_const_wrap(rtn_type_);
    *new_name = name;

    vec_append(&a_main, &env.fun_implementations_waiting_to_resolve, name);

    return true;
}

bool resolve_generics_function_def_implementation(Name name) {
    Name name_plain = name_new(name.mod_path, name.base, (Ulang_type_vec) {0}, name.scope_id);
    Uast_def* dummy = NULL;
    (void) dummy;
    Tast_def* dummy_2 = NULL;
    Uast_function_decl* dummy_3 = NULL;
    assert(
        !symbol_lookup(&dummy_2, name) &&
        "same function has been passed to resolve_generics_function_def_implementation "
        "more than once, and it should not have been"
    );

    Uast_def* result = NULL;
    if (usymbol_lookup(&result, name)) {
        // we only need to type check this function
        return resolve_generics_set_function_def_types(uast_function_def_unwrap(result));
    } else {
        // we need to make new uast function implementation and then type check it
        unwrap(usymbol_lookup(&result, name_plain));
        unwrap(function_decl_tbl_lookup(&dummy_3, name));
        Uast_function_def* def = uast_function_def_unwrap(result);
        Uast_block* new_block = uast_block_clone(def->body, def->decl->name.scope_id);
        assert(new_block != def->body);

        Uast_function_decl* new_decl = NULL;
        if (!resolve_generics_serialize_function_decl(&new_decl, def->decl, new_block, name.gen_args, (Pos) {0} /* TODO */)) {
            return false;
        }
        Uast_function_def* new_def = uast_function_def_new(new_decl->pos, new_decl, new_block);
        usym_tbl_add(uast_function_def_wrap(new_def));
        return resolve_generics_set_function_def_types(new_def);
    }
    unreachable("");
}
