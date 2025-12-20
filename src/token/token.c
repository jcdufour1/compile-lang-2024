#include "token.h"
#include "assert.h"

Strv token_type_to_strv_msg(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return sv("nontype");
        case TOKEN_SYMBOL:
            // TODO: change "`symbol`" in msg errors to "symbol"
            return sv("symbol");
        case TOKEN_OPEN_PAR:
            return sv("(");
        case TOKEN_CLOSE_PAR:
            return sv(")");
        case TOKEN_OPEN_CURLY_BRACE:
            return sv("{");
        case TOKEN_CLOSE_CURLY_BRACE:
            return sv("}");
        case TOKEN_DOUBLE_QUOTE:
            return sv("\"");
        case TOKEN_COMMA:
            return sv(",");
        case TOKEN_SINGLE_PLUS:
            return sv("+");
        case TOKEN_BITWISE_NOT:
            return sv("~");
        case TOKEN_SINGLE_MINUS:
            return sv("-");
        case TOKEN_ASTERISK:
            return sv("*");
        case TOKEN_STRING_LITERAL:
            return sv("string");
        case TOKEN_INT_LITERAL:
            return sv("int literal");
        case TOKEN_FLOAT_LITERAL:
            return sv("float literal");
        case TOKEN_COLON:
            return sv(":");
        case TOKEN_SINGLE_EQUAL:
            return sv("=");
        case TOKEN_DOUBLE_EQUAL:
            return sv("==");
        case TOKEN_SINGLE_DOT:
            return sv(".");
        case TOKEN_DOUBLE_DOT:
            return sv("..");
        case TOKEN_TRIPLE_DOT:
            return sv("...");
        case TOKEN_LESS_THAN:
            return sv("<");
        case TOKEN_LESS_OR_EQUAL:
            return sv("<=");
        case TOKEN_GREATER_THAN:
            return sv(">");
        case TOKEN_SLASH:
            return sv("/");
        case TOKEN_COMMENT:
            return sv("comment");
        case TOKEN_LOGICAL_NOT_EQUAL:
            return sv("!=");
        case TOKEN_LOGICAL_NOT:
            return sv("!");
        case TOKEN_BITWISE_XOR:
            return sv("^");
        case TOKEN_UNSAFE_CAST:
            return sv("unsafe_cast");
        case TOKEN_VOID:
            return sv("void");
        case TOKEN_FN:
            return sv("fn");
        case TOKEN_FOR:
            return sv("for");
        case TOKEN_IF:
            return sv("if");
        case TOKEN_RETURN:
            return sv("return");
        case TOKEN_EXTERN:
            return sv("extern");
        case TOKEN_STRUCT:
            return sv("struct");
        case TOKEN_LET:
            return sv("let");
        case TOKEN_IN:
            return sv("in");
        case TOKEN_BREAK:
            return sv("break");
        case TOKEN_NEW_LINE:
            return sv("newline");
        case TOKEN_RAW_UNION:
            return sv("unsafe_union");
        case TOKEN_ELSE:
            return sv("else");
        case TOKEN_OPEN_SQ_BRACKET:
            return sv("[");
        case TOKEN_CLOSE_SQ_BRACKET:
            return sv("]");
        case TOKEN_CHAR_LITERAL:
            return sv("char");
        case TOKEN_CONTINUE:
            return sv("continue");
        case TOKEN_GREATER_OR_EQUAL:
            return sv(">=");
        case TOKEN_TYPE_DEF:
            return sv("type");
        case TOKEN_SWITCH:
            return sv("switch");
        case TOKEN_CASE:
            return sv("case");
        case TOKEN_DEFAULT:
            return sv("default");
        case TOKEN_ENUM:
            return sv("enum");
        case TOKEN_MODULO:
            return sv("%");
        case TOKEN_BITWISE_AND:
            return sv("&");
        case TOKEN_BITWISE_OR:
            return sv("|");
        case TOKEN_LOGICAL_AND:
            return sv("&&");
        case TOKEN_LOGICAL_OR:
            return sv("||");
        case TOKEN_SHIFT_LEFT:
            return sv("<<");
        case TOKEN_SHIFT_RIGHT:
            return sv(">>");
        case TOKEN_OPEN_GENERIC:
            return sv("(<");
        case TOKEN_CLOSE_GENERIC:
            return sv(">)");
        case TOKEN_IMPORT:
            return sv("import");
        case TOKEN_DEF:
            return sv("def");
        case TOKEN_EOF:
            return sv("end-of-file");
        case TOKEN_ASSIGN_BY_BIN:
            return sv("assign-by-binary");
        case TOKEN_MACRO:
            return sv("#");
        case TOKEN_DEFER:
            return sv("defer");
        case TOKEN_SIZEOF:
            return sv("sizeof");
        case TOKEN_YIELD:
            return sv("yield");
        case TOKEN_COUNTOF:
            return sv("countof");
        case TOKEN_DOUBLE_TICK:
            return sv("''");
        case TOKEN_GENERIC_TYPE:
            return sv("Type");
        case TOKEN_ONE_LINE_BLOCK_START:
            return sv("=>");
        case TOKEN_USING:
            return sv("using");
        case TOKEN_ORELSE:
            return sv("orelse");
        case TOKEN_QUESTION_MARK:
            return sv("?");
        case TOKEN_UNDERSCORE:
            return sv("_");
        case TOKEN_AT_SIGN:
            return sv("@");
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("%d\n", token_type);
}

