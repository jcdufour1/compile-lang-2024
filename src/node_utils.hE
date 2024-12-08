#ifndef NODE_UTIL_H
#define NODE_UTIL_H

#define LANG_TYPE_FMT STR_VIEW_FMT

void extend_lang_type_to_string(
    Arena* arena,
    String* string,
    Lang_type lang_type,
    bool surround_in_lt_gt
);

static inline Lang_type lang_type_from_strv(Str_view str_view, int16_t pointer_depth) {
    Lang_type Lang_type = {.str = str_view, .pointer_depth = pointer_depth};
    assert(str_view.count < 1e9);
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

#define node_print(root) str_view_print(node_print_internal(&print_arena, root))

#define node_printf(node) \
    do { \
        log(LOG_NOTE, NODE_FMT"\n", node_print(node)); \
    } while (0);

static inline Lang_type get_operator_lang_type(const Node_operator* operator) {
    if (operator->type == NODE_UNARY) {
        return node_unwrap_unary_const(operator)->lang_type;
    } else if (operator->type == NODE_BINARY) {
        return node_unwrap_binary_const(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type* get_operator_lang_type_ref(Node_operator* operator) {
    if (operator->type == NODE_UNARY) {
        return &node_unwrap_unary(operator)->lang_type;
    } else if (operator->type == NODE_BINARY) {
        return &node_unwrap_binary(operator)->lang_type;
    } else {
        unreachable("");
    }
}

static inline Lang_type get_member_sym_piece_final_lang_type(const Node_member_sym_typed* struct_memb_sym) {
    Lang_type lang_type = {0};
    for (size_t idx = 0; idx < struct_memb_sym->children.info.count; idx++) {
        const Node* memb_piece_ = vec_at(&struct_memb_sym->children, idx);
        if (memb_piece_->type != NODE_MEMBER_SYM_PIECE_TYPED) {
            unreachable("invalid node is in here");
        }
        const Node_member_sym_piece_typed* memb_piece = 
            node_unwrap_member_sym_piece_typed_const(memb_piece_);
        lang_type = memb_piece->lang_type;
    }
    assert(lang_type.str.count > 0);
    return lang_type;
}

static inline Llvm_id get_llvm_id(const Node* node);

static inline Llvm_id get_llvm_id_expr(const Node_expr* expr) {
    switch (expr->type) {
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_LITERAL:
            unreachable("");
        case NODE_OPERATOR: {
            const Node_operator* operator = node_unwrap_operator_const(expr);
            if (operator->type == NODE_UNARY) {
                return node_unwrap_unary_const(operator)->llvm_id;
            } else if (operator->type == NODE_BINARY) {
                return node_unwrap_binary_const(operator)->llvm_id;
            } else {
                unreachable("");
            }
        }
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(expr)->llvm_id;
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_LLVM_PLACEHOLDER:
            return get_llvm_id(node_unwrap_llvm_placeholder_const(expr)->llvm_reg.node);
    }
    unreachable("");
}

static inline Llvm_id get_llvm_id(const Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_llvm_id_expr(node_unwrap_expr_const(node));
        case NODE_STRUCT_DEF:
            return node_unwrap_struct_def_const(node)->base.llvm_id;
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            return node_unwrap_function_def_const(node)->llvm_id;
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        break;
        case NODE_RETURN:
            unreachable("");
        case NODE_VARIABLE_DEF:
            return node_unwrap_variable_def_const(node)->llvm_id;
        case NODE_FUNCTION_DECL:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
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
        case NODE_LOAD_ELEMENT_PTR:
            return node_unwrap_load_element_ptr_const(node)->llvm_id;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        default:
            unreachable(""); // we cannot print node_type because it will cause infinite recursion
    }
}

static inline Lang_type get_lang_type_expr(const Node_expr* expr) {
    switch (expr->type) {
        case NODE_STRUCT_LITERAL:
            return node_unwrap_struct_literal_const(expr)->lang_type;
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(expr)->lang_type;
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            return get_member_sym_piece_final_lang_type(node_unwrap_member_sym_typed_const(expr));
        case NODE_LITERAL:
            return node_unwrap_literal_const(expr)->lang_type;
        case NODE_OPERATOR:
            return get_operator_lang_type(node_unwrap_operator_const(expr));
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            return node_unwrap_symbol_typed_const(expr)->lang_type;
        case NODE_LLVM_PLACEHOLDER:
            return node_unwrap_llvm_placeholder_const(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type* get_lang_type_expr_ref(Node_expr* expr) {
    switch (expr->type) {
        case NODE_STRUCT_LITERAL:
            return &node_unwrap_struct_literal(expr)->lang_type;
        case NODE_FUNCTION_CALL:
            return &node_unwrap_function_call(expr)->lang_type;
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_LITERAL:
            return &node_unwrap_literal(expr)->lang_type;
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            return &node_unwrap_symbol_typed(expr)->lang_type;
        case NODE_OPERATOR:
            return get_operator_lang_type_ref(node_unwrap_operator(expr));
        case NODE_LLVM_PLACEHOLDER:
            return &node_unwrap_llvm_placeholder(expr)->lang_type;
    }
    unreachable("");
}

static inline Lang_type get_lang_type(const Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_lang_type_expr(node_unwrap_expr_const(node));
        case NODE_STRUCT_DEF:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("NODE_MEMBER_SYM_PIECE_UNTYPED does not have lang_type");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            return node_unwrap_member_sym_piece_typed_const(node)->lang_type;
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            unreachable("");
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            return node_unwrap_lang_type_const(node)->lang_type;
        case NODE_RETURN:
            return get_lang_type_expr(node_unwrap_return_const(node)->child);
        case NODE_VARIABLE_DEF:
            return node_unwrap_variable_def_const(node)->lang_type;
        case NODE_FUNCTION_DECL:
            return node_unwrap_function_decl_const(node)->return_type->lang_type;
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
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
        case NODE_LOAD_ELEMENT_PTR:
            return node_unwrap_load_element_ptr_const(node)->lang_type;
        case NODE_LLVM_STORE_LITERAL:
            return node_unwrap_llvm_store_literal_const(node)->lang_type;
        case NODE_PTR_BYVAL_SYM:
            return node_unwrap_ptr_byval_sym_const(node)->lang_type;
        case NODE_RAW_UNION_DEF:
            unreachable("");
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_ENUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Lang_type* get_lang_type_ref(Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            todo();
        case NODE_STRUCT_DEF:
            unreachable("");
        case NODE_RAW_UNION_DEF:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("NODE_MEMBER_SYM_PIECE_UNTYPED does not have lang_type");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            return &node_unwrap_member_sym_piece_typed(node)->lang_type;
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            unreachable("");
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            return &node_unwrap_lang_type(node)->lang_type;
        case NODE_RETURN:
            return get_lang_type_expr_ref(node_unwrap_return(node)->child);
        case NODE_VARIABLE_DEF:
            return &node_unwrap_variable_def(node)->lang_type;
        case NODE_FUNCTION_DECL:
            return &node_unwrap_function_decl(node)->return_type->lang_type;
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
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
            return &node_unwrap_alloca(node)->lang_type;
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            return &node_unwrap_llvm_store_struct_literal(node)->lang_type;
        case NODE_LOAD_ANOTHER_NODE:
            return &node_unwrap_load_another_node(node)->lang_type;
        case NODE_STORE_ANOTHER_NODE:
            return &node_unwrap_store_another_node(node)->lang_type;
        case NODE_LOAD_ELEMENT_PTR:
            return &node_unwrap_load_element_ptr(node)->lang_type;
        case NODE_LLVM_STORE_LITERAL:
            return &node_unwrap_llvm_store_literal(node)->lang_type;
        case NODE_PTR_BYVAL_SYM:
            return &node_unwrap_ptr_byval_sym(node)->lang_type;
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_ENUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Node* get_expr_src(Node_expr* expr) {
    switch (expr->type) {
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_LITERAL:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_FUNCTION_CALL:
            unreachable("");
        case NODE_OPERATOR:
            unreachable("");
        case NODE_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Node* get_node_src(Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_expr_src(node_unwrap_expr(node));
        case NODE_STRUCT_DEF:
            unreachable("");
        case NODE_RAW_UNION_DEF:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            unreachable("");
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_RETURN:
            unreachable("");
        case NODE_VARIABLE_DEF:
            unreachable("");
        case NODE_FUNCTION_DECL:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
            unreachable("");
        case NODE_ASSIGNMENT:
            unreachable("");
        case NODE_GOTO:
            unreachable("");
        case NODE_COND_GOTO:
            return node_wrap_expr(node_wrap_operator(node_unwrap_cond_goto(node)->node_src));
        case NODE_LABEL:
            unreachable("");
        case NODE_ALLOCA:
            unreachable("");
        case NODE_LLVM_STORE_STRUCT_LITERAL:
            unreachable("");
        case NODE_LOAD_ANOTHER_NODE:
            return node_unwrap_load_another_node(node)->node_src.node;
        case NODE_STORE_ANOTHER_NODE:
            return node_unwrap_store_another_node(node)->node_src.node;
        case NODE_LOAD_ELEMENT_PTR:
            return node_unwrap_load_element_ptr(node)->node_src.node;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            return node_unwrap_ptr_byval_sym(node)->node_src.node;
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_ENUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Node* get_expr_dest(Node_expr* expr) {
    switch (expr->type) {
        case NODE_STRUCT_LITERAL:
            unreachable("");
        case NODE_MEMBER_SYM_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_TYPED:
            unreachable("");
        case NODE_LITERAL:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("");
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_FUNCTION_CALL:
            unreachable("");
        case NODE_OPERATOR:
            unreachable("");
        case NODE_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Node* get_node_dest(Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_expr_dest(node_unwrap_expr(node));
        case NODE_STRUCT_DEF:
            unreachable("");
        case NODE_RAW_UNION_DEF:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            unreachable("");
        case NODE_MEMBER_SYM_PIECE_TYPED:
            unreachable("");
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            unreachable("");
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_RETURN:
            unreachable("");
        case NODE_VARIABLE_DEF:
            unreachable("");
        case NODE_FUNCTION_DECL:
            unreachable("");
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
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
            return node_unwrap_llvm_store_struct_literal_const(node)->node_dest.node;
        case NODE_LOAD_ANOTHER_NODE:
            unreachable("");
        case NODE_STORE_ANOTHER_NODE:
            return node_unwrap_store_another_node_const(node)->node_dest.node;
        case NODE_LOAD_ELEMENT_PTR:
            unreachable("");
        case NODE_LLVM_STORE_LITERAL:
            return node_unwrap_llvm_store_literal_const(node)->node_dest.node;
        case NODE_PTR_BYVAL_SYM:
            unreachable("");
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_ENUM_DEF:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view get_expr_name(const Node_expr* expr) {
    switch (expr->type) {
        case NODE_OPERATOR:
            unreachable("");
        case NODE_STRUCT_LITERAL:
            return node_unwrap_struct_literal_const(expr)->name;
        case NODE_MEMBER_SYM_UNTYPED:
            return node_unwrap_member_sym_untyped_const(expr)->name;
        case NODE_MEMBER_SYM_TYPED:
            return node_unwrap_member_sym_typed_const(expr)->name;
        case NODE_SYMBOL_UNTYPED:
            return node_unwrap_symbol_untyped_const(expr)->name;
        case NODE_SYMBOL_TYPED:
            return node_unwrap_symbol_typed_const(expr)->name;
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(expr)->name;
        case NODE_LITERAL:
            return node_unwrap_literal_const(expr)->name;
        case NODE_LLVM_PLACEHOLDER:
            unreachable("");
    }
    unreachable("");
}

static inline Str_view get_node_name_expr(const Node_expr* expr) {
    switch (expr->type) {
        case NODE_SYMBOL_TYPED:
            return node_unwrap_symbol_typed_const(expr)->name;
        case NODE_FUNCTION_CALL:
            return node_unwrap_function_call_const(expr)->name;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(expr)));
    }
    unreachable("");
}

static inline Str_view get_node_name(const Node* node) {
    switch (node->type) {
        case NODE_EXPR:
            return get_expr_name(node_unwrap_expr_const(node));
        case NODE_STRUCT_DEF:
            return node_unwrap_struct_def_const(node)->base.name;
        case NODE_RAW_UNION_DEF:
            return node_unwrap_raw_union_def_const(node)->base.name;
        case NODE_MEMBER_SYM_PIECE_UNTYPED:
            return node_unwrap_member_sym_piece_untyped_const(node)->name;
        case NODE_MEMBER_SYM_PIECE_TYPED:
            return node_unwrap_member_sym_piece_typed_const(node)->name;
        case NODE_BLOCK:
            unreachable("");
        case NODE_FUNCTION_DEF:
            if (!node_unwrap_function_def_const(node)->declaration) {
                unreachable("");
            }
            return node_unwrap_function_def_const(node)->declaration->name;
        case NODE_FUNCTION_PARAMS:
            unreachable("");
        case NODE_LANG_TYPE:
            unreachable("");
        case NODE_RETURN:
            unreachable("");
        case NODE_VARIABLE_DEF:
            return node_unwrap_variable_def_const(node)->name;
        case NODE_FUNCTION_DECL:
            return node_unwrap_function_decl_const(node)->name;
        case NODE_FOR_RANGE:
            unreachable("");
        case NODE_FOR_WITH_COND:
            unreachable("");
        case NODE_FOR_LOWER_BOUND:
            unreachable("");
        case NODE_FOR_UPPER_BOUND:
            unreachable("");
        case NODE_BREAK:
            unreachable("");
        case NODE_IF:
            unreachable("");
        case NODE_CONDITION:
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
            return get_node_name(node_unwrap_load_another_node_const(node)->node_src.node);
        case NODE_STORE_ANOTHER_NODE:
            unreachable("");
        case NODE_LOAD_ELEMENT_PTR:
            return node_unwrap_load_element_ptr_const(node)->name;
        case NODE_LLVM_STORE_LITERAL:
            unreachable("");
        case NODE_PTR_BYVAL_SYM:
            unreachable("");
        case NODE_IF_ELSE_CHAIN:
            unreachable("");
        case NODE_ENUM_DEF:
            return node_unwrap_enum_def_const(node)->base.name;
    }
    unreachable("");
}

static inline const Node* get_node_src_const(const Node* node) {
    return get_node_src((Node*)node);
}

static inline const Node* get_node_dest_const(const Node* node) {
    return get_node_dest((Node*)node);
}

#endif // NODE_UTIL_H
