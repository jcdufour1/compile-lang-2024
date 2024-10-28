#include "error_msg.h"
#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include <stdarg.h>

static const char* get_log_level_str(int log_level) {
    switch (log_level) {
        case LOG_TRACE:
            return "";
        case LOG_DEBUG:
            return "debug";
        case LOG_VERBOSE:
            return "debug";
        case LOG_NOTE:
            return "note";
        case LOG_WARNING:
            return "warning";
        case LOG_ERROR:
            return "error";
        case LOG_FETAL:
            return "fetal error";
        default:
            abort();
    }
}

void log_common(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int idx = 0; idx < indent; idx++) {
        fprintf(stderr, " ");
    }
    if (log_level >= CURR_LOG_LEVEL) {
        fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    va_end(args);
}

void msg(LOG_LEVEL log_level, EXPECT_FAIL_TYPE expected_fail_type, Pos pos, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (log_level >= LOG_ERROR) {
        error_count++;
    } else if (log_level == LOG_WARNING) {
        warning_count++;
    }

    if (log_level >= CURR_LOG_LEVEL) {
        fprintf(stderr, "%s:%d:%d:%s:", pos.file_path, pos.line, pos.column, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    if (params.test_expected_fail && expected_fail_type == params.expected_fail_type) {
        assert(params.expected_fail_type != EXPECT_FAIL_TYPE_NONE);

        log(LOG_NOTE, "expected fail occured\n");
        exit(EXIT_CODE_EXPECTED_FAIL);
    }

    va_end(args);
}

void msg_redefinition_of_symbol(const Env* env, const Node* new_sym_def) {
    msg(
        LOG_ERROR, EXPECT_FAIL_REDEFINITION_SYMBOL, new_sym_def->pos,
        "redefinition of symbol "STR_VIEW_FMT"\n", str_view_print(get_node_name(new_sym_def))
    );

    Node* original_def;
    try(symbol_lookup(&original_def, env, get_node_name(new_sym_def)));
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, original_def->pos,
        STR_VIEW_FMT " originally defined here\n", str_view_print(get_node_name(original_def))
    );
}

void msg_parser_expected_internal(Token got, int count_expected, ...) {
    va_list args;
    va_start(args, count_expected);

    String message = {0};
    string_extend_cstr(&print_arena, &message, "got token `");
    string_extend_strv(&print_arena, &message, token_print_internal(&print_arena, got, true));
    string_extend_cstr(&print_arena, &message, "`, but expected ");

    for (int idx = 0; idx < count_expected; idx++) {
        if (idx > 0) {
            if (idx == count_expected - 1) {
                string_extend_cstr(&print_arena, &message, " or ");
            } else {
                string_extend_cstr(&print_arena, &message, ", ");
            }
        }
        string_extend_cstr(&print_arena, &message, "`");
        string_extend_strv(&print_arena, &message, token_type_to_str_view(va_arg(args, TOKEN_TYPE)));
        string_extend_cstr(&print_arena, &message, "`");
    }

    msg(LOG_ERROR, EXPECT_FAIL_TYPE_NONE, got.pos, STR_VIEW_FMT"\n", str_view_print(string_to_strv(message)));

    va_end(args);
}

void msg_undefined_symbol(const Node* sym_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_SYMBOL, sym_call->pos,
        "symbol `"STR_VIEW_FMT"` is not defined\n", str_view_print(get_node_name(sym_call))
    );
}

void msg_undefined_function(const Node_function_call* fun_call) {
    msg(
        LOG_ERROR, EXPECT_FAIL_UNDEFINED_FUNCTION, node_wrap(fun_call)->pos,
        "function `"STR_VIEW_FMT"` is not defined\n", str_view_print(fun_call->name)
    );
}

void msg_invalid_struct_member(
    const Env* env,
    const Node_struct_member_sym_typed* struct_memb_sym,
    const Node_struct_member_sym_piece_untyped* struct_memb_sym_piece
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER, node_wrap(struct_memb_sym_piece)->pos,
        "`"STR_VIEW_FMT"` is not a member of `"STR_VIEW_FMT"`\n", 
        str_view_print(struct_memb_sym_piece->name), str_view_print(struct_memb_sym->name)
    );
    Node* struct_memb_sym_def_;
    try(symbol_lookup(&struct_memb_sym_def_, env, struct_memb_sym->name));
    const Node_variable_def* struct_memb_sym_def = node_unwrap_variable_def_const(struct_memb_sym_def_);
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap(struct_memb_sym_def)->pos,
        "`"STR_VIEW_FMT"` defined here as type `"LANG_TYPE_FMT"`\n",
        str_view_print(struct_memb_sym_def->name),
        lang_type_print(struct_memb_sym_def->lang_type)
    );
    Node* struct_def;
    try(symbol_lookup(&struct_def, env, struct_memb_sym_def->lang_type.str));
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, struct_def->pos,
        "struct `"LANG_TYPE_FMT"` defined here\n", 
        lang_type_print(struct_memb_sym_def->lang_type)
    );
}

void msg_invalid_struct_member_assignment_in_literal(
    const Node_variable_def* struct_var_def,
    const Node_variable_def* memb_sym_def,
    const Node_symbol_untyped* memb_sym
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_INVALID_STRUCT_MEMBER_IN_LITERAL, node_wrap(memb_sym)->pos,
        "expected `."STR_VIEW_FMT" =`, got `."STR_VIEW_FMT" =`\n", 
        str_view_print(memb_sym_def->name), str_view_print(memb_sym->name)
    );
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap(struct_var_def)->pos,
        "variable `"STR_VIEW_FMT"` is defined as struct `"LANG_TYPE_FMT"`\n",
        str_view_print(struct_var_def->name), lang_type_print(struct_var_def->lang_type)
    );
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap(memb_sym_def)->pos,
        "member symbol `"STR_VIEW_FMT"` of struct `"STR_VIEW_FMT"` defined here\n", 
        str_view_print(memb_sym_def->name), lang_type_print(struct_var_def->lang_type)
    );
}

void msg_mismatched_return_types(
    const Node_return_statement* rtn_statement,
    const Node_function_definition* fun_def
) {
    msg(
        LOG_ERROR, EXPECT_FAIL_MISMATCHED_RETURN_TYPE, node_wrap(rtn_statement)->pos,
        "returning `"LANG_TYPE_FMT"`, but type `"LANG_TYPE_FMT"` expected\n",
        lang_type_print(get_lang_type(node_wrap(rtn_statement))), 
        lang_type_print(fun_def->declaration->return_types->child->lang_type)
    );
    msg(
        LOG_NOTE, EXPECT_FAIL_TYPE_NONE, node_wrap(fun_def->declaration->return_types)->pos,
        "function return type `"LANG_TYPE_FMT"` defined here\n",
        lang_type_print(fun_def->declaration->return_types->child->lang_type)
    );
}

