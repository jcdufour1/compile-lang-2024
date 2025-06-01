#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <newstring.h>
#include <tast.h>
#include <parameters.h>
#include <file.h>
#include <do_passes.h>
#include <uast.h>
#include <type_checking.h>
#include <symbol_log.h>
#include <strv_vec.h>
#include <subprocess.h>
#include <ir_graphvis.h>
#include <symbol_iter.h>
 
// TODO: make separate Env struct for every pass (each Env will need Env_common for things that all envs require (eg. for symbol table lookups))
//
// TODO: test case for handling assigning/returning void item
//
// TODO: expected success test for \r, \t, etc. in source file

static void add_any(const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN,
        lang_type_primitive_const_wrap(lang_type_any_const_wrap(lang_type_any_new(
            POS_BUILTIN, lang_type_atom_new_from_cstr(base_name, pointer_depth, 0)
        )))
    );
    unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
}

static void add_void(void) {
    unwrap(usym_tbl_add(uast_void_def_wrap(uast_void_def_new(POS_BUILTIN))));
}

static void add_primitives(void) {
    add_any("any", 0);
    add_void();
}

Ir_block* compile_file_to_ir(void) {
    memset(&env, 0, sizeof(env));
    // TODO: do this in a more proper way. this is temporary way to test
    //tokenize_do_test();
    memset(&env, 0, sizeof(env));

    symbol_collection_new(SCOPE_BUILTIN);

    add_primitives();

    Uast_block* untyped = NULL;
    bool status = parse_file(&untyped, params.input_file_path);
    if (error_count > 0) {
        log(LOG_DEBUG, "parse_file failed\n");
        assert((!status || params.error_opts_changed) && "parse_file is not returning false when it should\n");
        exit(EXIT_CODE_FAIL);
    }
    assert(status && "error_count should be zero if parse_file returns true");

    log(LOG_DEBUG, "\nafter parsing start--------------------\n");
    usymbol_log_level(LOG_DEBUG, 0);
    log(LOG_DEBUG, FMT, uast_block_print(untyped));
    log(LOG_DEBUG, "\nafter parsing end--------------------\n");

    arena_reset(&a_print);
    Tast_block* typed = NULL;
    status = try_set_types(&typed, untyped);
    if (error_count > 0) {
        log(LOG_DEBUG, "try_set_block_types failed\n");
        assert((!status || params.error_opts_changed) && "try_set_types is not returning false when it should\n");
        exit(EXIT_CODE_FAIL);
    }
    log(LOG_DEBUG, "try_set_block_types succedded\n");
    assert(status && "error_count should be zero if try_set_types returns true");
    
    unwrap(typed);
    arena_reset(&a_print);
    log(LOG_VERBOSE, "arena usage: %zu\n", arena_get_total_usage(&a_main));
    log(LOG_DEBUG,  "\nafter type checking start--------------------\n");
    symbol_log_level(LOG_DEBUG, 0);
    log(LOG_DEBUG,FMT, tast_block_print(typed));
    log(LOG_DEBUG,  "\nafter type checking end--------------------\n");

    Ir_block* ir_root = add_load_and_store(typed);
    log(LOG_DEBUG, "\nafter add_load_and_store start-------------------- \n");
    ir_log_level(LOG_DEBUG, 0);

    Alloca_iter iter = all_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        log(LOG_DEBUG, "\nbefore add_load_and_store aux end-------------------- \n");
        log(LOG_DEBUG, FMT, ir_print(curr));
        log(LOG_DEBUG, "\nafter add_load_and_store aux end-------------------- \n");
    }
    // TODO: for this to actually do anything, we need to iterate on scope_id SCOPE_BUILTIN
    log(LOG_DEBUG, FMT, ir_block_print(ir_root));
    log(LOG_DEBUG, "\nafter add_load_and_store end-------------------- \n");
    if (error_count > 0) {
        exit(EXIT_CODE_FAIL);
    }
    assert(ir_root);

    return ir_root;
}

void do_passes(void) {
    Ir_block* ir = NULL;
    if (params.compile_own) {
        ir = compile_file_to_ir();
    }

    if (params.dump_dot) {
        // TODO: add logic in parse_args to catch below error:
        unwrap(params.compile_own && "this should have been caught in parse_args");
        String graphvis = {0};
        string_extend_strv(&a_print, &graphvis, ir_graphvis(ir));
        write_file("dump.dot", string_to_strv(graphvis));
    }

    if (error_count > 0) {
        unreachable("should have exited before now\n");
    }

    if (params.stop_after > STOP_AFTER_GEN_IR) {
        if (params.emit_llvm) {
            switch (params.backend_info.backend) {
                case BACKEND_NONE:
                    unreachable("this should have been caught eariler");
                case BACKEND_LLVM:
                    emit_llvm_from_tree(ir);
                    break;
                case BACKEND_C:
                    emit_c_from_tree(ir);
                    break;
                default:
                    unreachable("");
            }
        } else {
            switch (params.backend_info.backend) {
                case BACKEND_NONE:
                    unreachable("this should have been caught eariler");
                case BACKEND_LLVM:
                    todo();
                case BACKEND_C:
                    emit_c_from_tree(ir);
                    break;
                default:
                    unreachable("");
            }
        }
    }

    static_assert(
        PARAMETERS_COUNT == 18,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );

    static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop_after states (not all are explicitly handled");
    if (params.stop_after == STOP_AFTER_RUN) {
        // TODO: add logic in parse_args to catch below error:
        Strv_vec cmd = {0};
        String output_path = {0};
        string_extend_cstr(&a_main, &output_path, "./");
        string_extend_strv(&a_main, &output_path, params.output_file_path);
        vec_append(&a_main, &cmd, string_to_strv(output_path));
        int status = subprocess_call(cmd);
        if (status != 0) {
            msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process for the compiled program returned exit code %d\n", status);
            msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_main, cmd)));
            // exit with the child process return status
            exit(status);
        }
    }

    if (error_count > 0) {
        exit(EXIT_CODE_FAIL);
    }
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    do_passes();
    return 0;
}
