#ifndef NODE_UTIL_H
#define NODE_UTIL_H

static inline Llvm_id get_llvm_id(const Node* node) {
    switch (node->type) {
        case NODE_NO_TYPE:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            return node_unwrap_struct_def_const(node)->llvm_id;
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEFINITION:
            return node_unwrap_function_definition_const(node)->llvm_id;
        case NODE_FUNCTION_PARAMETERS:
            unreachable("");
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("");
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(node)->llvm_id;
        case NODE_LITERAL:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_OPERATOR: {
            const Node_operator* operator = node_unwrap_operation_const(node);
            if (operator->type == NODE_OP_UNARY) {
                return node_unwrap_op_unary_const(operator)->llvm_id;
            } else if (operator->type == NODE_OP_BINARY) {
                return node_unwrap_op_binary_const(operator)->llvm_id;
            } else {
                unreachable("");
            }
        }
        break;
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_RETURN_STATEMENT:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            return node_unwrap_variable_def_const(node)->llvm_id;
        case NODE_FUNCTION_DECLARATION:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_CONDITION:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF_STATEMENT:
            unreachable("");
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_COND_GOTO:
            unreachable("");
        case NODE_LABEL:
            unreachable("");
        case NODE_ALLOCA:
            return node_unwrap_alloca_const(node)->llvm_id;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            unreachable("");
        case NODE_LOAD_ANOTHER_NODE:
            return node_unwrap_load_another_node_const(node)->llvm_id;
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            return node_unwrap_load_elem_ptr_const(node)->llvm_id;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        default:
            unreachable(""); // we cannot print node_type because it will cause infinite recursion
    }
}

static inline Lang_type get_lang_type(const Node* node) {
    switch (node->type) {
        case NODE_NO_TYPE:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            unreachable("");
        case NODE_STRUCT_LITERAL:
            return node_unwrap_struct_literal_const(node)->lang_type;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            return node_unwrap_struct_member_sym_untyped_const(node)->lang_type;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            return node_unwrap_struct_member_sym_typed_const(node)->lang_type;
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED does not have lang_type");
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            return node_unwrap_struct_member_sym_piece_typed_const(node)->lang_type;
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEFINITION:
            unreachable("");
        case NODE_FUNCTION_PARAMETERS:
            unreachable("");
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("");
        case NODE_FUNCTION_CALL:
            unreachable("");
        case NODE_LITERAL:
            return node_unwrap_literal_const(node)->lang_type;
        case NODE_LANG_TYPE:
            return node_unwrap_lang_type_const(node)->lang_type;
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            return node_unwrap_symbol_typed_const(node)->lang_type;
        case NODE_RETURN_STATEMENT:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            return node_unwrap_variable_def_const(node)->lang_type;
        case NODE_FUNCTION_DECLARATION:
            return node_unwrap_function_declaration_const(node)->return_types->child->lang_type;
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_CONDITION:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF_STATEMENT:
            unreachable("");
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_COND_GOTO:
            unreachable("");
        case NODE_LABEL:
            unreachable("");
        case NODE_ALLOCA:
            return node_unwrap_alloca_const(node)->lang_type;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            return node_unwrap_llvm_store_struct_literal_const(node)->lang_type;
        case NODE_LOAD_ANOTHER_NODE:
            return node_unwrap_load_another_node_const(node)->lang_type;
        case NODE_STORE_ANOTHER_NODE:
            return node_unwrap_store_another_node_const(node)->lang_type;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            return node_unwrap_load_elem_ptr_const(node)->lang_type;
        case NODE_LLVM_STORE_LITERAL:
            return node_unwrap_llvm_store_literal_const(node)->lang_type;
        case NODE_PTR_BYVAL_SYM:
            return node_unwrap_ptr_byval_sym_const(node)->lang_type;
        case NODE_LLVM_REGISTER_SYM:
            return node_unwrap_llvm_register_sym_const(node)->lang_type;
        default:
            unreachable(""); // we cannot print node_type because it will cause infinite recursion
    }
}

