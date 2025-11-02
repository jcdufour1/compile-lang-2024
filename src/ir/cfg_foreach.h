#ifndef CFG_FOREACH_H
#define CFG_FOREACH_H

#include <vector.h>
#include <util.h>
#include <cfg.h>

#define cfg_foreach(idx_name, cfg_name, vector) \
    Cfg_node cfg_name = {0}; \
    for (*(idx_name) = 0; cfg_name = (*(idx_name) < (vector).info.count) ? vec_at(vector, *(idx_name)) : ((Cfg_node) {0}), *(idx_name) < (vector).info.count; (*(idx_name))++) 


#endif // CFG_FOREACH_H

