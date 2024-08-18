#include "token.h"
#include "assert.h"

Str_view token_print_internal(Token token) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_strv(&buf, token_type_to_str_view(token.type));
    assert(strlen(buf.buf) == buf.count);

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
        case TOKEN_PLUS_SIGN:
        case TOKEN_MINUS_SIGN:
        case TOKEN_MULTIPLY_SIGN:
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

    assert(strlen(buf.buf) == buf.count);
    string_add_int(&buf, token.line_num);

    assert(strlen(buf.buf) == buf.count);
    Str_view str_view = {.str = buf.buf, .count = buf.count};
    return str_view;
}

