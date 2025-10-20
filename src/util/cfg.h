#ifndef CFG_H
#define CFG_H

#include <util.h>
#include <vector.h>
#include <size_t_vec.h>

typedef struct {
    Size_t_vec preds;
    Size_t_vec succs;
} Cfg_node;

typedef struct {
    Vec_base info;
    Cfg_node* buf;
} Cfg_node_vec;

#endif // CFG_H
