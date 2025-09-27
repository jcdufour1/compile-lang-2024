#include <do_passes.h>
#include <symbol_iter.h>
#include <msg.h>

static void check_unit_ir_from_block(const Ir* ir);

static void check_unit_src(const Name src) {
    if (!init_symbol_lookup(src)) {
        Ir* sym_def = NULL;
        unwrap(ir_lookup(&sym_def, src));
        log(LOG_DEBUG, FMT"\n", ir_print(sym_def));
        msg(DIAG_UNINITIALIZED_VARIABLE, ir_get_pos(sym_def), "symbol may be used uninitialized\n");
    }
}

static void check_unit_dest(const Name dest) {
    todo();
}

static void check_unit_block(const Ir_block* block) {
    // TODO: if imports are allowed locally (in functions, etc.), consider how to check those properly
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        check_unit_ir_from_block(vec_at(&block->children, idx));
    }
}

static void check_unit_import_path(const Ir_import_path* import) {
    check_unit_block(import->block);
}

static void check_unit_function_def(const Ir_function_def* def) {
    check_unit_block(def->body);
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src);
    check_unit_dest(store->ir_dest);
}

static void check_unit_def(const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            check_unit_function_def(ir_function_def_const_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            // TODO: this could be used to check whether user uses variable before assignment
            return;
        case IR_STRUCT_DEF:
            todo();
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            todo();
        case IR_LABEL:
            todo();
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void check_unit_ir_from_block(const Ir* ir) {
    log(LOG_DEBUG, FMT"\n", ir_print(ir));
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            todo();
        case IR_LOAD_ELEMENT_PTR:
            todo();
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            check_unit_def(ir_def_const_unwrap(ir));
            return;
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return;
        case IR_LOAD_ANOTHER_IR:
            todo();
        case IR_STORE_ANOTHER_IR:
            check_unit_store_another_ir(ir_store_another_ir_const_unwrap(ir));
            return;
        case IR_IMPORT_PATH:
            todo();
        case IR_REMOVED:
            todo();
    }
    unreachable("");
}

static void check_unit_ir_builtin(const Ir* ir) {
    log(LOG_DEBUG, FMT"\n", ir_print(ir));
    switch (ir->type) {
        case IR_BLOCK:
            check_unit_block(ir_block_const_unwrap(ir));
            return;
        case IR_EXPR:
            // TODO
            return;
        case IR_LOAD_ELEMENT_PTR:
            todo();
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            check_unit_def(ir_def_const_unwrap(ir));
            return;
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            // TODO
            return;
        case IR_LOAD_ANOTHER_IR:
            // TODO
            return;
        case IR_STORE_ANOTHER_IR:
            // TODO
            return;
        case IR_IMPORT_PATH:
            check_unit_import_path(ir_import_path_const_unwrap(ir));
            return;
        case IR_REMOVED:
            todo();
    }
    unreachable("");
}

void check_uninitialized(void) {
    Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        check_unit_ir_builtin(curr);
    }
}
