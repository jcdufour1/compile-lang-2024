#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "node_utils.h"

bool is_i_lang_type(Lang_type lang_type);

int64_t i_lang_type_to_bit_width(Lang_type lang_type);

Str_view literal_name_new(void);

Llvm_id get_prev_load_id(const Node* var_call);

Node* get_storage_location(const Env* env, Str_view sym_name);

Llvm_id get_store_dest_id(const Env* env, const Node* var_call);

const Node_variable_def* get_symbol_def_from_alloca(const Env* env, const Node* alloca);

Llvm_id get_matching_label_id(const Env* env, Str_view name);

// lhs and rhs should not be used for other tasks after this
Node_assignment* assignment_new(const Env* env, Node* lhs, Node* rhs);

Node_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos);

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos);

Node_binary* binary_new(const Env* env, Node* lhs, Node* rhs, TOKEN_TYPE operation_type);

Node_unary* unary_new(const Env* env, Node* child, TOKEN_TYPE operation_type, Lang_type init_lang_type);

Llvm_id get_matching_fun_param_load_id(const Node* src);

const Node* get_lang_type_from_sym_definition(const Node* sym_def);

uint64_t sizeof_lang_type(Lang_type lang_type);

uint64_t sizeof_item(const Env* env, const Node* item);

uint64_t sizeof_struct(const Env* env, const Node* struct_literal);

uint64_t sizeof_struct_definition(const Env* env, const Node_struct_def* struct_def);

uint64_t sizeof_struct_literal(const Env* env, const Node_struct_literal* struct_literal);

static inline bool is_struct_variable_definition(const Env* env, const Node_variable_def* var_def) {
    Node* struct_def;
    return symbol_lookup(&struct_def, env, var_def->lang_type.str);
}

static inline bool is_struct_symbol(const Env* env, const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL_TYPED || symbol->type == NODE_STRUCT_MEMBER_SYM_TYPED);

    Node* var_def;
    if (!symbol_lookup(&var_def, env, get_node_name(symbol))) {
        unreachable("");
    }
    return is_struct_variable_definition(env, node_unwrap_variable_def(var_def));
}

bool is_corresponding_to_a_struct(const Env* env, const Node* node);

static inline size_t get_member_index(const Node_struct_def* struct_def, const Node_struct_member_sym_piece_typed* member_symbol) {
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Node* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_node_name(node_wrap(curr_member)), get_node_name(node_wrap(member_symbol)))) {
            return idx;
        }
    }
    unreachable("member not found");
}

static inline bool try_get_member_def(Node_variable_def** member_def, const Node_struct_def* struct_def, const Node* member_symbol) {
    assert(
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED ||
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED
    );

    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Node* curr_member = vec_at(&struct_def->members, idx);
        if (str_view_is_equal(get_node_name(curr_member), get_node_name(member_symbol))) {
            assert(get_lang_type(curr_member).str.count > 0);
            *member_def = (Node_variable_def*)curr_member;
            return true;
        }
    }
    return false;
}

static inline const Node_variable_def* get_member_def(const Node_struct_def* struct_def, const Node* member_symbol) {
    Node_variable_def* member_def;
    if (!try_get_member_def(&member_def, struct_def, member_symbol)) {
        unreachable("could not find member definition");
    }
    return member_def;
}

bool try_get_struct_definition(const Env* env, Node_struct_def** struct_def, Node* node);

static inline Node_struct_def* get_struct_definition(const Env* env, Node* node) {
    Node_struct_def* struct_def;
    if (!try_get_struct_definition(env, &struct_def, node)) {
        unreachable("could not find struct definition for "NODE_FMT"\n", node_print(node));
    }
    return struct_def;
}

static inline const Node_struct_def* get_struct_definition_const(const Env* env, const Node* node) {
    return get_struct_definition(env, (Node*)node);
}

bool try_set_assignment_operand_types(const Env* env, Lang_type* lang_type, Node_assignment* assignment);

// returns false if unsuccessful
bool try_set_binary_lang_type(const Env* env, Lang_type* lang_type, Node_binary* operator);

// returns false if unsuccessful
bool try_set_binary_operand_lang_type(Lang_type* lang_type, Node* operand);

Lang_type get_parent_function_return_type(const Env* env);

bool try_set_unary_lang_type(const Env* env, Lang_type* lang_type, Node_unary* unary);

bool try_set_operation_lang_type(const Env* env, Lang_type* lang_type, Node_operator* operator);

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node_symbol_untyped* sym_untyped);

bool try_set_function_call_types(const Env* env, Lang_type* lang_type, Node_function_call* fun_call);

bool try_set_struct_member_symbol_types(const Env* env, Lang_type* lang_type, Node_struct_member_sym_untyped* struct_memb_sym);

bool try_set_node_type(const Env* env, Lang_type* lang_type, Node* node);

#endif // PARSER_UTIL_H
