#include "error_msg.h"
#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"

void msg_redefinition_of_symbol(const Node* new_sym_def) {
    msg(
        LOG_ERROR, new_sym_def->pos,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(new_sym_def->name)
    );

    Node* original_def;
    try(sym_tbl_lookup(&original_def, new_sym_def->name));
    msg(
        LOG_NOTE, original_def->pos,
        STR_VIEW_FMT " originally defined here\n", str_view_print(original_def->name)
    );
}

void msg_undefined_symbol(const Node* sym_call) {
    msg(
        LOG_ERROR, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(sym_call->name)
    );
}

void msg_undefined_function(const Node* fun_call) {
    msg(
        LOG_ERROR, fun_call->pos,
        "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
    );
}

void msg_invalid_struct_member(const Node* node) {
    switch (node->type) {
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            todo();
        case NODE_SYMBOL_TYPED: {
            Node* struct_memb_sym = node->parent;
            assert(struct_memb_sym->type == NODE_STRUCT_MEMBER_SYM_TYPED);
            msg(
                LOG_ERROR, node->pos,
                "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
                str_view_print(node->name), str_view_print(struct_memb_sym->name)
            );
            Node* struct_memb_sym_def;
            try(sym_tbl_lookup(&struct_memb_sym_def, struct_memb_sym->name));
            msg(
                LOG_NOTE, struct_memb_sym_def->pos,
                "`"STR_VIEW_FMT"` defined here as type `"LANG_TYPE_FMT"`\n",
                str_view_print(struct_memb_sym_def->name),
                lang_type_print(struct_memb_sym_def->lang_type)
            );
            Node* struct_def;
            try(sym_tbl_lookup(&struct_def, struct_memb_sym_def->lang_type.str));
            msg(
                LOG_NOTE, struct_def->pos,
                "struct `"LANG_TYPE_FMT"` defined here\n", 
                lang_type_print(struct_memb_sym_def->lang_type)
            );
            break;
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
        LOG_ERROR, memb_sym->pos,
        "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
        str_view_print(memb_sym_def->name), str_view_print(memb_sym->name)
    );
    msg(
        LOG_NOTE, struct_var_def->pos,
        "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
        str_view_print(struct_var_def->name), lang_type_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, memb_sym_def->pos,
        "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
        str_view_print(memb_sym_def->name), lang_type_print(struct_var_def->lang_type)
    );
}

void meg_struct_assigned_to_invalid_literal(const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL_TYPED && is_struct_symbol(lhs));
    assert(rhs->type == NODE_LITERAL);

    Node* struct_var_def;
    try(sym_tbl_lookup(&struct_var_def, lhs->name));
    msg(
        LOG_ERROR, rhs->pos,
        "invalid literal type is assigned to `"STR_VIEW_FMT"`, "
        "but `"STR_VIEW_FMT"` is of type `"STR_VIEW_FMT"`\n",
        str_view_print(lhs->name), 
        str_view_print(lhs->name), 
        lang_type_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, struct_var_def->pos,
        "variable `"STR_VIEW_FMT"` is defined as struct `"STR_VIEW_FMT"`\n",
        str_view_print(struct_var_def->name), lang_type_print(struct_var_def->lang_type)
    );
}

void msg_invalid_assignment_to_literal(const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL_TYPED && rhs->type == NODE_LITERAL);

    Node* var_def;
    try(sym_tbl_lookup(&var_def, lhs->name));
    msg(
        LOG_ERROR, rhs->pos,
        "invalid literal type is assigned to `"STR_VIEW_FMT"`\n",
        str_view_print(lhs->name)
    );
}

void msg_invalid_assignment_to_operation(const Node* lhs, const Node* operation) {
    assert(lhs->type == NODE_SYMBOL_TYPED);
    assert(operation->type == NODE_OPERATOR);

    Node* var_def;
    try(sym_tbl_lookup(&var_def, lhs->name));
    msg(
        LOG_ERROR, operation->pos,
        "operation is of type `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
        lang_type_print(operation->lang_type), lang_type_print(var_def->lang_type)
    );
}
