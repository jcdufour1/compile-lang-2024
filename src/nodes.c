#include "nodes.h"
#include "str_view.h"
#include "util.h"
#include "node_ptr_vec.h"
#include "node_utils.h"
#include "do_passes.h"
#include "symbol_table.h"

static void extend_node_text(Arena* arena, String* string, const Node* node, bool do_recursion);

static const char* NODE_E_LITERAL_DESCRIPTION = "literal";
static const char* NODE_E_FUNCTION_CALL_DESCRIPTION = "fn_call";
static const char* NODE_FUNCTION_DEF_DESCRIPTION = "fn_def";
static const char* NODE_FUNCTION_PARAMETERS_DESCRIPTION = "fn_params";
static const char* NODE_LANG_TYPE_DESCRIPTION = "lang_type";
static const char* NODE_BINARY_DESCRIPTION = "binary";
static const char* NODE_UNARY_DESCRIPTION = "unary";
static const char* NODE_BLOCK_DESCRIPTION = "block";
static const char* NODE_E_SYMBOL_UNTYPED_DESCRIPTION = "sym_untyped";
static const char* NODE_E_SYMBOL_TYPED_DESCRIPTION= "sym_typed";
static const char* NODE_RETURN_DESCRIPTION = "return";
static const char* NODE_VARIABLE_DEFINITION_DESCRIPTION = "var_def";
static const char* NODE_FUNCTION_DECL_DESCRIPTION = "fun_declaration";
static const char* NODE_ASSIGNMENT_DESCRIPTION = "assignment";
static const char* NODE_FOR_RANGE_DESCRIPTION = "for_range";
static const char* NODE_FOR_WITH_COND_DESCRIPTION = "for_with_cond";
static const char* NODE_FOR_LOWER_BOUND_DESCRIPTION = "lower_bound";
static const char* NODE_FOR_UPPER_BOUND_DESCRIPTION = "upper_bound";
static const char* NODE_BREAK_DESCRIPTION = "break";
static const char* NODE_GOTO_DESCRIPTION = "goto";
static const char* NODE_LABEL_DESCRIPTION = "label";
static const char* NODE_COND_GOTO_DESCRIPTION = "cond_goto";
static const char* NODE_ALLOCA_DESCRIPTION = "alloca";
static const char* NODE_IF_STATEMENT_DESCRIPTION = "if_statement";
static const char* NODE_CONDITION_DESCRIPTION = "condition";
static const char* NODE_STRUCT_DEFINITION_DESCRIPTION = "struct_def";
static const char* NODE_E_STRUCT_LITERAL_DESCRIPTION = "struct_literal";
static const char* NODE_E_MEMBER_SYM_TYPED_DESCRIPTION = "member_sym_typed";
static const char* NODE_E_MEMBER_SYM_UNTYPED_DESCRIPTION = "member_sym_untyped";
static const char* NODE_MEMBER_SYM_PIECE_TYPED_DESCRIPTION = "member_sym_piece_typed";
static const char* NODE_MEMBER_SYM_PIECE_UNTYPED_DESCRIPTION = "member_sym_piece_untyped";
static const char* NODE_LLVM_STORE_LITERAL_DESCRIPTION = "llvm_store_literal";
static const char* NODE_LLVM_STORE_STRUCT_LITERAL_DESCRIPTION = "llvm_store_struct_literal";
static const char* NODE_LOAD_STRUCT_ELEMENT_PTR_DESCRIPTION = "load_element_ptr";
static const char* NODE_LOAD_ANOTHER_NODE_DESCRIPTION = "load_another_node";
static const char* NODE_STORE_ANOTHER_NODE_DESCRIPTION = "store_another_node";
static const char* NODE_PTR_BYVAL_SYM_DESCRIPTION = "byval_sym";
static const char* NODE_E_LLVM_PLACEHOLDER_DESCRIPTION = "llvm_register_sym";
static const char* NODE_RAW_UNION_DEF_DESCRIPTION = "raw_union";
static const char* NODE_IF_ELSE_CHAIN_DESCRIPTION = "if_else_chain";
static const char* NODE_ENUM_CHAIN_DESCRIPTION = "enum";

