#ifndef NODE_H
#define NODE_H

#include "node_ptr_vec.h"
#include "newstring.h"
#include "str_view.h"
#include "token.h"
#include "util.h"
#include "symbol_table_struct.h"

#define FOR_LIST_OF_NODES(DO) \
    DO(llvm_store_struct_literal, NODE_LLVM_STORE_STRUCT_LITERAL) \
    DO(cond_goto, NODE_COND_GOTO) \
    DO(goto, NODE_GOTO) \
    DO(llvm_store_literal, NODE_LLVM_STORE_LITERAL) \
    DO(store_another_node, NODE_STORE_ANOTHER_NODE) \
    DO(load_another_node, NODE_LOAD_ANOTHER_NODE) \
    DO(alloca, NODE_ALLOCA) \
    DO(struct_member_sym_typed, NODE_STRUCT_MEMBER_SYM_TYPED) \
    DO(struct_member_sym_untyped, NODE_STRUCT_MEMBER_SYM_UNTYPED) \
    DO(block, NODE_BLOCK) \
    DO(if_condition, NODE_IF_CONDITION) \
    DO(if, NODE_IF) \
    DO(assignment, NODE_ASSIGNMENT) \
    DO(break, NODE_BREAK) \
    DO(for_upper_bound, NODE_FOR_UPPER_BOUND) \
    DO(for_lower_bound, NODE_FOR_LOWER_BOUND) \
    DO(return_statement, NODE_RETURN_STATEMENT) \
    DO(for_range, NODE_FOR_RANGE) \
    DO(function_definition, NODE_FUNCTION_DEFINITION) \
    DO(function_declaration, NODE_FUNCTION_DECLARATION) \
    DO(function_return_types, NODE_FUNCTION_RETURN_TYPES) \
    DO(struct_def, NODE_STRUCT_DEF) \
    DO(struct_member_sym_piece_typed, NODE_STRUCT_MEMBER_SYM_PIECE_TYPED) \
    DO(struct_member_sym_piece_untyped, NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED) \
    DO(variable_def, NODE_VARIABLE_DEF) \
    DO(struct_literal, NODE_STRUCT_LITERAL) \
    DO(lang_type, NODE_LANG_TYPE) \
    DO(function_params, NODE_FUNCTION_PARAMS) \
    DO(function_call, NODE_FUNCTION_CALL) \
    DO(literal, NODE_LITERAL) \
    DO(load_element_ptr, NODE_LOAD_ELEMENT_PTR) \
    DO(label, NODE_LABEL) \
    DO(symbol_typed, NODE_SYMBOL_TYPED) \
    DO(symbol_untyped, NODE_SYMBOL_UNTYPED) \
    DO(ptr_byval_sym, NODE_PTR_BYVAL_SYM) \
    DO(llvm_register_sym, NODE_LLVM_REGISTER_SYM) \
    DO(for_with_condition, NODE_FOR_WITH_CONDITION) \
    DO(operator, NODE_OPERATOR)

#define FOR_LIST_OF_NODE_OPERATORS(DO) \
    DO(unary, NODE_OP_UNARY) \
    DO(binary, NODE_OP_BINARY)

#define FOR_LIST_OF_NODE_LITERALS(DO) \
    DO(number, NODE_LIT_NUMBER) \
    DO(string, NODE_LIT_STRING) \
    DO(void, NODE_LIT_VOID)

struct Node_;

#define X(lower, upper) \
    upper,
typedef enum {
    FOR_LIST_OF_NODES(X)
} NODE_TYPE;
#undef X

typedef size_t Llvm_id;

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type;

typedef struct {
    Str_view name;
} Node_symbol_untyped;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Str_view str_data;
    Str_view name;
} Node_symbol_typed;

typedef struct {
    Llvm_id llvm_id;
    Str_view name;
} Node_label;

typedef struct {
    Lang_type lang_type;
    Llvm_id llvm_id;
    size_t struct_index;
    struct Node_* node_src;
    Str_view name;
} Node_load_element_ptr;

typedef struct {
    Str_view data; // eg. "hello" in "let string1: String = "hello""
} Node_lit_string;

typedef struct {
    int64_t data;
} Node_lit_number;

