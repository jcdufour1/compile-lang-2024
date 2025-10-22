#include <do_passes.h>
#include <symbol_iter.h>
#include <msg.h>
#include <ir_utils.h>
#include <name.h>
#include <ulang_type_get_pos.h>
#include <bool_vec.h>
#include <symbol_log.h>

static void check_unit_ir_builtin(const Ir* ir);

static Bool_vec bool_vec_clone(Bool_vec vec) {
    Bool_vec new_vec = {0};
    vec_extend(&a_main /* TODO */, &new_vec, &vec);
    return new_vec;
}

static bool init_table_is_equal(Init_table a, Init_table b) {
    if (a.count != b.count) {
        //log(LOG_INFO, "thing 325: no 2.1: %zu %zu\n", a.count, b.count);
        return false;
    }
    if (a.capacity != b.capacity) {
        //log(LOG_INFO, "thing 325: no 2.2\n");
        return false;
    }

    for (size_t idx = 0; idx < a.capacity; idx++) {
        if (a.table_tasts[idx].status != b.table_tasts[idx].status) {
            //log(LOG_INFO, "thing 325: no 2.3\n");
            return false;
        }
        if (a.table_tasts[idx].status == SYM_TBL_OCCUPIED && !strv_is_equal(a.table_tasts[idx].key, b.table_tasts[idx].key)) {
            //log(LOG_INFO, "thing 325: no 2.4\n");
            return false;
        }
    }

    //log(LOG_INFO, "thing 325: yes 2\n");
    return true;
}

static bool init_table_is_subset(Init_table superset, Init_table subset) {
    //if (a.count != b.count) {
    //    log(LOG_INFO, "thing 325: no 2.1: %zu %zu\n", a.count, b.count);
    //    return false;
    //}
    //if (a.capacity != b.capacity) {
    //    log(LOG_INFO, "thing 325: no 2.2\n");
    //    return false;
    //}

    for (size_t idx = 0; idx < subset.capacity; idx++) {
        void* dummy = NULL;
        if (subset.table_tasts[idx].status == SYM_TBL_OCCUPIED && !generic_tbl_lookup(&dummy, (Generic_symbol_table*)&superset, subset.table_tasts[idx].key)) {
            return false;
        }
    }

    return true;
}

static bool init_table_vec_is_equal(Init_table_vec a, Init_table_vec b) {
    if (a.info.count != b.info.count) {
        //log(LOG_INFO, "thing 324: no 1\n");
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!init_table_is_equal(vec_at(&a, idx), vec_at(&b, idx))) {
            //log(LOG_INFO, "thing 324: no 2\n");
            return false;
        }
    }

    //log(LOG_INFO, "thing 324: yes\n");
    return true;
}

static bool init_table_vec_is_subset(Init_table_vec superset, Init_table_vec subset) {
    if (superset.info.count < subset.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < subset.info.count; idx++) {
        if (!init_table_is_subset(vec_at(&superset, idx), vec_at(&subset, idx))) {
            //log(LOG_INFO, "thing 324: no 2\n");
            return false;
        }
    }

    //log(LOG_INFO, "thing 324: yes\n");
    return true;
}

typedef struct {
    Init_table_vec init_tables;
} Frame;

typedef struct {
    Vec_base info;
    Frame* buf;
} Frame_vec;

static Frame frame_new(Init_table_vec init_tables) {
    return (Frame) {.init_tables = init_tables};
}

static Init_table init_table_clone(Init_table table) {
    (void) table;
    todo();
    //Init_table new_table = {
    //    .table_tasts = arena_alloc(&a_main /* TODO */, sizeof(new_table.table_tasts[0])*table.capacity), 
    //    .count = table.count,
    //    .capacity = table.capacity
    //};
    //for (size_t idx = 0; idx < table.capacity; idx++) {
    //    Usymbol_table_tast curr = table.table_tasts[idx];
    //    new_table.table_tasts[idx] = (Usymbol_table_tast) {.key = curr.key, .status = curr.status};
    //}
    //return new_table;
}

