
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"

void do_passes(Node** root) {
    walk_tree(*root, analysis_1);
    log_tree(LOG_DEBUG, *root);
    if (error_count > 0) {
        exit(1);
    }
    arena_reset(&print_arena);

    walk_tree(*root, for_and_if_to_branch);
    log_tree(LOG_DEBUG, *root);
    arena_reset(&print_arena);

    walk_tree(*root, flatten_operations);
    log_tree(LOG_DEBUG, *root);
    arena_reset(&print_arena);

    walk_tree(*root, add_alloca);
    log_tree(LOG_DEBUG, *root);
    arena_reset(&print_arena);

    // TODO: do not set NODE_STRUCT_MEMBER_SYM_TYPED lang_type in add_load_and_store
    walk_tree(*root, add_load_and_store);
    log_tree(LOG_DEBUG, *root);
    arena_reset(&print_arena);

    walk_tree(*root, assign_llvm_ids);
    log_tree(LOG_DEBUG, *root);
    arena_reset(&print_arena);
}
