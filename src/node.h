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
    NODE_BREAK,
    NODE_IF_STATEMENT,
    NODE_IF_CONDITION,
    NODE_ASSIGNMENT,
    NODE_GOTO,
    NODE_COND_GOTO,
    NODE_LABEL,
    NODE_ALLOCA,
    NODE_LLVM_STORE_STRUCT_LITERAL,

    NODE_LOAD_ANOTHER_NODE,
    NODE_STORE_ANOTHER_NODE,
    NODE_LOAD_STRUCT_ELEMENT_PTR,
    NODE_PTR_BYVAL_SYM,
    NODE_LLVM_STORE_LITERAL,
    NODE_LLVM_REGISTER_SYM,
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
    struct Node_* child;
    Str_view str_data; // eg. "hello" in "let string1: String = "hello""
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Str_view name;
} Node_literal;

typedef struct {
    struct Node_* child;
    Str_view name;
    Llvm_id llvm_id;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_function_call;

typedef struct {
    struct Node_* child;
    Llvm_id llvm_id;
} Node_function_params;

typedef struct {
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
} Node_lang_type;

typedef struct {
    struct Node_* lhs;
    struct Node_* rhs;
    TOKEN_TYPE token_type;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
} Node_operator;

typedef struct {
    struct Node_* child;
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
    struct Node_* child;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
    Llvm_id llvm_id;
    Str_view name;
} Node_struct_def;

typedef struct {
    struct Node_* child;
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
    Node_variable_def* var_def;
    Node_for_lower_bound* lower_bound;
    Node_for_upper_bound* upper_bound;
    Node_block* body;
} Node_for_loop;

typedef struct {
    struct Node_* child;
} Node_return_statement;

typedef struct {
    struct Node_* child;
} Node_break;

typedef struct {
    struct Node_* lhs;
    struct Node_* rhs;
} Node_assignment;

typedef struct {
    struct Node_* child;
} Node_if;

typedef struct {
    struct Node_* child;
} Node_if_condition;

typedef struct {
    struct Node_* child;
    Str_view name;
    Lang_type lang_type;
} Node_struct_member_sym_untyped;

typedef struct {
    struct Node_* child;
    Lang_type lang_type; // eg. "String" in "let string1: String = "hello""
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
    Str_view name;
} Node_llvm_store_literal;

typedef struct {
    struct Node_* child;
    Str_view name;
    Llvm_id llvm_id;
} Node_goto;

typedef struct {
    struct Node_* child;
    Llvm_id llvm_id;
} Node_cond_goto;

typedef struct {
    struct Node_* child;
    Llvm_id llvm_id;
    Lang_type lang_type;
    struct Node_* node_dest;
    Str_view name;
} Node_llvm_store_struct_literal;

typedef struct {
    Lang_type lang_type;
    struct Node_* node_src;
    Llvm_id llvm_id;
} Node_ptr_byval_sym;

typedef union {
    Node_llvm_store_struct_literal node_llvm_store_struct_literal;
    Node_cond_goto node_cond_goto;
    Node_goto node_goto;
    Node_llvm_store_literal node_llvm_store_literal;
    Node_store_another_node node_store_another_node;
    Node_load_another_node node_load_another_node;
    Node_alloca node_alloca;
    Node_struct_member_sym_typed node_struct_member_sym_typed;
    Node_struct_member_sym_untyped node_struct_member_sym_untyped;
    Node_block node_block;
    Node_if_condition node_if_condition;
    Node_if node_if;
    Node_assignment node_assignment;
    Node_break node_break;
    Node_for_upper_bound node_for_upper_bound;
    Node_for_lower_bound node_for_lower_bound;
    Node_return_statement node_return_statement;
    Node_for_loop node_for_loop;
    Node_function_definition node_function_definition;
    Node_function_declaration node_function_declaration;
    Node_function_return_types node_function_return_types;
    Node_struct_def node_struct_def;
    Node_struct_member_sym_piece_typed node_struct_member_sym_piece_typed;
    Node_struct_member_sym_piece_untyped node_struct_member_sym_piece_untyped;
    Node_variable_def node_variable_def;
    Node_struct_literal node_struct_literal;
    Node_operator node_operator;
    Node_lang_type node_lang_type;
    Node_function_params node_function_params;
    Node_function_call node_function_call;
    Node_literal node_literal;
    Node_load_element_ptr node_load_element_ptr;
    Node_label node_label;
    Node_symbol_typed node_symbol_typed;
    Node_symbol_untyped node_symbol_untyped;
    Lang_type lang_type;
    Node_struct_member_sym_piece_untyped memb_sym_piece_untyped;
    Node_struct_member_sym_piece_typed memb_sym_piece_typed;
    Node_ptr_byval_sym node_ptr_byval_sym;
    Node_llvm_register_sym node_llvm_register_sym;
} Node_as;

typedef struct Node_ {
    Node_as as;
    Pos pos;
    NODE_TYPE type;
    //Str_view name; // eg. "string1" in "let string1: String = "hello""

    struct Node_* next;
    struct Node_* prev;
    struct Node_* parent;
} Node;

static inline Node_symbol_untyped* node_unwrap_symbol_untyped(Node* node) {
    assert(node->type == NODE_SYMBOL_UNTYPED);
    return &node->as.node_symbol_untyped;
}

static inline const Node_symbol_untyped* node_unwrap_symbol_untyped_const(const Node* node) {
    assert(node->type == NODE_SYMBOL_UNTYPED);
    return &node->as.node_symbol_untyped;
}

static inline Node_symbol_typed* node_unwrap_symbol_typed(Node* node) {
    assert(node->type == NODE_SYMBOL_TYPED);
    return &node->as.node_symbol_typed;
}

static inline const Node_symbol_typed* node_unwrap_symbol_typed_const(const Node* node) {
    assert(node->type == NODE_SYMBOL_TYPED);
    return &node->as.node_symbol_typed;
}

static inline Node_label* node_unwrap_label(Node* node) {
    assert(node->type == NODE_LABEL);
    return &node->as.node_label;
}

static inline const Node_label* node_unwrap_label_const(const Node* node) {
    assert(node->type == NODE_LABEL);
    return &node->as.node_label;
}

static inline Node_literal* node_unwrap_literal(Node* node) {
    assert(node->type == NODE_LITERAL);
    return &node->as.node_literal;
}

static inline const Node_literal* node_unwrap_literal_const(const Node* node) {
    assert(node->type == NODE_LITERAL);
    return &node->as.node_literal;
}

static inline Node_function_call* node_unwrap_function_call(Node* node) {
    assert(node->type == NODE_FUNCTION_CALL);
    return &node->as.node_function_call;
}

static inline const Node_function_call* node_unwrap_function_call_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_CALL);
    return &node->as.node_function_call;
}

