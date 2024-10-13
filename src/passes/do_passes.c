
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"
#include "../do_passes.h"

void do_passes(Node_block** root) {
    //log_tree(LOG_DEBUG, node_wrap(*root));
    start_walk(root, analysis_1);
    log_tree(LOG_DEBUG, node_wrap(*root));
    if (error_count > 0) {
        exit(1);
    }
    arena_reset(&print_arena);

    start_walk(root, for_and_if_to_branch);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(root, flatten_operations);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(root, add_alloca);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(root, add_load_and_store);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(root, change_operators);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(root, assign_llvm_ids);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);
}
