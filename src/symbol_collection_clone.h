#ifndef SYMBOL_COLLECTION_CLONE_H
#define SYMBOL_COLLECTION_CLONE_H

#include <env.h>
#include <util.h>
#include <uast.h>

Usymbol_table usymbol_table_clone(Usymbol_table tbl);

Symbol_table symbol_table_clone(Symbol_table tbl);

Alloca_table alloca_table_clone(Alloca_table tbl);

Symbol_collection symbol_collection_clone(Symbol_collection coll);

#endif // SYMBOL_COLLECTION_CLONE_H
