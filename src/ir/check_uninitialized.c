#include <do_passes.h>
#include <symbol_iter.h>
#include <msg.h>
#include <ir_utils.h>
#include <name.h>
#include <bool_vec.h>
#include <symbol_log.h>
#include <cfg_foreach.h>

// TODO: in generic function, check uninitalized variables for only one of the variants to save time?

static void check_unit_ir_builtin(const Ir* ir);

//static Frame_vec already_run_frames = {0};
static bool check_unit_src_internal_name_failed = false;

static size_t block_pos = {0};
static size_t cfg_node_idx = 0;
static Init_table* curr_cfg_node_area = NULL;
static Init_table_vec cfg_node_areas = {0};
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
    check_unit_src(call->callee, pos, ir_get_loc(call));
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
    check_unit_src_internal_name(def->name_self, def->pos, ir_get_loc(def));
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
    (void) loc;
    if (!print_errors_for_unit) {
        return;
    }

    todo();
    //if (name.attrs & ATTR_ALLOW_UNINIT) {
    //    return;
    //}

    Ir_name at_fun_start = name_to_ir_name(name_new(
        MOD_PATH_BUILTIN,
        sv("at_fun_start"),
        (Ulang_type_vec) {0},
        name.scope_id
    ));

    Init_table_node* result = NULL;
    if (!init_symbol_lookup(curr_cfg_node_area, &result, at_fun_start)) {
        //todo();
        // this frame is unreachable, so printing uninitalized error would not make sense
        return;
    }

    if (init_symbol_lookup(curr_cfg_node_area, &result, name)) {
        if (result->cfg_node_of_init != cfg_node_idx || result->block_pos_of_init < block_pos) {
            // symbol was initialized before this use
            return;
        }
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

#   ifndef NDEBUG
        log(LOG_DEBUG, FMT"\n", loc_print(loc));
        log(LOG_DEBUG, "%zu\n", cfg_node_areas.info.count);
        log(LOG_DEBUG, "curr_cfg_node_area idx: %zu\n", cfg_node_idx);
        log(LOG_DEBUG, "name.scope_id: %zu\n", name.scope_id);
        log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, name));
        todo();
        //name.attrs |= ATTR_ALLOW_UNINIT;
        log(LOG_DEBUG, FMT"\n", ir_name_print(NAME_LOG, name));

        log(LOG_DEBUG, "\n\n\n\nTHING THING:\n\n\n\n");
        vec_foreach(cfg_node_idx, Init_table, n_frame, cfg_node_areas) {
            log(LOG_DEBUG, "frame %zu:\n", cfg_node_idx);
            String buf = {0};
            string_extend_strv(&a_temp, &buf, cfg_node_print_internal(vec_at(curr_block_cfg, cfg_node_idx), cfg_node_idx, INDENT_WIDTH));
            log(LOG_DEBUG, FMT"\n", string_print(buf));
            init_level_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0 /* TODO */, n_frame, INDENT_WIDTH);

            log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, "body at frame\n");
            for (size_t block_idx = vec_at(curr_block_cfg, cfg_node_idx).pos_in_block; block_idx < curr_block_children.info.count; block_idx++) {
                log(LOG_DEBUG, FMT, strv_print(ir_print_internal(vec_at(curr_block_children, block_idx), INDENT_WIDTH)));

                if (
                    block_idx + 1 < curr_block_children.info.count &&
                    ir_is_label(vec_at(curr_block_children, block_idx + 1))
                ) {
                    break;
                }
            }
        }

    // TODO: make function to log entire cfg_node_areas
    vec_foreach(idx, Init_table, frame, cfg_node_areas) {
        log(LOG_DEBUG, "frame %zu:\n", idx);
        init_level_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0 /* TODO */, frame, INDENT_WIDTH);
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

