#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "node_utils.h"

bool is_i_lang_type(Lang_type lang_type);

int64_t i_lang_type_to_bit_width(Lang_type lang_type);

int64_t str_view_to_int64_t(Str_view str_view);

bool try_str_view_to_int64_t(int64_t* result, Str_view str_view);

bool try_str_view_to_size_t(size_t* result, Str_view str_view);

Str_view literal_name_new(void);

Llvm_id get_prev_load_id(const Node* var_call);

Llvm_register_sym get_storage_location(const Env* env, Str_view sym_name);

static inline bool Llvm_register_sym_is_some(Llvm_register_sym llvm_reg) {
    Llvm_register_sym reference = {0};
    return 0 != memcmp(&reference, &llvm_reg, sizeof(llvm_reg));
}

static inline Llvm_register_sym llvm_register_sym_new(Node* node) {
    if (node) {
        Llvm_register_sym llvm_reg = {.lang_type = get_lang_type(node), .node = node};
        return llvm_reg;
    } else {
        Llvm_register_sym llvm_reg = {0};
        return llvm_reg;
    }
    unreachable("");
}

static inline Node_llvm_placeholder* node_e_llvm_placeholder_new_from_reg(
    Llvm_register_sym llvm_reg, Lang_type lang_type
) {
    Node_llvm_placeholder* placeholder = node_llvm_placeholder_new(llvm_reg.node->pos);
    placeholder->llvm_reg = llvm_reg;
    placeholder->lang_type = lang_type;
    return placeholder;
}

static inline Llvm_register_sym llvm_register_sym_new_from_expr(Node_expr* expr) {
    return llvm_register_sym_new(node_wrap_expr(expr));
}

static inline Llvm_register_sym llvm_register_sym_new_from_e_operator(Node_operator* operator) {
    return llvm_register_sym_new_from_expr(node_wrap_operator(operator));
}

const Node_variable_def* get_symbol_def_from_alloca(const Env* env, const Node* alloca);

Llvm_id get_matching_label_id(const Env* env, Str_view name);

// lhs and rhs should not be used for other tasks after this
Node_assignment* assignment_new(const Env* env, Node* lhs, Node_expr* rhs);

Node_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos);

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos);

Node_expr* binary_new(const Env* env, Node_expr* lhs, Node_expr* rhs, TOKEN_TYPE operation_type);

Node_expr* unary_new(const Env* env, Node_expr* child, TOKEN_TYPE operation_type, Lang_type init_lang_type);

Llvm_id get_matching_fun_param_load_id(const Node* src);

const Node* get_lang_type_from_sym_definition(const Node* sym_def);

uint64_t sizeof_lang_type(const Env* env, Lang_type lang_type);

uint64_t sizeof_item(const Env* env, const Node* item);

uint64_t sizeof_struct(const Env* env, const Node* struct_literal);

uint64_t sizeof_struct_def_base(const Env* env, const Struct_def_base* base);

uint64_t sizeof_struct_literal(const Env* env, const Node_struct_literal* struct_literal);

static inline bool is_struct_variable_definition(const Env* env, const Node_variable_def* var_def) {
    Node* struct_def;
    return symbol_lookup(&struct_def, env, var_def->lang_type.str);
}

bool lang_type_is_struct(const Env* env, Lang_type lang_type);

bool lang_type_is_raw_union(const Env* env, Lang_type lang_type);

bool lang_type_is_enum(const Env* env, Lang_type lang_type);

static inline bool is_struct_symbol(const Env* env, const Node_expr* symbol) {
    assert(symbol->type == NODE_SYMBOL_TYPED || symbol->type == NODE_MEMBER_SYM_TYPED);

    Node* var_def;
    if (!symbol_lookup(&var_def, env, get_node_name_expr(symbol))) {
        unreachable("");
    }
    return is_struct_variable_definition(env, node_unwrap_variable_def(var_def));
}

bool is_corresponding_to_a_struct(const Env* env, const Node* node);

static inline size_t get_member_index(const Struct_def_base* struct_def, const Node_member_sym_piece_typed* member_symbol) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Node* curr_member = vec_at(&struct_def->members, idx);
        log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_member));
        Str_view name_to_find = get_node_name(node_wrap_member_sym_piece_typed_const(member_symbol));
        log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(name_to_find));
        if (str_view_is_equal(get_node_name(curr_member), name_to_find)) {
            return idx;
        }
    }
    unreachable("member not found");
}

static inline bool try_get_member_def(Node_variable_def** member_def, const Struct_def_base* struct_def, const Node* member_symbol) {
    assert(
        member_symbol->type == NODE_MEMBER_SYM_PIECE_TYPED ||
        member_symbol->type == NODE_MEMBER_SYM_PIECE_UNTYPED
    );

    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        Node* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_node_name(curr_member), get_node_name(member_symbol))) {
            assert(get_lang_type(curr_member).str.count > 0);
            *member_def = node_unwrap_variable_def(curr_member);
            log(LOG_DEBUG, "try_get_member_def: "NODE_FMT"\n", node_print(curr_member));
            return true;
        }
    }
    return false;
}

static inline const Node_variable_def* get_member_def(const Struct_def_base* struct_def, const Node* member_symbol) {
    Node_variable_def* member_def;
    if (!try_get_member_def(&member_def, struct_def, member_symbol)) {
        unreachable("could not find member definition");
    }
    return member_def;
}

bool try_get_struct_def(const Env* env, Node_struct_def** struct_def, Node* node);

static inline Node_struct_def* get_struct_def(const Env* env, Node* node) {
    Node_struct_def* struct_def;
    if (!try_get_struct_def(env, &struct_def, node)) {
        unreachable("could not find struct definition for "NODE_FMT"\n", node_print(node));
    }
    return struct_def;
}

static inline const Node_struct_def* get_struct_def_const(const Env* env, const Node* node) {
    return get_struct_def(env, (Node*)node);
}

bool try_set_assignment_operand_types(const Env* env, Lang_type* lang_type, Node_assignment* assignment);

// returns false if unsuccessful
bool try_set_expr_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_expr* expr);

// returns false if unsuccessful
bool try_set_binary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_binary* operator);

// returns false if unsuccessful
bool try_set_binary_operand_lang_type(Lang_type* lang_type, Node_expr* operand);

Lang_type get_parent_function_return_type(const Env* env);

bool try_set_unary_lang_type(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_unary* unary);

bool try_set_operation_lang_type(const Env* env, Node** new_node, Lang_type* lang_type, Node_operator* operator);

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node_symbol_untyped* sym_untyped);

bool try_set_function_call_types(const Env* env, Node_expr** new_node, Lang_type* lang_type, Node_function_call* fun_call);

bool try_set_member_symbol_types(const Env* env, Node** new_node, Lang_type* lang_type, Node_member_sym_untyped* struct_memb_sym);

bool try_set_node_lang_type(const Env* env, Node** new_node, Lang_type* lang_type, Node* node);

#endif // PARSER_UTIL_H
