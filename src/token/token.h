#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <strv.h>
#include <local_string.h>

#define token_print(mode, token) strv_print(token_print_internal(&a_temp, mode, token))

typedef enum {
    // nontype
    TOKEN_NONTYPE,

    // binary operators
    TOKEN_SINGLE_PLUS,
    TOKEN_SINGLE_MINUS,
    TOKEN_ASTERISK,
    TOKEN_MODULO,
    TOKEN_SLASH,
    TOKEN_LESS_THAN,
    TOKEN_LESS_OR_EQUAL,
    TOKEN_GREATER_THAN,
    TOKEN_GREATER_OR_EQUAL,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_LOGICAL_NOT_EQUAL,
    TOKEN_BITWISE_AND,
    TOKEN_BITWISE_OR,
    TOKEN_BITWISE_XOR,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,

    // unary operators
    TOKEN_LOGICAL_NOT,
    TOKEN_BITWISE_NOT,
    TOKEN_UNSAFE_CAST,

    // right unary operators
    TOKEN_ORELSE,
    TOKEN_QUESTION_MARK,

    // literals
    TOKEN_STRING_LITERAL,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_VOID,
    TOKEN_CHAR_LITERAL,

    // opening
    TOKEN_OPEN_PAR,
    TOKEN_OPEN_CURLY_BRACE,
    TOKEN_OPEN_SQ_BRACKET,
    TOKEN_OPEN_GENERIC,

    // closing
    TOKEN_CLOSE_PAR,
    TOKEN_CLOSE_CURLY_BRACE,
    TOKEN_CLOSE_SQ_BRACKET,
    TOKEN_CLOSE_GENERIC,

    // miscellaneous
    TOKEN_ENUM,
    TOKEN_SYMBOL,
    TOKEN_DOUBLE_QUOTE,
    TOKEN_NEW_LINE,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SINGLE_EQUAL,
    TOKEN_SINGLE_DOT,
    TOKEN_DOUBLE_DOT,
    TOKEN_TRIPLE_DOT,
    TOKEN_EOF,
    TOKEN_ASSIGN_BY_BIN,
    TOKEN_MACRO,
    TOKEN_DEFER,
    TOKEN_DOUBLE_TICK,
    TOKEN_ONE_LINE_BLOCK_START,
    TOKEN_UNDERSCORE,
    TOKEN_AT_SIGN,

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
    TOKEN_YIELD,
    TOKEN_CONTINUE,
    TOKEN_RAW_UNION,
    TOKEN_GENERIC_TYPE,
    TOKEN_IMPORT,
    TOKEN_DEF,
    TOKEN_SIZEOF,
    TOKEN_COUNTOF,
    TOKEN_USING,

    // comment
    TOKEN_COMMENT,

    // count of tokens (for static asserts)
    TOKEN_COUNT
} TOKEN_TYPE;

typedef struct {
    Strv text;
    TOKEN_TYPE type;

    Pos pos;
} Token;

typedef enum {
    TOKEN_MODE_LOG,
    TOKEN_MODE_MSG,
} TOKEN_MODE;

static inline Token token_new(Strv text, TOKEN_TYPE token_type) {
    Token token = {.text = text, .type = token_type};
    return token;
}

Strv token_print_internal(Arena* arena, TOKEN_MODE mode, Token token);