Strv token_type_to_strv_log(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return sv("nontype");
        case TOKEN_BITWISE_NOT:
            return sv("~");
        case TOKEN_SYMBOL:
            return sv("sym");
        case TOKEN_OPEN_PAR:
            return sv("(");
        case TOKEN_CLOSE_PAR:
            return sv(")");
        case TOKEN_OPEN_CURLY_BRACE:
            return sv("{");
        case TOKEN_CLOSE_CURLY_BRACE:
            return sv("}");
        case TOKEN_DOUBLE_QUOTE:
            return sv("\"");
        case TOKEN_COMMA:
            return sv(",");
        case TOKEN_SINGLE_PLUS:
            return sv("+");
        case TOKEN_SINGLE_MINUS:
            return sv("-");
        case TOKEN_ASTERISK:
            return sv("*");
        case TOKEN_STRING_LITERAL:
            return sv("str");
        case TOKEN_INT_LITERAL:
            return sv("int");
        case TOKEN_FLOAT_LITERAL:
            return sv("float");
        case TOKEN_COLON:
            return sv(":");
        case TOKEN_SINGLE_EQUAL:
            return sv("=");
        case TOKEN_DOUBLE_EQUAL:
            return sv("==");
        case TOKEN_SINGLE_DOT:
            return sv(".");
        case TOKEN_DOUBLE_DOT:
            return sv("..");
        case TOKEN_TRIPLE_DOT:
            return sv("...");
        case TOKEN_LESS_THAN:
            return sv("<");
        case TOKEN_LESS_OR_EQUAL:
            return sv("<=");
        case TOKEN_GREATER_THAN:
            return sv(">");
        case TOKEN_SLASH:
            return sv("/");
        case TOKEN_COMMENT:
            return sv("comment");
        case TOKEN_LOGICAL_NOT_EQUAL:
            return sv("!=");
        case TOKEN_LOGICAL_NOT:
            return sv("!");
        case TOKEN_BITWISE_XOR:
            return sv("^");
        case TOKEN_UNSAFE_CAST:
            return sv("unsafe_cast");
        case TOKEN_VOID:
            return sv("void");
        case TOKEN_FN:
            return sv("fn");
        case TOKEN_FOR:
            return sv("for");
        case TOKEN_IF:
            return sv("if");
        case TOKEN_RETURN:
            return sv("return");
        case TOKEN_EXTERN:
            return sv("extern");
        case TOKEN_STRUCT:
            return sv("struct");
        case TOKEN_LET:
            return sv("let");
        case TOKEN_IN:
            return sv("in");
        case TOKEN_BREAK:
            return sv("break");
        case TOKEN_NEW_LINE:
            return sv("newline");
        case TOKEN_RAW_UNION:
            return sv("unsafe_union");
        case TOKEN_ELSE:
            return sv("else");
        case TOKEN_OPEN_SQ_BRACKET:
            return sv("[");
        case TOKEN_CLOSE_SQ_BRACKET:
            return sv("]");
        case TOKEN_CHAR_LITERAL:
            return sv("char");
        case TOKEN_CONTINUE:
            return sv("continue");
        case TOKEN_GREATER_OR_EQUAL:
            return sv(">=");
        case TOKEN_TYPE_DEF:
            return sv("type");
        case TOKEN_SWITCH:
            return sv("switch");
        case TOKEN_CASE:
            return sv("case");
        case TOKEN_DEFAULT:
            return sv("default");
        case TOKEN_ENUM:
            return sv("enum");
        case TOKEN_MODULO:
            return sv("%");
        case TOKEN_BITWISE_AND:
            return sv("&");
        case TOKEN_BITWISE_OR:
            return sv("|");
        case TOKEN_LOGICAL_AND:
            return sv("&&");
        case TOKEN_LOGICAL_OR:
            return sv("||");
        case TOKEN_SHIFT_LEFT:
            return sv("<<");
        case TOKEN_SHIFT_RIGHT:
            return sv(">>");
        case TOKEN_OPEN_GENERIC:
            return sv("(<");
        case TOKEN_CLOSE_GENERIC:
            return sv(">)");
        case TOKEN_IMPORT:
            return sv("import");
        case TOKEN_DEF:
            return sv("def");
        case TOKEN_EOF:
            return sv("eof");
        case TOKEN_ASSIGN_BY_BIN:
            return sv("assign-by-binary");
        case TOKEN_MACRO:
            return sv("#");
        case TOKEN_DEFER:
            return sv("defer");
        case TOKEN_SIZEOF:
            return sv("sizeof");
        case TOKEN_YIELD:
            return sv("yield");
        case TOKEN_COUNTOF:
            return sv("countof");
        case TOKEN_DOUBLE_TICK:
            return sv("''");
        case TOKEN_GENERIC_TYPE:
            return sv("Type");
        case TOKEN_ONE_LINE_BLOCK_START:
            return sv("=>");
        case TOKEN_USING:
            return sv("using");
        case TOKEN_ORELSE:
            return sv("orelse");
        case TOKEN_QUESTION_MARK:
            return sv("?");
        case TOKEN_UNDERSCORE:
            return sv("_");
        case TOKEN_AT_SIGN:
            return sv("@");
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("%d\n", token_type);
}

