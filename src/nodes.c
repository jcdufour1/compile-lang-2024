#include "nodes.h"
#include "newstring.h"
#include "util.h"

void Nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, const Node* root, const char* file, int line) {
    static String padding = {0};
    String_set_to_zero_len(&padding);

    for (int idx = 0; idx < pad_x; idx++) {
        String_append(&padding, ' ');
    }

    log_file(file, line, log_level, STRING_FMT NODE_FMT, String_print(padding), Node_print(root, pad_x));

    if (root->parameters) {
        Nodes_log_tree_rec(log_level, pad_x + 2, root->parameters, file, line);
    }
    if (root->return_types) {
        Nodes_log_tree_rec(log_level, pad_x + 2, root->return_types, file, line);
    }
    if (root->body) {
        Nodes_log_tree_rec(log_level, pad_x + 2, root->body, file, line);
    }
    if (root->right) {
        Nodes_log_tree_rec(log_level, pad_x + 2, root->right, file, line);
    }
    if (root->left) {
        Nodes_log_tree_rec(log_level, pad_x + 2, root->left, file, line);
    }
}

