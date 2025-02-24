
#include "passes.h"
#include <util.h>
#include <llvm_utils.h>
#include <tast_utils.h>
#include <uast_utils.h>
#include <parser_utils.h>
#include <type_checking.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <lang_type_from_ulang_type.h>

// TODO: simplify everything (remove rhs specific thing if possible)

static Tast_symbol* rm_tuple_symbol_typed_new_from_var_def(const Tast_variable_def* var_def) {
    return tast_symbol_new(var_def->pos, (Sym_typed_base) {
        .lang_type = var_def->lang_type,
        .name = var_def->name,
        .llvm_id = 0
    });
}

static Lang_type rm_tuple_lang_type(Env* env, Lang_type lang_type, Pos lang_type_pos);

static Tast_stmt* rm_tuple_stmt(Env* env, Tast_stmt* stmt);

static Tast_expr* rm_tuple_expr_rhs(Env* env, Tast_expr* rhs, Pos assign_pos);

static Tast_stmt* rm_tuple_stmt_rhs(Env* env, Tast_stmt* rhs, Pos assign_pos);

static Tast_block* rm_tuple_block(Env* env, Tast_block* block);

static Tast_for_range* rm_tuple_for_range(Env* env, Tast_for_range* lang_for);

static Tast_for_with_cond* rm_tuple_for_with_cond(Env* env, Tast_for_with_cond* lang_for);

static Tast_stmt* rm_tuple_assignment(Env* env, Tast_assignment* assign);

static Tast_return* rm_tuple_return(Env* env, Tast_return* rtn);

static Tast_if* rm_tuple_if(Env* env, Tast_if* assign);

static Tast_if_else_chain* rm_tuple_if_else_chain(Env* env, Tast_if_else_chain* if_else);

static Tast_expr* rm_tuple_expr_not_in_assignment(Env* env, Tast_expr* expr);

static Tast_variable_def* rm_tuple_variable_def_sum_to_struct(Env* env, Tast_variable_def* def);

static Tast_operator* rm_tuple_operator_not_in_assignment(Env* env, Tast_operator* oper);

static Tast_expr* rm_tuple_symbol_not_in_assignment(Env* env, Tast_symbol* sym);

static Tast_expr* rm_tuple_raw_union_lit_rhs(Env* env, Tast_raw_union_lit* rhs, Pos assign_pos);

static Tast_member_access* rm_tuple_member_access_not_in_assignment(Env* env, Tast_member_access* access);

static Tast_expr* rm_tuple_sum_symbol_not_in_assignment(Env* env, Tast_symbol* sym);

static Tast_expr* rm_tuple_raw_union_symbol_not_in_assignment(Env* env, Tast_symbol* sym);

static Lang_type rm_tuple_lang_type(Env* env, Lang_type lang_type, Pos lang_type_pos) {
    switch (lang_type.type) {
        case LANG_TYPE_SUM: {

            log(LOG_DEBUG, LANG_TYPE_FMT, lang_type_print(lang_type));
            Tast_def* lang_type_def_ = NULL; 
            try(symbol_lookup(&lang_type_def_, env, lang_type_get_str(lang_type)));
            log(LOG_DEBUG, LANG_TYPE_FMT, tast_def_print(lang_type_def_));
            Tast_variable_def_vec members = {0};

            Tast_variable_def* tag = tast_variable_def_new(
                lang_type_pos,
                // TODO: make helper functions, etc. for line below, because this is too much to do every time
                lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0))),
                false,
                str_view_from_cstr("tag")
            );
            vec_append(&a_main, &members, tag);

            Tast_raw_union_def* item_type_def = tast_raw_union_def_new(
                lang_type_pos,
                tast_sum_def_unwrap(lang_type_def_)->base
            );

            item_type_def->base.name = util_literal_name_new_prefix("item_type_def");
            sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(item_type_def));
            for (size_t idx = 0; idx < item_type_def->base.members.info.count; idx++) {
                switch (vec_at(&item_type_def->base.members, idx)->lang_type.type) {
                    case LANG_TYPE_SUM:
                        vec_at(&item_type_def->base.members, idx)->lang_type = rm_tuple_lang_type(
                            env, 
                            vec_at(&item_type_def->base.members, idx)->lang_type,
                            item_type_def->pos
                        );
                        break;
                    case LANG_TYPE_STRUCT:
                        break;
                    case LANG_TYPE_RAW_UNION:
                        break;
                    case LANG_TYPE_PRIMITIVE:
                        break;
                    case LANG_TYPE_ENUM:
                        break;
                    case LANG_TYPE_TUPLE:
                        todo();
                    case LANG_TYPE_VOID:
                        break;
                    default:
                        unreachable("");
                }
            }

            Tast_variable_def* item = tast_variable_def_new(
                lang_type_pos,
                rm_tuple_lang_type(env, lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type_atom_new(item_type_def->base.name, 0))), lang_type_pos),
                false,
                str_view_from_cstr("item")
            );
            vec_append(&a_main, &members, item);

            Struct_def_base base = {
                .members = members,
                .name = util_literal_name_new_prefix("temp")
            };
            
            Tast_struct_def* struct_def = tast_struct_def_new(lang_type_pos, base);
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def)));
            struct_def->base.name = serialize_tast_struct_def(env, struct_def);
            // TODO: consider collisions with generated structs and user defined structs
            log(LOG_DEBUG, TAST_FMT, tast_struct_def_print(struct_def));
            sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def));

            return lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(struct_def->base.name, 0)));
        }
        case LANG_TYPE_RAW_UNION: {

            log(LOG_DEBUG, LANG_TYPE_FMT, lang_type_print(lang_type));
            Tast_def* lang_type_def_ = NULL; 
            try(symbol_lookup(&lang_type_def_, env, lang_type_get_str(lang_type)));
            log(LOG_DEBUG, LANG_TYPE_FMT, tast_def_print(lang_type_def_));
            Tast_variable_def_vec members = {0};

            Tast_variable_def* tag = tast_variable_def_new(
                lang_type_pos,
                // TODO: make helper functions, etc. for line below, because this is too much to do every time
                lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0))),
                false,
                str_view_from_cstr("tag")
            );
            vec_append(&a_main, &members, tag);

            Tast_raw_union_def* item_type_def = tast_raw_union_def_new(
                lang_type_pos,
                tast_raw_union_def_unwrap(lang_type_def_)->base
            );
            item_type_def->base.name = serialize_tast_raw_union_def(env, item_type_def);
            sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_raw_union_def_wrap(item_type_def));

            return lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type_atom_new(item_type_def->base.name, 0)));
        }
        case LANG_TYPE_TUPLE: {
            Tast_variable_def_vec members = {0};

            for (size_t idx = 0; idx < lang_type_tuple_const_unwrap(lang_type).lang_types.info.count; idx++) {
                Tast_variable_def* memb = tast_variable_def_new(
                    lang_type_pos,
                    rm_tuple_lang_type(env, vec_at_const(lang_type_tuple_const_unwrap(lang_type).lang_types, idx), lang_type_pos),
                    false,
                    util_literal_name_new_prefix("tuple_struct_member")
                );
                vec_append(&a_main, &members, memb);
            }

            Struct_def_base base = {
                .members = members,
                .name = serialize_lang_type(env, lang_type)
            };
            // todo: remove untyped things here
            Tast_struct_def* struct_def = tast_struct_def_new(lang_type_pos, base);
            // TODO: consider collisions with generated structs and user defined structs
            log(LOG_DEBUG, STR_VIEW_FMT, tast_struct_def_print(struct_def));
            sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_struct_def_wrap(struct_def));
            return lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0)));
        }
        case LANG_TYPE_PRIMITIVE:
            return lang_type;
        case LANG_TYPE_STRUCT:
            return lang_type;
        case LANG_TYPE_VOID:
            return lang_type;
        case LANG_TYPE_ENUM:
            return lang_type;
        default:
            unreachable(LANG_TYPE_FMT, lang_type_print(lang_type));
    }
    unreachable("");
}

