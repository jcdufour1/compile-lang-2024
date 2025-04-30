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

static void add_char(const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN,
        lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(
            (Pos) {0}, lang_type_atom_new_from_cstr(base_name, pointer_depth, 0)
        )))
    );
    unwrap(usym_tbl_add(&env.primitives, uast_primitive_def_wrap(def)));
}

static void add_any(const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN,
        lang_type_primitive_const_wrap(lang_type_any_const_wrap(lang_type_any_new(
            POS_BUILTIN, lang_type_atom_new_from_cstr(base_name, pointer_depth, 0)
        )))
    );
    unwrap(usym_tbl_add(&env.primitives, uast_primitive_def_wrap(def)));
}

static void add_void(void) {
    Uast_void_def* def = uast_void_def_new(POS_BUILTIN);
    unwrap(usym_tbl_add(&env.primitives, uast_literal_def_wrap(uast_void_def_wrap(def))));
}

static void add_primitives(void) {
    add_void();
    add_any("any", 0);
}

void do_passes(const Parameters* params) {
    // TODO: do this in a more proper way. this is temporary way to test
    tokenize_do_test();

    memset(&env, 0, sizeof(env));
    add_primitives();

    Uast_block* untyped = NULL;
    if (!parse_file(&untyped, str_view_from_cstr(params->input_file_name), true)) {
        fail();
    }
    assert(error_count < 1);
    log(LOG_DEBUG, "\n"TAST_FMT, uast_block_print(untyped));

    arena_reset(&print_arena);
    Tast_block* typed = NULL;
    if (!try_set_types(&typed, untyped)) {
        log(LOG_DEBUG, "try_set_block_types failed\n");
        assert(error_count > 0);
        fail();
    }
    log(LOG_DEBUG, "try_set_block_types succedded\n");
    assert(error_count < 1);
    
    unwrap(typed);
    arena_reset(&print_arena);
    log(LOG_NOTE, "arena usage: %zu\n", arena_get_total_usage(&a_main));
    log(LOG_DEBUG, "\n"TAST_FMT, tast_block_print(typed));

    Llvm_block* llvm_root = add_load_and_store(typed);
    log(LOG_DEBUG, "\n"TAST_FMT, llvm_block_print(llvm_root));
    if (error_count > 0) {
        fail();
    }
    assert(llvm_root);

    if (error_count > 0) {
        unreachable("should have exited before now\n");
    }

    if (params->emit_llvm) {
        emit_llvm_from_tree(llvm_root);
    } else if (params->test_expected_fail) {
        fail();
    } else {
        unreachable("");
    }
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    do_passes(&params);
    return 0;
}
