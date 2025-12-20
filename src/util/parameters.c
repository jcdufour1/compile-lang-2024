#include <parameters.h>
#include <file.h>
#include <msg.h>
#include <util.h>
#include <str_and_num_utils.h>
#include <ast_msg.h>

typedef struct {
    Strv text;
    Pos pos;
} Arg;

static void arg_consume_count(Arg* arg, size_t count) {
    strv_consume_count(&arg->text, count);
    arg->pos.column += count;
}

static Strv compiler_exe_name;

static void print_usage(Pos pos);

typedef struct {
    TARGET_ARCH arch;
    const char* arch_cstr;
    unsigned int sizeof_usize;
    unsigned int sizeof_ptr_non_fn;
} Arch_row;
static Arch_row arch_table[] = {
    {.arch = ARCH_X86_64, .arch_cstr = "x86_64", .sizeof_usize = 64, .sizeof_ptr_non_fn = 64},
};

static struct {
    TARGET_VENDOR vendor;
    const char* vendor_cstr;
} vendor_table[] = {
    {.vendor = VENDOR_UNKNOWN, .vendor_cstr = "unknown"},
    {.vendor = VENDOR_PC, .vendor_cstr = "pc"},
};

static struct {
    TARGET_OS os;
    const char* os_cstr;
} os_table[] = {
    {.os = OS_LINUX, .os_cstr = "linux"},
    {.os = OS_WINDOWS, .os_cstr = "windows"},
};

static struct {
    TARGET_ABI abi;
    const char* abi_cstr;
} abi_table[] = {
    {.abi = ABI_GNU, .abi_cstr = "gnu"},
};

static_assert(array_count(arch_table) == 1, "exhausive handling of architectures");
static TARGET_ARCH get_default_arch(void) {
#   if defined(__x86_64__) || defined(_M_X64)
        return ARCH_X86_64;
#   else
        // TODO: return ARCH_UNKNOWN?
#       error "unsupported architecture"
#   endif
}

static TARGET_VENDOR get_default_vendor(void) {
    return VENDOR_UNKNOWN; // TODO
}

static_assert(array_count(os_table) == 2, "exhausive handling of operating systems");
static TARGET_OS get_default_os(void) {
    // TODO: add ifdef for windows
#   ifdef __linux__
        return OS_LINUX;
#   else
        // TODO: return OS_UNKNOWN?
#       error "unsupported operating system"
#   endif
}

static_assert(array_count(abi_table) == 1, "exhausive handling of abis");
static TARGET_ABI get_default_abi(void) {
#   ifdef __GLIBC__
        return ABI_GNU;
#   else
        // TODO: return ABI_UNKNOWN?
#       error "unsupported abi"
#   endif
}

static Arch_row get_arch_row_from_arch(TARGET_ARCH arch) {
    for (size_t idx = 0; idx < array_count(arch_table); idx++) {
        if (array_at(arch_table, idx).arch == arch) {
            return array_at(arch_table, idx);
        }
    }
    unreachable("");
}

Strv strv_from_target_arch(TARGET_ARCH arch) {
    return sv(get_arch_row_from_arch(arch).arch_cstr);
}

Strv strv_from_target_vendor(TARGET_VENDOR vendor) {
    for (size_t idx = 0; idx < array_count(vendor_table); idx++) {
        if (array_at(vendor_table, idx).vendor == vendor) {
            return sv(array_at(vendor_table, idx).vendor_cstr);
        }
    }
    unreachable("");
}

Strv strv_from_target_os(TARGET_OS os) {
    for (size_t idx = 0; idx < array_count(os_table); idx++) {
        if (array_at(os_table, idx).os == os) {
            return sv(array_at(os_table, idx).os_cstr);
        }
    }
    unreachable("");
}

Strv strv_from_target_abi(TARGET_ABI abi) {
    for (size_t idx = 0; idx < array_count(abi_table); idx++) {
        if (array_at(abi_table, idx).abi == abi) {
            return sv(array_at(abi_table, idx).abi_cstr);
        }
    }
    unreachable("");
}

static bool try_target_arch_from_strv(TARGET_ARCH* arch, Strv strv) {
    for (size_t idx = 0; idx < array_count(arch_table); idx++) {
        if (strv_is_equal(sv(array_at(arch_table, idx).arch_cstr), strv)) {
            *arch = array_at(arch_table, idx).arch;
            return true;
        }
    }
    return false;
}

static bool try_target_vendor_from_strv(TARGET_VENDOR* vendor, Strv strv) {
    for (size_t idx = 0; idx < array_count(vendor_table); idx++) {
        if (strv_is_equal(sv(array_at(vendor_table, idx).vendor_cstr), strv)) {
            *vendor = array_at(vendor_table, idx).vendor;
            return true;
        }
    }
    return false;
}

static bool try_target_os_from_strv(TARGET_OS* os, Strv strv) {
    for (size_t idx = 0; idx < array_count(os_table); idx++) {
        if (strv_is_equal(sv(array_at(os_table, idx).os_cstr), strv)) {
            *os = array_at(os_table, idx).os;
            return true;
        }
    }
    return false;
}

static bool try_target_abi_from_strv(TARGET_ABI* abi, Strv strv) {
    for (size_t idx = 0; idx < array_count(abi_table); idx++) {
        if (strv_is_equal(sv(array_at(abi_table, idx).abi_cstr), strv)) {
            *abi = array_at(abi_table, idx).abi;
            return true;
        }
    }
    return false;
}

bool target_triplet_is_equal(Target_triplet a, Target_triplet b) {
    return a.arch == b.arch && a.vendor == b.vendor && a.os == b.os && a.abi == b.abi;
}