static inline bool token_is_literal(Token token) {
    switch (token.type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_AT_SIGN:
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
        case TOKEN_LOGICAL_NOT_EQUAL:
            return false;
        case TOKEN_LOGICAL_NOT:
            return false;
        case TOKEN_BITWISE_NOT:
            return false;
        case TOKEN_STRING_LITERAL:
            return true;
        case TOKEN_INT_LITERAL:
            return true;
        case TOKEN_FLOAT_LITERAL:
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
        case TOKEN_YIELD:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
        case TOKEN_CLOSE_SQ_BRACKET:
            return false;
        case TOKEN_CHAR_LITERAL:
            return true;
        case TOKEN_CONTINUE:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_MODULO:
            return false;
        case TOKEN_BITWISE_AND:
            return false;
        case TOKEN_BITWISE_OR:
            return false;
        case TOKEN_BITWISE_XOR:
            return false;
        case TOKEN_LOGICAL_AND:
            return false;
        case TOKEN_LOGICAL_OR:
            return false;
        case TOKEN_SHIFT_LEFT:
            return false;
        case TOKEN_SHIFT_RIGHT:
            return false;
        case TOKEN_OPEN_GENERIC:
            return false;
        case TOKEN_CLOSE_GENERIC:
            return false;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
        case TOKEN_EOF:
            return false;
        case TOKEN_ASSIGN_BY_BIN:
            return false;
        case TOKEN_MACRO:
            // TODO
            return false;
        case TOKEN_DEFER:
            return false;
        case TOKEN_SIZEOF:
            return false;
        case TOKEN_COUNTOF:
            return false;
        case TOKEN_DOUBLE_TICK:
            return false;
        case TOKEN_GENERIC_TYPE:
            return false;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
            return false;
        case TOKEN_ORELSE:
            return false;
        case TOKEN_QUESTION_MARK:
            return false;
        case TOKEN_UNDERSCORE:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

static inline bool token_is_operator(Token token, bool can_be_tuple) {
    switch (token.type) {
        case TOKEN_SINGLE_MINUS:
            return true;
        case TOKEN_AT_SIGN:
            return false;
        case TOKEN_SINGLE_PLUS:
            return true;
        case TOKEN_ASTERISK:
            return true;
        case TOKEN_SLASH:
            return true;
        case TOKEN_LESS_THAN:
            return true;
        case TOKEN_GREATER_THAN:
            return true;
        case TOKEN_DOUBLE_EQUAL:
            return true;
        case TOKEN_LOGICAL_NOT_EQUAL:
            return true;
        case TOKEN_LOGICAL_NOT:
            return true;
        case TOKEN_UNSAFE_CAST:
            return true;
        case TOKEN_INT_LITERAL:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_CLOSE_PAR:
            fallthrough;
        case TOKEN_DOUBLE_QUOTE:
            fallthrough;
        case TOKEN_CLOSE_CURLY_BRACE:
            return false;
        case TOKEN_SYMBOL:
            return false;
        case TOKEN_COLON:
            return false;
        case TOKEN_SINGLE_EQUAL:
            return false;
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_VOID:
            return false;
        case TOKEN_COMMENT:
            return false;
        case TOKEN_FOR:
            return false;
        case TOKEN_IF:
            return false;
        case TOKEN_RETURN:
            return false;
        case TOKEN_FN:
            return false;
        case TOKEN_STRUCT:
            return false;
        case TOKEN_EXTERN:
            return false;
        case TOKEN_LET:
            return false;
        case TOKEN_IN:
            return false;
        case TOKEN_BREAK:
            return false;
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_DOUBLE_DOT:
            return false;
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ELSE:
            return false;
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
        case TOKEN_OPEN_CURLY_BRACE:
            return false;
        case TOKEN_OPEN_PAR:
            return true;
        case TOKEN_OPEN_SQ_BRACKET:
            return true;
        case TOKEN_ENUM:
            return false;
        case TOKEN_MODULO:
            return true;
        case TOKEN_BITWISE_AND:
            return true;
        case TOKEN_BITWISE_OR:
            return true;
        case TOKEN_BITWISE_XOR:
            return true;
        case TOKEN_LOGICAL_AND:
            return true;
        case TOKEN_LOGICAL_OR:
            return true;
        case TOKEN_SHIFT_LEFT:
            return true;
        case TOKEN_SHIFT_RIGHT:
            return true;
        case TOKEN_OPEN_GENERIC:
            return true;
        case TOKEN_CLOSE_GENERIC:
            return true;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
        case TOKEN_EOF:
            return false;
        case TOKEN_ASSIGN_BY_BIN:
            return true;
        case TOKEN_FLOAT_LITERAL:
            return false;
        case TOKEN_MACRO:
            return false;
        case TOKEN_DEFER:
            return false;
        case TOKEN_SIZEOF:
            return true;
        case TOKEN_YIELD:
            return false;
        case TOKEN_COUNTOF:
            return true;
        case TOKEN_DOUBLE_TICK:
            return false;
        case TOKEN_GENERIC_TYPE:
            return false;
        case TOKEN_BITWISE_NOT:
            return true;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
            return false;
        case TOKEN_ORELSE:
            return true;
        case TOKEN_QUESTION_MARK:
            return true;
        case TOKEN_UNDERSCORE:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable(FMT"\n", token_print(TOKEN_MODE_LOG, token));
}

static const uint32_t TOKEN_MAX_PRECEDENCE = 20;

Strv token_type_to_strv_msg(TOKEN_TYPE token_type);

Strv token_type_to_strv_log(TOKEN_TYPE token_type);

static inline Strv token_type_to_strv(TOKEN_MODE mode, TOKEN_TYPE token_type) {
    switch (mode) {
        case TOKEN_MODE_LOG:
            return token_type_to_strv_log(token_type);
        case TOKEN_MODE_MSG:
            return token_type_to_strv_msg(token_type);
    }
    unreachable("");
}

#define token_type_print(mode, token_type) strv_print(token_type_to_strv(mode, token_type))

static inline bool token_is_equal(const Token a, const Token b) {
    if (a.type != b.type) {
        return false;
    }
    return strv_is_equal(a.text, b.text);
}

// TODO: rename this function
static inline bool token_is_equal_2(const Token a, const char* cstr, TOKEN_TYPE token_type) {
    Token b = {.text = sv(cstr), .type = token_type};
    return token_is_equal(a, b);
}

static inline bool token_is_binary(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_NONTYPE:
            return false;
        case TOKEN_AT_SIGN:
            return false;
        case TOKEN_SINGLE_PLUS:
            return true;
        case TOKEN_SINGLE_MINUS:
            return true;
        case TOKEN_ASTERISK:
            return true;
        case TOKEN_SLASH:
            return true;
        case TOKEN_LESS_THAN:
            return true;
        case TOKEN_GREATER_THAN:
            return true;
        case TOKEN_DOUBLE_EQUAL:
            return true;
        case TOKEN_LOGICAL_NOT_EQUAL:
            return true;
        case TOKEN_LOGICAL_NOT:
            return false;
        case TOKEN_STRING_LITERAL:
            return false;
        case TOKEN_INT_LITERAL:
            return false;
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
        case TOKEN_COMMA:
            return true;
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
        case TOKEN_BITWISE_XOR:
            return true;
        case TOKEN_BITWISE_NOT:
            return false;
        case TOKEN_VOID:
            return false;
        case TOKEN_UNSAFE_CAST:
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
        case TOKEN_NEW_LINE:
            return false;
        case TOKEN_RAW_UNION:
            return false;
        case TOKEN_ELSE:
            return false;
        case TOKEN_OPEN_SQ_BRACKET:
            return false;
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
        case TOKEN_SWITCH:
            return false;
        case TOKEN_CASE:
            return false;
        case TOKEN_DEFAULT:
            return false;
        case TOKEN_ENUM:
            return false;
        case TOKEN_MODULO:
            return true;
        case TOKEN_BITWISE_AND:
            return true;
        case TOKEN_BITWISE_OR:
            return true;
        case TOKEN_LOGICAL_AND:
            return true;
        case TOKEN_LOGICAL_OR:
            return true;
        case TOKEN_SHIFT_LEFT:
            return true;
        case TOKEN_SHIFT_RIGHT:
            return true;
        case TOKEN_OPEN_GENERIC:
            return true;
        case TOKEN_CLOSE_GENERIC:
            return true;
        case TOKEN_IMPORT:
            return false;
        case TOKEN_DEF:
            return false;
        case TOKEN_EOF:
            return false;
        case TOKEN_ASSIGN_BY_BIN:
            return false;
        case TOKEN_FLOAT_LITERAL:
            return false;
        case TOKEN_MACRO:
            return false;
        case TOKEN_DEFER:
            return false;
        case TOKEN_SIZEOF:
            return false;
        case TOKEN_YIELD:
            return false;
        case TOKEN_COUNTOF:
            return false;
        case TOKEN_DOUBLE_TICK:
            return false;
        case TOKEN_GENERIC_TYPE:
            return false;
        case TOKEN_ONE_LINE_BLOCK_START:
            return false;
        case TOKEN_USING:
            return false;
        case TOKEN_ORELSE:
            return false;
        case TOKEN_QUESTION_MARK:
            return false;
        case TOKEN_UNDERSCORE:
            return false;
        case TOKEN_COUNT:
            unreachable("");
    }
    unreachable("");
}

#endif // TOKEN_H

