#ifndef NODE_H
#define NODE_H
#include "newstring.h"
#include "str_view.h"
#include "token.h"
#include "util.h"
#include "stdint.h"

struct Node_;

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
    int dummy;
} Node_symbol_untyped;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Str_view str_data;
} Node_symbol_typed;

typedef struct {
    int dummy;
} Node_label;

typedef struct {
    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    TOKEN_TYPE token_type;
} Node_literal;

typedef struct {
    Llvm_id llvm_id;
} Node_function_call;

typedef struct {
    Llvm_id llvm_id;
} Node_function_params;

typedef struct {
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_lang_type;

typedef struct {
    TOKEN_TYPE token_type;
} Node_operator;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_struct_literal;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    struct Node_* node_src;
    struct Node_* node_dest;
} Node_load_struct_elem_ptr;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    bool is_variadic : 1;
    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Llvm_id llvm_id;
} Node_variable_def;

typedef struct {
    int a;
} Node_for_variable_def;

typedef struct {
    int a;
} Node_struct_member_sym_piece_untyped;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_struct_member_sym_piece_typed;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_struct_def;

typedef struct {
    int a;
} Node_function_return_types;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_struct_member_def;

typedef struct {
    int a;
} Node_function_declaration;

typedef struct {
    int a;
} Node_function_definition;

typedef struct {
    int a;
} Node_for_loop;

typedef struct {
    int a;
} Node_return_statement;

typedef struct {
    int a;
} Node_for_lower_bound;

typedef struct {
    int a;
} Node_for_upper_bound;

typedef struct {
    int a;
} Node_break;

typedef struct {
    int a;
} Node_assignment;

typedef struct {
    int a;
} Node_if;

typedef struct {
    int a;
} Node_if_condition;

typedef struct {
    int a;
} Node_block;

typedef struct {
    int a;
} Node_struct_member_sym_untyped;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    struct Node_* node_src;
} Node_struct_member_sym_typed;

typedef union {
    Node_struct_member_sym_piece_untyped memb_sym_piece_untyped;
    Node_struct_member_sym_piece_typed memb_sym_piece_typed;
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

static inline Node_symbol_untyped* node_unwrap_symbol_untyped(Node* node) {
    assert(node->type == NODE_SYMBOL_UNTYPED);
    return (Node_symbol_untyped*)node;
}

static inline const Node_symbol_untyped* node_unwrap_symbol_untyped_const(const Node* node) {
    assert(node->type == NODE_SYMBOL_UNTYPED);
    return (const Node_symbol_untyped*)node;
}

static inline Node_symbol_typed* node_unwrap_symbol_typed(Node* node) {
    assert(node->type == NODE_SYMBOL_TYPED);
    return (Node_symbol_typed*)node;
}

static inline const Node_symbol_typed* node_unwrap_symbol_typed_const(const Node* node) {
    assert(node->type == NODE_SYMBOL_TYPED);
    return (const Node_symbol_typed*)node;
}

static inline Node_label* node_unwrap_label(Node* node) {
    assert(node->type == NODE_LABEL);
    return (Node_label*)node;
}

static inline const Node_label* node_unwrap_label_const(const Node* node) {
    assert(node->type == NODE_LABEL);
    return (const Node_label*)node;
}

static inline Node_literal* node_unwrap_literal(Node* node) {
    assert(node->type == NODE_LITERAL);
    return (Node_literal*)node;
}

static inline const Node_literal* node_unwrap_literal_const(const Node* node) {
    assert(node->type == NODE_LITERAL);
    return (const Node_literal*)node;
}

static inline Node_function_call* node_unwrap_function_call(Node* node) {
    assert(node->type == NODE_FUNCTION_CALL);
    return (Node_function_call*)node;
}

static inline const Node_function_call* node_unwrap_function_call_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_CALL);
    return (const Node_function_call*)node;
}

static inline Node_lang_type* node_unwrap_lang_type(Node* node) {
    assert(node->type == NODE_LANG_TYPE);
    return (Node_lang_type*)node;
}

static inline const Node_lang_type* node_unwrap_lang_type_const(const Node* node) {
    assert(node->type == NODE_LANG_TYPE);
    return (const Node_lang_type*)node;
}

