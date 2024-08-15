#include "newstring.h"
#include "node.h"
#include "nodes.h"
#include "str_view.h"
#include "util.h"

static const char* NODE_LITERAL_DESCRIPTION = "literal";
static const char* NODE_FUNCTION_CALL_DESCRIPTION = "fn_call";
static const char* NODE_FUNCTION_DEFINITION_DESCRIPTION = "fn_def";
static const char* NODE_FUNCTION_PARAMETERS_DESCRIPTION = "fn_params";
static const char* NODE_FUNCTION_RETURN_TYPES_DESCRIPTION = "fn_return_types";
static const char* NODE_FUNCTION_BODY_DESCRIPTION = "fn_body";

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_idx root, const char* file, int line) {
    static String padding = {0};
    string_set_to_zero_len(&padding);

    for (int idx = 0; idx < pad_x; idx++) {
        string_append(&padding, ' ');
    }

    log_file(file, line, log_level, STRING_FMT NODE_FMT, string_print(padding), node_print(root, pad_x));

    if (nodes_at(root)->parameters != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->parameters, file, line);
    }
    if (nodes_at(root)->return_types != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->return_types, file, line);
    }
    if (nodes_at(root)->body != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->body, file, line);
    }
    if (nodes_at(root)->right != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->right, file, line);
    }
    if (nodes_at(root)->left != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->left, file, line);
    }
}

static Str_view node_type_get_strv(NODE_TYPE node_type) {
    switch (node_type) {
        case NODE_LITERAL:
            return str_view_from_cstr(NODE_LITERAL_DESCRIPTION);
        case NODE_FUNCTION_CALL:
            return str_view_from_cstr(NODE_FUNCTION_CALL_DESCRIPTION);
        case NODE_FUNCTION_DEFINITION:
            return str_view_from_cstr(NODE_FUNCTION_DEFINITION_DESCRIPTION);
        case NODE_FUNCTION_PARAMETERS:
            return str_view_from_cstr(NODE_FUNCTION_PARAMETERS_DESCRIPTION);
        case NODE_FUNCTION_RETURN_TYPES:
            return str_view_from_cstr(NODE_FUNCTION_RETURN_TYPES_DESCRIPTION);
        case NODE_FUNCTION_BODY:
            return str_view_from_cstr(NODE_FUNCTION_BODY_DESCRIPTION);
        default:
            todo();
    }
}

String node_print_internal(Node_idx node, int pad_x) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    for (int idx = 0; idx < pad_x; idx++) {
        string_append(&buf, ' ');
    }

    string_extend_strv(&buf, nodes_at(node)->name);

    string_append(&buf, ':');

    string_extend_strv(&buf, node_type_get_strv(nodes_at(node)->type));

    string_append(&buf, '\n');

    return buf;
}