static inline Node* get_node_src(Node* node) {
    switch (node->type) {
        case NODE_NO_TYPE:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            unreachable("");
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEFINITION:
            unreachable("");
        case NODE_FUNCTION_PARAMETERS:
            unreachable("");
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("");
        case NODE_FUNCTION_CALL:
            unreachable("");
        case NODE_LITERAL:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_RETURN_STATEMENT:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            unreachable("");
        case NODE_FUNCTION_DECLARATION:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_CONDITION:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF_STATEMENT:
            unreachable("");
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_COND_GOTO:
            return node_wrap(node_unwrap_cond_goto_const(node)->node_src);
        case NODE_LABEL:
            unreachable("");
        case NODE_ALLOCA:
            unreachable("");
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            unreachable("");
        case NODE_LOAD_ANOTHER_NODE:
            return node_unwrap_load_another_node_const(node)->node_src;
        case NODE_STORE_ANOTHER_NODE:
            return node_unwrap_store_another_node_const(node)->node_src;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            return node_unwrap_load_elem_ptr_const(node)->node_src;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            return node_unwrap_ptr_byval_sym(node)->node_src;
        case NODE_LLVM_REGISTER_SYM:
            return node_unwrap_llvm_register_sym_const(node)->node_src;
        default:
            unreachable(""); // we cannot print node_type because it will cause infinite recursion
    }
}

static inline Node* get_node_dest(Node* node) {
    switch (node->type) {
        case NODE_NO_TYPE:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            unreachable("");
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEFINITION:
            unreachable("");
        case NODE_FUNCTION_PARAMETERS:
            unreachable("");
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("");
        case NODE_FUNCTION_CALL:
            unreachable("");
        case NODE_LITERAL:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_RETURN_STATEMENT:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            unreachable("");
        case NODE_FUNCTION_DECLARATION:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_CONDITION:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF_STATEMENT:
            unreachable("");
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_COND_GOTO:
            unreachable("");
        case NODE_LABEL:
            unreachable("");
        case NODE_ALLOCA:
            unreachable("");
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            return node_unwrap_llvm_store_struct_literal_const(node)->node_dest;
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
        case NODE_STORE_ANOTHER_NODE:
            return node_unwrap_store_another_node_const(node)->node_dest;
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            unreachable("");
        case NODE_LLVM_STORE_LITERAL:
            return node_unwrap_llvm_store_literal_const(node)->node_dest;
        default:
            unreachable(""); // we cannot print node_type because it will cause infinite recursion
    }
}

static inline Str_view get_node_name(const Node* node) {
    switch (node->type) {
        case NODE_NO_TYPE:
            unreachable("");
        case NODE_STRUCT_DEFINITION:
            return node_unwrap_struct_def_const(node)->name;
        case NODE_STRUCT_LITERAL:
            return node_unwrap_struct_literal_const(node)->name;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            return node_unwrap_struct_member_sym_untyped_const(node)->name;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            return node_unwrap_struct_member_sym_typed_const(node)->name;
        case NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED:
            return node_unwrap_struct_member_sym_piece_untyped_const(node)->name;
        case NODE_STRUCT_MEMBER_SYM_PIECE_TYPED:
            return node_unwrap_struct_member_sym_piece_typed_const(node)->name;
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEFINITION:
            if (!node_unwrap_function_definition_const(node)->declaration) {
                unreachable("");
            }
            return node_unwrap_function_definition_const(node)->declaration->name;
        case NODE_FUNCTION_PARAMETERS:
            unreachable("");
        case NODE_FUNCTION_RETURN_TYPES:
            unreachable("");
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(node)->name;
        case NODE_LITERAL:
            return node_unwrap_literal_const(node)->name;
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            return node_unwrap_symbol_untyped_const(node)->name;
        case NODE_SYMBOL_TYPED:
            return node_unwrap_symbol_typed_const(node)->name;
        case NODE_RETURN_STATEMENT:
            unreachable("");
        case NODE_VARIABLE_DEFINITION:
            return node_unwrap_variable_def_const(node)->name;
        case NODE_FUNCTION_DECLARATION:
            return node_unwrap_function_declaration_const(node)->name;
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_CONDITION:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF_STATEMENT:
            unreachable("");
        case NODE_IF_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            return node_unwrap_goto_const(node)->name;
        case NODE_COND_GOTO:
            unreachable("");
        case NODE_LABEL:
            return node_unwrap_label_const(node)->name;
        case NODE_ALLOCA:
            return node_unwrap_alloca_const(node)->name;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            unreachable("");
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
        case NODE_LOAD_STRUCT_ELEMENT_PTR:
            return node_unwrap_load_elem_ptr_const(node)->name;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        case NODE_LLVM_REGISTER_SYM:
            unreachable("");
        case NODE_OPERATOR:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            unreachable("");
    }
}

static inline const Node* get_node_src_const(const Node* node) {
    return get_node_src((Node*)node);
}

static inline const Node* get_node_dest_const(const Node* node) {
    return get_node_dest((Node*)node);
}

#endif // NODE_UTIL_H
