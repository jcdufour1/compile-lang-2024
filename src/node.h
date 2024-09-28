#ifndef NODE_H
#define NODE_H
#include "newstring.h"
#include "str_view.h"
#include "token.h"
#include "util.h"
#include "stdint.h"

typedef enum {
    NODE_NO_TYPE,
    NODE_STRUCT_DEFINITION,
    NODE_STRUCT_LITERAL,
    NODE_STRUCT_MEMBER_SYM,
    NODE_STRUCT_MEMBER_SYM_PIECE,
    NODE_BLOCK,
    NODE_FUNCTION_DEFINITION,
    NODE_FUNCTION_PARAMETERS,
    NODE_FUNCTION_RETURN_TYPES,
    NODE_FUNCTION_CALL,
    NODE_FUNCTION_PARAM_SYM,
    NODE_LITERAL,
    NODE_LANG_TYPE,
    NODE_OPERATOR,
    NODE_SYMBOL_UNTYPED,
    NODE_SYMBOL_TYPED,
    NODE_RETURN_STATEMENT,
    NODE_VARIABLE_DEFINITION,
    NODE_FUNCTION_DECLARATION,
    NODE_FOR_LOOP,
    NODE_FOR_LOWER_BOUND,
    NODE_FOR_UPPER_BOUND,
    NODE_FOR_VARIABLE_DEF,
    NODE_IF_STATEMENT,
    NODE_IF_CONDITION,
    NODE_ASSIGNMENT,
    NODE_GOTO,
    NODE_COND_GOTO,
    NODE_LABEL,
    NODE_ALLOCA,
    NODE_LOAD_VARIABLE,
    NODE_LOAD_STRUCT_MEMBER,
    NODE_STORE_VARIABLE,
    NODE_STORE_STRUCT_MEMBER,
    NODE_LLVM_STORE_STRUCT_LITERAL,

    NODE_LOAD_ANOTHER_NODE,
    NODE_STORE_ANOTHER_NODE,
    NODE_LOAD_STRUCT_ELEMENT_PTR,
    NODE_FUNCTION_RETURN_VALUE_SYM,
    NODE_OPERATOR_RETURN_VALUE_SYM,
    NODE_LLVM_SYMBOL,
    NODE_LLVM_STORE_LITERAL,
} NODE_TYPE;

typedef size_t Llvm_id;

typedef struct Node_ {
    struct Node_* next;
    struct Node_* prev;
    struct Node_* left_child;
    struct Node_* parent;

    NODE_TYPE type;

    TOKEN_TYPE token_type;

    Llvm_id llvm_id;

    Str_view name; // eg. "string1" in "let string1: String = "hello""
    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Str_view lang_type; // eg. "String" in "let string1: String = "hello""
    uint16_t pointer_depth;

    bool is_variadic : 1;

    Pos pos;

    struct Node_* node_src;
    struct Node_* node_dest;
    struct Node_* storage_location;
} Node;

extern Node* root_of_tree;

#define NODE_FMT STR_VIEW_FMT

Str_view node_print_internal(const Node* node);

#define node_print(root) str_view_print(node_print_internal((root)))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

#endif // NODE_H
