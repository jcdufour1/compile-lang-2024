#include "passes.h"
#include <node.h>
#include <nodes.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <type_checking.h>

Node_block* analysis_1(Env* env, Node_block* root) {
    Node_block* new_block = NULL;
    if (!try_set_block_types(env, &new_block, root)) {
        new_block = NULL;
    }
    return new_block;
}