static inline Node_lang_type* node_unwrap_lang_type(Node* node) {
    assert(node->type == NODE_LANG_TYPE);
    return &node->as.node_lang_type;
}

static inline const Node_lang_type* node_unwrap_lang_type_const(const Node* node) {
    assert(node->type == NODE_LANG_TYPE);
    return &node->as.node_lang_type;
}

static inline Node_operator* node_unwrap_operator(Node* node) {
    assert(node->type == NODE_OPERATOR);
    return &node->as.node_operator;
}

static inline const Node_operator* node_unwrap_operator_const(const Node* node) {
    assert(node->type == NODE_OPERATOR);
    return &node->as.node_operator;
}

static inline Node_struct_literal* node_unwrap_struct_literal(Node* node) {
    assert(node->type == NODE_STRUCT_LITERAL);
    return &node->as.node_struct_literal;
}

static inline const Node_struct_literal* node_unwrap_struct_literal_const(const Node* node) {
    assert(node->type == NODE_STRUCT_LITERAL);
    return &node->as.node_struct_literal;
}

static inline Node_load_element_ptr* node_unwrap_load_elem_ptr(Node* node) {
    assert(node->type == NODE_LOAD_STRUCT_ELEMENT_PTR);
    return &node->as.node_load_element_ptr;
}

static inline const Node_load_element_ptr* node_unwrap_load_elem_ptr_const(const Node* node) {
    assert(node->type == NODE_LOAD_STRUCT_ELEMENT_PTR);
    return &node->as.node_load_element_ptr;
}

static inline Node_variable_def* node_unwrap_variable_def(Node* node) {
    assert(node->type == NODE_VARIABLE_DEFINITION);
    return &node->as.node_variable_def;
}

static inline const Node_variable_def* node_unwrap_variable_def_const(const Node* node) {
    assert(node->type == NODE_VARIABLE_DEFINITION);
    return &node->as.node_variable_def;
}

static inline Node_struct_member_sym_piece_untyped* node_unwrap_struct_member_sym_piece_untyped(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED);
    return &node->as.node_struct_member_sym_piece_untyped;
}

static inline const Node_struct_member_sym_piece_untyped* node_unwrap_struct_member_sym_piece_untyped_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED);
    return &node->as.node_struct_member_sym_piece_untyped;
}

static inline Node_struct_member_sym_piece_typed* node_unwrap_struct_member_sym_piece_typed(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED);
    return &node->as.node_struct_member_sym_piece_typed;
}

static inline const Node_struct_member_sym_piece_typed* node_unwrap_struct_member_sym_piece_typed_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED);
    return &node->as.node_struct_member_sym_piece_typed;
}