static Init_table_vec init_table_vec_clone(Init_table_vec vec) {
    Init_table_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main /* TODO */, &new_vec, init_table_clone(vec_at(&vec, idx)));
    }
    return new_vec;
}

static bool at_end_of_cfg_node = false;
//static Frame_vec already_run_frames = {0};
static bool check_unit_src_internal_name_failed = false;

static size_t block_pos = {0};
static size_t frame_idx = 0;
static Frame* curr_frame = NULL;
static Frame_vec frames = {0};
static bool print_errors_for_unit;

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

static void check_unit_src_internal_function_call(const Ir_function_call* call, Pos pos) {
    check_unit_src(call->callee, pos);
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        Name curr = vec_at(&call->args, idx);
        check_unit_src(curr, pos);
    }
}

static void check_unit_src_internal_binary(const Ir_binary* bin, Pos pos) {
    check_unit_src(bin->lhs, pos);
    check_unit_src(bin->rhs, pos);
}

static void check_unit_src_internal_unary(const Ir_unary* unary, Pos pos) {
    check_unit_src(unary->child, pos);
}

static void check_unit_src_internal_operator(const Ir_operator* oper, Pos pos) {
    switch (oper->type) {
        case IR_BINARY:
            check_unit_src_internal_binary(ir_binary_const_unwrap(oper), pos);
            return;
        case IR_UNARY:
            check_unit_src_internal_unary(ir_unary_const_unwrap(oper), pos);
            return;
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
            check_unit_src_internal_function_call(ir_function_call_const_unwrap(expr), pos);
            return;
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

// TODO: make helper function in ir_utils or similar (or figure out if I do already and clean it up)
static bool check_unit_is_struct(Name name) {
    Ir* def_ = NULL;
    unwrap(ir_lookup(&def_, name));
    if (def_->type != IR_ALLOCA) {
        return false;
    } 
    return ir_alloca_const_unwrap(def_)->lang_type.type == IR_LANG_TYPE_STRUCT;
}

static void check_unit_src_internal_name(Name name, Pos pos) {
    // TODO: !strv_is_equal(sv("builtin")/* TODO */, name.mod_path) is used to avoid checking implementation symbol
    //   (because otherwise it would be required to modify the add_load_and_store pass or
    //   ignore impossible paths (eg. neither the if nor else is taken).
    //   consider if builtin symbols should be checked or not
    if (!print_errors_for_unit) {
        return;
    }

    if (strv_is_equal(sv("builtin")/* TODO */, name.mod_path)) {
        return;
    }

    Init_table_node* result = NULL;
    if (init_symbol_lookup(&curr_frame->init_tables, &result, name)) {
        if (result->cfg_node_of_init != frame_idx || result->block_pos_of_init < block_pos) {
            // symbol was initialized before this use
            return;
        }
    }

    if (check_unit_is_struct(name)) {
        msg(
            DIAG_UNINITIALIZED_VARIABLE, pos,
            "symbol `"FMT"` is used uninitialized on some or all code paths; "
            "note that struct must be initialized with a struct literal or function call "
            "(individual member accesses do not satisfy initialization requirements)\n", name_print(NAME_MSG, name));
    } else {
        msg(DIAG_UNINITIALIZED_VARIABLE, pos, "symbol `"FMT"` is used uninitialized on some or all code paths\n", name_print(NAME_MSG, name));
    }

    for (size_t idx = 0; idx < frames.info.count; idx++) {
        // prevent printing error for the same symbol on several code paths
        init_symbol_add(&vec_at_ref(&frames, idx)->init_tables, (Init_table_node) {
            .name = name,
            .cfg_node_of_init = idx,
            .block_pos_of_init = 0,
        });
    }

    check_unit_src_internal_name_failed = true;
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
    if (!init_symbol_add(&curr_frame->init_tables, (Init_table_node) {
        .name = dest,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    })) {

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
    log(LOG_ERROR, FMT"\n", name_print(NAME_LOG, label));
    unreachable("label should have been found");
}

static void check_unit_block(const Ir_block* block, bool is_main /* TODO: remove */) {
    (void) is_main;
    static int count = 0;
    log(LOG_DEBUG, "%d\n", count);
    log(LOG_DEBUG, FMT"\n", ir_block_print(block));
    vec_foreach(idx, Ir*, curr, block->children) {//{
        log(LOG_DEBUG, "%zu: "FMT"\n", idx, ir_print(curr));
    }}

    // do one iter on cfg for now

    print_errors_for_unit = false;

    log(LOG_DEBUG, "%zu\n", frames.info.count);
    assert(frames.info.count == 0);
    assert(curr_frame);
    vec_foreach(idx, Cfg_node, curr, block->cfg) {//{
        (void) curr;
        if (idx == 0) {
            vec_append(&a_main /* TODO */, &frames, *curr_frame);
            continue;
        }
        vec_append(&a_main /* TODO */, &frames, ((Frame) {0}));
    }}

    // TODO: keep running this for loop until there are no changes
    for (size_t idx = 0; idx < 30; idx++) {
        vec_foreach(idx, Cfg_node, curr, block->cfg) {//{
            frame_idx = idx;
            curr_frame = vec_at_ref(&frames, idx);

            // TODO: make function, etc. to detect if we are at end of cfg node so that at_end_of_cfg_node 
            //   global variable will not be needed for several ir passes
            //   (this could be done by making special foreach macros/functions?)
            at_end_of_cfg_node = false;

            for (size_t block_idx = curr.pos_in_block; !at_end_of_cfg_node; block_idx++) {
                block_pos = block_idx;
                check_unit_ir_from_block(vec_at(&block->children, block_idx));

                if (
                    block_idx + 1 < block->children.info.count &&
                    ir_is_label(vec_at(&block->children, block_idx + 1))
                ) {
                    at_end_of_cfg_node = true;
                }

                if (block_idx + 1 >= block->children.info.count) {
                    at_end_of_cfg_node = true;
                }
            }

            //vec_foreach(succ_idx, size_t, succ, curr.succs) {/*{*/
            //    vec_foreach(frame_idx, Init_table, curr_table, curr_frame->init_tables) {/*{*/
            //        Init_table_iter iter = init_tbl_iter_new_table(curr_table);
            //        Init_table_node curr_in_tbl = {0};
            //        while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
            //            Frame* succ_frame = vec_at_ref(&frames, succ);
            //            Init_table_node* dummy = NULL;
            //            if (!init_symbol_lookup(&succ_frame->init_tables, &dummy, curr_in_tbl.name)) {
            //                unwrap(init_symbol_add(&succ_frame->init_tables, curr_in_tbl));
            //            }
            //        }
            //    }}
            //}}

            // TODO: make function to iterate over Init_table_vec automatically
            if (curr.preds.info.count > 0) {
                size_t pred_0 = vec_at(&curr.preds, 0);
                vec_foreach(frame_idx, Init_table, curr_table, vec_at_ref(&frames, pred_0)->init_tables) {/*{*/
                    Init_table_iter iter = init_tbl_iter_new_table(curr_table);
                    Init_table_node curr_in_tbl = {0};
                    bool is_init_in_pred = true;
                    while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
                        Init_table_node* dummy = NULL;
                        if (init_symbol_lookup(&curr_frame->init_tables, &dummy, curr_in_tbl.name)) {
                            continue;
                        }

                        vec_foreach(pred_idx, size_t, pred, curr.preds) {/*{*/
                            Frame* pred_frame = vec_at_ref(&frames, pred);
                            Init_table_node* dummy = NULL;
                            if (!init_symbol_lookup(&pred_frame->init_tables, &dummy, curr_in_tbl.name)) {
                                is_init_in_pred = false;
                                break;
                            }
                        }}

                        if (is_init_in_pred) {
                            unwrap(init_symbol_add(&curr_frame->init_tables, curr_in_tbl));
                        }
                    }
                }}
            }

        }}

    }

    print_errors_for_unit = true;
    vec_foreach(idx, Cfg_node, curr, block->cfg) {//{
        frame_idx = idx;
        curr_frame = vec_at_ref(&frames, idx);

        // TODO: make function, etc. to detect if we are at end of cfg node so that at_end_of_cfg_node 
        //   global variable will not be needed for several ir passes
        //   (this could be done by making special foreach macros/functions?)
        at_end_of_cfg_node = false;

        for (size_t block_idx = curr.pos_in_block; !at_end_of_cfg_node; block_idx++) {
            block_pos = block_idx;
            check_unit_ir_from_block(vec_at(&block->children, block_idx));

            if (
                block_idx + 1 < block->children.info.count &&
                ir_is_label(vec_at(&block->children, block_idx + 1))
            ) {
                at_end_of_cfg_node = true;
            }

            if (block_idx + 1 >= block->children.info.count) {
                at_end_of_cfg_node = true;
            }
        }
    }}

    frames = (Frame_vec) {0};
    curr_frame = arena_alloc(&a_main /* todo */, sizeof(*curr_frame));
}