Strv target_triplet_print_internal(Target_triplet triplet) {
    String buf = {0};

    string_extend_strv(&a_temp, &buf, strv_from_target_arch(triplet.arch));
    string_extend_cstr(&a_temp, &buf, "-");
    string_extend_strv(&a_temp, &buf, strv_from_target_os(triplet.os));
    string_extend_cstr(&a_temp, &buf, "-");
    string_extend_strv(&a_temp, &buf, strv_from_target_abi(triplet.abi));

    return string_to_strv(buf);
}

#define target_triplet_print(triplet) strv_print(target_triplet_print_internal(triplet))

Target_triplet get_default_target_triplet(void) {
    return (Target_triplet) {
        .arch = get_default_arch(),
        .vendor = get_default_vendor(),
        .os = get_default_os(),
        .abi = get_default_abi(),
    };
}

Strv stop_after_print_internal(STOP_AFTER stop_after) {
    switch (stop_after) {
        case STOP_AFTER_NONE:
            return sv("none");
        case STOP_AFTER_IR:
            return sv("gen_ir");
        case STOP_AFTER_BACKEND_IR:
            return sv("gen_backend_ir");
        case STOP_AFTER_UPPER_S:
            return sv("lower_s");
        case STOP_AFTER_OBJ:
            return sv("obj");
        case STOP_AFTER_BIN:
            return sv("bin");
        case STOP_AFTER_RUN:
            return sv("run");
        case STOP_AFTER_COUNT:
            unreachable("");
    }
    unreachable("");
}

bool is_compiling(void) {
    static_assert(
        STOP_AFTER_COUNT == 7,
        "exhausive handling of stop after states (not all states are explicitly handled)"
    );
    return params.stop_after == STOP_AFTER_RUN || params.stop_after == STOP_AFTER_BIN;
}

static void stop_after_set_if_none(STOP_AFTER set_to) {
    if (params.stop_after == STOP_AFTER_NONE) {
        params.stop_after = set_to;
    }
}

static bool is_long_option(char** argv) {
    if (strlen(argv[0]) < 2) {
        return false;
    }

    return argv[0][0] == '-' && argv[0][1] == '-';
}

static bool is_short_option(char** argv) {
    return !is_long_option(argv) && argv[0][0] == '-';
}

// this function will exclude - or -- part of arg if present
// TODO: consume_arg should accept printf style format for msg_if_missing?
static Arg consume_arg(int* argc, char*** argv, Strv msg_if_missing) {
    static uint32_t curr_col = 0;

    if (*argc < 1) {
        msg(
            DIAG_MISSING_COMMAND_LINE_ARG,
            (Pos) {.file_path = MOD_PATH_COMMAND_LINE, .line = 0, .column = curr_col, .expanded_from = NULL},
            FMT"\n",
            strv_print(msg_if_missing)
        );
        local_exit(EXIT_CODE_FAIL);
    }
    const char* curr_arg = argv[0][0];
    while (curr_arg[0] && curr_arg[0] == '-') {
        curr_arg++;
        curr_col++;
    }
    (*argv)++;
    (*argc)--;
    curr_col++;

    if (curr_arg[0]) {
        Arg new_arg = {.text = sv(curr_arg), .pos = {
            .file_path = MOD_PATH_COMMAND_LINE,
            .line = 0,
            .column = curr_col,
            .expanded_from = NULL
        }};
        curr_col += strlen(curr_arg);
        return new_arg;
    }

    return consume_arg(argc, argv, sv("stray `-` or `--` is not allowed"));
}

typedef struct {
    const char* str;
    DIAG_TYPE type;
    LOG_LEVEL default_level;
    bool must_be_error;
} Expect_fail_pair;

typedef struct {
    const char* str;
    LOG_LEVEL curr_level;
} Expect_fail_str_to_curr_log_level;

