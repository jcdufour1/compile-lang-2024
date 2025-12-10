#ifndef ENV_H
#define ENV_H

#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>
#include <ulang_type.h>
#include <msg.h>

typedef struct {
    Vec_base info;
    Symbol_collection* buf;
} Sym_coll_vec;

// TODO: consider if this should be moved
typedef struct {
    Vec_base info;
    Scope_id* buf;
} Scope_id_vec;

typedef struct Env_ {
    uint32_t error_count;
    uint32_t warning_count;

    Sym_coll_vec symbol_tables;

    // needed to prevent infinite recursion when printing errors
    bool silent_generic_resol_errors;

    Ulang_type parent_fn_rtn_type;

    // TODO: think about functions and structs in different scopes, but with the same name (for both of below objects)
    //   if I implement functions and structs in non top level
    Name_vec fun_implementations_waiting_to_resolve;
    Function_decl_tbl function_decl_tbl;
    
    Name_vec struct_like_waiting_to_resolve;
    Usymbol_table struct_like_tbl;

    Strv mod_path_main_fn;

    Ulang_type_vec gen_args_char;

    Defered_msg_vec defered_msgs;

    bool a_main_was_freed;
    bool supress_type_inference_failures;
} Env;

// TODO: move this function?
static inline void arena_free_a_main(void) {
    env.a_main_was_freed = true;
    arena_free_internal(&a_main);
    a_main.next = NULL;
}


#endif // ENV_H
