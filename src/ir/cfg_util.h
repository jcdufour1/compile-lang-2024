#ifndef CFG_UTIL_H
#define CFG_UTIL_H

#include <darr.h>
#include <util.h>
#include <cfg.h>

#define cfg_foreach(idx_name, cfg_name, cfg) \
    Cfg_node cfg_name = {0}; \
    size_t idx_name = 0; \
    for (; cfg_name = (idx_name < (cfg).info.count) ? darr_at(cfg, idx_name) : ((Cfg_node) {0}), idx_name < (cfg).info.count; (idx_name)++) 

#define cfg_foreach_ref(idx_name, cfg_name, cfg) \
    Cfg_node* cfg_name = NULL; \
    size_t idx_name = 0; \
    for (; cfg_name = (idx_name < (cfg)->info.count) ? darr_at_ref(cfg, idx_name) : NULL, idx_name < (cfg)->info.count; (idx_name)++) 

static inline bool is_at_end_cfg_node(size_t ir_idx_name, Ir_darr block_children) {
    if (
        ir_idx_name + 1 < block_children.info.count &&
        ir_is_label(darr_at(block_children, ir_idx_name + 1))
    ) {
        return true;
    }

    if (ir_idx_name + 1 >= block_children.info.count) {
        return true;
    }

    return false;
}

#define ir_in_cfg_node_foreach(ir_idx_name, ir_name, cfg_node, block_children) \
    bool at_end_of_cfg_node_474389725 = false; \
    bool at_end_of_cfg_node_474389725_ = at_end_of_cfg_node_474389725; \
    size_t ir_idx_name = (cfg_node).pos_in_block; \
    Ir* ir_name = NULL; \
    for ( \
        ; \
            ir_name = (ir_idx_name) < (block_children).info.count ? darr_at(block_children, ir_idx_name) : NULL, \
            at_end_of_cfg_node_474389725 = at_end_of_cfg_node_474389725_, \
            at_end_of_cfg_node_474389725_ = is_at_end_cfg_node(ir_idx_name, block_children), \
            !at_end_of_cfg_node_474389725 \
        ; \
        (ir_idx_name)++ \
    ) 

// TODO: move cfg_dump_internal and cfg_dump to another file, or rename this file
// TODO: make general version of this function that does not require Init_table_darr
static void cfg_dump_internal(const char* file, int line, LOG_LEVEL log_level, Cfg_node_darr cfg, Ir_darr block_children, Init_table_darr cfg_node_areas) {
#   define log_thing(...) log_internal_ex(stderr, log_level, false, file, line, INDENT_WIDTH, __VA_ARGS__)

    cfg_foreach(cfg_node_idx_, curr2, cfg) {
        log_thing("frame "SIZE_T_FMT":\n", cfg_node_idx_);
        ir_in_cfg_node_foreach(block_idx, ir, curr2, block_children) {
            log_thing(FMT, strv_print(ir_print_internal(ir, 2*INDENT_WIDTH)));
        }
        log_thing("\n");

        init_level_log_internal(LOG_DEBUG, __FILE__, __LINE__, 0 /* TODO */, darr_at(cfg_node_areas, cfg_node_idx_), INDENT_WIDTH);
        log_thing(FMT, cfg_node_print(curr2, cfg_node_idx_));
        log_thing("\n\n\n");
    }

#   undef log_thing
}

#define cfg_dump(log_level, cfg, block_children, cfg_node_areas) \
    cfg_dump_internal(__FILE__, __LINE__, log_level, cfg, block_children, cfg_node_areas)

#endif // CFG_UTIL_H

