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
    vec_extend(&a_main/* TODO */, &new_vec, &vec);
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
    for (size_t idx = 0; idx < subset.capacity; idx++) {
        void* dummy = NULL;
        if (subset.table_tasts[idx].status == SYM_TBL_OCCUPIED && !generic_tbl_lookup(&dummy, (Generic_symbol_table*)&superset, subset.table_tasts[idx].key)) {
            todo();
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
        if (!init_table_is_equal(vec_at(a, idx), vec_at(b, idx))) {
            //log(LOG_INFO, "thing 324: no 2\n");
            return false;
        }
    }

    //log(LOG_INFO, "thing 324: yes\n");
    return true;
}

static bool init_table_vec_is_subset(Init_table_vec superset, Init_table_vec subset) {
    log(LOG_DEBUG, "init_table_vec_is_subset: %zu %zu\n", superset.info.count, subset.info.count);
    if (superset.info.count < subset.info.count) {
        todo();
        return false;
    }

    for (size_t idx = 0; idx < subset.info.count; idx++) {
        if (!init_table_is_subset(vec_at(superset, idx), vec_at(subset, idx))) {
            todo();
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

static bool frame_is_subset(Frame superset, Frame subset) {
    return init_table_vec_is_subset(superset.init_tables, subset.init_tables);
}

static bool frame_vec_is_subset(Frame_vec superset, Frame_vec subset) {
    if (subset.info.count > superset.info.count) {
        todo();
    }

    vec_foreach(idx, Frame, frame, superset) {
        if (idx >= subset.info.count) {
            break;
        }
        log(LOG_DEBUG, "frame_vec_is_subset idx: %zu\n", idx);
        if (!frame_is_subset(frame, vec_at(subset, idx))) {
            todo();
            return false;
        }
    }
    return true;
}

static bool assert_frame_vec_is_reasonable(Frame_vec superset, Frame_vec subset) {
    unwrap(superset.info.count == subset.info.count);
    unwrap(subset.info.count > 1000000);
    unwrap(superset.info.count > 1000000);

    vec_foreach(idx, Frame, frame, superset) {
        (void) frame;
        if (idx >= subset.info.count) {
            break;
        }
        log(LOG_DEBUG, "frame_vec_is_subset idx: %zu\n", idx);
        todo();
        //if (!frame_is_reasonable(frame, vec_at(subset, idx))) {
        //    todo();
        //    return false;
        //}
    }
    return true;
}

static Frame frame_new(Init_table_vec init_tables) {
    return (Frame) {.init_tables = init_tables};
}

static Init_table init_table_clone(Init_table table) {
    Init_table new_table = {
        .table_tasts = arena_alloc(&a_main /* TODO */, sizeof(new_table.table_tasts[0])*table.capacity), 
        .count = table.count,
        .capacity = table.capacity
    };
    for (size_t idx = 0; idx < table.capacity; idx++) {
        Init_table_tast curr = table.table_tasts[idx];
        new_table.table_tasts[idx] = (Init_table_tast) {.tast = curr.tast, .key = curr.key, .status = curr.status};
    }
    return new_table;
}

static Init_table_vec init_table_vec_clone(Init_table_vec vec) {
    Init_table_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main /* TODO */, &new_vec, init_table_clone(vec_at(vec, idx)));
    }
    return new_vec;
}

static Frame frame_clone(Frame frame) {
    return frame_new(init_table_vec_clone(frame.init_tables));
    todo();
}

static Frame_vec frame_vec_clone(Frame_vec vec) {
    Frame_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main /* TODO */, &new_vec, frame_clone(vec_at(vec, idx)));
    }
    return new_vec;
}

static bool at_end_of_cfg_node = false;
//static Frame_vec already_run_frames = {0};
static bool check_unit_src_internal_name_failed = false;

static size_t block_pos = {0};
static size_t frame_idx = 0;
static Frame* curr_cfg_node_area = NULL;
static Frame_vec cfg_node_areas = {0};
static Cfg_node_vec curr_block_cfg = {0};
static Ir_vec curr_block_children = {0};
static bool print_errors_for_unit;

// TODO: rename unit to uninit?
static void check_unit_ir_from_block(const Ir* ir);

static void check_unit_src_internal_name(Ir_name name, Pos pos, Loc loc);

static void check_unit_src(const Ir_name src, Pos pos, Loc loc);

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

