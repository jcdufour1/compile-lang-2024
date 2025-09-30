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
#include <symbol_iter.h>
#include <expand_lang_def.h>

static bool is_in_struct_base_def;

#define msg_invalid_count_generic_args(pos_def, pos_gen_args, gen_args, min_args, max_args) \
    msg_invalid_count_generic_args_internal(__FILE__, __LINE__,  pos_def, pos_gen_args, gen_args, min_args, max_args)

// TODO: move this function and macro to better place
static void msg_undefined_type_internal(
    const char* file,
    int line,
    Pos pos,
    Ulang_type lang_type
) {
    if (!env.silent_generic_resol_errors) {
        msg_internal(
            file, line, DIAG_UNDEFINED_TYPE, pos,
            "type `"FMT"` is not defined\n", ulang_type_print(LANG_TYPE_MODE_MSG, lang_type)
        );
    }
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
    if (env.silent_generic_resol_errors) {
        return;
    }

    String message = {0};
    string_extend_size_t(&a_print, &message, gen_args.info.count);
    string_extend_cstr(&a_print, &message, " generic arguments are passed");
    // TODO: print base type (eg. `Token`)
    string_extend_cstr(&a_print, &message, ", but ");
    string_extend_size_t(&a_print, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&a_print, &message, " or more");
    }
    string_extend_cstr(&a_print, &message, " generic arguments expected\n");
    msg_internal(
        file, line, DIAG_INVALID_COUNT_GENERIC_ARGS, pos_gen_args,
        FMT, strv_print(string_to_strv(message))
    );

    msg_internal(
        file, line, DIAG_NOTE, pos_def,
        "generic parameters defined here\n" 
    );
}

static bool try_set_struct_base_types(Struct_def_base* new_base, Ustruct_def_base* base) {
    is_in_struct_base_def = true;
    bool status = true;
    Tast_variable_def_vec new_members = {0};

    if (base->members.info.count < 1) {
        todo();
    }

    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Uast_variable_def* curr = vec_at(&base->members, idx);

        for (size_t prev_idx = 0; prev_idx < idx; prev_idx++) {
            if (name_is_equal(vec_at(&base->members, prev_idx)->name, curr->name)) {
                if (env.silent_generic_resol_errors) {
                    return false;
                }

                msg(
                    DIAG_REDEF_STRUCT_BASE_MEMBER, curr->pos,
                    "redefinition of member `"FMT"`\n",
                    name_print(NAME_MSG, curr->name)
                );
                msg(
                    DIAG_NOTE, vec_at(&base->members, prev_idx)->pos,
                    "member `"FMT"` previously defined here\n",
                    name_print(NAME_MSG, curr->name)
                );
                status = false;
            }
        }

        Tast_variable_def* new_memb = NULL;
        if (try_set_variable_def_types(&new_memb, curr, false, false)) {
            vec_append(&a_main, &new_members, new_memb);
        } else {
            status = false;
        }
    }

    *new_base = (Struct_def_base) {
        .members = new_members,
        .name = base->name
    };

    is_in_struct_base_def = false;
    return status;
}

#define try_set_def_types_internal(after_res, before_res, new_def) \
    do { \
        usym_tbl_add(after_res); \
        sym_tbl_add(new_def); \
    } while (0)

