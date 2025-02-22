
#include <util.h>
#include <uast.h>
#include <tast.h>
#include <llvm.h>
#include <tasts.h>
#include <llvms.h>
#include "passes.h"
#include <do_passes.h>
#include <symbol_table.h>
#include <tokens.h>
 
// TODO: make separate Env struct for every pass (each Env will need Env_common for things that all envs require (eg. for symbol table lookups))

static void fail(void) {
    if (!params.test_expected_fail) {
        exit(EXIT_CODE_FAIL);
    }

    log(LOG_DEBUG, "%zu %zu\n", expected_fail_count, params.expected_fail_types.info.count); 
    if (expected_fail_count == params.expected_fail_types.info.count) {
        exit(EXIT_CODE_EXPECTED_FAIL);
    } else {
        log(
            LOG_FATAL, "%zu expected fails occured, but %zu expected fails were expected\n",
            expected_fail_count, params.expected_fail_types.info.count
        );
        exit(EXIT_CODE_FAIL);
    }
}

static void add_char(Env* env, const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type_atom_new_from_cstr(base_name, pointer_depth))))
    );
    try(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
}

static void add_any(Env* env, const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_any_const_wrap(lang_type_any_new(lang_type_atom_new_from_cstr(base_name, pointer_depth))))
    );
    try(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
}

static void add_void(Env* env) {
    Uast_void_def* def = uast_void_def_new(POS_BUILTIN);
    try(usym_tbl_add(&env->primitives, uast_literal_def_wrap(uast_void_def_wrap(def))));
}

static void add_primitives(Env* env) {
    add_void(env);
    add_any(env, "any", 0);
}

void do_passes(Str_view file_text, const Parameters* params) {
    // TODO: do this in a more proper way. this is temporary way to test
    tokenize_do_test();

    Env env = {0};
    add_primitives(&env);
    env.file_text = file_text;
    Tokens tokens = tokenize(&env, *params);
    if (error_count > 0) {
        fail();
    }

    Uast_block* untyped = parse(&env, tokens);
    if (error_count > 0) {
        fail();
    }
    arena_reset(&print_arena);
    log(LOG_DEBUG, "\n"TAST_FMT, uast_block_print(untyped));

    //log_tree(LOG_DEBUG, tast_block_wrap(*root));
    Tast_block* typed = analysis_1(&env, untyped);
    if (error_count > 0) {
        fail();
    }
    log(LOG_DEBUG, "\n"TAST_FMT, tast_block_print(typed));
    arena_reset(&print_arena);

    typed = change_operators(&env, typed);
    log(LOG_DEBUG, "\n"TAST_FMT, tast_block_print(typed));
    arena_reset(&print_arena);

    typed = remove_tuples(&env, typed);
    log(LOG_DEBUG, "\n"TAST_FMT, tast_block_print(typed));
    arena_reset(&print_arena);
    // TODO: remove llvm_sum

    Llvm_block* llvm_root = add_load_and_store(&env, typed);
    log(LOG_DEBUG, "\n"TAST_FMT, llvm_block_print(llvm_root));
    if (error_count > 0) {
        fail();
    }
    assert(llvm_root);

    llvm_root = assign_llvm_ids(&env, llvm_root);
    log(LOG_DEBUG, "\n"TAST_FMT, llvm_block_print(llvm_root));
    arena_reset(&print_arena);

    if (error_count > 0) {
        unreachable("should have exited before now\n");
    }

    if (params->emit_llvm) {
        emit_llvm_from_tree(&env, llvm_root);
    } else if (params->test_expected_fail) {
        fail();
    } else {
        unreachable("");
    }
}
