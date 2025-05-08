#ifndef SYMBOL_ITER_H
#define SYMBOL_ITER_H

#include <util.h>
#include <symbol_table.h>

typedef struct {
    size_t bucket_idx;
    Symbol_table tbl;
} Symbol_iter;

static inline Symbol_iter sym_tbl_iter_new(Scope_id scope_id) {
    return (Symbol_iter) {.bucket_idx = 0, .tbl = vec_at_ref(&env.symbol_tables, scope_id)->symbol_table};
}

static inline bool sym_tbl_iter_next(Tast_def** result, Symbol_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

typedef struct {
    size_t bucket_idx;
    Usymbol_table tbl;
} Usymbol_iter;

static inline Usymbol_iter usym_tbl_iter_new(Scope_id scope_id) {
    return (Usymbol_iter) {.bucket_idx = 0, .tbl = vec_at_ref(&env.symbol_tables, scope_id)->usymbol_table};
}

static inline bool usym_tbl_iter_next(Uast_def** result, Usymbol_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

typedef struct {
    size_t bucket_idx;
    Alloca_table tbl;
} Alloca_iter;

static inline Alloca_iter all_tbl_iter_new(Scope_id scope_id) {
    return (Alloca_iter) {.bucket_idx = 0, .tbl = vec_at_ref(&env.symbol_tables, scope_id)->alloca_table};
}

static inline bool all_tbl_iter_next(Llvm** result, Alloca_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

#endif // SYMBOL_ITER_H
