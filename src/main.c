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
#include <symbol_iter.h>
#include <msg.h>
#include <str_and_num_utils.h>
#include <lang_type_from_ulang_type.h>
#include <lang_type.h>
#include <time_utils.h>
#include <ir_utils.h>

static void add_opaque(int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN,
        lang_type_primitive_const_wrap(lang_type_opaque_const_wrap(lang_type_opaque_new(
            POS_BUILTIN,
            pointer_depth
        )))
    );
    unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
}

static void add_void(void) {
    unwrap(usym_tbl_add(uast_void_def_wrap(uast_void_def_new(POS_BUILTIN))));
}

static void add_primitives(void) {
    //add_opaque(0);
    add_void();

    vec_append(&a_main, &env.gen_args_char, ulang_type_new_char());
}

static void add_builtin_def(Strv name) {
    unwrap(usym_tbl_add(uast_builtin_def_wrap(uast_builtin_def_new(POS_BUILTIN, name_new(
        MOD_PATH_RUNTIME,
        name,
        (Ulang_type_vec) {0},
        SCOPE_TOP_LEVEL,
        (Attrs) {0}
    )))));
}

static void add_builtin_defs(void) {
    add_builtin_def(sv("static_array_access"));
    add_builtin_def(sv("static_array_slice"));
    add_builtin_def(sv("buf_at"));
}

#define do_pass(pass_fn, sym_log_fn) \
    do { \
        uint64_t before = get_time_milliseconds(); \
        pass_fn(); \
        uint64_t after = get_time_milliseconds(); \
        log(LOG_VERBOSE, "pass `" #pass_fn "` took "FMT"\n", milliseconds_print(after - before));\
        if (env.error_count > 0) { \
            log(LOG_DEBUG, #pass_fn " failed\n"); \
            local_exit(EXIT_CODE_FAIL); \
        } \
\
        log(LOG_DEBUG, "after " #pass_fn " start--------------------\n");\
        if (params_log_level <= LOG_DEBUG) { \
            sym_log_fn(LOG_DEBUG, SCOPE_TOP_LEVEL);\
        } \
        log(LOG_DEBUG, "after " #pass_fn " end--------------------\n");\
\
        arena_reset(&a_temp);\
        arena_reset(&a_pass);\
    } while (0)

#define do_pass_status(pass_fn, sym_log_fn) \
    do { \
        uint64_t before = get_time_milliseconds(); \
        bool status = pass_fn(); \
        (void) status; \
        uint64_t after = get_time_milliseconds(); \
        log(LOG_VERBOSE, "pass `" #pass_fn "` took "FMT"\n", milliseconds_print(after - before));\
        if (env.error_count > 0) { \
            log(LOG_DEBUG, #pass_fn " failed\n"); \
            assert((!status || params.error_opts_changed) && #pass_fn " is not returning false when it should\n"); \
            local_exit(EXIT_CODE_FAIL); \
        } \
\
        log(LOG_DEBUG, "after " #pass_fn " start--------------------\n");\
        if (params_log_level <= LOG_DEBUG) { \
            sym_log_fn(LOG_DEBUG, SCOPE_TOP_LEVEL);\
        } \
        log(LOG_DEBUG, "after " #pass_fn " end--------------------\n");\
\
        arena_reset(&a_temp);\
        arena_reset(&a_pass);\
    } while (0)

void compile_file_to_ir(void) {
    memset(&env, 0, sizeof(env));
    // TODO: do this in a more proper way. this is temporary way to test
    //tokenize_do_test();
    memset(&env, 0, sizeof(env));

    symbol_collection_new(SCOPE_TOP_LEVEL, util_literal_name_new());

    add_primitives();
    add_builtin_defs();

    Uast_mod_alias* new_alias = uast_mod_alias_new(
        POS_BUILTIN,
        MOD_ALIAS_BUILTIN,
        MOD_PATH_BUILTIN,
        SCOPE_TOP_LEVEL
    );
    unwrap(usymbol_add(uast_mod_alias_wrap(new_alias)));

    // generate ir from file(s)
    do_pass_status(parse, usymbol_log_level);
    do_pass_status(try_set_types, symbol_log_level);
    do_pass(add_load_and_store, ir_log_level);
    //Ir* result = NULL;
    //unwrap(ir_lookup(&result, ir_name_new(sv("tests/inputs/union"), sv("union"), (Ulang_type_vec) {0}, 2, (Attrs) {0})));
    //log(LOG_DEBUG, FMT"\n", ir_print(result));
    //log(LOG_DEBUG, "%d\n", ir_tast_get_name(result).attrs);

    // ir passes
    do_pass(construct_cfgs, ir_log_level);
    do_pass(remove_void_assigns, ir_log_level);
    do_pass(check_uninitialized, ir_log_level);
}

void do_passes(void) {
    if (params.compile_own) {
        compile_file_to_ir();
    }

    static_assert(
        PARAMETERS_COUNT == 24,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    if (params.stop_after == STOP_AFTER_IR) {
        if (params.dump_dot) {
            // TODO: add logic in parse_args to catch below error:
            unwrap(params.compile_own && "this should have been caught in parse_args");
            todo();
            //String graphvis = {0};
            //string_extend_strv(&a_temp, &graphvis, ir_graphvis(ir));
            //write_file("dump.dot", string_to_strv(graphvis));
        } else {
            String contents = {0};

            Alloca_iter iter = ir_tbl_iter_new(SCOPE_TOP_LEVEL);
            Ir* curr = NULL;
            while (ir_tbl_iter_next(&curr, &iter)) {
                string_extend_strv(&a_temp, &contents, ir_print_internal(curr, INDENT_WIDTH));
            }
            string_extend_strv(&a_temp, &contents, sv("\n\n"));

            write_file(strv_dup(&a_temp, params.output_file_path), string_to_strv(contents));
            return;
        }
    }

    if (env.error_count > 0) {
        unreachable("should have exited before now\n");
    }

    if (params.stop_after > STOP_AFTER_IR) {
        switch (params.backend_info.backend) {
            case BACKEND_NONE:
                unreachable("this should have been caught eariler");
            case BACKEND_LLVM:
                todo();
            case BACKEND_C:
                emit_c_from_tree();
                break;
            default:
                unreachable("");
        }
    }
    arena_reset(&a_temp);
    arena_reset(&a_pass);

    static_assert(
        PARAMETERS_COUNT == 24,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );

    static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop_after states (not all are explicitly handled");
    if (params.stop_after == STOP_AFTER_RUN) {
        Strv_vec cmd = {0};
        String output_path = {0};
        string_extend_cstr(&a_temp, &output_path, "./");
        string_extend_strv(&a_temp, &output_path, params.output_file_path);
        vec_append(&a_temp, &cmd, string_to_strv(output_path));
        arena_free(&a_main);
        int status = subprocess_call(cmd);
        if (status != 0) {
            msg(DIAG_CHILD_PROCESS_FAILURE, POS_BUILTIN, "child process for the compiled program returned exit code %d\n", status);
            msg(DIAG_NOTE, POS_BUILTIN, "child process run with command `"FMT"`\n", strv_print(cmd_to_strv(&a_main, cmd)));
            // exit with the child process return status
            local_exit(status);
        }
    }

    if (env.error_count > 0) {
        local_exit(EXIT_CODE_FAIL);
    }
    local_exit(0);
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    do_passes();
    unreachable("do_passes should call local_exit");
}