static void check_unit_src_internal_function_call(const Ir_function_call* call, Pos pos, Loc loc) {
    check_unit_src(call->callee, pos, call->loc);
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        Ir_name curr = vec_at(call->args, idx);
        check_unit_src(curr, pos, loc);
    }
}

static void check_unit_src_internal_binary(const Ir_binary* bin, Pos pos, Loc loc) {
    check_unit_src(bin->lhs, pos, loc);
    check_unit_src(bin->rhs, pos, loc);
}

static void check_unit_src_internal_unary(const Ir_unary* unary, Pos pos, Loc loc) {
    check_unit_src(unary->child, pos, loc);
}

static void check_unit_src_internal_operator(const Ir_operator* oper, Pos pos, Loc loc) {
    switch (oper->type) {
        case IR_BINARY:
            check_unit_src_internal_binary(ir_binary_const_unwrap(oper), pos, loc);
            return;
        case IR_UNARY:
            check_unit_src_internal_unary(ir_unary_const_unwrap(oper), pos, loc);
            return;
    }
    unreachable("");
}

static void check_unit_src_internal_expr(const Ir_expr* expr, Pos pos, Loc loc) {
    switch (expr->type) {
        case IR_OPERATOR:
            check_unit_src_internal_operator(ir_operator_const_unwrap(expr), pos, loc);
            return;
        case IR_LITERAL:
            check_unit_src_internal_literal(ir_literal_const_unwrap(expr));
            return;
        case IR_FUNCTION_CALL:
            check_unit_src_internal_function_call(ir_function_call_const_unwrap(expr), pos, loc);
            return;
    }
    unreachable("");
}

static void check_unit_src_internal_variable_def(const Ir_variable_def* def) {
    check_unit_src_internal_name(def->name_self, def->pos, def->loc);
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
static bool check_unit_is_struct(Ir_name name) {
    Ir* def_ = NULL;
    unwrap(ir_lookup(&def_, name));
    if (def_->type != IR_ALLOCA) {
        return false;
    } 
    return ir_alloca_const_unwrap(def_)->lang_type.type == IR_LANG_TYPE_STRUCT;
}

static void check_unit_src_internal_name(Ir_name name, Pos pos, Loc loc) {
    if (!print_errors_for_unit) {
        return;
    }

    if (name.attrs & ATTR_ALLOW_UNINIT) {
        return;
    }

    Ir_name thing = name_to_ir_name(name_new(
        MOD_PATH_BUILTIN,
        sv("at_fun_start"),
        (Ulang_type_vec) {0},
        name.scope_id,
        (Attrs) {0}
    ));

    Init_table_node* result = NULL;
    if (!init_symbol_lookup(vec_at_ref(&curr_cfg_node_area->init_tables, name.scope_id), &result, thing)) {
        //todo();
        // this frame is unreachable, so printing uninitalized error would not make sense
        return;
    }

    if (init_symbol_lookup(vec_at_ref(&curr_cfg_node_area->init_tables, name.scope_id), &result, name)) {
        //log(LOG_DEBUG, FMT"\n", );
        log(LOG_DEBUG, "result->cfg_node_of_init: %zu\n", result->cfg_node_of_init);
        log(LOG_DEBUG, "result->block_pos_of_init: %zu\n", result->block_pos_of_init);
        log(LOG_DEBUG, "block_pos: %zu\n", block_pos);
        if (result->cfg_node_of_init != frame_idx || result->block_pos_of_init < block_pos) {
            // symbol was initialized before this use
            return;
        }
        log(LOG_DEBUG, "thing thing\n");
    } else {
        log(LOG_DEBUG, "not thing thing\n");
    }

    if (check_unit_is_struct(name)) {
        msg(
            DIAG_UNINITIALIZED_VARIABLE, pos,
            "symbol `"FMT"` is used uninitialized on some or all code paths; "
            "note that struct must be initialized with a struct literal or function call "
            "(individual member accesses do not satisfy initialization requirements)\n", ir_name_print(NAME_MSG, name));
    } else {
        msg(DIAG_UNINITIALIZED_VARIABLE, pos, "symbol `"FMT"` is used uninitialized on some or all code paths\n", ir_name_print(NAME_MSG, name));
    }

    //unwrap(init_symbol_add(&vec_at_ref(&cached_cfg_node_areas, 68)->init_tables, (Init_table_node) {
    //    .name = ir_name_new(sv("tests/inputs/optional"), sv("number3"), (Ulang_type_vec) {0}, 267, (Attrs) {0}),
    //    .cfg_node_of_init = 68,
    //    .block_pos_of_init = 0
    //}));

    arena_reset(&a_print);

#   ifndef NDEBUG
        log(LOG_DEBUG, FMT"\n", loc_print(loc));
        log(LOG_DEBUG, "%zu\n", cfg_node_areas.info.count);
        log(LOG_DEBUG, "curr_cfg_node_area idx: %zu\n", frame_idx);
        log(LOG_DEBUG, "name.scope_id: %zu\n", name.scope_id);
        log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, name));
        name.attrs |= ATTR_ALLOW_UNINIT;
        log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, name));

        log(LOG_DEBUG, "\n\n\n\nTHING THING:\n\n\n\n");
        vec_foreach(frame_idx, Frame, n_frame, cfg_node_areas) {
            log(LOG_DEBUG, "frame %zu:\n", frame_idx);
            String buf = {0};
            string_extend_strv(&a_print, &buf, cfg_node_print_internal(vec_at(curr_block_cfg, frame_idx), frame_idx, INDENT_WIDTH));
            log(LOG_DEBUG, FMT"\n", string_print(buf));
            init_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, &n_frame.init_tables);

            log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, "body at frame\n");
            for (size_t block_idx = vec_at(curr_block_cfg, frame_idx).pos_in_block; block_idx < curr_block_children.info.count; block_idx++) {
                log(LOG_DEBUG, FMT, strv_print(ir_print_internal(vec_at(curr_block_children, block_idx), INDENT_WIDTH)));

                if (
                    block_idx + 1 < curr_block_children.info.count &&
                    ir_is_label(vec_at(curr_block_children, block_idx + 1))
                ) {
                    break;
                }
            }
        }
