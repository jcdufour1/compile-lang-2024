
#include "../util.h"
#include "../node.h"
#include "../nodes.h"
#include "passes.h"

void do_passes(Node_id* root) {
    walk_tree(*root, for_loop_to_branch);
    log_tree(LOG_DEBUG, *root);
}