static Tast_expr* rm_tuple_generic_assignment_rhs(
    Env* env,
    Tast_expr* rhs,
    Pos assign_pos
);

static Tast_for_range* rm_tuple_for_range(Env* env, Tast_for_range* lang_for) {
    // TODO: tuples in for lower bound, etc.
    lang_for->body = rm_tuple_block(env, lang_for->body);
    return lang_for;
}

static Tast_for_with_cond* rm_tuple_for_with_cond(Env* env, Tast_for_with_cond* lang_for) {
    // TODO: tuples in for condition, etc.
    lang_for->body = rm_tuple_block(env, lang_for->body);
    return lang_for;
}

static Tast_stmt* rm_tuple_assignment_tuple(Env* env, Tast_assignment* assign) {
    Tast_stmt_vec new_children = {0};

    Tast_tuple* dest = tast_tuple_unwrap(tast_expr_unwrap(assign->lhs));
    Tast_expr* src = assign->rhs;

    Tast_def* struct_def_ = NULL;
    // TODO: think about out of order things
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(rm_tuple_lang_type(env, tast_expr_get_lang_type(src), tast_expr_get_pos(src)))));

    Lang_type new_var_lang_type = rm_tuple_lang_type(env, tast_expr_get_lang_type(src), tast_expr_get_pos(src));

    Tast_variable_def* new_var = tast_variable_def_new(
        tast_stmt_get_pos(assign->lhs),
        new_var_lang_type,
        false,
        util_literal_name_new_prefix("tuple_assign_lhs")
    );
    try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_var)));
    Tast_assignment* new_assign = tast_assignment_new(
        dest->pos,
        tast_def_wrap(tast_variable_def_wrap(new_var)),
        src
    );
    vec_append(&a_main, &new_children, tast_assignment_wrap(new_assign));

    for (size_t idx = 0; idx < dest->members.info.count; idx++) {
        try(idx < tast_struct_def_unwrap(struct_def_)->base.members.info.count);
        Tast_variable_def* curr_memb_def = vec_at(&tast_struct_def_unwrap(struct_def_)->base.members, idx);

        Tast_member_access* curr_src = tast_member_access_new(
            tast_expr_get_pos(src),
            rm_tuple_lang_type(env, curr_memb_def->lang_type, tast_expr_get_pos(src)),
            curr_memb_def->name,
            tast_symbol_wrap(rm_tuple_symbol_typed_new_from_var_def(new_var))
        );

        Tast_expr* curr_dest = rm_tuple_expr_rhs(env, vec_at(&dest->members, idx), tast_expr_get_pos(vec_at(&dest->members, idx)));
        Tast_assignment* curr_assign = tast_assignment_new(
            dest->pos,
            tast_expr_wrap(curr_dest),
            tast_member_access_wrap(curr_src)
        );

        vec_append(&a_main, &new_children, tast_assignment_wrap(curr_assign));
    }

    tast_expr_set_lang_type(src, new_var_lang_type);

    log(LOG_DEBUG, TAST_FMT, tast_block_print(tast_block_new(assign->pos, false, new_children, (Symbol_collection) {0}, assign->pos)));
    return tast_block_wrap(tast_block_new(assign->pos, false, new_children, (Symbol_collection) {0}, assign->pos));
}

static Tast_stmt* rm_tuple_assignment(Env* env, Tast_assignment* assign) {
    log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));

    log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
    //Lang_type thing = rm_tuple_lang_type(env, tast_expr_get_lang_type(assign->rhs), assign->pos);
    //tast_expr_set_lang_type(assign->rhs, thing);
    //log(LOG_DEBUG, LANG_TYPE_FMT, lang_type_print(thing));
    log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
    //log(LOG_DEBUG, LANG_TYPE_FMT, lang_type_print(thing));
    assign->rhs = rm_tuple_expr_rhs(env, assign->rhs, assign->pos);
    log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
    //todo();

    switch (tast_stmt_get_lang_type(assign->lhs).type) {
        case LANG_TYPE_TUPLE:
            return rm_tuple_assignment_tuple(env, assign);
        case LANG_TYPE_SUM:
            assign->lhs = rm_tuple_stmt_rhs(env, assign->lhs, assign->pos);
            log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
            return tast_assignment_wrap(assign);
        case LANG_TYPE_PRIMITIVE:
            return tast_assignment_wrap(assign);
        case LANG_TYPE_STRUCT:
            return tast_assignment_wrap(assign);
        case LANG_TYPE_RAW_UNION:
            return tast_assignment_wrap(assign);
        case LANG_TYPE_ENUM:
            return tast_assignment_wrap(assign);
        case LANG_TYPE_VOID:
            return tast_assignment_wrap(assign);
    }
    unreachable("");
}