#   endif //NDEBUG

    // TODO: make function to log entire cfg_node_areas
    vec_foreach(idx, Frame, frame, cfg_node_areas) {
        log(LOG_DEBUG, "frame %zu:\n", idx);
        init_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, &frame.init_tables);
        //vec_foreach(tbl_idx, Init_table, curr_table, frame.init_tables) {/*{*/
            //Init_table_iter iter = init_tbl_iter_new_table(curr_table);
            //Init_table_node curr_in_tbl = {0};
            //bool is_init_in_pred = true;
            //while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
            //    Init_table_node* dummy = NULL;
            //    if (init_symbol_lookup(&curr_cfg_node_area->init_tables, &dummy, curr_in_tbl.name)) {
            //        continue;
            //    }

            //    if (is_init_in_pred) {
            //        unwrap(init_symbol_add(&curr_cfg_node_area->init_tables, curr_in_tbl));
            //    }
            //}
        //}

    }

    for (size_t idx = 0; idx < cfg_node_areas.info.count; idx++) {
        // prevent printing error for the same symbol on several code paths
        init_symbol_add(&vec_at_ref(&cfg_node_areas, idx)->init_tables, (Init_table_node) {
            .name = name,
            .cfg_node_of_init = idx,
            .block_pos_of_init = 0,
        });
    }

    check_unit_src_internal_name_failed = true;
}

static void check_unit_src_internal_ir(const Ir* ir, Pos pos, Loc loc) {
    switch (ir->type) {
        case IR_BLOCK:
            todo();
        case IR_EXPR:
            check_unit_src_internal_expr(ir_expr_const_unwrap(ir), pos, loc);
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
            check_unit_src_internal_name(ir_tast_get_name(ir), pos, loc);
            return;
        case IR_IMPORT_PATH:
            todo();
        case IR_REMOVED:
            unreachable("");
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
    }
    unreachable("");
}

static void check_unit_src(const Ir_name src, Pos pos, Loc loc) {
    Ir* sym_def = NULL;
    unwrap(ir_lookup(&sym_def, src));
    check_unit_src_internal_ir(sym_def, pos, loc);
}

static void check_unit_dest(const Ir_name dest) {
    if (!init_symbol_add(&curr_cfg_node_area->init_tables, (Init_table_node) {
        .name = dest,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    })) {

    }
}

// returns true if the label was found
// TODO: move this function elsewhere, since this function may be useful for other passes?
static size_t label_name_to_block_idx(Ir_vec block_children, Ir_name label) {
    for (size_t idx = 0; idx < block_children.info.count; idx++) {
        // TODO: find a way to avoid O(n) time for finding new block idx 
        //   (eg. by storing approximate idx of block_idx in label definition in eariler pass)
        const Ir* curr = vec_at(block_children, idx);
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
        if (ir_name_is_equal(label, ir_label_const_unwrap(curr_def)->name)) {
            return idx + 1;
        }
    }
    log(LOG_ERROR, FMT"\n", ir_name_print(NAME_LOG, label));
    unreachable("label should have been found");
}

