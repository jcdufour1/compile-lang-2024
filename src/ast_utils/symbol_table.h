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

bool usymbol_lookup(Uast_def** result, Name key);

#define Usymbol_iter Hash_table_scoped_iter_node_uast
#define Symbol_iter Hash_table_scoped_iter_node_tast
#define Ir_iter Hash_table_scoped_iter_node_ir

static inline Usymbol_iter usym_tbl_iter_new(Scope_id scope_id) {
    return hash_table_scoped_iter_new_uast(scope_id);
}

static inline Symbol_iter sym_tbl_iter_new(Scope_id scope_id) {
    return hash_table_scoped_iter_new_tast(scope_id);
}

static inline Ir_iter ir_tbl_iter_new(Scope_id scope_id) {
    return hash_table_scoped_iter_new_ir(scope_id);
}

static inline bool usym_tbl_iter_next(Uast_def** curr_def, Usymbol_iter* iter) {
    Strv curr_key = (Strv) {0};
    return hash_table_scoped_iter_uast(&symbol_tables.usymbol_table, &curr_key, curr_def, iter);
}

static inline bool sym_tbl_iter_next(Tast_def** curr_def, Symbol_iter* iter) {
    Strv curr_key = (Strv) {0};
    return hash_table_scoped_iter_tast(&symbol_tables.symbol_table, &curr_key, curr_def, iter);
}

static inline bool ir_tbl_iter_next(Ir** curr_def, Ir_iter* iter) {
    Strv curr_key = (Strv) {0};
    return hash_table_scoped_iter_ir(&symbol_tables.ir_table, &curr_key, curr_def, iter);
}

static inline bool symbol_lookup(Tast_def** result, Name key) {
    return hash_table_scoped_lookup_tast(result, &symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);
}

static inline bool ir_lookup(Ir** result, Name key) {
    return hash_table_scoped_lookup_ir(result, &symbol_tables.ir_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);
}

bool ir_add(Ir* def_to_add);
bool usymbol_add(Uast_def* def_to_add);
void usymbol_update(Uast_def* def_to_update);
bool symbol_add(Tast_def* def_to_add);
void symbol_update(Tast_def* def_to_add);
bool usym_tbl_add(Uast_def* def_to_add);
bool sym_tbl_add(Tast_def* def_to_add);
bool ir_tbl_add(Ir* def_to_add);
bool usym_tbl_lookup(Uast_def** result, Name key);
bool sym_tbl_lookup(Tast_def** result, Name key);
bool ir_tbl_lookup(Ir** result, Name key);
void ir_tbl_update(Ir* def_to_update);
void usym_tbl_update(Uast_def* def_to_update);
void sym_tbl_update(Tast_def* def_to_update);
void usymbol_log(LOG_LEVEL log_level, Scope_id scope_id);
void symbol_log(LOG_LEVEL log_level, Scope_id scope_id);
void ir_log(LOG_LEVEL log_level, Scope_id scope_id);

#endif // SYMBOL_TABLE_H