static Tast_function_def* rm_tuple_function_def_new(Env* env, Tast_function_decl* decl) {
    log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(decl));
    Tast_block* body = tast_block_new(decl->pos, false, (Tast_stmt_vec) {0}, (Symbol_collection) {0}, decl->pos);

    Tast_def* struct_def_ = NULL;
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(decl->return_type->lang_type)));
    Struct_def_base base = {0};
    Lang_type base_lang_type = {0};
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF:
            base = tast_struct_def_unwrap(struct_def_)->base;
            base_lang_type = lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0)));
            break;
        case TAST_SUM_DEF:
            base = tast_sum_def_unwrap(struct_def_)->base;
            base_lang_type = lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0))); // TODO: change lang_type->str?
            break;
        case TAST_RAW_UNION_DEF:
            base = tast_raw_union_def_unwrap(struct_def_)->base;
            base_lang_type = lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type_atom_new(base.name, 0))); // TODO: change lang_type->str?
            break;
        default:
            unreachable("");
    }
    Tast_variable_def_vec members = base.members;

    vec_append(&a_main, &env->ancesters, &body->symbol_collection);

    Str_view new_var_name = util_literal_name_new_prefix("struct_var_in_tuple_function");
    Tast_variable_def* new_var = tast_variable_def_new(
        decl->pos,
        base_lang_type,
        false,
        new_var_name
    );
    try(symbol_add(env, tast_variable_def_wrap(new_var)));

    vec_append(&a_main, &body->children, tast_def_wrap(tast_variable_def_wrap(new_var)));

    switch (struct_def_->type) {
        case TAST_STRUCT_DEF: {
            for (size_t idx = 0; idx < decl->params->params.info.count; idx++) {
                Tast_member_access* lhs = tast_member_access_new(
                    decl->pos,
                    rm_tuple_lang_type(env, vec_at(&members, idx)->lang_type, vec_at(&members, idx)->pos),
                    vec_at(&members, idx)->name,
                    tast_symbol_wrap(tast_symbol_new(decl->pos, (Sym_typed_base) {
                        .lang_type = rm_tuple_lang_type(env, new_var->lang_type, new_var->pos),
                        .name = new_var->name,
                        .llvm_id = 0
                    }))
                );

                Tast_expr* rhs = rm_tuple_expr_rhs(env, tast_symbol_wrap(tast_symbol_new(decl->pos, (Sym_typed_base) {
                        .lang_type = rm_tuple_lang_type(env, vec_at(&decl->params->params, idx)->lang_type, new_var->pos),
                        .name = vec_at(&decl->params->params, idx)->name,
                        .llvm_id = 0
                    }))
                , POS_BUILTIN);

                Tast_assignment* assign = tast_assignment_new(decl->pos, tast_expr_wrap(tast_member_access_wrap(lhs)), rhs);
                log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
                vec_append(&a_main, &body->children, tast_assignment_wrap(assign));
            }
            break;
        }
        case TAST_RAW_UNION_DEF: {
            log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(decl));
            Tast_member_access* lhs = tast_member_access_new(
                decl->pos,
                rm_tuple_lang_type(env, vec_at(&decl->params->params, 0)->lang_type, decl->pos),
                vec_at(&decl->params->params, 0)->name,
                tast_symbol_wrap(tast_symbol_new(decl->pos, (Sym_typed_base) {
                    .lang_type = rm_tuple_lang_type(env, new_var->lang_type, decl->pos),
                    .name = new_var->name,
                    .llvm_id = 0
                }))
            );

            Tast_symbol* rhs = tast_symbol_new(decl->pos, (Sym_typed_base) {
                .lang_type = rm_tuple_lang_type(env, vec_at(&decl->params->params, 0)->lang_type, decl->pos),
                .name = vec_at(&decl->params->params, 0)->name,
                .llvm_id = 0
            });

            Tast_assignment* assign = tast_assignment_new(decl->pos, tast_expr_wrap(tast_member_access_wrap(lhs)), tast_symbol_wrap(rhs));
            log(LOG_DEBUG, TAST_FMT, tast_member_access_print(lhs));
            log(LOG_DEBUG, TAST_FMT, tast_assignment_print(assign));
            vec_append(&a_main, &body->children, tast_assignment_wrap(assign));
            break;
        }
        default:
            todo();
    }

    Tast_function_def* new_def = tast_function_def_new(decl->pos, decl, body);

    Tast_symbol* rtn_child = tast_symbol_new(decl->pos, (Sym_typed_base) {
        .lang_type = rm_tuple_lang_type(env, new_var->lang_type, decl->pos),
        .name = new_var->name,
        .llvm_id = 0
    });

    Tast_return* new_rtn = tast_return_new(decl->pos, tast_symbol_wrap(rtn_child), true);
    vec_append(&a_main, &body->children, tast_return_wrap(new_rtn));
    
    vec_rem_last(&env->ancesters);

    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, TAST_FMT, tast_function_def_print(new_def));
    return new_def;
}

static Tast_expr* rm_tuple_struct_literal_rhs(
    Env* env,
    Tast_struct_literal* rhs,
    Pos assign_pos
) {
    log(LOG_DEBUG, TAST_FMT, tast_struct_literal_print(rhs));
    //Tast_variable_def* to_rtn = tast_variable_def_new(
    //    rtn->pos,
    //    vec_at(&fun_decl->return_type->lang_type, 0),
    //    false,
    //    util_literal_name_new_prefix("tuple_to_return")
    //);
    //
    Tast_expr_vec members = rhs->members;

    Struct_def_base base = {0};
    Tast_def* struct_def_ = NULL;
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(rhs->lang_type)));
    base = tast_struct_def_unwrap(struct_def_)->base;

    Tast_variable_def_vec struct_def_memb = base.members;

    Tast_expr_vec new_args = {0};

    //try(symbol_lookup(&struct_def_, env, vec_at(&fun_decl->return_type->lang_type, 0).str));
    Tast_variable_def_vec new_params = {0};
    Str_view new_fun_name = util_literal_name_new_prefix("tuple_function");
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF: {
            for (size_t idx = 0; idx < members.info.count; idx++) {
                try(idx < struct_def_memb.info.count);
                Tast_variable_def* curr_def = vec_at(&struct_def_memb, idx);
                //Tast_member_access* access = 

                Tast_variable_def* new_param = tast_variable_def_new(
                    assign_pos,
                    curr_def->lang_type,
                    false,
                    util_literal_name_new_prefix("tuple_function_param")
                );
                log(LOG_DEBUG, TAST_FMT, tast_variable_def_print(new_param));
                try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_param)));
                vec_append(&a_main, &new_params, new_param);
                log(LOG_DEBUG, STR_VIEW_FMT, tast_variable_def_print(new_param));

                Tast_expr* new_memb = rm_tuple_generic_assignment_rhs(
                    env,
                    vec_at(&members, idx),
                    tast_expr_get_pos(vec_at(&members, idx))
                );

                vec_append(&a_main, &new_args, new_memb);
            }
            break;
        }
        default:
            unreachable("");
    }

    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_tast_def(env, struct_def_)));
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(rm_tuple_struct_from_return_type_get_name(struct_def)));

    Tast_function_params* new_fun_params = tast_function_params_new(assign_pos, new_params);

    switch (struct_def_->type) {
         case TAST_STRUCT_DEF: {
            Tast_function_decl* new_fun_decl = tast_function_decl_new(
                assign_pos,
                new_fun_params,
                tast_lang_type_new(
                    assign_pos,
                    lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0)))
                ),
                new_fun_name
            );
            log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(new_fun_decl));
            Tast_function_def* new_fun_def = rm_tuple_function_def_new(env, new_fun_decl);
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_function_def_wrap(new_fun_def)));
            log(LOG_DEBUG, TAST_FMT, tast_function_def_print(new_fun_def));

            Tast_function_call* fun_call = tast_function_call_new(assign_pos, new_args, new_fun_decl->name, new_fun_decl->return_type->lang_type);
            return tast_function_call_wrap(fun_call);
         }
        default:
            todo();
    }

}