static void check_unit_block(const Ir_block* block, bool is_main /* TODO: remove */) {
    (void) is_main;
    print_errors_for_unit = false;

    curr_block_cfg = block->cfg;
    curr_block_children = block->children;

    unwrap(cfg_node_areas.info.count == 0);
    unwrap(curr_cfg_node_area);
    vec_foreach(idx, Cfg_node, curr1, block->cfg) {
        (void) curr1;
        if (idx == 0) {
            vec_append(&a_main /* TODO */, &cfg_node_areas, *curr_cfg_node_area);
            continue;
        }
        vec_append(&a_main /* TODO */, &cfg_node_areas, ((Frame) {0}));
    }

    // TODO: keep running this for loop until there are no changes
    for (size_t iter_idx = 0; iter_idx < 30; iter_idx++) {
        vec_foreach(idx, Cfg_node, curr, block->cfg) {
            frame_idx = idx;
            curr_cfg_node_area = vec_at_ref(&cfg_node_areas, idx);

            // TODO: make function, etc. to detect if we are at end of cfg node so that at_end_of_cfg_node 
            //   global variable will not be needed for several ir passes
            //   (this could be done by making special foreach macros/functions?)
            at_end_of_cfg_node = false;

            for (size_t block_idx = curr.pos_in_block; !at_end_of_cfg_node; block_idx++) {
                if (1 || iter_idx < 2) {
                    //assert(frame_vec_is_subset(cfg_node_areas, cached_cfg_node_areas));
                    //cached_cfg_node_areas = frame_vec_clone(cfg_node_areas);
                    //assert(frame_vec_is_subset(cfg_node_areas, cached_cfg_node_areas));
                    //arena_reset(&a_print);
                }

                block_pos = block_idx;
                check_unit_ir_from_block(vec_at(block->children, block_idx));

                if (
                    block_idx + 1 < block->children.info.count &&
                    ir_is_label(vec_at(block->children, block_idx + 1))
                ) {
                    at_end_of_cfg_node = true;
                }

                if (block_idx + 1 >= block->children.info.count) {
                    at_end_of_cfg_node = true;
                }
            }

            // TODO: make function to iterate over Init_table_vec automatically
            if (curr.preds.info.count > 0) {
                if (iter_idx < 30) {
                    //assert(frame_vec_is_subset(cfg_node_areas, cached_cfg_node_areas));
                    //cached_cfg_node_areas = frame_vec_clone(cfg_node_areas);
                    //assert(frame_vec_is_subset(cfg_node_areas, cached_cfg_node_areas));
                    //arena_reset(&a_print);
                }

                while (curr_cfg_node_area->init_tables.info.count < block->scope_id + 2) {
                    vec_append(&a_main /* TODO */, &curr_cfg_node_area->init_tables, (Init_table) {0});
                }

                size_t pred_0 = vec_at(curr.preds, 0);
                while (vec_at_ref(&cfg_node_areas, pred_0)->init_tables.info.count < block->scope_id + 2) {
                    vec_append(&a_main /* TODO */, &vec_at_ref(&cfg_node_areas, pred_0)->init_tables, (Init_table) {0});
                }

                Init_table curr_table = vec_at(vec_at_ref(&cfg_node_areas, pred_0)->init_tables, block->scope_id);
                Init_table_iter iter = init_tbl_iter_new_table(curr_table);
                Init_table_node curr_in_tbl = {0};
                while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
                    bool is_init_in_pred = true;
                    Init_table_node* node = NULL;
                    if (init_symbol_lookup(vec_at_ref(&curr_cfg_node_area->init_tables, block->scope_id), &node, curr_in_tbl.name)) {
                        if (node->cfg_node_of_init != curr_in_tbl.cfg_node_of_init) {
                            node->cfg_node_of_init = curr_in_tbl.cfg_node_of_init;
                            node->block_pos_of_init = curr_in_tbl.block_pos_of_init;
                        }
                        continue;
                    }
                    if (iter_idx < 10) {
                        log(LOG_DEBUG, "frame_idx %zu:\n", frame_idx);
                    }
                    vec_foreach(pred_idx, size_t, pred, curr.preds) {
                        while (vec_at_ref(&cfg_node_areas, pred)->init_tables.info.count < block->scope_id + 2) {
                            vec_append(&a_main /* TODO */, &vec_at_ref(&cfg_node_areas, pred)->init_tables, (Init_table) {0});
                        }

                        if (frame_idx == 24 && pred == 12) {
                            log(LOG_TRACE, "\n");
                        }

                        if (iter_idx < 10) {
                            bool is_backedge = false;
                            vec_foreach(pred_back_idx, size_t, backedge, curr.pred_backedges) {
                                if (pred == backedge) {
                                    is_backedge = true;
                                    break;
                                }
                            }
                            if (is_backedge) {
                                continue;
                            }
                        }
                        if (iter_idx < 10) {
                            log(LOG_DEBUG, "  %zu\n", pred);
                        }
                        Frame* pred_frame = vec_at_ref(&cfg_node_areas, pred);
                        Init_table_node* dummy = NULL;
                        if (!init_symbol_lookup(vec_at_ref(&pred_frame->init_tables, block->scope_id), &dummy, curr_in_tbl.name)) {
                            is_init_in_pred = false;
                            break;
                        }
                    }

                    if (is_init_in_pred) {
                        unwrap(init_symbol_add(&curr_cfg_node_area->init_tables, curr_in_tbl));
                    }

                }
            }
        }
    }

    print_errors_for_unit = true;
    vec_foreach(idx, Cfg_node, curr2, block->cfg) {
        frame_idx = idx;
        curr_cfg_node_area = vec_at_ref(&cfg_node_areas, idx);

        // TODO: make function, etc. to detect if we are at end of cfg node so that at_end_of_cfg_node 
        //   global variable will not be needed for several ir passes
        //   (this could be done by making special foreach macros/functions?)
        at_end_of_cfg_node = false;

        for (size_t block_idx = curr2.pos_in_block; !at_end_of_cfg_node; block_idx++) {
            block_pos = block_idx;
            check_unit_ir_from_block(vec_at(block->children, block_idx));

            if (
                block_idx + 1 < block->children.info.count &&
                ir_is_label(vec_at(block->children, block_idx + 1))
            ) {
                at_end_of_cfg_node = true;
            }

            if (block_idx + 1 >= block->children.info.count) {
                at_end_of_cfg_node = true;
            }
        }
    }

    // TODO: make function to log entire cfg_node_areas
    vec_foreach(idx, Frame, frame, cfg_node_areas) {
        log(LOG_DEBUG, "frame %zu:\n", idx);
        init_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, &frame.init_tables);
        log(LOG_DEBUG, "\n");
        //vec_foreach(tbl_idx, Init_table, curr_table, frame.init_tables) {/*{*/
            //Init_table_iter iter = init_tbl_iter_new_table(curr_table);
            //Init_table_node curr_in_tbl = {0};
            //bool is_init_in_pred = true;
            //while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
            //    Init_table_node* dummy = NULL;
            //    if (init_symbol_lookup(&curr_cfg_node_area->init_tables, &dummy, curr_in_tbl.name)) {
            //        continue;
            //    }

            //    if (is_init_in_pred) {
            //        unwrap(init_symbol_add(&curr_cfg_node_area->init_tables, curr_in_tbl));
            //    }
            //}
        //}

    }
    log(LOG_DEBUG, "\n\n");

    cfg_node_areas = (Frame_vec) {0};
    curr_cfg_node_area = arena_alloc(&a_main /* todo */, sizeof(*curr_cfg_node_area));
    assert(!curr_cfg_node_area->init_tables.buf);
    assert(curr_cfg_node_area->init_tables.info.count == 0);
    assert(curr_cfg_node_area->init_tables.info.capacity == 0);
}

