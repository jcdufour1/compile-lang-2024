#ifndef CFG_FOREACH_H
#define CFG_FOREACH_H

#include <vector.h>
#include <util.h>
#include <cfg.h>

#define cfg_foreach(idx_name, cfg_name, vector) \
    Cfg_node cfg_name = {0}; \
    for (*(idx_name) = 0; cfg_name = (*(idx_name) < (vector).info.count) ? vec_at(vector, *(idx_name)) : ((Cfg_node) {0}), *(idx_name) < (vector).info.count; (*(idx_name))++) 

// TODO: rename at_end_of_cfg_node variable to avoid conflicts?
#define ir_in_cfg_node_foreach(ir_idx_name, ir_name, cfg_node, block_children) \
    bool at_end_of_cfg_node = false; \
    size_t ir_idx_name = (cfg_node).pos_in_block; \
    Ir* ir_name = NULL; \
    for ( \
        ; \
            ir_name = (!at_end_of_cfg_node) ? vec_at(block_children, ir_idx_name) : NULL, \
            at_end_of_cfg_node = ir_idx_name < (block_children).info.count && (ir_idx_name + 1 == (block_children).info.count || ir_is_label(vec_at(block_children, ir_idx_name + 1))), \
            !at_end_of_cfg_node \
        ; \
        (ir_idx_name)++ \
    ) 

#endif // CFG_FOREACH_H