Strv token_print_internal(Arena* arena, TOKEN_MODE mode, Token token) {
    String buf = {0};
    darr_reset(&buf);

    switch (mode) {
        case TOKEN_MODE_LOG:
            string_extend_strv(arena, &buf, token_type_to_strv_log(token.type));
            break;
        case TOKEN_MODE_MSG:
            string_extend_strv(arena, &buf, token_type_to_strv_msg(token.type));
            break;
        default:
            unreachable("");
    }

    // add token text
    static_assert(TOKEN_COUNT == 79, "exhausive handling of token types");
    switch (token.type) {
        case TOKEN_SYMBOL:
            darr_append(arena, &buf, '(');
            string_extend_strv(arena, &buf, token.text);
            darr_append(arena, &buf, ')');
            break;
        case TOKEN_OPEN_PAR: fallthrough;
        case TOKEN_NONTYPE: fallthrough;
        case TOKEN_CLOSE_PAR: fallthrough;
        case TOKEN_OPEN_CURLY_BRACE: fallthrough;
        case TOKEN_CLOSE_CURLY_BRACE: fallthrough;
        case TOKEN_DOUBLE_QUOTE: fallthrough;
        case TOKEN_COMMA: fallthrough;
        case TOKEN_SINGLE_MINUS: fallthrough;
        case TOKEN_COLON: fallthrough;
        case TOKEN_ASTERISK: fallthrough;
        case TOKEN_SLASH: fallthrough;
        case TOKEN_SINGLE_EQUAL: fallthrough;
        case TOKEN_DOUBLE_EQUAL: fallthrough;
        case TOKEN_SINGLE_DOT: fallthrough;
        case TOKEN_DOUBLE_DOT: fallthrough;
        case TOKEN_TRIPLE_DOT: fallthrough;
        case TOKEN_SINGLE_PLUS: fallthrough;
        case TOKEN_LESS_THAN: fallthrough;
        case TOKEN_GREATER_THAN: fallthrough;
        case TOKEN_LOGICAL_NOT_EQUAL: fallthrough;
        case TOKEN_LOGICAL_NOT: fallthrough;
        case TOKEN_ENUM: fallthrough;
        case TOKEN_BITWISE_XOR: fallthrough;
        case TOKEN_VOID: fallthrough;
        case TOKEN_UNSAFE_CAST: fallthrough;
        case TOKEN_FN: fallthrough;
        case TOKEN_FOR: fallthrough;
        case TOKEN_IF: fallthrough;
        case TOKEN_SWITCH: fallthrough;
        case TOKEN_CASE: fallthrough;
        case TOKEN_DEFAULT: fallthrough;
        case TOKEN_ELSE: fallthrough;
        case TOKEN_RETURN: fallthrough;
        case TOKEN_EXTERN: fallthrough;
        case TOKEN_STRUCT: fallthrough;
        case TOKEN_RAW_UNION: fallthrough;
        case TOKEN_LET: fallthrough;
        case TOKEN_IN: fallthrough;
        case TOKEN_NEW_LINE: fallthrough;
        case TOKEN_BREAK: fallthrough;
        case TOKEN_CONTINUE: fallthrough;
        case TOKEN_OPEN_SQ_BRACKET: fallthrough;
        case TOKEN_CLOSE_SQ_BRACKET: fallthrough;
        case TOKEN_LESS_OR_EQUAL: fallthrough;
        case TOKEN_GREATER_OR_EQUAL: fallthrough;
        case TOKEN_MODULO: fallthrough;
        case TOKEN_BITWISE_AND: fallthrough;
        case TOKEN_BITWISE_NOT: fallthrough;
        case TOKEN_BITWISE_OR: fallthrough;
        case TOKEN_LOGICAL_AND: fallthrough;
        case TOKEN_LOGICAL_OR: fallthrough;
        case TOKEN_SHIFT_LEFT: fallthrough;
        case TOKEN_SHIFT_RIGHT: fallthrough;
        case TOKEN_OPEN_GENERIC: fallthrough;
        case TOKEN_CLOSE_GENERIC: fallthrough;
        case TOKEN_IMPORT: fallthrough;
        case TOKEN_DEF: fallthrough;
        case TOKEN_EOF: fallthrough;
        case TOKEN_TYPE_DEF: fallthrough;
        case TOKEN_DEFER: fallthrough;
        case TOKEN_SIZEOF: fallthrough;
        case TOKEN_YIELD: fallthrough;
        case TOKEN_COUNTOF: fallthrough;
        case TOKEN_DOUBLE_TICK: fallthrough;
        case TOKEN_ASSIGN_BY_BIN:  fallthrough;
        case TOKEN_GENERIC_TYPE: fallthrough;
        case TOKEN_ONE_LINE_BLOCK_START: fallthrough;
        case TOKEN_ORELSE: fallthrough;
        case TOKEN_QUESTION_MARK: fallthrough;
        case TOKEN_UNDERSCORE: fallthrough;
        case TOKEN_AT_SIGN: fallthrough;
        case TOKEN_USING:
            break;
        case TOKEN_MACRO: 
            string_extend_strv(arena, &buf, token.text);
            break;
        case TOKEN_COMMENT: 
            fallthrough;
        case TOKEN_STRING_LITERAL: 
            fallthrough;
        case TOKEN_CHAR_LITERAL: 
            fallthrough;
        case TOKEN_INT_LITERAL:
            darr_append(arena, &buf, '(');
            string_extend_strv(arena, &buf, token.text);
            darr_append(arena, &buf, ')');
            break;
        case TOKEN_FLOAT_LITERAL:
            darr_append(arena, &buf, '(');
            string_extend_strv(arena, &buf, token.text);
            darr_append(arena, &buf, ')');
            break;
        case TOKEN_COUNT:
            unreachable("");
        default:
            unreachable("");
    }

    if (mode == TOKEN_MODE_LOG) {
        string_extend_line(arena, &buf, token.pos.line);
    }

    Strv strv = {.str = buf.buf, .count = buf.info.count};
    return strv;
}

