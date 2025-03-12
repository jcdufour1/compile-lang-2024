#include <generic_sub.h>
#include <ulang_type_serialize.h>
#include <ulang_type.h>
#include <ulang_type_print.h>

void generic_sub_return(Uast_return* rtn, Str_view gen_param, Ulang_type gen_arg) {
    log(LOG_DEBUG, TAST_FMT, uast_return_print(rtn));
    (void) rtn;
    (void) gen_param;
    (void) gen_arg;
}

void generic_sub_param(Uast_param* def, Str_view gen_param, Ulang_type gen_arg) {
    generic_sub_variable_def(def->base, gen_param, gen_arg);
}

void generic_sub_lang_type(
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
) {
    Str_view lang_type_str = ulang_type_regular_const_unwrap(lang_type).atom.str;
    if (str_view_is_equal(gen_param, lang_type_str)) {
        // TODO: call generic_sub_param here
        *new_lang_type = gen_arg;
    } else {
        *new_lang_type = lang_type;
    }
    log(
        LOG_DEBUG, "gen_str: "STR_VIEW_FMT"    rtn_str: "STR_VIEW_FMT"\n",
        str_view_print(gen_param), str_view_print(lang_type_str)
    );

    ulang_type_regular_const_unwrap(*new_lang_type);
    //log(
    //    LOG_DEBUG, "new_lang_type: "STR_VIEW_FMT"\n",
    //    ulang_type_print(*new_lang_type)
    //);
}

void generic_sub_variable_def(Uast_variable_def* def, Str_view gen_param, Ulang_type gen_arg) {
    Str_view reg_param = ulang_type_regular_const_unwrap(def->lang_type).atom.str;
    if (str_view_is_equal(gen_param, reg_param)) {
        def->lang_type = gen_arg;
    }
}

void generic_sub_struct_def_base(Ustruct_def_base* base, Str_view gen_param, Ulang_type gen_arg) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        Str_view memb = ulang_type_regular_const_unwrap(vec_at(&base->members, idx)->lang_type).atom.str;
        if (str_view_is_equal(gen_param, memb)) {
            vec_at(&base->members, idx)->lang_type = gen_arg;
        }
    }
}

void generic_sub_def(Uast_def* def, Str_view gen_param, Ulang_type gen_arg) {
    switch (def->type) {
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_VARIABLE_DEF:
            generic_sub_variable_def(uast_variable_def_unwrap(def), gen_param, gen_arg);
            return;
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_SUM_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

void generic_sub_stmt(Uast_stmt* stmt, Str_view gen_param, Ulang_type gen_arg) {
    switch (stmt->type) {
        case UAST_BLOCK:
            generic_sub_block(uast_block_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_EXPR:
            todo();
        case UAST_DEF:
            todo();
        case UAST_FOR_RANGE:
            todo();
        case UAST_FOR_WITH_COND:
            todo();
        case UAST_BREAK:
            todo();
        case UAST_CONTINUE:
            todo();
        case UAST_ASSIGNMENT:
            todo();
        case UAST_RETURN:
            generic_sub_return(uast_return_unwrap(stmt), gen_param, gen_arg);
            return;
        case UAST_IF_ELSE_CHAIN:
            todo();
        case UAST_SWITCH:
            todo();
    }
    unreachable("");
}

void generic_sub_block(Uast_block* block, Str_view gen_param, Ulang_type gen_arg) {
    Usymbol_table tbl = block->symbol_collection.usymbol_table;
    for (size_t idx = 0; idx < tbl.capacity; idx++) {
        if (tbl.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }

        generic_sub_def(tbl.table_tasts[idx].tast, gen_param, gen_arg);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        generic_sub_stmt(vec_at(&block->children, idx), gen_param, gen_arg);
    }
}