static Tast_expr* rm_tuple_tuple_rhs(
    Env* env,
    Tast_tuple* rhs,
    Pos assign_pos
) {
    // TODO: change lang_type_tuple_const_wrap to lang_type_tuple_wrap
    Lang_type lhs_lang_type = rm_tuple_lang_type(env, lang_type_tuple_const_wrap(rhs->lang_type), rhs->pos);
    log(LOG_DEBUG, TAST_FMT, tast_tuple_print(rhs));
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    //Tast_variable_def* to_rtn = tast_variable_def_new(
    //    rtn->pos,
    //    vec_at(&fun_decl->return_type->lang_type, 0),
    //    false,
    //    util_literal_name_new_prefix("tuple_to_return")
    //);
    //
    Tast_expr_vec members = rhs->members;

    Struct_def_base base = {0};
    Tast_def* struct_def_ = NULL;
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(lhs_lang_type)));
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF:
            base = tast_struct_def_unwrap(struct_def_)->base;
            log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
            break;
        default:
            unreachable("");
    }
    Tast_variable_def_vec struct_def_memb = base.members;

    Tast_expr_vec new_args = {0};

    //try(symbol_lookup(&struct_def_, env, vec_at(&fun_decl->return_type->lang_type, 0).str));
    Tast_variable_def_vec new_params = {0};
    Str_view new_fun_name = util_literal_name_new_prefix("tuple_function");
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF:
            for (size_t idx = 0; idx < members.info.count; idx++) {
                try(idx < struct_def_memb.info.count);
                Tast_variable_def* curr_def = vec_at(&struct_def_memb, idx);
                //Tast_member_access* access = 

                Tast_variable_def* new_param = tast_variable_def_new(
                    assign_pos,
                    curr_def->lang_type,
                    false,
                    util_literal_name_new_prefix("tuple_function_param")
                );
                if (sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_param))) {
                    vec_append(&a_main, &new_params, new_param);
                }
                log(LOG_DEBUG, STR_VIEW_FMT, tast_variable_def_print(new_param));

                Tast_expr* new_memb = rm_tuple_generic_assignment_rhs(
                    env,
                    vec_at(&members, idx),
                    tast_expr_get_pos(vec_at(&members, idx))
                );

                vec_append(&a_main, &new_args, new_memb);
            }
            break;
        default:
            unreachable("");
    }

    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_tast_def(env, struct_def_)));
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(rm_tuple_struct_from_return_type_get_name(struct_def)));

    Tast_function_params* new_fun_params = tast_function_params_new(assign_pos, new_params);

    switch (struct_def_->type) {
         case TAST_STRUCT_DEF: {
            Tast_function_decl* new_fun_decl = tast_function_decl_new(
                assign_pos,
                new_fun_params,
                tast_lang_type_new(
                    assign_pos,
                    lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0)))
                ),
                new_fun_name
            );
            log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(new_fun_decl));
            Tast_function_def* new_fun_def = rm_tuple_function_def_new(env, new_fun_decl);
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_function_def_wrap(new_fun_def)));
            log(LOG_DEBUG, TAST_FMT, tast_function_def_print(new_fun_def));

            Tast_function_call* fun_call = tast_function_call_new(assign_pos, new_args, new_fun_decl->name, new_fun_decl->return_type->lang_type);
            log(LOG_DEBUG, TAST_FMT, tast_function_call_print(fun_call));
            return tast_function_call_wrap(fun_call);
         }
        default:
            todo();
    }

}