static bool try_set_struct_def_types(Uast_struct_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_struct_def_wrap(after_res),
        before_res,
        tast_struct_def_wrap(tast_struct_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_raw_union_def_types(Uast_raw_union_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_raw_union_def_wrap(after_res),
        before_res,
        tast_raw_union_def_wrap(tast_raw_union_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_enum_def_types(Uast_enum_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_enum_def_wrap(after_res),
        before_res,
        tast_enum_def_wrap(tast_enum_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool resolve_generics_serialize_struct_def_base(
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_vec gen_args,
    Name new_name
) {
    if (gen_args.info.count < 1) {
        *new_base = old_base;
        return true;
    }

    for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
        vec_append(&a_main, &new_base->members, uast_variable_def_clone(vec_at(&old_base.members, idx_memb), false, 0));
    }

    for (size_t idx_gen = 0; idx_gen < gen_args.info.count; idx_gen++) {
        Name gen_def = vec_at(&old_base.generics, idx_gen)->name;
        generic_sub_struct_def_base(new_base, gen_def, vec_at(&gen_args, idx_gen));
    }

    assert(old_base.members.info.count == new_base->members.info.count);

    new_base->name = new_name;
    return true;
}

typedef void*(*Obj_new)(Pos, Ustruct_def_base);

static bool resolve_generics_ulang_type_internal_struct_like(
    Uast_def** after_res,
    Ulang_type* result,
    Ustruct_def_base old_base,
    Ulang_type lang_type,
    Pos pos_def,
    Obj_new obj_new
) {
    Name new_name = name_new(old_base.name.mod_path, old_base.name.base, ulang_type_regular_const_unwrap(lang_type).atom.str.gen_args, SCOPE_TOP_LEVEL /* TODO */);

#   ifndef NDEBUG
        Uast_def* dummy_2 = NULL;
        (void) dummy_2;
        if (usymbol_lookup(&dummy_2, new_name)) {
            assert(dummy_2->type != UAST_LANG_DEF);
        }
#   endif // NDEBUG
       
    if (!struct_like_tbl_lookup(after_res, new_name)) {
        if (old_base.generics.info.count != new_name.gen_args.info.count) {
            msg_invalid_count_generic_args(
                pos_def,
                ulang_type_get_pos(lang_type),
                new_name.gen_args,
                old_base.generics.info.count,
                old_base.generics.info.count
            );
            return false;
        }

        Uast_def* new_def_ = NULL;
        if (usymbol_lookup(&new_def_, new_name)) {
            *after_res = new_def_;
        } else {
            Ustruct_def_base new_base = {0};
            if (!resolve_generics_serialize_struct_def_base(&new_base, old_base, new_name.gen_args, new_name)) {
                todo();
                return false;
            }
            *after_res = (void*)obj_new(pos_def, new_base);
        }

    }

    *result = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(
        name_to_uname(uast_def_get_struct_def_base(*after_res).name), ulang_type_get_atom(lang_type).pointer_depth
    ), ulang_type_get_pos(lang_type)));

    Tast_def* dummy = NULL;
    if (symbol_lookup(&dummy, new_name)) {
        return true;
    }
    if (struct_like_tbl_add(*after_res)) {
        usym_tbl_add(*after_res);
        vec_append(&a_main, &env.struct_like_waiting_to_resolve, new_name);
    }
        Uast_def* dummy_3 = NULL;
        (void) dummy_3;
        if (usymbol_lookup(&dummy_3, new_name)) {
            assert(dummy_3->type != UAST_LANG_DEF);
        }
    return true;
}

static void* local_uast_raw_union_def_new(Pos pos, Ustruct_def_base base) {
    return uast_raw_union_def_new(pos, base);
}

static void* local_uast_enum_def_new(Pos pos, Ustruct_def_base base) {
    return uast_enum_def_new(pos, base);
}

static void* local_uast_struct_def_new(Pos pos, Ustruct_def_base base) {
    return uast_struct_def_new(pos, base);
}

static bool resolve_generics_ulang_type_internal(LANG_TYPE_TYPE* type, Ulang_type* result, Uast_def* before_res, Ulang_type lang_type) {
    switch (before_res->type) {
        case UAST_RAW_UNION_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_internal_struct_like(&after_res_, result, uast_def_get_struct_def_base(before_res), lang_type, uast_def_get_pos(before_res), local_uast_raw_union_def_new)) {
                return false;
            }
            *type = LANG_TYPE_RAW_UNION;
            return true;
        }
        case UAST_ENUM_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_internal_struct_like(&after_res_, result, uast_def_get_struct_def_base(before_res), lang_type, uast_def_get_pos(before_res), local_uast_enum_def_new)) {
                return false;
            }
            *type = LANG_TYPE_ENUM;
            return true;
        }
        case UAST_STRUCT_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_internal_struct_like(&after_res_, result, uast_def_get_struct_def_base(before_res), lang_type, uast_def_get_pos(before_res), local_uast_struct_def_new)) {
                return false;
            }
            *type = LANG_TYPE_STRUCT;
            return true;
        }
        case UAST_PRIMITIVE_DEF:
            *result = lang_type;
            *type = LANG_TYPE_PRIMITIVE;
            return true;
        case UAST_VOID_DEF:
            *result = lang_type;
            *type = LANG_TYPE_VOID;
            return true;
        case UAST_LANG_DEF:
            log(LOG_ERROR, FMT"\n", uast_def_print(before_res));
            log(LOG_ERROR, "%d\n", uast_def_get_pos(before_res).line);
            unreachable("def should have been eliminated by now");
        case UAST_POISON_DEF:
            todo();
        case UAST_IMPORT_PATH:
            todo();
        case UAST_MOD_ALIAS:
            todo();
        case UAST_GENERIC_PARAM:
            // TODO: explain why it is unreachable
            log(LOG_ERROR, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, lang_type));
            unreachable("");
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_VARIABLE_DEF:
            todo();
        case UAST_LABEL:
            todo();
    }
    unreachable("");
}

