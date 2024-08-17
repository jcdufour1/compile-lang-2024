#include "token.h"

static void add_line_num_to_string(String* string, int line_num) {
    const char* fmt_str =  " (line %d)";
    static char num_str[20];
    sprintf(num_str, fmt_str, line_num);
    string_extend_cstr(string, num_str);
}

Str_view token_print_internal(Token token) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_strv(&buf, token_type_to_str_view(token.type));

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

    add_line_num_to_string(&buf, token.line_num);

    Str_view str_view = {.str = buf.buf, .count = buf.count};
    return str_view;
}

