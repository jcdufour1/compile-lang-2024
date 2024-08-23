#ifndef NODE_H
#define NODE_H

#include "newstring.h"
#include "str_view.h"
#include "token.h"
#include "util.h"
#include "stdint.h"

typedef enum {
    NODE_NO_TYPE,
    NODE_BLOCK,
    NODE_FUNCTION_DEFINITION,
    NODE_FUNCTION_PARAMETERS,
    NODE_FUNCTION_RETURN_TYPES,
    NODE_FUNCTION_BODY,
    NODE_FUNCTION_CALL,
    NODE_LITERAL,
    NODE_LANG_TYPE,
    NODE_OPERATOR,
    NODE_SYMBOL,
    NODE_RETURN_STATEMENT,
    NODE_VARIABLE_DEFINITION,
    NODE_FUNCTION_DECLARATION,
} NODE_TYPE;

typedef size_t Node_id;

typedef struct {
    Node_id next;
    Node_id prev;
    Node_id left_child;

    NODE_TYPE type;

    TOKEN_TYPE token_type;

    size_t llvm_id;

    Str_view name; // eg. "string1" in "let string1: String = "hello""
    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Str_view lang_type; // eg. "String" in "let string1: String = "hello""

    uint32_t line_num;
} Node;

#define NODE_FMT STRING_FMT

#define NODE_IDX_NULL SIZE_MAX

String node_print_internal(Node_id node);

#define node_print(root) string_print(node_print_internal((root)))

#endif // NODE_H
