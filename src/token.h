#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "str_view.h"
#include "newstring.h"

#define token_print(token) str_view_print(token_print_internal(&print_arena, token, false))

#define TOKEN_FMT STR_VIEW_FMT

typedef enum {
    // nontype
    TOKEN_NONTYPE,

    // binary operators
    TOKEN_SINGLE_PLUS,
    TOKEN_SINGLE_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_LESS_THAN,
    TOKEN_LESS_OR_EQUAL,
    TOKEN_GREATER_THAN,
    TOKEN_GREATER_OR_EQUAL,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_XOR,

    // unary operators
    TOKEN_NOT,
    TOKEN_DEREF,
    TOKEN_REFER,
    TOKEN_UNSAFE_CAST,

    // literals
    TOKEN_STRING_LITERAL,
    TOKEN_INT_LITERAL,
    TOKEN_VOID,
    TOKEN_NEW_LINE,
    TOKEN_CHAR_LITERAL,

    // miscellaneous
    TOKEN_SYMBOL,
    TOKEN_OPEN_PAR,
    TOKEN_CLOSE_PAR,
    TOKEN_OPEN_CURLY_BRACE,
    TOKEN_CLOSE_CURLY_BRACE,
    TOKEN_DOUBLE_QUOTE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SINGLE_EQUAL,
    TOKEN_SINGLE_DOT,
    TOKEN_DOUBLE_DOT,
    TOKEN_TRIPLE_DOT,
    TOKEN_OPEN_SQ_BRACKET,
    TOKEN_CLOSE_SQ_BRACKET,

    // keywords
    TOKEN_FN,
    TOKEN_FOR,
    TOKEN_IF,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_EXTERN,
    TOKEN_STRUCT,
    TOKEN_LET,
    TOKEN_IN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RAW_UNION,
    TOKEN_ENUM,
    TOKEN_TYPE_DEF,

    // comment
    TOKEN_COMMENT,
} TOKEN_TYPE;

typedef struct {
    Str_view text;
    TOKEN_TYPE type;

    Pos pos;
} Token;

Str_view token_print_internal(Arena* arena, Token token, bool msg_format);

static inline bool token_is_literal(Token token) {
    switch (token.type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return false;
        case TOKEN_GREATER_OR_EQUAL:
            return false;
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        case TOKEN_DOUBLE_EQUAL:
            return false;
        case TOKEN_NOT_EQUAL:
            return false;
        case TOKEN_XOR:
            return false;
        case TOKEN_NOT:
            return false;
        case TOKEN_DEREF:
            return false;
        case TOKEN_REFER:
            return false;
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_INT_LITERAL:
            return true;
        case TOKEN_VOID:
            return true;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_OPEN_PAR:
            return false;
        case TOKEN_CLOSE_PAR:
            return false;
        case TOKEN_OPEN_CURLY_BRACE:
            return false;
        case TOKEN_CLOSE_CURLY_BRACE:
            return false;
        case TOKEN_DOUBLE_QUOTE:
            return false;
        case TOKEN_SEMICOLON:
            return false;
        case TOKEN_COMMA:
            return false;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_SINGLE_DOT:
            return false;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_UNSAFE_CAST:
            return false;
        case TOKEN_FN:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_RETURN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
        case TOKEN_CLOSE_SQ_BRACKET:
            return false;
        case TOKEN_CHAR_LITERAL:
            return true;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
    }
    unreachable("");
}

static inline bool token_is_operator(Token token, bool can_be_tuple) {
    switch (token.type) {
        case TOKEN_SINGLE_MINUS:
            // fallthrough
        case TOKEN_SINGLE_PLUS:
            // fallthrough
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_SLASH:
            // fallthrough
        case TOKEN_LESS_THAN:
            // fallthrough
        case TOKEN_GREATER_THAN:
            // fallthrough
        case TOKEN_DOUBLE_EQUAL:
            // fallthrough
        case TOKEN_NOT_EQUAL:
            // fallthrough
        case TOKEN_NOT:
            // fallthrough
        case TOKEN_DEREF:
            // fallthrough
        case TOKEN_XOR:
            // fallthrough
        case TOKEN_REFER:
            // fallthrough
        case TOKEN_UNSAFE_CAST:
            return true;
        case TOKEN_INT_LITERAL:
            // fallthrough
        case TOKEN_STRING_LITERAL:
            // fallthrough
        case TOKEN_CLOSE_PAR:
            // fallthrough
        case TOKEN_OPEN_PAR:
            // fallthrough
        case TOKEN_DOUBLE_QUOTE:
            // fallthrough
        case TOKEN_OPEN_CURLY_BRACE:
            // fallthrough
        case TOKEN_CLOSE_CURLY_BRACE:
            // fallthrough
        case TOKEN_SYMBOL:
            // fallthrough
        case TOKEN_SEMICOLON:
            // fallthrough
        case TOKEN_COLON:
            // fallthrough
        case TOKEN_SINGLE_EQUAL:
            // fallthrough
        case TOKEN_NONTYPE:
            // fallthrough
        case TOKEN_VOID:
            // fallthrough
        case TOKEN_COMMENT:
            // fallthrough
        case TOKEN_FOR:
            // fallthrough
        case TOKEN_IF:
            // fallthrough
        case TOKEN_RETURN:
            // fallthrough
        case TOKEN_FN:
            // fallthrough
        case TOKEN_STRUCT:
            // fallthrough
        case TOKEN_EXTERN:
            // fallthrough
        case TOKEN_LET:
            // fallthrough
        case TOKEN_IN:
            // fallthrough
        case TOKEN_BREAK:
            // fallthrough
        case TOKEN_NEW_LINE:
            // fallthrough
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return true;
        case TOKEN_CLOSE_SQ_BRACKET:
            return false;
        case TOKEN_CHAR_LITERAL:
            return false;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return true;
        case TOKEN_GREATER_OR_EQUAL:
            return true;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SINGLE_DOT:
            return true;
        case TOKEN_COMMA:
            return can_be_tuple;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
    }
    unreachable(TOKEN_FMT"\n", token_print(token));
}

