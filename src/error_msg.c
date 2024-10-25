#include "error_msg.h"
#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include <stdarg.h>

void msg_redefinition_of_symbol(const Env* env, const Node* new_sym_def) {
    msg(
        LOG_ERROR, new_sym_def->pos,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(get_node_name(new_sym_def))
    );

    Node* original_def;
    try(symbol_lookup(&original_def, env, get_node_name(new_sym_def)));
    msg(
        LOG_NOTE, original_def->pos,
        STR_VIEW_FMT " originally defined here\n", str_view_print(get_node_name(original_def))
    );
}

void msg_parser_expected_internal(const Env* env, Token got, int count_expected, ...) {
    va_list args;
    va_start(args, count_expected);

    String msg = {0};
    string_extend_cstr(&print_arena, &msg, "got token `");
    string_extend_strv(&print_arena, &msg, token_print_internal(&print_arena, got, true));
    string_extend_cstr(&print_arena, &msg, "`, but expected ");

    for (int idx = 0; idx < count_expected; idx++) {
        if (idx > 0) {
            if (idx == count_expected - 1) {
                string_extend_cstr(&print_arena, &msg, " or ");
            } else {
                string_extend_cstr(&print_arena, &msg, ", ");
            }
        }
        string_extend_cstr(&print_arena, &msg, "`");
        string_extend_strv(&print_arena, &msg, token_type_to_str_view(va_arg(args, TOKEN_TYPE)));
        string_extend_cstr(&print_arena, &msg, "`");
    }

    msg(LOG_ERROR, got.pos, STR_VIEW_FMT"\n", str_view_print(string_to_strv(msg)));

    va_end(args);
}

void msg_undefined_symbol(const Node* sym_call) {
    msg(
        LOG_ERROR, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_node_name(sym_call))
    );
}

void msg_undefined_function(const Node_function_call* fun_call) {
    msg(
        LOG_ERROR, node_wrap(fun_call)->pos,
        "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
    );
}

void msg_invalid_struct_member(const Env* env, const Node* parent, const Node* node) {
    switch (node->type) {
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            todo();
        case NODE_SYMBOL_TYPED: {
            const Node* struct_memb_sym = parent;
            assert(struct_memb_sym->type == NODE_STRUCT_MEMBER_SYM_TYPED);
            msg(
                LOG_ERROR, node->pos,
                "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
                str_view_print(get_node_name(node)), str_view_print(get_node_name(struct_memb_sym))
            );
            Node* struct_memb_sym_def_;
            try(symbol_lookup(&struct_memb_sym_def_, env, get_node_name(struct_memb_sym)));
            const Node_variable_def* struct_memb_sym_def = node_unwrap_variable_def_const(struct_memb_sym_def_);
            msg(
                LOG_NOTE, node_wrap(struct_memb_sym_def)->pos,
                "`"STR_VIEW_FMT"` defined here as type `"LANG_TYPE_FMT"`\n",
                str_view_print(struct_memb_sym_def->name),
                lang_type_print(struct_memb_sym_def->lang_type)
            );
            Node* struct_def;
            try(symbol_lookup(&struct_def, env, struct_memb_sym_def->lang_type.str));
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
    const Node_variable_def* struct_var_def,
    const Node_variable_def* memb_sym_def,
    const Node_symbol_untyped* memb_sym
) {
    msg(
        LOG_ERROR, node_wrap(memb_sym)->pos,
        "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
        str_view_print(memb_sym_def->name), str_view_print(memb_sym->name)
    );
    msg(
        LOG_NOTE, node_wrap(struct_var_def)->pos,
        "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
        str_view_print(struct_var_def->name), lang_type_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, node_wrap(memb_sym_def)->pos,
        "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
        str_view_print(memb_sym_def->name), lang_type_print(struct_var_def->lang_type)
    );
}

void meg_struct_assigned_to_invalid_literal(const Env* env, const Node* lhs, const Node* rhs) {
    assert(lhs->type == NODE_SYMBOL_TYPED && is_struct_symbol(env, lhs));
    assert(rhs->type == NODE_LITERAL);

    Node* struct_var_def_;
    try(symbol_lookup(&struct_var_def_, env, get_node_name(lhs)));
    const Node_variable_def* struct_var_def = node_unwrap_variable_def_const(struct_var_def_);
    msg(
        LOG_ERROR, rhs->pos,
        "invalid literal type is assigned to `"STR_VIEW_FMT"`, "
        "but `"STR_VIEW_FMT"` is of type `"STR_VIEW_FMT"`\n",
        str_view_print(get_node_name(lhs)), 
        str_view_print(get_node_name(lhs)), 
        lang_type_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, node_wrap(struct_var_def)->pos,
        "variable `"STR_VIEW_FMT"` is defined as struct `"STR_VIEW_FMT"`\n",
        str_view_print(struct_var_def->name), lang_type_print(struct_var_def->lang_type)
    );
}

void msg_invalid_assignment_to_literal(const Env* env, const Node_symbol_typed* lhs, const Node_literal* rhs) {
    Node* var_def;
    try(symbol_lookup(&var_def, env, lhs->name));
    msg(
        LOG_ERROR, node_wrap(rhs)->pos,
        "invalid literal type is assigned to `"STR_VIEW_FMT"`\n",
        str_view_print(lhs->name)
    );
}

void msg_invalid_assignment_to_operation(const Env* env, const Node* lhs, const Node_operator* operation) {
    assert(lhs->type == NODE_SYMBOL_TYPED);

    Node* var_def_;
    try(symbol_lookup(&var_def_, env, get_node_name(lhs)));
    const Node_variable_def* var_def = node_unwrap_variable_def_const(var_def_);
    msg(
        LOG_ERROR, node_wrap(operation)->pos,
        "operation is of type `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
        lang_type_print(get_operator_lang_type(operation)), lang_type_print(var_def->lang_type)
    );
}

void msg_mismatched_return_types(
    const Node_return_statement* rtn_statement,
    const Node_function_definition* fun_def
) {
    msg(
        LOG_ERROR, node_wrap(rtn_statement)->pos,
        "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
        lang_type_print(get_lang_type(node_wrap(rtn_statement))), 
        lang_type_print(fun_def->declaration->return_types->child->lang_type)
    );
    msg(
        LOG_NOTE, node_wrap(fun_def->declaration->return_types)->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(fun_def->declaration->return_types->child->lang_type)
    );
}
