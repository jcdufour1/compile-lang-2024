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

    if (nodes_at(root)->right_child != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->right_child, file, line);
    }
    if (nodes_at(root)->left_child != NODE_IDX_NULL) {
        todo();
    }
    if (nodes_at(root)->next != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x, nodes_at(root)->next, file, line);
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

static void append_strv_in_par(String* string, Str_view str_view) {
    string_append(string, '(');
    string_extend_strv(string, str_view);
    string_append(string, ')');
}

String node_print_internal(Node_idx node, int pad_x) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    for (int idx = 0; idx < pad_x; idx++) {
        string_append(&buf, ' ');
    }

    string_extend_strv(&buf, node_type_get_strv(nodes_at(node)->type));


    switch (nodes_at(node)->type) {
        case NODE_FUNCTION_DEFINITION:
            append_strv_in_par(&buf, nodes_at(node)->name);
            break;
        case NODE_FUNCTION_RETURN_TYPES:
            append_strv_in_par(&buf, nodes_at(node)->lang_type);
            break;
        case NODE_FUNCTION_CALL:
            append_strv_in_par(&buf, nodes_at(node)->name);
            break;
        case NODE_LITERAL:
            append_strv_in_par(&buf, nodes_at(node)->name);
            break;
        case NODE_FUNCTION_PARAMETERS:
            // fallthrough
        case NODE_FUNCTION_BODY:
            break;
        default:
            log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(nodes_at(node)->type)));
            unreachable();
    }

    string_append(&buf, '\n');

    return buf;
}

