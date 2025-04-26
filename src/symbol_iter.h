#ifndef SYMBOL_ITER_H
#define SYMBOL_ITER_H

#include <util.h>
#include <symbol_table.h>

typedef struct {
    size_t bucket_idx;
    Symbol_table tbl;
} Symbol_iter;

static inline Symbol_iter sym_tbl_iter_new(Symbol_table tbl) {
    return (Symbol_iter) {.bucket_idx = 0, .tbl = tbl};
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

static inline Usymbol_iter usym_tbl_iter_new(Usymbol_table tbl) {
    return (Usymbol_iter) {.bucket_idx = 0, .tbl = tbl};
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

static inline Alloca_iter all_tbl_iter_new(Alloca_table tbl) {
    return (Alloca_iter) {.bucket_idx = 0, .tbl = tbl};
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