//static void check_unit_block(const Ir_block* block) {
//    assert(frames.info.count == 0);
//    assert(block_idx == 0);
//    assert(goto_or_cond_goto == false);
//    vec_append(&a_main /* TODO: use arena that is reset or freed after this pass */, &frames, frame_new(curr_frame.init_tables, (Name) {0}, (Bool_vec) {0}));
//
//    while (frames.info.count > 0) {
//        bool frame_not_needed = false;
//        curr_frame = vec_pop(&frames);
//        for (size_t idx = 0; idx < already_run_frames.info.count; idx++) {
//            Frame curr_already = vec_at(&already_run_frames, idx);
//            // TODO: init_table_vec_is_equal: look at subset/superset instead of just is_equal
//            //   init_table_vec_is_subset
//            if (
//                name_is_equal(curr_already.label_to_cont, curr_frame.label_to_cont) &&
//                init_table_vec_is_subset(curr_frame.init_tables, curr_already.init_tables)
//            ) {
//                frame_not_needed = true;
//                break;
//            }
//        }
//
//        if (frame_not_needed) {
//            continue;
//        }
//        vec_append(&a_main /* TODO */, &already_run_frames, curr_frame);
//
//        curr_frame.init_tables = curr_frame.init_tables;
//        if (curr_frame.label_to_cont.base.count < 1) {
//            assert(block_idx == 0);
//        } else {
//            // TODO: label_name_to_block_idx adds 1 to result, but 1 is added again at the end of 
//            //   this for loop. this may cause bugs?
//            block_idx = label_name_to_block_idx(block->children, curr_frame.label_to_cont);
//            log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, curr_frame.label_to_cont));
//        }
//
//
//        // TODO: if imports are allowed locally (in functions, etc.), consider how to check those properly
//        while (block_idx < block->children.info.count) {
//            check_unit_ir_from_block(vec_at(&block->children, block_idx));
//            if (check_unit_src_internal_name_failed) {
//                size_t temp_block_idx = 0;
//                size_t bool_idx = 0;
//
//                uint32_t prev_line = 0;
//                while (temp_block_idx < block->children.info.count) {
//                    const Ir* curr = vec_at(&block->children, temp_block_idx);
//                    if (curr->type == IR_COND_GOTO) {
//                        const Ir_cond_goto* cond_goto = ir_cond_goto_const_unwrap(curr);
//                        if (cond_goto->pos.line != prev_line) {
//                            msg(
//                                DIAG_NOTE,
//                                cond_goto->pos,
//                                "tracing path of uninitialized variable use\n"
//                            );
//                            prev_line = cond_goto->pos.line;
//                        }
//
//                        Name next_label = vec_at(&curr_frame.prev_desisions, bool_idx) ?
//                            cond_goto->if_true : cond_goto->if_false;
//                        temp_block_idx = label_name_to_block_idx(block->children, next_label);
//                        bool_idx++;
//
//                        if (bool_idx >= curr_frame.prev_desisions.info.count) {
//                            break;
//                        }
//                    } else if (curr->type == IR_GOTO) {
//                        const Ir_goto* lang_goto = ir_goto_const_unwrap(curr);
//                        temp_block_idx = label_name_to_block_idx(block->children, lang_goto->label);
//                    } else {
//                        temp_block_idx++;
//                    }
//                }
//
//                assert(bool_idx == curr_frame.prev_desisions.info.count);
//            }
//
//            if (goto_or_cond_goto) {
//                goto_or_cond_goto = false;
//                break;
//            }
//
//            block_idx++;
//            goto_or_cond_goto = false;
//            check_unit_src_internal_name_failed = false;
//        }
//        block_idx = 0;
//    }
//
//    memset(&curr_frame.init_tables, 0, sizeof(curr_frame.init_tables));
//    assert(frames.info.count == 0);
//}

