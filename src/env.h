#ifndef ENV_H
#define ENV_H

#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <llvm_forward_decl.h>
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
    Name is_brking; // only should be used when parent_of == DEFER_PARENT_OF_FOR
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
    Scope_id_vec scope_id_to_parent;
    Sym_coll_vec symbol_tables;

    Uast_def_vec udefered_symbols_to_add;
    Tast_def_vec defered_symbols_to_add;
    Llvm_vec defered_allocas_to_add;
    int recursion_depth;
    File_path_to_text file_path_to_text;

    Str_view curr_mod_path;

    bool type_checking_is_in_struct_base_def;

    Tast_variable_def_vec enum_case_vars;

    Ulang_type parent_fn_rtn_type;
    Lang_type break_type;
    bool break_in_case;

    Lang_type rm_tuple_parent_fn_lang_type;

    Uast_stmt_vec switch_case_defer_add_if_true;
    Uast_stmt_vec switch_case_defer_add_enum_case_part;

    Name struct_rtn_name_parent_function;

    Name name_parent_fn;
    Lang_type lhs_lang_type;
    PARENT_OF parent_of;
    Uast_expr* parent_of_operand;

    Name load_break_symbol_name;
    Name label_if_break;
    Name label_if_after;
    Name label_after_for;
    Name label_if_continue;

    // TODO: think about functions and structs in different scopes, but with the same name (for both of below objects)
    Name_vec fun_implementations_waiting_to_resolve;
    Function_decl_tbl function_decl_tbl;
    
    Name_vec struct_like_waiting_to_resolve;
    Usymbol_table struct_like_tbl;

    Raw_union_of_enum raw_union_of_enum;
    Struct_to_struct struct_to_struct;

    // TODO: remove
    Name_vec struct_like_objs_to_check;

    // this is used to define additional structs to get around the requirement of in order definitions in c
    C_forward_struct_tbl c_forward_struct_tbl;

    // in load_block_stmts
    Tast_variable_def* rtn_def;
    Defer_colls defered_collections;
} Env;

#endif // ENV_H
