#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include <stb_ds.h>

#define SYM_TBL_DEFAULT_CAPACITY 4048
#define SYM_TBL_MAX_DENSITY (0.6f)

typedef enum {
    SYM_TBL_NEVER_OCCUPIED = 0,
    SYM_TBL_PREVIOUSLY_OCCUPIED,
    SYM_TBL_OCCUPIED,
} SYM_TBL_STATUS;

typedef struct {
    Str_view name;
    Node_id node;
    SYM_TBL_STATUS status;
} Symbol_table_node;

typedef struct {
    Symbol_table_node* table_nodes;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Symbol_table;

extern Symbol_table symbol_table;

static inline void sym_tbl_expand_if_nessessary() {
    size_t minimum_count_to_reserve = 1;
    size_t new_count = symbol_table.count + minimum_count_to_reserve;
    size_t node_size = sizeof(symbol_table.table_nodes[0]);

    if (symbol_table.capacity < 1) {
        symbol_table.capacity = SYM_TBL_DEFAULT_CAPACITY;
        symbol_table.table_nodes = safe_malloc(symbol_table.capacity, node_size);
    }

    while ((float)new_count / symbol_table.capacity >= SYM_TBL_MAX_DENSITY) {
        symbol_table.capacity *= 2;
        symbol_table.table_nodes = safe_realloc(symbol_table.table_nodes, symbol_table.capacity, node_size);
    }
}

static inline size_t sym_tbl_calculate_idx(Str_view sym_name) {
    return stbds_hash_bytes(sym_name.str, sym_name.count, 0) % symbol_table.capacity;
}

static inline void sym_tbl_add(
    Node_id node_of_symbol
) {
    sym_tbl_expand_if_nessessary();

    assert(nodes_at(node_of_symbol));
    Str_view symbol_name = nodes_at(node_of_symbol)->name;
    assert(symbol_name.count > 0 && "invalid node_of_symbol");

    size_t table_idx = sym_tbl_calculate_idx(nodes_at(node_of_symbol)->name);
    if (symbol_table.table_nodes[table_idx].status == SYM_TBL_OCCUPIED) {
        todo();
    } else {
        Symbol_table_node node = {.name = nodes_at(node_of_symbol)->name, .node = node_of_symbol, .status = SYM_TBL_OCCUPIED};
        symbol_table.table_nodes[table_idx] = node;
    }

    symbol_table.count++;
}

#endif // SYMBOL_TABLE_H
