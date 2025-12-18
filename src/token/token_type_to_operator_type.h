#ifndef TOKEN_TYPE_TO_OPERATOR_TYPE
#define TOKEN_TYPE_TO_OPERATOR_TYPE

#include <token.h>
#include <operator_type.h>

static inline BINARY_TYPE token_type_to_binary_type(TOKEN_TYPE token_type) {
    static_assert(TOKEN_COUNT == 78, "exhausive handling of token types (only binary tokens are handled)");

    switch (token_type) {
        case TOKEN_DOUBLE_EQUAL:
            return BINARY_DOUBLE_EQUAL;
        case TOKEN_SINGLE_PLUS:
            return BINARY_ADD;
        case TOKEN_SINGLE_MINUS:
            return BINARY_SUB;
        case TOKEN_SLASH:
            return BINARY_DIVIDE;
        case TOKEN_ASTERISK:
            return BINARY_MULTIPLY;
        case TOKEN_MODULO:
            return BINARY_MODULO;
        case TOKEN_LESS_THAN:
            return BINARY_LESS_THAN;
        case TOKEN_GREATER_THAN:
            return BINARY_GREATER_THAN;
        case TOKEN_GREATER_OR_EQUAL:
            return BINARY_GREATER_OR_EQUAL;
        case TOKEN_LESS_OR_EQUAL:
            return BINARY_LESS_OR_EQUAL;
        case TOKEN_LOGICAL_NOT_EQUAL:
            return BINARY_NOT_EQUAL;
        case TOKEN_BITWISE_AND:
            return BINARY_BITWISE_AND;
        case TOKEN_BITWISE_OR:
            return BINARY_BITWISE_OR;
        case TOKEN_BITWISE_XOR:
            return BINARY_BITWISE_XOR;
        case TOKEN_LOGICAL_AND:
            return BINARY_LOGICAL_AND;
        case TOKEN_LOGICAL_OR:
            return BINARY_LOGICAL_OR;
        case TOKEN_SHIFT_LEFT:
            return BINARY_SHIFT_LEFT;
        case TOKEN_SHIFT_RIGHT:
            return BINARY_SHIFT_RIGHT;
        case TOKEN_SINGLE_EQUAL:
            return BINARY_SINGLE_EQUAL;
        case TOKEN_NONTYPE:
            unreachable("");
        case TOKEN_LOGICAL_NOT:
            unreachable("");
        case TOKEN_BITWISE_NOT:
            unreachable("");
        case TOKEN_UNSAFE_CAST:
            unreachable("");
        case TOKEN_ORELSE:
            unreachable("");
        case TOKEN_QUESTION_MARK:
            unreachable("");
        case TOKEN_OPEN_PAR:
            unreachable("");
        case TOKEN_OPEN_CURLY_BRACE:
            unreachable("");
        case TOKEN_OPEN_SQ_BRACKET:
            unreachable("");
        case TOKEN_OPEN_GENERIC:
            unreachable("");
        case TOKEN_CLOSE_PAR:
            unreachable("");
        case TOKEN_CLOSE_CURLY_BRACE:
            unreachable("");
        case TOKEN_CLOSE_SQ_BRACKET:
            unreachable("");
        case TOKEN_CLOSE_GENERIC:
            unreachable("");
        case TOKEN_ENUM:
            unreachable("");
        case TOKEN_SYMBOL:
            unreachable("");
        case TOKEN_DOUBLE_QUOTE:
            unreachable("");
        case TOKEN_NEW_LINE:
            unreachable("");
        case TOKEN_COMMA:
            unreachable("");
        case TOKEN_COLON:
            unreachable("");
        case TOKEN_SINGLE_DOT:
            unreachable("");
        case TOKEN_DOUBLE_DOT:
            unreachable("");
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_EOF:
            unreachable("");
        case TOKEN_ASSIGN_BY_BIN:
            unreachable("");
        case TOKEN_MACRO:
            unreachable("");
        case TOKEN_DEFER:
            unreachable("");
        case TOKEN_DOUBLE_TICK:
            unreachable("");
        case TOKEN_ONE_LINE_BLOCK_START:
            unreachable("");
        case TOKEN_UNDERSCORE:
            unreachable("");
        case TOKEN_FN:
            unreachable("");
        case TOKEN_FOR:
            unreachable("");
        case TOKEN_IF:
            unreachable("");
        case TOKEN_SWITCH:
            unreachable("");
        case TOKEN_CASE:
            unreachable("");
        case TOKEN_DEFAULT:
            unreachable("");
        case TOKEN_ELSE:
            unreachable("");
        case TOKEN_RETURN:
            unreachable("");
        case TOKEN_EXTERN:
            unreachable("");
        case TOKEN_STRUCT:
            unreachable("");
        case TOKEN_LET:
            unreachable("");
        case TOKEN_IN:
            unreachable("");
        case TOKEN_BREAK:
            unreachable("");
        case TOKEN_YIELD:
            unreachable("");
        case TOKEN_CONTINUE:
            unreachable("");
        case TOKEN_RAW_UNION:
            unreachable("");
        case TOKEN_TYPE_DEF:
            unreachable("");
        case TOKEN_GENERIC_TYPE:
            unreachable("");
        case TOKEN_IMPORT:
            unreachable("");
        case TOKEN_DEF:
            unreachable("");
        case TOKEN_SIZEOF:
            unreachable("");
        case TOKEN_COUNTOF:
            unreachable("");
        case TOKEN_USING:
            unreachable("");
        case TOKEN_COMMENT:
            unreachable("");
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_INT_LITERAL:
            unreachable("");
        case TOKEN_FLOAT_LITERAL:
            unreachable("");
        case TOKEN_VOID:
            unreachable("");
        case TOKEN_CHAR_LITERAL:
            unreachable("");
        case TOKEN_COUNT:
            unreachable("");
        default:
            unreachable(FMT, token_type_print(TOKEN_MODE_LOG, token_type));
    }
    unreachable("");
}

