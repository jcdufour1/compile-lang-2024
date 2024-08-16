#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include "str_view.h"
#include "newstring.h"

typedef enum {
    TOKEN_SYMBOL,
    TOKEN_OPEN_PAR,
    TOKEN_CLOSE_PAR,
    TOKEN_OPEN_CURLY_BRACE,
    TOKEN_CLOSE_CURLY_BRACE,
    TOKEN_DOUBLE_QUOTE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_STRING_LITERAL,
    TOKEN_NUM_LITERAL,
} TOKEN_TYPE;

typedef struct {
    Str_view text;
    TOKEN_TYPE type;
} Token;

static inline bool Token_is_literal(const Token token) {
    switch (token.type) {
        case TOKEN_NUM_LITERAL: // fallthrough
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_CLOSE_PAR: // fallthrough
        case TOKEN_OPEN_PAR: // fallthrough
        case TOKEN_COMMA: // fallthrough
        case TOKEN_DOUBLE_QUOTE: // fallthrough
        case TOKEN_OPEN_CURLY_BRACE: // fallthrough
        case TOKEN_CLOSE_CURLY_BRACE: // fallthrough
        case TOKEN_SYMBOL: // fallthrough
        case TOKEN_SEMICOLON:
            return false;
        default:
            unreachable();
    }
}

#define TOKEN_TYPE_FMT STR_VIEW_FMT

#define token_type_print(token_type) str_view_print(token_type_to_str_view(token_type))

static inline Str_view token_type_to_str_view(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_SYMBOL:
            return str_view_from_cstr("sym");
        case TOKEN_OPEN_PAR:
            return str_view_from_cstr("(");
        case TOKEN_CLOSE_PAR:
            return str_view_from_cstr(")");
        case TOKEN_OPEN_CURLY_BRACE:
            return str_view_from_cstr("{");
        case TOKEN_CLOSE_CURLY_BRACE:
            return str_view_from_cstr("}");
        case TOKEN_DOUBLE_QUOTE:
            return str_view_from_cstr("\"");
        case TOKEN_SEMICOLON:
            return str_view_from_cstr(";");
        case TOKEN_COMMA:
            return str_view_from_cstr(",");
        case TOKEN_STRING_LITERAL:
            return str_view_from_cstr("str");
        case TOKEN_NUM_LITERAL:
            return str_view_from_cstr("num");
        default:
            unreachable();
    }
}

static inline Str_view token_print_internal(Token token, bool type_only) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_strv(&buf, token_type_to_str_view(token.type));
    if (type_only) {
        Str_view str_view = {.str = buf.buf, .count = buf.count};
        return str_view;
    }

    // add token text
    switch (token.type) {
        case TOKEN_SYMBOL:
            string_append(&buf, '(');
            string_append_strv(&buf, token.text);
            string_append(&buf, ')');
            break;
        case TOKEN_OPEN_PAR: // fallthrough
        case TOKEN_CLOSE_PAR: // fallthrough
        case TOKEN_OPEN_CURLY_BRACE: // fallthrough
        case TOKEN_CLOSE_CURLY_BRACE: // fallthrough
        case TOKEN_DOUBLE_QUOTE: // fallthrough
        case TOKEN_SEMICOLON: // fallthrough
        case TOKEN_COMMA:
            break;
        case TOKEN_STRING_LITERAL: 
            string_append(&buf, '(');
            string_append_strv(&buf, token.text);
            string_append(&buf, ')');
            break;
        case TOKEN_NUM_LITERAL:
            string_append(&buf, '(');
            string_append_strv(&buf, token.text);
            string_append(&buf, ')');
            break;
        default:
            unreachable();
    }

    Str_view str_view = {.str = buf.buf, .count = buf.count};
    return str_view;
}

#define token_print(token) str_view_print(token_print_internal((token), false))

#define TOKEN_FMT STR_VIEW_FMT

#endif // TOKEN_H

