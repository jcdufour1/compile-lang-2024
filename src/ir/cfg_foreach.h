#ifndef CFG_FOREACH_H
#define CFG_FOREACH_H

#include <vector.h>
#include <util.h>
#include <cfg.h>

#define cfg_foreach(idx_name, cfg_name, cfg) \
    Cfg_node cfg_name = {0}; \
    size_t idx_name = 0; \
    for (; cfg_name = (idx_name < (cfg).info.count) ? vec_at(cfg, idx_name) : ((Cfg_node) {0}), idx_name < (cfg).info.count; (idx_name)++) 

#define cfg_foreach_ref(idx_name, cfg_name, cfg) \
    Cfg_node* cfg_name = NULL; \
    size_t idx_name = 0; \
    for (; cfg_name = (idx_name < (cfg)->info.count) ? vec_at_ref(cfg, idx_name) : NULL, idx_name < (cfg)->info.count; (idx_name)++) 

static inline bool is_at_end_cfg_node(size_t ir_idx_name, Ir_vec block_children) {
    if (
        ir_idx_name + 1 < block_children.info.count &&
        ir_is_label(vec_at(block_children, ir_idx_name + 1))
    ) {
        return true;
    }

    if (ir_idx_name + 1 >= block_children.info.count) {
        return true;
    }

    return false;
}

// TODO: rename at_end_of_cfg_node variable to avoid conflicts?
#define ir_in_cfg_node_foreach(ir_idx_name, ir_name, cfg_node, block_children) \
    bool at_end_of_cfg_node_474389725 = false; \
    bool at_end_of_cfg_node_474389725_ = at_end_of_cfg_node_474389725; \
    size_t ir_idx_name = (cfg_node).pos_in_block; \
    Ir* ir_name = NULL; \
    for ( \
        ; \
            ir_name = (ir_idx_name) < (block_children).info.count ? vec_at(block_children, ir_idx_name) : NULL, \
            at_end_of_cfg_node_474389725 = at_end_of_cfg_node_474389725_, \
            at_end_of_cfg_node_474389725_ = is_at_end_cfg_node(ir_idx_name, block_children), \
            !at_end_of_cfg_node_474389725 \
        ; \
        (ir_idx_name)++ \
    ) 

#endif // CFG_FOREACH_H

