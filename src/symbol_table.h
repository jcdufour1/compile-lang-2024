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
#include <stb_ds.h>

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f)

static inline void symbol_log_table_internal(int log_level, const Symbol_table sym_table, const char* file_path, int line) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Symbol_table_node* sym_node = &sym_table.table_nodes[idx];
        if (sym_node->status == SYM_TBL_OCCUPIED) {
            log_indent_file(log_level, file_path, line, 4, NODE_FMT"\n", node_print(sym_node->node));
        }
    }
}

#define symbol_log_table(log_level, sym_table) \
    do { \
        symbol_log_table_internal(log_level, sym_table, __FILE__, __LINE__) \
    } while(0)

static inline size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {
    assert(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

// returns false if symbol is already added to the table
static inline bool sym_tbl_add_internal(Symbol_table_node* sym_tbl_nodes, size_t capacity, Node* node_of_symbol) {
    assert(node_of_symbol);
    Str_view symbol_name = get_node_name(node_of_symbol);
    assert(symbol_name.count > 0 && "invalid node_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_nodes[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(get_node_name(sym_tbl_nodes[curr_table_idx].node), get_node_name(node_of_symbol))) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Symbol_table_node node = {.key = symbol_name, .node = node_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_nodes[curr_table_idx] = node;
    return true;
}

static inline void sym_tbl_cpy(
    Symbol_table_node* dest,
    const Symbol_table_node* src,
    size_t capacity,
    size_t count_nodes_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_nodes_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
            sym_tbl_add_internal(dest, capacity, src[bucket_src].node);
        }
    }
}

static inline void sym_tbl_expand_if_nessessary(Symbol_table* sym_table) {
    size_t old_capacity_node_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t node_size = sizeof(sym_table->table_nodes[0]);

    bool should_move_elements = false;
    Symbol_table_node* new_table_nodes;

    if (sym_table->capacity < 1) {
        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {
        sym_table->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_nodes = arena_alloc(&a_main, sym_table->capacity*node_size);
        sym_tbl_cpy(new_table_nodes, sym_table->table_nodes, sym_table->capacity, old_capacity_node_count);
        if (sym_table->table_nodes) {
            // not freeing here currently
        }
        sym_table->table_nodes = new_table_nodes;
    }
}

static inline bool sym_tbl_is_equal(Str_view a, Str_view b) {
    if (a.count != b.count) {
        return false;
    }

    return 0 == memcmp(a.str, b.str, b.count);
}

// return false if symbol is not found
static inline bool sym_tbl_lookup_internal(Symbol_table_node** result, const Symbol_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Symbol_table_node* curr_node = &sym_table->table_nodes[curr_table_idx];

        if (curr_node->status == SYM_TBL_OCCUPIED) {
            if (sym_tbl_is_equal(curr_node->key, key)) {
                *result = curr_node;
                return true;
            }
        }

        if (curr_node->status == SYM_TBL_NEVER_OCCUPIED) {
            return false;
        }

        curr_table_idx = (curr_table_idx + 1) % sym_table->capacity;
        if (curr_table_idx == init_table_idx) {
            return false;
        }
    }

    unreachable("");
}

static inline bool sym_tbl_lookup(Node** result, const Symbol_table* sym_table, Str_view key) {
    Symbol_table_node* sym_node;
    if (!sym_tbl_lookup_internal(&sym_node, sym_table, key)) {
        return false;
    }
    *result = sym_node->node;
    return true;
}

// returns false if symbol has already been added to the table
static inline bool sym_tbl_add(Symbol_table* sym_table, Node* node_of_symbol) {
    sym_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!sym_tbl_add_internal(sym_table->table_nodes, sym_table->capacity, node_of_symbol)) {
        return false;
    }
    Node* dummy;
    assert(sym_tbl_lookup(&dummy, sym_table, get_node_name(node_of_symbol)));
    sym_table->count++;
    return true;
}

