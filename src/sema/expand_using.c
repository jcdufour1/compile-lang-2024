#include <expand_using.h>
#include <symbol_iter.h>

static void expand_using_using(Uast_using* using) {
}

static void expand_using_stmt(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_DEFER:
            todo();
        case UAST_USING:
            expand_using_using(uast_using_unwrap(stmt));
            return;
        case UAST_EXPR:
            todo();
        case UAST_DEF:
            todo();
        case UAST_FOR_WITH_COND:
            todo();
        case UAST_YIELD:
            todo();
        case UAST_CONTINUE:
            todo();
        case UAST_ASSIGNMENT:
            todo();
        case UAST_RETURN:
            todo();
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
        expand_using_stmt(vec_at(&block->children, idx));
    }
}

void expand_using_def(Uast_def* def) {
    switch (def->type) {
        case UAST_LABEL:
            return;
        case UAST_VOID_DEF:
            todo();
        case UAST_POISON_DEF:
            todo();
        case UAST_IMPORT_PATH:
            expand_using_block(uast_import_path_unwrap(def)->block);
            return;
        case UAST_MOD_ALIAS:
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_VARIABLE_DEF:
            todo();
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_LANG_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
    }
    unreachable("");
}
