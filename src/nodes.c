#include "nodes.h"
#include "str_view.h"
#include "util.h"
#include "node_ptr_vec.h"

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
static const char* NODE_STORE_VARIABLE_DESCRIPTION = "store_variable";
static const char* NODE_LOAD_VARIABLE_DESCRIPTION = "load_varaible";
static const char* NODE_IF_STATEMENT_DESCRIPTION = "if_statement";
static const char* NODE_IF_CONDITION_DESCRIPTION = "if_condition";
static const char* NODE_FUNCTION_PARAM_SYM_DESCRIPTION = "fun_param_sym";
static const char* NODE_STRUCT_DEFINITION_DESCRIPTION = "struct_def";
static const char* NODE_STRUCT_MEMBER_SYM_DESCRIPTION = "struct_member_sym";
static const char* NODE_STRUCT_MEMBER_SYM_PIECE_DESCRIPTION = "struct_member_sym_piece";
static const char* NODE_STRUCT_LITERAL_DESCRIPTION = "struct_literal";
static const char* NODE_STORE_STRUCT_MEMBER_DESCRIPTION = "store_member";
static const char* NODE_LLVM_STORE_LITERAL_DESCRIPTION = "llvm_store_literal";
static const char* NODE_LLVM_STORE_STRUCT_LITERAL_DESCRIPTION = "llvm_store_struct_literal";
static const char* NODE_LOAD_STRUCT_MEMBER_DESCRIPTION = "load_member";
static const char* NODE_LOAD_STRUCT_ELEMENT_PTR_DESCRIPTION = "load_element_ptr";
static const char* NODE_LOAD_ANOTHER_NODE_DESCRIPTION = "load_another_node";
static const char* NODE_STORE_ANOTHER_NODE_DESCRIPTION = "store_another_node";
static const char* NODE_NODE_FUNCTION_RETURN_VALUE_SYM_DESCRIPTION = "fun_return_value_sym";
static const char* NODE_NODE_OPERATOR_RETURN_VALUE_SYM_DESCRIPTION = "operator_return_value_sym";
static const char* NODE_NO_TYPE_DESCRIPTION = "<not_parsed>";

#ifndef NDEBUG
static void nodes_assert_tree_linkage_is_consistant_internal(Node_ptr_vec* nodes_visited, const Node* root) {
    if (!root) {
        return;
    }

    //nodes_log_tree_rec(LOG_DEBUG, 0, root, __FILE__, __LINE__);
    assert(!node_ptr_vec_in(nodes_visited, root) && "same node found more than once");
    node_ptr_vec_append(nodes_visited, root);

    assert(
        (root != root->next) && \
        (root != root->prev) && \
        (root != root->left_child) && \
        (root != root->parent) && \
        "node points to itself"
    );

    Node* base_parent = root->parent;
    nodes_foreach_from_curr_const(curr_node, root) {
        bool dummy_bool = true;
        assert(dummy_bool);
        if (curr_node->next) {
            //node_printf(curr_node->prev->next);
            assert(curr_node == curr_node->next->prev);
        }
        if (curr_node->prev) {
            if (curr_node != curr_node->prev->next) {
                node_printf(curr_node);
                node_printf(curr_node->prev);
                node_printf(curr_node->prev->next);
                assert(false);
            }
        }
        assert(curr_node->parent == base_parent);

        Node* left_child = curr_node->left_child;
        if (left_child) {
            assert(curr_node == left_child->parent);
            nodes_assert_tree_linkage_is_consistant_internal(nodes_visited, left_child);
        }
    }
}

void nodes_assert_tree_linkage_is_consistant(const Node* root) {
    static Node_ptr_vec nodes_visited = {0};
    node_ptr_vec_set_to_zero_len(&nodes_visited);
    nodes_assert_tree_linkage_is_consistant_internal(&nodes_visited, root);
}
#endif // NDEBUG