static void check_unit_import_path(const Ir_import_path* import) {
    log(LOG_DEBUG, FMT"\n", strv_print(import->mod_path));
    check_unit_block(import->block, false);
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
    log(LOG_DEBUG, FMT"\n", ir_function_def_print(def));
    assert(curr_frame);
    // NOTE: decl must be checked before body so that parameters can be set as initialized
    check_unit_function_decl(def->decl);
    check_unit_block(def->body, strv_is_equal(def->decl->name.base, sv("main")));
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src, store->pos);
    check_unit_dest(store->ir_dest);
    init_symbol_add(&curr_frame->init_tables, (Init_table_node) {
        .name = store->name,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

// TODO: should Ir_load_another_ir and store_another_ir actually have name member 
//   instead of just loading/storing to another name?
static void check_unit_load_another_ir(const Ir_load_another_ir* load) {
    log(LOG_DEBUG, FMT"\n", ir_load_another_ir_print(load));
    check_unit_src(load->ir_src, load->pos);
    init_symbol_add(&curr_frame->init_tables, (Init_table_node) {
        .name = load->name,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_load_element_ptr(const Ir_load_element_ptr* load) {
    check_unit_src(load->ir_src, load->pos);
    init_symbol_add(&curr_frame->init_tables, (Init_table_node) {
        .name = load->name_self,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_array_access(const Ir_array_access* access) {
    check_unit_src(access->index, access->pos);
    check_unit_src(access->callee, access->pos);
    init_symbol_add(&curr_frame->init_tables, (Init_table_node) {
        .name = access->name_self,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_goto(const Ir_goto* lang_goto) {
    (void) lang_goto;
    at_end_of_cfg_node = true;
}

static void check_unit_cond_goto(const Ir_cond_goto* cond_goto) {
    (void) cond_goto;
    at_end_of_cfg_node = true;
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

static void check_unit_unary(const Ir_unary* unary) {
    check_unit_src(unary->child, unary->pos);
}

static void check_unit_operator(const Ir_operator* oper) {
    switch (oper->type) {
        case IR_UNARY:
            check_unit_unary(ir_unary_const_unwrap(oper));
            return;
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
    switch (ir->type) {
        case IR_BLOCK:
            unreachable("nested blocks should not be present at this point");
        case IR_EXPR:
            check_unit_expr(ir_expr_const_unwrap(ir));
            return;
        case IR_LOAD_ELEMENT_PTR:
            check_unit_load_element_ptr(ir_load_element_ptr_const_unwrap(ir));
            return;
        case IR_ARRAY_ACCESS:
            check_unit_array_access(ir_array_access_const_unwrap(ir));
            return;
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
            // TODO
            return;
        case IR_ARRAY_ACCESS:
            // TODO
            return;
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
    curr_frame = arena_alloc(&a_main /* todo */, sizeof(*curr_frame));

    Alloca_iter iter = ir_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        check_unit_ir_builtin(curr);
    }
}
