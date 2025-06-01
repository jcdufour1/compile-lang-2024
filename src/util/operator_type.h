#ifndef OPERATOR_TYPE_H
#define OPERATOR_TYPE_H

#include <strv.h>
#include <token.h>

typedef enum {
    BINARY_SINGLE_EQUAL,
    BINARY_SUB,
    BINARY_ADD,
    BINARY_MULTIPLY,
    BINARY_DIVIDE,
    BINARY_MODULO,
    BINARY_LESS_THAN,
    BINARY_LESS_OR_EQUAL,
    BINARY_GREATER_OR_EQUAL,
    BINARY_GREATER_THAN,
    BINARY_DOUBLE_EQUAL,
    BINARY_NOT_EQUAL,
    BINARY_BITWISE_XOR,
    BINARY_BITWISE_AND,
    BINARY_BITWISE_OR,
    BINARY_LOGICAL_AND,
    BINARY_LOGICAL_OR,
    BINARY_SHIFT_LEFT,
    BINARY_SHIFT_RIGHT,
} BINARY_TYPE;

static inline Strv binary_type_to_strv(BINARY_TYPE bin_type) {
    switch (bin_type) {
        case BINARY_SINGLE_EQUAL:
            return sv("=");
        case BINARY_SUB:
            return sv("-");
        case BINARY_ADD:
            return sv("+");
        case BINARY_MULTIPLY:
            return sv("*");
        case BINARY_DIVIDE:
            return sv("/");
        case BINARY_MODULO:
            return sv("%");
        case BINARY_LESS_THAN:
            return sv("<");
        case BINARY_LESS_OR_EQUAL:
            return sv("<=");
        case BINARY_GREATER_OR_EQUAL:
            return sv(">=");
        case BINARY_GREATER_THAN:
            return sv(">");
        case BINARY_DOUBLE_EQUAL:
            return sv("==");
        case BINARY_NOT_EQUAL:
            return sv("!=");
        case BINARY_BITWISE_XOR:
            // TODO: change to "^"
            return sv("xor");
        case BINARY_BITWISE_AND:
            return sv("&");
        case BINARY_BITWISE_OR:
            return sv("|");
        case BINARY_LOGICAL_AND:
            return sv("&&");
        case BINARY_LOGICAL_OR:
            return sv("||");
        case BINARY_SHIFT_LEFT:
            return sv("<<");
        case BINARY_SHIFT_RIGHT:
            return sv(">>");
    }
    unreachable("");
}

#define binary_type_print(bin_type) strv_print(binary_type_to_strv(bin_type))

typedef enum {
    UNARY_DEREF,
    UNARY_REFER,
    UNARY_UNSAFE_CAST,
    UNARY_NOT,
} UNARY_TYPE;

static inline Strv unary_type_to_strv(UNARY_TYPE unary_type) {
    switch (unary_type) {
        case UNARY_DEREF:
            return sv("deref");
        case UNARY_REFER:
            return sv("refer");
        case UNARY_UNSAFE_CAST:
            return sv("unsafe_cast");
        case UNARY_NOT:
            return sv("!");
    }
    unreachable("");
}

BINARY_TYPE binary_type_from_token_type(TOKEN_TYPE type);

#define unary_type_print(unary_type) strv_print(unary_type_to_strv(unary_type))

#endif // OPERATOR_TYPE_H
