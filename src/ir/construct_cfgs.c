#include <do_passes.h>
#include <symbol_iter.h>
#include <bool_darr.h>
#include <arena.h>

// TODO: figure out why blocks are encountered directly when iterating over scope builtin
//   (blocks should only be in functions, etc.)

typedef struct {
    Vec_base info;
    Bool_darr* buf;
} Bool_darr_darr;

static Cfg_node_darr* curr_cfg;
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
        darr_foreach(idx, Cfg_node, node, *curr_cfg) {
            if (node.pos_in_block == curr_pos) {
                unreachable("");
            }
        }
#   endif // NDEBUG

    Cfg_node new_node = (Cfg_node) {.label_name = label->name, .pos_in_block = curr_pos};
    if (!prev_is_cond_goto && curr_cfg->info.count > 0) {
        darr_append(&a_main, &new_node.preds, curr_cfg->info.count - 1);
        darr_append(&a_main, &darr_top_ref(curr_cfg)->succs, curr_cfg->info.count);
    }

    darr_append(&a_main, curr_cfg, new_node);
}

static void construct_cfg_cond_goto(Ir_cond_goto* cond_goto) {
    size_t if_true_idx = SIZE_MAX;
    darr_foreach_ref(idx, Cfg_node, curr1, *curr_cfg) {
        if (ir_name_is_equal(curr1->label_name, cond_goto->if_true)) {
            if_true_idx = idx;
            darr_append(&a_main, &curr1->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    unwrap(if_true_idx != SIZE_MAX && "could not find if true cfg node");

    size_t if_false_idx = SIZE_MAX;
    darr_foreach_ref(idx, Cfg_node, curr, *curr_cfg) {
        if (ir_name_is_equal(curr->label_name, cond_goto->if_false)) {
            if_false_idx = idx;
            darr_append(&a_main, &curr->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    unwrap(if_false_idx != SIZE_MAX && "could not find if false cfg node");

    Cfg_node* node = darr_at_ref(curr_cfg, curr_cfg_idx_for_cond_goto);
    darr_append(&a_main, &node->succs, if_true_idx);
    darr_append(&a_main, &node->succs, if_false_idx);
}

static void construct_cfg_goto(Ir_goto* lang_goto) {
    size_t branch_to_idx = SIZE_MAX;
    darr_foreach_ref(idx, Cfg_node, curr, *curr_cfg) {
        log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, ir_name_to_name(curr->label_name), NAME_FULL));
        if (ir_name_is_equal(curr->label_name, lang_goto->label)) {
            branch_to_idx = idx;
            darr_append(&a_main, &curr->preds, curr_cfg_idx_for_cond_goto);
            break;
        }
    }
    log(LOG_DEBUG, FMT"\n", ir_print(lang_goto));
    unwrap(branch_to_idx != SIZE_MAX && "could not find if true cfg node");

    Cfg_node* node = darr_at_ref(curr_cfg, curr_cfg_idx_for_cond_goto);
    darr_append(&a_main, &node->succs, branch_to_idx);
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
    unwrap(!curr_cfg);
    unwrap(curr_cfg_idx_for_cond_goto == 0);
    curr_cfg = &block->cfg;

    bool prev_is_cond_goto = false;
    {
        darr_foreach(idx, Ir*, curr, block->children) {
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
        darr_foreach(idx, Ir*, curr, block->children) {
            curr_pos = idx;
            if (curr_cfg_idx_for_cond_goto + 1 < curr_cfg->info.count && darr_at(*curr_cfg, curr_cfg_idx_for_cond_goto + 1).pos_in_block <= idx) {
                curr_cfg_idx_for_cond_goto++;
            }
            construct_cfg_ir_from_block(curr);
        }
    }

    Bool_darr_darr cfg_dominators = {0}; // eg. cfg_dominators[1] are the dominators for cfg node 1
                                       // cfg_dominators[n] is in ascending order
                                       // true means is dominator, false means not
                                       //   (eg. if cfg_dominators[1][0] is false, 
                                       //   then node 0 is not a dominator of node 1)
    //log(LOG_VERBOSE, "count cfg nodes: "SIZE_T_FMT"\n", block->cfg.info.count);

    //if 1 dominates 2, then every path from start to 2 must go through 1
    {
        darr_foreach_ref(node_idx, Cfg_node, dummy1, *curr_cfg) {
            (void) dummy1;
            Bool_darr dominators = {0};
            darr_foreach_ref(idx, Cfg_node, dummy2, *curr_cfg) {
                (void) dummy2;
                if (idx == 0 || node_idx != 0) {
                    darr_append(&a_pass, &dominators, true);
                } else {
                    darr_append(&a_pass, &dominators, false);
                }
            }
            darr_append(&a_pass, &cfg_dominators, dominators);
        }
    }

    // TODO: move below to separate function?
    //{
    //    String buf = {0};
    //    string_extend_cstr(&a_temp, &buf, "\n");
    //    darr_foreach(idx_size_t, Bool_darr, size_t_darr, cfg_dominators) {
    //        string_extend_cstr(&a_temp, &buf, "cfg_dominators[");
    //        string_extend_size_t(&a_temp, &buf, idx_size_t);
    //        string_extend_cstr(&a_temp, &buf, "]: [");
    //        darr_foreach(idx, size_t, curr, size_t_darr) {
    //            if (idx > 0) {
    //                string_extend_cstr(&a_temp, &buf, ", ");
    //            }
    //            string_extend_size_t(&a_temp, &buf, curr);
    //        }
    //        string_extend_cstr(&a_temp, &buf, "]\n");
    //    }
    //    log(LOG_DEBUG, FMT"\n", string_print(buf));
    //}
    
    // TODO: use Lengauer–Tarjan to allow for faster times with larger cfgs?
    bool changes_occured = false;
    do {
        changes_occured = false;

        darr_foreach_ref(node_idx, Cfg_node, node, *curr_cfg) {
            Bool_darr* new_doms = darr_at_ref(&cfg_dominators, node_idx);
            if (node_idx == 35) {
                breakpoint();
            }
            darr_foreach_ref(pred_idx, size_t, pred, node->preds) {
                (void) pred;
                darr_foreach_ref(dom_idx, bool, dom, *new_doms) {
                    log(LOG_DEBUG, "dom_idx = %zu\n", dom_idx);
                    if (dom_idx == node_idx) {
                        continue;
                    }
                    bool prev_dom = *dom;
                    *dom &= darr_at(darr_at(cfg_dominators, *pred), dom_idx);
                    log(LOG_DEBUG, "%s to %s\n", prev_dom ? "true" : "false", *dom ? "true" : "false");
                    if (prev_dom != *dom) {
                        changes_occured = true;
                    }
                }
            }
            if (node_idx == 35) {
                breakpoint();
            }
        }
    } while (changes_occured);

    {
        darr_foreach_ref(node_idx, Cfg_node, node, *curr_cfg) {
            size_t pred_idx = 0;
            while (pred_idx < node->preds.info.count - node->pred_backedge_start) {
                size_t pred = darr_at(node->preds, pred_idx);
                if (darr_at(darr_at(cfg_dominators, node_idx), pred)) {
                    if (pred_idx + 1 < node->preds.info.count - node->pred_backedge_start) {
                        darr_swap(&node->preds, size_t, node->pred_backedge_start, pred_idx);
                    }
                    node->pred_backedge_start++;
                }
                pred_idx++;
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