static_assert(DIAG_COUNT == 116, "exhaustive handling of expected fail types");
static const Expect_fail_pair expect_fail_pair[] = {
    {.str = "info", .type = DIAG_INFO, .default_level = LOG_INFO, .must_be_error = false},
    {.str = "note", .type = DIAG_NOTE, .default_level = LOG_NOTE, .must_be_error = false},
    {.str = "file-built", .type = DIAG_FILE_BUILT, .default_level = LOG_VERBOSE, .must_be_error = false},
    {.str = "missing-command-line-arg", .type = DIAG_MISSING_COMMAND_LINE_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "file-could-not-open", .type = DIAG_FILE_COULD_NOT_OPEN, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "directory-could-not-be-made", .type = DIAG_DIR_COULD_NOT_BE_MADE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "file-could-not-read", .type = DIAG_FILE_COULD_NOT_READ, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "diag-enum-non-void-case-no-par-on-assign", .type = DIAG_ENUM_NON_VOID_CASE_NO_PAR_ON_ASSIGN, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "diag-function-param-not-specified", .type = DIAG_FUNCTION_PARAM_NOT_SPECIFIED, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-double-quote", .type = DIAG_MISSING_CLOSE_DOUBLE_QUOTE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-single-quote", .type = DIAG_MISSING_CLOSE_SINGLE_QUOTE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "no-new-line-after-statement", .type = DIAG_NO_NEW_LINE_AFTER_STATEMENT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-par", .type = DIAG_MISSING_CLOSE_PAR, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-curly-brace", .type = DIAG_MISSING_CLOSE_CURLY_BRACE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-extern-type", .type = DIAG_INVALID_EXTERN_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-token", .type = DIAG_INVALID_TOKEN, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "parser-expected", .type = DIAG_PARSER_EXPECTED, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "redefinition-of-symbol", .type = DIAG_REDEFINITION_SYMBOL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-struct-member-in-literal", .type = DIAG_INVALID_MEMBER_IN_LITERAL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-member-access-callee", .type = DIAG_INVALID_MEMBER_ACCESS_CALLEE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-member-access", .type = DIAG_INVALID_MEMBER_ACCESS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-return-in-fun", .type = DIAG_MISSING_RETURN_IN_FUN, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-return-in-defer", .type = DIAG_MISSING_RETURN_IN_DEFER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-count-fun-args", .type = DIAG_INVALID_COUNT_FUN_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-function-arg", .type = DIAG_INVALID_FUN_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "mismatched-return-type", .type = DIAG_MISMATCHED_RETURN_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "mismatched-yield-type", .type = DIAG_MISMATCHED_YIELD_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "mismatched-closing-curly-brace", .type = DIAG_MISMATCHED_CLOSING_CURLY_BRACE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "assignment-mismatched-types", .type = DIAG_ASSIGNMENT_MISMATCHED_TYPES, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "unary-mismatched-types", .type = DIAG_UNARY_MISMATCHED_TYPES, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "binary-mismatched-types", .type = DIAG_BINARY_MISMATCHED_TYPES, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "expected-expression", .type = DIAG_EXPECTED_EXPRESSION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "undefined-symbol", .type = DIAG_UNDEFINED_SYMBOL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "undefined-function", .type = DIAG_UNDEFINED_FUNCTION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "struct-init-on-raw-union", .type = DIAG_STRUCT_INIT_ON_RAW_UNION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "struct-init-on-enum", .type = DIAG_STRUCT_INIT_ON_ENUM, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "struct-init-on-primitive", .type = DIAG_STRUCT_INIT_ON_PRIMITIVE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "undefined-type", .type = DIAG_UNDEFINED_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-sq-bracket", .type = DIAG_MISSING_CLOSE_SQ_BRACKET, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-generic", .type = DIAG_MISSING_CLOSE_GENERIC, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "deref_non_pointer", .type = DIAG_DEREF_NON_POINTER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "break-invalid-location", .type = DIAG_BREAK_INVALID_LOCATION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "continue-invalid-location", .type = DIAG_CONTINUE_INVALID_LOCATION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "mismatched-tuple-count", .type = DIAG_MISMATCHED_TUPLE_COUNT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "non-exhaustive-switch", .type = DIAG_NON_EXHAUSTIVE_SWITCH, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "enum-lit-invalid-arg", .type = DIAG_ENUM_LIT_INVALID_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "not-yet-implemented", .type = DIAG_NOT_YET_IMPLEMENTED, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "soft-not-yet-implemented", .type = DIAG_SOFT_NOT_YET_IMPLEMENTED, .default_level = LOG_WARNING, .must_be_error = true},
    {.str = "duplicate-default", .type = DIAG_DUPLICATE_DEFAULT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "duplicate-case", .type = DIAG_DUPLICATE_CASE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-count-generic-args", .type = DIAG_INVALID_COUNT_GENERIC_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-yield-statement", .type = DIAG_MISSING_YIELD_STATEMENT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-hex", .type = DIAG_INVALID_HEX, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-bin", .type = DIAG_INVALID_BIN, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-octal", .type = DIAG_INVALID_OCTAL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-char-lit", .type = DIAG_INVALID_CHAR_LIT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-decimal-lit", .type = DIAG_INVALID_DECIMAL_LIT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-float-lit", .type = DIAG_INVALID_FLOAT_LIT, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-close-multiline", .type = DIAG_MISSING_CLOSE_MULTILINE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-count-struct-lit-args", .type = DIAG_INVALID_COUNT_STRUCT_LIT_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "missing-enum-arg", .type = DIAG_MISSING_ENUM_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "enum-case-too-mopaque-args", .type = DIAG_ENUM_CASE_TOO_MOPAQUE_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "void-enum-case-has-arg", .type = DIAG_VOID_ENUM_CASE_HAS_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-stmt-top-level", .type = DIAG_INVALID_STMT_TOP_LEVEL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-function-callee", .type = DIAG_INVALID_FUNCTION_CALLEE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "optional-args-for-variadic-args", .type = DIAG_OPTIONAL_ARGS_FOR_VARIADIC_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-fail-type", .type = DIAG_INVALID_FAIL_TYPE, .default_level = LOG_ERROR, .must_be_error = false},
    {.str = "no-main-function", .type = DIAG_NO_MAIN_FUNCTION, .default_level = LOG_WARNING, .must_be_error = false},
    {.str = "struct-like-recursion", .type = DIAG_STRUCT_LIKE_RECURSION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "child-process-failure", .type = DIAG_CHILD_PROCESS_FAILURE, .default_level = LOG_FATAL, .must_be_error = true},
    {.str = "no-input-files", .type = DIAG_NO_INPUT_FILES, .default_level = LOG_FATAL, .must_be_error = true},
    {.str = "return-in-defer", .type = DIAG_RETURN_IN_DEFER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "break-out-of-defer", .type = DIAG_BREAK_OUT_OF_DEFER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "continue-out-of-defer", .type = DIAG_CONTINUE_OUT_OF_DEFER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "assignment-to-void", .type = DIAG_ASSIGNMENT_TO_VOID, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "if-else-in-if-let", .type = DIAG_IF_ELSE_IN_IF_LET, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "unknown-on-non-enum-type", .type = DIAG_UNKNOWN_ON_NON_ENUM_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-label-pos", .type = DIAG_INVALID_LABEL_POS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-countof", .type = DIAG_INVALID_COUNTOF, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "redef-struct-base-member", .type = DIAG_REDEF_STRUCT_BASE_MEMBER, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "wrong-gen-type", .type = DIAG_WRONG_GEN_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "uninitialized-variable", .type = DIAG_UNINITIALIZED_VARIABLE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "switch-no-cases", .type = DIAG_SWITCH_NO_CASES, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "using-on-non-struct-or-mod-alias", .type = DIAG_USING_ON_NON_STRUCT_OR_MOD_ALIAS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "file-invalid-name", .type = DIAG_FILE_INVALID_NAME, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "gen-infer-more-than-64-wide", .type = DIAG_GEN_INFER_MORE_THAN_64_WIDE, .default_level = LOG_WARNING, .must_be_error = false},
    {.str = "if-should-be-if-let", .type = DIAG_IF_SHOULD_BE_IF_LET, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "unsupported-target-triplet", .type = DIAG_UNSUPPORTED_TARGET_TRIPLET, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-literal-prefix", .type = DIAG_INVALID_LITERAL_PREFIX, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "def-recursion", .type = DIAG_DEF_RECURSION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "not-lvalue", .type = DIAG_NOT_LVALUE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-o-cmd-opt", .type = DIAG_INVALID_O_CMD_OPT, .default_level = LOG_FATAL, .must_be_error = true},
    {.str = "cmd-opt-invalid-syntax", .type = DIAG_CMD_OPT_INVALID_SYNTAX, .default_level = LOG_FATAL, .must_be_error = true},
    {.str = "cmd-opt-invalid-option", .type = DIAG_CMD_OPT_INVALID_OPTION, .default_level = LOG_FATAL, .must_be_error = true},
    {.str = "lang-def-in-runtime", .type = DIAG_LANG_DEF_IN_RUNTIME, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "type-could-not-be-infered", .type = DIAG_TYPE_COULD_NOT_BE_INFERED, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "expected-type-but-got-expr", .type = DIAG_EXPECTED_TYPE_BUT_GOT_EXPR, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "expected-expr-but-got-type", .type = DIAG_EXPECTED_EXPR_BUT_GOT_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-type", .type = DIAG_INVALID_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "non-int-literal-in-static-array", .type = DIAG_NON_INT_LITERAL_IN_STATIC_ARRAY, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "expected-variable-def", .type = DIAG_EXPECTED_VARIABLE_DEF, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "using-on-non-struct-variable", .type = DIAG_USING_ON_NON_STRUCT_VARIABLE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "def-dest-and-src-both-have-gen-args", .type = DIAG_DEF_DEST_AND_SRC_BOTH_HAVE_GEN_ARGS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "yield-out-of-error-handling-block", .type = DIAG_YIELD_OUT_OF_ERROR_HANDLING_BLOCK, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "orelse-err-symbol-on-optional", .type = DIAG_ORELSE_ERR_SYMBOL_ON_OPTIONAL, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "mismatched-error-t", .type = DIAG_MISMATCHED_ERROR_T, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "result-expected-on-question-mark-lhs", .type = DIAG_RESULT_EXPECTED_ON_QUESTION_MARK_LHS, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "assignment-used-as-expression", .type = DIAG_ASSIGNMENT_USED_AS_EXPRESSION, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "if-let-invalid_syntax", .type = DIAG_IF_LET_INVALID_SYNTAX, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "static-array-count-not-infered", .type = DIAG_STATIC_ARRAY_COUNT_NOT_INFERED, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-index-callee", .type = DIAG_INVALID_INDEX_CALLEE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "positional-arg-after-designated-arg", .type = DIAG_POSITIONAL_ARG_AFTER_DESIGNATED_ARG, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "got-type-but-expected-expr", .type = DIAG_GOT_TYPE_BUT_EXPECTED_EXPR, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "got-expr-but-expected-type", .type = DIAG_GOT_EXPR_BUT_EXPECTED_TYPE, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-attr", .type = DIAG_INVALID_ATTR, .default_level = LOG_ERROR, .must_be_error = true},
    {.str = "invalid-string-literal", .type = DIAG_INVALID_STRING_LITERAL, .default_level = LOG_ERROR, .must_be_error = true},
};

// error types are in the same order in expect_fail_str_to_curr_log_level_pair and expect_fail_pair
// TODO: make these names less verbose?
static Expect_fail_str_to_curr_log_level 
expect_fail_str_to_curr_log_level_pair[sizeof(expect_fail_pair)/sizeof(expect_fail_pair[0])] = {0};

bool expect_fail_type_from_strv(size_t* idx_result, DIAG_TYPE* type, Strv strv) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        if (strv_is_equal(sv(array_at(expect_fail_pair, idx).str), strv)) {
            *type = expect_fail_pair[idx].type;
            *idx_result = idx;
            return true;
        }
    }
    return false;
}

static size_t expect_fail_type_get_idx(DIAG_TYPE type) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        if (expect_fail_pair[idx].type == type) {
            return idx;
        }
    }
    unreachable("expect_fail_pair does not cover this DIAG_TYPE");
}

