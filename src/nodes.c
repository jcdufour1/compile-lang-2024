#include "nodes.h"
#include "str_view.h"
#include "util.h"
#include "size_t_vec.h"

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
static const char* NODE_GOTO_DESCRIPTION = "goto";
static const char* NODE_LABEL_DESCRIPTION = "label";
static const char* NODE_COND_GOTO_DESCRIPTION = "cond_goto";
static const char* NODE_ALLOCA_DESCRIPTION = "alloca";
static const char* NODE_STORE_DESCRIPTION = "store";
static const char* NODE_LOAD_DESCRIPTION = "load";
static const char* NODE_NO_TYPE_DESCRIPTION = "<not_parsed>";

static void nodes_assert_tree_linkage_is_consistant_internal(Size_t_vec* nodes_visited, Node_id root) {
    if (root == NODE_IDX_NULL) {
        return;
    }

    assert(!size_t_vec_in(nodes_visited, root) && "same node found more than once");
    size_t_vec_append(nodes_visited, root);

    {
        Node_id left_child = nodes_at(root)->left_child;
        if (left_child != NODE_IDX_NULL) {
            assert(root == nodes_at(left_child)->parent);
            nodes_assert_tree_linkage_is_consistant_internal(nodes_visited, left_child);
        }
    }

    {
        Node_id next = nodes_at(root)->next;
        if (next != NODE_IDX_NULL) {
            // TODO: don't do recursion for next and prev
            assert(root == nodes_at(next)->prev);
            nodes_assert_tree_linkage_is_consistant_internal(nodes_visited, next);
        }
    }

    {
        Node_id prev = nodes_at(root)->prev;
        if (prev != NODE_IDX_NULL) {
            assert(root == nodes_at(prev)->next);
        }
    }
}

void nodes_assert_tree_linkage_is_consistant(Node_id root) {
    static Size_t_vec nodes_visited = {0};
    memset(&nodes_visited, 0, sizeof(nodes_visited));
    nodes_assert_tree_linkage_is_consistant_internal(&nodes_visited, root);
}

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, Node_id root, const char* file, int line) {
    nodes_assert_tree_linkage_is_consistant(root);

    if (nodes.info.count < 1) {
        log_file_new(log_level, file, line, "<empty tree>\n");
        return;
    }

    static String padding = {0};

    nodes_foreach_from_curr(curr_node, root) {
        string_set_to_zero_len(&padding);
        for (int idx = 0; idx < pad_x; idx++) {
            string_append(&padding, ' ');
        }

        log_file_new(log_level, file, line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(curr_node));
        if (nodes_at(curr_node)->left_child != NODE_IDX_NULL) {
            nodes_log_tree_rec(log_level, pad_x + 2, nodes_at(curr_node)->left_child, file, line);
        }
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
        case NODE_COND_GOTO:
            return str_view_from_cstr(NODE_COND_GOTO_DESCRIPTION);
        case NODE_LABEL:
            return str_view_from_cstr(NODE_LABEL_DESCRIPTION);
        case NODE_ALLOCA:
            return str_view_from_cstr(NODE_ALLOCA_DESCRIPTION);
        case NODE_STORE:
            return str_view_from_cstr(NODE_STORE_DESCRIPTION);
        case NODE_LOAD:
            return str_view_from_cstr(NODE_LOAD_DESCRIPTION);
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
        case NODE_ALLOCA:
            // fallthrough
        case NODE_STORE:
            // fallthrough
        case NODE_LOAD:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_COND_GOTO:
            // fallthrough
        case NODE_LABEL:
            string_extend_strv_in_par(&buf, nodes_at(node)->name);
            break;
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
        case NODE_LANG_TYPE:
            string_extend_strv_in_gtlt(&buf, nodes_at(node)->lang_type);
            break;
        case NODE_OPERATOR:
            string_extend_strv(&buf, token_type_to_str_view(nodes_at(node)->token_type));
            break;
        case NODE_VARIABLE_DEFINITION:
            string_extend_strv_in_gtlt(&buf, nodes_at(node)->lang_type);
            string_extend_strv_in_par(&buf, nodes_at(node)->name);
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
        case NODE_NO_TYPE:
            break;
        default:
            log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(nodes_at(node)->type)));
            unreachable();
    }

    return buf;
}