static inline Node_struct_def* node_unwrap_struct_def(Node* node) {
    assert(node->type == NODE_STRUCT_DEFINITION);
    return &node->as.node_struct_def;
}

static inline const Node_struct_def* node_unwrap_struct_def_const(const Node* node) {
    assert(node->type == NODE_STRUCT_DEFINITION);
    return &node->as.node_struct_def;
}

static inline Node_function_params* node_unwrap_function_params(Node* node) {
    assert(node->type == NODE_FUNCTION_PARAMETERS);
    return &node->as.node_function_params;
}

static inline const Node_function_params* node_unwrap_function_params_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_PARAMETERS);
    return &node->as.node_function_params;
}

static inline Node_function_return_types* node_unwrap_function_return_types(Node* node) {
    assert(node->type == NODE_FUNCTION_RETURN_TYPES);
    return &node->as.node_function_return_types;
}

static inline const Node_function_return_types* node_unwrap_function_return_types_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_RETURN_TYPES);
    return &node->as.node_function_return_types;
}

static inline Node_function_declaration* node_unwrap_function_declaration(Node* node) {
    assert(node->type == NODE_FUNCTION_DECLARATION);
    return &node->as.node_function_declaration;
}

static inline const Node_function_declaration* node_unwrap_function_declaration_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_DECLARATION);
    return &node->as.node_function_declaration;
}

static inline Node_function_definition* node_unwrap_function_definition(Node* node) {
    assert(node->type == NODE_FUNCTION_DEFINITION);
    return &node->as.node_function_definition;
}

static inline const Node_function_definition* node_unwrap_function_definition_const(const Node* node) {
    assert(node->type == NODE_FUNCTION_DEFINITION);
    return &node->as.node_function_definition;
}

static inline Node_for_loop* node_unwrap_for_loop(Node* node) {
    assert(node->type == NODE_FOR_LOOP);
    return &node->as.node_for_loop;
}

static inline const Node_for_loop* node_unwrap_for_loop_const(const Node* node) {
    assert(node->type == NODE_FOR_LOOP);
    return &node->as.node_for_loop;
}

static inline Node_for_lower_bound* node_unwrap_for_lower_bound(Node* node) {
    assert(node->type == NODE_FOR_LOWER_BOUND);
    return &node->as.node_for_lower_bound;
}

static inline const Node_for_lower_bound* node_unwrap_for_lower_bound_const(const Node* node) {
    assert(node->type == NODE_FOR_LOWER_BOUND);
    return &node->as.node_for_lower_bound;
}

static inline Node_for_upper_bound* node_unwrap_for_upper_bound(Node* node) {
    assert(node->type == NODE_FOR_UPPER_BOUND);
    return &node->as.node_for_upper_bound;
}

static inline const Node_for_upper_bound* node_unwrap_for_upper_bound_const(const Node* node) {
    assert(node->type == NODE_FOR_UPPER_BOUND);
    return &node->as.node_for_upper_bound;
}

static inline Node_break* node_unwrap_break(Node* node) {
    assert(node->type == NODE_BREAK);
    return &node->as.node_break;
}

static inline const Node_break* node_unwrap_break_const(const Node* node) {
    assert(node->type == NODE_BREAK);
    return &node->as.node_break;
}

static inline Node_return_statement* node_unwrap_return_statement(Node* node) {
    assert(node->type == NODE_RETURN_STATEMENT);
    return &node->as.node_return_statement;
}

static inline const Node_return_statement* node_unwrap_return_statement_const(const Node* node) {
    assert(node->type == NODE_RETURN_STATEMENT);
    return &node->as.node_return_statement;
}

static inline Node_assignment* node_unwrap_assignment(Node* node) {
    assert(node->type == NODE_ASSIGNMENT);
    return &node->as.node_assignment;
}

static inline const Node_assignment* node_unwrap_assignment_const(const Node* node) {
    assert(node->type == NODE_ASSIGNMENT);
    return &node->as.node_assignment;
}

static inline Node_if* node_unwrap_if(Node* node) {
    assert(node->type == NODE_IF_STATEMENT);
    return &node->as.node_if;
}

static inline const Node_if* node_unwrap_if_const(const Node* node) {
    assert(node->type == NODE_IF_STATEMENT);
    return &node->as.node_if;
}

static inline Node_if_condition* node_unwrap_if_condition(Node* node) {
    assert(node->type == NODE_IF_CONDITION);
    return &node->as.node_if_condition;
}

static inline const Node_if_condition* node_unwrap_if_condition_const(const Node* node) {
    assert(node->type == NODE_IF_CONDITION);
    return &node->as.node_if_condition;
}