Strv expect_fail_type_print_internal(DIAG_TYPE type) {
    return sv(expect_fail_pair[expect_fail_type_get_idx(type)].str);
}

bool expect_fail_type_is_type_inference_error(DIAG_TYPE type) {
    return type == DIAG_TYPE_COULD_NOT_BE_INFERED;
}

static void expect_fail_str_to_curr_log_level_init(void) {
    for (size_t idx = 0; idx < array_count(expect_fail_pair); idx++) {
        expect_fail_str_to_curr_log_level_pair[idx].str = expect_fail_pair[idx].str;
        expect_fail_str_to_curr_log_level_pair[idx].curr_level = expect_fail_pair[idx].default_level;
    }
}

LOG_LEVEL expect_fail_type_to_curr_log_level(DIAG_TYPE type) {
    return array_at(expect_fail_str_to_curr_log_level_pair, expect_fail_type_get_idx(type)).curr_level;
}

static void parse_file_option(int* argc, char*** argv) {
    Arg curr_opt = consume_arg(argc, argv, sv("arg expected"));
    assert(strv_is_equal(curr_opt.pos.file_path, MOD_PATH_COMMAND_LINE));

    static_assert(
        PARAMETERS_COUNT == 32,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    static_assert(FILE_TYPE_COUNT == 7, "exhaustive handling of file types");

    FILE_TYPE file_type = 0;
    Strv err_text = {0};
    if (!get_file_type(&file_type, &err_text, curr_opt.text)) {
        msg_todo_strv(err_text, curr_opt.pos);
        local_exit(EXIT_CODE_FAIL);
    }
    switch (file_type) {
        case FILE_TYPE_OWN:
            if (is_compiling()) {
                msg_todo("multiple .own files specified on the command line", curr_opt.pos);
                local_exit(EXIT_CODE_FAIL);
            }
            stop_after_set_if_none(STOP_AFTER_BIN);
            params.compile_own = true;
            params.input_file_path = curr_opt.text;
            break;
        case FILE_TYPE_STATIC_LIB:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.static_libs, curr_opt.text);
            break;
        case FILE_TYPE_DYNAMIC_LIB:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.dynamic_libs, curr_opt.text);
            break;
        case FILE_TYPE_C:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.c_input_files, curr_opt.text);
            break;
        case FILE_TYPE_OBJECT:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.object_files, curr_opt.text);
            break;
        case FILE_TYPE_LOWER_S:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.lower_s_files, curr_opt.text);
            break;
        case FILE_TYPE_UPPER_S:
            stop_after_set_if_none(STOP_AFTER_BIN);
            darr_append(&a_main, &params.upper_s_files, curr_opt.text);
            break;
        case FILE_TYPE_COUNT:
            unreachable("");
        default:
            unreachable("");
    }
}

