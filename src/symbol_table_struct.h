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
static inline Str_view alloca_get_name(const Llvm_alloca* llvm);
static inline Str_view tast_def_get_name(const Tast_def* def);
static inline Str_view uast_def_get_name(const Uast_def* def);
static inline Str_view llvm_tast_get_name(const Llvm* llvm);
static inline Str_view rm_tuple_struct_get_name(const Tast_struct_def* struct_def);
bool lang_type_atom_is_signed(Lang_type_atom atom);
bool lang_type_atom_is_unsigned(Lang_type_atom atom);
typedef struct {
    Uast_def* tast;
    SYM_TBL_STATUS status;
} Usymbol_table_tast;

typedef struct {
    Usymbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Usymbol_table;


typedef struct {
    Tast_def* tast;
    SYM_TBL_STATUS status;
} Symbol_table_tast;

typedef struct {
    Symbol_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Symbol_table;


typedef struct {
    Llvm* tast;
    SYM_TBL_STATUS status;
} Alloca_table_tast;

typedef struct {
    Alloca_table_tast* table_tasts;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Alloca_table;

typedef struct {
    void* tast;
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

