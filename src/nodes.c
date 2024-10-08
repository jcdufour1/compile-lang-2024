#include "nodes.h"
#include "str_view.h"
#include "util.h"
#include "node_ptr_vec.h"
#include "node_utils.h"
#include "do_passes.h"

static void extend_node_text(Arena* arena, String* string, const Node* node, bool do_recursion);

static const char* NODE_LITERAL_DESCRIPTION = "literal";
static const char* NODE_FUNCTION_CALL_DESCRIPTION = "fn_call";
static const char* NODE_FUNCTION_DEFINITION_DESCRIPTION = "fn_def";
static const char* NODE_FUNCTION_PARAMETERS_DESCRIPTION = "fn_params";
static const char* NODE_FUNCTION_RETURN_TYPES_DESCRIPTION = "fn_return_types";
static const char* NODE_LANG_TYPE_DESCRIPTION = "lang_type";
static const char* NODE_OPERATOR_DESCRIPTION = "operator";
static const char* NODE_BLOCK_DESCRIPTION = "block";
static const char* NODE_SYMBOL_UNTYPED_DESCRIPTION = "sym_untyped";
static const char* NODE_SYMBOL_TYPED_DESCRIPTION= "sym_typed";
static const char* NODE_RETURN_STATEMENT_DESCRIPTION = "return";
static const char* NODE_VARIABLE_DEFINITION_DESCRIPTION = "var_def";
static const char* NODE_FUNCTION_DECLARATION_DESCRIPTION = "fun_declaration";
static const char* NODE_ASSIGNMENT_DESCRIPTION = "assignment";
static const char* NODE_FOR_LOOP_DESCRIPTION = "for";
static const char* NODE_FOR_LOWER_BOUND_DESCRIPTION = "lower_bound";
static const char* NODE_FOR_UPPER_BOUND_DESCRIPTION = "upper_bound";
static const char* NODE_BREAK_DESCRIPTION = "break";
static const char* NODE_GOTO_DESCRIPTION = "goto";
static const char* NODE_LABEL_DESCRIPTION = "label";
static const char* NODE_COND_GOTO_DESCRIPTION = "cond_goto";
static const char* NODE_ALLOCA_DESCRIPTION = "alloca";
static const char* NODE_IF_STATEMENT_DESCRIPTION = "if_statement";
static const char* NODE_IF_CONDITION_DESCRIPTION = "if_condition";
static const char* NODE_STRUCT_DEFINITION_DESCRIPTION = "struct_def";
static const char* NODE_STRUCT_LITERAL_DESCRIPTION = "struct_literal";
static const char* NODE_STRUCT_MEMBER_SYM_TYPED_DESCRIPTION = "struct_member_sym_typed";
static const char* NODE_STRUCT_MEMBER_SYM_UNTYPED_DESCRIPTION = "struct_member_sym_untyped";
static const char* NODE_STRUCT_MEMBER_SYM_PIECE_TYPED_DESCRIPTION = "struct_member_sym_piece_typed";
static const char* NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED_DESCRIPTION = "struct_member_sym_piece_untyped";
static const char* NODE_LLVM_STORE_LITERAL_DESCRIPTION = "llvm_store_literal";
static const char* NODE_LLVM_STORE_STRUCT_LITERAL_DESCRIPTION = "llvm_store_struct_literal";
static const char* NODE_LOAD_STRUCT_ELEMENT_PTR_DESCRIPTION = "load_element_ptr";
static const char* NODE_LOAD_ANOTHER_NODE_DESCRIPTION = "load_another_node";
static const char* NODE_STORE_ANOTHER_NODE_DESCRIPTION = "store_another_node";
static const char* NODE_PTR_BYVAL_SYM_DESCRIPTION = "byval_sym";
static const char* NODE_LLVM_REGISTER_SYM_DESCRIPTION = "llvm_register_sym";
static const char* NODE_NO_TYPE_DESCRIPTION = "<not_parsed>";