static void set_backend(BACKEND backend) {
    switch (backend) {
        case BACKEND_C:
            params.backend_info.backend = BACKEND_C;
            params.backend_info.struct_rtn_through_param = false;
            return;
        case BACKEND_LLVM:
            params.backend_info.backend = BACKEND_LLVM;
            params.backend_info.struct_rtn_through_param = true;
            return;
        case BACKEND_NONE:
            unreachable("");
    }
    unreachable("");
}

typedef void(*Long_option_action)(Pos pos_self, Arg curr_opt);

typedef enum {
    ARG_NONE,
    ARG_SINGLE,
    ARG_REMAINING_RUN_ONLY,

    // for static asserts
    ARG_COUNT,
} LONG_OPTION_ARG_TYPE;

typedef struct {
    const char* text;
    const char* description;
    Long_option_action action;
    LONG_OPTION_ARG_TYPE arg_type;
} Long_option_pair;

static void long_option_help(Pos pos_self, Arg curr_opt) {
    (void) curr_opt;
    print_usage(pos_self);
    local_exit(EXIT_CODE_SUCCESS);
}

static void long_option_l(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    darr_append(&a_main, &params.l_flags, curr_opt.text);
}

static void long_option_backend(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv backend = curr_opt.text;

    if (strv_is_equal(backend, sv("c"))) {
        set_backend(BACKEND_C);
    } else if (strv_is_equal(backend, sv("llvm"))) {
        set_backend(BACKEND_LLVM);
    } else {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "backend `"FMT"` is not a supported backend\n", strv_print(backend));
        local_exit(EXIT_CODE_FAIL);
    }
}

static void long_option_all_errors_fetal(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.all_errors_fatal = true;
}

static void long_option_dump_backend_ir(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.stop_after = STOP_AFTER_BACKEND_IR;
}

static void long_option_dump_ir(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.stop_after = STOP_AFTER_IR;
}

static void long_option_upper_s(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.stop_after = STOP_AFTER_UPPER_S;
}

static void long_option_upper_c(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.stop_after = STOP_AFTER_OBJ;
}

static void long_option_dump_dot(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    msg_todo("dump_dot", curr_opt.pos);
    //params.stop_after = STOP_AFTER_IR;
    //params.dump_dot = true;
}

static void long_option_run(Pos pos_self, Strv first_arg, int* argc, char *** argv) {
    static_assert(
        PARAMETERS_COUNT == 32,
        "exhausive handling of params for if statement below "
        "(not all parameters are explicitly handled)"
    );
    if (params.stop_after == STOP_AFTER_NONE) {
        msg(DIAG_CMD_OPT_INVALID_SYNTAX, pos_self, "file to be compiled must be specified prior to `--run` argument\n");
        local_exit(EXIT_CODE_FAIL);
    }
    if (!is_compiling()) {
        msg(
            DIAG_CMD_OPT_INVALID_OPTION,
            pos_self,
            "`--run` option cannot be used when generating intermediate files (eg. .s files)\n"
        );
        local_exit(EXIT_CODE_FAIL);
    }
    params.stop_after = STOP_AFTER_RUN;

    if (first_arg.count > 0) {
        darr_append(&a_leak, &params.run_args, first_arg);
    }
    while (*argc > 0) {
        darr_append(&a_leak, &params.run_args, consume_arg(argc, argv, sv("internal error")).text);
    }
}

static void long_option_lower_o(Pos pos_self, Arg curr_opt) {
    params.is_output_file_path = true;
    params.output_file_path = curr_opt.text;
    params.pos_lower_o = pos_self;
}

static void long_option_upper_o0(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_O0;
}

static void long_option_upper_og(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_OG;
}

static void long_option_upper_o1(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_O1;
}

static void long_option_upper_o2(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_O2;
}

