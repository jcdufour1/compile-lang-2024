#ifndef NODE_H
#define NODE_H

#include "newstring.h"
#include "str_view.h"
#include "token.h"
#include "util.h"
#include "stdint.h"

typedef enum {
    NODE_NO_TYPE,
    NODE_FUNCTION_DEFINITION,
    NODE_FUNCTION_PARAMETERS,
    NODE_FUNCTION_RETURN_TYPES,
    NODE_FUNCTION_BODY,
    NODE_FUNCTION_CALL,
    NODE_FUNCTION_ARGUMENTS,
    NODE_LITERAL,
} NODE_TYPE;

typedef size_t Node_idx;

typedef struct {
    Node_idx next;
    Node_idx prev;
    Node_idx left_child;
    Node_idx right_child;

    NODE_TYPE type;

    TOKEN_TYPE literal_type;

    Str_view name;
    Str_view lang_type;
} Node;

#define NODE_FMT STRING_FMT

#define NODE_IDX_NULL SIZE_MAX

String node_print_internal(Node_idx node, int pad_x);

#define node_print(root, pad_x_) string_print(node_print_internal((root), (pad_x_)))

#endif // NODE_H
