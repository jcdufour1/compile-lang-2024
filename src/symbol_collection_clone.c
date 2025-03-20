#include <symbol_collection_clone.h>
#include <uast_clone.h>
#include <symbol_iter.h>

Usymbol_table usymbol_table_clone(Usymbol_table tbl) {
    Usymbol_table new_tbl = {0};
    if (tbl.count < 1) {
        return new_tbl;
    }

    Usymbol_iter iter = usym_tbl_iter_new(tbl);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        usym_tbl_add(&new_tbl, uast_def_clone(curr));
    }

    return new_tbl;
}

Symbol_table symbol_table_clone(Symbol_table tbl) {
    if (tbl.count < 1) {
        return (Symbol_table) {0};
    }
    todo();
}

Alloca_table alloca_table_clone(Alloca_table tbl) {
    if (tbl.count < 1) {
        return (Alloca_table) {0};
    }
    todo();
}

Symbol_collection symbol_collection_clone(Symbol_collection coll) {
    return (Symbol_collection) {
        .usymbol_table = usymbol_table_clone(coll.usymbol_table),
        .symbol_table = symbol_table_clone(coll.symbol_table),
        .alloca_table = alloca_table_clone(coll.alloca_table)
    };
}