static void long_option_upper_os(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.opt_level = OPT_LEVEL_OS;
}

static void long_option_error(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv error = curr_opt.text;

    DIAG_TYPE type = {0};
    size_t idx = 0;
    if (!expect_fail_type_from_strv(&idx, &type, error)) {
        msg(
            DIAG_INVALID_FAIL_TYPE, curr_opt.pos,
            "invalid fail type `"FMT"`\n", strv_print(error)
        );
        local_exit(EXIT_CODE_FAIL);
    }
    array_at_ref(expect_fail_str_to_curr_log_level_pair, idx)->curr_level = LOG_ERROR;
    params.error_opts_changed = true;
}

static void long_option_path_c_compiler(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv cc = curr_opt.text;

    params.path_c_compiler = cc;
    params.is_path_c_compiler = true;
}

static void long_option_target_triplet(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv cc = curr_opt.text;

    Strv temp[4] = {0};

    for (size_t idx = 0; idx < 3; idx++) {
        if (!strv_try_consume_until(&temp[idx], &cc, '-')) {
            size_t count_substrings = idx;
            if (cc.count > 0) {
                count_substrings++;
            }
            msg(
                DIAG_CMD_OPT_INVALID_SYNTAX,
                curr_opt.pos,
                "target triplet has only %zu substrings, but 4 was expected\n",
                count_substrings
            );
            local_exit(EXIT_CODE_FAIL);
        }
        strv_consume(&cc);
    }
    Strv dummy = {0};
    if (strv_try_consume_until(&dummy, &cc, '-')) {
        msg(DIAG_CMD_OPT_INVALID_SYNTAX, curr_opt.pos, "target triplet has too many substrings (4 was expected)\n");
        local_exit(EXIT_CODE_FAIL);
    }
    temp[3] = cc;

    if (!try_target_arch_from_strv(&params.target_triplet.arch, temp[0])) {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "unsupported architecture `"FMT"`\n", strv_print(temp[0]));
        local_exit(EXIT_CODE_FAIL);
    }

    if (!try_target_vendor_from_strv(&params.target_triplet.vendor, temp[1])) {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "unsupported vendor `"FMT"`\n", strv_print(temp[1]));
        local_exit(EXIT_CODE_FAIL);
    }

    if (!try_target_os_from_strv(&params.target_triplet.os, temp[2])) {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "unsupported operating system `"FMT"`\n", strv_print(temp[2]));
        local_exit(EXIT_CODE_FAIL);
    }

    if (!try_target_abi_from_strv(&params.target_triplet.abi, temp[3])) {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "unsupported abi (application binary interface, a.k.a. environment type) `"FMT"`\n", strv_print(temp[3]));
        local_exit(EXIT_CODE_FAIL);
    }
}

static void long_option_print_immediately(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.print_immediately = true;
}

static void long_option_build_dir(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.build_dir = curr_opt.text;
}

static void long_option_no_prelude(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
    params.do_prelude = false;
}

static void long_option_log_level(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv log_level = curr_opt.text;

    if (strv_is_equal(log_level, sv("FETAL"))) {
        params_log_level = LOG_FATAL;
    } else if (strv_is_equal(log_level, sv("ERROR"))) {
        params_log_level = LOG_ERROR;
    } else if (strv_is_equal(log_level, sv("WARNING"))) {
        params_log_level = LOG_WARNING;
    } else if (strv_is_equal(log_level, sv("NOTE"))) {
        params_log_level = LOG_NOTE;
    } else if (strv_is_equal(log_level, sv("INFO"))) {
        params_log_level = LOG_INFO;
    } else if (strv_is_equal(log_level, sv("VERBOSE"))) {
        params_log_level = LOG_VERBOSE;
    } else if (strv_is_equal(log_level, sv("DEBUG"))) {
        params_log_level = LOG_DEBUG;
    } else if (strv_is_equal(log_level, sv("TRACE"))) {
        params_log_level = LOG_TRACE;
    } else {
        msg(DIAG_CMD_OPT_INVALID_OPTION, curr_opt.pos, "log level `"FMT"` is not a supported log level\n", strv_print(log_level));
        local_exit(EXIT_CODE_FAIL);
    }
}

static void long_option_max_errors(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    Strv max_errors = curr_opt.text;
    int64_t result = 0;
    if (!try_strv_to_int64_t(&result, curr_opt.pos, max_errors)) {
        msg(DIAG_CMD_OPT_INVALID_SYNTAX, curr_opt.pos, "expected number after `max-errors=`\n");
        local_exit(EXIT_CODE_FAIL);
    }
    params.max_errors = result;
}

static void long_option_dummy(Pos pos_self, Arg curr_opt) {
    (void) pos_self;
    (void) curr_opt;
}

static_assert(OPT_LEVEL_COUNT == 5, "exhausive handling of opt types in params below");
static_assert(
    PARAMETERS_COUNT == 32,
    "exhausive handling of params (not all parameters are explicitly handled)"
);
    const char* text;
    const char* description;
    Long_option_action action;
    LONG_OPTION_ARG_TYPE arg_type;