static Tast_expr* rm_tuple_sum_lit_rhs(
    Env* env,
    Tast_sum_lit* rhs,
    Pos assign_pos
) {
    Lang_type lhs_lang_type = rm_tuple_lang_type(env, rhs->sum_lang_type, rhs->pos);
    log(LOG_DEBUG, TAST_FMT, tast_sum_lit_print(rhs));
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    //Tast_variable_def* to_rtn = tast_variable_def_new(
    //    rtn->pos,
    //    vec_at(&fun_decl->return_type->lang_type, 0),
    //    false,
    //    util_literal_name_new_prefix("tuple_to_return")
    //);
    //
    Tast_expr_vec members = {0};
    Tast_sum_lit* sum_lit = rhs;
    Tast_expr* inner_thing = rm_tuple_expr_rhs(env, sum_lit->item, assign_pos);
    log(LOG_DEBUG, TAST_FMT, tast_sum_lit_print(rhs));
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(sum_lit->item));
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(inner_thing));
    vec_append(&a_main, &members, tast_literal_wrap(tast_enum_lit_wrap(sum_lit->tag)));
    if (tast_expr_get_lang_type(inner_thing).type != LANG_TYPE_VOID) {
        vec_append(&a_main, &members, inner_thing);
    }

    Tast_def* struct_def_ = NULL;
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(lhs_lang_type)));
    Struct_def_base base = tast_struct_def_unwrap(struct_def_)->base;
    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));

    Tast_expr_vec new_args = {0};

    //try(symbol_lookup(&struct_def_, env, vec_at(&fun_decl->return_type->lang_type, 0).str));
    Tast_variable_def_vec new_params = {0};
    Str_view new_fun_name = util_literal_name_new_prefix("tuple_function");
    switch (struct_def_->type) {
        case TAST_STRUCT_DEF: {
            //try(idx < struct_def_memb.info.count);
            //Tast_variable_def* curr_def = vec_at(&struct_def_memb, idx);
            //Tast_member_access* access = 

            Tast_variable_def* new_tag = tast_variable_def_new(
                assign_pos,
                vec_at(&tast_struct_def_unwrap(struct_def_)->base.members, 0)->lang_type,
                false,
                util_literal_name_new_prefix("tuple_function_param_tag")
            );
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_tag)));
            vec_append(&a_main, &new_params, new_tag);
            vec_append(&a_main, &new_args, vec_at(&members, 0));

            Tast_variable_def* new_item = tast_variable_def_new(
                assign_pos,
                vec_at(&tast_struct_def_unwrap(struct_def_)->base.members, 1)->lang_type,
                false,
                util_literal_name_new_prefix("tuple_function_param_item")
            );
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_item)));
            vec_append(&a_main, &new_params, new_item);
            if (tast_expr_get_lang_type(inner_thing).type != LANG_TYPE_VOID) {
                vec_append(&a_main, &new_args, rm_tuple_raw_union_lit_rhs(
                    env,
                    tast_raw_union_lit_new(
                        tast_expr_get_pos(vec_at(&members, 1)),
                        rhs->tag, // TODO: clone this?
                        rm_tuple_lang_type(
                            env,
                            vec_at(&tast_struct_def_unwrap(struct_def_)->base.members, 1)->lang_type,
                            vec_at(&tast_struct_def_unwrap(struct_def_)->base.members, 1)->pos
                        ),
                        rm_tuple_expr_rhs(env, vec_at(&members, 1), tast_expr_get_pos(vec_at(&members, 1)))
                    ),
                    assign_pos
                ));
            }

            log(LOG_DEBUG, TAST_FMT, tast_expr_print(vec_at(&new_args, 0)));
            if (tast_expr_get_lang_type(inner_thing).type == LANG_TYPE_VOID) {
                assert(new_args.info.count == 1);
            } else {
                assert(new_args.info.count == 2);
                log(LOG_DEBUG, TAST_FMT, tast_expr_print(vec_at(&new_args, 1)));
            }
            break;
        }
        default:
            unreachable("");
    }

    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_tast_def(env, struct_def_)));
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(rm_tuple_struct_from_return_type_get_name(struct_def)));

    Tast_function_params* new_fun_params = tast_function_params_new(assign_pos, new_params);

    switch (struct_def_->type) {
        case TAST_STRUCT_DEF: {
            Tast_function_decl* new_fun_decl = tast_function_decl_new(
                assign_pos,
                new_fun_params,
                tast_lang_type_new(
                    assign_pos,
                    lang_type_struct_const_wrap(lang_type_struct_new(lang_type_atom_new(base.name, 0)))
                ),
                new_fun_name
            );
            log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(new_fun_decl));
            Tast_function_def* new_fun_def = rm_tuple_function_def_new(env, new_fun_decl);
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_function_def_wrap(new_fun_def)));
            log(LOG_DEBUG, TAST_FMT, tast_function_def_print(new_fun_def));

            Tast_function_call* fun_call = tast_function_call_new(assign_pos, new_args, new_fun_decl->name, new_fun_decl->return_type->lang_type);
            log(LOG_DEBUG, TAST_FMT, tast_function_call_print(fun_call));
            return tast_function_call_wrap(fun_call);
        }
        default:
            todo();
    }

}

// returns new rhs
static Tast_expr* rm_tuple_generic_assignment_rhs(Env* env, Tast_expr* rhs, Pos assign_pos) {
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(rhs));
    if (rhs->type == TAST_TUPLE) {
        return rm_tuple_tuple_rhs(env, tast_tuple_unwrap(rhs), assign_pos);
    } else if (rhs->type == TAST_STRUCT_LITERAL) {
        return rm_tuple_struct_literal_rhs(env, tast_struct_literal_unwrap(rhs), assign_pos);
    } else if (rhs->type == TAST_LITERAL && tast_literal_unwrap(rhs)->type == TAST_SUM_LIT) {
        return rm_tuple_sum_lit_rhs(env, tast_sum_lit_unwrap(tast_literal_unwrap(rhs)), assign_pos);
    } else if (rhs->type == TAST_SYMBOL && tast_symbol_unwrap(rhs)->base.lang_type.type == LANG_TYPE_SUM) {
        return rm_tuple_sum_symbol_not_in_assignment(env, tast_symbol_unwrap(rhs));
    } else {
        return rm_tuple_expr_rhs(env, rhs, assign_pos);
    }
}

static Tast_return* rm_tuple_return(Env* env, Tast_return* rtn) {
    if (env->name_parent_function.count < 1) {
        return rtn;
    }

    Tast_def* def = NULL;
    try(symbol_lookup(&def, env, env->name_parent_function));
    log(LOG_DEBUG, TAST_FMT, tast_def_print(def));
    Tast_function_def* fun_def = tast_function_def_unwrap(def);

    Tast_def* struct_def_ = NULL;
    if (!symbol_lookup(&struct_def_, env, lang_type_get_str(fun_def->decl->return_type->lang_type))) {
        return rtn;
    }

    rtn->child = rm_tuple_generic_assignment_rhs(
        env,
        rtn->child,
        rtn->pos
    );
    return rtn;
}

static Tast_if* rm_tuple_if(Env* env, Tast_if* lang_if) {
    PARENT_OF old_parent_of = env->parent_of;
    env->parent_of = PARENT_OF_CASE;
    lang_if->condition->child = rm_tuple_operator_not_in_assignment(env, lang_if->condition->child);
    env->parent_of = old_parent_of;

    lang_if->body = rm_tuple_block(env, lang_if->body);

    return lang_if;
}

static Tast_if_else_chain* rm_tuple_if_else_chain(Env* env, Tast_if_else_chain* if_else) {
    Tast_if_vec new_ifs = {0};
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        vec_append(&a_main, &new_ifs, rm_tuple_if(env, vec_at(&if_else->tasts, idx)));
    }

    if_else->tasts = new_ifs;
    return if_else;
}

static Tast_variable_def* rm_tuple_variable_def_sum_to_struct(Env* env, Tast_variable_def* def) {
    def->lang_type = rm_tuple_lang_type(env, def->lang_type, def->pos);
    return def;
}

static Tast_variable_def* rm_tuple_variable_def(Env* env, Tast_variable_def* def) {
    switch (def->lang_type.type) {
        case LANG_TYPE_SUM:
            return rm_tuple_variable_def_sum_to_struct(env, def);
        case LANG_TYPE_STRUCT:
            return def;
        case LANG_TYPE_PRIMITIVE:
            return def;
        case LANG_TYPE_RAW_UNION:
            return def;
        case LANG_TYPE_ENUM:
            return def;
        case LANG_TYPE_TUPLE:
            return def;
        case LANG_TYPE_VOID:
            return def;
    }
    unreachable("");
}

