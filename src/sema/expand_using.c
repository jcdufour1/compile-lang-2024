#include <expand_using.h>
#include <symbol_iter.h>
#include <ast_msg.h>
#include <lang_type_from_ulang_type.h>
#include <ulang_type_get_atom.h>

typedef enum {
    USING_STMT_KEEP,
    USING_STMT_DISCARD,
} EXPAND_USING_STMT;

static void expand_using_block(Uast_block* block);

static void expand_using_using(Uast_using* using) {
    Uast_def* def = NULL;
    if (!usymbol_lookup(&def, using->sym_name)) {
        msg_undefined_symbol(using->sym_name, using->pos);
        return;
    }

    log(LOG_DEBUG, FMT"\n", uast_def_print(def));
    if (def->type == UAST_VARIABLE_DEF) {
        Uast_variable_def* var_def = uast_variable_def_unwrap(def);
        Name lang_type_name = {0};
        name_from_uname(&lang_type_name, ulang_type_get_atom(var_def->lang_type).str, ulang_type_get_pos(var_def->lang_type));
        vec_reset(&lang_type_name.gen_args);
        Uast_def* struct_def_ = NULL;
        log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, lang_type_name));
        unwrap(usymbol_lookup(&struct_def_, lang_type_name));
        // TODO: expected failure case for using `using` on enum, etc.
        Uast_struct_def* struct_def = uast_struct_def_unwrap(struct_def_);
        for (size_t idx = 0; idx < struct_def->base.members.info.count; idx++) {
            Uast_variable_def* curr = vec_at(struct_def->base.members, idx);
            Name alias_name = using->sym_name;
            alias_name.mod_path = using->mod_path_to_put_defs;
            alias_name.base = curr->name.base;
            Uast_lang_def* lang_def = uast_lang_def_new(
                using->pos,
                alias_name,
                uast_member_access_wrap(uast_member_access_new(
                    curr->pos,
                    uast_symbol_new(curr->pos, curr->name),
                    uast_symbol_wrap(uast_symbol_new(using->pos, using->sym_name))
                )),
                true
            );
            if (!usymbol_add(uast_lang_def_wrap(lang_def))) {
                msg_redefinition_of_symbol(uast_lang_def_wrap(lang_def));
            }
        }
        return;
    } else if (def->type == UAST_MOD_ALIAS) {
        Strv mod_path = uast_mod_alias_unwrap(def)->mod_path;
        bool is_builtin = strv_is_equal(MOD_PATH_BUILTIN, using->sym_name.mod_path);
        // TODO: this linear search searches through all mod_paths, which may be slow for large projects.
        //   eventually, it may be a good idea to speed this up 
        //   (eg. by keeping (array of symbols of top level) of each module)

        Usymbol_iter iter = usym_tbl_iter_new(SCOPE_TOP_LEVEL);
        Uast_def* curr = NULL;
        while (usym_tbl_iter_next(&curr, &iter)) {
            Name curr_name = uast_def_get_name(curr);
            if (strv_is_equal(curr_name.mod_path, mod_path)) {
                Name alias_name = using->sym_name;
                alias_name.mod_path = using->mod_path_to_put_defs;
                alias_name.base = curr_name.base;
                alias_name.scope_id = curr_name.scope_id;
                Uast_lang_def* lang_def = uast_lang_def_new(
                    using->pos,
                    alias_name,
                    uast_symbol_wrap(uast_symbol_new(uast_def_get_pos(curr), curr_name)),
                    true
                );
                lang_def->pos.expanded_from = uast_def_get_pos_ref(curr);
                assert(!pos_is_equal(lang_def->pos, (Pos) {0}));
                if (!usymbol_add(uast_lang_def_wrap(lang_def))) {
                    Uast_def* prev_def = NULL;
                    unwrap(usymbol_lookup(&prev_def, lang_def->alias_name));
                    if (prev_def->type != UAST_LANG_DEF || !uast_lang_def_unwrap(prev_def)->is_from_using) {
                        if (!is_builtin || !strv_starts_with(uast_def_get_name(prev_def).mod_path, MOD_PATH_STD)) {
                            msg_redefinition_of_symbol(uast_lang_def_wrap(lang_def));
                        }
                    }
                }
            }
        }
        return;
    } else {
        msg(
            DIAG_USING_ON_NON_STRUCT_OR_MOD_ALIAS,
            using->pos,
            "symbol after `using` must be struct or module alias\n"
        );
        return;
    }
    unreachable("");

}

static EXPAND_USING_STMT expand_using_stmt(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_DEFER:
            return USING_STMT_KEEP;
        case UAST_USING:
            expand_using_using(uast_using_unwrap(stmt));
            return USING_STMT_DISCARD;
        case UAST_EXPR:
            return USING_STMT_KEEP;
        case UAST_DEF:
            expand_using_def(uast_def_unwrap(stmt));
            return USING_STMT_KEEP;
        case UAST_FOR_WITH_COND:
            expand_using_block(uast_for_with_cond_unwrap(stmt)->body);
            return USING_STMT_KEEP;
        case UAST_YIELD:
            return USING_STMT_KEEP;
        case UAST_CONTINUE:
            return USING_STMT_KEEP;
        case UAST_ASSIGNMENT:
            return USING_STMT_KEEP;
        case UAST_RETURN:
            return USING_STMT_KEEP;
        case UAST_STMT_REMOVED:
            return USING_STMT_KEEP;
    }
    unreachable("");
}

static void expand_using_block(Uast_block* block) {
    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        expand_using_def(curr);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Uast_stmt** curr = vec_at_ref(&block->children, idx);
        if (USING_STMT_DISCARD == expand_using_stmt(*curr)) {
            *curr = uast_stmt_removed_wrap(uast_stmt_removed_new(uast_stmt_get_pos(*curr)));
        }
    }
}

void expand_using_def(Uast_def* def) {
    switch (def->type) {
        case UAST_LABEL:
            return;
        case UAST_VOID_DEF:
            return;
        case UAST_POISON_DEF:
            return;
        case UAST_IMPORT_PATH:
            expand_using_block(uast_import_path_unwrap(def)->block);
            return;
        case UAST_MOD_ALIAS:
            return;
        case UAST_GENERIC_PARAM:
            return;
        case UAST_FUNCTION_DEF:
            expand_using_block(uast_function_def_unwrap(def)->body);
            return;
        case UAST_VARIABLE_DEF:
            return;
        case UAST_STRUCT_DEF:
            return;
        case UAST_RAW_UNION_DEF:
            return;
        case UAST_ENUM_DEF:
            return;
        case UAST_LANG_DEF:
            return;
        case UAST_PRIMITIVE_DEF:
            return;
        case UAST_FUNCTION_DECL:
            return;
        case UAST_BUILTIN_DEF:
            return;
    }
    unreachable("");
}
