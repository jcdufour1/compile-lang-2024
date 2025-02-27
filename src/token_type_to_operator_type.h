#ifndef TOKEN_TYPE_TO_OPERATOR_TYPE
#define TOKEN_TYPE_TO_OPERATOR_TYPE

#include <token.h>
#include <operator_type.h>

static inline BINARY_TYPE token_type_to_binary_type(TOKEN_TYPE token_type) {
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
        case TOKEN_NOT_EQUAL:
            return BINARY_NOT_EQUAL;
        case TOKEN_BITWISE_AND:
            return BINARY_BITWISE_AND;
        case TOKEN_BITWISE_OR:
            return BINARY_BITWISE_OR;
        default:
            unreachable(TOKEN_TYPE_FMT, token_type_print(TOKEN_MODE_LOG, token_type));
    }
    unreachable("");
}

static inline UNARY_TYPE token_type_to_unary_type(TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_UNSAFE_CAST:
            return UNARY_UNSAFE_CAST;
        case TOKEN_DEREF:
            return UNARY_DEREF;
        case TOKEN_REFER:
            return UNARY_REFER;
        case TOKEN_NOT:
            return UNARY_NOT;
        default:
            unreachable(TOKEN_TYPE_FMT, token_type_print(TOKEN_MODE_LOG, token_type));
    }
    unreachable("");
}

#endif // TOKEN_TYPE_TO_OPERATOR_TYPE
