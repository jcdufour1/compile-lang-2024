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
    NODE_BLOCK,
    NODE_FUNCTION_DEFINITION,
    NODE_FUNCTION_PARAMETERS,
    NODE_FUNCTION_RETURN_TYPES,
    NODE_FUNCTION_CALL,
    NODE_FUNCTION_PARAM_SYM,
    NODE_LITERAL,
    NODE_LANG_TYPE,
    NODE_OPERATOR,
    NODE_SYMBOL,
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
    NODE_LOAD,
    NODE_LOAD_STRUCT_MEMBER,
    NODE_STORE,
    NODE_STORE_STRUCT_MEMBER,

    NODE_FUNCTION_RETURN_VALUE_SYM,
    NODE_OPERATOR_RETURN_VALUE_SYM,
    NODE_MEMCPY,
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

    bool is_variadic : 1;
    bool is_struct_associated : 1;
    bool is_fun_param_associated : 1;

    uint32_t line_num;
    const char* file_path;
} Node;

extern Node* root_of_tree;

#define NODE_FMT STRING_FMT

String node_print_internal(const Node* node);

#define node_print(root) string_print(node_print_internal((root)))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

#endif // NODE_H