static Tast_function_def* rm_tuple_function_def(Env* env, Tast_function_def* def) {
    Str_view old_fun_name = env->name_parent_function;

    for (size_t idx = 0; idx < def->decl->params->params.info.count; idx++) {
        Tast_variable_def* new_param = rm_tuple_variable_def(env, vec_at(&def->decl->params->params, idx));
        *vec_at_ref(&def->decl->params->params, idx) = new_param;
    }

    env->name_parent_function = def->decl->name;

    Lang_type rtn_type = def->decl->return_type->lang_type;
    env->rm_tuple_parent_fn_lang_type = rtn_type;

    vec_append(&a_main, &env->ancesters, &def->body->symbol_collection);

    def->decl->return_type->lang_type = rm_tuple_lang_type(env, rtn_type, def->decl->return_type->pos);
    
    vec_rem_last(&env->ancesters);

    def->body = rm_tuple_block(env, def->body);

    env->name_parent_function = old_fun_name;

    return def;
}

static Tast_def* rm_tuple_def(Env* env, Tast_def* def) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            return tast_function_def_wrap(rm_tuple_function_def(env, tast_function_def_unwrap(def)));
        case TAST_VARIABLE_DEF:
            return tast_variable_def_wrap(rm_tuple_variable_def(env, tast_variable_def_unwrap(def)));
        case TAST_STRUCT_DEF:
            return def;
        case TAST_RAW_UNION_DEF:
            return def;
        case TAST_ENUM_DEF:
            return def;
        case TAST_SUM_DEF:
            return def;
        case TAST_PRIMITIVE_DEF:
            return def;
        case TAST_LITERAL_DEF:
            return def;
        case TAST_FUNCTION_DECL:
            // TODO: do function decl if tuple is contained within it
            return def;
    }
    unreachable("");
}

static Tast_function_call* rm_tuple_function_call(Env* env, Tast_function_call* fun_call) {
    Uast_def* fun_decl_ = NULL;
    try(usymbol_lookup(&fun_decl_, env, fun_call->name));
    Uast_function_decl* fun_decl = uast_function_decl_unwrap(fun_decl_);
    fun_call->lang_type = rm_tuple_lang_type(env, fun_call->lang_type, fun_call->pos);

    for (size_t idx = 0; idx < MIN(fun_decl->params->params.info.count, fun_call->args.info.count); idx++) {
        Tast_expr** curr = vec_at_ref(&fun_call->args, idx);
        log(LOG_DEBUG, TAST_FMT, tast_expr_print(*curr));
        *curr = rm_tuple_generic_assignment_rhs(
            env,
            *curr,
            tast_expr_get_pos(*curr)
        );
        log(LOG_DEBUG, TAST_FMT, tast_expr_print(*curr));
    }
    return fun_call;
}

static Tast_expr* rm_tuple_raw_union_lit_rhs(Env* env, Tast_raw_union_lit* rhs, Pos assign_pos) {
    log(LOG_DEBUG, TAST_FMT, tast_raw_union_lit_print(rhs));

    Lang_type lhs_lang_type = rm_tuple_lang_type(env, rhs->lang_type, rhs->pos);
    log(LOG_DEBUG, TAST_FMT, tast_raw_union_lit_print(rhs));
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    //Tast_variable_def* to_rtn = tast_variable_def_new(
    //    rtn->pos,
    //    vec_at(&fun_decl->return_type->lang_type, 0),
    //    false,
    //    util_literal_name_new_prefix("tuple_to_return")
    //);
    //
    Tast_expr_vec members = {0};
    Tast_raw_union_lit* raw_union_lit = rhs;
    env->parent_of = PARENT_OF_CASE;
    vec_append(&a_main, &members, raw_union_lit->item);
    env->parent_of = PARENT_OF_NONE;

    Tast_def* struct_def_ = NULL;
    log(LOG_DEBUG, TAST_FMT, lang_type_print(lhs_lang_type));
    try(symbol_lookup(&struct_def_, env, lang_type_get_str(lhs_lang_type)));
    Struct_def_base base = tast_raw_union_def_unwrap(struct_def_)->base;
    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));

    Tast_expr_vec new_args = {0};

    //try(symbol_lookup(&struct_def_, env, vec_at(&fun_decl->return_type->lang_type, 0).str));
    Tast_variable_def_vec new_params = {0};
    Str_view new_fun_name = util_literal_name_new_prefix("tuple_function");
    switch (struct_def_->type) {
        case TAST_RAW_UNION_DEF: {
            //try(idx < struct_def_memb.info.count);
            //Tast_variable_def* curr_def = vec_at(&struct_def_memb, idx);
            //Tast_member_access* access = 

            Tast_variable_def* new_tag = tast_variable_def_new(
                assign_pos,
                vec_at(&tast_raw_union_def_unwrap(struct_def_)->base.members, rhs->tag->data)->lang_type,
                false,
                util_literal_name_new_prefix("tuple_function_param_item")
            );
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_variable_def_wrap(new_tag)));
            vec_append(&a_main, &new_params, new_tag);
            vec_append(&a_main, &new_args, vec_at(&members, 0));
            //vec_append(&a_main, &new_args, tast_operator_wrap(tast_unary_wrap(tast_unary_new(
            //    tast_expr_get_pos(vec_at(&members, 0)),
            //    vec_at(&members, 0),
            //    TOKEN_UNSAFE_CAST,
            //    vec_at(&tast_raw_union_def_unwrap(struct_def_)->base.members, rhs->tag->data)->lang_type
            //))));
            log(LOG_DEBUG, TAST_FMT, tast_expr_print(vec_at(&new_args, 0)));

            assert(new_args.info.count == 1);
            break;
        }
        default:
            unreachable("");
    }

    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def_));
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(serialize_tast_def(env, struct_def_)));
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(rm_tuple_struct_from_return_type_get_name(struct_def)));

    Tast_function_params* new_fun_params = tast_function_params_new(assign_pos, new_params);

    switch (struct_def_->type) {
        case TAST_RAW_UNION_DEF: {
            Tast_function_decl* new_fun_decl = tast_function_decl_new(
                assign_pos,
                new_fun_params,
                tast_lang_type_new(
                    assign_pos,
                    lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type_atom_new(base.name, 0)))
                ),
                new_fun_name
            );
            log(LOG_DEBUG, TAST_FMT, tast_function_decl_print(new_fun_decl));
            Tast_function_def* new_fun_def = rm_tuple_function_def_new(env, new_fun_decl);
            try(sym_tbl_add(&vec_at(&env->ancesters, 0)->symbol_table, tast_function_def_wrap(new_fun_def)));
            log(LOG_DEBUG, TAST_FMT, tast_function_def_print(new_fun_def));

            assert(new_args.info.count == 1);
            Tast_function_call* fun_call = tast_function_call_new(assign_pos, new_args, new_fun_decl->name, new_fun_decl->return_type->lang_type);
            assert(fun_call->args.info.count == 1);
            log(LOG_DEBUG, TAST_FMT, tast_function_call_print(fun_call));
            return tast_function_call_wrap(fun_call);
        }
        default:
            todo();
    }

}

