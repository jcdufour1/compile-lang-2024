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

PARSE_STATUS msg_redefinition_of_symbol_internal(const char* file, int line, const Uast_def* new_sym_def);

#define msg_redefinition_of_symbol(new_sym_def) \
    msg_redefinition_of_symbol_internal(__FILE__, __LINE__, new_sym_def)

void msg_undefined_symbol_internal(const char* file, int line, Name sym_name, Pos sym_pos);

#define msg_undefined_symbol(sym_name, sym_pos) \
    msg_undefined_symbol_internal(__FILE__, __LINE__, sym_name, sym_pos)

void msg_not_lvalue_internal(const char* file, int line, Pos pos);

#define msg_not_lvalue(pos) \
    msg_not_lvalue_internal(__FILE__, __LINE__, pos)

#endif // AST_MSG_H