bool resolve_generics_ulang_type_regular(LANG_TYPE_TYPE* type, Ulang_type* result, Ulang_type_regular lang_type) {
    Uast_def* before_res = NULL;
    Name name_base = {0};
    if (!name_from_uname(&name_base, lang_type.atom.str, lang_type.pos)) {
        return false;
    }

    if (strv_is_equal(lang_type.atom.str.base, sv("Str"))) {
        log(LOG_DEBUG, "sdflk\n");
        log(LOG_DEBUG, "sdflk\n");
        log(LOG_DEBUG, "sdflk\n");
        log(LOG_DEBUG, "sdflk\n");
        log(LOG_DEBUG, "sdflk\n");
    }

    Ulang_type_regular new_lang_type = {0};
    if (!expand_def_ulang_type_regular(&new_lang_type, lang_type, lang_type.pos /* TODO */)) {
        todo();
        return false;
    }

    Uast_expr* dummy = NULL;
    if (EXPAND_NAME_NORMAL != expand_def_name(&dummy, &name_base, lang_type.pos /* TODO */)) {
        todo();
    }

    memset(&name_base.gen_args, 0, sizeof(name_base.gen_args));
    if (!usymbol_lookup(&before_res, name_base)) {
        msg_undefined_type(new_lang_type.pos, ulang_type_regular_const_wrap(new_lang_type));
        return false;
    }

    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name_base));
    log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(lang_type)));
    log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(new_lang_type)));
    log(LOG_DEBUG, FMT"\n", uast_def_print(before_res));
    return resolve_generics_ulang_type_internal(
        type,
        result,
        before_res,
        ulang_type_regular_const_wrap(new_lang_type)
    );
}

bool resolve_generics_struct_like_def_implementation(Name name) {
    Uast_def* before_res = NULL;
    Name name_before = name_clone(name, false, 0);
    memset(&name_before.gen_args, 0, sizeof(name_before.gen_args));
    unwrap(usym_tbl_lookup(&before_res, name_before));
    Ulang_type dummy = {0};
    Ulang_type lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(name_to_uname(name), 0), uast_def_get_pos(before_res)));

    Uast_def* after_res = NULL;
    if (!resolve_generics_ulang_type_internal_struct_like(&after_res, &dummy, uast_def_get_struct_def_base(before_res), lang_type, uast_def_get_pos(before_res), local_uast_struct_def_new)) {
        return false;
    }

    switch (before_res->type) {
        case UAST_STRUCT_DEF:
            return try_set_struct_def_types(uast_struct_def_unwrap(after_res));
        case UAST_RAW_UNION_DEF:
            return try_set_raw_union_def_types(uast_raw_union_def_unwrap(after_res));
        case UAST_ENUM_DEF:
            return try_set_enum_def_types(uast_enum_def_unwrap(after_res));
        case UAST_POISON_DEF:
            unreachable("");
        case UAST_IMPORT_PATH:
            unreachable("");
        case UAST_MOD_ALIAS:
            unreachable("");
        case UAST_GENERIC_PARAM:
            unreachable("");
        case UAST_FUNCTION_DEF:
            unreachable("");
        case UAST_VARIABLE_DEF:
            unreachable("");
        case UAST_LANG_DEF:
            unreachable("");
        case UAST_PRIMITIVE_DEF:
            log(LOG_DEBUG, FMT"\n", uast_def_print(before_res));
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_VOID_DEF:
            unreachable("");
        case UAST_LABEL:
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
    if (!try_set_block_types(&new_body, def->body, true)) {
        status = false;
        goto error;
    }

    Tast_def* result = NULL;
    unwrap(symbol_lookup(&result, new_decl->name));
    sym_tbl_update(SCOPE_TOP_LEVEL, tast_function_def_wrap(tast_function_def_new(def->pos, new_decl, new_body)));
    unwrap(symbol_lookup(&result, new_decl->name));

error:
    env.parent_fn_rtn_type = prev_fn_rtn_type;
    return status;
}

static bool resolve_generics_serialize_function_decl(
    Uast_function_decl** new_decl,
    const Uast_function_decl* old_decl,
    Uast_block* new_block,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
) {
    memset(new_decl, 0, sizeof(*new_decl));

    Uast_param_vec params = {0};
    for (size_t idx = 0; idx < old_decl->params->params.info.count; idx++) {
        vec_append(&a_main, &params, uast_param_clone(vec_at(&old_decl->params->params, idx), true, new_block->scope_id));
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
            Name curr_arg = vec_at(&old_decl->generics, idx_arg)->name;
            // TODO: same params are being replaced both here and in generic_sub_block?
            generic_sub_param(vec_at(&params, idx_fun_param), curr_arg, vec_at(&gen_args, idx_arg));
        }
        Name curr_gen = vec_at(&old_decl->generics, idx_arg)->name;
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
        name_new(old_decl->name.mod_path, old_decl->name.base, gen_args, scope_get_parent_tbl_lookup(new_block->scope_id))
    );

    return true;
}