#ifndef NDEBUG
#if 0
static void nodes_assert_tree_linkage_is_consistant_internal(Node_ptr_vec* nodes_visited, const Node* root) {
    if (!root) {
        return;
    }

    assert(!node_ptr_vec_in(nodes_visited, root) && "same node found more than once");
    node_ptr_vec_append(nodes_visited, root);

    assert(
        (root != root->next) && \
        (root != root->prev) && \
        (root != get_left_child_const(root)) && \
        (root != root->parent) && \
        "node points to itself"
    );

    Node* base_parent = root->parent;
    nodes_foreach_from_curr_const(curr_node, root) {
        bool dummy_bool = true;
        assert(dummy_bool);
        if (curr_node->next) {
            assert(curr_node == curr_node->next->prev);
        }
        if (curr_node->prev) {
            if (curr_node != curr_node->prev->next) {
                assert(false);
            }
        }
        assert(curr_node->parent == base_parent);

        const Node* left_child = get_left_child_const(curr_node);
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

#else

void nodes_assert_tree_linkage_is_consistant(const Node* root) {
    (void) root;
}
#endif // 0
#endif // NDEBUG

void extend_lang_type_to_string(Arena* arena, String* string, Lang_type lang_type, bool surround_in_lt_gt) {
    if (surround_in_lt_gt) {
        string_append(arena, string, '<');
    }
    string_extend_strv(arena, string, lang_type.str);
    if (lang_type.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < lang_type.pointer_depth; idx++) {
        string_append(arena, string, '*');
    }
    if (surround_in_lt_gt) {
        string_append(arena, string, '>');
    }
}

static int log_file_level = 0;
static const char* log_file = NULL;
static int log_line = 0;

Str_view lang_type_print_internal(Arena* arena, Lang_type lang_type, bool surround_in_lt_gt) {
    String buf = {0};
    extend_lang_type_to_string(arena, &buf, lang_type, surround_in_lt_gt);
    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

bool log_node_in_tree_internal(Node* node, int recursion_depth) {
    static String padding = {0};
    string_set_to_zero_len(&padding);

    for (int idx = 0; idx < 2*recursion_depth; idx++) {
        string_append(&print_arena, &padding, ' ');
    }

    log_file_new(log_file_level, log_file, log_line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(node));
    return false;
}

void nodes_log_tree_internal(LOG_LEVEL log_level, const Node* root, const char* file, int line) {
    if (!root_of_tree) {
        log_file_new(log_level, file, line, "<empty tree>\n");
        return;
    }

    log_file_level = log_level;
    log_file = file;
    log_line = line;

    walk_tree((Node*)root, 0, log_node_in_tree_internal);
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
        case NODE_SYMBOL_UNTYPED:
            return str_view_from_cstr(NODE_SYMBOL_UNTYPED_DESCRIPTION);
        case NODE_SYMBOL_TYPED:
            return str_view_from_cstr(NODE_SYMBOL_TYPED_DESCRIPTION);
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
        case NODE_IF_STATEMENT:
            return str_view_from_cstr(NODE_IF_STATEMENT_DESCRIPTION);
        case NODE_IF_CONDITION:
            return str_view_from_cstr(NODE_IF_CONDITION_DESCRIPTION);
        case NODE_STRUCT_DEFINITION:
            return str_view_from_cstr(NODE_STRUCT_DEFINITION_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_UNTYPED_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_TYPED_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_PIECE_TYPED_DESCRIPTION);
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            return str_view_from_cstr(NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED_DESCRIPTION);
        case NODE_STRUCT_LITERAL:
            return str_view_from_cstr(NODE_STRUCT_LITERAL_DESCRIPTION);
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
        case NODE_BREAK:
            return str_view_from_cstr(NODE_BREAK_DESCRIPTION);
        case NODE_PTR_BYVAL_SYM:
            return str_view_from_cstr(NODE_PTR_BYVAL_SYM_DESCRIPTION);
        case NODE_LLVM_REGISTER_SYM:
            return str_view_from_cstr(NODE_LLVM_REGISTER_SYM_DESCRIPTION);
        case NODE_NO_TYPE:
            return str_view_from_cstr(NODE_NO_TYPE_DESCRIPTION);
        default:
            log(LOG_FETAL, "node_type: %d\n", node_type);
            todo();
    }
}

static void print_node_dest(Arena* arena, String* string, const Node* node, bool do_recursion) {
    string_extend_cstr(arena, string, " ");
    if (get_node_dest_const(node) && do_recursion) {
        string_extend_cstr(arena, string, "node_dest:");
        extend_node_text(arena, string, get_node_dest_const(node), false);
    }
    string_extend_cstr(arena, string, "]");
}

static void print_node_src(Arena* arena, String* string, const Node* node, bool do_recursion) {
    string_extend_cstr(arena, string, "[");
    if (get_node_src_const(node) && do_recursion) {
        string_extend_cstr(arena, string, "node_src:");
        extend_node_text(arena, string, get_node_src_const(node), false);
    }
}

static void extend_node_text(Arena* arena, String* string, const Node* node, bool do_recursion) {
    string_extend_strv(arena, string, node_type_get_strv(node->type));

    switch (node->type) {
        case NODE_ALLOCA:
            // fallthrough
        case NODE_GOTO:
            // fallthrough
        case NODE_LABEL:
            string_extend_strv_in_par(arena, string, get_node_name(node));
            break;
        case NODE_COND_GOTO:
            // fallthrough
            break;
        case NODE_LITERAL:
            extend_lang_type_to_string(arena, string, node_unwrap_literal_const(node)->lang_type, true);
            string_extend_strv(arena, string, get_node_name(node));
            string_extend_strv_in_par(arena, string, node_unwrap_literal_const(node)->str_data);
            break;
        case NODE_SYMBOL_UNTYPED:
            string_extend_strv_in_par(arena, string, get_node_name(node));
            break;
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            // fallthrough
            break;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            // fallthrough
            break;
        case NODE_FUNCTION_DEFINITION:
            // fallthrough
            break;
        case NODE_STRUCT_DEFINITION:
            // fallthrough
            break;
        case NODE_FUNCTION_CALL:
            string_extend_strv_in_par(arena, string, get_node_name(node));
            string_extend_size_t(arena, string, get_llvm_id(node));
            break;
        case NODE_LANG_TYPE:
            extend_lang_type_to_string(arena, string, node_unwrap_lang_type_const(node)->lang_type, true);
            break;
        case NODE_OPERATOR:
            string_extend_strv(arena, string, token_type_to_str_view(node_unwrap_operator_const(node)->token_type));
            break;
        case NODE_VARIABLE_DEFINITION:
            extend_lang_type_to_string(arena, string, node_unwrap_variable_def_const(node)->lang_type, true);
            string_extend_strv(arena, string, get_node_name(node));
            break;
        case NODE_STRUCT_LITERAL:
            extend_lang_type_to_string(arena, string, node_unwrap_struct_literal_const(node)->lang_type, true);
            string_extend_strv(arena, string, get_node_name(node));
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
        case NODE_IF_STATEMENT:
            // fallthrough
        case NODE_IF_CONDITION:
            // fallthrough
        case NODE_BREAK:
            // fallthrough
        case NODE_NO_TYPE:
            break;
        case NODE_LOAD_ANOTHER_NODE:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            print_node_src(arena, string, node, do_recursion);
            break;
        case NODE_STORE_ANOTHER_NODE:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            print_node_src(arena, string, node, do_recursion);
            print_node_dest(arena, string, node, do_recursion);
            break;
        case NODE_LLVM_STORE_LITERAL:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            string_extend_strv_in_par(arena, string, get_node_name(node));
            print_node_dest(arena, string, node, do_recursion);
            break;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            print_node_dest(arena, string, node, do_recursion);
            break;
        case NODE_SYMBOL_TYPED:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            string_extend_strv_in_par(arena, string, get_node_name(node));
            break;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            string_extend_strv_in_par(arena, string, get_node_name(node));
            break;
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            string_extend_strv_in_par(arena, string, get_node_name(node));
            break;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            string_extend_strv_in_par(arena, string, get_node_name(node));
            print_node_src(arena, string, node, do_recursion);
            break;
        case NODE_PTR_BYVAL_SYM:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            print_node_src(arena, string, node, do_recursion);
            break;
        case NODE_LLVM_REGISTER_SYM:
            extend_lang_type_to_string(arena, string, get_lang_type(node), true);
            print_node_src(arena, string, node, do_recursion);
            break;
        default:
            log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(node->type)));
            unreachable("");
    }

    string_extend_cstr(arena, string, "    line:");
    string_extend_size_t(arena, string, node->pos.line);
}

Str_view node_print_internal(Arena* arena, const Node* node) {
    if (!node) {
        return str_view_from_cstr("<null>");
    }

    String buf = {0}; // todo: put these strings in an arena to free, etc

    if (!node) {
        string_extend_cstr(arena, &buf, "<null>");
        Str_view str_view = {.str = buf.buf, .count = buf.info.count};
        return str_view;
    }

    extend_node_text(arena, &buf, node, true);

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