typedef struct {
    int dummy;
} Node_lit_void;

#define X(lower, upper) \
    Node_lit_##lower _##lower;
typedef union {
    FOR_LIST_OF_NODE_LITERALS(X)
} Node_literal_as;
#undef X

#define X(lower, upper) \
    upper,
typedef enum {
    FOR_LIST_OF_NODE_LITERALS(X)
} NODE_LITERAL_TYPE;
#undef X

typedef struct {
    Node_literal_as as;
    NODE_LITERAL_TYPE type;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Str_view name;
} Node_literal;

typedef struct {
    Node_ptr_vec args;
    Str_view name;
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_function_call;

typedef struct {
    Node_ptr_vec params;
    Llvm_id llvm_id;
} Node_function_params;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_lang_type;

typedef struct {
    struct Node_* child;
    TOKEN_TYPE token_type;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
} Node_op_unary;

typedef struct {
    struct Node_* lhs;
    struct Node_* rhs;
    TOKEN_TYPE token_type;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
} Node_op_binary;

#define X(lower, upper) \
    Node_op_##lower _##lower;
typedef union {
    FOR_LIST_OF_NODE_OPERATORS(X)
} Node_operator_as;
#undef X

#define X(lower, upper) \
    upper,
typedef enum {
    FOR_LIST_OF_NODE_OPERATORS(X)
} NODE_OPERATOR_TYPE;
#undef X

typedef struct {
    Node_operator_as as;
    NODE_OPERATOR_TYPE type;
} Node_operator;

typedef struct {
    Node_ptr_vec members;
    Str_view name;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
} Node_struct_literal;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    bool is_variadic : 1;
    Llvm_id llvm_id;
    struct Node_* storage_location;
    Str_view name;
} Node_variable_def;

typedef struct {
    Str_view name;
} Node_struct_member_sym_piece_untyped;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    size_t struct_index;
    Str_view name;
} Node_struct_member_sym_piece_typed;

typedef struct {
    Node_ptr_vec members;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
    Str_view name;
} Node_struct_def;

typedef struct {
    Node_ptr_vec children;
    Symbol_table symbol_table;
} Node_block;

typedef struct {
    Node_lang_type* child;
} Node_function_return_types;

typedef struct {
    Node_function_params* parameters;
    Node_function_return_types* return_types;
    Str_view name;
} Node_function_declaration;

typedef struct {
    Node_function_declaration* declaration;
    Node_block* body;
    Llvm_id llvm_id;
} Node_function_definition;

typedef struct {
    struct Node_* child;
} Node_for_lower_bound;

typedef struct {
    struct Node_* child;
} Node_for_upper_bound;

typedef struct {
    struct Node_* child;
} Node_if_condition;

typedef struct {
    Node_variable_def* var_def;
    Node_for_lower_bound* lower_bound;
    Node_for_upper_bound* upper_bound;
    Node_block* body;
} Node_for_range;

typedef struct {
    Node_if_condition* condition;
    Node_block* body;
} Node_for_with_condition;

typedef struct {
    struct Node_* child;
    bool auto_inserted : 1;
} Node_return_statement;

typedef struct {
    struct Node_* child;
} Node_break;

typedef struct {
    struct Node_* lhs;
    struct Node_* rhs;
} Node_assignment;

typedef struct {
    Node_if_condition* condition;
    Node_block* body;
} Node_if;

typedef struct {
    Node_ptr_vec children;
    Str_view name;
    Lang_type lang_type;
} Node_struct_member_sym_untyped;

typedef struct {
    Node_ptr_vec children;
    Llvm_id llvm_id;
    Str_view name;
} Node_struct_member_sym_typed;

typedef struct {
    Llvm_id llvm_id;
    Lang_type lang_type;
    Str_view name;
} Node_alloca;

