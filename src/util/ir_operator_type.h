#ifndef IR_OPERATOR_TYPE_H
#define IR_OPERATOR_TYPE_H

#include <strv.h>
#include <token.h>
#include <operator_type.h>

typedef enum {
    IR_BINARY_SINGLE_EQUAL,
    IR_BINARY_SUB,
    IR_BINARY_ADD,
    IR_BINARY_MULTIPLY,
    IR_BINARY_DIVIDE,
    IR_BINARY_MODULO,
    IR_BINARY_LESS_THAN,
    IR_BINARY_LESS_OR_EQUAL,
    IR_BINARY_GREATER_OR_EQUAL,
    IR_BINARY_GREATER_THAN,
    IR_BINARY_DOUBLE_EQUAL,
    IR_BINARY_NOT_EQUAL,
    IR_BINARY_BITWISE_XOR,
    IR_BINARY_BITWISE_AND,
    IR_BINARY_BITWISE_OR,
    IR_BINARY_LOGICAL_AND,
    IR_BINARY_LOGICAL_OR,
    IR_BINARY_SHIFT_LEFT,
    IR_BINARY_SHIFT_RIGHT,

    IR_BINARY_COUNT,
} IR_BINARY_TYPE;

static inline Strv ir_binary_type_to_strv(IR_BINARY_TYPE bin_type) {
    switch (bin_type) {
        case IR_BINARY_SINGLE_EQUAL:
            return sv("=");
        case IR_BINARY_SUB:
            return sv("-");
        case IR_BINARY_ADD:
            return sv("+");
        case IR_BINARY_MULTIPLY:
            return sv("*");
        case IR_BINARY_DIVIDE:
            return sv("/");
        case IR_BINARY_MODULO:
            return sv("%");
        case IR_BINARY_LESS_THAN:
            return sv("<");
        case IR_BINARY_LESS_OR_EQUAL:
            return sv("<=");
        case IR_BINARY_GREATER_OR_EQUAL:
            return sv(">=");
        case IR_BINARY_GREATER_THAN:
            return sv(">");
        case IR_BINARY_DOUBLE_EQUAL:
            return sv("==");
        case IR_BINARY_NOT_EQUAL:
            return sv("!=");
        case IR_BINARY_BITWISE_XOR:
            return sv("^");
        case IR_BINARY_BITWISE_AND:
            return sv("&");
        case IR_BINARY_BITWISE_OR:
            return sv("|");
        case IR_BINARY_LOGICAL_AND:
            return sv("&&");
        case IR_BINARY_LOGICAL_OR:
            return sv("||");
        case IR_BINARY_SHIFT_LEFT:
            return sv("<<");
        case IR_BINARY_SHIFT_RIGHT:
            return sv(">>");
        case IR_BINARY_COUNT:
            unreachable("");
    }
    unreachable("");
}

#define ir_binary_type_print(bin_type) strv_print(ir_binary_type_to_strv(bin_type))

typedef enum {
    IR_UNARY_DEREF,
    IR_UNARY_REFER,
    IR_UNARY_UNSAFE_CAST,
    IR_UNARY_LOGICAL_NOT,
    IR_UNARY_SIZEOF,
    IR_UNARY_COUNTOF,
} IR_UNARY_TYPE;

static inline Strv ir_unary_type_to_strv(IR_UNARY_TYPE ir_unary_type) {
    switch (ir_unary_type) {
        case IR_UNARY_DEREF:
            return sv("deref");
        case IR_UNARY_REFER:
            return sv("refer");
        case IR_UNARY_UNSAFE_CAST:
            return sv("unsafe_cast");
        case IR_UNARY_LOGICAL_NOT:
            return sv("!");
        case IR_UNARY_SIZEOF:
            return sv("sizeof");
        case IR_UNARY_COUNTOF:
            return sv("countof");
    }
    unreachable("");
}

IR_BINARY_TYPE ir_binary_type_from_binary_type(BINARY_TYPE type);

IR_UNARY_TYPE ir_unary_type_from_unary_type(UNARY_TYPE type);

#define ir_unary_type_print(ir_unary_type) strv_print(ir_unary_type_to_strv(ir_unary_type))

#endif // IR_OPERATOR_TYPE_H
