#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "str_view.h"
#include "newstring.h"

#define token_print(token) str_view_print(token_print_internal(token))

#define TOKEN_FMT STR_VIEW_FMT

typedef enum {
    // operators
    TOKEN_SINGLE_PLUS,
    TOKEN_SINGLE_MINUS,
    TOKEN_ASTERISK,
    TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN,
    TOKEN_SLASH,

    // literals
    TOKEN_STRING_LITERAL,
    TOKEN_NUM_LITERAL,

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
} TOKEN_TYPE;

typedef struct {
    Str_view text;
    TOKEN_TYPE type;

    uint32_t line_num;
} Token;

Str_view token_print_internal(Token token);

static inline bool token_is_literal(Token token) {
    switch (token.type) {
        case TOKEN_NUM_LITERAL:
            // fallthrough
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_CLOSE_PAR:
            // fallthrough
        case TOKEN_OPEN_PAR:
            // fallthrough
        case TOKEN_COMMA:
            // fallthrough
        case TOKEN_SINGLE_MINUS:
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
            return false;
        default:
            unreachable(TOKEN_FMT"\n", token_print(token));
    }
}

static inline bool token_is_operator(Token token) {
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
            return true;
        case TOKEN_NUM_LITERAL:
            // fallthrough
        case TOKEN_STRING_LITERAL:
            // fallthrough
        case TOKEN_CLOSE_PAR:
            // fallthrough
        case TOKEN_OPEN_PAR:
            // fallthrough
        case TOKEN_COMMA:
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
            return false;
        default:
            unreachable(TOKEN_FMT"\n", token_print(token));
    }
}

// higher number returned from this function means that operator has higher precedence
static inline uint32_t token_get_precedence_operator(Token token) {
    switch (token.type) {
        case TOKEN_LESS_THAN:
            // fallthrough
        case TOKEN_GREATER_THAN:
            return 1;
        case TOKEN_SINGLE_PLUS:
            // fallthrough
        case TOKEN_SINGLE_MINUS:
            return 2;
        case TOKEN_ASTERISK:
            // fallthrough
        case TOKEN_SLASH:
            return 3;
        default:
            unreachable("");
    }
}

#define TOKEN_TYPE_FMT STR_VIEW_FMT

#define token_type_print(token_type) str_view_print(token_type_to_str_view(token_type))

Str_view token_type_to_str_view(TOKEN_TYPE token_type);

static inline bool token_is_closing(Token curr_token) {
    switch (curr_token.type) {
        case TOKEN_CLOSE_PAR:
            return true;
        case TOKEN_CLOSE_CURLY_BRACE:
            return true;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_NUM_LITERAL:
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
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        default:
            unreachable("");
    }
}

static inline bool token_is_opening(Token curr_token) {
    switch (curr_token.type) {
        case TOKEN_OPEN_PAR:
            return true;
        case TOKEN_OPEN_CURLY_BRACE:
            return true;
        case TOKEN_SINGLE_PLUS:
            return false;
        case TOKEN_SINGLE_MINUS:
            return false;
        case TOKEN_ASTERISK:
            return false;
        case TOKEN_SLASH:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_NUM_LITERAL:
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
        case TOKEN_LESS_THAN:
            return false;
        case TOKEN_GREATER_THAN:
            return false;
        default:
            unreachable("");
    }
}

static inline bool token_is_equal(const Token token, const char* cstr, TOKEN_TYPE token_type) {
    if (token.type != token_type) {
        return false;
    }

    return str_view_cstr_is_equal(token.text, cstr);
}

#endif // TOKEN_H

