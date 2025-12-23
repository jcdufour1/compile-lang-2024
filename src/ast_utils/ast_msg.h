#ifndef AST_MSG_H
#define AST_MSG_H

#include <uast.h>

// functions return bool if they do not report error to the user
// functions return PARSE_STATUS if they report error to the user
// functions return PARSE_EXPR_STATUS if they may report error to the user or do nothing without reporting an error
typedef enum {
    PARSE_OK, // no need for callers to sync tokens
    PARSE_ERROR, // tokens need to be synced by callers
} PARSE_STATUS;

typedef enum {
    PARSE_EXPR_OK, // no need for callers to sync tokens, and no message reported to the user
    PARSE_EXPR_NONE, // no expr parsed; no message reported to the user, and no need for callers to sync tokens
    PARSE_EXPR_ERROR, // tokens need to be synced by callers
} PARSE_EXPR_STATUS;

typedef enum {
    PARSE_EXPR_EX_OK_NORMAL, // no need for callers to sync tokens, and no message reported to the user
    PARSE_EXPR_EX_OK_IF_LET, // no need for callers to sync tokens, and no message reported to the user
    PARSE_EXPR_EX_NONE, // no expr parsed; no message reported to the user, and no need for callers to sync tokens
    PARSE_EXPR_EX_ERROR, // tokens need to be synced by callers
} PARSE_EXPR_EX_STATUS;

static inline PARSE_EXPR_STATUS parse_expr_ex_status_to_parse_expr_status(PARSE_EXPR_EX_STATUS status) {
    switch (status) {
        case PARSE_EXPR_EX_OK_NORMAL:
            return PARSE_EXPR_OK;
        case PARSE_EXPR_EX_ERROR:
            return PARSE_EXPR_ERROR;
        case PARSE_EXPR_EX_NONE:
            return PARSE_EXPR_NONE;
        case PARSE_EXPR_EX_OK_IF_LET:
            unreachable("");
        default:
            unreachable("");
    }
    unreachable("");
}

static inline PARSE_EXPR_EX_STATUS parse_expr_status_to_parse_expr_ex_status(PARSE_EXPR_STATUS status) {
    switch (status) {
        case PARSE_EXPR_OK:
            return PARSE_EXPR_EX_OK_NORMAL;
        case PARSE_EXPR_ERROR:
            return PARSE_EXPR_EX_ERROR;
        case PARSE_EXPR_NONE:
            return PARSE_EXPR_EX_NONE;
        default:
            unreachable("");
    }
    unreachable("");
}

PARSE_STATUS msg_redefinition_of_symbol_internal(const char* file, int line, const Uast_def* new_sym_def);

#define msg_redefinition_of_symbol(new_sym_def) \
    msg_redefinition_of_symbol_internal(__FILE__, __LINE__, new_sym_def)

void msg_undefined_symbol_internal(const char* file, int line, Name sym_name, Pos sym_pos);

#define msg_undefined_symbol(sym_name, sym_pos) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, sym_name, sym_pos)

void msg_not_lvalue_internal(const char* file, int line, Pos pos);

#define msg_not_lvalue(pos) \
    msg_not_lvalue_internal(__FILE__, __LINE__, pos)

void msg_got_type_but_expected_expr_internal(const char* file, int line, Pos pos);

#define msg_got_type_but_expected_expr(pos) \
    msg_got_type_but_expected_expr_internal(__FILE__, __LINE__, pos)

void msg_got_expr_but_expected_type_internal(const char* file, int line, Pos pos);

#define msg_got_expr_but_expected_type(pos) \
    msg_got_expr_but_expected_type_internal(__FILE__, __LINE__, pos)

void msg_struct_literal_assigned_to_non_struct_gen_param_internal(
    const char* file,
    int line,
    Pos pos_struct_lit,
    Pos pos_gen_param
);

#define msg_struct_literal_assigned_to_non_struct_gen_param(pos_struct_lit, pos_gen_param) \
    msg_struct_literal_assigned_to_non_struct_gen_param_internal(__FILE__, __LINE__, pos_struct_lit, pos_gen_param)

#define msg_todo(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, sv(feature), pos);

#define msg_soft_todo(feature, pos) \
    msg_soft_todo_internal(__FILE__, __LINE__, sv(feature), pos);

#define msg_todo_strv(feature, pos) \
    msg_todo_internal(__FILE__, __LINE__, feature, pos);

void msg_todo_internal(const char* file, int line, Strv feature, Pos pos);

void msg_soft_todo_internal(const char* file, int line, Strv feature, Pos pos);

#endif // AST_MSG_H
