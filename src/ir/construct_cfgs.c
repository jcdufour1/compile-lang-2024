#include <do_passes.h>
#include <symbol_iter.h>

static Cfg_node_vec* curr_cfg;
static size_t curr_pos;

static void construct_cfg_ir_from_block(Ir* ir);
static void construct_cfg_ir_from_scope_builtin(Ir* ir);
static void construct_cfg_block(Ir_block* block);

static void construct_cfg_function_def(Ir_function_def* def) {
    construct_cfg_block(def->body);
}

static void construct_cfg_label(Ir_label* label) {
    unwrap(curr_cfg && "construct_cfg_label should only be called from block foreach loop\n");
#   ifndef NDEBUG
        vec_foreach(idx, Cfg_node, node, *curr_cfg) {
            if (node.pos_in_block == curr_pos) {
                unreachable("");
            }
        }}
#   endif // NDEBUG

    vec_append(&a_main, curr_cfg, ((Cfg_node) {.label_name = label->name, .pos_in_block = curr_pos}));
}

static void construct_cfg_cond_goto(Ir_cond_goto* cond_goto) {
    size_t cfg_node_
    todo();
}

static void construct_cfg_def_from_scope_builtin(Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            construct_cfg_function_def(ir_function_def_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            return;
        case IR_STRUCT_DEF:
            return;
        case IR_PRIMITIVE_DEF:
            return;
        case IR_FUNCTION_DECL:
            return;
        case IR_LABEL:
            return;
        case IR_LITERAL_DEF:
            return;
    }
    unreachable("");
}

static void construct_cfg_block(Ir_block* block) {
    assert(!curr_cfg);
    curr_cfg = &block->cfg;

    Size_t_vec succs = {0};
    vec_append(&a_main, &succs, 0);
    vec_append(&a_main, curr_cfg, ((Cfg_node) {.succs = succs, .pos_in_block = CFG_NODE_START_OF_BLOCK}));

    vec_foreach(idx, Ir*, curr, block->children) {
        curr_pos = idx;
        if (curr->type == IR_DEF && ir_def_unwrap(curr)->type == IR_LABEL) {
            construct_cfg_label(ir_label_unwrap(ir_def_unwrap(curr)));
        }
    }}

    vec_foreach(idx, Cfg_node, curr, *curr_cfg) {
        log(LOG_DEBUG, FMT"\n", cfg_node_print(curr));
    }}
    vec_foreach(idx, Ir*, curr, block->children) {
        log(LOG_DEBUG, "%zu: "FMT"\n", idx, ir_print(curr));
    }}

    vec_foreach(idx, Ir*, curr, block->children) {
        curr_pos = idx;
        construct_cfg_ir_from_block(curr);
    }}

    vec_foreach(idx, Cfg_node, curr, *curr_cfg) {
        log(LOG_DEBUG, FMT"\n", cfg_node_print(curr));
    }}
    vec_foreach(idx, Ir*, curr, block->children) {
        log(LOG_DEBUG, "%zu: "FMT"\n", idx, ir_print(curr));
    }}
    todo();

    curr_cfg = NULL;
}

static void construct_cfg_import_path(Ir_import_path* import) {
    construct_cfg_block(import->block);
}

static void construct_cfg_ir_from_block(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            unreachable("blocks should not be nested here");
        case IR_EXPR:
            return;
        case IR_LOAD_ELEMENT_PTR:
            return;
        case IR_ARRAY_ACCESS:
            return;
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            return;
        case IR_RETURN:
            return;
        case IR_GOTO:
            // TODO
            return;
        case IR_COND_GOTO:
            construct_cfg_cond_goto(ir_cond_goto_unwrap(ir));
            return;
        case IR_ALLOCA:
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_STORE_ANOTHER_IR:
            return;
        case IR_IMPORT_PATH:
            unreachable("import path should not be contained within blocks?");
        case IR_REMOVED:
            todo();
    }
    unreachable("");
}

static void construct_cfg_ir_from_scope_builtin(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            construct_cfg_block(ir_block_unwrap(ir));
            return;
        case IR_EXPR:
            return;
        case IR_LOAD_ELEMENT_PTR:
            return;
        case IR_ARRAY_ACCESS:
            return;
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_DEF:
            construct_cfg_def_from_scope_builtin(ir_def_unwrap(ir));
            return;
        case IR_RETURN:
            todo();
        case IR_GOTO:
            // TODO
            return;
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_STORE_ANOTHER_IR:
            return;
        case IR_IMPORT_PATH:
            construct_cfg_import_path(ir_import_path_unwrap(ir));
            return;
        case IR_REMOVED:
            todo();
    }
    unreachable("");
}

void construct_cfgs(void) {
    Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        construct_cfg_ir_from_scope_builtin(curr);
    }
}