static inline void sym_tbl_update(Symbol_table* sym_table, Node* node_of_symbol) {
    Symbol_table_node* sym_node;
    if (sym_tbl_lookup_internal(&sym_node, sym_table, get_node_name(node_of_symbol))) {
        sym_node->node = node_of_symbol;
        return;
    }
    try(sym_tbl_add(sym_table, node_of_symbol));
}

#define SYM_TBL_STATUS_FMT "%s"

static inline const char* sym_tbl_status_print(SYM_TBL_STATUS status) {
    switch (status) {
        case SYM_TBL_NEVER_OCCUPIED:
            return "never occupied";
        case SYM_TBL_PREVIOUSLY_OCCUPIED:
            return "prev occupied";
        case SYM_TBL_OCCUPIED:
            return "currently occupied";
        default:
            unreachable("");
    }
}

static inline void symbol_log_internal(int log_level, const Env* env, const char* file_path, int line) {
    if (env->ancesters.info.count < 1) {
        return;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&env->ancesters, idx);
        if (curr_node->type == NODE_BLOCK) {
            log_indent_file(log_level, file_path, line, 0, "at index: %zu\n", idx);
            symbol_log_table_internal(log_level, node_unwrap_block(curr_node)->symbol_table, file_path, line);
        }

        if (idx < 1) {
            log(log_level, "\n\n");
            return;
        }
    }
}

#define symbol_log(log_level, env) \
    do { \
        symbol_log_internal(log_level, env, __FILE__, __LINE__); \
    } while(0)

static inline bool symbol_lookup(Node** result, const Env* env, Str_view key) {
    if (env->ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            const Node_block* block = node_unwrap_block_const(vec_at(&env->ancesters, idx));
            if (sym_tbl_lookup(result, &block->symbol_table, key)) {
                return true;
            }
        }

        if (idx < 1) {
            return false;
        }
    }
}

static inline bool symbol_add(Env* env, Node* node_of_symbol) {
    Node* dummy;
    if (symbol_lookup(&dummy, env, get_node_name(node_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            Node_block* block = node_unwrap_block(vec_at(&env->ancesters, idx));
            try(sym_tbl_add(&block->symbol_table, node_of_symbol));
            return true;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

// these nodes will be actually added to a symbol table when `symbol_do_add_defered` is called
static inline void symbol_add_defer(Env* env, Node* node_of_symbol) {
    vec_append(&a_main, &env->defered_symbols_to_add, node_of_symbol);
}

static inline bool symbol_do_add_defered(Node** redefined_sym, Env* env) {
    for (size_t idx = 0; idx < env->defered_symbols_to_add.info.count; idx++) {
        if (!symbol_add(env, vec_at(&env->defered_symbols_to_add, idx))) {
            *redefined_sym = vec_at(&env->defered_symbols_to_add, idx);
            vec_reset(&env->defered_symbols_to_add);
            return false;
        }
    }

    vec_reset(&env->defered_symbols_to_add);
    return true;
}

static inline void symbol_ignore_defered(Env* env) {
    vec_reset(&env->defered_symbols_to_add);
}

static inline void symbol_update(Env* env, Node* node_of_symbol) {
    Node* existing;
    if (symbol_lookup(&existing, env, get_node_name(node_of_symbol))) {
        todo();
    }
}

static inline Symbol_table* symbol_get_block(Env* env) {
    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            return &node_unwrap_block(vec_at(&env->ancesters, idx))->symbol_table;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

static void log_symbol_table_if_block(Env* env) {
    Node* curr_node = vec_top(&env->ancesters);
    if (curr_node->type != NODE_BLOCK)  {
        return;
    }

    Node_block* block = node_unwrap_block(curr_node);
    symbol_log_table_internal(LOG_DEBUG, block->symbol_table, __FILE__, __LINE__);
}

static inline void symbol_log_deep(Node* root) {
    Env env = {0};
    vec_append(&a_main, &env.ancesters, root);
    walk_tree(&env, log_symbol_table_if_block);
}

#endif // SYMBOL_TABLE_H
