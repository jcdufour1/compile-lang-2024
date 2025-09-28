#include <do_passes.h>
#include <symbol_iter.h>
#include <msg.h>
#include <ir_utils.h>
#include <name.h>
#include <ulang_type_get_pos.h>
#include <bool_vec.h>

static Bool_vec bool_vec_clone(Bool_vec vec) {
    Bool_vec new_vec = {0};
    vec_extend(&a_main /* TODO */, &new_vec, &vec);
    return new_vec;
}
typedef struct {
    Init_table_vec init_tables;
    Name label_to_cont;
    Bool_vec prev_desisions; // true on element [0] means that is_true path was taken on first cond_goto,
                             // true on element [1] means that is_true path was taken on second cond_goto,
                             // etc.
} Frame;

typedef struct {
    Vec_base info;
    Frame* buf;
} Frame_vec;

static Frame frame_new(Init_table_vec init_tables, Name label_to_cont, Bool_vec prev_desisions) {
    return (Frame) {.init_tables = init_tables, .label_to_cont = label_to_cont, .prev_desisions = prev_desisions};
}

static Init_table init_table_clone(Init_table table) {
    Init_table new_table = {
        .table_tasts = arena_alloc(&a_main /* TODO */, sizeof(new_table.table_tasts[0])*table.capacity), 
        .count = table.count,
        .capacity = table.capacity
    };
    for (size_t idx = 0; idx < table.capacity; idx++) {
        Usymbol_table_tast curr = table.table_tasts[idx];
        new_table.table_tasts[idx] = (Usymbol_table_tast) {.key = curr.key, .status = curr.status};
    }
    return new_table;
}

static Init_table_vec init_table_vec_clone(Init_table_vec vec) {
    Init_table_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main /* TODO */, &new_vec, init_table_clone(vec_at(&vec, idx)));
    }
    return new_vec;
}

static size_t block_idx = 0;
static bool goto_or_cond_goto = false;
static Frame curr_frame = {0};
static Frame_vec frames = {0};
static bool check_unit_src_internal_name_failed = false;

// TODO: rename unit to uninit?
static void check_unit_ir_from_block(const Ir* ir);

static void check_unit_src_internal_name(Name name, Pos pos);

static void check_unit_src(const Name src, Pos pos);

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

static void check_unit_src_internal_binary(const Ir_binary* bin, Pos pos) {
    check_unit_src(bin->lhs, pos);
    check_unit_src(bin->rhs, pos);
}

static void check_unit_src_internal_operator(const Ir_operator* oper, Pos pos) {
    switch (oper->type) {
        case IR_BINARY:
            check_unit_src_internal_binary(ir_binary_const_unwrap(oper), pos);
            return;
        case IR_UNARY:
            todo();
    }
    unreachable("");
}

static void check_unit_src_internal_expr(const Ir_expr* expr, Pos pos) {
    switch (expr->type) {
        case IR_OPERATOR:
            check_unit_src_internal_operator(ir_operator_const_unwrap(expr), pos);
            return;
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
    // TODO: !strv_is_equal(sv("builtin")/* TODO */, name.mod_path) is used to avoid checking implementation symbol
    //   (because otherwise it would be required to modify the add_load_and_store pass or
    //   ignore impossible paths (eg. neither the if nor else is taken).
    //   consider if builtin symbols should be checked or not
    if (!init_symbol_lookup(&curr_frame.init_tables, name) && !strv_is_equal(sv("builtin")/* TODO */, name.mod_path)) {
        msg(DIAG_UNINITIALIZED_VARIABLE, pos, "symbol `"FMT"` is used uninitialized on some or all code paths\n", name_print(NAME_MSG, name));
        for (size_t idx = 0; idx < frames.info.count; idx++) {
            // prevent printing error for the same symbol on several code paths
            init_symbol_add(&vec_at_ref(&frames, idx)->init_tables, name);
        }
        check_unit_src_internal_name_failed = true;
    }
}

static void check_unit_src_internal_ir(const Ir* ir, Pos pos) {
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            check_unit_src_internal_expr(ir_expr_const_unwrap(ir), pos);
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
    if (!init_symbol_add(&curr_frame.init_tables, dest)) {
    }
}

// returns true if the label was found
// TODO: move this function elsewhere, since this function may be useful for other passes?
static size_t label_name_to_block_idx(Ir_vec block_children, Name label) {
    for (size_t idx = 0; idx < block_children.info.count; idx++) {
        // TODO: find a way to avoid O(n) time for finding new block idx 
        //   (eg. by storing approximate idx of block_idx in label definition in eariler pass)
        const Ir* curr = vec_at(&block_children, idx);
        if (curr->type == IR_BLOCK) {
            todo();
        }
        if (curr->type != IR_DEF) {
            continue;
        }
        const Ir_def* curr_def = ir_def_const_unwrap(curr);
        if (curr_def->type != IR_LABEL) {
            continue;
        }
        if (name_is_equal(label, ir_label_const_unwrap(curr_def)->name)) {
            return idx + 1;
        }
    }
    unreachable("label should have been found");
}

static void check_unit_block(const Ir_block* block) {
    assert(frames.info.count == 0);
    assert(block_idx == 0);
    assert(goto_or_cond_goto == false);
    vec_append(&a_main /* TODO: use arena that is reset or freed after this pass */, &frames, frame_new(curr_frame.init_tables, (Name) {0}, (Bool_vec) {0}));

    while (frames.info.count > 0) {
        curr_frame = vec_pop(&frames);
        curr_frame.init_tables = curr_frame.init_tables;
        if (curr_frame.label_to_cont.base.count < 1) {
            assert(block_idx == 0);
        } else {
            // TODO: label_name_to_block_idx adds 1 to result, but 1 is added again at the end of 
            //   this for loop. this may cause bugs?
            block_idx = label_name_to_block_idx(block->children, curr_frame.label_to_cont);
            log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, curr_frame.label_to_cont));
        }


        // TODO: if imports are allowed locally (in functions, etc.), consider how to check those properly
        while (block_idx < block->children.info.count) {
            check_unit_ir_from_block(vec_at(&block->children, block_idx));
            if (check_unit_src_internal_name_failed) {
                String buf = {0};
                size_t temp_block_idx = 0;
                size_t bool_idx = 0;

                uint32_t prev_line = 0;
                while (temp_block_idx < block->children.info.count) {
                    const Ir* curr = vec_at(&block->children, temp_block_idx);
                    if (curr->type == IR_COND_GOTO) {
                        const Ir_cond_goto* cond_goto = ir_cond_goto_const_unwrap(curr);
                        if (cond_goto->pos.line != prev_line) {
                            msg(
                                DIAG_NOTE,
                                cond_goto->pos,
                                "tracing path of uninitialized variable use\n"
                            );
                            prev_line = cond_goto->pos.line;
                        }

                        Name next_label = vec_at(&curr_frame.prev_desisions, bool_idx) ?
                            cond_goto->if_true : cond_goto->if_false;
                        temp_block_idx = label_name_to_block_idx(block->children, next_label);
                        bool_idx++;

                        if (bool_idx >= curr_frame.prev_desisions.info.count) {
                            break;
                        }
                    } else if (curr->type == IR_GOTO) {
                        const Ir_goto* lang_goto = ir_goto_const_unwrap(curr);
                        temp_block_idx = label_name_to_block_idx(block->children, lang_goto->label);
                    } else {
                        temp_block_idx++;
                    }
                }

                assert(bool_idx == curr_frame.prev_desisions.info.count);
            }

            if (goto_or_cond_goto) {
                goto_or_cond_goto = false;
                break;
            }

            block_idx++;
            goto_or_cond_goto = false;
            check_unit_src_internal_name_failed = false;
        }
        block_idx = 0;
    }

    memset(&curr_frame.init_tables, 0, sizeof(curr_frame.init_tables));
}

