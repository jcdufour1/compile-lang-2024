#ifndef ENV_H
#define ENV_H

#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <ir_forward_decl.h>
#include <ulang_type.h>
#include <msg.h>
#include <hash_tables.h>

typedef struct Env_ {
    uint32_t error_count;
    uint32_t warning_count;

    // needed to prevent infinite recursion when printing errors
    bool silent_generic_resol_errors;

    Ulang_type parent_fn_rtn_type;

    Name_darr fun_implementations_waiting_to_resolve;
    Hash_table_function_decl_was_encountered function_decl_tbl;
    
    Name_darr struct_like_waiting_to_resolve;
    Hash_table_stable_uast struct_like_tbl;

    Strv mod_path_main_fn;
    Uname name_main_fn;

    Ulang_type_darr gen_args_char;

    Defered_msg_darr defered_msgs;

    bool a_main_was_freed;
    bool supress_type_inference_failures;

    bool is_printing;

    bool do_initialize_globals;

    Strv mod_path_curr_file;

    Hash_table_stable_raw_union_of_enum raw_union_of_enum;
    Hash_table_stable_struct_to_struct struct_to_struct;
    Hash_table_stable_file_path_to_text file_path_to_text;
    Hash_table_stable_function_decls function_decls;
} Env;

// TODO: move this function?
static inline void arena_free_a_main(void) {
    env.a_main_was_freed = true;
    arena_free_internal(&a_main);
    a_main.next = NULL;
}


#endif // ENV_H
