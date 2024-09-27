#include "token.h"
#include "assert.h"

Str_view token_type_to_str_view(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return str_view_from_cstr("nontype");
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
        case TOKEN_SINGLE_PLUS:
            return str_view_from_cstr("+");
        case TOKEN_SINGLE_MINUS:
            return str_view_from_cstr("-");
        case TOKEN_ASTERISK:
            // TODO: * may not always be multiplication
            return str_view_from_cstr("*");
        case TOKEN_STRING_LITERAL:
            return str_view_from_cstr("str");
        case TOKEN_NUM_LITERAL:
            return str_view_from_cstr("num");
        case TOKEN_COLON:
            return str_view_from_cstr(":");
        case TOKEN_SINGLE_EQUAL:
            return str_view_from_cstr("=");
        case TOKEN_SINGLE_DOT:
            return str_view_from_cstr(".");
        case TOKEN_DOUBLE_DOT:
            return str_view_from_cstr("..");
        case TOKEN_TRIPLE_DOT:
            return str_view_from_cstr("...");
        case TOKEN_LESS_THAN:
            return str_view_from_cstr("<");
        case TOKEN_GREATER_THAN:
            return str_view_from_cstr(">");
        case TOKEN_SLASH:
            return str_view_from_cstr("/");
        case TOKEN_COMMENT:
            return str_view_from_cstr("comment");
        default:
            unreachable("%d\n", token_type);
    }
}

Str_view token_print_internal(Token token) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    string_extend_strv(&buf, token_type_to_str_view(token.type));
    assert(strlen(buf.buf) == buf.info.count);

    // add token text
    switch (token.type) {
        case TOKEN_SYMBOL:
            string_append(&buf, '(');
            string_extend_strv(&buf, token.text);
            string_append(&buf, ')');
            break;
        case TOKEN_OPEN_PAR: // fallthrough
        case TOKEN_CLOSE_PAR: // fallthrough
        case TOKEN_OPEN_CURLY_BRACE: // fallthrough
        case TOKEN_CLOSE_CURLY_BRACE: // fallthrough
        case TOKEN_DOUBLE_QUOTE: // fallthrough
        case TOKEN_SEMICOLON: // fallthrough
        case TOKEN_COMMA: // fallthrough
        case TOKEN_SINGLE_MINUS: // fallthrough
        case TOKEN_COLON: // fallthrough
        case TOKEN_ASTERISK: // fallthrough
        case TOKEN_SLASH: // fallthrough
        case TOKEN_SINGLE_EQUAL: // fallthrough
        case TOKEN_SINGLE_DOT: // fallthrough
        case TOKEN_DOUBLE_DOT: // fallthrough
        case TOKEN_TRIPLE_DOT: // fallthrough
        case TOKEN_SINGLE_PLUS: // fallthrough
        case TOKEN_LESS_THAN: // fallthrough
        case TOKEN_GREATER_THAN: // fallthrough
            break;
        case TOKEN_COMMENT: 
            // fallthrough
        case TOKEN_STRING_LITERAL: 
            // fallthrough
        case TOKEN_NUM_LITERAL:
            string_append(&buf, '(');
            string_extend_strv(&buf, token.text);
            string_append(&buf, ')');
            break;
        default:
            unreachable("");
    }

    assert(strlen(buf.buf) == buf.info.count);
    string_add_int(&buf, token.pos.line);

    assert(strlen(buf.buf) == buf.info.count);
    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