void extend_lang_type_to_string(Arena* arena, String* string, Lang_type lang_type, bool surround_in_lt_gt) {
    if (surround_in_lt_gt) {
        vec_append(arena, string, '<');
    }

    string_extend_strv(arena, string, lang_type.str);
    if (lang_type.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < lang_type.pointer_depth; idx++) {
        vec_append(arena, string, '*');
    }
    if (surround_in_lt_gt) {
        vec_append(arena, string, '>');
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

void log_node_in_tree_internal(Env* env) {
    Node* node = vec_top(&env->ancesters);
    String padding = {0};
    vec_reset(&padding);

    for (int idx = 0; idx < 2*env->recursion_depth; idx++) {
        vec_append(&print_arena, &padding, ' ');
    }

    log_file_new(log_file_level, log_file, log_line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(node));
    log_symbol_table_if_block(env, log_file, log_line);
}

void nodes_log_tree_internal(LOG_LEVEL log_level, const Node* root, const char* file, int line) {
    if (!root_of_tree) {
        log_file_new(log_level, file, line, "<empty tree>\n");
        return;
    }

    log_file_level = log_level;
    log_file = file;
    log_line = line;

    Env env = {0};
    vec_append(&a_main, &env.ancesters, (Node*)root);
    walk_tree(&env, log_node_in_tree_internal);
}

static Str_view node_type_get_strv_expr(const Node_expr* node) {
    switch (node->type) {
        case NODE_E_LITERAL:
            return str_view_from_cstr(NODE_E_LITERAL_DESCRIPTION);
        case NODE_E_OPERATOR: {
            const Node_e_operator* operator = node_unwrap_e_operator_const(node);
            if (operator->type == NODE_OP_UNARY) {
                return str_view_from_cstr(NODE_UNARY_DESCRIPTION);
            } else if (operator->type == NODE_OP_BINARY) {
                return str_view_from_cstr(NODE_BINARY_DESCRIPTION);
            } else {
                unreachable("");
            }
        }
        break;
        case NODE_E_MEMBER_SYM_UNTYPED:
            return str_view_from_cstr(NODE_E_MEMBER_SYM_UNTYPED_DESCRIPTION);
        case NODE_E_MEMBER_SYM_TYPED:
            return str_view_from_cstr(NODE_E_MEMBER_SYM_TYPED_DESCRIPTION);
        case NODE_E_STRUCT_LITERAL:
            return str_view_from_cstr(NODE_E_STRUCT_LITERAL_DESCRIPTION);
        case NODE_E_FUNCTION_CALL:
            return str_view_from_cstr(NODE_E_FUNCTION_CALL_DESCRIPTION);
        case NODE_E_SYMBOL_UNTYPED:
            return str_view_from_cstr(NODE_E_SYMBOL_UNTYPED_DESCRIPTION);
        case NODE_E_SYMBOL_TYPED:
            return str_view_from_cstr(NODE_E_SYMBOL_TYPED_DESCRIPTION);
        case NODE_E_LLVM_PLACEHOLDER:
            return str_view_from_cstr(NODE_E_LLVM_PLACEHOLDER_DESCRIPTION);
    }
    unreachable("");
}

static Str_view node_type_get_strv(const Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return node_type_get_strv_expr(node_unwrap_expr_const(node));
        case NODE_FUNCTION_DEF:
            return str_view_from_cstr(NODE_FUNCTION_DEF_DESCRIPTION);
        case NODE_FUNCTION_PARAMS:
            return str_view_from_cstr(NODE_FUNCTION_PARAMETERS_DESCRIPTION);
        case NODE_LANG_TYPE:
            return str_view_from_cstr(NODE_LANG_TYPE_DESCRIPTION);
        case NODE_BLOCK:
            return str_view_from_cstr(NODE_BLOCK_DESCRIPTION);
        case NODE_RETURN:
            return str_view_from_cstr(NODE_RETURN_DESCRIPTION);
        case NODE_VARIABLE_DEF:
            return str_view_from_cstr(NODE_VARIABLE_DEFINITION_DESCRIPTION);
        case NODE_FUNCTION_DECL:
            return str_view_from_cstr(NODE_FUNCTION_DECL_DESCRIPTION);
        case NODE_ASSIGNMENT:
            return str_view_from_cstr(NODE_ASSIGNMENT_DESCRIPTION);
        case NODE_FOR_RANGE:
            return str_view_from_cstr(NODE_FOR_RANGE_DESCRIPTION);
        case NODE_FOR_WITH_COND:
            return str_view_from_cstr(NODE_FOR_WITH_COND_DESCRIPTION);
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
        case NODE_IF:
            return str_view_from_cstr(NODE_IF_STATEMENT_DESCRIPTION);
        case NODE_CONDITION:
            return str_view_from_cstr(NODE_CONDITION_DESCRIPTION);
        case NODE_STRUCT_DEF:
            return str_view_from_cstr(NODE_STRUCT_DEFINITION_DESCRIPTION);
        case NODE_MEMBER_SYM_PIECE_TYPED:
            return str_view_from_cstr(NODE_MEMBER_SYM_PIECE_TYPED_DESCRIPTION);
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            return str_view_from_cstr(NODE_MEMBER_SYM_PIECE_UNTYPED_DESCRIPTION);
        case NODE_LOAD_ELEMENT_PTR:
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
        case NODE_RAW_UNION_DEF:
            return str_view_from_cstr(NODE_RAW_UNION_DEF_DESCRIPTION);
        case NODE_IF_ELSE_CHAIN:
            return str_view_from_cstr(NODE_IF_ELSE_CHAIN_DESCRIPTION);
        case NODE_ENUM_DEF:
            return str_view_from_cstr(NODE_ENUM_CHAIN_DESCRIPTION);
    }
    unreachable( "node->type: %d\n", node->type);
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
    (void) node;
    (void) do_recursion;
    string_extend_cstr(arena, string, "[");
    // TODO: restore this?
    //if (get_node_src_const(node) && do_recursion) {
        //string_extend_cstr(arena, string, "node_src:");
        //extend_node_text(arena, string, get_node_src_const(node), false);
    //}
}

static void extend_expr_text(Arena* arena, String* string, const Node_expr* expr, bool do_recursion) {
    (void) do_recursion;
    switch (expr->type) {
        case NODE_E_LITERAL: {
            const Node_e_literal* literal = node_unwrap_e_literal_const(expr);
            extend_lang_type_to_string(arena, string, literal->lang_type, true);
            string_extend_strv(arena, string, literal->name);
            if (literal->type == NODE_LIT_STRING) {
                string_extend_strv_in_par(arena, string, node_unwrap_lit_string_const(literal)->data);
            } else if (literal->type == NODE_LIT_NUMBER) {
                string_extend_int64_t(arena, string, node_unwrap_lit_number_const(literal)->data);
            } else if (literal->type == NODE_LIT_VOID) {
                string_extend_strv_in_par(arena, string, str_view_from_cstr("void"));
            } else {
                unreachable("");
            }
            break;
        }
        case NODE_E_SYMBOL_UNTYPED:
            string_extend_strv_in_par(arena, string, get_expr_name(expr));
            break;
        case NODE_E_MEMBER_SYM_UNTYPED:
            break;
        case NODE_E_OPERATOR: {
            const Node_e_operator* operator = node_unwrap_e_operator_const(expr);
            if (operator->type == NODE_OP_UNARY) {
                extend_lang_type_to_string(arena, string, node_unwrap_op_unary_const(operator)->lang_type, true);
                string_extend_strv(arena, string, token_type_to_str_view(node_unwrap_op_unary_const(operator)->token_type));
            } else if (operator->type == NODE_OP_BINARY) {
                extend_lang_type_to_string(arena, string, node_unwrap_op_binary_const(operator)->lang_type, true);
                string_extend_strv(arena, string, token_type_to_str_view(node_unwrap_op_binary_const(operator)->token_type));
            } else {
                unreachable("");
            }
            break;
        }
        case NODE_E_STRUCT_LITERAL:
            extend_lang_type_to_string(arena, string, node_unwrap_e_struct_literal_const(expr)->lang_type, true);
            string_extend_strv(arena, string, get_expr_name(expr));
            break;
        case NODE_E_SYMBOL_TYPED:
            extend_lang_type_to_string(arena, string, get_lang_type_expr(expr), true);
            string_extend_strv_in_par(arena, string, get_expr_name(expr));
            break;
        case NODE_E_MEMBER_SYM_TYPED:
            extend_lang_type_to_string(arena, string, get_lang_type_expr(expr), true);
            string_extend_strv_in_par(arena, string, get_expr_name(expr));
            break;
        case NODE_E_FUNCTION_CALL:
            string_extend_strv_in_par(arena, string, get_expr_name(expr));
            string_extend_size_t(arena, string, get_llvm_id_expr(expr));
            break;
        case NODE_E_LLVM_PLACEHOLDER:
            extend_lang_type_to_string(arena, string, get_lang_type_expr(expr), true);
            print_node_src(arena, string, node_wrap_expr_const(expr), do_recursion);
            break;
        default:
            unreachable("expr->type: %d\n", expr->type);
    }
}

static void extend_node_text(Arena* arena, String* string, const Node* node, bool do_recursion) {
    assert(node);
    string_extend_strv(arena, string, node_type_get_strv(node));

    if (node->type == NODE_EXPR) {
        extend_expr_text(arena, string, node_unwrap_expr_const(node), do_recursion);
    } else {
        switch (node->type) {
            case NODE_ALLOCA:
                // fallthrough
            case NODE_GOTO:
                // fallthrough
            case NODE_LABEL:
                string_extend_strv_in_par(arena, string, get_node_name(node));
                break;
            case NODE_COND_GOTO:
                print_node_src(arena, string, node, do_recursion);
                break;
            case NODE_MEMBER_SYM_PIECE_UNTYPED:
                break;
            case NODE_FUNCTION_DEF:
                string_extend_strv_in_par(arena, string, get_node_name(node));
                break;
            case NODE_STRUCT_DEF:
                break;
            case NODE_RAW_UNION_DEF:
                break;
            case NODE_ENUM_DEF:
                break;
            case NODE_LANG_TYPE:
                extend_lang_type_to_string(arena, string, node_unwrap_lang_type_const(node)->lang_type, true);
                break;
            case NODE_VARIABLE_DEF:
                extend_lang_type_to_string(arena, string, node_unwrap_variable_def_const(node)->lang_type, true);
                string_extend_strv(arena, string, get_node_name(node));
                break;
            case NODE_FUNCTION_DECL:
                string_extend_strv_in_par(arena, string, get_node_name(node));
                break;
            case NODE_FUNCTION_PARAMS:
                // fallthrough
            case NODE_BLOCK:
                // fallthrough
            case NODE_RETURN:
                // fallthrough
            case NODE_ASSIGNMENT:
                // fallthrough
            case NODE_FOR_RANGE:
                // fallthrough
            case NODE_FOR_WITH_COND:
                // fallthrough
            case NODE_FOR_UPPER_BOUND:
                // fallthrough
            case NODE_FOR_LOWER_BOUND:
                // fallthrough
            case NODE_IF:
                // fallthrough
            case NODE_CONDITION:
                // fallthrough
            case NODE_BREAK:
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
                print_node_dest(arena, string, node, do_recursion);
                break;
            case NODE_LLVM_STORE_STRUCT_LITERAL:
                extend_lang_type_to_string(arena, string, get_lang_type(node), true);
                print_node_dest(arena, string, node, do_recursion);
                break;
            case NODE_MEMBER_SYM_PIECE_TYPED:
                extend_lang_type_to_string(arena, string, get_lang_type(node), true);
                string_extend_strv_in_par(arena, string, get_node_name(node));
                break;
            case NODE_LOAD_ELEMENT_PTR:
                extend_lang_type_to_string(arena, string, get_lang_type(node), true);
                string_extend_strv_in_par(arena, string, get_node_name(node));
                print_node_src(arena, string, node, do_recursion);
                break;
            case NODE_PTR_BYVAL_SYM:
                extend_lang_type_to_string(arena, string, get_lang_type(node), true);
                print_node_src(arena, string, node, do_recursion);
                break;
            case NODE_IF_ELSE_CHAIN:
                break;
            default:
                log(LOG_FETAL, "type: "STR_VIEW_FMT"\n", str_view_print(node_type_get_strv(node)));
                unreachable("");
        }
    }

    assert(node->pos.line < 1e6);
    string_extend_cstr(arena, string, "    line:");
    string_extend_size_t(arena, string, node->pos.line);
}

Str_view node_print_internal(Arena* arena, const Node* node) {
    if (!node) {
        return str_view_from_cstr("<null>");
    }

    String buf = {0};

    if (!node) {
        string_extend_cstr(arena, &buf, "<null>");
        Str_view str_view = {.str = buf.buf, .count = buf.info.count};
        return str_view;
    }

    extend_node_text(arena, &buf, node, true);

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