Long_option_pair long_options[] = {
    {.text = "help", .description = "display usage", .action = long_option_help, .arg_type = ARG_NONE},
    {.text = "l", .description = "library name to link", .action = long_option_l, .arg_type = ARG_SINGLE},
    {.text = "backend", .description = "c or llvm", .action = long_option_backend, .arg_type = ARG_SINGLE},
    {.text = "all-errors-fetal", .description = "stop immediately after an error occurs", .action = long_option_all_errors_fetal, .arg_type = ARG_NONE},
    {.text = "dump-ir", .description = "stop compiling after IR file(s) have been generated", .action = long_option_dump_ir, .arg_type = ARG_NONE},
    {.text = "dump-backend-ir", .description = "stop compiling after .c file(s) or .ll file(s) have been generated", .action = long_option_dump_backend_ir, .arg_type = ARG_NONE},
    {.text = "S", .description = "stop compiling after assembly file(s) have been generated", .action = long_option_upper_s, .arg_type = ARG_NONE},
    {.text = "c", .description = "stop compiling after object file(s) have been generated", .action = long_option_upper_c, .arg_type = ARG_NONE},
    {.text = "dump-dot", .description = "stop compiling after IR file(s) have been generated, and dump .dot file(s)", .action = long_option_dump_dot, .arg_type = ARG_NONE},
    {.text = "o", .description = "output file path", .action = long_option_lower_o, .arg_type = ARG_SINGLE},
    {.text = "O0", .description = "disable most optimizations", .action = long_option_upper_o0, .arg_type = ARG_NONE},
    {.text = "Og", .description = "enable only optimizations that do not affect debugging", .action = long_option_upper_og, .arg_type = ARG_NONE},
    {.text = "O1", .description = "enable slight optimizations", .action = long_option_upper_o1, .arg_type = ARG_NONE},
    {.text = "O2", .description = "enable optimizations", .action = long_option_upper_o2, .arg_type = ARG_NONE},
    {.text = "Os", .description = "enable O2 optimizations, except those that increase binary sizes", .action = long_option_upper_os, .arg_type = ARG_NONE},
    {.text = "error", .description = "TODO", .action = long_option_error, .arg_type = ARG_SINGLE},
    {
        .text = "print-immediately",
        .description = "print errors immediately. This is intended for debugging the compiler. "
        "This option will cause the error order to be unstable and seemingly random",
        .action = long_option_print_immediately,
        .arg_type = ARG_NONE
    },
    {
        .text = "build-dir",
        .description = "directory to store build artifacts (default is `"DEFAULT_BUILD_DIR"`)",
        .action = long_option_build_dir,
        .arg_type = ARG_SINGLE
    },
    {
        .text = "target-triplet",
        .description = " ARCH-VENDOR-OS-ABI    (eg. \"target-triplet x86_64-unknown-linux-gnu\"",
        .action = long_option_target_triplet,
        .arg_type = ARG_SINGLE
    },
    {
        .text = "path-c-compiler",
        .description = "specify the c compiler to use to compile program",
        .action = long_option_path_c_compiler,
        .arg_type = ARG_SINGLE
    },
    {.text = "no-prelude", .description = "disable the prelude (std::prelude)", .action = long_option_no_prelude, .arg_type = ARG_NONE},
    {
        .text = "set-log-level",
        .description = " OPT where OPT is "
          "\"FETAL\", \"ERROR\", \"WARNING\", \"NOTE\", \"INFO\", \"VERBOSE\", \"DEBUG\", or \"TRACE\" ("
          "eg. \"set-log-level NOTE\" will suppress messages that are less important than \"NOTE\")",
        .action = long_option_log_level,
        .arg_type = ARG_SINGLE
    },
    {
        .text = "max-errors",
        .description = " COUNT where COUNT is the maximum number of errors that should be printed"
        "(eg. \"max-errors 20\" will print a maximum of 20 errors)",
        .action = long_option_max_errors,
        .arg_type = ARG_SINGLE
    },

    {.text = "run", .description = "n/a", .action = long_option_dummy, .arg_type = ARG_REMAINING_RUN_ONLY},
};

static void parse_long_option(int* argc, char*** argv) {
    Arg curr_arg = consume_arg(argc, argv, sv("arg expected"));
    Pos pos_self = curr_arg.pos;

    for (size_t idx = 0; idx < array_count(long_options); idx++) {
        Long_option_pair curr = array_at(long_options, idx);
        if (strv_starts_with(curr_arg.text, sv(curr.text))) {
            arg_consume_count(&curr_arg, sv(curr.text).count);
            static_assert(ARG_COUNT == 3, "exhausive handling of arg types");
            switch (curr.arg_type) {
                case ARG_NONE:
                    if (curr_arg.text.count > 0) {
                        msg(DIAG_CMD_OPT_INVALID_SYNTAX, curr_arg.pos, "expected no arg for `%s`\n", curr.text);
                        local_exit(EXIT_CODE_FAIL);
                    }
                    curr.action(pos_self, curr_arg);
                    return;
                case ARG_SINGLE: {
                    String buf = {0};
                    // TODO: use format in consume_arg.msg_if_missing instead of using local buf
                    string_extend_strv(&a_temp, &buf, sv("argument expected after `"));
                    string_extend_strv(&a_temp, &buf, sv(curr.text));
                    if (curr_arg.text.count < 1) {
                        curr_arg = consume_arg(argc, argv, string_to_strv(buf));
                    }
                    curr.action(pos_self, curr_arg);
                    return;
                }
                case ARG_REMAINING_RUN_ONLY:
                    assert(0 == strcmp(curr.text, "run") && "ARG_REMAINING_RUN_ONLY must only be used for run");
                    long_option_run(pos_self, curr_arg.text, argc, argv);
                    assert(*argc == 0 && "not all args were consumed in long_option_run");
                    return;
                case ARG_COUNT:
                    unreachable("");
                default:
                    unreachable("");
            }
        }
    }

    msg(DIAG_CMD_OPT_INVALID_OPTION, curr_arg.pos, "invalid option: "FMT"\n", strv_print(curr_arg.text));
    local_exit(EXIT_CODE_FAIL);
}

