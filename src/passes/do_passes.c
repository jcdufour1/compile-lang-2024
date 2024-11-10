
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"
#include "../do_passes.h"
#include "../symbol_table.h"
#include "../tokens.h"

static void fail(void) {
    if (!params.test_expected_fail) {
        exit(EXIT_CODE_FAIL);
    }

    log(LOG_DEBUG, "%zu %zu\n", expected_fail_count, params.expected_fail_types.info.count); 
    if (expected_fail_count == params.expected_fail_types.info.count) {
        exit(EXIT_CODE_EXPECTED_FAIL);
    } else {
        log(
            LOG_FETAL, "%zu expected fails occured, but %zu expected fails were expected\n",
            expected_fail_count, params.expected_fail_types.info.count
        );
        exit(EXIT_CODE_FAIL);
    }
}

void do_passes(String file_text, const Parameters* params) {
    Tokens tokens = tokenize(file_text, *params);
    if (error_count > 0) {
        fail();
    }

    Node_block* root = parse(tokens);
    if (error_count > 0) {
        fail();
    }
    arena_reset(&print_arena);

    Env env = {0};
    //log_tree(LOG_DEBUG, node_wrap(*root));
    start_walk(&env, &root, analysis_1);
    log_tree(LOG_DEBUG, node_wrap(root));
    if (error_count > 0) {
        fail();
    }
    arena_reset(&print_arena);

    start_walk(&env, &root, for_and_if_to_branch);
    log_tree(LOG_DEBUG, node_wrap(root));
    arena_reset(&print_arena);

    start_walk(&env, &root, add_alloca);
    log_tree(LOG_DEBUG, node_wrap(root));
    arena_reset(&print_arena);

    start_walk(&env, &root, add_load_and_store);
    log_tree(LOG_DEBUG, node_wrap(root));
    arena_reset(&print_arena);

    start_walk(&env, &root, change_operators);
    log_tree(LOG_DEBUG, node_wrap(root));
    arena_reset(&print_arena);

    start_walk(&env, &root, assign_llvm_ids);
    log_tree(LOG_DEBUG, node_wrap(root));
    arena_reset(&print_arena);

    if (params->emit_llvm) {
        emit_llvm_from_tree(&env, root);
    } else if (params->test_expected_fail) {
        fail();
    } else {
        unreachable("");
    }
}
