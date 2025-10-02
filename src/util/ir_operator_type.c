#include <ir_operator_type.h>

IR_BINARY_TYPE ir_binary_type_from_binary_type(BINARY_TYPE type) {
    static_assert(BINARY_COUNT == 19, "exhausive handling of binary types");
    switch (type) {
        case BINARY_SINGLE_EQUAL:
            return IR_BINARY_SINGLE_EQUAL;
        case BINARY_SUB:
            return IR_BINARY_SUB;
        case BINARY_ADD:
            return IR_BINARY_ADD;
        case BINARY_MULTIPLY:
            return IR_BINARY_MULTIPLY;
        case BINARY_DIVIDE:
            return IR_BINARY_DIVIDE;
        case BINARY_MODULO:
            return IR_BINARY_MODULO;
        case BINARY_LESS_THAN:
            return IR_BINARY_LESS_THAN;
        case BINARY_LESS_OR_EQUAL:
            return IR_BINARY_LESS_OR_EQUAL;
        case BINARY_GREATER_OR_EQUAL:
            return IR_BINARY_GREATER_OR_EQUAL;
        case BINARY_GREATER_THAN:
            return IR_BINARY_GREATER_THAN;
        case BINARY_DOUBLE_EQUAL:
            return IR_BINARY_DOUBLE_EQUAL;
        case BINARY_NOT_EQUAL:
            return IR_BINARY_NOT_EQUAL;
        case BINARY_BITWISE_XOR:
            return IR_BINARY_BITWISE_XOR;
        case BINARY_BITWISE_AND:
            return IR_BINARY_BITWISE_AND;
        case BINARY_BITWISE_OR:
            return IR_BINARY_BITWISE_OR;
        case BINARY_LOGICAL_AND:
            return IR_BINARY_LOGICAL_AND;
        case BINARY_LOGICAL_OR:
            return IR_BINARY_LOGICAL_OR;
        case BINARY_SHIFT_LEFT:
            return IR_BINARY_SHIFT_LEFT;
        case BINARY_SHIFT_RIGHT:
            return IR_BINARY_SHIFT_RIGHT;
        case BINARY_COUNT:
            unreachable("");
    }
    unreachable("");
}

IR_UNARY_TYPE ir_unary_type_from_unary_type(UNARY_TYPE type) {
    static_assert(BINARY_COUNT == 19, "exhausive handling of binary types");
    switch (type) {
        case UNARY_DEREF:
            return IR_UNARY_DEREF;
        case UNARY_REFER:
            return IR_UNARY_REFER;
        case UNARY_UNSAFE_CAST:
            return IR_UNARY_UNSAFE_CAST;
        case UNARY_LOGICAL_NOT:
            return IR_UNARY_LOGICAL_NOT;
        case UNARY_SIZEOF:
            return IR_UNARY_SIZEOF;
        case UNARY_COUNTOF:
            return IR_UNARY_COUNTOF;
        case UNARY_COUNT:
            unreachable("");
    }
    unreachable("");
}