static inline Node_operator* node_unwrap_operator(Node* node) {
    assert(node->type == NODE_OPERATOR);
    return (Node_operator*)node;
}

static inline const Node_operator* node_unwrap_operator_const(const Node* node) {
    assert(node->type == NODE_OPERATOR);
    return (const Node_operator*)node;
}

static inline Node_struct_literal* node_unwrap_struct_literal(Node* node) {
    assert(node->type == NODE_STRUCT_LITERAL);
    return (Node_struct_literal*)node;
}

static inline const Node_struct_literal* node_unwrap_struct_literal_const(const Node* node) {
    assert(node->type == NODE_STRUCT_LITERAL);
    return (const Node_struct_literal*)node;
}

static inline Node_load_struct_elem_ptr* node_unwrap_load_struct_elem_ptr(Node* node) {
    assert(node->type == NODE_LOAD_STRUCT_ELEMENT_PTR);
    return (Node_load_struct_elem_ptr*)node;
}

static inline const Node_load_struct_elem_ptr* node_unwrap_load_struct_elem_ptr_const(const Node* node) {
    assert(node->type == NODE_LOAD_STRUCT_ELEMENT_PTR);
    return (const Node_load_struct_elem_ptr*)node;
}

static inline Node_variable_def* node_unwrap_variable_def(Node* node) {
    assert(node->type == NODE_VARIABLE_DEFINITION);
    return (Node_variable_def*)node;
}

static inline const Node_variable_def* node_unwrap_variable_def_const(const Node* node) {
    assert(node->type == NODE_VARIABLE_DEFINITION);
    return (const Node_variable_def*)node;
}

static inline Node_struct_member_sym_piece_untyped* node_unwrap_struct_member_sym_piece_untyped(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return (Node_struct_member_sym_piece_untyped*)node;
}

static inline const Node_struct_member_sym_piece_untyped* node_unwrap_struct_member_sym_piece_untyped_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return (const Node_struct_member_sym_piece_untyped*)node;
}

static inline Node_struct_member_sym_piece_typed* node_unwrap_struct_member_sym_piece_typed(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return (Node_struct_member_sym_piece_typed*)node;
}

static inline const Node_struct_member_sym_piece_typed* node_unwrap_struct_member_sym_piece_typed_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return (const Node_struct_member_sym_piece_typed*)node;
}

static inline Node_struct_def* node_unwrap_struct_def(Node* node) {
    assert(node->type == NODE_STRUCT_DEFINITION);
    return (Node_struct_def*)node;
}

static inline const Node_struct_def* node_unwrap_struct_def_const(const Node* node) {
    assert(node->type == NODE_STRUCT_DEFINITION);
    return (const Node_struct_def*)node;
}

static inline Node_function_params* node_unwrap_function_params(Node* node) {
    assert(node->type == NODE_FUNCTION_PARAMETERS);
    return (Node_function_params*)node;
}

static inline const Node_function_params* node_unwrap_function_params_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_PARAMETERS);
    return (const Node_function_params*)node;
}

static inline Node_function_return_types* node_unwrap_function_return_types(Node* node) {
    assert(node->type == NODE_FUNCTION_RETURN_TYPES);
    return (Node_function_return_types*)node;
}

static inline const Node_function_return_types* node_unwrap_function_return_types_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_RETURN_TYPES);
    return (const Node_function_return_types*)node;
}

static inline Node_function_declaration* node_unwrap_function_declaration(Node* node) {
    assert(node->type == NODE_FUNCTION_DECLARATION);
    return (Node_function_declaration*)node;
}

static inline const Node_function_declaration* node_unwrap_function_declaration_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_DECLARATION);
    return (const Node_function_declaration*)node;
}

static inline Node_function_definition* node_unwrap_function_definition(Node* node) {
    assert(node->type == NODE_FUNCTION_DEFINITION);
    return (Node_function_definition*)node;
}

static inline const Node_function_definition* node_unwrap_function_definition_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_DEFINITION);
    return (const Node_function_definition*)node;
}

static inline Node_for_loop* node_unwrap_for_loop(Node* node) {
    assert(node->type == NODE_FOR_LOOP);
    return (Node_for_loop*)node;
}