static inline UNARY_TYPE token_type_to_unary_type(TOKEN_TYPE token_type) {
    static_assert(TOKEN_COUNT == 78, "exhausive handling of token types (only unary tokens are handled)");

    switch (token_type) {
        case TOKEN_UNSAFE_CAST:
            return UNARY_UNSAFE_CAST;
        case TOKEN_ASTERISK:
            return UNARY_DEREF;
        case TOKEN_BITWISE_AND:
            return UNARY_REFER;
        case TOKEN_SIZEOF:
            return UNARY_SIZEOF;
        case TOKEN_COUNTOF:
            return UNARY_COUNTOF;
        case TOKEN_LOGICAL_NOT:
            return UNARY_LOGICAL_NOT;
        case TOKEN_BITWISE_NOT:
            unreachable("");
        case TOKEN_ORELSE:
            unreachable("");
        case TOKEN_QUESTION_MARK:
            unreachable("");
        case TOKEN_OPEN_PAR:
            unreachable("");
        case TOKEN_OPEN_CURLY_BRACE:
            unreachable("");
        case TOKEN_OPEN_SQ_BRACKET:
            unreachable("");
        case TOKEN_OPEN_GENERIC:
            unreachable("");
        case TOKEN_CLOSE_PAR:
            unreachable("");
        case TOKEN_CLOSE_CURLY_BRACE:
            unreachable("");
        case TOKEN_CLOSE_SQ_BRACKET:
            unreachable("");
        case TOKEN_CLOSE_GENERIC:
            unreachable("");
        case TOKEN_ENUM:
            unreachable("");
        case TOKEN_SYMBOL:
            unreachable("");
        case TOKEN_DOUBLE_QUOTE:
            unreachable("");
        case TOKEN_NEW_LINE:
            unreachable("");
        case TOKEN_COMMA:
            unreachable("");
        case TOKEN_COLON:
            unreachable("");
        case TOKEN_SINGLE_DOT:
            unreachable("");
        case TOKEN_DOUBLE_DOT:
            unreachable("");
        case TOKEN_TRIPLE_DOT:
            unreachable("");
        case TOKEN_EOF:
            unreachable("");
        case TOKEN_ASSIGN_BY_BIN:
            unreachable("");
        case TOKEN_MACRO:
            unreachable("");
        case TOKEN_DEFER:
            unreachable("");
        case TOKEN_DOUBLE_TICK:
            unreachable("");
        case TOKEN_ONE_LINE_BLOCK_START:
            unreachable("");
        case TOKEN_UNDERSCORE:
            unreachable("");
        case TOKEN_FN:
            unreachable("");
        case TOKEN_FOR:
            unreachable("");
        case TOKEN_IF:
            unreachable("");
        case TOKEN_SWITCH:
            unreachable("");
        case TOKEN_CASE:
            unreachable("");
        case TOKEN_DEFAULT:
            unreachable("");
        case TOKEN_ELSE:
            unreachable("");
        case TOKEN_RETURN:
            unreachable("");
        case TOKEN_EXTERN:
            unreachable("");
        case TOKEN_STRUCT:
            unreachable("");
        case TOKEN_LET:
            unreachable("");
        case TOKEN_IN:
            unreachable("");
        case TOKEN_BREAK:
            unreachable("");
        case TOKEN_YIELD:
            unreachable("");
        case TOKEN_CONTINUE:
            unreachable("");
        case TOKEN_RAW_UNION:
            unreachable("");
        case TOKEN_TYPE_DEF:
            unreachable("");
        case TOKEN_GENERIC_TYPE:
            unreachable("");
        case TOKEN_IMPORT:
            unreachable("");
        case TOKEN_DEF:
            unreachable("");
        case TOKEN_USING:
            unreachable("");
        case TOKEN_COMMENT:
            unreachable("");
        case TOKEN_STRING_LITERAL:
            unreachable("");
        case TOKEN_INT_LITERAL:
            unreachable("");
        case TOKEN_FLOAT_LITERAL:
            unreachable("");
        case TOKEN_VOID:
            unreachable("");
        case TOKEN_CHAR_LITERAL:
            unreachable("");
        case TOKEN_COUNT:
            unreachable("");
        case TOKEN_DOUBLE_EQUAL:
            unreachable("");
        case TOKEN_SINGLE_PLUS:
            unreachable("");
        case TOKEN_SINGLE_MINUS:
            unreachable("");
        case TOKEN_SLASH:
            unreachable("");
        case TOKEN_MODULO:
            unreachable("");
        case TOKEN_LESS_THAN:
            unreachable("");
        case TOKEN_GREATER_THAN:
            unreachable("");
        case TOKEN_GREATER_OR_EQUAL:
            unreachable("");
        case TOKEN_LESS_OR_EQUAL:
            unreachable("");
        case TOKEN_LOGICAL_NOT_EQUAL:
            unreachable("");
        case TOKEN_BITWISE_OR:
            unreachable("");
        case TOKEN_BITWISE_XOR:
            unreachable("");
        case TOKEN_LOGICAL_AND:
            unreachable("");
        case TOKEN_LOGICAL_OR:
            unreachable("");
        case TOKEN_SHIFT_LEFT:
            unreachable("");
        case TOKEN_SHIFT_RIGHT:
            unreachable("");
        case TOKEN_SINGLE_EQUAL:
            unreachable("");
        case TOKEN_NONTYPE:
            unreachable("");
        default:
            unreachable(FMT, token_type_print(TOKEN_MODE_LOG, token_type));
    }
    unreachable("");
}

#endif // TOKEN_TYPE_TO_OPERATOR_TYPE
