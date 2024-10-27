
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"
#include "../do_passes.h"
#include "../emit_llvm.h"
#include "../symbol_table.h"

void do_passes(Node_block** root, const Parameters* params) {
    Env env = {0};
    //log_tree(LOG_DEBUG, node_wrap(*root));
    start_walk(&env, root, analysis_1);
    log_tree(LOG_DEBUG, node_wrap(*root));
    if (error_count > 0) {
        exit(EXIT_CODE_FAIL);
    }
    arena_reset(&print_arena);

    start_walk(&env, root, for_and_if_to_branch);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(&env, root, add_alloca);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(&env, root, add_load_and_store);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(&env, root, change_operators);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    start_walk(&env, root, assign_llvm_ids);
    log_tree(LOG_DEBUG, node_wrap(*root));
    arena_reset(&print_arena);

    if (params->emit_llvm) {
        emit_llvm_from_tree(&env, *root);
    } else if (params->test_expected_fail) {
        log(LOG_FETAL, "expected fail did not fail\n");
        exit(EXIT_CODE_FAIL);
    } else {
        unreachable("");
    }
}
