#include "passes.h"
#include <tast.h>
#include <tasts.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>

Tast_block* analysis_1(Env* env, Tast_block* root) {
    Tast_block* new_block = NULL;
    if (!try_set_block_types(env, &new_block, root, false)) {
        new_block = NULL;
    }
    return new_block;
}