bool resolve_generics_function_def_call(
    Lang_type_fn* type_res,
    Name* new_name,
    Uast_function_def* def,
    Ulang_type_vec gen_args, // TODO: remove or refactor name?
    Pos pos_gen_args
) {
    // TODO: do not call expand_def_function_def on every call to resolve_generics_function_def_call (this could be wasteful)
    if (!expand_def_function_def(def)) {
        return false;
    }

    Name name = name_new(def->decl->name.mod_path, def->decl->name.base, gen_args, def->decl->name.scope_id);
    Name name_plain = name_new(def->decl->name.mod_path, def->decl->name.base, (Ulang_type_vec) {0}, def->decl->name.scope_id);

    // TODO: put pos_gen_args as value in resolved_already_tbl_add?
    Uast_function_decl* cached = NULL;
    if (function_decl_tbl_lookup(&cached, name)) {
        // TODO: consider caching ulang_types
        Ulang_type_vec ulang_types = {0};
        for (size_t idx = 0; idx < cached->params->params.info.count; idx++) {
            vec_append(&a_main, &ulang_types, vec_at(&cached->params->params, idx)->base->lang_type);
        }

        Ulang_type* ulang_type_rtn_type = arena_alloc(&a_main, sizeof(*ulang_type_rtn_type));
        *ulang_type_rtn_type = cached->return_type;
        Ulang_type_fn new_fn = ulang_type_fn_new(
            ulang_type_tuple_new(ulang_types, def->decl->pos),
            ulang_type_rtn_type,
            def->decl->pos
        );
        if (!try_lang_type_from_ulang_type_fn(type_res, new_fn)) {
            return false;
        }
        *new_name = name;
        return true;
    }

    if (def->decl->generics.info.count != gen_args.info.count) {
        msg_invalid_count_generic_args(def->pos, pos_gen_args, gen_args, def->decl->generics.info.count, def->decl->generics.info.count);
        return false;
    }

    Uast_function_decl* decl = uast_function_decl_clone(def->decl, true, def->decl->name.scope_id);
    decl->name = name_plain;
    if (def->decl->generics.info.count > 0) {
        for (size_t idx_gen_param = 0; idx_gen_param < gen_args.info.count; idx_gen_param++) {
            Name gen_param = vec_at(&decl->generics, idx_gen_param)->name;
            Ulang_type gen_arg = vec_at(&gen_args, idx_gen_param);
            generic_sub_lang_type(&decl->return_type, decl->return_type, gen_param, gen_arg);
            for (size_t idx_param = 0; idx_param < decl->params->params.info.count; idx_param++) {
                Uast_param* param = vec_at(&decl->params->params, idx_param);
                if (param->base->lang_type.type != ULANG_TYPE_GEN_PARAM) {
                    generic_sub_param(param, gen_param, gen_arg);
                }
            }
        }
    }
    decl->name = name;
    Uast_function_decl* dummy = NULL;
    unwrap(function_decl_tbl_add(decl));
    unwrap(function_decl_tbl_lookup(&dummy, decl->name));

    // TODO: consider caching ulang_types
    Ulang_type_vec ulang_types = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        vec_append(&a_main, &ulang_types, vec_at(&decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* ulang_type_rtn_type = arena_alloc(&a_main, sizeof(*ulang_type_rtn_type));
    *ulang_type_rtn_type = decl->return_type;
    Ulang_type_fn new_fn = ulang_type_fn_new(
        ulang_type_tuple_new(ulang_types, def->decl->pos),
        ulang_type_rtn_type,
        def->decl->pos
    );
    Lang_type_fn rtn_type_ = {0};
    if (!try_lang_type_from_ulang_type_fn(&rtn_type_, new_fn)) {
        return false;
    }
    *type_res = rtn_type_;
    *new_name = name;

    vec_append(&a_main, &env.fun_implementations_waiting_to_resolve, name);

    return true;
}

bool resolve_generics_function_def_implementation(Name name) {
    Name name_plain = name_new(name.mod_path, name.base, (Ulang_type_vec) {0}, name.scope_id);
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
        Uast_block* new_block = uast_block_clone(def->body, def->decl->name.scope_id, true, def->body->pos);
        assert(new_block != def->body);

        Uast_function_decl* new_decl = NULL;
        if (!resolve_generics_serialize_function_decl(&new_decl, def->decl, new_block, name.gen_args, def->decl->pos)) {
            return false;
        }
        Uast_function_def* new_def = uast_function_def_new(new_decl->pos, new_decl, new_block);
        usym_tbl_add(uast_function_def_wrap(new_def));
        return resolve_generics_set_function_def_types(new_def);
    }
    unreachable("");
}