void nodes_log_tree_rec(LOG_LEVEL log_level, int pad_x, const Node* root, const char* file, int line) {
    if (!root_of_tree) {
        log_file_new(log_level, file, line, "<empty tree>\n");
        return;
    }

    static String padding = {0};

    nodes_foreach_from_curr_const(curr_node, root) {
        string_set_to_zero_len(&padding);
        for (int idx = 0; idx < pad_x; idx++) {
            string_append(&padding, ' ');
        }

        log_file_new(log_level, file, line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(curr_node));
        if (curr_node->left_child) {
            nodes_log_tree_rec(log_level, pad_x + 2, curr_node->left_child, file, line);
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
        case NODE_STORE_VARIABLE:
            return str_view_from_cstr(NODE_STORE_VARIABLE_DESCRIPTION);
        case NODE_LOAD_VARIABLE:
            return str_view_from_cstr(NODE_LOAD_VARIABLE_DESCRIPTION);
        case NODE_IF_STATEMENT:
            return str_view_from_cstr(NODE_IF_STATEMENT_DESCRIPTION);
        case NODE_IF_CONDITION:
            return str_view_from_cstr(NODE_IF_CONDITION_DESCRIPTION);
        case NODE_FUNCTION_PARAM_SYM:
            return str_view_from_cstr(NODE_FUNCTION_PARAM_SYM_DESCRIPTION);
        case NODE_STRUCT_DEFINITION:
            return str_view_from_cstr(NODE_STRUCT_DEFINITION_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM_PIECE:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_PIECE_DESCRIPTION);
        case NODE_STRUCT_LITERAL:
            return str_view_from_cstr(NODE_STRUCT_LITERAL_DESCRIPTION);
        case NODE_STORE_STRUCT_MEMBER:
            return str_view_from_cstr(NODE_STORE_STRUCT_MEMBER_DESCRIPTION);
        case NODE_LOAD_STRUCT_MEMBER:
            return str_view_from_cstr(NODE_LOAD_STRUCT_MEMBER_DESCRIPTION);
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            return str_view_from_cstr(NODE_NODE_FUNCTION_RETURN_VALUE_SYM_DESCRIPTION);
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            return str_view_from_cstr(NODE_NODE_OPERATOR_RETURN_VALUE_SYM_DESCRIPTION);
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            return str_view_from_cstr(NODE_LOAD_STRUCT_ELEMENT_PTR_DESCRIPTION);
        case NODE_LOAD_ANOTHER_NODE:
            return str_view_from_cstr(NODE_LOAD_ANOTHER_NODE_DESCRIPTION);
        case NODE_STORE_ANOTHER_NODE:
            return str_view_from_cstr(NODE_STORE_ANOTHER_NODE_DESCRIPTION);
        case NODE_LLVM_STORE_LITERAL:
            return str_view_from_cstr(NODE_LLVM_STORE_LITERAL_DESCRIPTION);
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            return str_view_from_cstr(NODE_LLVM_STORE_STRUCT_LITERAL_DESCRIPTION);
        case NODE_NO_TYPE:
            return str_view_from_cstr(NODE_NO_TYPE_DESCRIPTION);
        default:
            log(LOG_FETAL, "node_type: %d\n", node_type);
            todo();
    }
}

static void extend_node_text(String* string, const Node* node, bool do_recursion) {
    string_extend_strv(string, node_type_get_strv(node->type));

    switch (node->type) {
        case NODE_ALLOCA:
            // fallthrough
        case NODE_STORE_VARIABLE:
            // fallthrough
        case NODE_FUNCTION_PARAM_SYM:
            // fallthrough
        case NODE_LOAD_VARIABLE:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_COND_GOTO:
            // fallthrough
        case NODE_LABEL:
            string_extend_strv_in_par(string, node->name);
            break;
        case NODE_LITERAL:
            string_extend_strv_in_gtlt(string, token_type_to_str_view(node->token_type));
            string_extend_strv(string, node->name);
            string_extend_strv_in_par(string, node->str_data);
            break;
        case NODE_SYMBOL:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM_PIECE:
            // fallthrough
        case NODE_FUNCTION_DEFINITION:
            // fallthrough
        case NODE_LOAD_STRUCT_MEMBER:
            // fallthrough
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            // fallthrough
        case NODE_STORE_STRUCT_MEMBER:
            // fallthrough
        case NODE_STRUCT_DEFINITION:
            // fallthrough
        case NODE_FUNCTION_CALL:
            string_extend_strv_in_par(string, node->name);
            string_extend_size_t(string, node->llvm_id);
            break;
        case NODE_LANG_TYPE:
            string_extend_strv_in_gtlt(string, node->lang_type);
            break;
        case NODE_OPERATOR:
            string_extend_strv(string, token_type_to_str_view(node->token_type));
            break;
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_STRUCT_LITERAL:
            string_extend_strv_in_gtlt(string, node->lang_type);
            string_extend_strv(string, node->name);
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
        case NODE_IF_STATEMENT:
            // fallthrough
        case NODE_IF_CONDITION:
            // fallthrough
        case NODE_NO_TYPE:
            break;
        case NODE_LOAD_ANOTHER_NODE:
            // fallthrough
        case NODE_STORE_ANOTHER_NODE:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM:
            // fallthrough
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            // fallthrough
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            // fallthrough
        case NODE_LLVM_STORE_LITERAL:
            // fallthrough
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            // fallthrough
            string_extend_strv_in_gtlt(string, node->lang_type);
            string_extend_strv_in_par(string, node->name);
                string_extend_cstr(string, "[");
            if (node->node_src && do_recursion) {
                string_extend_cstr(string, "node_src:");
                extend_node_text(string, node->node_src, false);
            }
            string_extend_cstr(string, " ");
            if (node->node_dest && do_recursion) {
                string_extend_cstr(string, "node_dest:");
                extend_node_text(string, node->node_dest, false);
            }
            string_extend_cstr(string, "]");
            break;
        default:
            log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(node->type)));
            unreachable("");
    }
}

Str_view node_print_internal(const Node* node) {
    String buf = {0}; // todo: put these strings in an arena to free, etc

    if (!node) {
        string_extend_cstr(&buf, "<null>");
        Str_view str_view = {.str = buf.buf, .count = buf.info.count};
        return str_view;
    }

    extend_node_text(&buf, node, true);

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