typedef struct {
    struct Node_* node_src;
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_load_another_node;

typedef struct {
    struct Node_* node_src;
    struct Node_* node_dest;
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_store_another_node;

typedef struct {
    Lang_type lang_type;
    struct Node_* node_src;
    Llvm_id llvm_id;
} Node_llvm_register_sym;

typedef struct {
    Node_literal* child;
    struct Node_* node_dest;
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_llvm_store_literal;

typedef struct {
    Str_view name;
    Llvm_id llvm_id;
} Node_goto;

typedef struct {
    Node_operator* node_src;
    Node_symbol_untyped* if_true;
    Node_symbol_untyped* if_false;
    Llvm_id llvm_id;
} Node_cond_goto;

typedef struct {
    Node_struct_literal* child;
    Llvm_id llvm_id;
    Lang_type lang_type;
    struct Node_* node_dest;
} Node_llvm_store_struct_literal;

typedef struct {
    Lang_type lang_type;
    struct Node_* node_src;
    Llvm_id llvm_id;
} Node_ptr_byval_sym;

#define X(lower, upper) \
    Node_##lower _##lower;
typedef union {
    FOR_LIST_OF_NODES(X)
} Node_as;
#undef X

typedef struct Node_ {
    Node_as as;
    Pos pos;
    NODE_TYPE type;
} Node;

#define X(lower, upper) \
static inline Node_##lower* node_unwrap_##lower(Node* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline const Node_##lower* node_unwrap_##lower##_const(const Node* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline Node* node_wrap_##lower(Node_##lower* node) { \
    return (Node*)node; \
} \
 \
static inline const Node* node_wrap_##lower##_const(const Node_##lower* node) { \
    return (const Node*)node; \
}
FOR_LIST_OF_NODES(X)
#undef X

#define X(lower, upper) \
static inline Node_lit_##lower* node_unwrap_lit_##lower(Node_literal* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline const Node_lit_##lower* node_unwrap_lit_##lower##_const(const Node_literal* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline Node_literal* node_wrap_lit_##lower(Node_lit_##lower* node) { \
    return (Node_literal*)node; \
} \
 \
static inline const Node_literal* node_wrap_lit_##lower##_const(const Node_lit_##lower* node) { \
    return (const Node_literal*)node; \
} \
static inline Node_lit_##lower* node_auto_unwrap_lit_##lower(Node* node) { \
    Node_literal* literal = node_unwrap_literal(node); \
    assert(literal->type == upper); \
    return &literal->as._##lower; \
} \
static inline bool node_is_lit_##lower(const Node* node) { \
    if (node->type != NODE_LITERAL) { \
        return false; \
    } \
    const Node_literal* literal = node_unwrap_literal_const(node); \
    return literal->type == upper; \
}
FOR_LIST_OF_NODE_LITERALS(X)
#undef X

#define X(lower, upper) \
static inline Node_op_##lower* node_unwrap_op_##lower(Node_operator* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline const Node_op_##lower* node_unwrap_op_##lower##_const(const Node_operator* node) { \
    assert(node->type == upper); \
    return &node->as._##lower; \
} \
 \
static inline Node_operator* node_wrap_op_##lower(Node_op_##lower* node) { \
    return (Node_operator*)node; \
} \
 \
static inline const Node_operator* node_wrap_op_##lower##_const(const Node_op_##lower* node) { \
    return (const Node_operator*)node; \
} \
static inline Node_op_##lower* node_auto_unwrap_op_##lower(Node* node) { \
    Node_operator* operator = node_unwrap_operator(node); \
    assert(operator->type == upper); \
    return &operator->as._##lower; \
} \
static inline bool node_is_##lower(const Node* node) { \
    if (node->type != NODE_OPERATOR) { \
        return false; \
    } \
    const Node_operator* operator = node_unwrap_operator_const(node); \
    return operator->type == upper; \
}
FOR_LIST_OF_NODE_OPERATORS(X)
#undef X

#define node_wrap_operator_generic(operator) ((Node_operator*)(operator))

extern Node* root_of_tree;

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

static inline Lang_type lang_type_from_strv(Str_view str_view, int16_t pointer_depth) {
    Lang_type Lang_type = {.str = str_view, .pointer_depth = pointer_depth};
    return Lang_type;
}

// only literals can be used here
static inline Lang_type lang_type_from_cstr(const char* cstr, int16_t pointer_depth) {
    return lang_type_from_strv(str_view_from_cstr(cstr), pointer_depth);
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

#define node_print(root) str_view_print(node_print_internal(&print_arena, (const Node*)(root)))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

#endif // NODE_H
