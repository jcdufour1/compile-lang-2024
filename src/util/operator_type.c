#include <operator_type.h>

BINARY_TYPE binary_type_from_token_type(TOKEN_TYPE type) {
    static_assert(TOKEN_COUNT == 77, "exhausive handling of token types; only binary operators need to be explicitly handled here");
    switch (type) {
        case TOKEN_SINGLE_EQUAL:
            return BINARY_SINGLE_EQUAL;
        case TOKEN_SINGLE_MINUS:
            return BINARY_SUB;
        case TOKEN_SINGLE_PLUS:
            return BINARY_ADD;
        case TOKEN_ASTERISK:
            return BINARY_MULTIPLY;
        case TOKEN_SLASH:
            return BINARY_DIVIDE;
        case TOKEN_MODULO:
            return BINARY_MODULO;
        case TOKEN_LESS_THAN:
            return BINARY_LESS_THAN;
        case TOKEN_LESS_OR_EQUAL:
            return BINARY_LESS_OR_EQUAL;
        case TOKEN_GREATER_OR_EQUAL:
            return BINARY_GREATER_OR_EQUAL;
        case TOKEN_GREATER_THAN:
            return BINARY_GREATER_THAN;
        case TOKEN_DOUBLE_EQUAL:
            return BINARY_DOUBLE_EQUAL;
        case TOKEN_LOGICAL_NOT_EQUAL:
            return BINARY_NOT_EQUAL;
        case TOKEN_BITWISE_XOR:
            return BINARY_BITWISE_XOR;
        case TOKEN_BITWISE_AND:
            return BINARY_BITWISE_AND;
        case TOKEN_BITWISE_OR:
            return BINARY_BITWISE_OR;
        case TOKEN_LOGICAL_AND:
            return BINARY_LOGICAL_AND;
        case TOKEN_LOGICAL_OR:
            return BINARY_LOGICAL_OR;
        case TOKEN_SHIFT_LEFT:
            return BINARY_SHIFT_LEFT;
        case TOKEN_SHIFT_RIGHT:
            return BINARY_SHIFT_RIGHT;
        default:
            unreachable("");
    }
    unreachable("");
}
