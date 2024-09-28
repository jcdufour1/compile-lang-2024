#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "str_view.h"
#include "string.h"
#include "node.h"
#include "nodes.h"
#include <stb_ds.h>

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f)

typedef enum {
    SYM_TBL_NEVER_OCCUPIED = 0,
    SYM_TBL_PREVIOUSLY_OCCUPIED,
    SYM_TBL_OCCUPIED,
} SYM_TBL_STATUS;

typedef struct {
    Str_view key;
    Node* node;
    SYM_TBL_STATUS status;
} Symbol_table_node;

typedef struct {
    Symbol_table_node* table_nodes;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Symbol_table;

extern Symbol_table symbol_table;

static inline size_t sym_tbl_calculate_idx(Str_view sym_name, size_t capacity) {
    return stbds_hash_bytes(sym_name.str, sym_name.count, 0) % capacity;
}

// returns false if symbol is already added to the table
static inline bool sym_tbl_add_internal(Symbol_table_node* sym_tbl_nodes, size_t capacity, Node* node_of_symbol) {
    assert(node_of_symbol);
    Str_view symbol_name = node_of_symbol->name;
    assert(symbol_name.count > 0 && "invalid node_of_symbol");

    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_nodes[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(sym_tbl_nodes[curr_table_idx].node->name, node_of_symbol->name)) {
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

static inline void sym_tbl_cpy(Symbol_table_node* dest, const Symbol_table_node* src, size_t count_nodes_to_cpy) {
    for (size_t bucket_src = 0; bucket_src < count_nodes_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
            sym_tbl_add_internal(dest, symbol_table.capacity, src[bucket_src].node);
        }
    }
}

static inline void sym_tbl_expand_if_nessessary(void) {
    size_t old_capacity_node_count = symbol_table.capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = symbol_table.count + minimum_count_to_reserve;
    size_t node_size = sizeof(symbol_table.table_nodes[0]);

    bool should_move_elements = false;
    Symbol_table_node* new_table_nodes;

    if (symbol_table.capacity < 1) {
        symbol_table.capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / symbol_table.capacity) >= SYM_TBL_MAX_DENSITY) {
        symbol_table.capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_nodes = arena_alloc(&arena, symbol_table.capacity*node_size);
        sym_tbl_cpy(new_table_nodes, symbol_table.table_nodes, old_capacity_node_count);
        if (symbol_table.table_nodes) {
            // not freeing here currently
        }
        symbol_table.table_nodes = new_table_nodes;
    }
}

static inline bool sym_tbl_is_equal(Str_view a, Str_view b) {
    if (a.count != b.count) {
        return false;
    }

    return 0 == memcmp(a.str, b.str, b.count);
}

// return false if symbol is not found
static inline bool sym_tbl_lookup(
    Node** result,
    Str_view key
) {
    size_t curr_table_idx = sym_tbl_calculate_idx(key, symbol_table.capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Symbol_table_node curr_node = symbol_table.table_nodes[curr_table_idx];

        if (curr_node.status == SYM_TBL_OCCUPIED) {
            if (sym_tbl_is_equal(curr_node.key, key)) {
                *result = curr_node.node;
                return true;
            }
        }

        if (curr_node.status == SYM_TBL_NEVER_OCCUPIED) {
            return false;
        }

        curr_table_idx = (curr_table_idx + 1) % symbol_table.capacity;
        if (curr_table_idx == init_table_idx) {
            return false;
        }
    }

    unreachable("");
}

// returns false if symbol has already been added to the table
static inline bool sym_tbl_add(
    Node* node_of_symbol
) {
    sym_tbl_expand_if_nessessary();
    if (!sym_tbl_add_internal(symbol_table.table_nodes, symbol_table.capacity, node_of_symbol)) {
        return false;
    }
    symbol_table.count++;
    return true;
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

#endif // SYMBOL_TABLE_H
