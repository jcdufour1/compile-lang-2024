
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

static void add_primitive(Env* env, const char* base_name, int16_t pointer_depth) {
    Uast_primitive_def* def = uast_primitive_def_new(
        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_primitive_new(lang_type_atom_new_from_cstr(base_name, pointer_depth)))
    );
    try(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
}

static void add_void(Env* env) {
    Uast_void_def* def = uast_void_def_new(POS_BUILTIN);
    try(usym_tbl_add(&env->primitives, uast_literal_def_wrap(uast_void_def_wrap(def))));
}

static void add_primitives(Env* env) {
    // TODO: come up with a smarter system for primitives
    add_primitive(env, "i1", 0);
    add_primitive(env, "i2", 0);
    add_primitive(env, "i3", 0);
    add_primitive(env, "i4", 0);
    add_primitive(env, "i5", 0);
    add_primitive(env, "i6", 0);
    add_primitive(env, "i7", 0);
    add_primitive(env, "i8", 0);
    add_primitive(env, "i9", 0);
    add_primitive(env, "i10", 0);
    add_primitive(env, "i11", 0);
    add_primitive(env, "i12", 0);
    add_primitive(env, "i13", 0);
    add_primitive(env, "i14", 0);
    add_primitive(env, "i15", 0);
    add_primitive(env, "i16", 0);
    add_primitive(env, "i17", 0);
    add_primitive(env, "i18", 0);
    add_primitive(env, "i19", 0);
    add_primitive(env, "i20", 0);
    add_primitive(env, "i21", 0);
    add_primitive(env, "i22", 0);
    add_primitive(env, "i23", 0);
    add_primitive(env, "i24", 0);
    add_primitive(env, "i25", 0);
    add_primitive(env, "i26", 0);
    add_primitive(env, "i27", 0);
    add_primitive(env, "i28", 0);
    add_primitive(env, "i29", 0);
    add_primitive(env, "i30", 0);
    add_primitive(env, "i31", 0);
    add_primitive(env, "i32", 0);
    add_primitive(env, "i33", 0);
    add_primitive(env, "i34", 0);
    add_primitive(env, "i35", 0);
    add_primitive(env, "i36", 0);
    add_primitive(env, "i37", 0);
    add_primitive(env, "i38", 0);
    add_primitive(env, "i39", 0);
    add_primitive(env, "i40", 0);
    add_primitive(env, "i41", 0);
    add_primitive(env, "i42", 0);
    add_primitive(env, "i43", 0);
    add_primitive(env, "i44", 0);
    add_primitive(env, "i45", 0);
    add_primitive(env, "i46", 0);
    add_primitive(env, "i47", 0);
    add_primitive(env, "i48", 0);
    add_primitive(env, "i49", 0);
    add_primitive(env, "i50", 0);
    add_primitive(env, "i51", 0);
    add_primitive(env, "i52", 0);
    add_primitive(env, "i53", 0);
    add_primitive(env, "i54", 0);
    add_primitive(env, "i55", 0);
    add_primitive(env, "i56", 0);
    add_primitive(env, "i57", 0);
    add_primitive(env, "i58", 0);
    add_primitive(env, "i59", 0);
    add_primitive(env, "i60", 0);
    add_primitive(env, "i61", 0);
    add_primitive(env, "i62", 0);
    add_primitive(env, "i63", 0);
    add_primitive(env, "i64", 0);

    add_primitive(env, "u1", 0);
    add_primitive(env, "u2", 0);
    add_primitive(env, "u3", 0);
    add_primitive(env, "u4", 0);
    add_primitive(env, "u5", 0);
    add_primitive(env, "u6", 0);
    add_primitive(env, "u7", 0);
    add_primitive(env, "u8", 0);
    add_primitive(env, "u9", 0);
    add_primitive(env, "u10", 0);
    add_primitive(env, "u11", 0);
    add_primitive(env, "u12", 0);
    add_primitive(env, "u13", 0);
    add_primitive(env, "u14", 0);
    add_primitive(env, "u15", 0);
    add_primitive(env, "u16", 0);
    add_primitive(env, "u17", 0);
    add_primitive(env, "u18", 0);
    add_primitive(env, "u19", 0);
    add_primitive(env, "u20", 0);
    add_primitive(env, "u21", 0);
    add_primitive(env, "u22", 0);
    add_primitive(env, "u23", 0);
    add_primitive(env, "u24", 0);
    add_primitive(env, "u25", 0);
    add_primitive(env, "u26", 0);
    add_primitive(env, "u27", 0);
    add_primitive(env, "u28", 0);
    add_primitive(env, "u29", 0);
    add_primitive(env, "u30", 0);
    add_primitive(env, "u31", 0);
    add_primitive(env, "u32", 0);
    add_primitive(env, "u33", 0);
    add_primitive(env, "u34", 0);
    add_primitive(env, "u35", 0);
    add_primitive(env, "u36", 0);
    add_primitive(env, "u37", 0);
    add_primitive(env, "u38", 0);
    add_primitive(env, "u39", 0);
    add_primitive(env, "u40", 0);
    add_primitive(env, "u41", 0);
    add_primitive(env, "u42", 0);
    add_primitive(env, "u43", 0);
    add_primitive(env, "u44", 0);
    add_primitive(env, "u45", 0);
    add_primitive(env, "u46", 0);
    add_primitive(env, "u47", 0);
    add_primitive(env, "u48", 0);
    add_primitive(env, "u49", 0);
    add_primitive(env, "u50", 0);
    add_primitive(env, "u51", 0);
    add_primitive(env, "u52", 0);
    add_primitive(env, "u53", 0);
    add_primitive(env, "u54", 0);
    add_primitive(env, "u55", 0);
    add_primitive(env, "u56", 0);
    add_primitive(env, "u57", 0);
    add_primitive(env, "u58", 0);
    add_primitive(env, "u59", 0);
    add_primitive(env, "u60", 0);
    add_primitive(env, "u61", 0);
    add_primitive(env, "u62", 0);
    add_primitive(env, "u63", 0);
    add_primitive(env, "u64", 0);

    add_void(env);
    add_primitive(env, "any", 0);
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
