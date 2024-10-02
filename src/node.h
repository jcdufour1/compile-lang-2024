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
    NODE_STRUCT_MEMBER_SYM_UNTYPED,
    NODE_STRUCT_MEMBER_SYM_TYPED,
    NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED,
    NODE_STRUCT_MEMBER_SYM_PIECE_TYPED,
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
    NODE_BREAK,
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

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type;

typedef struct {
    TOKEN_TYPE token_type;

    Llvm_id llvm_id;

    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""

    bool is_variadic : 1;

    size_t struct_index;

    struct Node_* node_src;
    struct Node_* node_dest;
    struct Node_* storage_location;
} Node_generic;

typedef union {
    Node_generic node_generic;
} Node_as;

typedef struct Node_ {
    Node_as as;
    Pos pos;
    NODE_TYPE type;

    Str_view name; // eg. "string1" in "let string1: String = "hello""

    struct Node_* next;
    struct Node_* prev;
    struct Node_* left_child;
    struct Node_* parent;
} Node;

static inline Node_generic* node_unwrap_generic(Node* node) {
    return (Node_generic*)node;
}

static inline const Node_generic* node_unwrap_generic_const(const Node* node) {
    return (const Node_generic*)node;
}

extern Node* root_of_tree;

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

// only literals can be used here
static inline Lang_type lang_type_from_cstr(const char* cstr, int16_t pointer_depth) {
    Lang_type Lang_type = {.str = str_view_from_cstr(cstr), .pointer_depth = pointer_depth};
    return Lang_type;
}

static inline bool lang_type_is_equal(Lang_type a, Lang_type b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return str_view_is_equal(a.str, b.str);
}

Str_view lang_type_print_internal(Arena* arena, Lang_type lang_type, bool surround_in_lt_gt);

#define lang_type_print(lang_type) str_view_print(lang_type_print_internal(&print_arena, (lang_type), false))

#define NODE_FMT STR_VIEW_FMT

Str_view node_print_internal(Arena* arena, const Node* node);

#define node_print(root) str_view_print(node_print_internal(&print_arena, (root)))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

#endif // NODE_H
