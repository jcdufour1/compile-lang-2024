#include <resolve_generics.h>
#include <type_checking.h>
#include <ulang_type.h>
#include <uast_clone.h>
#include <uast_utils.h>
#include <generic_sub.h>
#include <symbol_log.h>
#include <symbol_iter.h>
#include <uast_expr_to_ulang_type.h>
#include <check_gen_constraints.h>
#include <ulang_type_remove_expr.h>
#include <did_you_mean.h>

static bool is_in_struct_base_def;

// TODO: move this function and macro to better place
static void msg_undefined_type_internal(
    const char* file,
    int line,
    Pos pos,
    Ulang_type lang_type
) {
    if (!env.silent_generic_resol_errors) {
        if (lang_type.type == ULANG_TYPE_REGULAR) {
            Uast_def* dummy = NULL;
            Name base_name = {0};
            if (name_from_uname(&base_name, ulang_type_regular_const_unwrap(lang_type).name, pos)) {
                base_name.gen_args = (Ulang_type_darr) {0};
                if (!usymbol_lookup(&dummy, base_name)) {
                    msg_internal(
                        file, line,
                        DIAG_UNDEFINED_TYPE,
                        pos,
                        "type `"FMT"` is not defined"FMT"\n",
                        name_print(NAME_MSG, base_name),
                        did_you_mean_type_print(base_name)
                    );
                    goto end;
                }
            }
        }

        msg_internal(
            file, line,
            DIAG_UNDEFINED_TYPE,
            pos,
            "type `"FMT"` is not defined\n",
            ulang_type_print(LANG_TYPE_MODE_MSG, lang_type)
        );

end:
        do_nothing();
        // TODO: add name member to all ulang_types so that poison can be added for all of them?
        Name name = {0};
        if (lang_type.type == ULANG_TYPE_REGULAR && name_from_uname(
            &name,
            ulang_type_regular_const_unwrap(lang_type).name,
            pos
        )) {
            usymbol_add(uast_poison_def_wrap(uast_poison_def_new(pos, name)));
        }
    }
}

#define msg_undefined_type(pos, lang_type) \
    msg_undefined_type_internal(__FILE__, __LINE__,  pos, lang_type)

void msg_invalid_count_generic_args_internal(
    const char* file,
    int line,
    Pos pos_def,
    Pos pos_gen_args,
    size_t gen_args_count,
    size_t min_args,
    size_t max_args
) {
    if (env.silent_generic_resol_errors) {
        return;
    }

    String message = {0};
    string_extend_size_t(&a_temp, &message, gen_args_count);
    string_extend_cstr(&a_temp, &message, " generic arguments are passed");
    // TODO: print base type (eg. `Token`)
    string_extend_cstr(&a_temp, &message, ", but ");
    string_extend_size_t(&a_temp, &message, min_args);
    if (max_args > min_args) {
        string_extend_cstr(&a_temp, &message, " or more");
    }
    string_extend_cstr(&a_temp, &message, " generic arguments expected\n");
    msg_internal(
        file, line, DIAG_INVALID_COUNT_GENERIC_ARGS, pos_gen_args,
        FMT, strv_print(string_to_strv(message))
    );

    msg_internal(
        file, line, DIAG_NOTE, pos_def,
        "generic parameters defined here\n" 
    );
}

