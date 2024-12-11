#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "str_view.h"
#include "string.h"
#include "node.h"
#include "nodes.h"
#include "node_utils.h"
#include "env.h"
#include "symbol_table_struct.h"
#include "do_passes.h"

void symbol_log_table_internal(int log_level, const Symbol_table sym_table, int recursion_depth, const char* file_path, int line);

#define symbol_log_table(log_level, sym_table) \
    do { \
        symbol_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__) \
    } while(0)

// returns false if symbol is already added to the table
bool sym_tbl_add_internal(Symbol_table_node* sym_tbl_nodes, size_t capacity, Node* node_of_symbol);

bool sym_tbl_lookup_internal(Symbol_table_node** result, const Symbol_table* sym_table, Str_view key);

static inline bool sym_tbl_lookup(Node** result, const Symbol_table* sym_table, Str_view key) {
    Symbol_table_node* sym_node;
    if (!sym_tbl_lookup_internal(&sym_node, sym_table, key)) {
        return false;
    }
    *result = sym_node->node;
    return true;
}

// returns false if symbol has already been added to the table
bool sym_tbl_add(Symbol_table* sym_table, Node* node_of_symbol);

void sym_tbl_update(Symbol_table* sym_table, Node* node_of_symbol);

#define SYM_TBL_STATUS_FMT "%s"

const char* sym_tbl_status_print(SYM_TBL_STATUS status);

void symbol_log_internal(int log_level, const Env* env, const char* file_path, int line);

#define symbol_log(log_level, env) \
    do { \
        symbol_log_internal(log_level, env, __FILE__, __LINE__); \
    } while(0)

bool symbol_lookup(Node** result, const Env* env, Str_view key);

bool symbol_add(Env* env, Node* node_of_symbol);

// these nodes will be actually added to a symbol table when `symbol_do_add_defered` is called
static inline void symbol_add_defer(Env* env, Node* node_of_symbol) {
    if (str_view_is_equal(str_view_from_cstr("str8"), get_node_name(node_of_symbol))) {
    }
    assert(node_of_symbol);
    vec_append(&a_main, &env->defered_symbols_to_add, node_of_symbol);
}

bool symbol_do_add_defered(Node** redefined_sym, Env* env);

static inline void symbol_ignore_defered(Env* env) {
    vec_reset(&env->defered_symbols_to_add);
}

static inline void symbol_update(Env* env, Node* node_of_symbol) {
    Node* existing;
    if (symbol_lookup(&existing, env, get_node_name(node_of_symbol))) {
        todo();
    }
    todo();
}

Symbol_table* symbol_get_block(Env* env);

void log_symbol_table_if_block(Env* env, const char* file_path, int line);

#endif // SYMBOL_TABLE_H
