
#include <do_passes.h>
#include <util.h>
#include <arena.h>
#include <symbol_iter.h>

static Ir* rm_void_block(Ir_block* block);

static Ir* rm_void_function_def(Ir_function_def* def) {
    rm_void_block(def->body);
    return ir_def_wrap(ir_function_def_wrap(def));
}

// TODO: maybe return Ir_def instead?
static Ir* rm_void_def(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            return rm_void_function_def(ir_function_def_unwrap(def));
        case IR_VARIABLE_DEF:
            todo();
        case IR_STRUCT_DEF:
            // TODO
            return ir_def_wrap(def);
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            return ir_def_wrap(def);
        case IR_LABEL:
            return ir_def_wrap(def);
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static Ir* rm_void_expr(Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            todo();
        case IR_LITERAL:
            return ir_expr_wrap(expr);
        case IR_FUNCTION_CALL:
            todo();
    }
    unreachable("");
}

// TODO: do not call variables/parameters "alloca" because it could conflict with library alloca
static Ir* rm_void_alloca(Ir_alloca* alloca) {
    if (alloca->lang_type.type == LLVM_LANG_TYPE_VOID) {
        return ir_removed_wrap(ir_removed_new(alloca->pos));
    }
    return ir_alloca_wrap(alloca);
}

static Ir* rm_void_load_another_ir(Ir_load_another_ir* load) {
    if (load->lang_type.type == LLVM_LANG_TYPE_VOID) {
        return ir_removed_wrap(ir_removed_new(load->pos));
    }
    return ir_load_another_ir_wrap(load);
}

static Ir* rm_void_store_another_ir(Ir_store_another_ir* store) {
    if (store->lang_type.type == LLVM_LANG_TYPE_VOID) {
        return ir_removed_wrap(ir_removed_new(store->pos));
    }
    return ir_store_another_ir_wrap(store);
}

static Ir* rm_void_ir(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            return rm_void_block(ir_block_unwrap(ir));
        case IR_EXPR:
            return rm_void_expr(ir_expr_unwrap(ir));
        case IR_LOAD_ELEMENT_PTR:
            todo();
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            return rm_void_def(ir_def_unwrap(ir));
        case IR_RETURN:
            todo();
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return rm_void_alloca(ir_alloca_unwrap(ir));
        case IR_LOAD_ANOTHER_IR:
            return rm_void_load_another_ir(ir_load_another_ir_unwrap(ir));
        case IR_STORE_ANOTHER_IR:
            return rm_void_store_another_ir(ir_store_another_ir_unwrap(ir));
        case IR_REMOVED:
            return ir;
    }
    unreachable("");
}

static Ir* rm_void_block(Ir_block* block) {
    log(LOG_DEBUG, FMT"\n", ir_block_print(block));
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Ir* curr_ = vec_at(&block->children, idx);
        if (curr_->type != IR_DEF) {
            continue;
        }
        Ir_def* curr = ir_def_unwrap(curr_);
        if (curr->type != IR_VARIABLE_DEF) {
            continue;
        }
        Ir_variable_def* var = ir_variable_def_unwrap(curr);
        if (var->lang_type.type == LLVM_LANG_TYPE_VOID) {
            todo();
            *vec_at_ref(&block->children, idx) = ir_removed_wrap(ir_removed_new(var->pos));
        }
    }

    Alloca_iter iter = ir_tbl_iter_new(block->scope_id);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        rm_void_ir(curr);
    }
    return ir_block_wrap(block);
}

void remove_void_assigns(Ir_block* block) {
    rm_void_block(block);

    Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        rm_void_ir(curr);
    }
}

