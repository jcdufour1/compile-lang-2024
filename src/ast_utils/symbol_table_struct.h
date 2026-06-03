#ifndef SYMBOL_TABLE_STRUCT_H
#define SYMBOL_TABLE_STRUCT_H

#if 0
typedef enum {
    SYM_TBL_NEVER_OCCUPIED = 0,
    SYM_TBL_PREVIOUSLY_OCCUPIED,
    SYM_TBL_OCCUPIED,
} SYM_TBL_STATUS;

#include <uast_forward_decl.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>
#include <name.h>


typedef struct {
    void* tast;
    Strv key;
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
} Generic_darr;


typedef struct {
    Uast_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Hash_table_stable_uast_tast;
static_assert(sizeof(Hash_table_stable_uast_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Hash_table_stable_uast_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Hash_table_stable_uast;


typedef struct {
    Tast_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Hash_table_stable_tast_tast;
static_assert(sizeof(Hash_table_stable_tast_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Hash_table_stable_tast_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Hash_table_stable_tast;


typedef struct {
    Tast_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Hash_table_stable_expand_again_tast;
static_assert(sizeof(Hash_table_stable_expand_again_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Hash_table_stable_expand_again_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Hash_table_stable_expand_again;


typedef struct {
    Ir* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Hash_table_stable_ir_tast;
static_assert(sizeof(Hash_table_stable_ir_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Hash_table_stable_ir_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Hash_table_stable_ir;


// TODO: rename Scope_id_to_next_table_tast to Scope_id_to_next_table_node, etc.


typedef struct {
    Uast_function_decl* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Hash_table_function_decl_was_encountered_tast;
static_assert(sizeof(Hash_table_function_decl_was_encountered_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Hash_table_function_decl_was_encountered_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Hash_table_function_decl_was_encountered;


typedef struct {
    Tast_raw_union_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Raw_union_of_enum_tast;
static_assert(sizeof(Raw_union_of_enum_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Raw_union_of_enum_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Raw_union_of_enum;


typedef struct {
    Tast_struct_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Struct_to_struct_tast;
static_assert(sizeof(Struct_to_struct_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Struct_to_struct_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Struct_to_struct;


typedef struct {
    Name* tast;
    Strv key;
    SYM_TBL_STATUS status;
} C_forward_struct_tbl_tast;
static_assert(sizeof(C_forward_struct_tbl_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    C_forward_struct_tbl_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} C_forward_struct_tbl;


typedef struct {
    Strv* item;
    Strv key;
    SYM_TBL_STATUS status;
} File_path_to_text_tast;
static_assert(sizeof(File_path_to_text_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    File_path_to_text_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} File_path_to_text;


#endif // 0

#endif // SYMBOL_TABLE_STRUCT_H

