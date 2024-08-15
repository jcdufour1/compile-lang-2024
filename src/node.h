#ifndef NODE_H
#define NODE_H

#include "str_view.h"
#include "token.h"

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

typedef struct _Node {
    struct _Node* left;
    struct _Node* right;

    NODE_TYPE type;

    TOKEN_TYPE literal_type;

    Str_view name;
    Str_view lang_type;
    struct _Node* parameters;
    struct _Node* return_types;
    struct _Node* body;
} Node;

#endif // NODE_H
