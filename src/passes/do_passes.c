
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

static void add_signed_int(Env* env, const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_atom_new_from_cstr(base_name, pointer_depth))))
    );
    try(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
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
    // TODO: come up with a smarter system for primitives
    add_signed_int(env, "i1", 0);
    add_signed_int(env, "i2", 0);
    add_signed_int(env, "i3", 0);
    add_signed_int(env, "i4", 0);
    add_signed_int(env, "i5", 0);
    add_signed_int(env, "i6", 0);
    add_signed_int(env, "i7", 0);
    add_signed_int(env, "i8", 0);
    add_signed_int(env, "i9", 0);
    add_signed_int(env, "i10", 0);
    add_signed_int(env, "i11", 0);
    add_signed_int(env, "i12", 0);
    add_signed_int(env, "i13", 0);
    add_signed_int(env, "i14", 0);
    add_signed_int(env, "i15", 0);
    add_signed_int(env, "i16", 0);
    add_signed_int(env, "i17", 0);
    add_signed_int(env, "i18", 0);
    add_signed_int(env, "i19", 0);
    add_signed_int(env, "i20", 0);
    add_signed_int(env, "i21", 0);
    add_signed_int(env, "i22", 0);
    add_signed_int(env, "i23", 0);
    add_signed_int(env, "i24", 0);
    add_signed_int(env, "i25", 0);
    add_signed_int(env, "i26", 0);
    add_signed_int(env, "i27", 0);
    add_signed_int(env, "i28", 0);
    add_signed_int(env, "i29", 0);
    add_signed_int(env, "i30", 0);
    add_signed_int(env, "i31", 0);
    add_signed_int(env, "i32", 0);
    add_signed_int(env, "i33", 0);
    add_signed_int(env, "i34", 0);
    add_signed_int(env, "i35", 0);
    add_signed_int(env, "i36", 0);
    add_signed_int(env, "i37", 0);
    add_signed_int(env, "i38", 0);
    add_signed_int(env, "i39", 0);
    add_signed_int(env, "i40", 0);
    add_signed_int(env, "i41", 0);
    add_signed_int(env, "i42", 0);
    add_signed_int(env, "i43", 0);
    add_signed_int(env, "i44", 0);
    add_signed_int(env, "i45", 0);
    add_signed_int(env, "i46", 0);
    add_signed_int(env, "i47", 0);
    add_signed_int(env, "i48", 0);
    add_signed_int(env, "i49", 0);
    add_signed_int(env, "i50", 0);
    add_signed_int(env, "i51", 0);
    add_signed_int(env, "i52", 0);
    add_signed_int(env, "i53", 0);
    add_signed_int(env, "i54", 0);
    add_signed_int(env, "i55", 0);
    add_signed_int(env, "i56", 0);
    add_signed_int(env, "i57", 0);
    add_signed_int(env, "i58", 0);
    add_signed_int(env, "i59", 0);
    add_signed_int(env, "i60", 0);
    add_signed_int(env, "i61", 0);
    add_signed_int(env, "i62", 0);
    add_signed_int(env, "i63", 0);
    add_signed_int(env, "i64", 0);

    add_signed_int(env, "u1", 0);
    add_signed_int(env, "u2", 0);
    add_signed_int(env, "u3", 0);
    add_signed_int(env, "u4", 0);
    add_signed_int(env, "u5", 0);
    add_signed_int(env, "u6", 0);
    add_signed_int(env, "u7", 0);
    add_signed_int(env, "u8", 0);
    add_signed_int(env, "u9", 0);
    add_signed_int(env, "u10", 0);
    add_signed_int(env, "u11", 0);
    add_signed_int(env, "u12", 0);
    add_signed_int(env, "u13", 0);
    add_signed_int(env, "u14", 0);
    add_signed_int(env, "u15", 0);
    add_signed_int(env, "u16", 0);
    add_signed_int(env, "u17", 0);
    add_signed_int(env, "u18", 0);
    add_signed_int(env, "u19", 0);
    add_signed_int(env, "u20", 0);
    add_signed_int(env, "u21", 0);
    add_signed_int(env, "u22", 0);
    add_signed_int(env, "u23", 0);
    add_signed_int(env, "u24", 0);
    add_signed_int(env, "u25", 0);
    add_signed_int(env, "u26", 0);
    add_signed_int(env, "u27", 0);
    add_signed_int(env, "u28", 0);
    add_signed_int(env, "u29", 0);
    add_signed_int(env, "u30", 0);
    add_signed_int(env, "u31", 0);
    add_signed_int(env, "u32", 0);
    add_signed_int(env, "u33", 0);
    add_signed_int(env, "u34", 0);
    add_signed_int(env, "u35", 0);
    add_signed_int(env, "u36", 0);
    add_signed_int(env, "u37", 0);
    add_signed_int(env, "u38", 0);
    add_signed_int(env, "u39", 0);
    add_signed_int(env, "u40", 0);
    add_signed_int(env, "u41", 0);
    add_signed_int(env, "u42", 0);
    add_signed_int(env, "u43", 0);
    add_signed_int(env, "u44", 0);
    add_signed_int(env, "u45", 0);
    add_signed_int(env, "u46", 0);
    add_signed_int(env, "u47", 0);
    add_signed_int(env, "u48", 0);
    add_signed_int(env, "u49", 0);
    add_signed_int(env, "u50", 0);
    add_signed_int(env, "u51", 0);
    add_signed_int(env, "u52", 0);
    add_signed_int(env, "u53", 0);
    add_signed_int(env, "u54", 0);
    add_signed_int(env, "u55", 0);
    add_signed_int(env, "u56", 0);
    add_signed_int(env, "u57", 0);
    add_signed_int(env, "u58", 0);
    add_signed_int(env, "u59", 0);
    add_signed_int(env, "u60", 0);
    add_signed_int(env, "u61", 0);
    add_signed_int(env, "u62", 0);
    add_signed_int(env, "u63", 0);
    add_signed_int(env, "u64", 0);

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