// TODO: add Pos to Ustruct_def_base
static bool try_set_struct_def_base_types(Struct_def_base* new_base, Ustruct_def_base* base) {
    is_in_struct_base_def = true;
    bool status = true;

    if (base->members.info.count < 1) {
        // TODO
        msg_todo_strv(base->name.base, POS_BUILTIN);
        return false;
        //todo();
    }

    {
        darr_foreach(curr_idx, Uast_generic_param*, curr, base->generics) {
            darr_foreach(prev_idx, Uast_generic_param*, prev, base->generics) {
                if (prev_idx >= curr_idx) {
                    continue;
                }

                if (name_is_equal(curr->name, prev->name)) {
                    if (env.silent_generic_resol_errors) {
                        return false;
                    }

                    msg(
                        DIAG_REDEF_STRUCT_BASE_MEMBER, curr->pos,
                        "redefinition of member `"FMT"`\n",
                        name_print(NAME_MSG, curr->name)
                    );
                    msg(
                        DIAG_NOTE, prev->pos,
                        "member `"FMT"` previously defined here\n",
                        name_print(NAME_MSG, prev->name)
                    );
                    status = false;
                }
            }
        }
    }

    Tast_variable_def_darr new_members = {0};
    {
        for (size_t idx = 0; idx < base->members.info.count; idx++) {
            Uast_variable_def* curr = darr_at(base->members, idx);

            for (size_t prev_idx = 0; prev_idx < idx; prev_idx++) {
                if (name_is_equal(darr_at(base->members, prev_idx)->name, curr->name)) {
                    if (env.silent_generic_resol_errors) {
                        return false;
                    }

                    msg(
                        DIAG_REDEF_STRUCT_BASE_MEMBER, curr->pos,
                        "redefinition of member `"FMT"`\n",
                        name_print(NAME_MSG, curr->name)
                    );
                    msg(
                        DIAG_NOTE, darr_at(base->members, prev_idx)->pos,
                        "member `"FMT"` previously defined here\n",
                        name_print(NAME_MSG, curr->name)
                    );
                    status = false;
                }
            }

            darr_foreach(gen_idx, Uast_generic_param*, gen_param, base->generics) {
                if (name_is_equal(gen_param->name, curr->name)) {
                    if (env.silent_generic_resol_errors) {
                        return false;
                    }

                    msg(
                        DIAG_REDEF_STRUCT_BASE_MEMBER, curr->pos,
                        "redefinition of member `"FMT"`\n",
                        name_print(NAME_MSG, curr->name)
                    );
                    msg(
                        DIAG_NOTE, gen_param->pos,
                        "member `"FMT"` previously defined here\n",
                        name_print(NAME_MSG, curr->name)
                    );
                    status = false;
                }
            }

            Tast_variable_def* new_memb = NULL;
            if (try_set_variable_def_types(&new_memb, curr, false, false)) {
                darr_append(&a_main, &new_members, new_memb);
            } else {
                status = false;
            }
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
    bool success = try_set_struct_def_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_struct_def_wrap(after_res),
        before_res,
        tast_struct_def_wrap(tast_struct_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_raw_union_def_types(Uast_raw_union_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_def_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_raw_union_def_wrap(after_res),
        before_res,
        tast_raw_union_def_wrap(tast_raw_union_def_new(after_res->pos, new_base))
    );
    return success;
}

static bool try_set_enum_def_types(Uast_enum_def* after_res) {
    Struct_def_base new_base = {0};
    bool success = try_set_struct_def_base_types(&new_base, &after_res->base);
    try_set_def_types_internal(
        uast_enum_def_wrap(after_res),
        before_res,
        tast_enum_def_wrap(tast_enum_def_new(after_res->pos, new_base))
    );
    return success;
}

static void resolve_generics_serialize_struct_def_base(
    Ustruct_def_base* new_base,
    Ustruct_def_base old_base,
    Ulang_type_darr gen_args,
    Name new_name
) {
    if (gen_args.info.count < 1) {
        *new_base = old_base;
        return;
    }

    for (size_t idx_memb = 0; idx_memb < old_base.members.info.count; idx_memb++) {
        darr_append(&a_main, &new_base->members, uast_variable_def_clone(darr_at(old_base.members, idx_memb), false, 0));
    }

    darr_foreach_ref(idx_, Ulang_type, gen_arg, gen_args) {
        Ulang_type inner = {0};
        // TODO: remove this unwrap?
        unwrap(ulang_type_remove_expr(&inner, *gen_arg));
        *gen_arg = inner;
    }

    for (size_t idx_gen = 0; idx_gen < gen_args.info.count; idx_gen++) {
        Name gen_def = darr_at(old_base.generics, idx_gen)->name;
        generic_sub_struct_def_base(new_base, gen_def, darr_at(gen_args, idx_gen));
    }

    unwrap(old_base.members.info.count == new_base->members.info.count);

    new_base->name = new_name;
    new_base->generics = old_base.generics;
    return;
}

typedef void*(*Obj_new)(Pos, Ustruct_def_base);

static bool resolve_generics_ulang_type_int_liternal_struct_like(
    Uast_def** after_res,
    Ulang_type* result,
    Ustruct_def_base old_base,
    Ulang_type lang_type, // TODO: change Ulang_type to Ulang_type_regular
    Pos pos_def,
    Obj_new obj_new
) {
    {
        darr_foreach(idx, Ulang_type, gen_arg, ulang_type_regular_const_unwrap(lang_type).name.gen_args) {
            Lang_type dummy = {0};
            if (!try_lang_type_from_ulang_type(&dummy, gen_arg)) {
                return false;
            }
        }
    }

    Name new_name = name_new(
        old_base.name.mod_path,
        old_base.name.base,
        ulang_type_regular_const_unwrap(lang_type).name.gen_args,
        SCOPE_TOP_LEVEL /* TODO */
    );

    if (!struct_like_tbl_lookup(after_res, new_name)) {
        if (env.silent_generic_resol_errors) {
            // this early return is nessessary to avoid storing uasts in struct_like_tbl that have
            //   not been fully run through the expand_def pass
            return false;
        }

        if (old_base.generics.info.count != new_name.gen_args.info.count) {
            msg_invalid_count_generic_args(
                pos_def,
                ulang_type_get_pos(lang_type),
                new_name.gen_args.info.count,
                old_base.generics.info.count,
                old_base.generics.info.count
            );
            return false;
        }

        if (!check_gen_constraints(old_base.generics, new_name.gen_args)) {
            return false;
        }

        Uast_def* new_def_ = NULL;
        if (usymbol_lookup(&new_def_, new_name)) {
            *after_res = new_def_;
        } else {
            Ustruct_def_base new_base = {0};
            // TODO: struct def base is substituted for every encounter of a struct like Lang_type
            //   compilation times could possibly be improved by only making def base sometimes
            resolve_generics_serialize_struct_def_base(&new_base, old_base, new_name.gen_args, new_name);
            // TODO: avoid casting function pointers?
            *after_res = (void*)obj_new(pos_def, new_base);
        }
    }

    *result = ulang_type_regular_const_wrap(ulang_type_regular_new(
        ulang_type_get_pos(lang_type),
        name_to_uname(uast_def_get_struct_def_base(*after_res).name),
        ulang_type_get_pointer_depth(lang_type)
    ));

    Tast_def* dummy = NULL;
    // TODO: remove symbol_lookup?
    if (symbol_lookup(&dummy, new_name)) {
        return true;
    }
    if (struct_like_tbl_add(*after_res)) {
        usym_tbl_update(*after_res);
        darr_append(&a_main, &env.struct_like_waiting_to_resolve, new_name);
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

static bool resolve_generics_ulang_type_int_liternal(LANG_TYPE_TYPE* type, Ulang_type* result, Uast_def* before_res, Ulang_type lang_type) {
    switch (before_res->type) {
        case UAST_RAW_UNION_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_int_liternal_struct_like(
                &after_res_,
                result,
                uast_def_get_struct_def_base(before_res),
                lang_type,
                uast_def_get_pos(before_res),
                local_uast_raw_union_def_new
            )) {
                return false;
            }
            *type = LANG_TYPE_RAW_UNION;
            return true;
        }
        case UAST_ENUM_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_int_liternal_struct_like(
                &after_res_,
                result,
                uast_def_get_struct_def_base(before_res),
                lang_type,
                uast_def_get_pos(before_res),
                local_uast_enum_def_new
            )) {
                return false;
            }
            *type = LANG_TYPE_ENUM;
            return true;
        }
        case UAST_STRUCT_DEF: {
            Uast_def* after_res_ = NULL;
            if (!resolve_generics_ulang_type_int_liternal_struct_like(
                &after_res_,
                result,
                uast_def_get_struct_def_base(before_res),
                lang_type,
                uast_def_get_pos(before_res),
                local_uast_struct_def_new
            )) {
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
            if (env.silent_generic_resol_errors) {
                return false;
            }
            log(LOG_ERROR, FMT"\n", uast_print(UAST_LOG, before_res));
            log(LOG_ERROR, "%d\n", uast_def_get_pos(before_res).line);
            unreachable("def should have been eliminated by now");
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_MOD_ALIAS:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_GENERIC_PARAM:
            // TODO: explain why it is unreachable
            log(LOG_ERROR, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, lang_type));
            unreachable("");
        case UAST_FUNCTION_DEF:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_FUNCTION_DECL:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_VARIABLE_DEF:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_LABEL:
            msg_todo("", ulang_type_get_pos(lang_type));
            return false;
        case UAST_BUILTIN_DEF:
            log(LOG_FATAL, FMT"\n", uast_print(UAST_LOG, before_res));
            msg(DIAG_NOTE, uast_def_get_pos(before_res), "\n");
            unreachable(
                "this should have been removed in expand_lang_def "
                "(or error should have been printed in expand_lang_def)"
            );
            return false;
    }
    unreachable("");
}

bool resolve_generics_ulang_type_regular(LANG_TYPE_TYPE* type, Ulang_type* result, Ulang_type_regular lang_type) {
    Uast_def* before_res = NULL;
    Name name_base = {0};
    if (!name_from_uname(&name_base, lang_type.name, lang_type.pos)) {
        return false;
    }
    assert(name_base.scope_id != SCOPE_NOT);

    memset(&name_base.gen_args, 0, sizeof(name_base.gen_args));
    if (!usymbol_lookup(&before_res, name_base)) {
        msg_undefined_type(lang_type.pos, ulang_type_regular_const_wrap(lang_type));
        return false;
    }

    darr_foreach_ref(idx, Ulang_type, gen_arg, lang_type.name.gen_args) {
        Ulang_type inner = {0};
        if (!ulang_type_remove_expr(&inner, *gen_arg)) {
            return false;
        }
        *gen_arg = inner;
    }

    return resolve_generics_ulang_type_int_liternal(
        type,
        result,
        before_res,
        ulang_type_regular_const_wrap(lang_type)
    );
}

bool resolve_generics_struct_like_def_implementation(Name name) {
    Uast_def* before_res = NULL;
    Name name_before = name_clone(name, false, 0);
    memset(&name_before.gen_args, 0, sizeof(name_before.gen_args));
    unwrap(usym_tbl_lookup(&before_res, name_before));
    Ulang_type dummy = {0};
    Ulang_type lang_type = ulang_type_regular_const_wrap(
        ulang_type_regular_new(
            uast_def_get_pos(before_res),
            name_to_uname(name),
            0
        )
    );

    Uast_def* after_res = NULL;
    // TODO: rename resolve_generics_ulang_type_int_liternal_struct_like?
    if (!resolve_generics_ulang_type_int_liternal_struct_like(
        &after_res,
        &dummy,
        uast_def_get_struct_def_base(before_res),
        lang_type,
        uast_def_get_pos(before_res),
        local_uast_struct_def_new
       )) {
        return false;
    }

    bool status = true;
    Strv old_mod_path_curr_file = env.mod_path_curr_file;
    env.mod_path_curr_file = name.mod_path;

    switch (before_res->type) {
        case UAST_STRUCT_DEF:
            status = try_set_struct_def_types(uast_struct_def_unwrap(after_res));
            break;
        case UAST_RAW_UNION_DEF:
            status = try_set_raw_union_def_types(uast_raw_union_def_unwrap(after_res));
            break;
        case UAST_ENUM_DEF:
            status = try_set_enum_def_types(uast_enum_def_unwrap(after_res));
            break;
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
            log(LOG_FATAL, FMT"\n", uast_print(UAST_LOG, before_res));
            unreachable("");
        case UAST_FUNCTION_DECL:
            unreachable("");
        case UAST_VOID_DEF:
            unreachable("");
        case UAST_LABEL:
            todo();
        case UAST_BUILTIN_DEF:
            todo();
        default:
            unreachable("");
    }

    env.mod_path_curr_file = old_mod_path_curr_file;
    return status;
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
    if (!try_set_block_types(&new_body, def->body, true, false)) {
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
    Ulang_type_darr gen_args,
    Pos pos_gen_args
) {
    memset(new_decl, 0, sizeof(*new_decl));

    if (!check_gen_constraints(old_decl->generics, gen_args)) {
        return false;
    }

    Uast_param_darr params = {0};
    for (size_t idx = 0; idx < old_decl->params->params.info.count; idx++) {
        darr_append(&a_main, &params, uast_param_clone(darr_at(old_decl->params->params, idx), true, new_block->scope_id));
    }

    Ulang_type new_rtn_type = old_decl->return_type;

    {
        darr_foreach_ref(idx_, Ulang_type, gen_arg, gen_args) {
            Ulang_type inner = {0};
            if (!ulang_type_remove_expr(&inner, *gen_arg)) {
                return false;
            }
            *gen_arg = inner;
        }
    }

    size_t args_covered = 0;
    {
        darr_foreach(idx_arg, Ulang_type, gen_arg, gen_args) {
            (void) gen_arg;
            args_covered++;

            if (idx_arg >= old_decl->generics.info.count) {
                msg_invalid_count_generic_args(
                    old_decl->pos,
                    pos_gen_args,
                    gen_args.info.count,
                    old_decl->generics.info.count,
                    old_decl->generics.info.count
                );
                return false;
            }

            for (size_t idx_fun_param = 0; idx_fun_param < params.info.count; idx_fun_param++) {
                Name curr_arg = darr_at(old_decl->generics, idx_arg)->name;
                // TODO: same params are being replaced both here and in generic_sub_block?
                generic_sub_param(
                    darr_at(params, idx_fun_param),
                    curr_arg,
                    darr_at(gen_args, idx_arg)
                );
            }
            Name curr_gen = darr_at(old_decl->generics, idx_arg)->name;
            generic_sub_lang_type(&new_rtn_type, new_rtn_type, curr_gen, darr_at(gen_args, idx_arg));
            generic_sub_block(new_block, curr_gen, darr_at(gen_args, idx_arg));
        }
    }

    if (args_covered < old_decl->generics.info.count) {
        msg_invalid_count_generic_args(
            old_decl->pos,
            pos_gen_args,
            gen_args.info.count,
            old_decl->generics.info.count,
            old_decl->generics.info.count
        );
        return false;
    }

    *new_decl = uast_function_decl_new(
        old_decl->pos,
        (Uast_generic_param_darr) {0},
        uast_function_params_new(old_decl->params->pos, params),
        new_rtn_type,
        name_new(old_decl->name.mod_path, old_decl->name.base, gen_args, scope_get_parent_tbl_lookup(new_block->scope_id))
    );

    return true;
}

static void name_normalize(Uast_generic_param_darr gen_params, Name* name) {
    darr_foreach_ref(idx, Ulang_type, gen_arg, name->gen_args) {
        if (gen_arg->type == ULANG_TYPE_LIT) {
            // TODO: rename ulang_type_lit to ulang_type_lit?
            Ulang_type_lit const_expr = ulang_type_lit_const_unwrap(*gen_arg);
            if (const_expr.type == ULANG_TYPE_STRUCT_LIT) {
                Ulang_type_struct_lit struct_lit = ulang_type_struct_lit_const_unwrap(const_expr);
                unwrap(darr_at(gen_params, idx)->is_expr);
                *gen_arg = ulang_type_lit_const_wrap(ulang_type_struct_lit_const_wrap(ulang_type_struct_lit_new(
                    struct_lit.pos,
                    uast_operator_wrap(uast_unary_wrap(uast_unary_new(struct_lit.pos, struct_lit.expr, UNARY_UNSAFE_CAST, darr_at(gen_params, idx)->expr_lang_type))),
                    struct_lit.pointer_depth
                )));
            }
            
        }
    }
}

bool resolve_generics_function_def_call(
    Lang_type_fn* type_res,
    Name* new_name,
    Uast_function_def* def,
    Ulang_type_darr gen_args, // TODO: remove or refactor name?
    Pos pos_gen_args
) {
    Name name = name_new(def->decl->name.mod_path, def->decl->name.base, gen_args, def->decl->name.scope_id);
    name_normalize(def->decl->generics, &name);
    Name name_plain = name_new(def->decl->name.mod_path, def->decl->name.base, (Ulang_type_darr) {0}, def->decl->name.scope_id);

    // TODO: put pos_gen_args as value in resolved_already_tbl_add?
    Uast_function_decl* cached = NULL;
    if (function_decl_tbl_lookup(&cached, name)) {
        // TODO: consider caching ulang_types
        Ulang_type_darr ulang_types = {0};
        for (size_t idx = 0; idx < cached->params->params.info.count; idx++) {
            darr_append(&a_main, &ulang_types, darr_at(cached->params->params, idx)->base->lang_type);
        }

        Ulang_type* ulang_type_rtn_type = arena_alloc(&a_main, sizeof(*ulang_type_rtn_type));
        *ulang_type_rtn_type = cached->return_type;
        Ulang_type_fn new_fn = ulang_type_fn_new(
            def->decl->pos,
            ulang_type_tuple_new(def->decl->pos, ulang_types, 0),
            ulang_type_rtn_type,
            1/*TODO*/
        );
        if (!try_lang_type_from_ulang_type_fn(type_res, new_fn)) {
            return false;
        }
        *new_name = name;
        return true;
    }

    if (def->decl->generics.info.count != gen_args.info.count) {
        if (!env.supress_type_inference_failures) {
            msg_invalid_count_generic_args(def->pos, pos_gen_args, gen_args.info.count, def->decl->generics.info.count, def->decl->generics.info.count);
        }
        return false;
    }

    Uast_function_decl* decl = uast_function_decl_clone(def->decl, true, def->decl->name.scope_id);
    decl->name = name_plain;
    if (def->decl->generics.info.count > 0) {
        for (size_t idx_gen_param = 0; idx_gen_param < gen_args.info.count; idx_gen_param++) {
            Name gen_param = darr_at(decl->generics, idx_gen_param)->name;
            Ulang_type gen_arg = darr_at(gen_args, idx_gen_param);
            generic_sub_lang_type(&decl->return_type, decl->return_type, gen_param, gen_arg);
            for (size_t idx_param = 0; idx_param < decl->params->params.info.count; idx_param++) {
                Uast_param* param = darr_at(decl->params->params, idx_param);
                generic_sub_param(param, gen_param, gen_arg);
            }
        }
    }
    decl->name = name;
    Uast_function_decl* dummy = NULL;
    (void) dummy;
    unwrap(function_decl_tbl_add(decl));
    assert(function_decl_tbl_lookup(&dummy, decl->name));

    // TODO: consider caching ulang_types
    Ulang_type_darr ulang_types = {0};
    for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
        darr_append(&a_main, &ulang_types, darr_at(decl->params->params, idx)->base->lang_type);
    }

    Ulang_type* ulang_type_rtn_type = arena_alloc(&a_main, sizeof(*ulang_type_rtn_type));
    *ulang_type_rtn_type = decl->return_type;
    Ulang_type_fn new_fn = ulang_type_fn_new(
        def->decl->pos,
        ulang_type_tuple_new(def->decl->pos, ulang_types, 0),
        ulang_type_rtn_type,
        1/*TODO*/
    );
    Lang_type_fn rtn_type_ = {0};
    if (!try_lang_type_from_ulang_type_fn(&rtn_type_, new_fn)) {
        return false;
    }
    *type_res = rtn_type_;
    *new_name = name;

    darr_append(&a_main, &env.fun_implementations_waiting_to_resolve, name);

    return true;
}

bool resolve_generics_function_def_implementation(Name name) {
    Name name_plain = name_new(name.mod_path, name.base, (Ulang_type_darr) {0}, name.scope_id);
    Tast_def* dummy_2 = NULL;
    Uast_function_decl* dummy_3 = NULL;
    unwrap(
        !symbol_lookup(&dummy_2, name) &&
        "same function has been passed to resolve_generics_function_def_implementation "
        "more than once, and it should not have been"
    );

    Uast_def* result = NULL;
    if (usymbol_lookup(&result, name)) {
        // we only need to type check this function
        Strv old_mod_path_curr_file = env.mod_path_curr_file;
        env.mod_path_curr_file = name.mod_path;
        bool status = resolve_generics_set_function_def_types(uast_function_def_unwrap(result));
        env.mod_path_curr_file = old_mod_path_curr_file;
        return status;
    } else {
        // we need to make new uast function implementation and then type check it
        unwrap(usymbol_lookup(&result, name_plain));
        unwrap(function_decl_tbl_lookup(&dummy_3, name));
        Uast_function_def* def = uast_function_def_unwrap(result);
        Uast_block* new_block = uast_block_clone(def->body, true, def->decl->name.scope_id, def->body->pos);
        assert(new_block != def->body);

        Strv old_mod_path_curr_file = env.mod_path_curr_file;
        env.mod_path_curr_file = name.mod_path;
        Uast_function_decl* new_decl = NULL;
        assert(env.mod_path_curr_file.count > 0);
        bool status = resolve_generics_serialize_function_decl(&new_decl, def->decl, new_block, name.gen_args, def->decl->pos);
        if (!status) {
            env.mod_path_curr_file = old_mod_path_curr_file;
            return false;
        }
        
        Uast_function_def* new_def = uast_function_def_new(new_decl->pos, new_decl, new_block);
        usym_tbl_add(uast_function_def_wrap(new_def));
        assert(env.mod_path_curr_file.count > 0);
        status = resolve_generics_set_function_def_types(new_def);
        env.mod_path_curr_file = old_mod_path_curr_file;
        return status;
    }
    unreachable("");
}