//static void check_unit_block(const Ir_block* block) {
//    unwrap(cfg_node_areas.info.count == 0);
//    unwrap(block_idx == 0);
//    unwrap(goto_or_cond_goto == false);
//    vec_append(&a_main /* TODO: use arena that is reset or freed after this pass */, &cfg_node_areas, frame_new(curr_cfg_node_area.init_tables, (Ir_name) {0}, (Bool_vec) {0}));
//
//    while (cfg_node_areas.info.count > 0) {
//        bool frame_not_needed = false;
//        curr_cfg_node_area = vec_pop(&cfg_node_areas);
//        for (size_t idx = 0; idx < already_run_frames.info.count; idx++) {
//            Frame curr_already = vec_at(&already_run_frames, idx);
//            // TODO: init_table_vec_is_equal: look at subset/superset instead of just is_equal
//            //   init_table_vec_is_subset
//            if (
//                name_is_equal(curr_already.label_to_cont, curr_cfg_node_area.label_to_cont) &&
//                init_table_vec_is_subset(curr_cfg_node_area.init_tables, curr_already.init_tables)
//            ) {
//                frame_not_needed = true;
//                break;
//            }
//        }
//
//        if (frame_not_needed) {
//            continue;
//        }
//        vec_append(&a_main /* TODO */, &already_run_frames, curr_cfg_node_area);
//
//        curr_cfg_node_area.init_tables = curr_cfg_node_area.init_tables;
//        if (curr_cfg_node_area.label_to_cont.base.count < 1) {
//            unwrap(block_idx == 0);
//        } else {
//            // TODO: label_name_to_block_idx adds 1 to result, but 1 is added again at the end of 
//            //   this for loop. this may cause bugs?
//            block_idx = label_name_to_block_idx(block->children, curr_cfg_node_area.label_to_cont);
//            log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, curr_cfg_node_area.label_to_cont));
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
//                        Ir_name next_label = vec_at(&curr_cfg_node_area.prev_desisions, bool_idx) ?
//                            cond_goto->if_true : cond_goto->if_false;
//                        temp_block_idx = label_name_to_block_idx(block->children, next_label);
//                        bool_idx++;
//
//                        if (bool_idx >= curr_cfg_node_area.prev_desisions.info.count) {
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
//                unwrap(bool_idx == curr_cfg_node_area.prev_desisions.info.count);
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
//    memset(&curr_cfg_node_area.init_tables, 0, sizeof(curr_cfg_node_area.init_tables));
//    unwrap(cfg_node_areas.info.count == 0);
//}