static Tast_expr* rm_tuple_literal_rhs(Env* env, Tast_literal* rhs, Pos assign_pos) {
    switch (rhs->type) {
        case TAST_NUMBER:
            return tast_literal_wrap(rhs);
        case TAST_STRING:
            return tast_literal_wrap(rhs);
        case TAST_VOID:
            return tast_literal_wrap(rhs);
        case TAST_ENUM_LIT:
            return tast_literal_wrap(rhs);
        case TAST_CHAR:
            return tast_literal_wrap(rhs);
        case TAST_SUM_LIT:
            return rm_tuple_sum_lit_rhs(env, tast_sum_lit_unwrap(rhs), assign_pos);
        case TAST_RAW_UNION_LIT:
            return rm_tuple_raw_union_lit_rhs(env, tast_raw_union_lit_unwrap(rhs), assign_pos);
    }
    unreachable("");
}

static Tast_member_access* rm_tuple_sum_access_rhs(Env* env, Tast_sum_access* rhs, Pos assign_pos) {
    log(LOG_DEBUG, TAST_FMT, tast_sum_access_print(rhs));
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(rhs->callee));
    Tast_expr* new_union_callee = rm_tuple_expr_rhs(env, rhs->callee, assign_pos);
    log(LOG_DEBUG, TAST_FMT, tast_expr_print(new_union_callee));

    Tast_def* struct_def = NULL;
    try(symbol_lookup(&struct_def, env, lang_type_get_str(tast_expr_get_lang_type(new_union_callee))));
    log(LOG_DEBUG, TAST_FMT, tast_def_print(struct_def));
    Tast_member_access* new_union = tast_member_access_new(
        assign_pos,
        vec_at(&tast_struct_def_unwrap(struct_def)->base.members, 1)->lang_type,
        str_view_from_cstr("item"),
        new_union_callee
    );
    log(LOG_DEBUG, TAST_FMT, tast_member_access_print(new_union));
    Tast_def* union_def = NULL;
    try(symbol_lookup(&union_def, env, lang_type_get_str(vec_at(&tast_struct_def_unwrap(struct_def)->base.members, 1)->lang_type)));
    log(LOG_DEBUG, TAST_FMT, tast_def_print(union_def));
    Str_view union_memb_name = vec_at(&tast_raw_union_def_unwrap(union_def)->base.members, rhs->tag->data)->name;
    log(LOG_DEBUG, TAST_FMT"\n", str_view_print(union_memb_name));

    Tast_member_access* new_access = tast_member_access_new(
        assign_pos,
        rhs->lang_type,
        union_memb_name,
        tast_member_access_wrap(new_union)
    );

    return rm_tuple_member_access_not_in_assignment(env, new_access);
}

static Tast_expr* rm_tuple_expr_rhs(Env* env, Tast_expr* rhs, Pos assign_pos) {
    log(LOG_DEBUG, "THING321: "TAST_FMT"\n", tast_expr_print(rhs));
    switch (rhs->type) {
        case TAST_OPERATOR:
            return rhs;
        case TAST_SYMBOL:
            return rm_tuple_symbol_not_in_assignment(env, tast_symbol_unwrap(rhs));
        case TAST_MEMBER_ACCESS:
            return rhs;
        case TAST_INDEX:
            return rhs;
        case TAST_LITERAL:
            return rm_tuple_literal_rhs(env, tast_literal_unwrap(rhs), assign_pos);
        case TAST_STRUCT_LITERAL:
            return rm_tuple_generic_assignment_rhs(env, rhs, assign_pos);
        case TAST_TUPLE:
            return rm_tuple_generic_assignment_rhs(env, rhs, assign_pos);
        case TAST_FUNCTION_CALL:
            return tast_function_call_wrap(rm_tuple_function_call(env, tast_function_call_unwrap(rhs)));
        case TAST_SUM_CALLEE:
            unreachable("callee should not make it to this pass");
        case TAST_SUM_CASE:
            todo();
        case TAST_SUM_ACCESS:
            return tast_member_access_wrap(rm_tuple_sum_access_rhs(env, tast_sum_access_unwrap(rhs), assign_pos));
    }
    unreachable("");
}

static Tast_stmt* rm_tuple_stmt_rhs(Env* env, Tast_stmt* rhs, Pos assign_pos) {
    switch (rhs->type) {
        case TAST_EXPR:
            return tast_expr_wrap(rm_tuple_expr_rhs(env, tast_expr_unwrap(rhs), assign_pos));
        case TAST_DEF:
            return tast_def_wrap(tast_variable_def_wrap(rm_tuple_variable_def_sum_to_struct(env, tast_variable_def_unwrap(tast_def_unwrap(rhs)))));
        default:
            unreachable("");
    }
    unreachable("");
}

static Tast_binary* rm_tuple_binary_not_in_assignment(Env* env, Tast_binary* oper) {
    oper->lhs = rm_tuple_expr_not_in_assignment(env, oper->lhs);
    oper->rhs = rm_tuple_expr_not_in_assignment(env, oper->rhs);
    return oper;
}

static Tast_unary* rm_tuple_unary_not_in_assignment(Env* env, Tast_unary* unary) {
    unary->child = rm_tuple_expr_not_in_assignment(env, unary->child);
    return unary;
}

static Tast_operator* rm_tuple_operator_not_in_assignment(Env* env, Tast_operator* oper) {
    switch (tast_operator_get_lang_type(oper).type) {
        case LANG_TYPE_SUM:
            tast_operator_set_lang_type(
                oper,
                lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0)))
            );
            break;
        case LANG_TYPE_ENUM:
            break;
        case LANG_TYPE_PRIMITIVE:
            break;
        case LANG_TYPE_STRUCT:
            break;
        case LANG_TYPE_RAW_UNION:
            break;
        case LANG_TYPE_TUPLE:
            break;
        case LANG_TYPE_VOID:
            break;
        default:
            unreachable("");
    }

    switch (oper->type) {
        case TAST_UNARY:
            return tast_unary_wrap(rm_tuple_unary_not_in_assignment(env, tast_unary_unwrap(oper)));
        case TAST_BINARY:
            return tast_binary_wrap(rm_tuple_binary_not_in_assignment(env, tast_binary_unwrap(oper)));
    }
    unreachable("");
}

