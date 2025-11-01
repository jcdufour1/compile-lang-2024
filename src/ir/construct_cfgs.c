#include <do_passes.h>
#include <symbol_iter.h>
#include <ulang_type_get_pos.h>

// TODO: figure out why blocks are encountered directly when iterating over scope builtin
//   (blocks should only be in functions, etc.)

static Cfg_node_vec* curr_cfg;
static size_t curr_pos;
static size_t curr_cfg_idx_for_cond_goto;

static void construct_cfg_ir_from_block(Ir* ir);
static void construct_cfg_ir_from_scope_builtin(Ir* ir);
static void construct_cfg_block(Ir_block* block);

static void construct_cfg_function_def(Ir_function_def* def) {
    construct_cfg_block(def->body);
}

// TODO: goto should be handled same as cond goto to make things easier (for now)
static void construct_cfg_label(Ir_label* label, bool prev_is_cond_goto) {
    unwrap(curr_cfg && "construct_cfg_label should only be called from block foreach loop\n");
#   ifndef NDEBUG
        vec_foreach(idx, Cfg_node, node, *curr_cfg) {
            if (node.pos_in_block == curr_pos) {
                unreachable("");
            }
        }
#   endif // NDEBUG

    Cfg_node new_node = (Cfg_node) {.label_name = label->name, .pos_in_block = curr_pos};
    if (!prev_is_cond_goto && curr_cfg->info.count > 0) {
        vec_append(&a_main, &new_node.preds, curr_cfg->info.count - 1);
        vec_append(&a_main, &vec_top_ref(curr_cfg)->succs, curr_cfg->info.count);
    }

    vec_append(&a_main, curr_cfg, new_node);
}

