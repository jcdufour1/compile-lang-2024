#include <do_passes.h>
#include <symbol_iter.h>
#include <msg.h>
#include <ir_utils.h>

// TODO: rename unit to uninit?
static void check_unit_ir_from_block(const Ir* ir);

static void check_unit_src_internal_name(Name name, Pos pos);

static void check_unit_src_internal_literal(const Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            return;
        case IR_FLOAT:
            return;
        case IR_STRING:
            return;
        case IR_VOID:
            return;
        case IR_FUNCTION_NAME:
            return;
    }
    unreachable("");
}

static void check_unit_src_internal_expr(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            todo();
        case IR_LITERAL:
            check_unit_src_internal_literal(ir_literal_const_unwrap(expr));
            return;
        case IR_FUNCTION_CALL:
            todo();
    }
    unreachable("");
}

static void check_unit_src_internal_variable_def(const Ir_variable_def* def) {
    check_unit_src_internal_name(def->name_self, def->pos);
}

static void check_unit_src_internal_def(const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            todo();
        case IR_VARIABLE_DEF:
            check_unit_src_internal_variable_def(ir_variable_def_const_unwrap(def));
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

static void check_unit_src_internal_name(Name name, Pos pos) {
    if (!init_symbol_lookup(name)) {
        msg(DIAG_UNINITIALIZED_VARIABLE, pos, "symbol `"FMT"` may be used uninitialized\n", name_print(NAME_MSG, name));
        Ir* sym_def = NULL;
        unwrap(ir_lookup(&sym_def, name));
        log(LOG_DEBUG, FMT"\n", ir_print(sym_def));
    }
}

static void check_unit_src_internal_ir(const Ir* ir, Pos pos) {
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            check_unit_src_internal_expr(ir_expr_const_unwrap(ir));
            return;
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            check_unit_src_internal_def(ir_def_const_unwrap(ir));
            return;
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_LOAD_ANOTHER_IR:
            // fallthrough
        case IR_STORE_ANOTHER_IR:
            // fallthrough
        case IR_LOAD_ELEMENT_PTR:
            // fallthrough
        case IR_ARRAY_ACCESS:
            // fallthrough
        case IR_ALLOCA:
            check_unit_src_internal_name(ir_tast_get_name(ir), pos);
            return;
        case IR_IMPORT_PATH:
            todo();
        case IR_REMOVED:
            unreachable("");
    }
    unreachable("");
}

static void check_unit_src(const Name src, Pos pos) {
    Ir* sym_def = NULL;
    unwrap(ir_lookup(&sym_def, src));
    check_unit_src_internal_ir(sym_def, pos);
}

static void check_unit_dest(const Name dest) {
    if (!init_symbol_add(dest)) {
    }
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

static void check_unit_function_params(const Ir_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        check_unit_dest(vec_at(&params->params, idx)->name_self);
    }
}

static void check_unit_function_decl(const Ir_function_decl* decl) {
    check_unit_function_params(decl->params);
}

static void check_unit_function_def(const Ir_function_def* def) {
    // NOTE: decl must be checked before body so that parameters can be set as initialized
    check_unit_function_decl(def->decl);
    check_unit_block(def->body);
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src, store->pos);
    check_unit_dest(store->ir_dest);
    unwrap(init_symbol_add(store->name));
}

// TODO: should Ir_load_another_ir and store_another_ir actually have name member 
//   instead of just loading/storing to another name?
static void check_unit_load_another_ir(const Ir_load_another_ir* load) {
    check_unit_src(load->ir_src, load->pos);
    unwrap(init_symbol_add(load->name));
}

static void check_unit_goto(const Ir_goto* lang_goto) {
    log(LOG_DEBUG, FMT"\n", ir_goto_print(lang_goto));
    // TODO: actually do something because we need to trace things in the order that they will execute
    (void) lang_goto;
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
            // TODO
            return;
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void check_unit_function_call(const Ir_function_call* call) {
    check_unit_src(call->callee, call->pos);
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        Name curr = vec_at(&call->args, idx);
        check_unit_src(curr, call->pos /* TODO: call->pos may not always be good enough? */);
    }
    
}

static void check_unit_expr(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            todo();
        case IR_LITERAL:
            todo();
        case IR_FUNCTION_CALL:
            check_unit_function_call(ir_function_call_const_unwrap(expr));
            return;
    }
    unreachable("");
}

static void check_unit_ir_from_block(const Ir* ir) {
    log(LOG_DEBUG, FMT"\n", ir_print(ir));
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            check_unit_expr(ir_expr_const_unwrap(ir));
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
            // NOTE: return is not checked here because the add_load_and_store pass assigns return value 
            //   to other variables that will trigger uninitialized error anyway
            return;
        case IR_GOTO:
            check_unit_goto(ir_goto_const_unwrap(ir));
            return;
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return;
        case IR_LOAD_ANOTHER_IR:
            check_unit_load_another_ir(ir_load_another_ir_const_unwrap(ir));
            return;
        case IR_STORE_ANOTHER_IR:
            check_unit_store_another_ir(ir_store_another_ir_const_unwrap(ir));
            return;
        case IR_IMPORT_PATH:
            todo();
        case IR_REMOVED:
            return;
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