static Tast_expr* rm_tuple_sum_symbol_not_in_assignment(Env* env, Tast_symbol* sym) {
    assert(sym->base.lang_type.type == LANG_TYPE_SUM);

    switch (env->parent_of) {
        case PARENT_OF_CASE:
            sym->base.lang_type = rm_tuple_lang_type(env, sym->base.lang_type, sym->pos);
            Tast_member_access* new_access = tast_member_access_new(
                sym->pos,
                lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0))),
                str_view_from_cstr("tag"),
                tast_symbol_wrap(sym)
            );
            return tast_member_access_wrap(new_access);
        case PARENT_OF_NONE:
            sym->base.lang_type = rm_tuple_lang_type(env, sym->base.lang_type, sym->pos);
            return tast_symbol_wrap(sym);
        default:
            unreachable("");
    }
    unreachable("");
}

static Tast_expr* rm_tuple_raw_union_symbol_not_in_assignment(Env* env, Tast_symbol* sym) {
    (void) env;
    return tast_symbol_wrap(sym);
}

static Tast_expr* rm_tuple_symbol_not_in_assignment(Env* env, Tast_symbol* sym) {
    switch (sym->base.lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return tast_symbol_wrap(sym);
        case LANG_TYPE_ENUM:
            return tast_symbol_wrap(sym);
        case LANG_TYPE_STRUCT:
            return tast_symbol_wrap(sym);
        case LANG_TYPE_RAW_UNION:
            return rm_tuple_raw_union_symbol_not_in_assignment(env, sym);
        case LANG_TYPE_SUM:
            return rm_tuple_sum_symbol_not_in_assignment(env, sym);
        case LANG_TYPE_TUPLE:
            return tast_symbol_wrap(sym);
        case LANG_TYPE_VOID:
            return tast_symbol_wrap(sym);
    }
    unreachable("");
}

static Tast_index* rm_tuple_index_not_in_assignment(Env* env, Tast_index* index) {
    index->index = rm_tuple_expr_not_in_assignment(env, index->index);
    index->callee = rm_tuple_expr_not_in_assignment(env, index->callee);
    return index;
}

static Tast_member_access* rm_tuple_member_access_not_in_assignment(Env* env, Tast_member_access* access) {
    access->lang_type = rm_tuple_lang_type(env, access->lang_type, access->pos);
    access->callee = rm_tuple_expr_not_in_assignment(env, access->callee);
    return access;
}

static Tast_expr* rm_tuple_expr_not_in_assignment(Env* env, Tast_expr* expr) {
    switch (expr->type) {
        case TAST_OPERATOR:
            return tast_operator_wrap(rm_tuple_operator_not_in_assignment(env, tast_operator_unwrap(expr)));
        case TAST_SYMBOL:
            return rm_tuple_symbol_not_in_assignment(env, tast_symbol_unwrap(expr));
        case TAST_MEMBER_ACCESS:
            return tast_member_access_wrap(rm_tuple_member_access_not_in_assignment(env, tast_member_access_unwrap(expr)));
        case TAST_INDEX:
            return tast_index_wrap(rm_tuple_index_not_in_assignment(env, tast_index_unwrap(expr)));
        case TAST_LITERAL:
            return expr;
        case TAST_STRUCT_LITERAL:
            unreachable("");
        case TAST_FUNCTION_CALL:
            return tast_function_call_wrap(rm_tuple_function_call(env, tast_function_call_unwrap(expr)));
        case TAST_TUPLE:
            unreachable("");
        case TAST_SUM_CALLEE:
            unreachable("");
        case TAST_SUM_CASE:
            return tast_literal_wrap(tast_enum_lit_wrap(tast_sum_case_unwrap(expr)->tag));
        case TAST_SUM_ACCESS:
            todo();
    }
    unreachable("");
}

static Tast_stmt* rm_tuple_stmt(Env* env, Tast_stmt* stmt) {
    switch (stmt->type) {
        case TAST_BLOCK:
            return tast_block_wrap(rm_tuple_block(env, tast_block_unwrap(stmt)));
        case TAST_EXPR:
            return tast_expr_wrap(rm_tuple_expr_not_in_assignment(env, tast_expr_unwrap(stmt)));
        case TAST_FOR_RANGE:
            return tast_for_range_wrap(rm_tuple_for_range(env, tast_for_range_unwrap(stmt)));
        case TAST_FOR_WITH_COND:
            return tast_for_with_cond_wrap(rm_tuple_for_with_cond(env, tast_for_with_cond_unwrap(stmt)));
        case TAST_IF_ELSE_CHAIN:
            return tast_if_else_chain_wrap(rm_tuple_if_else_chain(env, tast_if_else_chain_unwrap(stmt)));
        case TAST_RETURN:
            return tast_return_wrap(rm_tuple_return(env, tast_return_unwrap(stmt)));
        case TAST_BREAK:
            return stmt;
        case TAST_CONTINUE:
            return stmt;
        case TAST_ASSIGNMENT:
            return rm_tuple_assignment(env, tast_assignment_unwrap(stmt));
        case TAST_DEF:
            return tast_def_wrap(rm_tuple_def(env, tast_def_unwrap(stmt)));
    }
    unreachable("");
}

static Tast* rm_tuple(Env* env, Tast* tast) {
    switch (tast->type) {
        case TAST_STMT:
            return tast_stmt_wrap(rm_tuple_stmt(env, tast_stmt_unwrap(tast)));
        case TAST_LANG_TYPE:
            return tast;
        case TAST_FOR_LOWER_BOUND:
            return tast;
        case TAST_FUNCTION_PARAMS:
            return tast;
        case TAST_FOR_UPPER_BOUND:
            return tast;
        case TAST_CONDITION:
            return tast;
        case TAST_IF:
            return tast_if_wrap(rm_tuple_if(env, tast_if_unwrap(tast)));
    }
    unreachable("");
}

static Tast_block* rm_tuple_block(Env* env, Tast_block* block) {
    Tast_stmt_vec new_children = {0};

    vec_append(&a_main, &env->ancesters, &block->symbol_collection);

    Symbol_table table = vec_top(&env->ancesters)->symbol_table;
    for (size_t idx = 0; idx < table.capacity; idx++) {
        if (table.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }

        rm_tuple_def(env, table.table_tasts[idx].tast);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Tast_stmt* curr = vec_at(&block->children, idx);

        vec_append(&a_main, &new_children, rm_tuple_stmt(env, curr));
        
    }
    vec_rem_last(&env->ancesters);

    block->children = new_children;
    return block;
}

Tast_block* remove_tuples(Env* env, Tast_block* root) {
    Tast_block* new_block = rm_tuple_block(env, root);
    // TODO: use faster algorithm for inserting extra structs and functions
    return new_block;
}
