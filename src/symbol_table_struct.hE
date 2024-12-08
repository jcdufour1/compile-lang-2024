#ifndef SYMBOL_TABLE_STRUCT_H
#define SYMBOL_TABLE_STRUCT_H

struct Node_;
typedef struct Node_ Node;

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

#endif // SYMBOL_TABLE_STRUCT_H