#   endif //NDEBUG
          
    for (size_t idx = 0; idx < cfg_node_areas.info.count; idx++) {
        // prevent printing error for the same symbol on several code paths
        init_symbol_add(vec_at_ref(&cfg_node_areas, idx), (Init_table_node) {
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
            fallthrough;
        case IR_STORE_ANOTHER_IR:
            fallthrough;
        case IR_LOAD_ELEMENT_PTR:
            fallthrough;
        case IR_ARRAY_ACCESS:
            fallthrough;
        case IR_ALLOCA:
            check_unit_src_internal_name(ir_get_name(LANG_TYPE_MODE_LOG, ir), pos, loc);
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
    init_symbol_add(curr_cfg_node_area, (Init_table_node) {
        .name = dest,
        .cfg_node_of_init = cfg_node_idx,
        .block_pos_of_init = block_pos
    });
}

// returns true if the label was found
// TODO: move this function elsewhere, since this function may be useful for other passes? (or remove this function)
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

static void check_unit_block(const Ir_block* block) {
    print_errors_for_unit = false;

    curr_block_cfg = block->cfg;
    curr_block_children = block->children;

    unwrap(cfg_node_areas.info.count == 0);
    unwrap(curr_cfg_node_area);
    vec_foreach(idx, Cfg_node, curr1, block->cfg) {
        (void) curr1;
        if (idx == 0) {
            vec_append(&a_pass, &cfg_node_areas, *curr_cfg_node_area);
            continue;
        }
        vec_append(&a_pass, &cfg_node_areas, ((Init_table) {0}));
    }

    for (size_t iter_idx = 0; iter_idx < 1; iter_idx++) {
        vec_foreach(idx, Cfg_node, curr, block->cfg) {
            cfg_node_idx = idx;
            curr_cfg_node_area = vec_at_ref(&cfg_node_areas, idx);

            ir_in_cfg_node_foreach(block_idx, curr_ir, curr, block->children) {
                (void) curr_ir;
                block_pos = block_idx;
                check_unit_ir_from_block(vec_at(block->children, block_idx));
            }

            if (curr.preds.info.count > 0) {
                size_t pred_0 = vec_at(curr.preds, 0);
                Init_table_iter iter = init_tbl_iter_new_table(vec_at(cfg_node_areas, pred_0));
                Init_table_node curr_in_tbl = {0};
                while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
                    bool is_init_in_pred = true;
                    Init_table_node* node = NULL;
                    if (init_symbol_lookup(curr_cfg_node_area, &node, curr_in_tbl.name)) {
                        if (node->cfg_node_of_init != curr_in_tbl.cfg_node_of_init) {
                            node->cfg_node_of_init = curr_in_tbl.cfg_node_of_init;
                            node->block_pos_of_init = curr_in_tbl.block_pos_of_init;
                        }
                        continue;
                    }
                    vec_foreach(pred_idx, size_t, pred, curr.preds) {
                        if (iter_idx < 10) {
                            if (pred_idx >= curr.pred_backedge_start) {
                                continue;
                            }
                        }

                        Init_table* pred_frame = vec_at_ref(&cfg_node_areas, pred);
                        Init_table_node* dummy = NULL;
                        if (!init_symbol_lookup(pred_frame, &dummy, curr_in_tbl.name)) {
                            is_init_in_pred = false;
                            break;
                        }
                    }

                    if (is_init_in_pred) {
                        unwrap(init_symbol_add(curr_cfg_node_area, curr_in_tbl));
                    }

                }
            }
        }
    }

    print_errors_for_unit = true;

    cfg_foreach(cfg_node_idx_, curr2, block->cfg) {
        cfg_node_idx = cfg_node_idx_;
        curr_cfg_node_area = vec_at_ref(&cfg_node_areas, cfg_node_idx);

        ir_in_cfg_node_foreach(block_idx, ir, curr2, block->children) {
            block_pos = block_idx;
            check_unit_ir_from_block(ir);
        }
    }
    

    // TODO: make function to log entire cfg_node_areas
    //vec_foreach(idx, Frame, frame, cfg_node_areas) {
    //    log(LOG_DEBUG, "frame %zu:\n", idx);
    //    init_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0, &frame.init_tables);
    //    log(LOG_DEBUG, "\n");
    //    //vec_foreach(tbl_idx, Init_table, curr_table, frame.init_tables) {/*{*/
    //        //Init_table_iter iter = init_tbl_iter_new_table(curr_table);
    //        //Init_table_node curr_in_tbl = {0};
    //        //bool is_init_in_pred = true;
    //        //while (init_tbl_iter_next(&curr_in_tbl, &iter)) {
    //        //    Init_table_node* dummy = NULL;
    //        //    if (init_symbol_lookup(&curr_cfg_node_area->init_tables, &dummy, curr_in_tbl.name)) {
    //        //        continue;
    //        //    }

    //        //    if (is_init_in_pred) {
    //        //        unwrap(init_symbol_add(&curr_cfg_node_area->init_tables, curr_in_tbl));
    //        //    }
    //        //}
    //    //}

    //}
    //log(LOG_DEBUG, "\n\n");

    cfg_node_areas = (Init_table_vec) {0};
    curr_cfg_node_area = arena_alloc(&a_main /* todo */, sizeof(*curr_cfg_node_area));
    assert(curr_cfg_node_area->count == 0);
    assert(curr_cfg_node_area->capacity == 0);
}

static void check_unit_import_path(const Ir_import_path* import) {
    check_unit_block(import->block);
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
    check_unit_block(def->body);
}

static void check_unit_store_another_ir(const Ir_store_another_ir* store) {
    // NOTE: src must be checked before dest
    check_unit_src(store->ir_src, store->pos, ir_get_loc(store));
    check_unit_dest(store->ir_dest);
    init_symbol_add(curr_cfg_node_area, (Init_table_node) {
        .name = store->name,
        .cfg_node_of_init = cfg_node_idx,
        .block_pos_of_init = block_pos
    });
}

// TODO: should Ir_load_another_ir and store_another_ir actually have name member 
//   instead of just loading/storing to another name?
static void check_unit_load_another_ir(const Ir_load_another_ir* load) {
    check_unit_src(load->ir_src, load->pos, ir_get_loc(load));
    init_symbol_add(curr_cfg_node_area, (Init_table_node) {
        .name = load->name,
        .cfg_node_of_init = cfg_node_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_load_element_ptr(const Ir_load_element_ptr* load) {
    check_unit_src(load->ir_src, load->pos, ir_get_loc(load));
    init_symbol_add(curr_cfg_node_area, (Init_table_node) {
        .name = load->name_self,
        .cfg_node_of_init = cfg_node_idx,
        .block_pos_of_init = block_pos
    });
}

static void check_unit_array_access(const Ir_array_access* access) {
    check_unit_src(access->index, access->pos, ir_get_loc(access));
    check_unit_src(access->callee, access->pos, ir_get_loc(access));
    init_symbol_add(curr_cfg_node_area, (Init_table_node) {
        .name = access->name_self,
        .cfg_node_of_init = cfg_node_idx,
        .block_pos_of_init = block_pos
    });
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
    check_unit_src(call->callee, call->pos, ir_get_loc(call));
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        Ir_name curr = vec_at(call->args, idx);
        check_unit_src(curr, call->pos /* TODO: call->pos may not always be good enough? */, ir_get_loc(call));
    }
}

static void check_unit_binary(const Ir_binary* bin) {
    check_unit_src(bin->lhs, bin->pos, ir_get_loc(bin));
    check_unit_src(bin->rhs, bin->pos, ir_get_loc(bin));
}

static void check_unit_unary(const Ir_unary* unary) {
    check_unit_src(unary->child, unary->pos, ir_get_loc(unary));
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
            return;
        case IR_COND_GOTO:
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
            return;
        case IR_LOAD_ELEMENT_PTR:
            return;
        case IR_ARRAY_ACCESS:
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
            return;
        case IR_LOAD_ANOTHER_IR:
            return;
        case IR_STORE_ANOTHER_IR:
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
