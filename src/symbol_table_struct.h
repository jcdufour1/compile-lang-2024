#ifndef SYMBOL_TABLE_STRUCT_H
#define SYMBOL_TABLE_STRUCT_H

typedef enum {
    SYM_TBL_NEVER_OCCUPIED = 0,
    SYM_TBL_PREVIOUSLY_OCCUPIED,
    SYM_TBL_OCCUPIED,
} SYM_TBL_STATUS;

#include <uast_forward_decl.h>
#include <tast_forward_decl.h>
#include <llvm_forward_decl.h>

typedef struct {
    Uast_def* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Usymbol_table_tast;

typedef struct {
    Usymbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Usymbol_table;


typedef struct {
    Tast_def* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Symbol_table_tast;

typedef struct {
    Symbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Symbol_table;


typedef struct {
    Llvm* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Alloca_table_tast;

typedef struct {
    Alloca_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Alloca_table;


// TODO: rename Scope_id_to_next_table_tast to Scope_id_to_next_table_node, etc.


typedef struct {
    Scope_id* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Scope_id_to_parent_tast;

typedef struct {
    Scope_id_to_parent_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Scope_id_to_parent;


typedef struct {
    void* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Resolved_done_or_waiting_tast;

typedef struct {
    Resolved_done_or_waiting_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Resolved_done_or_waiting;


typedef struct {
    Uast_function_decl* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Function_decl_tbl_tast;

typedef struct {
    Function_decl_tbl_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Function_decl_tbl;


typedef struct {
    Str_view* item;
    Str_view key;
    SYM_TBL_STATUS status;
} File_path_to_text_tast;

typedef struct {
    File_path_to_text_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} File_path_to_text;


typedef struct {
    void* tast;
    Str_view key;
    SYM_TBL_STATUS status;
} Generic_symbol_table_tast;

typedef struct {
    void* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Generic_symbol_table;

typedef struct {
    Vec_base info;
    void** buf;
} Generic_vec;

typedef struct {
    Usymbol_table usymbol_table;
    Symbol_table symbol_table;
    Alloca_table alloca_table;
} Symbol_collection;

#endif // SYMBOL_TABLE_STRUCT_H