static void construct_cfg_cond_goto(Ir_cond_goto* cond_goto) {
    size_t if_true_idx = SIZE_MAX;
    vec_foreach_ref(idx, Cfg_node, curr1, *curr_cfg) {
        if (ir_name_is_equal(curr1->label_name, cond_goto->if_true)) {
            if_true_idx = idx;
            vec_append(&a_main, &curr1->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    unwrap(if_true_idx != SIZE_MAX && "could not find if true cfg node");

    size_t if_false_idx = SIZE_MAX;
    vec_foreach_ref(idx, Cfg_node, curr, *curr_cfg) {
        if (ir_name_is_equal(curr->label_name, cond_goto->if_false)) {
            if_false_idx = idx;
            vec_append(&a_main, &curr->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    unwrap(if_false_idx != SIZE_MAX && "could not find if false cfg node");

    Cfg_node* node = vec_at_ref(curr_cfg, curr_cfg_idx_for_cond_goto);
    vec_append(&a_main, &node->succs, if_true_idx);
    vec_append(&a_main, &node->succs, if_false_idx);
}

static void construct_cfg_goto(Ir_goto* lang_goto) {
    size_t branch_to_idx = SIZE_MAX;
    vec_foreach_ref(idx, Cfg_node, curr, *curr_cfg) {
        if (ir_name_is_equal(curr->label_name, lang_goto->label)) {
            branch_to_idx = idx;
            vec_append(&a_main, &curr->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    unwrap(branch_to_idx != SIZE_MAX && "could not find if true cfg node");

    Cfg_node* node = vec_at_ref(curr_cfg, curr_cfg_idx_for_cond_goto);
    vec_append(&a_main, &node->succs, branch_to_idx);
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

static bool pred_is_dead_end(Size_t_vec* already_covered, Cfg_node_vec cfg, size_t is_backedge) {
    vec_foreach(idx, size_t, covered, *already_covered) {
        if (is_backedge == covered) {
            return false;
        }
    }
    vec_append(&a_print /* TODO */, already_covered, is_backedge);

    //log(LOG_DEBUG, "%zu\n", is_backedge);
    //log(LOG_DEBUG, "%zu\n", cfg.info.count);
    vec_foreach(succ_idx, size_t, pred, vec_at_ref(&cfg, is_backedge)->preds) {
        if (!pred_is_dead_end(already_covered, cfg, pred)) {
            return false;
        }
    }
    return true;
}

// TODO: move this function
static bool cfg_node_is_backedge_internal(Size_t_vec* already_covered, Cfg_node_vec cfg, size_t control, size_t is_backedge) {
    // control == 2, is_backedge == 6
    vec_foreach(idx, size_t, covered, *already_covered) {
        if (control == covered) {
            return false;
        }
    }
    vec_append(&a_print /* TODO */, already_covered, control);
    if (control == 2 && is_backedge == 0) {
    }
    if (control == 2 && is_backedge > 0) {
        //log(LOG_DEBUG, "thing\n");
    }

    if (control == is_backedge) {
        return true;
    }

    //log(LOG_DEBUG, "%zu\n", control);
    //log(LOG_DEBUG, "%zu\n", cfg.info.count);
    vec_foreach(succ_idx, size_t, succ, vec_at_ref(&cfg, control)->succs) {
        //log(LOG_DEBUG, "%zu\n", succ);
        if (cfg_node_is_backedge_internal(already_covered, cfg, succ, is_backedge)) {
            return true;
        }
    }

    return false;
}

// TODO: move this function
static bool cfg_node_is_backedge(Cfg_node_vec cfg, size_t control, size_t is_backedge) {
    assert(control != is_backedge);

    Size_t_vec already_covered = {0};
    bool status = pred_is_dead_end(&already_covered, cfg, is_backedge);
    already_covered = (Size_t_vec) {0};
    if (!status) {
        status = cfg_node_is_backedge_internal(&already_covered, cfg, control, is_backedge);
    }

    arena_reset(&a_print /* TODO */);
    return status;
}

static void construct_cfg_block(Ir_block* block) {
    unwrap(!curr_cfg);
    unwrap(curr_cfg_idx_for_cond_goto == 0);
    curr_cfg = &block->cfg;

    bool prev_is_cond_goto = false;
    {
        vec_foreach(idx, Ir*, curr, block->children) {
            curr_pos = idx;

            if (curr->type == IR_DEF && ir_def_unwrap(curr)->type == IR_LABEL) {
                construct_cfg_label(ir_label_unwrap(ir_def_unwrap(curr)), prev_is_cond_goto);
            } else if (idx == 0) {
                unreachable("the first ir of the block must be a label");
            }

            prev_is_cond_goto = curr->type == IR_COND_GOTO || curr->type == IR_GOTO;
        }
    }

    {
        vec_foreach(idx, Ir*, curr, block->children) {
            curr_pos = idx;
            if (curr_cfg_idx_for_cond_goto + 1 < curr_cfg->info.count && vec_at(*curr_cfg, curr_cfg_idx_for_cond_goto + 1).pos_in_block <= idx) {
                curr_cfg_idx_for_cond_goto++;
            }
            construct_cfg_ir_from_block(curr);
        }
    }

    {
        vec_foreach_ref(node_idx, Cfg_node, node, *curr_cfg) {
            vec_foreach(pred_idx, size_t, pred, node->preds) {
                if (cfg_node_is_backedge(block->cfg, node_idx, pred)) {
                    vec_append(&a_main, &node->pred_backedges, pred);
                }
            }
        }
    }

    curr_cfg = NULL;
    curr_cfg_idx_for_cond_goto = 0;
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
            construct_cfg_goto(ir_goto_unwrap(ir));
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
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
    }
    unreachable("");
}

static void construct_cfg_ir_from_scope_builtin(Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
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
        case IR_STRUCT_MEMB_DEF:
            unreachable("");
    }
    unreachable("");
}

void construct_cfgs(void) {
    Alloca_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        construct_cfg_ir_from_scope_builtin(curr);
    }
}
