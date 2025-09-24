
#include <do_passes.h>
#include <util.h>
#include <arena.h>
#include <symbol_iter.h>

#define rm_void_internal(item, wrap_fn) \
    do { \
        if ((item)->lang_type.type == LLVM_LANG_TYPE_VOID) { \
            return ir_removed_wrap(ir_removed_new((item)->pos)); \
        } \
        return (wrap_fn)(item); \
    } while (0)

#define rm_void_internal_2(item, wrap_fn_inner, wrap_fn_outer) \
    do { \
        if ((item)->lang_type.type == LLVM_LANG_TYPE_VOID) { \
            return ir_removed_wrap(ir_removed_new((item)->pos)); \
        } \
        return wrap_fn_outer(wrap_fn_inner(item)); \
    } while (0)

static Ir* rm_void_block(Ir_block* block);

static Ir* rm_void_function_def(Ir_function_def* def) {
    rm_void_block(def->body);
    return ir_def_wrap(ir_function_def_wrap(def));
}

static Ir* rm_void_variable_def(Ir_variable_def* def) {
    rm_void_internal_2(def, ir_variable_def_wrap, ir_def_wrap);
}

// TODO: maybe return Ir_def instead?
static Ir* rm_void_def(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            return rm_void_function_def(ir_function_def_unwrap(def));
        case IR_VARIABLE_DEF:
            return rm_void_variable_def(ir_variable_def_unwrap(def));
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
            // TODO
            return ir_expr_wrap(expr);
        case IR_LITERAL:
            return ir_expr_wrap(expr);
        case IR_FUNCTION_CALL:
            // TODO
            return ir_expr_wrap(expr);
    }
    unreachable("");
}

// TODO: do not call variables/parameters "alloca" because it could conflict with library alloca
static Ir* rm_void_alloca(Ir_alloca* alloca) {
    rm_void_internal(alloca, ir_alloca_wrap);
}

static Ir* rm_void_load_another_ir(Ir_load_another_ir* load) {
    rm_void_internal(load, ir_load_another_ir_wrap);
}

static Ir* rm_void_store_another_ir(Ir_store_another_ir* store) {
    rm_void_internal(store, ir_store_another_ir_wrap);
}

static Ir* rm_void_ir(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            return rm_void_block(ir_block_unwrap(ir));
        case IR_EXPR:
            return rm_void_expr(ir_expr_unwrap(ir));
        case IR_LOAD_ELEMENT_PTR:
            // TODO
            return ir;
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            return rm_void_def(ir_def_unwrap(ir));
        case IR_RETURN:
            // TODO
            return ir;
        case IR_GOTO:
            return ir;
        case IR_COND_GOTO:
            // TODO
            return ir;
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
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        *vec_at_ref(&block->children, idx) = rm_void_ir(vec_at(&block->children, idx));
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

