
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"

void do_passes(Node_id* root) {
    walk_tree(*root, for_and_if_to_branch);
    //log_tree(LOG_DEBUG, *root);

    walk_tree(*root, add_load_and_store);
    //log_tree(LOG_DEBUG, *root);

    walk_tree(*root, assign_llvm_ids);
    //log_tree(LOG_DEBUG, *root);
}
