#include "nodes.h"
#include "str_view.h"
#include "util.h"

static const char* NODE_LITERAL_DESCRIPTION = "literal";
static const char* NODE_FUNCTION_CALL_DESCRIPTION = "fn_call";
static const char* NODE_FUNCTION_DEFINITION_DESCRIPTION = "fn_def";
static const char* NODE_FUNCTION_PARAMETERS_DESCRIPTION = "fn_params";
static const char* NODE_FUNCTION_RETURN_TYPES_DESCRIPTION = "fn_return_types";
static const char* NODE_LANG_TYPE_DESCRIPTION = "lang_type";
static const char* NODE_OPERATOR_DESCRIPTION = "operator";
static const char* NODE_BLOCK_DESCRIPTION = "block";
static const char* NODE_SYMBOL_DESCRIPTION = "sym";
static const char* NODE_RETURN_STATEMENT_DESCRIPTION = "return";
static const char* NODE_VARIABLE_DEFINITION_DESCRIPTION = "var_def";
static const char* NODE_FUNCTION_DECLARATION_DESCRIPTION = "fun_declaration";
static const char* NODE_ASSIGNMENT_DESCRIPTION = "assignment";
static const char* NODE_FOR_LOOP_DESCRIPTION = "for";
static const char* NODE_FOR_VARIABLE_DEF_DESCRIPTION = "for_var_def";
static const char* NODE_FOR_LOWER_BOUND_DESCRIPTION = "lower_bound";
static const char* NODE_FOR_UPPER_BOUND_DESCRIPTION = "upper_bound";
static const char* NODE_NO_TYPE_DESCRIPTION = "<not_parsed>";
static const char* NODE_GOTO_DESCRIPTION = "goto";

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_id root, const char* file, int line) {
    if (nodes.info.count < 1) {
        log_file_new(log_level, file, line, "<empty tree>\n");
        return;
    }

    static String padding = {0};
    string_set_to_zero_len(&padding);

    for (int idx = 0; idx < pad_x; idx++) {
        string_append(&padding, ' ');
    }

    log_file_new(log_level, file, line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(root));

    if (nodes_at(root)->left_child != NODE_IDX_NULL) {
        nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(root)->left_child, file, line);
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
        case NODE_LANG_TYPE:
            return str_view_from_cstr(NODE_LANG_TYPE_DESCRIPTION);
        case NODE_OPERATOR:
            return str_view_from_cstr(NODE_OPERATOR_DESCRIPTION);
        case NODE_BLOCK:
            return str_view_from_cstr(NODE_BLOCK_DESCRIPTION);
        case NODE_SYMBOL:
            return str_view_from_cstr(NODE_SYMBOL_DESCRIPTION);
        case NODE_RETURN_STATEMENT:
            return str_view_from_cstr(NODE_RETURN_STATEMENT_DESCRIPTION);
        case NODE_VARIABLE_DEFINITION:
            return str_view_from_cstr(NODE_VARIABLE_DEFINITION_DESCRIPTION);
        case NODE_FUNCTION_DECLARATION:
            return str_view_from_cstr(NODE_FUNCTION_DECLARATION_DESCRIPTION);
        case NODE_ASSIGNMENT:
            return str_view_from_cstr(NODE_ASSIGNMENT_DESCRIPTION);
        case NODE_FOR_LOOP:
            return str_view_from_cstr(NODE_FOR_LOOP_DESCRIPTION);
        case NODE_FOR_VARIABLE_DEF:
            return str_view_from_cstr(NODE_FOR_VARIABLE_DEF_DESCRIPTION);
        case NODE_FOR_LOWER_BOUND:
            return str_view_from_cstr(NODE_FOR_LOWER_BOUND_DESCRIPTION);
        case NODE_FOR_UPPER_BOUND:
            return str_view_from_cstr(NODE_FOR_UPPER_BOUND_DESCRIPTION);
        case NODE_GOTO:
            return str_view_from_cstr(NODE_GOTO_DESCRIPTION);
        case NODE_NO_TYPE:
            return str_view_from_cstr(NODE_NO_TYPE_DESCRIPTION);
        default:
            log(LOG_FETAL, "node_type: %d\n", node_type);
            todo();
    }
}

String node_print_internal(Node_id node) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_strv(&buf, node_type_get_strv(nodes_at(node)->type));

    switch (nodes_at(node)->type) {
        case NODE_LITERAL:
            string_extend_strv_in_gtlt(&buf, token_type_to_str_view(nodes_at(node)->token_type));
            string_extend_strv_in_par(&buf, nodes_at(node)->name);
            string_extend_strv_in_par(&buf, nodes_at(node)->str_data);
            break;
        case NODE_SYMBOL:
            // fallthrough
        case NODE_FUNCTION_DEFINITION:
            // fallthrough
        case NODE_FUNCTION_CALL:
            string_extend_strv_in_par(&buf, nodes_at(node)->name);
            break;
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_LANG_TYPE:
            string_extend_strv_in_gtlt(&buf, nodes_at(node)->lang_type);
            break;
        case NODE_OPERATOR:
            string_extend_strv(&buf, token_type_to_str_view(nodes_at(node)->token_type));
            break;
        case NODE_FUNCTION_PARAMETERS:
            // fallthrough
        case NODE_FUNCTION_RETURN_TYPES:
            // fallthrough
        case NODE_BLOCK:
            // fallthrough
        case NODE_RETURN_STATEMENT:
            // fallthrough
        case NODE_FUNCTION_DECLARATION:
            // fallthrough
        case NODE_ASSIGNMENT:
            // fallthrough
        case NODE_FOR_LOOP:
            // fallthrough
        case NODE_FOR_UPPER_BOUND:
            // fallthrough
        case NODE_FOR_LOWER_BOUND:
            // fallthrough
        case NODE_FOR_VARIABLE_DEF:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_NO_TYPE:
            break;
        default:
            log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(nodes_at(node)->type)));
            unreachable();
    }

    return buf;
}