static inline const Node_for_loop* node_unwrap_for_loop_const(const Node* node) {
    assert(node->type == NODE_FOR_LOOP);
    return (const Node_for_loop*)node;
}

static inline Node_for_variable_def* node_unwrap_for_variable_def(Node* node) {
    assert(node->type == NODE_FOR_VARIABLE_DEF);
    return (Node_for_variable_def*)node;
}

static inline const Node_for_variable_def* node_unwrap_for_variable_def_const(const Node* node) {
    assert(node->type == NODE_FOR_VARIABLE_DEF);
    return (const Node_for_variable_def*)node;
}

static inline Node_for_lower_bound* node_unwrap_for_lower_bound(Node* node) {
    assert(node->type == NODE_FOR_LOWER_BOUND);
    return (Node_for_lower_bound*)node;
}

static inline const Node_for_lower_bound* node_unwrap_for_lower_bound_const(const Node* node) {
    assert(node->type == NODE_FOR_LOWER_BOUND);
    return (const Node_for_lower_bound*)node;
}

static inline Node_for_upper_bound* node_unwrap_for_upper_bound(Node* node) {
    assert(node->type == NODE_FOR_UPPER_BOUND);
    return (Node_for_upper_bound*)node;
}

static inline const Node_for_upper_bound* node_unwrap_for_upper_bound_const(const Node* node) {
    assert(node->type == NODE_FOR_UPPER_BOUND);
    return (const Node_for_upper_bound*)node;
}

static inline Node_break* node_unwrap_break(Node* node) {
    assert(node->type == NODE_BREAK);
    return (Node_break*)node;
}

static inline const Node_break* node_unwrap_break_const(const Node* node) {
    assert(node->type == NODE_BREAK);
    return (const Node_break*)node;
}

static inline Node_return_statement* node_unwrap_return_statement(Node* node) {
    assert(node->type == NODE_RETURN_STATEMENT);
    return (Node_return_statement*)node;
}

static inline const Node_return_statement* node_unwrap_return_statement_const(const Node* node) {
    assert(node->type == NODE_RETURN_STATEMENT);
    return (const Node_return_statement*)node;
}

static inline Node_assignment* node_unwrap_assignment(Node* node) {
    assert(node->type == NODE_ASSIGNMENT);
    return (Node_assignment*)node;
}

static inline const Node_assignment* node_unwrap_assignment_const(const Node* node) {
    assert(node->type == NODE_ASSIGNMENT);
    return (const Node_assignment*)node;
}

static inline Node_if* node_unwrap_if(Node* node) {
    assert(node->type == NODE_IF_STATEMENT);
    return (Node_if*)node;
}

static inline const Node_if* node_unwrap_if_const(const Node* node) {
    assert(node->type == NODE_IF_STATEMENT);
    return (const Node_if*)node;
}

static inline Node_if_condition* node_unwrap_if_condition(Node* node) {
    assert(node->type == NODE_IF_CONDITION);
    return (Node_if_condition*)node;
}

static inline const Node_if_condition* node_unwrap_if_condition_const(const Node* node) {
    assert(node->type == NODE_IF_CONDITION);
    return (const Node_if_condition*)node;
}

static inline Node_block* node_unwrap_block(Node* node) {
    assert(node->type == NODE_BLOCK);
    return (Node_block*)node;
}

static inline const Node_block* node_unwrap_block_const(const Node* node) {
    assert(node->type == NODE_BLOCK);
    return (const Node_block*)node;
}

static inline Node_struct_member_sym_untyped* node_unwrap_struct_member_sym_untyped(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return (Node_struct_member_sym_untyped*)node;
}

static inline const Node_struct_member_sym_untyped* node_unwrap_struct_member_sym_untyped_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return (const Node_struct_member_sym_untyped*)node;
}

static inline Node_struct_member_sym_typed* node_wrap_struct_member_sym_typed(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return (Node_struct_member_sym_typed*)node;
}

static inline const Node_struct_member_sym_typed* node_wrap_struct_member_sym_typed_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return (const Node_struct_member_sym_typed*)node;
}

#define node_wrap(node) ((Node*)node)

#define node_wrap_const(node) ((const Node*)node)

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

#define node_print(root) str_view_print(node_print_internal(&print_arena, node_wrap(root)))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

#endif // NODE_H