static void check_unit_import_path(const Ir_import_path* import) {
    check_unit_block(import->block);
}

static void check_unit_function_params(const Ir_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, vec_at(&params->params, idx)->name_self));
        check_unit_dest(vec_at(&params->params, idx)->name_self);
    }
}

static void check_unit_function_decl(const Ir_function_decl* decl) {
    check_unit_function_params(decl->params);
}

static void check_unit_function_def(const Ir_function_def* def) {
    assert(curr_frame.init_tables.info.count == 0);
    // NOTE: decl must be checked before body so that parameters can be set as initialized
    check_unit_function_decl(def->decl);
    check_unit_block(def->body);
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src, store->pos);
    check_unit_dest(store->ir_dest);
    unwrap(init_symbol_add(&curr_frame.init_tables, store->name));
}

// TODO: should Ir_load_another_ir and store_another_ir actually have name member 
//   instead of just loading/storing to another name?
static void check_unit_load_another_ir(const Ir_load_another_ir* load) {
    log(LOG_DEBUG, FMT"\n", ir_load_another_ir_print(load));
    check_unit_src(load->ir_src, load->pos);
    unwrap(init_symbol_add(&curr_frame.init_tables, load->name));
}

static void check_unit_goto(const Ir_goto* lang_goto) {
    goto_or_cond_goto = true;
    vec_append(&a_main /* TODO */, &frames, frame_new(init_table_vec_clone(curr_frame.init_tables)/*TODO: remove this clone */, lang_goto->label, curr_frame.prev_desisions));
}

static void check_unit_cond_goto(const Ir_cond_goto* cond_goto) {
    goto_or_cond_goto = true;
    Bool_vec if_true_decisions = curr_frame.prev_desisions;
    Bool_vec if_false_decisions = bool_vec_clone(curr_frame.prev_desisions);
    vec_append(&a_main, &if_true_decisions, true);
    vec_append(&a_main, &if_false_decisions, false);
    vec_append(&a_main /* TODO */, &frames, frame_new(init_table_vec_clone(curr_frame.init_tables)/*TODO: remove this clone */, cond_goto->if_true, if_true_decisions));
    vec_append(&a_main /* TODO */, &frames, frame_new(init_table_vec_clone(curr_frame.init_tables), cond_goto->if_false, if_false_decisions));
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

static void check_unit_binary(const Ir_binary* bin) {
    check_unit_src(bin->lhs, bin->pos);
    check_unit_src(bin->rhs, bin->pos);
}

static void check_unit_operator(const Ir_operator* oper) {
    switch (oper->type) {
        case IR_UNARY:
            todo();
        case IR_BINARY:
            check_unit_binary(ir_binary_const_unwrap(oper));
            return;
    }
    unreachable("");
}

static void check_unit_literal(const Ir_literal* lit) {
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

static void check_unit_expr(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            check_unit_operator(ir_operator_const_unwrap(expr));
            return;
        case IR_LITERAL:
            check_unit_literal(ir_literal_const_unwrap(expr));
            return;
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
            unreachable("nested blocks should not be present at this point");
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
            check_unit_cond_goto(ir_cond_goto_const_unwrap(ir));
            return;
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
            // TODO: uncomment below unreachable (there should be no block in the top level?)
            //unreachable("");
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