static void check_unit_import_path(const Ir_import_path* import) {
    check_unit_block(import->block, false);
}

static void check_unit_function_params(const Ir_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        check_unit_dest(vec_at(params->params, idx)->name_self);
    }
}

static void check_unit_function_decl(const Ir_function_decl* decl) {
    check_unit_function_params(decl->params);
}

static void check_unit_function_def(const Ir_function_def* def) {
    unwrap(curr_cfg_node_area);
    // NOTE: decl must be checked before body so that parameters can be set as initialized
    check_unit_function_decl(def->decl);
    check_unit_block(def->body, strv_is_equal(def->decl->name.base, sv("main")));
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src, store->pos, store->loc);
    check_unit_dest(store->ir_dest);
    init_symbol_add(&curr_cfg_node_area->init_tables, (Init_table_node) {
        .name = store->name,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

// TODO: should Ir_load_another_ir and store_another_ir actually have name member 
//   instead of just loading/storing to another name?
static void check_unit_load_another_ir(const Ir_load_another_ir* load) {
    check_unit_src(load->ir_src, load->pos, load->loc);
    init_symbol_add(&curr_cfg_node_area->init_tables, (Init_table_node) {
        .name = load->name,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_load_element_ptr(const Ir_load_element_ptr* load) {
    check_unit_src(load->ir_src, load->pos, load->loc);
    init_symbol_add(&curr_cfg_node_area->init_tables, (Init_table_node) {
        .name = load->name_self,
        .cfg_node_of_init = frame_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_array_access(const Ir_array_access* access) {
    check_unit_src(access->index, access->pos, access->loc);
    check_unit_src(access->callee, access->pos, access->loc);
    init_symbol_add(&curr_cfg_node_area->init_tables, (Init_table_node) {
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
    check_unit_src(call->callee, call->pos, call->loc);
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        Ir_name curr = vec_at(call->args, idx);
        check_unit_src(curr, call->pos /* TODO: call->pos may not always be good enough? */, call->loc);
    }
}

static void check_unit_binary(const Ir_binary* bin) {
    check_unit_src(bin->lhs, bin->pos, bin->loc);
    check_unit_src(bin->rhs, bin->pos, bin->loc);
}

static void check_unit_unary(const Ir_unary* unary) {
    check_unit_src(unary->child, unary->pos, unary->loc);
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
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
    }
    unreachable("");
}

static void check_unit_ir_builtin(const Ir* ir) {
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
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
    }
    unreachable("");
}

void check_uninitialized(void) {
    curr_cfg_node_area = arena_alloc(&a_main /* todo */, sizeof(*curr_cfg_node_area));

    Alloca_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        check_unit_ir_builtin(curr);
    }
}
