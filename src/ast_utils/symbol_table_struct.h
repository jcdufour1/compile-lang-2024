#ifndef SYMBOL_TABLE_STRUCT_H
#define SYMBOL_TABLE_STRUCT_H

typedef enum {
    SYM_TBL_NEVER_OCCUPIED = 0,
    SYM_TBL_PREVIOUSLY_OCCUPIED,
    SYM_TBL_OCCUPIED,
} SYM_TBL_STATUS;

#include <uast_forward_decl.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>


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
} Generic_vec;


typedef struct {
    Uast_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Usymbol_table_tast;
static_assert(sizeof(Usymbol_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Usymbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Usymbol_table;


typedef struct {
    Tast_def* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Symbol_table_tast;
static_assert(sizeof(Symbol_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Symbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Symbol_table;


typedef struct {
    Ir* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Ir_table_tast;
static_assert(sizeof(Ir_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Ir_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Ir_table;

typedef struct {
    Ir_name name;
    size_t cfg_node_of_init;
    size_t block_pos_of_init;
} Init_table_node;

typedef struct {
    Init_table_node* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Init_table_tast;
static_assert(sizeof(Init_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Init_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Init_table;

typedef struct {
    Vec_base info;
    Init_table* buf;
} Init_table_vec;

// TODO: rename Scope_id_to_next_table_tast to Scope_id_to_next_table_node, etc.

typedef struct {
    Ir_name name_self;
    Name name_regular;
} Ir_name_to_name_table_node;

typedef struct {
    Ir_name_to_name_table_node* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Ir_name_to_name_table_tast;
static_assert(sizeof(Ir_name_to_name_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Ir_name_to_name_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Ir_name_to_name_table;

typedef struct {
    Vec_base info;
    Ir_name_to_name_table* buf;
} Ir_name_to_name_table_vec;


typedef struct {
    Name name_self;
    Ir_name ir_name;
} Name_to_ir_name_table_node;

typedef struct {
    Name_to_ir_name_table_node* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Name_to_ir_name_table_tast;
static_assert(sizeof(Name_to_ir_name_table_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Name_to_ir_name_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Name_to_ir_name_table;

typedef struct {
    Vec_base info;
    Name_to_ir_name_table* buf;
} Name_to_ir_name_table_vec;


typedef struct {
    Uast_function_decl* tast;
    Strv key;
    SYM_TBL_STATUS status;
} Function_decl_tbl_tast;
static_assert(sizeof(Function_decl_tbl_tast) == sizeof(Generic_symbol_table_tast), "");

typedef struct {
    Function_decl_tbl_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Function_decl_tbl;


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


typedef struct {
    Usymbol_table usymbol_table;
    Symbol_table symbol_table;
    Ir_table alloca_table;
} Symbol_collection;

#endif // SYMBOL_TABLE_STRUCT_H

