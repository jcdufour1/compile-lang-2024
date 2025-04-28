#ifndef SYMBOL_COLLECTION_CLONE_H
#define SYMBOL_COLLECTION_CLONE_H

#include <env.h>
#include <util.h>
#include <uast.h>

Usymbol_table usymbol_table_clone(Usymbol_table tbl, Scope_id new_scope);

Symbol_table symbol_table_clone(Symbol_table tbl, Scope_id new_scope);

Alloca_table alloca_table_clone(Alloca_table tbl, Scope_id new_scope);

Symbol_collection symbol_collection_clone(Symbol_collection coll, Scope_id scope_id);

#endif // SYMBOL_COLLECTION_CLONE_H
