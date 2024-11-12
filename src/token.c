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
            return str_view_from_cstr("*");
        case TOKEN_STRING_LITERAL:
            return str_view_from_cstr("str");
        case TOKEN_INT_LITERAL:
            return str_view_from_cstr("num");
        case TOKEN_COLON:
            return str_view_from_cstr(":");
        case TOKEN_SINGLE_EQUAL:
            return str_view_from_cstr("=");
        case TOKEN_DOUBLE_EQUAL:
            return str_view_from_cstr("==");
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
        case TOKEN_NOT_EQUAL:
            return str_view_from_cstr("!=");
        case TOKEN_NOT:
            return str_view_from_cstr("!");
        case TOKEN_XOR:
            return str_view_from_cstr("xor");
        case TOKEN_DEREF:
            return str_view_from_cstr("deref");
        case TOKEN_REFER:
            return str_view_from_cstr("refer");
        case TOKEN_UNSAFE_CAST:
            return str_view_from_cstr("unsafe_cast");
        case TOKEN_VOID:
            return str_view_from_cstr("void");
        case TOKEN_FN:
            return str_view_from_cstr("fn");
        case TOKEN_FOR:
            return str_view_from_cstr("for");
        case TOKEN_IF:
            return str_view_from_cstr("if");
        case TOKEN_RETURN:
            return str_view_from_cstr("return");
        case TOKEN_EXTERN:
            return str_view_from_cstr("extern");
        case TOKEN_STRUCT:
            return str_view_from_cstr("struct");
        case TOKEN_LET:
            return str_view_from_cstr("let");
        case TOKEN_IN:
            return str_view_from_cstr("in");
        case TOKEN_BREAK:
            return str_view_from_cstr("break");
        case TOKEN_NEW_LINE:
            return str_view_from_cstr("newline");
        case TOKEN_RAW_UNION:
            return str_view_from_cstr("unsafe_union");
    }
    unreachable("");
}

Str_view token_print_internal(Arena* arena, Token token, bool msg_format) {
    String buf = {0};
    vec_reset(&buf);

    string_extend_strv(arena, &buf, token_type_to_str_view(token.type));
    assert(strlen(buf.buf) == buf.info.count);

    // add token text
    switch (token.type) {
        case TOKEN_SYMBOL:
            vec_append(arena, &buf, '(');
            string_extend_strv(arena, &buf, token.text);
            vec_append(arena, &buf, ')');
            break;
        case TOKEN_OPEN_PAR: // fallthrough
        case TOKEN_NONTYPE: // fallthrough
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
        case TOKEN_DOUBLE_EQUAL: // fallthrough
        case TOKEN_SINGLE_DOT: // fallthrough
        case TOKEN_DOUBLE_DOT: // fallthrough
        case TOKEN_TRIPLE_DOT: // fallthrough
        case TOKEN_SINGLE_PLUS: // fallthrough
        case TOKEN_LESS_THAN: // fallthrough
        case TOKEN_GREATER_THAN: // fallthrough
        case TOKEN_NOT_EQUAL: // fallthrough
        case TOKEN_NOT: // fallthrough
        case TOKEN_DEREF: // fallthrough
        case TOKEN_XOR: // fallthrough
        case TOKEN_REFER: // fallthrough
        case TOKEN_VOID: // fallthrough
        case TOKEN_UNSAFE_CAST: // fallthrough
        case TOKEN_FN: // fallthrough
        case TOKEN_FOR: // fallthrough
        case TOKEN_IF: // fallthrough
        case TOKEN_RETURN: // fallthrough
        case TOKEN_EXTERN: // fallthrough
        case TOKEN_STRUCT: // fallthrough
        case TOKEN_RAW_UNION: // fallthrough
        case TOKEN_LET: // fallthrough
        case TOKEN_IN: // fallthrough
        case TOKEN_NEW_LINE: // fallthrough
        case TOKEN_BREAK:
            break;
        case TOKEN_COMMENT: 
            // fallthrough
        case TOKEN_STRING_LITERAL: 
            // fallthrough
        case TOKEN_INT_LITERAL:
            vec_append(arena, &buf, '(');
            string_extend_strv(arena, &buf, token.text);
            vec_append(arena, &buf, ')');
            break;
    }

    if (!msg_format) {
        string_add_int(arena, &buf, token.pos.line);
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

