#ifndef NODE_H
#define NODE_H

#include "str_view.h"

typedef enum {
    NODE_FUNCTION_START,
    NODE_FUNCTION_PARAMETERS,
    NODE_FUNCTION_RETURN_TYPES,
    NODE_FUNCTION_BODY,
} NODE_TYPE;

typedef struct _Node {
    struct _Node* left;
    struct _Node* right;

    NODE_TYPE type;

    Str_view name;
    Str_view lang_type;
    struct _Node* parameters;
    struct _Node* return_types;
    struct _Node* body;
} Node;

#endif // NODE_H
