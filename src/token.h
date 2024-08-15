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
} TOKEN_TYPE;

typedef struct {
    Str_view text;
    TOKEN_TYPE type;
} Token;

static inline void token_init(Token* token) {
    memset(token, 0, sizeof(*token));
}

static inline Str_view token_print_internal(Token token, bool type_only) {
    static String text = {0};

    switch (token.type) {
        case TOKEN_SYMBOL:
            string_cpy_cstr_inplace(&text, "sym");
            if (!type_only) {
                string_append(&text, '(');
                string_append_strv(&text, token.text);
                string_append(&text, ')');
            }
            break;
        case TOKEN_OPEN_PAR:
            string_cpy_cstr_inplace(&text, "(");
            break;
        case TOKEN_CLOSE_PAR:
            string_cpy_cstr_inplace(&text, ")");
            break;
        case TOKEN_OPEN_CURLY_BRACE:
            string_cpy_cstr_inplace(&text, "{");
            break;
        case TOKEN_CLOSE_CURLY_BRACE:
            string_cpy_cstr_inplace(&text, "}");
            break;
        case TOKEN_DOUBLE_QUOTE:
            string_cpy_cstr_inplace(&text, "\"");
            break;
        case TOKEN_SEMICOLON:
            string_cpy_cstr_inplace(&text, ";");
            break;
        case TOKEN_STRING_LITERAL:
            string_cpy_cstr_inplace(&text, "str");
            if (!type_only) {
                string_append(&text, '(');
                string_append_strv(&text, token.text);
                string_append(&text, ')');
            }
            break;
        default:
            unreachable();
    }

    Str_view str_view = {.str = text.buf, .count = text.count};
    return str_view;
}

static inline Str_view token_print_type(Token token) {
    return token_print_internal(token, true);
}

#define token_print(token) str_view_print(token_print_internal((token), false))

#define TOKEN_FMT STR_VIEW_FMT

#endif // TOKEN_H

