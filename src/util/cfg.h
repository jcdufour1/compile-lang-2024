#ifndef CFG_H
#define CFG_H

#include <util.h>
#include <darr.h>
#include <size_t_darr.h>
#include <name.h>

typedef struct {
    Size_t_darr preds;
    Size_t_darr succs;
    Ir_name label_name; // should be {0} if pos_in_block is at start of block
    size_t pred_backedge_start; // eg. if preds has one non-backedge node, then pred_backedge_start == 1
                                //   if 1 is backedge of 2, control flow must pass through 1 to get to 2
    size_t pos_in_block;
} Cfg_node;

typedef struct {
    Vec_base info;
    Cfg_node* buf;
} Cfg_node_darr;

Strv cfg_node_print_internal(Cfg_node node, size_t idx, Indent indent);

#define cfg_node_print(node, idx) \
    strv_print(cfg_node_print_internal(node, idx, 0))

#endif // CFG_H
