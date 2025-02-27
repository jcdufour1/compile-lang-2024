#ifndef OPERATOR_TYPE_H
#define OPERATOR_TYPE_H

#include <str_view.h>

typedef enum {
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

static inline Str_view binary_type_to_str_view(BINARY_TYPE bin_type) {
    switch (bin_type) {
        case BINARY_SUB:
            return str_view_from_cstr("-");
        case BINARY_ADD:
            return str_view_from_cstr("+");
        case BINARY_MULTIPLY:
            return str_view_from_cstr("*");
        case BINARY_DIVIDE:
            return str_view_from_cstr("/");
        case BINARY_MODULO:
            return str_view_from_cstr("%");
        case BINARY_LESS_THAN:
            return str_view_from_cstr("<");
        case BINARY_LESS_OR_EQUAL:
            return str_view_from_cstr("<=");
        case BINARY_GREATER_OR_EQUAL:
            return str_view_from_cstr(">=");
        case BINARY_GREATER_THAN:
            return str_view_from_cstr(">");
        case BINARY_DOUBLE_EQUAL:
            return str_view_from_cstr("==");
        case BINARY_NOT_EQUAL:
            return str_view_from_cstr("!=");
        case BINARY_BITWISE_XOR:
            return str_view_from_cstr("xor");
        case BINARY_BITWISE_AND:
            return str_view_from_cstr("&");
        case BINARY_BITWISE_OR:
            return str_view_from_cstr("|");
        case BINARY_LOGICAL_AND:
            return str_view_from_cstr("&&");
        case BINARY_LOGICAL_OR:
            return str_view_from_cstr("||");
        case BINARY_SHIFT_LEFT:
            return str_view_from_cstr("<<");
        case BINARY_SHIFT_RIGHT:
            return str_view_from_cstr(">>");
    }
    unreachable("");
}

#define binary_type_print(bin_type) str_view_print(binary_type_to_str_view(bin_type))

typedef enum {
    UNARY_DEREF,
    UNARY_REFER,
    UNARY_UNSAFE_CAST,
    UNARY_NOT,
} UNARY_TYPE;

static inline Str_view unary_type_to_str_view(UNARY_TYPE unary_type) {
    switch (unary_type) {
        case UNARY_DEREF:
            return str_view_from_cstr("deref");
        case UNARY_REFER:
            return str_view_from_cstr("refer");
        case UNARY_UNSAFE_CAST:
            return str_view_from_cstr("unsafe_cast");
        case UNARY_NOT:
            return str_view_from_cstr("!");
    }
    unreachable("");
}

#define unary_type_print(unary_type) str_view_print(unary_type_to_str_view(unary_type))

#endif // OPERATOR_TYPE_H