static inline Node_block* node_unwrap_block(Node* node) {
    assert(node->type == NODE_BLOCK);
    return &node->as.node_block;
}

static inline const Node_block* node_unwrap_block_const(const Node* node) {
    assert(node->type == NODE_BLOCK);
    return &node->as.node_block;
}

static inline Node_struct_member_sym_untyped* node_unwrap_struct_member_sym_untyped(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return &node->as.node_struct_member_sym_untyped;
}

static inline const Node_struct_member_sym_untyped* node_unwrap_struct_member_sym_untyped_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_UNTYPED);
    return &node->as.node_struct_member_sym_untyped;
}

static inline Node_struct_member_sym_typed* node_unwrap_struct_member_sym_typed(Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return &node->as.node_struct_member_sym_typed;
}

static inline const Node_struct_member_sym_typed* node_unwrap_struct_member_sym_typed_const(const Node* node) {
    assert(node->type == NODE_STRUCT_MEMBER_SYM_TYPED);
    return &node->as.node_struct_member_sym_typed;
}

static inline Node_cond_goto* node_unwrap_cond_goto(Node* node) {
    assert(node->type == NODE_COND_GOTO);
    return &node->as.node_cond_goto;
}

static inline const Node_cond_goto* node_unwrap_cond_goto_const(const Node* node) {
    assert(node->type == NODE_COND_GOTO);
    return &node->as.node_cond_goto;
}

static inline Node_goto* node_unwrap_goto(Node* node) {
    assert(node->type == NODE_GOTO);
    return &node->as.node_goto;
}

static inline const Node_goto* node_unwrap_goto_const(const Node* node) {
    assert(node->type == NODE_GOTO);
    return &node->as.node_goto;
}

static inline Node_alloca* node_unwrap_alloca(Node* node) {
    assert(node->type == NODE_ALLOCA);
    return &node->as.node_alloca;
}

static inline const Node_alloca* node_unwrap_alloca_const(const Node* node) {
    assert(node->type == NODE_ALLOCA);
    return &node->as.node_alloca;
}

static inline const Node_llvm_store_literal* node_unwrap_llvm_store_literal_const(const Node* node) {
    assert(node->type == NODE_LLVM_STORE_LITERAL);
    return &node->as.node_llvm_store_literal;
}

static inline Node_llvm_store_literal* node_unwrap_llvm_store_literal(Node* node) {
    assert(node->type == NODE_LLVM_STORE_LITERAL);
    return &node->as.node_llvm_store_literal;
}

static inline const Node_load_another_node* node_unwrap_load_another_node_const(const Node* node) {
    assert(node->type == NODE_LOAD_ANOTHER_NODE);
    return &node->as.node_load_another_node;
}

static inline Node_load_another_node* node_unwrap_load_another_node(Node* node) {
    assert(node->type == NODE_LOAD_ANOTHER_NODE);
    return &node->as.node_load_another_node;
}

static inline const Node_store_another_node* node_unwrap_store_another_node_const(const Node* node) {
    assert(node->type == NODE_STORE_ANOTHER_NODE);
    return &node->as.node_store_another_node;
}

static inline Node_store_another_node* node_unwrap_store_another_node(Node* node) {
    assert(node->type == NODE_STORE_ANOTHER_NODE);
    return &node->as.node_store_another_node;
}

static inline const Node_llvm_store_struct_literal* node_unwrap_llvm_store_struct_literal_const(const Node* node) {
    assert(node->type == NODE_LLVM_STORE_STRUCT_LITERAL);
    return &node->as.node_llvm_store_struct_literal;
}

static inline Node_llvm_store_struct_literal* node_unwrap_llvm_store_struct_literal(Node* node) {
    assert(node->type == NODE_LLVM_STORE_STRUCT_LITERAL);
    return &node->as.node_llvm_store_struct_literal;
}

static inline Node_ptr_byval_sym* node_unwrap_ptr_byval_sym(Node* node) {
    assert(node->type == NODE_PTR_BYVAL_SYM);
    return &node->as.node_ptr_byval_sym;
}

static inline const Node_ptr_byval_sym* node_unwrap_ptr_byval_sym_const(const Node* node) {
    assert(node->type == NODE_PTR_BYVAL_SYM);
    return &node->as.node_ptr_byval_sym;
}

static inline Node_llvm_register_sym* node_unwrap_llvm_register_sym(Node* node) {
    assert(node->type == NODE_LLVM_REGISTER_SYM);
    return &node->as.node_llvm_register_sym;
}

static inline const Node_llvm_register_sym* node_unwrap_llvm_register_sym_const(const Node* node) {
    assert(node->type == NODE_LLVM_REGISTER_SYM);
    return &node->as.node_llvm_register_sym;
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
