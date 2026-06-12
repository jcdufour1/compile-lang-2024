#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "strv.h"
#include <local_string.h>
#include "symbol_table_struct.h"
#include <do_passes.h>
#include <uast_forward_decl.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>
#include <env.h>

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Strv key);

void usymbol_log_table_internal(int log_level, const Hash_table_stable_uast sym_table, Indent indent, const char* file_path, int line);

#define usymbol_log_table(log_level, sym_table) \
    do { \
        usymbol_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__); \
    } while(0)

void symbol_log_table_internal(int log_level, const Hash_table_stable_tast sym_table, Indent indent, const char* file_path, int line);

#define symbol_log_table(log_level, sym_table) \
    do { \
        symbol_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__); \
    } while(0)

// returns false if symbol is already added to the table
bool sym_tbl_add_internal(Hash_table_stable_tast_tast* sym_tbl_tasts, size_t capacity, Tast_def* tast_of_symbol);

bool sym_tbl_lookup_internal(Hash_table_stable_tast_tast** result, const Hash_table_stable_tast* sym_table, Strv key);

bool sym_tbl_lookup(Tast_def** result, Name key);

bool symbol_lookup(Tast_def** result, Name key);

bool symbol_add(Tast_def* tast_of_symbol);

void symbol_update(Tast_def* tast_of_symbol);

void ir_log_table_internal(int log_level, const Hash_table_stable_ir sym_table, Indent indent, const char* file_path, int line);

#define alloca_log_table(log_level, sym_table) \
    do { \
        alloca_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__); \
    } while(0)

Hash_table_stable_tast* symbol_get_block(void);

void log_symbol_table_if_block(const char* file_path, int line);

bool c_forward_struct_tbl_lookup(Name** result, Name key);

// returns false if value has already been added to the table
bool c_forward_struct_tbl_add(Name* value, Name key);

bool file_path_to_text_tbl_lookup(Strv** result, Strv key);

// returns false if file_path_to_text has already been added to the table
bool file_path_to_text_tbl_add(Strv* file_text, Strv key);

// returns parent of key
Scope_id scope_get_parent_tbl_lookup(Scope_id key);

void scope_get_parent_tbl_update(Scope_id key, Scope_id parent);

bool scope_id_is_top_level(Scope_id scope);

bool resolved_done_or_waiting_tbl_add(Name key);

bool function_decl_tbl_lookup(Uast_function_decl** decl, Name key);

bool function_decl_tbl_add(Uast_function_decl* decl);

bool struct_like_tbl_add(Uast_def* def);

bool raw_union_of_enum_add(Tast_raw_union_def* def, Name enum_name);

bool raw_union_of_enum_lookup(Tast_raw_union_def** def, Name enum_name);

bool struct_to_struct_add(Tast_struct_def* def, Name enum_name);

bool struct_to_struct_lookup(Tast_struct_def** def, Name enum_name);

bool struct_like_tbl_lookup(Uast_def** def, Name key);

Name scope_to_name_tbl_lookup(Scope_id key);

void scope_to_name_tbl_add(Scope_id key, Name scope_name);

void scope_to_name_tbl_update(Scope_id key, Name scope_name);

void scope_id_to_parent_dump(LOG_LEVEL log_level);

Scope_id symbol_collection_new(Scope_id parent, Name scope_name);

bool expand_again_add(Arena* arena, Uast_def* item);
    
bool expand_again_lookup(Uast_def** result, Name name);

#endif // SYMBOL_TABLE_H