static_assert(
    PARAMETERS_COUNT == 32,
    "exhausive handling of params (not all parameters are explicitly handled)"
);
static void set_params_to_defaults(int argc, char** argv) {
    set_backend(BACKEND_C);
    params.do_prelude = true;
    params.target_triplet = get_default_target_triplet();
    params.max_errors = 30;
    params.build_dir = sv(DEFAULT_BUILD_DIR);

    params.argc = argc;
    params.argv = argv;

#ifdef NDEBUG
    params_log_level = LOG_INFO;
#endif // NDEBUG
}

static void print_usage(Pos pos_help) {
    String buf = {0};

    string_extend_f(&a_temp, &buf, "\n\nusage:\n");
    string_extend_f(&a_temp, &buf, "    "FMT" <files> [options] [--run [subprocess arguments]]\n", strv_print(compiler_exe_name));
    string_extend_f(&a_temp, &buf, "\n");

    string_extend_cstr(&a_temp, &buf, "options:\n");
    for (size_t idx = 0; idx < array_count(long_options); idx++) {
        Long_option_pair curr = array_at(long_options, idx);
        if (!strv_is_equal(sv(curr.text), sv("run"))) {
            string_extend_f(&a_temp, &buf, "    -%s\n", curr.text);
            string_extend_f(&a_temp, &buf, "        %s\n\n", curr.description);
        }
    }

    string_extend_cstr(&a_temp, &buf, "\n\n");
    msg(DIAG_INFO, pos_help, FMT, string_print(buf));
}

void parse_args(int argc, char** argv) {
    set_params_to_defaults(argc, argv);
    expect_fail_str_to_curr_log_level_init();

#   ifndef NDEBUG
        for (size_t last = 0; last < array_count(long_options); last++) {
            for (size_t first = 0; first < last; first++) {
                if (first == last) {
                    continue;
                }

                Strv first_str = sv(array_at(long_options, first).text);
                Strv last_str = sv(array_at(long_options, last).text);
                if (strv_starts_with(first_str, last_str) || strv_starts_with(last_str, first_str)) {
                    log(LOG_FATAL, "options with indices %zu and %zu have overlapping names\n", first, last);
                    local_exit(EXIT_CODE_FAIL);
                }
            }
        }
#   endif // NDEBUG

    // consume compiler executable name
    Arg first_arg = consume_arg(&argc, &argv, sv("internal error"));
    compiler_exe_name = first_arg.text;

    while (argc > 0) {
        if (is_short_option(argv) || is_long_option(argv)) {
            parse_long_option(&argc, &argv);
        } else {
            parse_file_option(&argc, &argv);
        }
    }

    static_assert(
        PARAMETERS_COUNT == 32,
        "exhausive handling of params (not all parameters are explicitly handled)"
    );
    if (
        !params.compile_own &&
        params.static_libs.info.count == 0 &&
        params.c_input_files.info.count == 0 &&
        params.object_files.info.count == 0 &&
        params.lower_s_files.info.count == 0 &&
        params.upper_s_files.info.count == 0
    ) {
        print_usage(first_arg.pos);
        msg(DIAG_NO_INPUT_FILES, first_arg.pos, "no input files were provided\n");
        local_exit(EXIT_CODE_FAIL);
    }

    if (params.compile_own && (params.stop_after == STOP_AFTER_BIN || params.stop_after == STOP_AFTER_RUN)) {
        darr_append(&a_main, &params.c_input_files, sv("std/util.c"));
    }

    // set default output file path
    if (params.output_file_path.count < 1) {
        static_assert(STOP_AFTER_COUNT == 7, "exhausive handling of stop after states");
        switch (params.stop_after) {
            case STOP_AFTER_NONE:
                unreachable("");
            case STOP_AFTER_IR:
                if (params.dump_dot) {
                    todo();
                } else {
                    params.output_file_path = sv("test.ownir");
                }
                break;
            case STOP_AFTER_BACKEND_IR:
                params.output_file_path = sv("test.c");
                break;
            case STOP_AFTER_UPPER_S:
                params.output_file_path = sv("test.s");
                break;
            case STOP_AFTER_OBJ:
                params.output_file_path = sv("test.o");
                break;
            case STOP_AFTER_BIN:
                fallthrough;
            case STOP_AFTER_RUN:
                params.output_file_path = sv("a.out");
                break;
            case STOP_AFTER_COUNT:
                unreachable("");
            default:
                unreachable("");
        }
    } else {
        assert(params.is_output_file_path);
        size_t count_files_compile = 
            params.c_input_files.info.count +
            params.lower_s_files.info.count +
            params.upper_s_files.info.count;
        if (params.compile_own) {
            count_files_compile++;
        }
        if (
            params.stop_after != STOP_AFTER_RUN &&
            params.stop_after != STOP_AFTER_BIN &&
            count_files_compile > 1
        ) {
            msg(
                DIAG_INVALID_O_CMD_OPT,
                params.pos_lower_o,
                "-o <file path> option cannot be used when compiling multiple files to an intermediate step\n"
            );
            local_exit(EXIT_CODE_FAIL);
        }
    }

    Arch_row arch_row = get_arch_row_from_arch(params.target_triplet.arch);
    params.sizeof_usize = arch_row.sizeof_usize;
    params.sizeof_ptr_non_fn = arch_row.sizeof_ptr_non_fn;
    unwrap(
        (size_t)snprintf(params.usize_size_ux, array_count(params.usize_size_ux), "u%u", params.sizeof_usize) <
        array_count(params.usize_size_ux) &&
        "the buffer (params.usize_size_ux) is too small"
    );
    params.sizeof_usize = arch_row.sizeof_usize;

    // TODO: decide best place to do this
    if (!make_dir(params.build_dir)) {
        local_exit(EXIT_CODE_FAIL);
    }
}

