#ifndef ENV_H
#define ENV_H

#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>
#include <ulang_type.h>

typedef struct {
    Vec_base info;
    Symbol_collection* buf;
} Sym_coll_vec;

// TODO: consider if this should be moved
typedef struct {
    Vec_base info;
    Scope_id* buf;
} Scope_id_vec;

// TODO: separate Env for different passes
typedef struct Env_ {
    Sym_coll_vec symbol_tables;

    // needed to prevent infinite recursion when printing errors
    bool silent_generic_resol_errors;

    Ulang_type parent_fn_rtn_type;

    // TODO: think about functions and structs in different scopes, but with the same name (for both of below objects)
    Name_vec fun_implementations_waiting_to_resolve;
    Function_decl_tbl function_decl_tbl;
    
    Name_vec struct_like_waiting_to_resolve;
    Usymbol_table struct_like_tbl;
} Env;

#endif // ENV_H
