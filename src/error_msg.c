#include "error_msg.h"
#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"

void msg_redefinition_of_symbol(const Node* new_sym_def) {
    assert(new_sym_def->line_num > 0);
    assert(new_sym_def->file_path && strlen(new_sym_def->file_path) > 0);

    msg(
        LOG_ERROR, new_sym_def->file_path, new_sym_def->line_num,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(new_sym_def->name)
    );

    Node* original_def;
    try(sym_tbl_lookup(&original_def, new_sym_def->name));
    assert(original_def->line_num > 0);
    assert(original_def->file_path && strlen(original_def->file_path) > 0);
    msg(
        LOG_NOTE, original_def->file_path, original_def->line_num,
        STR_VIEW_FMT " originally defined here\n", str_view_print(original_def->name)
    );
}

void msg_undefined_symbol(const Node* sym_call) {
    assert(sym_call->line_num > 0);
    assert(sym_call->file_path && strlen(sym_call->file_path) > 0);

    msg(
        LOG_ERROR, sym_call->file_path, sym_call->line_num,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(sym_call->name)
    );
}

void msg_undefined_function(const Node* fun_call) {
    msg(
        LOG_ERROR, fun_call->file_path, fun_call->line_num,
        "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
    );
}

void msg_invalid_struct_member(const Node* node) {
    switch (node->type) {
        case NODE_STRUCT_MEMBER_SYM:
            todo();
        case NODE_SYMBOL: {
            Node* struct_memb_sym = node->parent;
            assert(struct_memb_sym->type == NODE_STRUCT_MEMBER_SYM);
            msg(
                LOG_ERROR, node->file_path, node->line_num, 
                "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
                str_view_print(node->name), str_view_print(struct_memb_sym->name)
            );
            Node* struct_memb_sym_def;
            try(sym_tbl_lookup(&struct_memb_sym_def, struct_memb_sym->name));
            msg(
                LOG_NOTE, struct_memb_sym_def->file_path, struct_memb_sym_def->line_num, 
                "`"STR_VIEW_FMT"` defined here\n", str_view_print(struct_memb_sym_def->name)
            );

            todo();
        }
        default:
            unreachable(NODE_FMT"\n", node_print(node));
    }
}


void msg_invalid_struct_member_assignment_in_literal(
    const Node* struct_var_def,
    const Node* memb_sym_def,
    const Node* memb_sym
) {
    msg(
        LOG_ERROR, memb_sym->file_path, memb_sym->line_num, 
        "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
        str_view_print(memb_sym_def->name), str_view_print(memb_sym->name)
    );
    msg(
        LOG_NOTE, struct_var_def->file_path, struct_var_def->line_num, 
        "variable `"STR_VIEW_FMT"` is defined as struct `"STR_VIEW_FMT"`\n",
        str_view_print(struct_var_def->name), str_view_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, memb_sym_def->file_path, memb_sym_def->line_num, 
        "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
        str_view_print(memb_sym_def->name), str_view_print(struct_var_def->lang_type)
    );
}

void meg_struct_assigned_to_invalid_literal(const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL && is_struct_symbol(lhs));
    assert(rhs->type == NODE_LITERAL);

    Node* struct_var_def;
    try(sym_tbl_lookup(&struct_var_def, lhs->name));
    msg(
        LOG_ERROR, rhs->file_path, rhs->line_num, 
        "invalid literal type is assigned to `"STR_VIEW_FMT"`, "
        "but `"STR_VIEW_FMT"` is of type `"STR_VIEW_FMT"`\n",
        str_view_print(lhs->name), 
        str_view_print(lhs->name), 
        str_view_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, struct_var_def->file_path, struct_var_def->line_num, 
        "variable `"STR_VIEW_FMT"` is defined as struct `"STR_VIEW_FMT"`\n",
        str_view_print(struct_var_def->name), str_view_print(struct_var_def->lang_type)
    );
}

void msg_invalid_assignment_to_literal(const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL && rhs->type == NODE_LITERAL);

    Node* var_def;
    try(sym_tbl_lookup(&var_def, lhs->name));
    msg(
        LOG_ERROR, rhs->file_path, rhs->line_num, 
        "invalid literal type is assigned to `"STR_VIEW_FMT"`\n",
        str_view_print(lhs->name)
    );
}