static const uint32_t TOKEN_MAX_PRECEDENCE = 20;

#define TOKEN_TYPE_FMT STR_VIEW_FMT

#define token_type_print(token_type) str_view_print(token_type_to_str_view(token_type))

Str_view token_type_to_str_view(TOKEN_TYPE token_type);

static inline bool token_is_closing(Token curr_token) {
    switch (curr_token.type) {
        case TOKEN_CLOSE_PAR:
            return true;
        case TOKEN_CLOSE_CURLY_BRACE:
            return true;
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        case TOKEN_DOUBLE_EQUAL:
            return false;
        case TOKEN_NOT_EQUAL:
            return false;
        case TOKEN_XOR:
            return false;
        case TOKEN_NOT:
            return false;
        case TOKEN_DEREF:
            return false;
        case TOKEN_REFER:
            return false;
        case TOKEN_UNSAFE_CAST:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_INT_LITERAL:
            return false;
        case TOKEN_VOID:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_OPEN_PAR:
            return false;
        case TOKEN_OPEN_CURLY_BRACE:
            return false;
        case TOKEN_DOUBLE_QUOTE:
            return false;
        case TOKEN_SEMICOLON:
            return false;
        case TOKEN_COMMA:
            return false;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_SINGLE_DOT:
            return false;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            return false;
        case TOKEN_FN:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_RETURN:
            return false;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
        case TOKEN_CLOSE_SQ_BRACKET:
            return true;
        case TOKEN_CHAR_LITERAL:
            return false;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return false;
        case TOKEN_GREATER_OR_EQUAL:
            return false;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
    }
    unreachable("");
}

static inline bool token_is_opening(Token curr_token) {
    switch (curr_token.type) {
        case TOKEN_OPEN_PAR:
            return true;
        case TOKEN_OPEN_CURLY_BRACE:
            return true;
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        case TOKEN_DOUBLE_EQUAL:
            return false;
        case TOKEN_NOT_EQUAL:
            return false;
        case TOKEN_XOR:
            return false;
        case TOKEN_NOT:
            return false;
        case TOKEN_DEREF:
            return false;
        case TOKEN_REFER:
            return false;
        case TOKEN_UNSAFE_CAST:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_INT_LITERAL:
            return false;
        case TOKEN_VOID:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_CLOSE_PAR:
            return false;
        case TOKEN_CLOSE_CURLY_BRACE:
            return false;
        case TOKEN_DOUBLE_QUOTE:
            return false;
        case TOKEN_SEMICOLON:
            return false;
        case TOKEN_COMMA:
            return false;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_SINGLE_DOT:
            return false;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            return false;
        case TOKEN_FN:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_RETURN:
            return false;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return true;
        case TOKEN_CLOSE_SQ_BRACKET:
            return false;
        case TOKEN_CHAR_LITERAL:
            return false;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_LESS_OR_EQUAL:
            return false;
        case TOKEN_GREATER_OR_EQUAL:
            return false;
        case TOKEN_TYPE_DEF:
            return false;
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
    }
    unreachable("");
}

static inline bool token_is_equal(const Token a, const Token b) {
    if (a.type != b.type) {
        return false;
    }
    return str_view_is_equal(a.text, b.text);
}

// TODO: rename this function
static inline bool token_is_equal_2(const Token a, const char* cstr, TOKEN_TYPE token_type) {
    Token b = {.text = str_view_from_cstr(cstr), .type = token_type};
    return token_is_equal(a, b);
}

#endif // TOKEN_H

