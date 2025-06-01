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

typedef struct {
    Vec_base info;
    Scope_id* buf;
} Scope_id_vec;

// only used in type_checking.c
typedef enum {
    PARENT_OF_NONE = 0,
    PARENT_OF_CASE,
    PARENT_OF_ASSIGN_RHS,
    PARENT_OF_RETURN,
    PARENT_OF_BREAK,
    PARENT_OF_IF,
} PARENT_OF;

typedef enum {
    DEFER_PARENT_OF_FUN,
    DEFER_PARENT_OF_FOR,
    DEFER_PARENT_OF_IF,
    DEFER_PARENT_OF_BLOCK,
    DEFER_PARENT_OF_TOP_LEVEL,
} DEFER_PARENT_OF;

// a defered statement
typedef struct {
    Tast_defer* defer;
    Tast_label* label;
} Defer_pair;

// TODO: move this macro?
#define defer_pair_print(pair) strv_print(defer_pair_print_internal(pair))

// TODO: move this function?
static inline Strv defer_pair_print_internal(Defer_pair pair) {
    String buf = {0};
    string_extend_strv(&a_print, &buf, tast_defer_print_internal(pair.defer, 0));
    string_extend_strv(&a_print, &buf, tast_label_print_internal(pair.label, 0));
    return string_to_strv(buf);
}

// all defered statements in one scope
typedef struct {
    Vec_base info;
    Defer_pair* buf;
} Defer_pair_vec;

// all defered statements and parent_of
typedef struct {
    Defer_pair_vec pairs;
    DEFER_PARENT_OF parent_of;
    Tast_expr* rtn_val;
    Name is_brking;
    Name is_conting;
} Defer_collection;

// stack of scope defered statements
typedef struct {
    Vec_base info;
    Defer_collection* buf;
} Defer_collection_vec;

typedef struct {
    Defer_collection_vec coll_stack;
    Name is_rtning;
} Defer_colls;

// TODO: separate Env for different passes
typedef struct Env_ {
    Sym_coll_vec symbol_tables;

    Uast_def_vec udefered_symbols_to_add; // TODO: remove

    Strv curr_mod_path; // TODO: remove

    bool type_checking_is_in_struct_base_def;

    Ulang_type parent_fn_rtn_type;

    // TODO: think about functions and structs in different scopes, but with the same name (for both of below objects)
    Name_vec fun_implementations_waiting_to_resolve;
    Function_decl_tbl function_decl_tbl;
    
    Name_vec struct_like_waiting_to_resolve;
    Usymbol_table struct_like_tbl;
} Env;

#endif // ENV_H
